/* ==================================================================
 *  UARTRx.c  —— 串口 DMA + 空闲中断接收（USART1 / USART2 共用一套代码）
 *
 *  数据流：
 *
 *     USARTx --DMA(循环)--> dma_buf --(IDLE/HT/TC 中断)--> 环形缓冲 --> 主循环取走
 *
 *  两个关键点：
 *   1. HAL_UARTEx_RxEventCallback 全工程只能有一份（HAL 里是 __weak 空函数，
 *      你写一份同名的强定义就覆盖它）。所以这里按 huart->Instance 分发，
 *      而不是每个串口写一个同名函数。
 *   2. 中断里只做"搬字节"，解析和发送都放到主循环，中断要尽量短。
 * ================================================================== */

#include "UARTRx.h"
#include "Ring.h"
#include "usart.h"

#define UART_RX_DMA_SIZE    64u     /* DMA 循环缓冲，必须是 2 的幂 */
#define UART_RX_RING_SIZE   256u    /* 环形缓冲，必须是 2 的幂 */

/* 每个串口一份上下文，状态都放结构体里，所以一套代码能管两个串口 */
typedef struct
{
    uint8_t           dma_buf[UART_RX_DMA_SIZE];      /* DMA 直接往里写 */
    volatile uint16_t dma_pos;                        /* 上一轮已经取到哪个位置 */
    uint8_t           ring_buf[UART_RX_RING_SIZE];    /* 环形缓冲的存储区 */
    Ring_t            ring;                           /* 环形缓冲实例 */
    volatile uint32_t lost;                           /* 环形缓冲满而丢掉的字节数 */
} UartRxCtx_t;

/* 串口句柄表：下标就是 UartPort_t */
static UART_HandleTypeDef *const s_huart[UART_PORT_NUM] = { &huart1, &huart2 };

static UartRxCtx_t s_ctx[UART_PORT_NUM];

//  ====================== 内部函数 ===========================

/**
  * @brief  把 DMA 缓冲里新到的数据搬进环形缓冲
  * @param  c    串口上下文
  * @param  pos  本轮 DMA 写指针（绝对位置，0 ~ UART_RX_DMA_SIZE）
  * @note   pos 不是"这一轮的长度"，是"从缓冲区开头算写到哪了"，
  *         所以要用和上次的差来算长度，并且要处理绕回。
  */
static void UartRx_PushNewData(UartRxCtx_t *c, uint16_t pos)
{
    uint16_t len;
    uint16_t i;
    uint16_t idx;

    if (pos > UART_RX_DMA_SIZE) pos = UART_RX_DMA_SIZE;   /* 防御，正常用不上 */

    /* 这一段有多长：没绕回就是差值，绕回了就是"剩下的一截 + 新的一截" */
    if (pos >= c->dma_pos)
    {
        len = (uint16_t)(pos - c->dma_pos);
    }
    else
    {
        len = (uint16_t)(UART_RX_DMA_SIZE - c->dma_pos + pos);
    }

    for (i = 0u; i < len; i++)
    {
        idx = (uint16_t)((c->dma_pos + i) & (UART_RX_DMA_SIZE - 1u));

        if (Ring_Write(&c->ring, c->dma_buf[idx]) == 0u)
        {
            c->lost++;          /* 环形缓冲满了，这个字节只能丢 */
        }
    }

    c->dma_pos = (uint16_t)(pos & (UART_RX_DMA_SIZE - 1u));
}

//  ====================== 对外函数 ===========================

/**
  * @brief  启动两路串口的 DMA 空闲中断接收，初始化完调一次
  */
void UartRx_Start(void)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)UART_PORT_NUM; i++)
    {
        s_ctx[i].dma_pos = 0u;
        s_ctx[i].lost    = 0u;
        Ring_Init(&s_ctx[i].ring, s_ctx[i].ring_buf, UART_RX_RING_SIZE);

        /* 注意顺序：先把环形缓冲准备好，再开接收，否则第一包可能写进未初始化的结构 */
        if (HAL_UARTEx_ReceiveToIdle_DMA(s_huart[i], s_ctx[i].dma_buf, UART_RX_DMA_SIZE) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

/**
  * @brief  从环形缓冲取数据（主循环里调，可以反复调到返回 0 为止）
  */
uint16_t UartRx_Read(UartPort_t port, uint8_t *dst, uint16_t max)
{
    if ((port >= UART_PORT_NUM) || (dst == 0) || (max == 0u)) return 0u;

    return Ring_ReadBlock(&s_ctx[port].ring, dst, max);
}

/**
  * @brief  查询丢包数
  */
uint32_t UartRx_GetLost(UartPort_t port)
{
    if (port >= UART_PORT_NUM) return 0u;

    return s_ctx[port].lost;
}

//  ====================== HAL 回调 ===========================

/**
  * @brief  接收事件回调（空闲 IDLE / 半满 HT / 全满 TC 都会进来）
  * @param  Size  本轮 DMA 写指针的位置，不是长度
  * @note   全工程只此一份，靠 Instance 区分是哪个串口
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)UART_PORT_NUM; i++)
    {
        if (s_huart[i]->Instance == huart->Instance)
        {
            UartRx_PushNewData(&s_ctx[i], Size);
            return;
        }
    }
}

/**
  * @brief  串口出错回调（溢出 ORE / 帧错误 FE / 噪声 NE）
  * @note   DMA 模式下 HAL 把任何错误都当"阻塞错误"，会直接终止接收。
  *         不在这里重启的话，串口会"莫名其妙再也不收数据"了。
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)UART_PORT_NUM; i++)
    {
        if (s_huart[i]->Instance == huart->Instance)
        {
            s_ctx[i].dma_pos = 0u;      /* 重新从缓冲区开头收 */
            (void)HAL_UARTEx_ReceiveToIdle_DMA(s_huart[i], s_ctx[i].dma_buf, UART_RX_DMA_SIZE);
            return;
        }
    }
}
