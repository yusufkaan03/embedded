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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum CommandMode_e
{
	COMMAND_WAIT,
	COMMAND_PING,
	COMMAND_SET_LED,
	COMMAND_GET_LED,
	COMMAND_GET_ADC,
	COMMAND_COUNT
}CommandMode_t;

typedef enum WorkState_e
{
	PARSER_WAIT_START,
	PARSER_READ_PACKET,
	PARSER_APPLICATION,
	PARSER_ERROR
}WorkState_t;

typedef enum ErrorState_e
{
	ERROR_NO							= 0x00,
	ERROR_RX_TIMEOUT					= 0xA0,
	ERROR_RX_PAYLOAD_LENGTH_OVERFLOW	= 0xA1,
	ERROR_RX_START_BYTE_NOT_FOUND		= 0xA2,

}ErrorState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define UART_BUFFER_RING_SIZE	50U
#define RX_PACKET_SIZE			8U
#define TIMEOUT_TIME			5000U
#define PAYLOAD_DATA_LENGTH		3U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static CommandMode_t 	command_state = COMMAND_WAIT;
static WorkState_t 		parser_state = PARSER_WAIT_START;
static ErrorState_t 	error_state = ERROR_NO;

static volatile uint8_t uart_buffer_ring[UART_BUFFER_RING_SIZE];
static volatile uint8_t buffer_write_index = 0U;
static uint8_t buffer_read_index = 0U;
static uint8_t rx_byte = 0U;

static uint8_t rx_packet[RX_PACKET_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ResetProtocolState(void)
{
	command_state = COMMAND_WAIT;
	parser_state = PARSER_WAIT_START;
	error_state = ERROR_NO;

	for(uint8_t i = 0; i < RX_PACKET_SIZE; i++)
	{
		rx_packet[i] = 0U;
	}

}

void ResetIndex(void)
{
	if(buffer_read_index == 50)
	{
		buffer_read_index = 0U;
	}
}

uint8_t ProcessBuffer(void)
{
	uint32_t last_time = 0U;
	uint8_t packet_index = 0U;
	uint8_t packet_length = 0U;
	bool start_packet = true;
	uint8_t pass = 0x10;

	HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);

	while(start_packet)
	{
		if(buffer_read_index != buffer_write_index)
		{
			if(uart_buffer_ring[buffer_read_index] == 0xAAU)
			{
				pass = 0x11;
				rx_packet[packet_index] = uart_buffer_ring[buffer_read_index];
				packet_index++;
				buffer_read_index++;
				start_packet = false;
				HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
			}
			else
			{
				pass = 0x12;
				buffer_read_index++;
				HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
			}
			ResetIndex();
		}
	}

	last_time = HAL_GetTick();

	pass = 0x13;
	HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);

	while(parser_state == PARSER_WAIT_START)
	{
		if((HAL_GetTick() - last_time) >= TIMEOUT_TIME)
		{
			parser_state = PARSER_ERROR;
			error_state = ERROR_RX_TIMEOUT;
		}

		else if(buffer_read_index != buffer_write_index)
		{
			pass = 0x14;
			HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
			rx_packet[packet_index] = uart_buffer_ring[buffer_read_index];

			if(packet_index == 2)
			{
				pass = 0x15;
				HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
				if(rx_packet[2] <= PAYLOAD_DATA_LENGTH)
				{
					pass = 0x16;
					HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
					packet_length = rx_packet[2] + 4U;
				}
				else
				{
					pass = 0x17;
					HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
					parser_state = PARSER_ERROR;
					error_state = ERROR_RX_PAYLOAD_LENGTH_OVERFLOW;
				}
			}

			buffer_read_index++;
			packet_index++;

			pass = 0x18;
			HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);

			if(packet_index == packet_length && error_state == ERROR_NO)
			{
				pass = 0x19;
				HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
				parser_state = PARSER_READ_PACKET;
			}

			ResetIndex();
			last_time = HAL_GetTick();
		}
	}

	HAL_UART_Transmit(&huart2, &error_state, 1U, HAL_MAX_DELAY);

	return packet_length;


}

void ParserPipeline(uint8_t length)
{
	uint8_t packet_index = 0U;

	if(rx_packet[packet_index] != 0xAA)
	{
		parser_state = PARSER_ERROR;
		error_state = ERROR_RX_START_BYTE_NOT_FOUND;
	}

	packet_index++;

	for(uint8_t i = 0; i < COMMAND_COUNT; i++)
	{
		if(rx_packet[i] == CommandMode_e)
		{

		}
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
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  if(HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
  {
	  Error_Handler();
  }

  ResetProtocolState();

  uint8_t pass = 20;

  uint8_t length = 0U;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  switch(parser_state)
	  {
	  case PARSER_WAIT_START:
		  pass = 0x21;
		  HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
		  length = ProcessBuffer();
		  pass = 0x22;
		  HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
		  break;

	  case PARSER_READ_PACKET:
		  pass = 0x23;
		  HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
		  HAL_UART_Transmit(&huart2, (uint8_t* )rx_packet, length, HAL_MAX_DELAY);
		  parser_state = PARSER_WAIT_START;
		  ResetProtocolState();
		  pass = 0x24;
		  HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
		  break;

	  default:
		  pass = 0x25;
		  HAL_UART_Transmit(&huart2, &pass, 1U, HAL_MAX_DELAY);
		  ResetProtocolState();
		  parser_state = PARSER_WAIT_START;
		  break;
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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	if(huart->Instance == USART2)
	{
		uart_buffer_ring[buffer_write_index] = rx_byte;
		buffer_write_index++;

		if(buffer_write_index == 50U)
		{
			buffer_write_index = 0U;
		}

		if(HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
		{
			Error_Handler();
		}
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
