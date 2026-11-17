// ##---------环形缓冲区代码-----------##
// ** 可直接引用

#include "Ring.h"

#define RING_MASK   (RING_BUF_SIZE - 1u)

static uint8_t  s_buf[RING_BUF_SIZE];       /* static：外部看不到 */
static volatile uint16_t s_head;            /* 写：中断里改 */
static volatile uint16_t s_tail;            /* 读：主循环改 */

void Ring2_Clear(void)
{
    s_head = 0u;
    s_tail = 0u;
}

uint8_t Ring2_Write(uint8_t byte)
{
    uint16_t next = (uint16_t)((s_head + 1u) & RING_MASK);

    if (next == s_tail) return 0u;

    s_buf[s_head] = byte;
    s_head = next;
    return 1u;
}

int16_t Ring2_Read(void)
{
    uint8_t byte;

    if (s_tail == s_head) return -1;

    byte = s_buf[s_tail];
    s_tail = (uint16_t)((s_tail + 1u) & RING_MASK);
    return (int16_t)byte;
}

uint16_t Ring2_Count(void)
{
    return (uint16_t)((s_head + RING_BUF_SIZE - s_tail) & RING_MASK);
}

uint16_t Ring2_Free(void)
{
    return (uint16_t)((RING_BUF_SIZE - 1u) - Ring_Count());
}
