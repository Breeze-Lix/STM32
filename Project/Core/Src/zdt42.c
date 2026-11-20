#include "zdt42.h"
#include <stddef.h>

#define ZDT42_CHECK_BYTE         0x6BU				/* 数据检查位 */
#define ZDT42_TX_TIMEOUT_MS      100U					/* 数据接送时限 */
#define ZDT42_RX_TIMEOUT_MS      500U					/* 数据接收时限 */
#define ZDT42_VERSION_REPLY_LEN  5U						/* 电机回复长度 */
#define ZDT42_MAX_SPEED_RPM      3000					/* Emm 速度模式最大转速 */
#define ZDT42_PULSES_PER_REV     3200ULL				/* 默认 1.8° 电机、16 细分 */

static UART_HandleTypeDef *zdt42_uart = NULL;     /* 步进电机绑定串口 */

/* ============ 把 HAL 状态转换成驱动状态 ================ */
static ZDT42_Status ZDT42_FromHal(HAL_StatusTypeDef result)  
{
    if (result == HAL_OK)
    {
        return ZDT42_OK;
    }
    if (result == HAL_TIMEOUT)
    {
        return ZDT42_TIMEOUT;
    }
    return ZDT42_HAL_ERROR;
}

/* =============== 保存要使用的 UART 句柄 ================ */
ZDT42_Status ZDT42_Init(UART_HandleTypeDef *huart)						
{
    if (huart == NULL)
    {
        return ZDT42_PARAM_ERROR;
    }

    zdt42_uart = huart;
    return ZDT42_OK;
}

/* =============== 内部发送函数 =================== */
static ZDT42_Status ZDT42_Send(const uint8_t *data, uint16_t length)  
{
    if ((zdt42_uart == NULL) || (data == NULL) || (length == 0U))
    {
        return ZDT42_PARAM_ERROR;
    }

    return ZDT42_FromHal(
        HAL_UART_Transmit(zdt42_uart, (uint8_t *)data, length, ZDT42_TX_TIMEOUT_MS));
}

/* ================ 内部接收函数 =================== */
static ZDT42_Status ZDT42_Receive(uint8_t *data, uint16_t length)
{
    if ((zdt42_uart == NULL) || (data == NULL) || (length == 0U))
    {
        return ZDT42_PARAM_ERROR;
    }

    return ZDT42_FromHal(
        HAL_UART_Receive(zdt42_uart, data, length, ZDT42_RX_TIMEOUT_MS));
}

/* =============== 发送控制命令并检查 4 字节回复 =============== */
static ZDT42_Status ZDT42_ControlCommand(const uint8_t *tx,
                                         uint16_t tx_length,
                                         uint8_t id,
                                         uint8_t function_code)
{
    uint8_t rx[4] = {0};
    ZDT42_Status status;

    if (id == 0U)
    {
        return ZDT42_PARAM_ERROR;
    }

    status = ZDT42_Send(tx, tx_length);
    if (status != ZDT42_OK)
    {
        return status;
    }

    status = ZDT42_Receive(rx, sizeof(rx));
    if (status != ZDT42_OK)
    {
        return status;
    }

    if ((rx[0] != id) || (rx[3] != ZDT42_CHECK_BYTE))
    {
        return ZDT42_RESPONSE_ERROR;
    }

    if (rx[2] == 0xEEU)
    {
        return ZDT42_FORMAT_ERROR;
    }
    if (rx[2] == 0xE2U)
    {
        return ZDT42_MOTOR_ERROR;
    }
    if ((rx[1] != function_code) || (rx[2] != 0x02U))
    {
        return ZDT42_RESPONSE_ERROR;
    }

    return ZDT42_OK;
}

/* =============== 电机使能：enable 为 1 使能，为 0 失能 =============== */
ZDT42_Status ZDT42_Enable(uint8_t id, uint8_t enable)
{
    uint8_t tx[6];

    if (enable > 1U)
    {
        return ZDT42_PARAM_ERROR;
    }

    tx[0] = id;
    tx[1] = 0xF3U;
    tx[2] = 0xABU;
    tx[3] = enable;
    tx[4] = 0x00U;
    tx[5] = ZDT42_CHECK_BYTE;

    return ZDT42_ControlCommand(tx, sizeof(tx), id, 0xF3U);
}

/* =============== 立即停止 =============== */
ZDT42_Status ZDT42_Stop(uint8_t id)
{
    uint8_t tx[5] = {id, 0xFEU, 0x98U, 0x00U, ZDT42_CHECK_BYTE};

    return ZDT42_ControlCommand(tx, sizeof(tx), id, 0xFEU);
}

/* =============== 速度模式：正数正转，负数反转 =============== */
ZDT42_Status ZDT42_SetSpeed(uint8_t id, int16_t rpm, uint8_t accel)
{
    uint8_t tx[8];
    uint16_t speed;
    uint8_t direction;

    if ((rpm == 0) || (rpm > ZDT42_MAX_SPEED_RPM) ||
        (rpm < -ZDT42_MAX_SPEED_RPM))
    {
        return ZDT42_PARAM_ERROR;
    }

    if (rpm < 0)
    {
        direction = 0x01U;
        speed = (uint16_t)(-(int32_t)rpm);
    }
    else
    {
        direction = 0x00U;
        speed = (uint16_t)rpm;
    }

    tx[0] = id;
    tx[1] = 0xF6U;
    tx[2] = direction;
    tx[3] = (uint8_t)(speed >> 8);
    tx[4] = (uint8_t)(speed & 0xFFU);
    tx[5] = accel;
    tx[6] = 0x00U;
    tx[7] = ZDT42_CHECK_BYTE;

    return ZDT42_ControlCommand(tx, sizeof(tx), id, 0xF6U);
}

/* =============== 位置模式内部组帧函数 =============== */
static ZDT42_Status ZDT42_MovePosition(uint8_t id,
                                       int32_t angle_deg,
                                       uint16_t rpm,
                                       uint8_t accel,
                                       uint8_t mode)
{
    uint8_t tx[13];
    uint64_t angle_magnitude;
    uint64_t pulses;
    uint32_t pulse_count;
    uint8_t direction;

    if ((rpm == 0U) || (rpm > ZDT42_MAX_SPEED_RPM))
    {
        return ZDT42_PARAM_ERROR;
    }

    if (angle_deg < 0)
    {
        direction = 0x01U;
        angle_magnitude = (uint64_t)(-(int64_t)angle_deg);
    }
    else
    {
        direction = 0x00U;
        angle_magnitude = (uint64_t)angle_deg;
    }

    /* 四舍五入到最接近的脉冲数。 */
    pulses = (angle_magnitude * ZDT42_PULSES_PER_REV + 180ULL) / 360ULL;
    if (pulses > 0xFFFFFFFFULL)
    {
        return ZDT42_PARAM_ERROR;
    }
    pulse_count = (uint32_t)pulses;

    tx[0] = id;
    tx[1] = 0xFDU;
    tx[2] = direction;
    tx[3] = (uint8_t)(rpm >> 8);
    tx[4] = (uint8_t)(rpm & 0xFFU);
    tx[5] = accel;
    tx[6] = (uint8_t)(pulse_count >> 24);
    tx[7] = (uint8_t)(pulse_count >> 16);
    tx[8] = (uint8_t)(pulse_count >> 8);
    tx[9] = (uint8_t)(pulse_count & 0xFFU);
    tx[10] = mode;
    tx[11] = 0x00U;
    tx[12] = ZDT42_CHECK_BYTE;

    return ZDT42_ControlCommand(tx, sizeof(tx), id, 0xFDU);
}

/* =============== 相对位置：Emm42 V5.0 使用模式 00 =============== */
ZDT42_Status ZDT42_MoveRelative(uint8_t id, int32_t angle_deg,
                                uint16_t rpm, uint8_t accel)
{
    return ZDT42_MovePosition(id, angle_deg, rpm, accel, 0x00U);
}

/* =============== 绝对位置：相对坐标零点运动，使用模式 01 =============== */
ZDT42_Status ZDT42_MoveAbsolute(uint8_t id, int32_t angle_deg,
                                uint16_t rpm, uint8_t accel)
{
    return ZDT42_MovePosition(id, angle_deg, rpm, accel, 0x01U);
}

/* =============== 发送读取版本命令并检查回复 =============== */
ZDT42_Status ZDT42_ReadVersion(uint8_t id, uint8_t *rx, uint16_t rx_size)
{
    uint8_t tx[3] = {id, 0x1FU, ZDT42_CHECK_BYTE};
    ZDT42_Status status;

    if ((id == 0U) || (rx == NULL) || (rx_size < ZDT42_VERSION_REPLY_LEN))
    {
        return ZDT42_PARAM_ERROR;
    }

    status = ZDT42_Send(tx, sizeof(tx));
    if (status != ZDT42_OK)
    {
        return status;
    }

    status = ZDT42_Receive(rx, ZDT42_VERSION_REPLY_LEN);
    if (status != ZDT42_OK)
    {
        return status;
    }

    if ((rx[0] != id) || (rx[1] != 0x1FU) ||
        (rx[ZDT42_VERSION_REPLY_LEN - 1U] != ZDT42_CHECK_BYTE))
    {
        return ZDT42_RESPONSE_ERROR;
    }

    return ZDT42_OK;
}
