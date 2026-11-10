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

#include "tim.h"

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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityLow, 0, 512);
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
	// Motor_Init();
	// Motor_SetOutput(30.0f);
	
	// uint32_t last_print = 0;
  // float v = 0.0f;
  /* Infinite loop */
  for(;;)
  {
		// 并发测试代码
    // printf("A ");
    // osDelay(100);
		
		// 打印模拟PID
		printf("%f,%f\r\n", g_target, g_actual);
    // osDelay(20);          /* 100ms 打印 */
		
		// v = Motor_GetSpeed();                    /* 必须�? 10ms �? */
    // Motor_SetOutput(30.0f);                  /* 保持输出不变 */

    // if (HAL_GetTick() - last_print >= 100u)
    // {
    //   last_print = HAL_GetTick();
    //   printf("%d\r\n", (int)v);
    // }
		// printf("%f\r\n",(float)g_pid.kp);
		
    osDelay(50);
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
	
	// HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	// HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET); 
  for(;;)
  {
		// 并发测试代码
		// printf("B ");
    // osDelay(500);
		
	 // /* 2 秒一个完整周期，幅度 ±100 */
   // float t = (float)(HAL_GetTick() % 2000u) / 1000.0f;
   // g_target = 100.0f /* * sinf(2.0f * 3.1415926f * t / 2.0f)*/;
		
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	 // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1080);
   // osDelay(2000);
	 // 
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	 // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1080);
   // osDelay(2000);
	 // 
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	 // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	 // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
   // osDelay(2000);
	 
	 // g_target = 100.0f;
	 
	 // g_target = ((HAL_GetTick() / 3000u) % 2u) ? 100.0f : 0.0f;
	 
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
	TickType_t xLastWake = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
		float actual = Motor_GetSpeed();    // 反馈
		float out = PID_Calc(&g_pid, g_target, actual, 0.01f);
		Motor_SetOutput(out);								// 输出
		g_actual = actual;
		
    vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10));
  }
  /* USER CODE END StartpidTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/* USER CODE END Application */

