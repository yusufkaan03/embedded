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
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	ADC_LOW,
	ADC_OK,
	ADC_HIGH,
	ADC_ERROR
} AdcSituation_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define UART_LOG_INTERVAL_MS 1000
#define ADC_SAMPLE_COUNT 10

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


uint16_t read_adc_average(void)
{
	uint32_t adc_sum = 0;

	for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++)
	{
		HAL_ADC_Start(&hadc1);

		if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
		{
			adc_sum = adc_sum + HAL_ADC_GetValue(&hadc1);
		}

		HAL_ADC_Stop(&hadc1);
	}

	return (uint16_t)(adc_sum/ADC_SAMPLE_COUNT);
}

uint8_t read_user_button(void)
{
	if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET)
	{
		return 1;
	}

	else
	{
		return 0;
	}
}

AdcSituation_t adc_situation(uint32_t adc_mv)
{
	if (adc_mv < 500U) return ADC_LOW;

	else if (adc_mv <= 3000U) return ADC_OK;

	else if (adc_mv <= 3300U) return ADC_HIGH;

	else return ADC_ERROR;
}

const char* get_situation_char(AdcSituation_t situation)
{
	switch (situation)
	{
	case ADC_LOW: return "LOW";

	case ADC_OK: return "OK";

	case ADC_HIGH: return "HIGH";

	case ADC_ERROR: return "ERROR";

	default: return "UNKNOWN";
	}
}

uint32_t convert_mv(uint16_t adc_raw)
{
	return ((uint32_t)adc_raw * 3300U) / 4095U;
}

void turnoff_leds(void)
{
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
}

void update_situation_led(AdcSituation_t status)
{
	turnoff_leds();

	switch(status)
	{
	case ADC_LOW: HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET); break;

	case ADC_OK: HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET); break;

	case ADC_HIGH: HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET); break;

	case ADC_ERROR: HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET); break;

	default: HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET); break;
	}
}

void scan_i2c_devices(void)
{
	char buffer[80];

	char start_msg[] = "I2C scan started\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t* )start_msg, strlen(start_msg), HAL_MAX_DELAY);

	for (uint8_t address = 1; address < 128; address++)
	{
		if (HAL_I2C_IsDeviceReady(&hi2c2, (address << 1), 1, 10) == HAL_OK)
		{
			int len = snprintf(
					buffer,
					sizeof(buffer),
					"Device found at 0x%02X\r\n",
					address
					);

			HAL_UART_Transmit(&huart2, (uint8_t* )buffer, len, HAL_MAX_DELAY);
		}
	}

	char end_msg[] = "I2C scan finished\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t* )end_msg, strlen(end_msg), HAL_MAX_DELAY);
}

void send_uart_head(void)
{
	char header[] = "timestamp_ms,adc_raw,adc_mv,button,status\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t*)header, sizeof(header) - 1, 100);
}

void send_sensor_log(uint32_t timestamp_t, uint16_t adc_raw, uint32_t adc_mv, uint8_t button, AdcSituation_t situation)
{
	char buffer[100];

	const char *situation_char = get_situation_char(situation);

	int len = snprintf(
			buffer,
			sizeof(buffer),
			"%lu,%u,%lu,%u,%s\r\n",
			(unsigned long)timestamp_t,
			adc_raw,
			(unsigned long)adc_mv,
			button,
			situation_char
			);

	HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
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
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  uint32_t last_log_time = 0;

  scan_i2c_devices();


  send_uart_head();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  uint32_t now = HAL_GetTick();
	  if ((now - last_log_time) >= UART_LOG_INTERVAL_MS)
	  {
	      uint8_t button = read_user_button();
		  uint16_t adc_raw = read_adc_average();
		  uint32_t adc_mv = convert_mv(adc_raw);
		  AdcSituation_t current_situation = adc_situation(adc_mv);

		  update_situation_led(current_situation);

		  send_sensor_log(now, adc_raw, adc_mv, button, current_situation);
		  last_log_time = now;
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
