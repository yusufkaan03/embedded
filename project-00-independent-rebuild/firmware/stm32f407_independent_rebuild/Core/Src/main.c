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

#include <string.h>
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

int button_control(uint8_t status)
    {
  	uint32_t delay_time = HAL_GetTick();

  	while ((HAL_GetTick()- delay_time ) < 500)
  	{

  	  if (status == 1)
  	  {
  		  status = 1;
  	  }

  	  else if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET)
  	  {
  		  status = 1;
  	  }

  	  else
  	  {
  		  status = 0;
  	  }

  	}

  	return status;
    }

  uint32_t start = 0;


  void UART(const char *mod, int button, const char *status)
  {
  	if (HAL_GetTick() - start < 1000)
  	{

  	}

  	else
  	{
  		start = HAL_GetTick();

  		char tx_buffer[64];

  		int len = snprintf(
  				tx_buffer,
				sizeof(tx_buffer),
				"%lu,%s,%d,%s\r\n",
				(unsigned long)start,
				mod,
				button,
				status);

  		HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, HAL_MAX_DELAY);

  	}
  }

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint8_t button_count = 0;
    uint8_t mode_selector = 0;
    uint8_t button = 0;
    char *mode;
    char *status;

    char header[] = "timestamp_ms,mode,button_count,status\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)header, strlen(header), HAL_MAX_DELAY);


    while (1)
    {
  	  button = 0;
  	  mode_selector = (button_count%3);

  	  if (mode_selector == 0)
  	  {
  		  HAL_GPIO_WritePin (LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD6_BLUE_GPIO_Port, LD6_BLUE_Pin, GPIO_PIN_RESET);

  		  mode = "IDLE";
  		  status = "OK";


  		  while (button == 0)
  		  {
  			  HAL_GPIO_TogglePin (LD4_GREEN_GPIO_Port, LD4_GREEN_Pin);
  			  button = button_control(button);
  			  UART(mode, button_count, status);

  		  }

  		  button_count++;

  	  }



  	  else if (mode_selector == 1)
  	  {
  		  HAL_GPIO_WritePin (LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD6_BLUE_GPIO_Port, LD6_BLUE_Pin, GPIO_PIN_RESET);

  		mode = "ACTIVE";
  		status = "OK";


  		  while (button == 0)
  		  {
  			  HAL_GPIO_TogglePin (LD6_BLUE_GPIO_Port, LD6_BLUE_Pin);
  			  button = button_control(button);
  			UART(mode, button_count, status);

  		  }

  		  button_count++;

  	  }




  	  else if (mode_selector == 2)
  	  {
  		  HAL_GPIO_WritePin (LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_RESET);
  		  HAL_GPIO_WritePin (LD6_BLUE_GPIO_Port, LD6_BLUE_Pin, GPIO_PIN_RESET);

  		  mode = "ERROR_SIM";
  		  status = "ERROR";

  		  while (button == 0)
  		  {
  			  HAL_GPIO_TogglePin (LD5_RED_GPIO_Port, LD5_RED_Pin);
  			  button = button_control(button);
  			UART(mode, button_count, status);

  		  }

  		  button_count++;

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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
