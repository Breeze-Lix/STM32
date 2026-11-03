#include "PID.h"

void PID_Init(PID_t *p, float kp, float ki, float kd, float out_max, float int_max)
{
	p->kp = kp;  p->ki = ki;  p->kd = kd;
	p->out_max = out_max;
	p->int_max = int_max;
	
	PID_Reset(p);
}

void PID_Reset(PID_t *p)
{
    p->integral = 0.0f;
    p->err_prev = 0.0f;
    p->started  = 0u;
}

float PID_Calc(PID_t *p, float target, float actual, float dt)
{
	float err = target - actual;
	float deriv = 0.0f;
	float out;
	
	/* -------- 积分项 ：累加 + 限幅 -------- */
	p->integral += err * dt;
	if (p->integral >  p->int_max) p->integral =  p->int_max;
  if (p->integral < -p->int_max) p->integral = -p->int_max;
	
	/* -------- 微分项：第一次不算，避免起步冲击 --------*/
	if( p->started )
	{
		deriv = (err - p->err_prev) / dt;
	}
	p->started = 1u;
	p->err_prev = err;
	
	/* -------- 三项相加 -------- */
	out = p->kp * err + p->ki * p->integral + p->kd * deriv;
	
	/* -------- 输出限幅 -------- */
	if (out >  p->out_max) out =  p->out_max;
	if (out < -p->out_max) out = -p->out_max;
	
	return out;
}
