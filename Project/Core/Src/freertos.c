/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <math.h>
#include "Motor.h"
#include "PID.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile float g_target;           /* 任务间共享：目标速度 */
PID_t g_pid;
volatile float g_actual;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId sinTaskHandle;
osThreadId pidTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSinTask(void const * argument);
void StartpidTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of sinTask */
  osThreadDef(sinTask, StartSinTask, osPriorityNormal, 0, 256);
  sinTaskHandle = osThreadCreate(osThread(sinTask), NULL);

  /* definition and creation of pidTask */
  osThreadDef(pidTask, StartpidTask, osPriorityAboveNormal, 0, 256);
  pidTaskHandle = osThreadCreate(osThread(pidTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
		// 并发测试代码
    // printf("A ");
    // osDelay(100);
		
		printf("%d,%d\r\n", (int)g_target, (int)g_actual);
    osDelay(100);          /* 100ms 打印一次 -> 10Hz */
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartSinTask */
/**
* @brief Function implementing the sinTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSinTask */
void StartSinTask(void const * argument)
{
  /* USER CODE BEGIN StartSinTask */
  /* Infinite loop */
  for(;;)
  {
		// 并发测试代码
		// printf("B ");
    // osDelay(500);
		
	 /* 2 秒一个完整周期，幅度 ±100 */
   float t = (float)(HAL_GetTick() % 2000u) / 1000.0f;
   g_target = 100.0f * sinf(2.0f * 3.1415926f * t / 2.0f);
	 osDelay(10);
  }
  /* USER CODE END StartSinTask */
}

/* USER CODE BEGIN Header_StartpidTask */
/**
* @brief Function implementing the pidTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartpidTask */
void StartpidTask(void const * argument)
{
  /* USER CODE BEGIN StartpidTask */
	PID_Init(&g_pid, 1.0f, 0.5f, 0.0f, 100.0f, 500.0f);
  Motor_Init();
  /* Infinite loop */
  for(;;)
  {
		float actual = Motor_GetSpeed();    // 反馈
		float out = PID_Calc(&g_pid, g_target, actual, 0.01f);
		Motor_SetOutput(out);								// 输出
		g_actual = actual;
		
    osDelay(10);
  }
  /* USER CODE END StartpidTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

