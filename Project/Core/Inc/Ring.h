#ifndef __RING_H
#define __RING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------
 * 通用环形缓冲（可以按需建多个实例，不用复制代码）
 *
 * 用法：
 *      Ring_t ring;
 *      uint8_t storage[128];
 *      Ring_Init(&ring, storage, sizeof(storage));
 *
 * 注意：size 必须是 2 的幂（用位与代替取模，省时间）
 *
 * 读写分别在两边：中断里只调 Write，主循环里只调 Read，
 * 这样单生产者 / 单消费者不需要关中断。
 * ------------------------------------------------------------------ */
typedef struct
{
    uint8_t          *buf;      /* 存储区（外部提供） */
    uint16_t          size;     /* 存储区大小，2 的幂 */
    uint16_t          mask;     /* size - 1 */
    volatile uint16_t head;     /* 写指针：只在中断里改 */
    volatile uint16_t tail;     /* 读指针：只在主循环里改 */
} Ring_t;

void     Ring_Init(Ring_t *r, uint8_t *storage, uint16_t size);
void     Ring_Clear(Ring_t *r);                                 /* 清空 */
uint8_t  Ring_Write(Ring_t *r, uint8_t byte);                   /* 写 1 字节，1=成功 0=满 */
int16_t  Ring_Read(Ring_t *r);                                  /* 读 1 字节，-1=空 */
uint16_t Ring_ReadBlock(Ring_t *r, uint8_t *dst, uint16_t max); /* 批量读，返回实际字节数 */
uint16_t Ring_Count(const Ring_t *r);                           /* 还有多少字节待处理 */
uint16_t Ring_Free(const Ring_t *r);                            /* 还能写多少字节 */

#ifdef __cplusplus
}
#endif

#endif
