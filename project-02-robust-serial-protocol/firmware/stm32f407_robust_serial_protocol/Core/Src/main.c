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
#include <stdbool.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    PROTOCOL_CMD_PING          = 0x01U,
    PROTOCOL_CMD_GET_VERSION   = 0x02U,
    PROTOCOL_CMD_SET_LED       = 0x03U,
    PROTOCOL_CMD_GET_LED       = 0x04U,
    PROTOCOL_CMD_RESET_COUNTER = 0x05U
} ProtocolCommand_t;

typedef enum
{
    PROTOCOL_RESP_PONG          = 0x81U,
    PROTOCOL_RESP_VERSION       = 0x82U,
    PROTOCOL_RESP_LED_SET       = 0x83U,
    PROTOCOL_RESP_LED_STATE     = 0x84U,
    PROTOCOL_RESP_COUNTER_RESET = 0x85U,
    PROTOCOL_RESP_ERROR         = 0xFFU
} ProtocolResponse_t;

typedef enum
{
    PROTOCOL_ERROR_UNKNOWN_COMMAND = 0x01U,
    PROTOCOL_ERROR_INVALID_LENGTH  = 0x02U,
    PROTOCOL_ERROR_INVALID_PAYLOAD = 0x03U,
    PROTOCOL_ERROR_BAD_CHECKSUM    = 0x04U,
    PROTOCOL_ERROR_RX_TIMEOUT      = 0x05U,
    PROTOCOL_ERROR_BUFFER_OVERFLOW = 0x06U
} ProtocolError_t;

typedef enum
{
    PARSER_WAIT_START = 0,
    PARSER_READ_COMMAND,
    PARSER_READ_LENGTH,
    PARSER_READ_PAYLOAD,
    PARSER_READ_CHECKSUM
} ProtocolParserState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RX_BUFFER_SIZE 				32U

#define PROTOCOL_START_BYTE        0xAAU
#define PROTOCOL_MAX_PAYLOAD_SIZE  16U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t uart_rx_byte;
uint8_t rx_buffer[RX_BUFFER_SIZE];

volatile uint8_t rx_index = 0U;
volatile uint8_t line_ready = 0U;
volatile uint8_t discard_until_newline = 0U;
volatile uint8_t rx_overflow = 0U;
volatile uint8_t rx_rearm_error = 0U;

static uint8_t green_led_state = 0U;

static ProtocolParserState_t parser_state = PARSER_WAIT_START;
static uint8_t parser_command = 0U;

static uint8_t parser_length = 0U;
static uint8_t parser_payload_index = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static void uart_send_text(const char *text);
static void process_command(const char *command);

static uint8_t protocol_calculate_checksum(
    uint8_t command,
    uint8_t length,
    const uint8_t *payload
);

static bool protocol_checksum_self_test(void);

static void protocol_parser_reset(void);

static void protocol_parser_process_byte(uint8_t byte);

static bool protocol_parser_stage1_self_test(void);

static bool protocol_parser_stage2_self_test(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void uart_send_text(const char *text)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)text,
        strlen(text),
        HAL_MAX_DELAY
    );
}

static void process_command(const char *command)
{
    static const char set_led_prefix[] = "SET_LED ";
    const size_t set_led_prefix_length = sizeof(set_led_prefix) - 1U;

    if (strcmp(command, "PING") == 0)
    {
        uart_send_text("PONG\r\n");
    }
    else if (strcmp(command, "GET_VERSION") == 0)
    {
        uart_send_text("VERSION:1.0.0\r\n");
    }
    else if (strcmp(command, "GET_LED") == 0)
    {
        if (green_led_state == 1U)
        {
            uart_send_text("LED:1\r\n");
        }
        else
        {
            uart_send_text("LED:0\r\n");
        }
    }
    else if (strncmp(
                 command,
                 set_led_prefix,
                 set_led_prefix_length
             ) == 0)
    {
        const char *payload = command + set_led_prefix_length;

        if (strcmp(payload, "1") == 0)
        {
            HAL_GPIO_WritePin(
                GREEN_LED_GPIO_Port,
                GREEN_LED_Pin,
                GPIO_PIN_SET
            );

            green_led_state = 1U;
            uart_send_text("OK:LED_ON\r\n");
        }
        else if (strcmp(payload, "0") == 0)
        {
            HAL_GPIO_WritePin(
                GREEN_LED_GPIO_Port,
                GREEN_LED_Pin,
                GPIO_PIN_RESET
            );

            green_led_state = 0U;
            uart_send_text("OK:LED_OFF\r\n");
        }
        else
        {
            uart_send_text("ERR:INVALID_PAYLOAD\r\n");
        }
    }
    else if (strcmp(command, "SET_LED") == 0)
    {
        uart_send_text("ERR:INVALID_PAYLOAD\r\n");
    }
    else
    {
        uart_send_text("ERR:UNKNOWN_COMMAND\r\n");
    }
}

static uint8_t protocol_calculate_checksum(
    uint8_t command,
    uint8_t length,
    const uint8_t *payload
)
{
    uint8_t checksum = command ^ length;

    for (uint8_t index = 0U; index < length; index++)
    {
        checksum ^= payload[index];
    }

    return checksum;
}

static bool protocol_checksum_self_test(void)
{
    const uint8_t led_on_payload[] = {1U};
    const uint8_t led_off_payload[] = {0U};

    const uint8_t ping_checksum =
        protocol_calculate_checksum(
            PROTOCOL_CMD_PING,
            0U,
            NULL
        );

    const uint8_t led_on_checksum =
        protocol_calculate_checksum(
            PROTOCOL_CMD_SET_LED,
            1U,
            led_on_payload
        );

    const uint8_t led_off_checksum =
        protocol_calculate_checksum(
            PROTOCOL_CMD_SET_LED,
            1U,
            led_off_payload
        );

    if (ping_checksum != 0x01U)
    {
        return false;
    }

    if (led_on_checksum != 0x03U)
    {
        return false;
    }

    if (led_off_checksum != 0x02U)
    {
        return false;
    }

    return true;
}

static void protocol_parser_reset(void)
{
    parser_state = PARSER_WAIT_START;
    parser_command = 0U;
    parser_length = 0U;
    parser_payload_index = 0U;
}

static void protocol_parser_process_byte(uint8_t byte)
{
    switch (parser_state)
    {
        case PARSER_WAIT_START:

            if (byte == PROTOCOL_START_BYTE)
            {
                parser_state = PARSER_READ_COMMAND;
            }

            break;

        case PARSER_READ_COMMAND:

            parser_command = byte;
            parser_state = PARSER_READ_LENGTH;

            break;

        case PARSER_READ_LENGTH:

            parser_length = byte;
            parser_payload_index = 0U;

            if (parser_length > PROTOCOL_MAX_PAYLOAD_SIZE)
            {
                protocol_parser_reset();
            }
            else if (parser_length == 0U)
            {
                parser_state = PARSER_READ_CHECKSUM;
            }
            else
            {
                parser_state = PARSER_READ_PAYLOAD;
            }

            break;

        default:

            protocol_parser_reset();

            break;
    }
}

static bool protocol_parser_stage1_self_test(void)
{
    protocol_parser_reset();

    protocol_parser_process_byte(0x55U);

    if (parser_state != PARSER_WAIT_START)
    {
        protocol_parser_reset();
        return false;
    }

    protocol_parser_process_byte(PROTOCOL_START_BYTE);

    if (parser_state != PARSER_READ_COMMAND)
    {
        protocol_parser_reset();
        return false;
    }

    protocol_parser_process_byte(PROTOCOL_CMD_SET_LED);

    if (parser_command != PROTOCOL_CMD_SET_LED)
    {
        protocol_parser_reset();
        return false;
    }

    if (parser_state != PARSER_READ_LENGTH)
    {
        protocol_parser_reset();
        return false;
    }

    protocol_parser_reset();

    return true;
}

static bool protocol_parser_stage2_self_test(void)
{
    /*
     * Test 1:
     * PING has zero payload bytes.
     * Parser must continue directly to checksum.
     */
    protocol_parser_reset();

    protocol_parser_process_byte(PROTOCOL_START_BYTE);
    protocol_parser_process_byte(PROTOCOL_CMD_PING);
    protocol_parser_process_byte(0U);

    if (parser_length != 0U)
    {
        protocol_parser_reset();
        return false;
    }

    if (parser_state != PARSER_READ_CHECKSUM)
    {
        protocol_parser_reset();
        return false;
    }

    /*
     * Test 2:
     * SET_LED has one payload byte.
     * Parser must continue to payload state.
     */
    protocol_parser_reset();

    protocol_parser_process_byte(PROTOCOL_START_BYTE);
    protocol_parser_process_byte(PROTOCOL_CMD_SET_LED);
    protocol_parser_process_byte(1U);

    if (parser_length != 1U)
    {
        protocol_parser_reset();
        return false;
    }

    if (parser_state != PARSER_READ_PAYLOAD)
    {
        protocol_parser_reset();
        return false;
    }

    /*
     * Test 3:
     * Reject lengths larger than the maximum payload size.
     */
    protocol_parser_reset();

    protocol_parser_process_byte(PROTOCOL_START_BYTE);
    protocol_parser_process_byte(PROTOCOL_CMD_SET_LED);
    protocol_parser_process_byte(PROTOCOL_MAX_PAYLOAD_SIZE + 1U);

    if (parser_state != PARSER_WAIT_START)
    {
        protocol_parser_reset();
        return false;
    }

    protocol_parser_reset();

    return true;
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

  const uint8_t boot_msg[] = "BOOT_OK\r\n";

  HAL_UART_Transmit(&huart2, (uint8_t* )boot_msg, sizeof(boot_msg) - 1U, HAL_MAX_DELAY);

  if (protocol_checksum_self_test())
  {
      uart_send_text("CHECKSUM_TEST:PASS\r\n");
  }
  else
  {
      uart_send_text("CHECKSUM_TEST:FAIL\r\n");
  }

  if (protocol_parser_stage1_self_test())
  {
      uart_send_text("PARSER_STAGE1_TEST:PASS\r\n");
  }
  else
  {
      uart_send_text("PARSER_STAGE1_TEST:FAIL\r\n");
  }

  if (protocol_parser_stage2_self_test())
  {
      uart_send_text("PARSER_STAGE2_TEST:PASS\r\n");
  }
  else
  {
      uart_send_text("PARSER_STAGE2_TEST:FAIL\r\n");
  }

  if (HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1U) != HAL_OK)
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
	  if (line_ready == 1U)
	  {
	      process_command((char *)rx_buffer);

	      rx_index = 0U;
	      line_ready = 0U;
	  }

	  if (rx_overflow == 1U)
	  {
	      uart_send_text("ERR:RX_OVERFLOW\r\n");
	      rx_overflow = 0U;
	  }

	  if (rx_rearm_error == 1U)
	  {
	      Error_Handler();
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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (line_ready == 0U)
        {
            if ((uart_rx_byte == '\r') || (uart_rx_byte == '\n'))
            {
                if (discard_until_newline == 1U)
                {
                    discard_until_newline = 0U;
                    rx_index = 0U;
                    rx_overflow = 1U;
                }
                else if (rx_index > 0U)
                {
                    rx_buffer[rx_index] = '\0';
                    line_ready = 1U;
                }
            }
            else if (discard_until_newline == 0U)
            {
                if (rx_index < (RX_BUFFER_SIZE - 1U))
                {
                    rx_buffer[rx_index] = uart_rx_byte;
                    rx_index++;
                }
                else
                {
                    discard_until_newline = 1U;
                }
            }
        }

        if (HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1U) != HAL_OK)
        {
            rx_rearm_error = 1U;
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
