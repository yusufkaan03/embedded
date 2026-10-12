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

#define SPI_READ_DATA	1U
#define SPI_WRITE_DATA	0U
#define INFO_01_REG		0x0DU
#define INFO_02_REG		0x0EU
#define WHO_AM_I_REG	0x0FU

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

void uart_send_int(uint8_t *data, uint8_t data_size)
{
	uint8_t char_size = (data_size*5U) + 1U;
	char output_str[char_size];

	for (int i = 0; i < data_size; i++) {
	    snprintf(output_str + (i * 5), sizeof(output_str) - (i * 5), "0x%02X ", data[i]);
	}

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void buildSpiPacket(uint8_t *rx_data, uint8_t rx_data_size, uint8_t mode, uint8_t adress, uint8_t *data)
{
	rx_data[0] = (mode << 7) | (adress & 0x7FU);

	for(uint8_t i = 1; i < rx_data_size; i++)
	{
		rx_data[i] = data[i-1U];
	}
}

HAL_StatusTypeDef spiTransmitRecivece(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint8_t size, uint8_t timeout, GPIO_TypeDef *port, uint16_t pin)
{
	HAL_StatusTypeDef status;
	HAL_GPIO_WritePin(port, pin, RESET);
	status = HAL_SPI_TransmitReceive(hspi, pTxData, pRxData, size, timeout);
	HAL_GPIO_WritePin(port, pin, SET);

	return status;
}

void uartPrintHalStatus(HAL_StatusTypeDef status)
{
	switch (status){
		case HAL_OK:
			uart_send("HAL_OK\r\n");
			break;

		case HAL_ERROR:
			uart_send("HAL_ERROR\r\n");
			break;

		case HAL_BUSY:
			uart_send("HAL_BUSY\r\n");
			break;

		case HAL_TIMEOUT:
			uart_send("HAL_TIMEOUT\r\n");
			break;

		default:
			uart_send("HAL_UNKNOWN\r\n");
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

  __HAL_SPI_ENABLE(&hspi1);

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  uint8_t tx_data = 0x00U;
  uint8_t tx_buffer[2];
  uint8_t rx_buffer[] = {0x00U, 0x00U};

  HAL_StatusTypeDef status;

  buildSpiPacket(tx_buffer, sizeof(tx_buffer), SPI_READ_DATA, WHO_AM_I_REG, &tx_data);
  status = spiTransmitRecivece(&hspi1, tx_buffer, rx_buffer, sizeof(tx_buffer), 100, MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin);
  if(status == HAL_OK)
  {
	  uart_send("WHO_AM_I: ");
	  uart_send_int(rx_buffer, sizeof(rx_buffer));
  }
  else
  {
	  uartPrintHalStatus(status);
  }


  buildSpiPacket(tx_buffer, sizeof(tx_buffer), SPI_READ_DATA, INFO_01_REG, &tx_data);
  status = spiTransmitRecivece(&hspi1, tx_buffer, rx_buffer, sizeof(tx_buffer), 100, MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin);
  if(status == HAL_OK)
  {
	  uart_send("\r\nINFO_01: ");
	  uart_send_int(rx_buffer, sizeof(rx_buffer));
  }
  else
  {
	  uartPrintHalStatus(status);
  }


  buildSpiPacket(tx_buffer, sizeof(tx_buffer), SPI_READ_DATA, INFO_02_REG, &tx_data);
  status = spiTransmitRecivece(&hspi1, tx_buffer, rx_buffer, sizeof(tx_buffer), 100, MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin);
  if(status == HAL_OK)
  {
	  uart_send("\r\nINFO_02: ");
	  uart_send_int(rx_buffer, sizeof(rx_buffer));
  }
  else
  {
	  uartPrintHalStatus(status);
  }

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
