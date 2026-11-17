/* ==================================================================
 *  zdt42.h —— ZDT / X42 步进驱动器的驱动层（协议层）头文件
 *
 *  分层关系：
 *
 *      应用层（main.c / 后续的任务）
 *          |  只传物理量：rpm、度、使能/停止
 *      zdt42.c（本层：拼帧、校验、解析回复）
 *          |
 *      UARTRx.c（收发层：DMA + 空闲中断 + 环形缓冲）
 *
 *  规矩：十六进制命令只允许出现在 zdt42.c 里，上层看不到一个字节。
 * ================================================================== */

#ifndef __ZDT42_H
#define __ZDT42_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ---------- 设备参数：换电机 / 改编号，只改这两行 ---------- */
#define ZDT42_ADDR              0x05u   /* 电机 ID，屏幕菜单 ID_Addr 里那个数 */
#define ZDT42_CHECKSUM          0x6Bu   /* 固定校验字节，出厂默认 0x6B */

/* ---------- 功能码（帧里第 2 个字节，决定"要干什么"） ---------- */
#define ZDT42_FUNC_ENABLE       0xF3u   /* 使能 / 失能 */
#define ZDT42_FUNC_STOP         0xFEu   /* 立即停止 */
#define ZDT42_FUNC_SPEED        0xF6u   /* 速度模式 */
#define ZDT42_FUNC_POSITION     0xFDu   /* 位置模式 */
#define ZDT42_FUNC_READ_POS     0x36u   /* 读实时位置 */
#define ZDT42_FUNC_READ_SPEED   0x35u   /* 读实时转速 */
#define ZDT42_FUNC_VERSION      0x1Fu   /* 读固件 / 硬件版本 */

/* ---------- 回复码（"纯确认类"命令的回复帧第 3 字节） ---------- */
#define ZDT42_ACK_OK            0x02u   /* 命令正确 */
#define ZDT42_ACK_PARAM         0xE2u   /* 参数越界 / 保护中 */
#define ZDT42_ACK_FORMAT        0xEEu   /* 帧格式错（多半是固件版本或长度不对） */
#define ZDT42_ACK_DONE          0x9Fu   /* 动作执行完成（电机主动上报） */

/* ---------- 驱动返回值 ---------- */
typedef enum
{
    ZDT42_OK = 0,        /* 成功 */
    ZDT42_ERR_TIMEOUT,   /* 等不到回复（链路 / 供电 / 地址 / 波特率） */
    ZDT42_ERR_ACK,       /* 收到了，但电机说这帧不合法 */
    ZDT42_ERR_FORMAT     /* 收到了，但帧结构不对（地址、校验、长度） */
} Zdt42Status_t;

/* ---------- 对外接口 ---------- */
void          Zdt42_Init(void);
Zdt42Status_t Zdt42_ReadVersion(uint8_t *reply, uint16_t max, uint16_t *got);
const char   *Zdt42_StatusStr(Zdt42Status_t st);

#ifdef __cplusplus
}
#endif

#endif /* __ZDT42_H */
