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
	PARSER_BUILD_PACKET,
	PARSER_APPLICATION,
	PARSER_ERROR
}WorkState_t;

typedef enum ErrorState_e
{
	ERROR_NO,
	ERROR_RX_OVERFLOW,
	ERROR_START_BYTE_FOUND,
	ERROR_INVALID_COMMAND,
	ERROR_RX_PAYLOAD_LENGTH_OVERFLOW,
	ERROR_RX_PACKET_OVERFLOW,
	ERROR_RX_PACKET_UNDERFLOW,
	ERROR_CHECKSUM
}ErrorState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RX_PACKET_SIZE 			8U
#define RX_PACKET_TIMEOUT_TIME	5000U
#define RX_PAYLOAD_SIZE 		3U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static CommandMode_t 	command_state = COMMAND_WAIT;
static WorkState_t 		parser_state = PARSER_WAIT_START;
static ErrorState_t 	error_state = ERROR_NO;

static uint8_t	rx_byte = 0U;
static bool 	rx_byte_ready = false;

static uint8_t rx_packet[RX_PACKET_SIZE];
static uint8_t rx_command = 0U;
static uint8_t rx_length = 0U;
static uint8_t rx_payload[RX_PAYLOAD_SIZE];

static uint8_t available_commands_size = COMMAND_COUNT;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t CalculateChecksum(uint8_t packet[], uint8_t length)
{
	uint8_t sum = 0x00U;

	for (uint8_t i = 1; i < length; i++)
	{
		sum ^= packet[i];
	}
	return sum;
}
uint8_t PacketBuilder(void)
{
	uint32_t start_time;
	uint8_t packet_index = 0U;
	bool packet_start = false;

	while(packet_start == false)
	{
		if (rx_byte_ready)
		{
			if(rx_byte == 0xAA)
			{
				rx_packet[packet_index] = rx_byte;
				packet_index++;
				packet_start = true;
				rx_byte_ready = false;
				start_time = HAL_GetTick();

				parser_state = PARSER_READ_PACKET;
			}

			if(HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
			{
				Error_Handler();
			}
		}
	}

	while((HAL_GetTick() - start_time) <= RX_PACKET_TIMEOUT_TIME)
	{
		if(rx_byte_ready)
		{
			if (packet_index < RX_PACKET_SIZE)
			{
				start_time = HAL_GetTick();
				rx_packet[packet_index] = rx_byte;
				packet_index++;
			}
			else
			{
				/* RX packet overflow error. */
				parser_state = PARSER_ERROR;
				error_state = ERROR_RX_OVERFLOW;
			}

			rx_byte_ready = false;
			if(HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
			{
				Error_Handler();
			}
		}
	}

	return packet_index;
}

void ParserPipline(uint8_t packet_lenght)
{
	uint8_t packet_index = 0U;
	uint8_t rx_checksum = 0U;

	if (rx_packet[packet_index] == 0xAA)
	{
		packet_index++;
		parser_state = PARSER_BUILD_PACKET;
	}
	else
	{
		/* Start byte found error. */
		parser_state = PARSER_ERROR;
		error_state = ERROR_START_BYTE_FOUND;
	}


	if(error_state == ERROR_NO)
	{
		if (rx_packet[packet_index] == COMMAND_WAIT)
		{
			command_state = rx_packet[packet_index];
			rx_command = rx_packet[packet_index];
		}
		else if (rx_packet[packet_index] < available_commands_size)
		{
			command_state = rx_packet[packet_index];
			rx_command = rx_packet[packet_index];
		}
		else
		{
			rx_command = rx_packet[packet_index];
			parser_state = PARSER_ERROR;
			error_state = ERROR_INVALID_COMMAND;
		}

		packet_index++;

		if(error_state == ERROR_NO)
		{
			if(rx_packet[packet_index] > RX_PAYLOAD_SIZE)
			{
				parser_state = PARSER_ERROR;
				error_state = ERROR_RX_PAYLOAD_LENGTH_OVERFLOW;
			}
			else
			{
				rx_length = rx_packet[packet_index];
			}

			packet_index++;

			if(error_state == ERROR_NO)
			{
				for(uint8_t i = 0; i < rx_length; i++)
				{
					rx_payload[i] = rx_packet[packet_index];
					packet_index++;
				}

				if(packet_lenght == packet_index + 1)
				{
					rx_checksum = CalculateChecksum(rx_packet, packet_index);

					if (rx_packet[packet_index] != rx_checksum)
					{
						parser_state = PARSER_ERROR;
						error_state = ERROR_CHECKSUM;
					}

				}
				else if(packet_lenght > packet_index + 1)
				{
					/* Packet length overflow  */
					parser_state = PARSER_ERROR;
					error_state = ERROR_RX_PACKET_OVERFLOW;
				}
				else
				{
					/* Packet length underflow  */
					parser_state = PARSER_ERROR;
					error_state = ERROR_RX_PACKET_UNDERFLOW;
				}

			}
		}
	}
}

void reset_init(void)
{
	command_state = COMMAND_WAIT;
	parser_state = PARSER_WAIT_START;

	rx_command = 0U;

	for (uint8_t i = 0; i < RX_PACKET_SIZE; i++)
	{
		rx_packet[i] = 0x00U;
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

  HAL_UART_Transmit(&huart2, &available_commands_size, 1U, HAL_MAX_DELAY);

  if(HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
  {
	  Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint8_t rx_packet_length = PacketBuilder();

	  if(error_state == ERROR_NO)
	  {
		  ParserPipline(rx_packet_length);
	  }

	  reset_init();
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
	if(huart->Instance)
	{
		rx_byte_ready = true;
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
