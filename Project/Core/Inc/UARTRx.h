#ifndef __UARTRX_H
#define __UARTRX_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>

void     UartRx_Start(void);            /* 启动接收，初始化时调一次 */
uint32_t UartRx_GetLostCount(void);     /* 查询累计丢包数 */
void UartRx_Restart(void);               /* 重启接收数据 */
	
#ifdef __cplusplus
}
#endif

#endif
