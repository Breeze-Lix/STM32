#ifndef ZDT42_H
#define ZDT42_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum
{
    ZDT42_OK = 0,           /* 执行成功 */
    ZDT42_PARAM_ERROR,      /* 本地参数错误 */
    ZDT42_HAL_ERROR,        /* HAL 收发失败 */
    ZDT42_TIMEOUT,          /* 收发超时 */
    ZDT42_RESPONSE_ERROR,   /* 回复地址、功能码或帧尾不符合要求 */
    ZDT42_MOTOR_ERROR,      /* 电机返回 E2：参数或运行条件不满足 */
    ZDT42_FORMAT_ERROR      /* 电机返回 EE：命令格式错误 */
} ZDT42_Status;

ZDT42_Status ZDT42_Init(UART_HandleTypeDef *huart);
ZDT42_Status ZDT42_ReadVersion(uint8_t id, uint8_t *rx, uint16_t rx_size);
ZDT42_Status ZDT42_Enable(uint8_t id, uint8_t enable);
ZDT42_Status ZDT42_Stop(uint8_t id);
ZDT42_Status ZDT42_SetSpeed(uint8_t id, int16_t rpm, uint8_t accel);

#endif
