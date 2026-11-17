/* ==================================================================
 *  zdt42.c —— ZDT / X42 步进驱动器驱动层（协议层）
 *
 *  一帧命令长这样（TTL 形态）：
 *
 *      地址 | 功能码 | 命令数据 ... | 校验码
 *      0x05 |  0xF6  |  方向/速度/加速度  |  0x6B
 *
 *  本文件只做三件事：
 *      1. 把"功能码 + 数据"拼成完整帧，加地址和校验发出去
 *      2. 把电机回来的字节收进缓冲，直到总线安静
 *      3. 检查地址、校验，把结果翻译成 Zdt42Status_t
 *
 *  分辨率：X 固件位置单位 0.1°，Emm 固件用脉冲数（默认细分 16 → 3200 脉冲/圈）。
 *          本机实测为 Emm 固件，换算见第 4 步。
 * ================================================================== */

#include "zdt42.h"
#include "UARTRx.h"
#include "usart.h"
#include <string.h>

/* ---------- 本层用的常量 ---------- */
#define ZDT42_PORT            UART_PORT_1   /* 电机挂在 USART1 (PA9/PA10) */
#define ZDT42_FRAME_MAX       32u           /* 一帧最大字节数，位置模式 13 字节够用 */
#define ZDT42_TX_TIMEOUT_MS   50u           /* 发送超时：115200 下 32 字节约 2.8ms */
#define ZDT42_REPLY_TIMEOUT   200u          /* 等第一个字节最多 200ms */
#define ZDT42_GAP_MS          15u           /* 连续静默 15ms 就认为这一帧结束了 */

/* ==================================================================
 *  内部函数
 * ================================================================== */

/**
  * @brief  清空接收缓冲，避免读到上一次残留的字节
  * @note   调试时最怕"收到了上次的回复"，所以每次发命令前先倒干净
  */
static void Zdt42_Drain(void)
{
    uint8_t tmp[16];

    while (UartRx_Read(ZDT42_PORT, tmp, (uint16_t)sizeof(tmp)) > 0u)
    {
        /* 读走丢掉 */
    }
}

/**
  * @brief  发一帧：自动补地址和校验
  * @param  payload  功能码 + 命令数据（不含地址、不含校验）
  * @param  len      payload 的字节数
  */
static Zdt42Status_t Zdt42_SendFrame(const uint8_t *payload, uint16_t len)
{
    uint8_t  frame[ZDT42_FRAME_MAX];
    uint16_t i;

    if ((len == 0u) || ((len + 2u) > ZDT42_FRAME_MAX))
    {
        return ZDT42_ERR_FORMAT;
    }

    frame[0] = ZDT42_ADDR;                       /* 第 1 字节：寄给谁 */
    for (i = 0u; i < len; i++)
    {
        frame[1u + i] = payload[i];              /* 第 2 字节起：功能码 + 数据 */
    }
    frame[1u + len] = ZDT42_CHECKSUM;            /* 最后 1 字节：封口 */

    if (HAL_UART_Transmit(&huart1, frame, (uint16_t)(len + 2u), ZDT42_TX_TIMEOUT_MS) != HAL_OK)
    {
        return ZDT42_ERR_TIMEOUT;
    }

    return ZDT42_OK;
}

/**
  * @brief  收一帧：等到有第一个字节，然后一直收到"总线安静"为止
  * @param  got  实际收到的字节数
  * @note   为什么不按手册写死长度？因为手册是理想值，实际固件版本会差一两个字节
  *         （本机实测读版本只回 5 字节，手册写的是 7 字节）。以实测为准。
  */
static Zdt42Status_t Zdt42_ReadFrame(uint8_t *buf, uint16_t max, uint16_t *got)
{
    uint32_t t0   = HAL_GetTick();
    uint32_t last = t0;
    uint16_t n    = 0u;
    uint8_t  b;

    *got = 0u;

    /* 第一步：等第一个字节，等不到就是超时 */
    while ((HAL_GetTick() - t0) < ZDT42_REPLY_TIMEOUT)
    {
        if (UartRx_Read(ZDT42_PORT, &b, 1u) == 1u)
        {
            buf[n] = b;
            n++;
            last = HAL_GetTick();
            break;
        }
    }
    if (n == 0u)
    {
        return ZDT42_ERR_TIMEOUT;
    }

    /* 第二步：继续收，直到静默超过 GAP 或者缓冲用满 */
    while (n < max)
    {
        if (UartRx_Read(ZDT42_PORT, &b, 1u) == 1u)
        {
            buf[n] = b;
            n++;
            last = HAL_GetTick();
        }
        else if ((HAL_GetTick() - last) >= ZDT42_GAP_MS)
        {
            break;
        }
    }

    *got = n;
    return ZDT42_OK;
}

/* ==================================================================
 *  对外接口
 * ================================================================== */

/**
  * @brief  初始化驱动：清空接收缓冲
  * @note   串口本身由 MX_USART1_UART_Init() + UartRx_Start() 配好，这里不重复配
  */
void Zdt42_Init(void)
{
    Zdt42_Drain();
}

/**
  * @brief  读固件版本和硬件版本（最短的一条命令，最适合验证链路）
  * @param  reply  收到的完整回复帧
  * @param  max    reply 缓冲大小
  * @param  got    实际收到的字节数
  */
Zdt42Status_t Zdt42_ReadVersion(uint8_t *reply, uint16_t max, uint16_t *got)
{
    uint8_t       payload[1];
    Zdt42Status_t st;

    payload[0] = ZDT42_FUNC_VERSION;

    Zdt42_Drain();                                   /* 先倒干净，防止读到上一次的回复 */
    st = Zdt42_SendFrame(payload, 1u);
    if (st != ZDT42_OK)
    {
        return st;
    }

    st = Zdt42_ReadFrame(reply, max, got);
    if (st != ZDT42_OK)
    {
        return st;
    }

    /* 结构检查：地址对不对、有没有校验字节、长度够不够
       注意：读版本命令不带 02 状态码，第 3 字节是固件版本高字节，
             所以这里不能套用"回复码 == 0x02"的判断 */
    if ((*got < 4u) ||
        (reply[0] != ZDT42_ADDR) ||
        (reply[1] != ZDT42_FUNC_VERSION) ||
        (reply[*got - 1u] != ZDT42_CHECKSUM))
    {
        return ZDT42_ERR_FORMAT;
    }

    return ZDT42_OK;
}

/**
  * @brief  把返回值翻译成人话，调试打印用
  */
const char *Zdt42_StatusStr(Zdt42Status_t st)
{
    switch (st)
    {
        case ZDT42_OK:          return "OK";
        case ZDT42_ERR_TIMEOUT: return "TIMEOUT (no reply)";
        case ZDT42_ERR_ACK:     return "NACK (motor rejected)";
        case ZDT42_ERR_FORMAT:  return "BAD FRAME";
        default:                return "UNKNOWN";
    }
}
