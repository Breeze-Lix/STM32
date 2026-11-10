#include "Motor.h"
#include "tim.h"
#include "main.h"

/* ---------------- TB6612 引脚 ---------------- */
#define AIN1_PORT      GPIOB
#define AIN1_PIN       GPIO_PIN_0
#define AIN2_PORT      GPIOB
#define AIN2_PIN       GPIO_PIN_1
#define STBY_PORT      GPIOB
#define STBY_PIN       GPIO_PIN_5

/* ---------------- 常量 ---------------- */
#define PWM_FULL       3600.0f      /* TIM4 ARR + 1 */

/* 输出轴每转计数（4 倍频）
 * 理论：11 PPR × 1:30 × 4 = 1320
 * 实测：手动转 10 圈，约 1320 —— 与理论一致
 */
#define ENCODER_CPR    1320.0f

/* 控制周期，必须和 pidTask 的 osDelay 一致 */
#define CTRL_DT        0.01f

static int16_t s_last_cnt;       /* 上次计数器的值 */
static float   s_speed_filt;     /* 滤波后的速度 */

void Motor_Init(void)
{
	/* PWM 使能 */
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	
	/* 方向使能 */
	HAL_GPIO_WritePin(STBY_PORT, STBY_PIN, GPIO_PIN_SET);
	
	/* 编码器使能 */
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	s_last_cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
	
	/* 滤波初始化 */
	s_speed_filt = 0.0f;
}

void Motor_SetOutput(float out)
{
	uint16_t pwm;
	
	/* 输入限幅 */
	if(out > 100.0f) out =  100.0f;
	if(out < -100.0f)out = -100.0f;
	
	if(out >=0.0f)
	{
		HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_RESET);
		pwm = (uint16_t)(out * PWM_FULL / 100.0f);
	}else{
		HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_SET);
		pwm = (uint16_t)(-out * PWM_FULL / 100.0f);
	}
	
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pwm);
}

float Motor_GetSpeed(void)
{
	int16_t now = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
	int16_t delta = now - s_last_cnt;						/* int16 减法自动处理回绕 */
	s_last_cnt = now;
	
	float raw = (float)delta / ENCODER_CPR / CTRL_DT * 60.0f;
	
	/* 滤波 */
	s_speed_filt += 0.1 * (raw - s_speed_filt); 
	
	/* 转速(rpm) = 圈数 / 时间 × 60 */
	return s_speed_filt;
}
