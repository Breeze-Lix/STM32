#include "Ring.h"
#include "UARTRx.h"
#include "usart.h" 

// ---------- 用来记录搬运JY60 -------------
#define DMA_BUF_SIZE   64u

static uint8_t  s_dma_buf[DMA_BUF_SIZE];
static uint16_t s_dma_pos;
static volatile uint32_t s_lost; // 累计丢包数

//  ====================== 函数 ===========================

// *** 外部串口初始化接收数据
void UartRx_Start(void)
{
    s_dma_pos = 0u;
   // (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_dma_buf, DMA_BUF_SIZE);
	
	if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_dma_buf, DMA_BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

// *** 计算是否数据丢包
static void UartRx_PushByte(uint8_t b)
{
    if (Ring_Write(b) == 0u)
    {
        s_lost++;               /* 缓冲区满了，这个字节丢了 */
    }
}

// *** DMA中断接收数据并写入
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	// USART1 的数据
	uint16_t pos = Size;							/* 当前写指针位置，不是长度 */
	uint16_t i;
	
	// UARST1 部分
	if(huart->Instance == USART1)
	{
		if( pos > DMA_BUF_SIZE ) pos = DMA_BUF_SIZE; // 防止数据出bug（基本用不上）
		if( pos == s_dma_pos ) return; // 没有新数据
		
		if( pos > s_dma_pos ) // 如果没绕回
		{
			for( i = s_dma_pos ; i < pos ; i++ )
			{
				UartRx_PushByte(s_dma_buf[i]);
			}
		}else{                 // 如果绕回
			
			for( i = s_dma_pos ; i < DMA_BUF_SIZE ; i++ )
			{
				UartRx_PushByte(s_dma_buf[i]);
			}
			
			for( i = 0u ; i < pos ; i++)
			{
				UartRx_PushByte(s_dma_buf[i]);
			}
		}
		
		s_dma_pos = ( pos >= DMA_BUF_SIZE ) ? 0u : pos ;
	}
	return;
}

// *** 查询丢包数
uint32_t UartRx_GetLostCount(void)
{
    return s_lost;
}

// *** 重启数据接收
void UartRx_Restart(void)
{
	(void)HAL_UART_AbortReceive(&huart1);    /* 先释放，否则重挂会返回 BUSY */
    UartRx_Start();
}
