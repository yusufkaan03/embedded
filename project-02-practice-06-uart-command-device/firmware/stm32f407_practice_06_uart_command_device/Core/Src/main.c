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
	NO_ERROR,
	LENGTH_ERROR,
	CHECKSUM_ERROR
}ErrorMode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PAYLOAD_SIZE 3U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint8_t rx_byte = 0U;
volatile static bool rx_byte_ready = false;

static uint8_t rx_command = 0U;
static uint8_t rx_length = 0U;
static uint8_t rx_payload[PAYLOAD_SIZE];
static uint8_t payload_index = 0U;
static uint8_t rx_checksum = 0U;

static Parser_t parser_state = PARSER_WAIT_START;
static ErrorMode_t error_state = NO_ERROR;

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

void uart_send_byte(uint8_t byte)
{
	HAL_UART_Transmit(&huart2, &byte, 1U, HAL_MAX_DELAY);
}

void payload_recoder(uint8_t byte)
{
	rx_payload[payload_index] = byte;
	payload_index++;
}

uint8_t calc_checksum(void)
{
	uint8_t sum = 0U;

	sum = rx_command ^ rx_length;

	for(uint8_t i = 0; i < rx_length; i++)
	{
		sum = sum ^ rx_payload[i];
	}

	return sum;
}

void SuccessHandler(void)
{
	uart_send("RX_COMMAND:");
	uart_send_byte(rx_command);
	uart_send("\r\n_RX_LENGTH:");
	uart_send_byte(rx_length);
	uart_send("\r\nRX_PAYLOAD:");

	if (rx_length == 0x00U)
	{
		uart_send("NO_PAYLOAD!");
	}
	else
	{
		for (uint8_t i = 0U; i < rx_length; i++)
		{
			uart_send_byte(rx_payload[i]);
		}
	}

	uart_send("\r\nRX_CHECKSUM:");
	uart_send_byte(rx_checksum);
	uart_send("\r\nRX_RESULT:TRUE\r\n");

}

void FailHandler(void)
{
	if(parser_state == PARSER_ERROR && error_state == LENGTH_ERROR)
	{
		uart_send("LENGTH_ERROR\r\n");
	}

	else if (parser_state == PARSER_ERROR && error_state == CHECKSUM_ERROR)
	{
		uart_send("CHECKSUM_ERROR\r\n");
	}

	uart_send("RESULT:FALSE\r\n");

}

bool WorkflowManager(void)
{
	bool workflow_pass = true;
	uint8_t checksum = 0U;

	if (rx_length > PAYLOAD_SIZE)
	{
		parser_state = PARSER_ERROR;
		error_state = LENGTH_ERROR;
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
			}
			break;

		case PARSER_READ_COMMAND:
			uart_send("command\r\n");
			rx_command = byte;
			parser_state = PARSER_READ_LENGTH;
			break;

		case PARSER_READ_LENGTH:
			uart_send("length\r\n");
			rx_length = byte;
			if (rx_length == 0x00U)
			{
				parser_state = PARSER_READ_CALC_CHECKSUM;
			}
			else if(rx_length <= PAYLOAD_SIZE)
			{
				parser_state = PARSER_READ_PAYLOAD;
			}
			else
			{
				parser_state = PARSER_ERROR;
			}
			break;

		case PARSER_READ_PAYLOAD:
			payload_recoder(byte);
			uart_send("record\r\n");
			if(payload_index >= rx_length)
			{
				parser_state = PARSER_READ_CALC_CHECKSUM;
			}
			break;

		case PARSER_READ_CALC_CHECKSUM:
			uart_send("checsum\r\n");
			rx_checksum = byte;
			parser_state = PARSER_HANDLER;
			break;

		default:
			break;
		}
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

	parser_state = PARSER_WAIT_START;
	error_state = NO_ERROR;
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

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  if (HAL_UART_Receive_IT(&huart2, &rx_byte, 1U) != HAL_OK)
  {
	  Error_Handler();
  }

bool success = false;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  if (rx_byte_ready == true)
	  {

		  parserDataPipeline(rx_byte);

		  if(parser_state == PARSER_HANDLER || parser_state == PARSER_ERROR)
		  {
			  success = WorkflowManager();
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
