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

typedef enum
{
	PARSER_WAIT_START,
	PARSER_READ_COMMAND,
	PARSER_READ_LENGTH,
	PARSER_READ_PAYLOAD,
	PARSER_READ_CALC_CHECKSUM,
	PARSER_HANDLER,
	PARSER_ERROR
}Parser_t;

typedef enum
{
	NO_ERROR 				= 0x00,
	INVALID_COMMAND_ERROR	= 0x01,
	LENGTH_OVERFLOW_ERROR	= 0x02,
	INVALID_LENGTH_ERROR	= 0x03,
	INVALID_PAYLOAD_ERROR	= 0x04,
	CHECKSUM_ERROR			= 0x05,
	ADC_READ_ERROR			= 0x06,
	TIMEOUT_ERROR			= 0x07,
	SET_LED_WRITE_ERROR		= 0x08
}ErrorMode_t;

typedef enum
{
	COMMAND_WAIT			= 0x00,
	COMMAND_PING			= 0x01,
	COMMAND_SET_LED			= 0x02,
	COMMAND_GET_LED			= 0x03,
	COMMAND_GET_ADC			= 0x04
}CommandMode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* If you want to run in debug mode, set the OPERATING_MODE value to 1; if you want to run in normal mode, set the OPERATING_MODE value to 0. */

#define OPERATING_MODE	0

#define PAYLOAD_SIZE 	3U
#define ADC_SUM_COUNT 	10U
#define TIMEOUT_TIME 	5000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint32_t timeout_count = 0U;

static uint8_t rx_byte = 0U;
volatile static bool rx_byte_ready = false;

static uint8_t rx_command = 0U;
static uint8_t rx_length = 0U;
static uint8_t rx_payload[PAYLOAD_SIZE];
static uint8_t payload_index = 0U;
static uint8_t rx_checksum = 0U;

static uint8_t available_commands[] = {0x01U, 0x02U, 0x03U, 0x04U};
static uint8_t command_length = 0U;
static uint8_t command_payload_1_max = 0U;
static uint8_t command_payload_1_min = 0U;
static uint16_t check = 0U;

static uint8_t command_value[PAYLOAD_SIZE];

static uint8_t response_length = 0U;

const uint8_t commands_size = (sizeof(available_commands) / sizeof(available_commands[0]));

static Parser_t parser_state = PARSER_WAIT_START;
static ErrorMode_t error_state = NO_ERROR;
static CommandMode_t command_state = COMMAND_WAIT;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void uart_send(char* text);
void resetState(void);
void send_packet(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void command_msg(void)
{
	uart_send("START_PACKET\r\n");
	uart_send("\r\n--- AVAILABLE COMMANDS ---\r\n");
	uart_send("0x01 : PING COMMAND\r\n");
	uart_send("0x02 : SET_LED COMMAND\r\n");
	uart_send("0x03 : GET_LED COMMAND\r\n");
	uart_send("0x04 : GET_ADC COMMAND\r\n");
	uart_send("--------------------------\r\n\r\n");
}
void uart_send(char* text)
{
	HAL_UART_Transmit(&huart2, (uint8_t* )text, strlen(text), HAL_MAX_DELAY);
}

void uart_send_uint16(uint16_t byte)
{
	char send_msg[12];

	uint8_t len = sprintf(send_msg, "%d", byte);

	HAL_UART_Transmit(&huart2, (uint8_t* )send_msg, len, HAL_MAX_DELAY);
}

void uart_send_hex(uint8_t byte)
{
	char send_msg[12];

	uint8_t len = sprintf(send_msg, "0x%02X ", byte);

	HAL_UART_Transmit(&huart2, (uint8_t* )send_msg, len, HAL_MAX_DELAY);
}

void uart_hex(uint8_t hex[], uint8_t size)
{
	HAL_UART_Transmit(&huart2, (uint8_t* )hex, size, HAL_MAX_DELAY);
}

void payload_recorder(uint8_t byte)
{
	rx_payload[payload_index] = byte;
	payload_index++;
}

uint8_t calc_checksum()
{
	uint8_t sum = 0U;

		sum = rx_command ^ rx_length;
		for(uint8_t i = 0; i < rx_length; i++)
		{
			sum = sum ^ rx_payload[i];
		}


	return sum;
}

bool adc_value_check(void)
{
	uint32_t raw_sum = 0U;
	uint8_t sum_count = 0U;

	for (uint8_t i = 0U; i < ADC_SUM_COUNT; i++)
	{
		HAL_ADC_Start(&hadc1);

		if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
		{
			raw_sum += HAL_ADC_GetValue(&hadc1);
			sum_count++;
		}

		HAL_ADC_Stop(&hadc1);
	}

	if (sum_count != 0)
	{
		uint16_t raw = (uint16_t)(raw_sum/(uint32_t)sum_count);
		check = raw;

		command_value[0] = (uint8_t)((0xFF00 & raw) >> 8);
		command_value[1] = (uint8_t)(0x00FF & raw);

		return true;
	}

	else
	{
		return false;
	}

}

void SuccessHandler(void)
{
	uart_send("\r\nRX_COMMAND:");
	uart_send_hex(rx_command);
	uart_send("\r\nRX_LENGTH:");
	uart_send_hex(rx_length);
	uart_send("\r\nRX_PAYLOAD:");

	if (rx_length == 0x00U)
	{
		uart_send("NO_PAYLOAD");
	}
	else
	{
		for (uint8_t i = 0U; i < rx_length; i++)
		{
			uart_send_hex(rx_payload[i]);
		}
	}


	uart_send("\r\nRX_CHECKSUM:");
	uart_send_hex(rx_checksum);
	uart_send("\r\nRESULT:OK\r\nRESPONSE:");


	switch(command_state)
	{
	case COMMAND_PING:
		uart_send("PONG\r\n");
		break;

	case COMMAND_SET_LED:
		uart_send("LED ");
		if(rx_payload[0] == 0x00)
		{
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
			uart_send("OFF\r\n");
		}
		else
		{
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
			uart_send("ON\r\n");
		}
		break;

	case COMMAND_GET_LED:
		uart_send("LED_STATUS_");
		command_value[0] = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
		if (command_value[0])
		{
			uart_send("ON\r\n");
		}
		else
		{
			uart_send("OFF\r\n");
		}
		break;

	case COMMAND_GET_ADC:
		uart_send("ADC_RAW:");
		uart_send_uint16(check);
		uart_send("\r\n");
		break;

	default:
		break;
	}
}

bool command_details_check(void)
{
	bool pass = true;

	if(pass && rx_length != command_length)
	{
		parser_state = PARSER_ERROR;
		error_state = INVALID_LENGTH_ERROR;
		pass = false;
	}

	if(pass)
	{
		for (uint8_t i = 0; i < rx_length; i++)
		{
			if (rx_payload[i] < command_payload_1_min || rx_payload[i] > command_payload_1_max)
			{
				parser_state = PARSER_ERROR;
				error_state = INVALID_PAYLOAD_ERROR;
				pass = false;
			}
		}
	}

	return pass;
}

void FailHandler(void)
{
	uart_send("\r\nRESULT:ERROR\r\nERROR:");
	if(parser_state == PARSER_ERROR && error_state == INVALID_COMMAND_ERROR)
	{
		uart_send("INVALID_COMMAND_ERROR\r\n");
	}
	else if(parser_state == PARSER_ERROR && error_state == INVALID_LENGTH_ERROR)
	{
		uart_send("INVALID_LENGTH_ERROR\r\n");
	}

	else if(parser_state == PARSER_ERROR && error_state == LENGTH_OVERFLOW_ERROR)
	{
		uart_send("LENGTH_OVERFLOW_ERROR\r\n");
	}

	else if(parser_state == PARSER_ERROR && error_state == INVALID_PAYLOAD_ERROR)
	{
		uart_send("INVALID_PAYLOAD_ERROR\r\n");
	}

	else if (parser_state == PARSER_ERROR && error_state == CHECKSUM_ERROR)
	{
		uart_send("CHECKSUM_ERROR\r\n");
	}

	else if (parser_state == PARSER_ERROR && error_state == TIMEOUT_ERROR)
	{
		uart_send("TIMEOUT_ERROR\r\n");
	}
	else if (parser_state == PARSER_ERROR && error_state == SET_LED_WRITE_ERROR)
	{
		uart_send("SET_LED_WRITE_ERROR\r\n");
	}
	else if (parser_state == PARSER_ERROR && error_state == ADC_READ_ERROR)
	{
		uart_send("ADC_READ_ERROR\r\n");
	}

	resetState();
}

bool WorkflowManager(void)
{
	bool workflow_pass = true;
	uint8_t checksum = 0U;

	command_length = 0U;
	command_payload_1_max = 0U;
	command_payload_1_min = 0U;
	response_length = 0U;
	command_state = COMMAND_WAIT;

	if (rx_length > PAYLOAD_SIZE)
	{
		parser_state = PARSER_ERROR;
		error_state = LENGTH_OVERFLOW_ERROR;
		workflow_pass = false;
	}

	if (workflow_pass)
	{
		checksum = calc_checksum();

		if (checksum != rx_checksum)
		{
			parser_state = PARSER_ERROR;
			error_state = CHECKSUM_ERROR;
			workflow_pass = false;
		}
	}

	for (uint8_t i = 0; i < commands_size; i++)
	{
		if (rx_command == available_commands[i])
		{
			command_state = rx_command;
		}
	}

	if(workflow_pass && command_state == COMMAND_WAIT)
	{
		parser_state = PARSER_ERROR;
		error_state = INVALID_COMMAND_ERROR;
		workflow_pass = false;
	}


	if (workflow_pass)
	{
		if (command_state == 0x01)
		{
			command_length = 0x00U;
			command_payload_1_max = 0x01U;
			command_payload_1_min = 0x00U;
			response_length = 0x01U;
			workflow_pass = command_details_check();

			if(workflow_pass)
			{
				command_value[0] = 0x01U;
			}
			else
			{
				response_length = 0x00U;
			}
		}

		else if (command_state == 0x02)
		{
			command_length = 0x01U;
			command_payload_1_max = 0x01U;
			command_payload_1_min = 0x00U;
			response_length = 0x01U;
			workflow_pass = command_details_check();
			if(workflow_pass)
			{
				if(rx_payload[0] == 0x00)
				{
					HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
					if (HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin) == GPIO_PIN_RESET)
					{
						command_value[0] = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
					}
					else
					{
						parser_state = PARSER_ERROR;
						error_state = SET_LED_WRITE_ERROR;
						response_length = 0x00U;
						workflow_pass = false;
					}
				}
				else
				{
					HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
					if (HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin) == GPIO_PIN_SET)
					{
						command_value[0] = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
					}
					else
					{
						parser_state = PARSER_ERROR;
						error_state = SET_LED_WRITE_ERROR;
						response_length = 0x00U;
						workflow_pass = false;
					}
				}
			}
			else
			{
				response_length = 0x00U;
			}
		}

		else if (command_state == 0x03)
		{
			command_length = 0x00U;
			command_payload_1_max = 0U;
			command_payload_1_min = 0U;
			response_length = 0x01U;
			workflow_pass = command_details_check();
			if(workflow_pass)
			{
				command_value[0] = HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
			}
			else
			{
				response_length = 0x00U;
				workflow_pass = false;
			}

		}

		else if (command_state == 0x04)
		{
			command_length = 0x00U;
			command_payload_1_max = 0U;
			command_payload_1_min = 0U;
			response_length = 0x02U;
			workflow_pass = command_details_check();
			if(workflow_pass)
			{
				workflow_pass = adc_value_check();
				if(workflow_pass == false)
				{
					parser_state = PARSER_ERROR;
					error_state = ADC_READ_ERROR;
					response_length = 0x00U;
				}
			}
			else
			{
				response_length = 0x00U;
			}
		}
	}

	return workflow_pass;
}

void parserDataPipeline(uint8_t byte)
{
	switch(parser_state)
		{
		case PARSER_WAIT_START:
			if (byte == 0xAAU)
			{
				parser_state = PARSER_READ_COMMAND;
				if(OPERATING_MODE)
				{
					uart_send("AA\r\n");
					command_msg();
					uart_send("[STATUS: AWAITING_COMMAND] Please enter a valid command:");
				}
				timeout_count = HAL_GetTick();
			}
			break;

		case PARSER_READ_COMMAND:
			rx_command = byte;
			parser_state = PARSER_READ_LENGTH;
			if(OPERATING_MODE)
			{
				uart_send_hex(rx_command);
				uart_send("\r\n[STATUS: AWAITING_LENGTH] Please enter the payload length (in bytes):");
			}
			timeout_count = HAL_GetTick();
			break;

		case PARSER_READ_LENGTH:
			rx_length = byte;
			if(OPERATING_MODE)
			{
				uart_send_hex(rx_length);
			}
			if (rx_length == 0x00U)
			{
				parser_state = PARSER_READ_CALC_CHECKSUM;
				if(OPERATING_MODE)
				{
					uart_send("\r\n[STATUS: AWAITING_CHECKSUM] Please enter the checksum value for verification:");
				}
			}
			else if(rx_length <= PAYLOAD_SIZE)
			{
				parser_state = PARSER_READ_PAYLOAD;
				if(OPERATING_MODE)
				{
					uart_send("\r\n[STATUS: AWAITING_PAYLOAD] Please enter the hex data payload:");
				}
			}
			else
			{
				parser_state = PARSER_ERROR;
			}
			timeout_count = HAL_GetTick();
			break;

		case PARSER_READ_PAYLOAD:
			payload_recorder(byte);
			if(OPERATING_MODE)
			{
				uart_send_hex(byte);
			}
			if(payload_index >= rx_length)
			{
				parser_state = PARSER_READ_CALC_CHECKSUM;
				if(OPERATING_MODE)
				{
					uart_send("\r\n[STATUS: AWAITING_CHECKSUM] Please enter the checksum value for verification:");
				}
			}
			timeout_count = HAL_GetTick();
			break;

		case PARSER_READ_CALC_CHECKSUM:
			rx_checksum = byte;
			if(OPERATING_MODE)
			{
				uart_send_hex(rx_checksum);
				uart_send("\r\n");
			}
			parser_state = PARSER_HANDLER;
			break;

		default:
			break;
		}
}

uint8_t calc_checksum_response(uint8_t data[], uint8_t size)
{
	uint8_t sum = 0x00U;

	for (uint8_t i = 1; i < size; i++)
	{
		sum ^= data[i];
	}

	return sum;
}

void send_packet(void)
{
	uint8_t response_pkt[10];
	uint8_t response_index = 0U;

	if(error_state != NO_ERROR)
	{
		response_length = 0x00U;
	}

	response_pkt[response_index] = 0xAA;
	response_index++;
	response_pkt[response_index] = rx_command;
	response_index++;
	response_pkt[response_index] = error_state;
	response_index++;
	response_pkt[response_index] = response_length;
	response_index++;

	for(uint8_t i = 0; i < response_length; i++)
	{
		response_pkt[response_index] = command_value[i];
		response_index++;
	}

	response_pkt[response_index] = calc_checksum_response(response_pkt, response_index);
	response_index++;

	uart_hex(response_pkt, response_index);
}

void resetState(void)
{
	rx_command = 0U;
	rx_length = 0U;
	payload_index = 0U;
	for (uint8_t i = 0; i < PAYLOAD_SIZE; i++)
	{
		rx_payload[i] = '\0';
	}
	rx_checksum = 0U;

	for(uint8_t i = 0; i < PAYLOAD_SIZE; i++)
	{
		command_value[i] = 0x00U;
	}
	command_length = 0U;
	command_payload_1_max = 0U;
	command_payload_1_min = 0U;
	response_length = 0U;

	parser_state = PARSER_WAIT_START;
	error_state = NO_ERROR;
	command_state = COMMAND_WAIT;
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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  if(OPERATING_MODE)
  {
	  char boot_msg[] = "/----BOOT_OK----/\r\n";
	  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
	  uart_send(boot_msg_head);
	  uart_send(boot_msg);
	  uart_send("\r\n[STATUS: AWAITING_START] Please enter 'AA' to initiate the packet:");
  }

  if (HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
  {
	  Error_Handler();
  }

bool success = false;
bool start_msg_is = false;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if(OPERATING_MODE)
	  {
		  if (start_msg_is)
		  {
			  uart_send("\r\n\r\n[STATUS: AWAITING_START] Please enter 'AA' to initiate the packet:");
			  start_msg_is = false;
		  }
	  }

	  if(parser_state != PARSER_WAIT_START && (HAL_GetTick() - timeout_count) >= TIMEOUT_TIME)
	  {
		  parser_state = PARSER_ERROR;
		  error_state = TIMEOUT_ERROR;

		  if(OPERATING_MODE)
		  {
			  FailHandler();
		  }
		  else
		  {
			  send_packet();
			  resetState();
		  }
		  start_msg_is = true;
	  }

	  if (rx_byte_ready == true)
	  {
		  parserDataPipeline(rx_byte);

		  if(parser_state == PARSER_HANDLER || parser_state == PARSER_ERROR)
		  {
			  success = WorkflowManager();


			  if(OPERATING_MODE)
			  {
				  if (success == true)
				  {
					  SuccessHandler();
					  resetState();
				  }
				  else
				  {
					  FailHandler();
					  resetState();
				  }

			  }
			  else
			  {
				  send_packet();
				  resetState();
			  }
			  start_msg_is = true;
		  }

		  rx_byte_ready = false;

		  if (HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
		  {
			  Error_Handler();
		  }
	  }

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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
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
