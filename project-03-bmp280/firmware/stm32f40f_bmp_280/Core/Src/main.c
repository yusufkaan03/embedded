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
#include "bmp280.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include<stdio.h>
#include<string.h>
#include<stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

typedef enum
{
	SENSOR_INIT,
	SENSOR_MEASURE,
}Status_t;

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

void uart_send(char* text)
{
	HAL_UART_Transmit(&huart2, (uint8_t* )text, strlen(text), HAL_MAX_DELAY);
}

void intToCharFix(int32_t value, char *response, uint8_t buffer_size)
{
	char merge[15];

	char sign = (value < 0) ? '-' : ' ';

	uint32_t u_value = (value < 0) ? -value : value;

	uint32_t u_value_int = (u_value / 100);
	uint32_t u_value_dec = (u_value % 100);

	snprintf(merge, sizeof(merge), "%c%lu.%02lu", sign, u_value_int, u_value_dec);

	snprintf(response, buffer_size, "%s", merge);
}

void uintToCharFix(uint32_t value, char *response, uint8_t buffer_size)
{
	char convert[15];

	uint32_t u_value_int = (value / 100);
	uint32_t u_value_dec = (value % 100);

	snprintf(convert, sizeof(convert), "%lu.%02lu", u_value_int, u_value_dec);

	snprintf(response, buffer_size, "%s", convert);
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  /*
  int32_t temp;
  int32_t temp_f;
  uint32_t press;
  uint32_t press_f;

  BMP280_Init(&BMP280, &hi2c1, 0x76U);

  fastTemp(&BMP280, &temp);
  fastPress(&BMP280, &press);
  fastMeasure(&BMP280, &temp_f, &press_f);


  char temp_char_f[15];

  char press_char_f[10];

  intToCharFix(temp, temp_char, sizeof(temp_char));
  intToCharFix(temp_f, temp_char_f, sizeof(temp_char_f));

  uintToCharFix(press, press_char, sizeof(press_char));
  uintToCharFix(press_f, press_char_f, sizeof(press_char_f));

  char response_s[150];
  char response_f[150];

  snprintf(response_s, sizeof(response_s), "TEMPERATURE:%7sDegC\r\nPRESSURE:%10shPa\r\n", temp_char, press_char);
  snprintf(response_f, sizeof(response_f), "TEMPERATURE_F:%7sDegC\r\nPRESSURE_F:%10shPa\r\n", temp_char_f, press_char_f);

  uart_send(response_s);
  uart_send(response_f);*/

  BMP280_t BMP280 = {0};


  int32_t temp_s;
  uint32_t press_s;

  char temp_char[15];
  char press_char[10];


  Status_t status = SENSOR_INIT;
  BMP280_Init_Status_t init_status;
  BMP280_Measure_Status_t measure_status;

  uint32_t last_time = 0U;

  uint16_t error_code_init = 0x0000;
  uint16_t error_code_measure = 0x0000;

  char response[150];

  uint32_t init_count = 0U;

  uint32_t ok_count = 0U;
  uint32_t error_count = 0U;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if((HAL_GetTick() - last_time) >= 1000U)
	  /*if(1)*/
	  {
		  last_time = HAL_GetTick();

		  if(status == SENSOR_INIT)
		  {
			  init_status = BMP280_Init(&BMP280, &hi2c1, 0x76U);
			  init_count++;

			  if(init_status == INIT_OK)
			  {
				  status = SENSOR_MEASURE;
				  uart_send("--------------------\r\n");
				  uart_send("BMP280_INIT_OK\r\n");
				  error_code_init = 0x00A1;
				  ok_count ++;
			  }
			  else
			  {
				  uint16_t error = ((uint16_t)(init_status) << 8U) | ((uint16_t)(BMP280.Error));

				  if(error_code_init != error)
				  {
					  error_code_init = error;
					  snprintf(response, sizeof(response), "INIT_ERROR_CODE: %X\r\n", error_code_init);
					  uart_send("--------------------\r\n");
					  uart_send(response);
				  }

				  error_count ++;
				  status = SENSOR_INIT;
			  }

			  char try[150];
			  snprintf(try, sizeof(try), "init_count: %lu\r\nok_count: %lu\r\nerror_count: %lu\r\n",init_count, ok_count, error_count);
			  uart_send("--------------------\r\n");
			  uart_send(try);
		  }

		  if(status == SENSOR_MEASURE)
		  {
			  measure_status = fastMeasure(&BMP280, &temp_s, &press_s);

			  if(measure_status == MEASURE_OK)
			  {
				  intToCharFix(temp_s, temp_char, sizeof(temp_char));
				  uintToCharFix(press_s, press_char, sizeof(press_char));
				  snprintf(response, sizeof(response), "TEMPERATURE:%7sDegC\r\nPRESSURE:%10shPa\r\n", temp_char, press_char);
				  uart_send("--------------------\r\n");
				  uart_send(response);
				  error_code_measure = 0x001A;
				  status = SENSOR_MEASURE;
			  }
			  else
			  {
				  uint16_t error = ((uint16_t)(measure_status) << 8U) | ((uint16_t)(BMP280.Error));

				  if(error_code_measure != error)
				  {
					  error_code_measure = error;
					  snprintf(response, sizeof(response), "MEASURE_ERROR_CODE: %X\r\n", error_code_measure);
					  uart_send("--------------------\r\n");
					  uart_send(response);
				  }

				  status = SENSOR_INIT;
			  }
		  }
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
