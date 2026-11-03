#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct{
	float kp , ki , kd ;
		
	float out_max;  // 输出限幅
	float int_max;  // 积分限幅（防饱和）
	
	float integral; // 积分累加值
	float err_prev; // 上一次误差
	uint8_t started;  // 是否已经跑过一次
}PID_t;

void PID_Init(PID_t *p, float kp, float ki, float kd, float out_max, float int_max);
float PID_Calc(PID_t *p, float target, float actual, float dt);
void PID_Reset(PID_t *p);
	
#ifdef __cplusplus
}
#endif

#endif
