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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <lis3dsh.h>

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

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

void uart_send(char* text)
{
	HAL_UART_Transmit(&huart2, (uint8_t* )text, strlen(text), HAL_MAX_DELAY);
}

void uart_send_int(uint8_t data)
{
	char output_str[5];

	snprintf(output_str, sizeof(output_str), "%d", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void uart_send_int16(int16_t data)
{
	char output_str[7];

	snprintf(output_str, sizeof(output_str), "%d", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void mgToGCharFix(char *response, uint8_t buffer_size, int16_t mg_value)
{
	char merge[50];

	char sing;
	uint16_t u_mg_value;

	if(mg_value < 0)
	{
		sing = '-';
		u_mg_value = -mg_value;
	}
	else
	{
		sing = ' ';
		u_mg_value = mg_value;
	}

	uint16_t u_g_value_int = (u_mg_value / 1000);
	uint16_t u_g_value_dec = (u_mg_value % 1000);

	snprintf(merge, sizeof(merge), "%c%01u,%03u", sing, u_g_value_int, u_g_value_dec);
	snprintf(response, buffer_size, "%s", merge);

}

void uartPrintHalStatus(Lis3dsh_StatusTypeDef status)
{
	uart_send("\r\n");

	switch (status){
		case LIS3DSH_OK:
			uart_send("LIS3DSH_OK\r\n");
			break;

		case LIS3DSH_ERROR_SPI_READ:
			uart_send("LIS3DSH_ERROR_SPI_READ\r\n");
			break;

		case LIS3DSH_ERROR_SPI_WRITE:
			uart_send("LIS3DSH_ERROR_SPI_WRITE\r\n");
			break;

		case LIS3DSH_ERROR_OVERFLOW:
			uart_send("LIS3DSH_ERROR_OVERFLOW\r\n");
			break;

		case LIS3DSH_ERROR_AXIS:
			uart_send("LIS3DSH_ERROR_AXIS\r\n");
			break;

		case LIS3DSH_ERROR_DATA_MISMATCH:
			uart_send("LIS3DSH_ERROR_DATA_MISMATCH\r\n");
			break;

		case LIS3DSH_ERROR_NULL_POINTER:
			uart_send("LIS3DSH_ERROR_NULL_POINTER\r\n");
			break;

		case LIS3DSH_ERROR_NO_INIT:
			uart_send("LIS3DSH_ERROR_NO_INIT\r\n");
			break;

		case LIS3DSH_ERROR_TIMEOUT:
			uart_send("LIS3DSH_ERROR_TIMEOUT\r\n");
			break;

		case LIS3DSH_ERROR_SPI_TIMEOUT:
			uart_send("LIS3DSH_ERROR_SPI_TIMEOUT\r\n");
			break;

		case LIS3DSH_ERROR_SPI_BUSY:
			uart_send("LIS3DSH_ERROR_SPI_BUSY\r\n");
			break;

		case LIS3DSH_ERROR_SPI_ZERO_LENGTH:
			uart_send("LIS3DSH_ERROR_SPI_ZERO_LENGTH\r\n");
			break;

		case LIS3DSH_ERROR_WHO_AM_I:
			uart_send("LIS3DSH_ERROR_WHO_AM_I\r\n");
			break;

		default:
			uart_send("LIS3DSH_UNKNOWN\r\n");
			break;

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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  Lis3dsh_StatusTypeDef status;

  Lis3dsh_t sensor = {0};

  int16_t x_mg = 0xFFU;
  int16_t y_mg = 0xFFU;
  int16_t z_mg = 0xFFU;

  char x_char[10], y_char[10], z_char[10];

  uint8_t init_counter = 0U;

  uint32_t delay_time = 2000U;

  void LIS3DSH_Workflow(void)
  {
	  while(1)
	  {
		  if(sensor.init_status != LIS3DSH_INIT_OK)
	 	  {
			  status = sensorInit(&sensor, &hspi1, GPIOE, GPIO_PIN_3);
			  if(status == LIS3DSH_OK)
	 		  {
				  uart_send("--------------------------------------\r\n");
	 		  	  uart_send("INIT_OK\r\n");

	 		  	  init_counter = 0U;
	 		  }
	 		  else
	 		  {
	 			  uart_send("----------------------------------------\r\n");
	 			  uartPrintHalStatus(status);
	 		  	  uart_send("\r\n");
	 		  	  uart_send_int(sensor.init_status);

	 		  	  init_counter++;

	 		  	  if(init_counter > 5U)
	 		  	  {
	 		  		  while(1);
	 		  	  }
	 		  }
	 	  }

		  if(sensor.init_status == LIS3DSH_INIT_OK)
		  {
			  status = readAxesMg(&sensor, &x_mg, &y_mg, &z_mg);
			    if(status == LIS3DSH_OK)
			    {

			  	  uart_send("--------------------------------------\r\n");

			  	  uart_send("X_AXIX_MG: ");
			  	  uart_send_int16(x_mg);
			  	  uart_send("\r\nY_AXIX_MG: ");
			  	  uart_send_int16(y_mg);
			  	  uart_send("\r\nZ_AXIX_MG: ");
			  	  uart_send_int16(z_mg);

			  	  mgToGCharFix(x_char, 10U, x_mg);
			  	  mgToGCharFix(y_char, 10U, y_mg);
			  	  mgToGCharFix(z_char, 10U, z_mg);

			  	  uart_send("\r\n--------------------------------------\r\n");

			  	  uart_send("X_AXIS: ");
			  	  uart_send(x_char);
			  	  uart_send("\r\nY_AXIS: ");
			  	  uart_send(y_char);
			  	  uart_send("\r\nZ_AXIS: ");
			  	  uart_send(z_char);
			  	  uart_send("\r\n");
			    }
			    else
			    {
			  	  uartPrintHalStatus(status);
			    }
		  }

		  HAL_Delay(delay_time);
	   }
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  LIS3DSH_Workflow();
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
