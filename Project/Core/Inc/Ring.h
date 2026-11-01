#ifndef __RING_H
#define __RING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 缓冲区大小，必须是 2 的幂（减少内存运行） */
#define RING_BUF_SIZE   128u

void     Ring_Clear(void);              /* 清空 */
uint8_t  Ring_Write(uint8_t byte);      /* 写一个字节，1=成功 0=满 */
int16_t  Ring_Read(void);               /* 读一个字节，-1=空 */
uint16_t Ring_Count(void);              /* 还有多少字节待处理 */
uint16_t Ring_Free(void);               /* 还能写多少字节 */
uint8_t  Ring_Peek(uint16_t i);					/* 只看i个字节 */
void     Ring_Drop(uint16_t n);					/* 一次性丢n个字节*/

#ifdef __cplusplus
}
#endif
	
#endif
