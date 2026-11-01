// ================= 惯性测量单元 ==================

// =========== JY60(可以更改) ===========
// ***   JY60绕的某根轴转动，角度以及角速度
// **    X轴：Roll -> ∓180°
// **    Y轴：Pitch -> ∓90°
// **    Z轴：Yaw -> ∓180°
// ** 	 角速度: ~ Rate -> °/s

#include "Ring.h"
#include "Imu.h"
#include "main.h"

/* ------------ 协议常量 --------------- */
#define JY60_FRAME_LEN    11u												/* 帧长度 */
#define JY60_HEAD         0x55u											/* 代表帧的开头 */
#define JY60_TYPE_GYRO    0x52u											/*  */
#define JY60_TYPE_ANGLE   0x53u											/* 代表角度数据帧 */
#define IMU_TIMEOUT_MS   100u    /* 超过这么久没收到有效帧就算断流 */

/* ------------ 量纲换算 --------------- */
#define JY60_GYRO_LSB     (2000.0f / 32768.0f)    /* 度/秒 每 LSB */
#define JY60_ANGLE_LSB    (180.0f  / 32768.0f)    /* 度    每 LSB */

/* --------------- 内部状态 --------------- */

static float   s_raw_roll;				/* 传感器给的x角度，未偏置 */
static float   s_raw_pitch;				/* 传感器给的y角度，未偏置 */
static float   s_raw_yaw;         /* 传感器给的z角度，未偏置 */

static float   s_roll_offset;     /* 上电时记下的角度，当零点用 */
static float   s_pitch_offset;    /* 上电时记下的角度，当零点用 */
static float   s_yaw_offset;      /* 上电时记下的角度，当零点用 */

static float   s_roll_rate;       /* X 轴角速度 */
static float   s_pitch_rate;      /* Y 轴角速度 */
static float   s_yaw_rate;        /* Z 轴角速度 */


static uint8_t s_offset_ready;    /* 是否已经记录过零点 */
static uint32_t s_frame_count;    /* 调试用：累计切出多少合法帧 */
static uint32_t s_last_tick;     /* 最后一次收到有效帧的时刻 */

/* ----------------- 内部工具 ----------------- */

// 小端两字节拼成有符号 16 位 
static int16_t Jy60_Int16(uint8_t lo, uint8_t hi)
{
    return (int16_t)((uint16_t)hi << 8 | (uint16_t)lo);
}

/* ---------------- 对外接口 ---------------- */
void IMU_Poll(void)
{
	uint8_t frame[JY60_FRAME_LEN];
	uint8_t sum;
	uint8_t i;													/* 参数i用于for循环 */
	
	// 数据不够一帧长度时，等到够再调用
	while( Ring_Count() >= JY60_FRAME_LEN )
	{
		// 检验帧头是不是 0x55 ,不是就丢一个字节
		if( Ring_Peek(0u) != JY60_HEAD)
		{
			Ring_Drop(1u);
			continue;
		}
		
		// 算前十个字节之和，避免中间数据假冒帧头
		sum = 0u;
		for( i = 0u ; i < JY60_FRAME_LEN - 1u; i++ )
		{
			sum += Ring_Peek(i);
		}
		
		// 对不上：这个 0x55 是数据冒充的，同样丢 1 个
		if( sum != Ring_Peek(JY60_FRAME_LEN - 1u) )
		{
			Ring_Drop(1u);
			continue;
		}
		
		// 数据确实是一帧，读取数据
		for( i = 0u ; i < JY60_FRAME_LEN ; i++ )
		{
			frame[i] = (uint8_t)Ring_Read();
		}
		s_frame_count++;
		s_last_tick = HAL_GetTick();
		
		// 分类数据，按类型换算
		if( frame[1] == JY60_TYPE_ANGLE ) // 角度计算
		{
			s_raw_roll  = (float)Jy60_Int16(frame[2], frame[3]) * JY60_ANGLE_LSB;
      s_raw_pitch = (float)Jy60_Int16(frame[4], frame[5]) * JY60_ANGLE_LSB;
			s_raw_yaw = (float)Jy60_Int16(frame[6] , frame[7]) * JY60_ANGLE_LSB;
		
			if( s_offset_ready == 0u)
			{
				s_roll_offset  = s_raw_roll;
				s_pitch_offset = s_raw_pitch;
				s_yaw_offset   = s_raw_yaw;
				s_offset_ready = 1u;                           // 已经记录零点位置
			}
			
		}else if (frame[1] == JY60_TYPE_GYRO) // 角速度计算
    {
      s_yaw_rate = (float)Jy60_Int16(frame[6], frame[7]) * JY60_GYRO_LSB;
    }
		
	}
	
}

// 输出角度（已经计算归零）
float IMU_GetRoll(void)
{
	return s_raw_roll - s_roll_offset;
}

float IMU_GetPitch(void)
{
	return s_raw_pitch - s_pitch_offset;
}

float IMU_GetYaw(void)
{
    return s_raw_yaw - s_yaw_offset;
}

// 输出Yaw角速度
float IMU_GetYawRate(void)
{
    return s_yaw_rate;
}

// 重置Yaw为零
void IMU_ResetYawZero(void)
{
    if (s_offset_ready != 0u)
    {
        s_yaw_offset = s_raw_yaw;
    }
}

uint8_t IMU_IsOnline(void)
{
	if( s_frame_count == 0u )
	{
		return 0u;
	}
	return (( HAL_GetTick() - s_last_tick ) < IMU_TIMEOUT_MS )? 1u : 0u;
}
