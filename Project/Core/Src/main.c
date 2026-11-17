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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "UARTRx.h"
#include "zdt42.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TX_CHUNK_SIZE   64u      /* 每次最多搬多少字节，不要超过环形缓冲大小 */
#define TX_TIMEOUT_MS   100u     /* 单次发送超时，115200 下 64 字节约 5.6ms */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void U2_Text(const char *s);
static void U2_HexLine(const char *tag, const uint8_t *p, uint16_t n);
static void Demo_SelfTest(void);
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	UartRx_Start();
	Zdt42_Init();          /* 驱动层初始化：清空接收缓冲 */
	Demo_SelfTest();       /* 上电自测一次：读固件版本，验证"发一帧 / 收一帧" */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		uint8_t  buf[TX_CHUNK_SIZE];
		uint16_t n;

		/* ===== USART2 进来的数据：只转发给 USART1，不回显 ===== */
		while ((n = UartRx_Read(UART_PORT_2, buf, (uint16_t)sizeof(buf))) > 0u)
		{
			HAL_UART_Transmit(&huart1, buf, n, TX_TIMEOUT_MS);
		}

		/* ===== USART1 进来的数据：只发给 USART2 ===== */
		while ((n = UartRx_Read(UART_PORT_1, buf, (uint16_t)sizeof(buf))) > 0u)
		{
			HAL_UART_Transmit(&huart2, buf, n, TX_TIMEOUT_MS);
		}
  
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  往 USART2 发一串普通文本
  */
static void U2_Text(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), 50u);
}

/**
  * @brief  把一段字节以 HEX 形式打印到 USART2，前面加个标签
  * @note   这是调试用的"翻译"，不属于驱动层，所以放在 main 里
  */
static void U2_HexLine(const char *tag, const uint8_t *p, uint16_t n)
{
  static const char HEX[] = "0123456789ABCDEF";
  uint8_t  out[3];
  uint16_t i;

  U2_Text(tag);

  for (i = 0u; i < n; i++)
  {
    out[0] = (uint8_t)HEX[p[i] >> 4];
    out[1] = (uint8_t)HEX[p[i] & 0x0Fu];
    out[2] = ' ';
    HAL_UART_Transmit(&huart2, out, 3u, 50u);
  }

  U2_Text("\r\n");
}

/**
  * @brief  上电自测：调用驱动层读一次固件版本，把结果打印出来
  * @note   这一步只验证"能不能发出去、能不能收回来"，还不管字段含义
  */
static void Demo_SelfTest(void)
{
  uint8_t       reply[16];
  uint16_t      n = 0u;
  Zdt42Status_t st;

  U2_Text("\r\n=== ZDT42 self test ===\r\n");

  st = Zdt42_ReadVersion(reply, (uint16_t)sizeof(reply), &n);

  if (st == ZDT42_OK)
  {
    U2_HexLine("RX: ", reply, n);
  }
  else
  {
    U2_Text("FAIL: ");
    U2_Text(Zdt42_StatusStr(st));
    U2_Text("\r\n");
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
