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
	PARSER_SEND,
	PARSER_ERROR
}WorkState_t;

typedef enum ErrorState_e
{
	ERROR_NO								= 0x00,
	ERROR_RX_TIMEOUT						= 0xA0,
	ERROR_RX_PAYLOAD_LENGTH_OVERFLOW		= 0xA1,
	ERROR_RX_START_BYTE_NOT_FOUND			= 0xA2,
	ERROR_RX_INVALID_COMMAND				= 0xA3,
	ERROR_RX_CHECKSUM						= 0xA4,
	ERROR_RX_INCONSISTENCY					= 0xA5,
	ERROR_RX_BUFFER_FULL					= 0xA6,
	ERROR_COMMAND_INVALID_PAYLOAD_LENGTH	= 0xB0,
	ERROR_COMMAND_INVALID_PAYLOAD			= 0xB1,
	ERROR_EXEC_VERIFICATION_FAILED			= 0xC0,
	ERROR_EXEC_READ_ADC_FAILED				= 0xC1

}ErrorState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define UART_BUFFER_RING_SIZE	50U
#define RX_TX_PACKET_SIZE		8U
#define TIMEOUT_TIME			5000U
#define PAYLOAD_DATA_SIZE		3U
#define ADC_READ_COUNT			10U
#define ADC_READ_MIN_COUNT		3U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static CommandMode_t 	command_state = COMMAND_WAIT;
static WorkState_t 		parser_state = PARSER_WAIT_START;
static ErrorState_t 	error_state = ERROR_NO;

static volatile bool backtop_write = false;
static volatile bool backtop_read = false;
static volatile bool buffer_full = false;

static volatile uint8_t uart_buffer_ring[UART_BUFFER_RING_SIZE];
static volatile uint8_t buffer_write_index = 0U;
static volatile uint8_t buffer_read_index = 0U;
static uint8_t rx_byte = 0U;

static uint8_t rx_packet[RX_TX_PACKET_SIZE];
static uint8_t rx_command = 0U;
static uint8_t rx_payload_length = 0U;
static uint8_t rx_payload[PAYLOAD_DATA_SIZE];
static uint8_t rx_checksum = 0U;

static uint8_t command_payload_length = 0U;
static uint8_t command_payload_value_min[PAYLOAD_DATA_SIZE];
static uint8_t command_payload_value_max[PAYLOAD_DATA_SIZE];
static uint8_t response_data_length = 0U;
static uint8_t response_data_value[PAYLOAD_DATA_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void TransmitPacket(void);
void ResetIndex(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ResetProtocolState(void)
{
	command_state = COMMAND_WAIT;
	parser_state = PARSER_WAIT_START;
	error_state = ERROR_NO;

	rx_command = 0U;
	rx_payload_length = 0U;
	rx_checksum = 0U;

	command_payload_length = 0U;
	response_data_length = 0U;

	for(uint8_t i = 0; i < RX_TX_PACKET_SIZE; i++)
	{
		rx_packet[i] = 0U;
	}

	for(uint8_t i = 0; i < PAYLOAD_DATA_SIZE; i++)
	{
		rx_payload[i] = 0U;
		command_payload_value_min[i] = 0U;
		command_payload_value_max[i] = 0U;
		response_data_value[i] = 0U;
	}

}

void BufferFullRecovery(void)
{
	if (buffer_full == true)
	{
		parser_state = PARSER_ERROR;
		error_state = ERROR_RX_BUFFER_FULL;

		TransmitPacket();

		ResetProtocolState();

		buffer_full = false;

	}
}

void ResetIndex(void)
{
	if(buffer_read_index == UART_BUFFER_RING_SIZE)
	{
		buffer_read_index = 0U;

		if (backtop_read == true)
		{
			backtop_read = false;
		}
		else
		{
			backtop_read = true;
		}
	}
}

uint8_t CalculateChecksum(uint8_t data[], uint8_t length)
{
	uint8_t sum = 0U;

	for(uint8_t i = 1; i < length; i++)
	{
		sum ^= data[i];
	}

	return sum;
}

void ADCReadAverage(void)
{
	uint32_t sum = 0U;
	uint8_t count = 0U;

	for(uint8_t i = 0; i < ADC_READ_COUNT; i++)
	{
		HAL_ADC_Start(&hadc1);

		if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
		{
			sum += HAL_ADC_GetValue(&hadc1);
			count++;
		}

		HAL_ADC_Stop(&hadc1);
	}

	if (count >= ADC_READ_MIN_COUNT)
	{
		uint16_t raw = (uint16_t)(sum/((uint32_t)count));
		response_data_value[0] = (uint8_t)(raw >> 8);
		response_data_value[1] = (uint8_t)(raw);
		response_data_length = 0x02U;
	}

	else
	{
		parser_state = PARSER_ERROR;
		error_state = ERROR_EXEC_READ_ADC_FAILED;
	}
}

uint8_t ProcessBuffer(void)
{
	uint32_t last_time = 0U;
	uint8_t packet_index = 0U;
	uint8_t packet_length = 0U;
	bool start_packet = true;

	while(start_packet)
	{
		if(buffer_read_index != buffer_write_index || backtop_write != backtop_read)
		{
			if(uart_buffer_ring[buffer_read_index] == 0xAAU)
			{
				rx_packet[packet_index] = uart_buffer_ring[buffer_read_index];
				packet_index++;
				buffer_read_index++;
				start_packet = false;
			}
			else
			{
				buffer_read_index++;
			}
			ResetIndex();
		}
	}

	last_time = HAL_GetTick();

	while(parser_state == PARSER_WAIT_START)
	{
		if((HAL_GetTick() - last_time) >= TIMEOUT_TIME)
		{
			parser_state = PARSER_ERROR;
			error_state = ERROR_RX_TIMEOUT;
		}

		else if(buffer_read_index != buffer_write_index || backtop_write != backtop_read)
		{
			rx_packet[packet_index] = uart_buffer_ring[buffer_read_index];

			if(packet_index == 2)
			{
				if(rx_packet[2] <= PAYLOAD_DATA_SIZE)
				{
					packet_length = rx_packet[2] + 4U;
				}
				else
				{
					parser_state = PARSER_ERROR;
					error_state = ERROR_RX_PAYLOAD_LENGTH_OVERFLOW;
				}
			}

			buffer_read_index++;
			packet_index++;

			if(packet_index == packet_length && error_state == ERROR_NO)
			{
				parser_state = PARSER_READ_PACKET;
			}

			ResetIndex();
			last_time = HAL_GetTick();
		}
	}


	return packet_length;

}

void ParserPipeline(uint8_t length)
{
	uint8_t packet_index = 0U;
	uint8_t checksum = 0U;

	if(rx_packet[packet_index] != 0xAA)
	{
		parser_state = PARSER_ERROR;
		error_state = ERROR_RX_START_BYTE_NOT_FOUND;
	}

	packet_index++;

	if(error_state == ERROR_NO)
	{
		if(rx_packet[packet_index] == 0x00 || rx_packet[packet_index] >= COMMAND_COUNT)
		{
			rx_command = rx_packet[packet_index];
			parser_state = PARSER_ERROR;
			error_state = ERROR_RX_INVALID_COMMAND;
		}

		else
		{
			rx_command = rx_packet[packet_index];
			command_state = rx_packet[packet_index];

			packet_index++;

			rx_payload_length = rx_packet[packet_index];

			packet_index++;

			for(uint8_t i = 0; i < rx_payload_length; i++)
			{
				rx_payload[i] = rx_packet[packet_index];
				packet_index++;
			}

			checksum = CalculateChecksum(rx_packet, packet_index);

			if(checksum == rx_packet[packet_index])
			{
				rx_checksum = rx_packet[packet_index];
				packet_index++;

				if(packet_index == length)
				{
					parser_state = PARSER_APPLICATION;
				}
				else
				{
					parser_state = PARSER_ERROR;
					error_state = ERROR_RX_INCONSISTENCY;
				}
			}
			else
			{
				parser_state = PARSER_ERROR;
				error_state = ERROR_RX_CHECKSUM;
			}
		}
	}
}

bool CommandDetailsCheck(uint8_t payload_length, uint8_t value_min[], uint8_t value_max[])
{
	uint8_t pass = false;

	if(rx_payload_length == payload_length)
	{
		pass = true;

		for(uint8_t i = 0; i < payload_length; i++)
		{
			if(pass)
			{
				if(rx_payload[i] < value_min[i] || rx_payload[i] > value_max[i])
				{
					parser_state = PARSER_ERROR;
					error_state = ERROR_COMMAND_INVALID_PAYLOAD;
					pass = false;
				}
			}
		}
	}
	else
	{
		parser_state = PARSER_ERROR;
		error_state = ERROR_COMMAND_INVALID_PAYLOAD_LENGTH;
		pass = false;
	}

	return pass;
}

void WorkflowManager(void)
{
	bool pass = false;

	switch(command_state)
	{
	case COMMAND_PING:
		command_payload_length = 0x00U;
		pass = CommandDetailsCheck(command_payload_length, command_payload_value_min, command_payload_value_max);
		if(pass)
		{
			response_data_length = 0x01U;
			response_data_value[0] = 0x01U;
		}
		break;

	case COMMAND_SET_LED:
		command_payload_length = 0x01U;
		command_payload_value_min[0] = 0x00U;
		command_payload_value_max[0] = 0x01U;
		pass = CommandDetailsCheck(command_payload_length, command_payload_value_min, command_payload_value_max);
		if(pass)
		{
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, rx_payload[0]);
			uint8_t test = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
			if(test != rx_payload[0])
			{
				parser_state = PARSER_ERROR;
				error_state = ERROR_EXEC_VERIFICATION_FAILED;
			}
			else
			{
				response_data_value[0] = test;
				response_data_length = 0x01U;
			}
		}
		break;

	case COMMAND_GET_LED:
		command_payload_length = 0x00U;
		pass = CommandDetailsCheck(command_payload_length, command_payload_value_min, command_payload_value_max);
		if (pass)
		{
			response_data_length = 0x01U;
			response_data_value[0] = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
		}
		break;

	case COMMAND_GET_ADC:
		command_payload_length = 0x00U;
		pass = CommandDetailsCheck(command_payload_length, command_payload_value_min, command_payload_value_max);
		if (pass)
		{
			ADCReadAverage();
		}
		break;

	default:
		break;
	}

	if(error_state == ERROR_NO)
	{
		parser_state = PARSER_SEND;
	}
}

void TransmitPacket(void)
{
	uint8_t response_msg[RX_TX_PACKET_SIZE];
	uint8_t start_byte = 0xAAU;
	uint8_t checksum = 0U;
	uint8_t msg_index = 0U;

	response_msg[msg_index] = start_byte;
	msg_index++;

	response_msg[msg_index] = rx_command;
	msg_index++;

	response_msg[msg_index] = error_state;
	msg_index++;

	if(error_state != ERROR_NO)
	{
		response_data_length = 0x0U;
	}

	response_msg[msg_index] = response_data_length;
	msg_index++;

	for(uint8_t i = 0; i < response_data_length; i++)
	{
		response_msg[msg_index] = response_data_value[i];
		msg_index++;
	}

	checksum = CalculateChecksum(response_msg, msg_index);

	response_msg[msg_index] = checksum;
	msg_index++;

	HAL_UART_Transmit(&huart2, (uint8_t* )response_msg, msg_index, HAL_MAX_DELAY);
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

  uint8_t length = 0U;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  switch(parser_state)
	  {
	  case PARSER_WAIT_START:
		  length = ProcessBuffer();
		  break;

	  case PARSER_READ_PACKET:
		  ParserPipeline(length);
		  break;

	  case PARSER_APPLICATION:
		  WorkflowManager();
		  break;

	  case PARSER_SEND:
		  TransmitPacket();
		  ResetProtocolState();
		  BufferFullRecovery();
		  break;

	  default:
		  TransmitPacket();
		  ResetProtocolState();
		  BufferFullRecovery();
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

		if(buffer_read_index == buffer_write_index && backtop_write != backtop_read)
		{
			buffer_full = true;
		}
		else
		{
			uart_buffer_ring[buffer_write_index] = rx_byte;
			buffer_write_index++;
			if(buffer_write_index == UART_BUFFER_RING_SIZE)
			{
				buffer_write_index = 0U;

				if(backtop_write == true)
				{
					backtop_write = false;
				}
				else
				{
					backtop_write = true;
				}
			}
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
