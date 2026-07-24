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
	NO_REQUEST,
	PING_REQUEST,
	VERSION_REQUEST,
	LED_REQUEST,
	LED_STATUS_REQUEST,
	ERROR_MODE
}RequestStatus_t;

typedef enum
{
	LED_WAIT,
	LED_SET,
	LED_RESET
}LedRequest_t;

typedef enum
{
	NO_ERROR,
	OVERFLOW_ERROR,
	INVALID_PAYLOAD_ERROR,
	UNKNOWN_COMMAND_ERROR
} ErrorStatus_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BUFFER_SIZE 32U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static volatile bool rx_byte_ready = false;
static uint8_t buffer[BUFFER_SIZE];
static uint8_t rx_msg_byte = 0U;
static uint8_t buffer_index = 0U;

static RequestStatus_t request = NO_REQUEST;
static ErrorStatus_t error = NO_ERROR;
static LedRequest_t led = LED_WAIT;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void led_request(void);
void error_mode(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void uart_send(char* text)
{
	HAL_UART_Transmit(&huart2, (uint8_t* )text, strlen(text), HAL_MAX_DELAY);
}

void byte_writer(void)
{
	buffer[buffer_index] = rx_msg_byte;
	buffer_index++;
}

bool overflow_check(void)
{
	if (buffer_index >= BUFFER_SIZE - 1U)
	{
		return true;
	}

	else
	{
		return false;
	}
}

bool enter_check(void)
{
	if ((rx_msg_byte == '\n') || (rx_msg_byte == '\r'))
	{
		return false;
	}

	else
	{
		return true;
	}
}

void msg_writer(void)
{
	bool enter = true;

	while (enter == true)
	{
		if (rx_byte_ready == true)
		{
			enter = enter_check();
			if (overflow_check())
			{
				request = ERROR_MODE;
				error = OVERFLOW_ERROR;
				rx_byte_ready = false;
			}

			else if (enter == true)
			{
				byte_writer();
				rx_byte_ready = false;
				HAL_UART_Transmit(&huart2, &rx_msg_byte, 1, HAL_MAX_DELAY);
			}

			if (HAL_UART_Receive_IT(&huart2, &rx_msg_byte, 1U) != HAL_OK)
			{
				Error_Handler();
			}
		}

	}

	char new_line[] = "\r\n";
	uart_send(new_line);
}

void request_check(void)
{
	if (error == NO_ERROR)
	{
		if (strcmp((char* )buffer, "PING") == 0)
		{
			request = PING_REQUEST;
		}

		else if (strcmp((char* )buffer, "GET_VERSION") == 0)
		{
			request = VERSION_REQUEST;
		}

		else if (strncmp((char* )buffer, "SET_LED ", sizeof("SET_LED")) == 0)
		{
			request = LED_REQUEST;
			led_request();
		}

		else if (strcmp((char* )buffer, "GET_LED") == 0)
		{
			request = LED_STATUS_REQUEST;
		}

		else
		{
			request = ERROR_MODE;
			error = UNKNOWN_COMMAND_ERROR;
		}
	}

}

void led_request(void)
{
	if (request == LED_REQUEST)
	{
		const char* payload = (char* )buffer + sizeof("SET_LED");

		if (strcmp(payload, "0") == 0)
		{
			led = LED_RESET;
		}

		else if (strcmp(payload, "1") == 0)
		{
			led = LED_SET;
		}

		else
		{
			led = LED_WAIT;
			error = INVALID_PAYLOAD_ERROR;
			request = ERROR_MODE;
		}
	}
}

void ping_mode(void)
{
	char ping_response_text[] = "PONG\r\n";
	uart_send(ping_response_text);
}

void version_mode(void)
{
	char version_response_text[] = "VERSION: V1.0.0\r\n";
	uart_send(version_response_text);
}

void update_led(void)
{
	if (led == LED_SET)
	{
		HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
		char led_turnon_text[] = "OK:LED_ON\r\n";
		uart_send(led_turnon_text);
	}

	else
	{
		HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
		char led_turnoff_text[] = "OK:LED_OFF\r\n";
		uart_send(led_turnoff_text);
	}
}

void led_status_check(void)
{
	if (HAL_GPIO_ReadPin(GREEN_LED_GPIO_Port, GREEN_LED_Pin) == GPIO_PIN_SET)
	{
		char led_on_status_text[] = "LED_STATUS:ON\r\n";
		uart_send(led_on_status_text);
	}

	else
	{
		char led_off_status_text[] = "LED_STATUS:OFF\r\n";
		uart_send(led_off_status_text);
	}
}

void response(void)
{
	switch(request)
	{
	case PING_REQUEST:
		ping_mode();
		break;

	case VERSION_REQUEST:
		version_mode();
		break;

	case LED_REQUEST:
		update_led();
		break;

	case LED_STATUS_REQUEST:
		led_status_check();
		break;

	default:
		error_mode();
		break;
	}
}

void overflow_error(void)
{
	char overflow_error_text[] = "RX_OVERFLOW\r\n";
	uart_send(overflow_error_text);
}

void invalid_payload_error(void)
{
	char invalid_payload_error_text[] = "INVALID_PAYLOAD\r\n";
	uart_send(invalid_payload_error_text);
}

void unknown_command_error(void)
{
	char unknown_command_error_text[] = "UNKNOWN_COMMAND\r\n";
	uart_send(unknown_command_error_text);
}

void error_mode(void)
{
	char error_mode_head_text[] = "ERR:";
	uart_send(error_mode_head_text);

	switch (error)
	{
	case OVERFLOW_ERROR:
		overflow_error();
		break;

	case INVALID_PAYLOAD_ERROR:
		invalid_payload_error();
		break;

	case UNKNOWN_COMMAND_ERROR:
		unknown_command_error();
		break;

	default:
		break;
	}

}

void cleaning(void)
{
	rx_msg_byte = 0U;
	buffer_index = 0U;
	request = NO_REQUEST;
	error = NO_ERROR;
	led = LED_WAIT;
	rx_byte_ready = false;

	for (uint8_t i = 0; i < BUFFER_SIZE; i++)
	{
		buffer[i] = '\0';
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

  if (HAL_UART_Receive_IT(&huart2, &rx_msg_byte, 1U) != HAL_OK)
  {
	  Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  msg_writer();
	  request_check();
	  response();
	  cleaning();


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
