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
#include "bmp280.h"

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

#define BMP280_ADRESS 0x76

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
	BMP280_DataRaw data_raw;
	BMP280_UncompensatedData uncompensated_data;
	BMP280_Status parse_status;

	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xF7, I2C_MEMADD_SIZE_8BIT, data_raw.bytes, BMP280_RAW_DATA_LENGTH, 100) == HAL_OK)
	{
		parse_status = BMP280_ParseRawData(&data_raw, &uncompensated_data);

		if(parse_status == BMP280_STATUS_OK)
		{
			char uncompensated_msg[50];

			snprintf(uncompensated_msg, sizeof(uncompensated_msg), "TEMPERATURE_RAW: %u\r\nPRESSURE_RAW: %u\r\n",
					(unsigned int)(uint32_t)uncompensated_data.adc_temperature,
					(unsigned int)(uint32_t)uncompensated_data.adc_pressure);

			uart_send(uncompensated_msg);

		}
	}
}

void readCalbData(void)
{

	BMP280_CalibrationRaw raw_calibration;
	BMP280_CalibrationData calibration_data;
	BMP280_Status parse_status;


	if(HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0x88, I2C_MEMADD_SIZE_8BIT, raw_calibration.bytes, BMP280_CALIBRATION_DATA_LENGTH, 100) == HAL_OK)
	{
		parse_status = BMP280_ParseCalibration(&raw_calibration, &calibration_data);
		if(parse_status == BMP280_STATUS_OK)
		{
			char temperature_msg[150];
			char pressure_msg[180];

			snprintf(temperature_msg, sizeof(temperature_msg),
					"CALIB_T_HEX: T1=0x%04X T2=0x%04X T3=0x%04X\r\n"
					"CALIB_T_DEC: T1=%u T2=%d T3=%d\r\n",
					(unsigned int)(uint16_t)calibration_data.dig_T1,
					(unsigned int)(uint16_t)calibration_data.dig_T2,
					(unsigned int)(uint16_t)calibration_data.dig_T3,
					(unsigned int)(uint16_t)calibration_data.dig_T1,
					(int16_t)calibration_data.dig_T2,
					(int16_t)calibration_data.dig_T3);

			snprintf(pressure_msg, sizeof(pressure_msg),
					"CALIB_P1_DEC: P1=%u P2=%d P3=%d\r\n"
					"CALIB_P2_DEC: P4=%d P5=%d P6=%d\r\n"
					"CALIB_P3_DEC: P7=%d P8=%d P9=%d\r\n",
					(unsigned int)(uint16_t)calibration_data.dig_P1,
					(int16_t)calibration_data.dig_P2,
					(int16_t)calibration_data.dig_P3,
					(int16_t)calibration_data.dig_P4,
					(int16_t)calibration_data.dig_P5,
					(int16_t)calibration_data.dig_P6,
					(int16_t)calibration_data.dig_P7,
					(int16_t)calibration_data.dig_P8,
					(int16_t)calibration_data.dig_P9);


			uart_send(temperature_msg);
			uart_send(pressure_msg);
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
