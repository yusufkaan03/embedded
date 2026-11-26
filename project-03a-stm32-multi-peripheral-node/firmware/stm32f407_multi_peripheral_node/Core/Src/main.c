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

/*#define SPI_READ_DATA	1U
#define SPI_WRITE_DATA	0U

#define INFO_01_REG		0x0DU
#define INFO_02_REG		0x0EU
#define WHO_AM_I_REG	0x0FU
#define CTRL_REG4_REG	0x20U
#define STATUS_REG		0x27U
#define OUT_X_L_REG		0x28U
#define OUT_X_H_REG		0x29U
#define OUT_Y_L_REG		0x2AU
#define OUT_Y_H_REG		0x2BU
#define OUT_Z_L_REG		0x2CU
#define OUT_Z_H_REG		0x2DU

#define OUT_AXIS_REG	0x28U
#define X_AXIS			0x00U
#define Y_AXIS			0x02U
#define Z_AXIS			0x04U

#define MAX_SPI_SIZE	64U*/

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

void uart_send_int_list_hex(uint8_t *data, uint8_t data_size)
{
	uint8_t char_size = (data_size*5U) + 1U;
	char output_str[char_size];

	for (int i = 0; i < data_size; i++) {
	    snprintf(output_str + (i * 5), sizeof(output_str) - (i * 5), "0x%02X ", data[i]);
	}

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void uart_send_int_hex(uint8_t data)
{
	char output_str[5];

	snprintf(output_str, sizeof(output_str), "0x%02X", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void uart_send_int(uint8_t data)
{
	char output_str[5];

	snprintf(output_str, sizeof(output_str), "%d", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void uart_send_uint16_hex(uint16_t data)
{
	char output_str[7];

	snprintf(output_str, sizeof(output_str), "0x%04X", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

void uart_send_int16(int16_t data)
{
	char output_str[7];

	snprintf(output_str, sizeof(output_str), "%d", data);

	HAL_UART_Transmit(&huart2, (uint8_t* )output_str, strlen(output_str), HAL_MAX_DELAY);
}

/*void buildSpiPacket(uint8_t *tx_data, const uint8_t mode, const uint8_t adress, const uint8_t *data, const uint8_t size)
{
	tx_data[0] = (mode << 7) | (adress & 0x7FU);

	for(uint8_t i = 0; i<size; i++)
	{
		tx_data[i+1] = data[i];
	}
}

HAL_StatusTypeDef spiWriteRegister(const uint8_t reg_adress, const uint8_t *data, const uint8_t size)
{
	if(size>MAX_SPI_SIZE)
	{
		return HAL_ERROR;
	}
	uint8_t command_data[MAX_SPI_SIZE + 1] = {0};
	buildSpiPacket(command_data, SPI_WRITE_DATA, reg_adress, data, size);

	HAL_StatusTypeDef status;
	HAL_GPIO_WritePin(MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin, RESET);
	status = HAL_SPI_Transmit(&hspi1, command_data, size+1, 100);
	HAL_GPIO_WritePin(MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin, SET);

	return status;
}

HAL_StatusTypeDef spiReadRegister(const uint8_t reg_adress, uint8_t *response, const uint8_t size)
{
	uint8_t command_data[MAX_SPI_SIZE + 1] = {0};
	uint8_t data[MAX_SPI_SIZE] = {0};

	if (size > MAX_SPI_SIZE)
	{
		return HAL_ERROR;
	}

	uint8_t rx_msg[size+1];

	for(uint8_t i = 0; i<size+1; i++)
	{
		rx_msg[i] = 0xFFU;
	}

	buildSpiPacket(command_data, SPI_READ_DATA, reg_adress, data, size);

	HAL_StatusTypeDef status;
	HAL_GPIO_WritePin(MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin, RESET);
	status = HAL_SPI_TransmitReceive(&hspi1, command_data, rx_msg, size+1, 100);
	HAL_GPIO_WritePin(MOTION_SENSOR_CS_GPIO_Port, MOTION_SENSOR_CS_Pin, SET);

	if(status == HAL_OK)
	{
		for(uint8_t i = 0; i<size; i++)
		{
			response[i] = rx_msg[i+1];
		}
	}

	return status;
}

HAL_StatusTypeDef writeRegisterAndVerify(const uint8_t reg_adress, const uint8_t *data, uint8_t *response, const uint8_t size)
{
	HAL_StatusTypeDef status;

	status = spiWriteRegister(reg_adress, data, size);

	if(status != HAL_OK)
	{
		return status;
	}

	status = spiReadRegister(reg_adress, response, size);

	if(status != HAL_OK)
	{
		return status;
	}

	for(uint8_t i = 0; i<size; i++)
	{
		if(response[i] != data[i])
		{
			return HAL_ERROR;
		}
	}

		return status;
}


void rawToMg(const int16_t *values, int16_t *value_x, int16_t *value_y, int16_t *value_z)
{
	*value_x = (int16_t)((((int32_t)values[0]) * 6) / 100);
	*value_y = (int16_t)((((int32_t)values[1]) * 6) / 100);
	*value_z = (int16_t)((((int32_t)values[2]) * 6) / 100);
}

HAL_StatusTypeDef readRawValues(const uint8_t axis, const uint8_t size, int16_t *values)
{
	HAL_StatusTypeDef status;

	uint8_t raw_values[6] = {0};
	uint8_t buffer_size;
	uint8_t first_axis;

	bool axis_ok = false;
	bool size_ok = false;

	if(axis == X_AXIS && axis == Y_AXIS && axis == Z_AXIS)
	{
		axis_ok = true;
	}
	else
	{
		return HAL_ERROR;
	}

	if(axis_ok)
	{
		if(size == 1U)
		{
			size_ok = true;
		}
		else if((size + axis) == 3U)
		{
			size_ok = true;
		}
		else
		{
			size_ok = false;
		}
	}

	if(size_ok == true || axis_ok == true)
	{
		first_axis = OUT_AXIS_REG + axis;
		buffer_size = (size*2U);
	}
	else
	{
		return HAL_ERROR;
	}

	status = spiReadRegister(first_axis, raw_values, buffer_size);
	if(status != HAL_OK)
	{
		return HAL_ERROR;
	}

	for(uint8_t i = 0; i < size; i++)
	{
		values[i] = (int16_t)((uint16_t)(raw_values[(i*2)+1] << 8) | (uint16_t)(raw_values[i*2]));
	}

	return status;
}

HAL_StatusTypeDef readAxisRaw(const uint8_t axis, int16_t *value)
{
	HAL_StatusTypeDef status;

	status = readRawValues(axis, 1U, value);

	if(status != HAL_OK)
	{
		return HAL_ERROR;
	}

	return status;
}

HAL_StatusTypeDef readAxesRaw(int16_t *values)
{
	HAL_StatusTypeDef status;

	status = readRawValues(X_AXIS, 3U, values);

	if(status != HAL_OK)
	{
		return HAL_ERROR;
	}

	return status;
}

HAL_StatusTypeDef readAxesMg(int16_t *value_x, int16_t *value_y, int16_t *value_z)
{
	HAL_StatusTypeDef status;

	int16_t values[3] = {0};

	status = readAxesRaw(values);

	if(status != HAL_OK)
	{
		return HAL_ERROR;
	}

	rawToMg(values, value_x, value_y, value_z);

	return status;
}*/



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

  /*__HAL_SPI_ENABLE(&hspi1);*/

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  Lis3dsh_StatusTypeDef status;

  Lis3dsh_t sensor = {0};

  /*sensor.hspi = &hspi1;
  sensor.gpıo.gpio_port = GPIOE;
  sensor.gpıo.gpio_pin = GPIO_PIN_3;

  uint8_t response = 0xFF;

  status = spiReadRegister(&sensor, WHO_AM_I_REG, &response, 1U);
  if(status == LIS3DSH_OK)
  {
	  uart_send("WHO_AM_I: ");
	  uart_send_int_hex(response);
	  uart_send("\r\n");
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  uint8_t data = 0x6f;


  status = writeRegisterAndVerify(&sensor, CTRL_REG4_REG, &data, &response, 1U);
  if(status == LIS3DSH_OK)
  {
	  uart_send("CTRL_REG4: ");
	  uart_send_int_hex(response);
	  uart_send("\r\n");
  }
  else
  {
	  uartPrintHalStatus(status);
  }*/

  status = sensorInit(&sensor, &hspi1, GPIOE, GPIO_PIN_3);
  if(status == LIS3DSH_OK)
  {
	  uart_send("--------------------------------------\r\n");
	  uart_send("INIT_OK\r\n");
  }
  else
  {
	  uart_send("----------------------------------------\r\n");
	  uartPrintHalStatus(status);
	  uart_send("\r\n");
	  uart_send_int(sensor.init_status);
  }


  int16_t x_mg = 0xFFU;
  int16_t y_mg = 0xFFU;
  int16_t z_mg = 0xFFU;

  char x_char[10], y_char[10], z_char[10];

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
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  /*uint8_t data_0x6_f = 0x6FU;
  uint8_t response = 0x00U;

  status = spiReadRegister(&sensor, WHO_AM_I_REG, &response, 1U);
  if(status == LIS3DSH_INIT_OK)
  {
   uart_send("\r\nWHO_AM_I_VALUE: ");
   uart_send_int_hex(response);
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  status = spiReadRegister(&sensor, INFO_01_REG, &response, 1U);
  if(status == LIS3DSH_INIT_OK)
  {
	  uart_send("\r\nINFO_01_REG_VALUE: ");
	  uart_send_int_hex(response);
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  status = spiReadRegister(&sensor, INFO_02_REG, &response, 1U);
  if(status == LIS3DSH_INIT_OK)
  {
	  uart_send("\r\nINFO_02_REG_VALUE: ");
	  uart_send_int_hex(response);
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  status = spiReadRegister(&sensor, CTRL_REG4_REG, &response, 1U);
  if(status == LIS3DSH_INIT_OK)
  {
	  uart_send("\r\nCTRL_REG4_REG_VALUE: ");
	  uart_send_int_hex(response);
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  status = writeRegisterAndVerify(&sensor, CTRL_REG4_REG, &data_0x6_f, &response, 1U);
  if(status == LIS3DSH_INIT_OK)
  {
	  uart_send("\r\nCTRL_REG4_VALUE: ");
	  uart_send_int_hex(response);
  }
  else
  {
	  uartPrintHalStatus(status);
  }

  uart_send("\r\n-----------------------------");

  int16_t x_mg, y_mg, z_mg;

  status = readAxesMg(&sensor, &x_mg, &y_mg, &z_mg);

  if(status == LIS3DSH_INIT_OK)
  {
	  uart_send("\r\nX_AXIS_MG: ");
	  uart_send_int16(x_mg);
	  uart_send("\r\nY_AXIS_MG: ");
	  uart_send_int16(y_mg);
	  uart_send("\r\nZ_AXIS_MG: ");
	  uart_send_int16(z_mg);

	  char x_g[10];
	  char y_g[10];
	  char z_g[10];

	  uart_send("\r\n----------------------");

	  mgToGCharFix(x_g, 10, x_mg);
	  mgToGCharFix(y_g, 10, y_mg);
	  mgToGCharFix(z_g, 10, z_mg);

	  uart_send("\r\nX_AXIS_G: ");
	  uart_send(x_g);
	  uart_send("\r\nY_AXIS_G: ");
	  uart_send(y_g);
	  uart_send("\r\nZ_AXIS_G: ");
	  uart_send(z_g);

  }*/





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
