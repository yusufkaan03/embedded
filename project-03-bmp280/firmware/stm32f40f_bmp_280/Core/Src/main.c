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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include<stdio.h>
#include<string.h>
#include<stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static uint8_t f3 = 0x00U;
static uint8_t f4 = 0x00U;
static uint8_t f5 = 0x00U;

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

void ReadControlRegisters(bool uart_write)
{
	bool f3_ok = false;
	bool f4_ok = false;
	bool f5_ok = false;

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xF3, I2C_MEMADD_SIZE_8BIT, &f3, 1U, 100) == HAL_OK)
	{
		f3_ok = true;
	}

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xF4, I2C_MEMADD_SIZE_8BIT, &f4, 1U, 100) == HAL_OK)
	{
		f4_ok = true;
	}

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xF5, I2C_MEMADD_SIZE_8BIT, &f5, 1U, 100) == HAL_OK)
	{
		f5_ok = true;
	}

	if(uart_write)
	{
		if(f3_ok)
		{
			char f3_msg[20];
			snprintf(f3_msg, sizeof(f3_msg), "0xF3: 0x%02X\r\n", f3);
			uart_send(f3_msg);
		}

		if(f4_ok)
		{
			char f4_msg[20];
			snprintf(f4_msg, sizeof(f4_msg), "0xF4: 0x%02X\r\n", f4);
			uart_send(f4_msg);
		}

		if(f5_ok)
		{
			char f5_msg[20];
			snprintf(f5_msg, sizeof(f5_msg), "0xF5: 0x%02X\r\n", f5);
			uart_send(f5_msg);
		}



	}
}


bool WriteRegister(uint8_t address, uint8_t data)
{
	if(HAL_I2C_Mem_Write(&hi2c1, 0x76 << 1, address, I2C_MEMADD_SIZE_8BIT, &data, 1U, 100) == HAL_OK)
	{
		char write_ok_msg[30];

		snprintf(write_ok_msg, sizeof(write_ok_msg), "\r\n0x%02X_WRITE_OK: 0x%02X\r\n", address, data);

		uart_send(write_ok_msg);

		ReadControlRegisters(false);

		return true;

	}
	else
	{
		char write_fail_msg[30];

		snprintf(write_fail_msg, sizeof(write_fail_msg), "0x%02X_WRITE_FAIL: 0x%02X\r\n", address, data);

		uart_send(write_fail_msg);

		ReadControlRegisters(false);

		return false;
	}
}

void ReadRaw(void)
{
	uint8_t raw_data[6];

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xF7, I2C_MEMADD_SIZE_8BIT, (uint8_t* )raw_data, 6U, 100) == HAL_OK)
	{
		char raw_msg[64];

		snprintf(raw_msg, sizeof(raw_msg), "DATA_0xF7-0xFC: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",
				raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4], raw_data[5]);

		uart_send(raw_msg);
	}
}

void readCalbData(void)
{
	uint8_t calbdata[25];

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0x88, I2C_MEMADD_SIZE_8BIT, (uint8_t* )calbdata, 24U, 100) == HAL_OK)
	{
		char data_msg[150];

		snprintf(data_msg, sizeof(data_msg), "CALIB_RAW 0x88-0x9F: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
				" 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
				" 0x%02X 0x%02X 0x%02X\r\n", calbdata[0], calbdata[1], calbdata[2], calbdata[3], calbdata[4], calbdata[5],
				calbdata[6], calbdata[7], calbdata[8], calbdata[9], calbdata[10], calbdata[11], calbdata[12], calbdata[13],
				calbdata[14], calbdata[15], calbdata[16], calbdata[17], calbdata[18], calbdata[19], calbdata[20], calbdata[21],
				calbdata[22], calbdata[23]);

		uart_send(data_msg);
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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  char boot_msg[] = "/----BOOT_OK----/\r\n";
  char boot_msg_head[] = "\r\n\r\n/---------------/\r\n";
  uart_send(boot_msg_head);
  uart_send(boot_msg);

  uint8_t chip_id;

  HAL_StatusTypeDef results = HAL_I2C_IsDeviceReady(&hi2c1, 0x76 << 1, 3U, 100);
  if(results == HAL_OK)
  {
	  char ready_msg[] = "\r\nI2C_DEVICE_READY\r\n";
	  uart_send(ready_msg);

	  if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xD0, I2C_MEMADD_SIZE_8BIT, &chip_id, 1U, 100) == HAL_OK)
	  {
		  char id_msg[32];

		  if(chip_id == 0x58U)
		  {
			  snprintf(id_msg, sizeof(id_msg), "CHIP_ID_OK: 0x%02X\r\n", chip_id);
			  uart_send(id_msg);
		  }
		  else
		  {
			  snprintf(id_msg, sizeof(id_msg), "CHIP_ID_ERROR: 0x%02X\r\n", chip_id);
			  uart_send(id_msg);
		  }
	  }
	  else
	  {
		  char error_msg[] = "CHIP_ID: ERROR\r\n";

		  uart_send(error_msg);

	  }
  }
  else
  {
	  char ready_msg[] = "I2C_DEVICE_NOT_READY";
	  uart_send(ready_msg);
  }



  char reset_start_msg[] = "\r\nRESET_START\r\n";
  uart_send(reset_start_msg);

  if(WriteRegister(0xE0, 0xB6))
  {

	  HAL_Delay(5);

	  uint8_t im_update = f3 & 0x01U;
	  uint32_t last_time = HAL_GetTick();

	  do
	  {
		  ReadControlRegisters(false);
		  im_update = f3 & 0x01U;

	  } while(im_update != 0U && (HAL_GetTick() - last_time) < 20U);

	  if(im_update == 0U)
	  {
		  char reset_finish_msg[] = "RESET_FINISH\r\n";
		  uart_send(reset_finish_msg);

		  ReadControlRegisters(true);
		  ReadRaw();
	  }

	  else
	  {
		  char reset_timeout_msg[] = "RESET_TIMEOUT\r\n";
		  uart_send(reset_timeout_msg);

		  ReadControlRegisters(true);
		  ReadRaw();
	  }
  }


  char measuring_start_msg[] = "\r\nMEASURING_START\r\n";
  uart_send(measuring_start_msg);

  if(WriteRegister(0xF4U, 0x25U))
  {
	  HAL_Delay(5);

	  uint8_t measuring = f3 & 0x08U;
	  uint32_t last_time = HAL_GetTick();

	  do
	  {
		  ReadControlRegisters(false);
		  measuring = f3 & 0x08U;

	  } while(measuring != 0U && (HAL_GetTick() - last_time) < 20U);

	  if(measuring == 0U)
	  {
	  	  char measuring_finish_msg[] = "MEASURING_FINISH\r\n";
	  	  uart_send(measuring_finish_msg);

	  	  ReadControlRegisters(true);
	  	  ReadRaw();

	  	readCalbData();
	    }

	  else
	  {
		  char measuring_timeout_msg[] = "MEASURING_TIMEOUT\r\n";
		  uart_send(measuring_timeout_msg);

	 	  ReadControlRegisters(true);
	  	  ReadRaw();
	  }



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
