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
	PARSER_READ_CHECKSUM,
	PARSER_CALCULATE_CHECKSUM,
	PARSER_ERROR
}Parser_t;

typedef enum
{
	NO_ERROR,
	LENGTH_ERROR,
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

Parser_t parser_state = PARSER_WAIT_START;
ErrorMode_t error_state = NO_ERROR;

static const uint8_t test_data1[] = {0xAAU, 0x03U, 0x04U};
static const uint8_t test_data2[] = {0xAAU, 0x03U, 0x01U, 0x01U, 0x03U};
static const uint8_t test_data3[] = {0xAAU, 0x03U, 0x04U, 0xAAU, 0x03U, 0x01U, 0x01U, 0x03U};
static const uint8_t test_data4[] = {0xAAU, 0x03U, 0x04U, 0x55U, 0x66U, 0xAAU, 0x03U, 0x01U, 0x01U, 0x03U};
static const uint8_t test_data5[] = {0xAAU, 0x03U, 0x01U, 0x01U, 0x03U, 0xAAU, 0x03U, 0x00U, 0x03U};
static const uint8_t test_data6[] = {0xAAU, 0x03U, 0x01U, 0x01U, 0x00U,  0xAAU, 0x03U, 0x00U, 0x03U};

static uint8_t command = 0U;
static uint8_t length = 0U;
static uint8_t payload[PAYLOAD_SIZE];
static uint8_t payload_index = 0U;
static uint8_t checksum_read = 0U;
static bool checksum_is_true = false;

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

void uart_send_int(uint8_t number)
{
	char buffer[12];

	int len = sprintf(buffer, "%d", number);

	HAL_UART_Transmit(&huart2, (uint8_t* )buffer, len, HAL_MAX_DELAY);
}

void payload_recorder(uint8_t byte)
{
		payload[payload_index] =  byte;
		payload_index++;
}

void parser_process_byte(uint8_t byte)
{
	switch (parser_state)
	{
	case PARSER_WAIT_START:
		if(byte == 0xAAU)
		{
			parser_state = PARSER_READ_COMMAND;
		}
		break;

	case PARSER_READ_COMMAND:
		command = byte;
		parser_state = PARSER_READ_LENGTH;
		break;

	case PARSER_READ_LENGTH:
		length = byte;
		if(length == 0x00)
		{
			parser_state = PARSER_READ_CHECKSUM;
		}

		else if (length > PAYLOAD_SIZE)
		{
			parser_state = PARSER_ERROR;
			error_state = LENGTH_ERROR;
		}
		else
		{
		parser_state = PARSER_READ_PAYLOAD;
		}
		break;

	case PARSER_READ_PAYLOAD:
		if (PAYLOAD_SIZE >= length)
		{
			if (payload_index < length)
			{
				payload_recorder(byte);

				if(payload_index >= length)
				{
					parser_state = PARSER_READ_CHECKSUM;
				}
			}
		}
		break;

	case PARSER_READ_CHECKSUM:
		checksum_read = byte;
		parser_state = PARSER_CALCULATE_CHECKSUM;
		break;

	default:
		break;
	}
}

bool calculate_checksum(void)
{
	uint8_t sum = 0U;

	sum = command ^ length;

	if (length > 0)
	{
		for(uint8_t i = 0; i < length; i++)
		{
			sum = sum ^ payload[i];
		}
	}

	if(sum == checksum_read)
	{
		return true;
	}

	else
	{
		return false;
	}
}

void cleaning_var(void)
{
	parser_state = PARSER_WAIT_START;
	error_state = NO_ERROR;
	command = 0U;
	length = 0U;
	payload_index = 0U;
	checksum_read = 0U;
	checksum_is_true = false;
}

void error_msg(uint8_t error)
{
	if(error == 1U)
	{
		uart_send("ERROR_STATE_3:PASS VALUE: LENGTH_OVERFLOW");
	}
}

void process(const uint8_t *data, uint8_t size)
{
	parser_state = PARSER_WAIT_START;
	error_state = NO_ERROR;
	command = 0U;
	length = 0U;
	payload_index = 0U;
	checksum_read = 0U;
	checksum_is_true = false;

	bool state1 = false;
	bool state2 = false;
	bool state3 = false;
	bool state4 = false;
	bool state5 = false;

	bool state3_control = false;

	uint8_t error_stage_number = 0U;

	for(uint8_t i = 0; i < size; i++)
	{
		parser_process_byte(data[i]);

		if (error_state == NO_ERROR)
		{

			if (parser_state == PARSER_READ_COMMAND && state1 == false)
			{
				state1 = true;
				uart_send("PARSER_STATE_1:PASS VALUE:AA\r\n");
			}

			if (parser_state == PARSER_READ_LENGTH && state1 == true)
			{
				state2 = true;
				uart_send("PARSER_STATE_2:PASS VALUE:");
				uart_send_int(command);
				uart_send("\r\n");
			}

			if (parser_state == PARSER_READ_CHECKSUM && state2 == true && state3_control == false)
			{
				state3 = true;
				state3_control = true;
				uart_send("PARSER_STATE_3:PASS VALUE:");
				uart_send_int(length);
				uart_send("\r\n");
			}

			else if (parser_state == PARSER_READ_PAYLOAD && state2 == true && state3_control == false)
			{
				state3 = true;
				state3_control = true;
				uart_send("PARSER_STATE_3:PASS VALUE:");
				uart_send_int(length);
				uart_send("\r\n");
			}

			if (parser_state == PARSER_READ_CHECKSUM && state3 == true)
			{
				state4 = true;
				uart_send("PARSER_STATE_4:PASS VALUE:");
				if (length == 0x00)
				{
					uart_send("NO_PAYLOAD");
				}
				else
				{
					for(uint8_t i = 0; i < length; i++)
					{
						uart_send("[");
						uart_send_int(i);
						uart_send("]:");
						uart_send_int(payload[i]);
						uart_send(" ");
					}
				}

				uart_send("\r\n");
			}

			if (parser_state == PARSER_CALCULATE_CHECKSUM && state4 == true)
			{
				state5 = true;
				uart_send("PARSER_STATE_5:PASS VALUE:");
				uart_send_int(checksum_read);
				uart_send("\r\n");
			}

			if (parser_state == PARSER_CALCULATE_CHECKSUM && state5 == true)
			{
				checksum_is_true = calculate_checksum();
				uart_send("PARSER_STATE_6:PASS VALUE:");

				if(checksum_is_true)
				{
					uart_send("TRUE");
				}
				else
				{
					uart_send("FALSE");
				}

				uart_send("\r\n");

				cleaning_var();

				state1 = false;
				state2 = false;
				state3 = false;
				state4 = false;
				state5 = false;

				state3_control = false;

				uart_send("\r\n");
			}

		}

		if(parser_state == PARSER_ERROR)
		{
			if(error_state == LENGTH_ERROR && state2 == true)
			{
				error_stage_number = 1U;
				error_msg(error_stage_number);
				uart_send("\r\n");

				cleaning_var();

				state1 = false;
				state2 = false;
				state3 = false;
				state4 = false;
				state5 = false;

				state3_control = false;

				uart_send("\r\n");
			}
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  uart_send("FIRST_PACKET: AA 03 04\r\n");
  process(test_data1, sizeof(test_data1) / sizeof(test_data1[0]));

  uart_send("\r\n\r\nSECOND_PACKET: AA 03 01 01 03\r\n");
  process(test_data2, sizeof(test_data2) / sizeof(test_data2[0]));

  uart_send("\r\n\r\nTHIRD_PACKET: AA 03 04  AA 03 01 01 03\r\n");
  process(test_data3, sizeof(test_data3) / sizeof(test_data3[0]));

  uart_send("\r\n\r\nFOURTH_PACKET: AA 03 04 55 66 AA 03 01 01 03\r\n");
  process(test_data4, sizeof(test_data4) / sizeof(test_data4[0]));

  uart_send("\r\n\r\nFIFTH_PACKET: AA 03 01 01 03  AA 03 00 03\r\n");
  process(test_data5, sizeof(test_data5) / sizeof(test_data5[0]));

  uart_send("\r\n\r\nSIXTH_PACKET: AA 03 01 01 00  AA 03 00 03\r\n");
  process(test_data6, sizeof(test_data6) / sizeof(test_data6[0]));








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
