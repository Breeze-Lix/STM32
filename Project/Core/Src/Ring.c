// ##---------环形缓冲区代码-----------##
// ** 一套代码，多个实例（把状态放进 Ring_t 里，不再靠复制文件）

#include "Ring.h"

void Ring_Init(Ring_t *r, uint8_t *storage, uint16_t size)
{
    r->buf  = storage;
    r->size = size;
    r->mask = (uint16_t)(size - 1u);
    r->head = 0u;
    r->tail = 0u;
}

void Ring_Clear(Ring_t *r)
{
    r->head = 0u;
    r->tail = 0u;
}

uint8_t Ring_Write(Ring_t *r, uint8_t byte)
{
    uint16_t next = (uint16_t)((r->head + 1u) & r->mask);

    if (next == r->tail) return 0u;     /* 满了，留一个空位区分空/满 */

    r->buf[r->head] = byte;
    r->head = next;
    return 1u;
}

int16_t Ring_Read(Ring_t *r)
{
    uint8_t byte;

    if (r->tail == r->head) return -1;  /* 空 */

    byte = r->buf[r->tail];
    r->tail = (uint16_t)((r->tail + 1u) & r->mask);
    return (int16_t)byte;
}

uint16_t Ring_ReadBlock(Ring_t *r, uint8_t *dst, uint16_t max)
{
    uint16_t n = 0u;
    int16_t  b;

    while (n < max)
    {
        b = Ring_Read(r);
        if (b < 0) break;
        dst[n] = (uint8_t)b;
        n++;
    }
    return n;
}

uint16_t Ring_Count(const Ring_t *r)
{
    return (uint16_t)((r->head + r->size - r->tail) & r->mask);
}

uint16_t Ring_Free(const Ring_t *r)
{
    return (uint16_t)((r->size - 1u) - Ring_Count(r));
}
