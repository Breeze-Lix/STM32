#ifndef __UARTRX_H
#define __UARTRX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 串口编号，数组下标直接用，顺序不能乱 */
typedef enum
{
    UART_PORT_1 = 0,    /* USART1 : PA9  TX / PA10 RX */
    UART_PORT_2,        /* USART2 : PA2  TX / PA3  RX */
    UART_PORT_NUM
} UartPort_t;

void     UartRx_Start(void);                                        /* 启动两路 DMA 接收，初始化后调一次 */
uint16_t UartRx_Read(UartPort_t port, uint8_t *dst, uint16_t max);  /* 取数据，返回实际字节数（0=暂时没有） */
uint32_t UartRx_GetLost(UartPort_t port);                           /* 累计丢包：环形缓冲满了没存下的字节数 */

/* 说明：HAL_UARTEx_RxEventCallback 全工程只有 UARTRx.c 里那一份，
         它按 huart->Instance 自动分发，外面不用管。 */

#ifdef __cplusplus
}
#endif

#endif
