#ifndef __MOTOR_H
#define __MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>
	
void Motor_Init(void);
void Motor_SetOutput(float output);
float Motor_GetSpeed(void);
	
#ifdef __cplusplus
}
#endif

#endif
