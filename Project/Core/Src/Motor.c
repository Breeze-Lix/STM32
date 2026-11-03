#include "Motor.h"

static float s_speed;      /* 当前转速 rpm */
static float s_out;        /* 上一次的输出 */

void Motor_Init(void)
{
	s_speed = 0.0f;
	s_out = 0.0f;
}

void Motor_SetOutput(float out)
{
	s_out = out;
}

float Motor_GetSpeed(void)
{
	/* ----- 仿真计算 ----- */
	const float T = 0.1f;  // 时间固定10ms
	const float K = 3.0f;  // 100%输出时 -> 300rmp
	float target = s_out * K;
	
	s_speed += (target - s_speed) * 0.01f / T;
	return s_speed;
}
	