// ##---------环形缓冲区代码-----------##
// ** 可直接引用

#include "Ring.h"

#define RING_MASK   (RING_BUF_SIZE - 1u)

static uint8_t  s_buf[RING_BUF_SIZE];       /* static：外部看不到 */
static volatile uint16_t s_head;            /* 写：中断里改 */
static volatile uint16_t s_tail;            /* 读：主循环改 */

// 缓冲区清理
void Ring_Clear(void)
{
    s_head = 0u;
    s_tail = 0u;
}

// 将字节写进缓冲区
uint8_t Ring_Write(uint8_t byte)
{
    uint16_t next = (uint16_t)((s_head + 1u) & RING_MASK);

    if (next == s_tail) return 0u;

    s_buf[s_head] = byte;
    s_head = next;
    return 1u;
}

// 读取缓冲区字节并处理
int16_t Ring_Read(void)
{
    uint8_t byte;

    if (s_tail == s_head) return -1;

    byte = s_buf[s_tail];
    s_tail = (uint16_t)((s_tail + 1u) & RING_MASK);
    return (int16_t)byte;
}

// 还有多少个字节待处理
uint16_t Ring_Count(void)
{
    return (uint16_t)((s_head + RING_BUF_SIZE - s_tail) & RING_MASK);
}

// 还剩几个空位
uint16_t Ring_Free(void)
{
    return (uint16_t)((RING_BUF_SIZE - 1u) - Ring_Count());
}

// 只读i个字节
// 调用前先 Ring_Count() >= 11 判断，是否返回假的0 (保证 i < Count)
uint8_t Ring_Peek(uint16_t i)
{
    if (i >= Ring_Count())
    {
        return 0u;
    }
    return s_buf[(uint16_t)((s_tail + i) & RING_MASK)];
}

// 一次性丢n个字节
void Ring_Drop(uint16_t n)
{
    uint16_t cnt = Ring_Count();

    if (n > cnt)
    {
        n = cnt;                     /* 不能丢得比现有的还多 */
    }
    s_tail = (uint16_t)((s_tail + n) & RING_MASK);
}
