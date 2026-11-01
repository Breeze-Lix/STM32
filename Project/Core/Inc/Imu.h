#ifndef __IMU_H
#define __IMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
	
void  IMU_Poll(void);            /* 主循环调用：取帧、校验、更新数据 */
	
float IMU_GetRoll(void);				 /* 横滚角，单位度 */
float IMU_GetPitch(void);        /* 俯仰角，单位度 */
float IMU_GetYaw(void);          /* 相对航向角，单位度，上电为 0 */
	
float IMU_GetYawRate(void);      /* Z 轴角速度，单位度/秒 */
	
void  IMU_ResetYawZero(void);    /* 把当前角度设为新的零点 */
uint8_t IMU_IsOnline(void);      /* 数据流是否正常，1=在线 0=断流 */
      
#ifdef __cplusplus
}
#endif

#endif
