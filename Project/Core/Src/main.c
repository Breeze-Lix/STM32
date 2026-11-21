/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "zdt42.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ZDT42_MOTOR_ID      0x05U
#define ZDT42_TEST_STAGE    0U
#define ZDT42_TEST_WAIT_MS  3000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
ZDT42_Status zdt_status = ZDT42_OK;
uint8_t zdt_version[5] = {0};
int16_t zdt_speed = 0;
float zdt_angle = 0.0f;
int32_t zdt_target_angle = 0;
volatile uint8_t test_done = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void ZDT42_AbortMotion(uint8_t id);
static ZDT42_Status ZDT42_TestReadOnly(uint8_t id);
static ZDT42_Status ZDT42_TestBasicControl(uint8_t id);
static ZDT42_Status ZDT42_TestSpeed(uint8_t id);
static ZDT42_Status ZDT42_TestRelativePosition(uint8_t id);
static ZDT42_Status ZDT42_TestAbsolutePosition(uint8_t id);
static ZDT42_Status ZDT42_TestRun(uint8_t id, uint8_t stage);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  zdt_status = ZDT42_Init(&huart1);

  if (zdt_status == ZDT42_OK)
  {
      HAL_Delay(1000);
      zdt_status = ZDT42_TestRun(ZDT42_MOTOR_ID, ZDT42_TEST_STAGE);
  }

  test_done = 1;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void ZDT42_AbortMotion(uint8_t id)
{
    (void)ZDT42_Stop(id);
    (void)ZDT42_Enable(id, 0);
}

static ZDT42_Status ZDT42_TestReadOnly(uint8_t id)
{
    ZDT42_Status status;

    status = ZDT42_ReadVersion(id, zdt_version, sizeof(zdt_version));
    if (status != ZDT42_OK)
    {
        return status;
    }

    status = ZDT42_ReadSpeed(id, &zdt_speed);
    if (status != ZDT42_OK)
    {
        return status;
    }

    return ZDT42_ReadPosition(id, &zdt_angle);
}

static ZDT42_Status ZDT42_TestBasicControl(uint8_t id)
{
    ZDT42_Status status;

    status = ZDT42_Enable(id, 1);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(500U);

    status = ZDT42_Stop(id);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(200U);

    status = ZDT42_Enable(id, 0);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
    }

    return status;
}

static ZDT42_Status ZDT42_TestSpeed(uint8_t id)
{
    ZDT42_Status status;

    status = ZDT42_Enable(id, 1);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_SetSpeed(id, 10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadSpeed(id, &zdt_speed);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Stop(id);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(500U);

    status = ZDT42_SetSpeed(id, -10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadSpeed(id, &zdt_speed);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Stop(id);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(500U);

    status = ZDT42_Enable(id, 0);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
    }

    return status;
}

static ZDT42_Status ZDT42_TestRelativePosition(uint8_t id)
{
    ZDT42_Status status;

    status = ZDT42_Enable(id, 1);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_MoveRelative(id, 90, 10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadPosition(id, &zdt_angle);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_MoveRelative(id, -90, 10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadPosition(id, &zdt_angle);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Stop(id);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Enable(id, 0);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
    }

    return status;
}

static ZDT42_Status ZDT42_TestAbsolutePosition(uint8_t id)
{
    ZDT42_Status status;
    int32_t start_angle;

    status = ZDT42_Enable(id, 1);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_ReadPosition(id, &zdt_angle);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }
    start_angle = (int32_t)zdt_angle;

    zdt_target_angle = start_angle + 90;
    status = ZDT42_MoveAbsolute(id, zdt_target_angle, 10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadPosition(id, &zdt_angle);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_MoveAbsolute(id, start_angle, 10, 10);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    HAL_Delay(ZDT42_TEST_WAIT_MS);

    status = ZDT42_ReadPosition(id, &zdt_angle);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Stop(id);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
        return status;
    }

    status = ZDT42_Enable(id, 0);
    if (status != ZDT42_OK)
    {
        ZDT42_AbortMotion(id);
    }

    return status;
}

static ZDT42_Status ZDT42_TestRun(uint8_t id, uint8_t stage)
{
    ZDT42_Status status;

    if (stage > 4U)
    {
        return ZDT42_PARAM_ERROR;
    }

    /* 无论哪个阶段，都先验证只读通信。 */
    status = ZDT42_TestReadOnly(id);
    if (status != ZDT42_OK)
    {
        return status;
    }

    switch (stage)
    {
        case 0U:
            return ZDT42_OK;
        case 1U:
            return ZDT42_TestBasicControl(id);
        case 2U:
            return ZDT42_TestSpeed(id);
        case 3U:
            return ZDT42_TestRelativePosition(id);
        case 4U:
            return ZDT42_TestAbsolutePosition(id);
        default:
            return ZDT42_PARAM_ERROR;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
