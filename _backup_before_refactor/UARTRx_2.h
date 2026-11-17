#ifndef __UARTRX_H
#define __UARTRX_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>

void     UartRx2_Start(void);            /* 启动接收，初始化时调一次 */
uint32_t UartRx2_GetLostCount(void);     /* 查询累计丢包数 */
	
#ifdef __cplusplus
}
#endif

#endif
