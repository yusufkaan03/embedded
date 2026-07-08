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

#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    MODE_IDLE,
    MODE_ACTIVE,
    MODE_ERROR_SIM
} SystemMode_t;

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

uint32_t last_push_time = 0;
uint32_t last_uart_send_time = 0;

const char* get_mode_string(SystemMode_t mode)
{
	switch (mode)
	{
	case MODE_IDLE: return "IDLE";

	case MODE_ACTIVE: return "ACTIVE";

	case MODE_ERROR_SIM: return "ERROR_SIM";

	default: return "UNKNOWN";
	}
}

const char* get_status(SystemMode_t mode)
{
	if (mode == MODE_ERROR_SIM)
	{
		return "ERROR";
	}

	else
	{
		return "OK";
	}
}

void uart_send_and_time_check(SystemMode_t mode, uint32_t button_count)
{

	if ((HAL_GetTick() - last_uart_send_time) >= 1000)
	{
		char tx_buffer[64];

		const char *mode_string = get_mode_string(mode);

		const char *status = get_status(mode);

		last_uart_send_time = HAL_GetTick();

		int len = snprintf(
				tx_buffer,
				sizeof(tx_buffer),
				"%lu,%s,%lu,%s\r\n",
				(unsigned long)last_uart_send_time,
				mode_string,
				(unsigned long)button_count,
				status);

		HAL_UART_Transmit(&huart2, (uint8_t*)tx_buffer, len, HAL_MAX_DELAY);
	}
}

uint8_t debounce_check(void)
{
	if ((HAL_GetTick() - last_push_time) > 150)
	{
		while(HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET);

		last_push_time = HAL_GetTick();
		return 1;
	}

	else
	{
		return 0;
	}

}

int button_control(void)
{
	  uint8_t control = 0;

	  if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET)
	  {
		  control = debounce_check();
		  if (control == 1)
		  {
			  return 1;
		  }

		  else
		  {
			  return 0;
		  }
	  }

	  else
	  {
		  return 0;
	  }

}


uint8_t time_control(uint32_t time, SystemMode_t mode, uint32_t button_count)
{
	uint32_t start = HAL_GetTick();
	uint8_t push = 0;

	while((HAL_GetTick() - start) <= time)
	{
		push = button_control();
		uart_send_and_time_check(mode, button_count);

		if (push == 1)
		{
			return 1;
		}

	}

	return 0;
}

void turnoff_all_led(void)
{
	HAL_GPIO_WritePin(LD4_GREEN_GPIO_Port, LD4_GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD3_ORANGE_GPIO_Port, LD3_ORANGE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD5_RED_GPIO_Port, LD5_RED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD6_BLUE_GPIO_Port, LD6_BLUE_Pin, GPIO_PIN_RESET);
}

SystemMode_t get_next_mode (SystemMode_t mode)
{
	switch(mode)
	{
	case MODE_IDLE:
		return MODE_ACTIVE;

	case MODE_ACTIVE:
		return MODE_ERROR_SIM;

	case MODE_ERROR_SIM:
		return MODE_IDLE;

	default:
		return MODE_IDLE;
	}
}



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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  SystemMode_t current_mode = MODE_IDLE;
  uint32_t button_count = 0;
  uint16_t toggle_time = 500;
  uint8_t button = 0;

  turnoff_all_led();

  char header[] = "timestamp_ms,mode,button_count,status\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)header, strlen(header), HAL_MAX_DELAY);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if (current_mode == MODE_IDLE)
	  {
		  turnoff_all_led();

		  while(button == 0)
		  {
			  HAL_GPIO_TogglePin(LD4_GREEN_GPIO_Port, LD4_GREEN_Pin);
			  button = time_control(toggle_time, current_mode, button_count);
		  }

		  button_count++;
		  current_mode = get_next_mode(current_mode);
		  button = 0;
	  }

	  else if (current_mode == MODE_ACTIVE)
	  {
		  turnoff_all_led();

		  while (button == 0)
		  {
			  HAL_GPIO_TogglePin(LD6_BLUE_GPIO_Port, LD6_BLUE_Pin);
			  button = time_control(toggle_time, current_mode, button_count);
		  }

		  button_count++;
		  current_mode = get_next_mode(current_mode);
		  button = 0;
	  }

	  else if (current_mode == MODE_ERROR_SIM)
	  {
		  turnoff_all_led();

		  while (button == 0)
		  {
			  HAL_GPIO_TogglePin(LD5_RED_GPIO_Port, LD5_RED_Pin);
			  button = time_control(toggle_time, current_mode, button_count);
		  }

		  button_count++;
		  current_mode = get_next_mode(current_mode);
		  button = 0;
	  }


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
