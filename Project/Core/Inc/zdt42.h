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

/**
 * @brief  初始化驱动并绑定 UART。
 * @param  huart：指向已初始化的 UART 句柄，例如 &huart1。
 * @return ZDT42_OK 表示成功；ZDT42_PARAM_ERROR 表示传入空指针。
 */
ZDT42_Status ZDT42_Init(UART_HandleTypeDef *huart);

/**
 * @brief  读取电机版本信息，不控制电机运动。
 * @param  id：电机地址，例如 0x05。
 * @param  rx：用于保存原始回复的数组。
 * @param  rx_size：数组长度，当前老板卡要求至少为 5。
 * @return 返回 ZDT42_Status；成功时 rx 中保存 5 字节版本回复。
 */
ZDT42_Status ZDT42_ReadVersion(uint8_t id, uint8_t *rx, uint16_t rx_size);

/**
 * @brief  控制电机使能或失能。
 * @param  id：电机地址，例如 0x05。
 * @param  enable：1 = 使能并锁轴；0 = 失能并松轴。
 * @return 返回 ZDT42_Status；ZDT42_OK 表示电机已正确接收命令。
 */
ZDT42_Status ZDT42_Enable(uint8_t id, uint8_t enable);

/**
 * @brief  立即停止电机运动；停止后仍可能保持使能锁轴。
 * @param  id：电机地址，例如 0x05。
 * @return 返回 ZDT42_Status。
 */
ZDT42_Status ZDT42_Stop(uint8_t id);

/**
 * @brief  设置 Emm 速度模式的转速。
 * @param  id：电机地址，例如 0x05。
 * @param  rpm：转速，正数正转、负数反转；范围为 ±1～±3000 RPM。
 *              rpm = 0 不被接受，需要停止时调用 ZDT42_Stop()。
 * @param  accel：加速度档位 0～255；0 为直接启动，数值越大加速越快。
 * @return 返回 ZDT42_Status。
 */
ZDT42_Status ZDT42_SetSpeed(uint8_t id, int16_t rpm, uint8_t accel);

/**
 * @brief  执行相对位置运动，使用 Emm42 V5.0 的模式 00。
 * @param  id：电机地址，例如 0x05。
 * @param  angle_deg：相对转角，单位为整数度；正数正转、负数反转。
 * @param  rpm：运动速度，范围为 1～3000 RPM，只填写正数。
 * @param  accel：加速度档位 0～255。
 * @return 返回 ZDT42_Status；收到确认不代表已经运动到位。
 * @note   默认按 1.8° 电机、16 细分，即 3200 脉冲/圈换算；
 *         模式 00 相对上一输入目标位置运动。
 */
ZDT42_Status ZDT42_MoveRelative(uint8_t id, int32_t angle_deg,
                                uint16_t rpm, uint8_t accel);

/**
 * @brief  转到绝对坐标位置，使用模式 01。
 * @param  id：电机地址，例如 0x05。
 * @param  angle_deg：相对坐标零点的目标角度，单位为整数度。
 * @param  rpm：运动速度，范围为 1～3000 RPM，只填写正数。
 * @param  accel：加速度档位 0～255。
 * @return 返回 ZDT42_Status；收到确认不代表已经运动到位。
 * @note   默认按 1.8° 电机、16 细分，即 3200 脉冲/圈换算。
 */
ZDT42_Status ZDT42_MoveAbsolute(uint8_t id, int32_t angle_deg,
                                uint16_t rpm, uint8_t accel);

#endif
