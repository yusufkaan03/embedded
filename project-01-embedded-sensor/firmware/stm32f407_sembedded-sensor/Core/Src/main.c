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

#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	ADC_LOW,
	ADC_OK,
	ADC_HIGH,
	ADC_ERROR
} AdcSituation_t;

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData_t;

typedef struct
{
    int32_t raw_temperature;
    int32_t raw_pressure;
} BMP280_RawData_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define UART_LOG_INTERVAL_MS		1000
#define ADC_SAMPLE_COUNT 			10
#define BME280_I2C_ADDR         	(0x77 << 1)
#define BME280_REG_CHIP_ID      	0xD0
#define BME280_CHIP_ID  			0x60
#define BMP280_CHIP_ID          	0x58
#define BMP280_REG_CALIB_START		0x88
#define BMP280_CALIB_DATA_LENGTH	24

#define BMP280_REG_STATUS           0xF3
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_PRESS_MSB        0xF7

#define BMP280_CONFIG_VALUE         0x00
#define BMP280_CTRL_MEAS_FORCED     0x25

#define BMP280_CTRL_MEAS_VALUE      0x27
#define BMP280_RAW_DATA_LENGTH      6

#define BMP280_REG_RESET      0xE0
#define BMP280_RESET_VALUE    0xB6

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

BMP280_CalibData_t bmp280_calib;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

HAL_StatusTypeDef bmp280_read_register(uint8_t reg_addr, uint8_t *value)
{
    return HAL_I2C_Mem_Read(
        &hi2c2,
        BME280_I2C_ADDR,
        reg_addr,
        I2C_MEMADD_SIZE_8BIT,
        value,
        1,
        HAL_MAX_DELAY
    );
}

void print_bmp280_debug_registers(void)
{
    uint8_t status = 0;
    uint8_t ctrl_meas = 0;
    uint8_t config = 0;
    char buffer[120];

    bmp280_read_register(BMP280_REG_STATUS, &status);
    bmp280_read_register(BMP280_REG_CTRL_MEAS, &ctrl_meas);
    bmp280_read_register(BMP280_REG_CONFIG, &config);

    int len = snprintf(
        buffer,
        sizeof(buffer),
        "BMP280 regs: status=0x%02X, ctrl_meas=0x%02X, config=0x%02X\r\n",
        status,
        ctrl_meas,
        config
    );

    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
}

HAL_StatusTypeDef bmp280_write_register(uint8_t reg_addr, uint8_t value)
{
    return HAL_I2C_Mem_Write(
        &hi2c2,
        BME280_I2C_ADDR,
        reg_addr,
        I2C_MEMADD_SIZE_8BIT,
        &value,
        1,
        HAL_MAX_DELAY
    );
}

HAL_StatusTypeDef bmp280_soft_reset(void)
{
    return bmp280_write_register(BMP280_REG_RESET, BMP280_RESET_VALUE);
}

HAL_StatusTypeDef bmp280_configure(void)
{
    return bmp280_write_register(BMP280_REG_CONFIG, BMP280_CONFIG_VALUE);
}

HAL_StatusTypeDef bmp280_trigger_forced_measurement(void)
{
    return bmp280_write_register(BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_FORCED);
}

HAL_StatusTypeDef bmp280_read_raw_data(BMP280_RawData_t *raw_data)
{
    uint8_t raw_buffer[BMP280_RAW_DATA_LENGTH];

    HAL_StatusTypeDef result = HAL_I2C_Mem_Read(
        &hi2c2,
        BME280_I2C_ADDR,
        BMP280_REG_PRESS_MSB,
        I2C_MEMADD_SIZE_8BIT,
        raw_buffer,
        BMP280_RAW_DATA_LENGTH,
        HAL_MAX_DELAY
    );

    if (result != HAL_OK)
    {
        return result;
    }

    char dbg[120];

    int dbg_len = snprintf(
        dbg,
        sizeof(dbg),
        "raw bytes: %02X %02X %02X %02X %02X %02X\r\n",
        raw_buffer[0],
        raw_buffer[1],
        raw_buffer[2],
        raw_buffer[3],
        raw_buffer[4],
        raw_buffer[5]
    );

    HAL_UART_Transmit(&huart2, (uint8_t*)dbg, dbg_len, HAL_MAX_DELAY);

    raw_data->raw_pressure =
        ((int32_t)raw_buffer[0] << 12) |
        ((int32_t)raw_buffer[1] << 4)  |
        ((int32_t)raw_buffer[2] >> 4);

    raw_data->raw_temperature =
        ((int32_t)raw_buffer[3] << 12) |
        ((int32_t)raw_buffer[4] << 4)  |
        ((int32_t)raw_buffer[5] >> 4);

    return HAL_OK;
}

void print_bmp280_raw_data(const BMP280_RawData_t *raw_data)
{
    char buffer[120];

    int len = snprintf(
        buffer,
        sizeof(buffer),
        "BMP280 raw: temp=%ld, press=%ld\r\n",
        (long)raw_data->raw_temperature,
        (long)raw_data->raw_pressure
    );

    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
}

uint16_t u16_from_lsb_msb(uint8_t lsb, uint8_t msb)
{
    return (uint16_t)((uint16_t)msb << 8 | lsb);
}

int16_t s16_from_lsb_msb(uint8_t lsb, uint8_t msb)
{
    return (int16_t)((uint16_t)msb << 8 | lsb);
}

HAL_StatusTypeDef bmp280_read_calibration(BMP280_CalibData_t *calib)
{
    uint8_t calib_raw[BMP280_CALIB_DATA_LENGTH];

    HAL_StatusTypeDef result = HAL_I2C_Mem_Read(
        &hi2c2,
        BME280_I2C_ADDR,
        BMP280_REG_CALIB_START,
        I2C_MEMADD_SIZE_8BIT,
        calib_raw,
        BMP280_CALIB_DATA_LENGTH,
        HAL_MAX_DELAY
    );

    if (result != HAL_OK)
    {
        return result;
    }

    calib->dig_T1 = u16_from_lsb_msb(calib_raw[0],  calib_raw[1]);
    calib->dig_T2 = s16_from_lsb_msb(calib_raw[2],  calib_raw[3]);
    calib->dig_T3 = s16_from_lsb_msb(calib_raw[4],  calib_raw[5]);

    calib->dig_P1 = u16_from_lsb_msb(calib_raw[6],  calib_raw[7]);
    calib->dig_P2 = s16_from_lsb_msb(calib_raw[8],  calib_raw[9]);
    calib->dig_P3 = s16_from_lsb_msb(calib_raw[10], calib_raw[11]);
    calib->dig_P4 = s16_from_lsb_msb(calib_raw[12], calib_raw[13]);
    calib->dig_P5 = s16_from_lsb_msb(calib_raw[14], calib_raw[15]);
    calib->dig_P6 = s16_from_lsb_msb(calib_raw[16], calib_raw[17]);
    calib->dig_P7 = s16_from_lsb_msb(calib_raw[18], calib_raw[19]);
    calib->dig_P8 = s16_from_lsb_msb(calib_raw[20], calib_raw[21]);
    calib->dig_P9 = s16_from_lsb_msb(calib_raw[22], calib_raw[23]);

    return HAL_OK;
}

void print_bmp280_calibration(const BMP280_CalibData_t *calib)
{
    char buffer[120];

    char title[] = "BMP280 calibration data:\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)title, strlen(title), HAL_MAX_DELAY);

    int len = snprintf(buffer, sizeof(buffer), "dig_T1=%u\r\n", calib->dig_T1);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_T2=%d\r\n", calib->dig_T2);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_T3=%d\r\n", calib->dig_T3);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P1=%u\r\n", calib->dig_P1);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P2=%d\r\n", calib->dig_P2);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P3=%d\r\n", calib->dig_P3);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P4=%d\r\n", calib->dig_P4);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P5=%d\r\n", calib->dig_P5);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P6=%d\r\n", calib->dig_P6);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P7=%d\r\n", calib->dig_P7);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P8=%d\r\n", calib->dig_P8);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);

    len = snprintf(buffer, sizeof(buffer), "dig_P9=%d\r\n", calib->dig_P9);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
}

uint16_t read_adc_average(void)
{
	uint32_t adc_sum = 0;

	for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++)
	{
		HAL_ADC_Start(&hadc1);

		if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
		{
			adc_sum = adc_sum + HAL_ADC_GetValue(&hadc1);
		}

		HAL_ADC_Stop(&hadc1);
	}

	return (uint16_t)(adc_sum/ADC_SAMPLE_COUNT);
}

uint8_t read_user_button(void)
{
	if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET)
	{
		return 1;
	}

	else
	{
		return 0;
	}
}

AdcSituation_t adc_situation(uint32_t adc_mv)
{
	if (adc_mv < 500U) return ADC_LOW;

	else if (adc_mv <= 3000U) return ADC_OK;

	else if (adc_mv <= 3300U) return ADC_HIGH;

	else return ADC_ERROR;
}

const char* get_situation_char(AdcSituation_t situation)
{
	switch (situation)
	{
	case ADC_LOW: return "LOW";

	case ADC_OK: return "OK";

	case ADC_HIGH: return "HIGH";

	case ADC_ERROR: return "ERROR";

	default: return "UNKNOWN";
	}
}

uint32_t convert_mv(uint16_t adc_raw)
{
	return ((uint32_t)adc_raw * 3300U) / 4095U;
}

void turnoff_leds(void)
{
	HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
}

void update_situation_led(AdcSituation_t status)
{
	turnoff_leds();

	switch(status)
	{
	case ADC_LOW: HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET); break;

	case ADC_OK: HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET); break;

	case ADC_HIGH: HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET); break;

	case ADC_ERROR: HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET); break;

	default: HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET); break;
	}
}

void scan_i2c_devices(void)
{
	char buffer[80];

	char start_msg[] = "I2C scan started\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t* )start_msg, strlen(start_msg), HAL_MAX_DELAY);

	for (uint8_t address = 1; address < 128; address++)
	{
		if (HAL_I2C_IsDeviceReady(&hi2c2, (address << 1), 1, 10) == HAL_OK)
		{
			int len = snprintf(
					buffer,
					sizeof(buffer),
					"Device found at 0x%02X\r\n",
					address
					);

			HAL_UART_Transmit(&huart2, (uint8_t* )buffer, len, HAL_MAX_DELAY);
		}
	}

	char end_msg[] = "I2C scan finished\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t* )end_msg, strlen(end_msg), HAL_MAX_DELAY);
}

HAL_StatusTypeDef bme280_read_chip_id(uint8_t *chip_id)
{
	return HAL_I2C_Mem_Read(
			&hi2c2,
			BME280_I2C_ADDR,
			BME280_REG_CHIP_ID,
			I2C_MEMADD_SIZE_8BIT,
			chip_id,
			1,
			HAL_MAX_DELAY);
}

void print_bme280_chip_id(void)
{
	uint8_t chip_id = 0;

	char buffer[100];

	HAL_StatusTypeDef result = bme280_read_chip_id(&chip_id);

	if (result == HAL_OK)
	{
		int len = snprintf(
				buffer,
				sizeof(buffer),
				"Chip ID: 0x%02X\r\n",
				chip_id);

		HAL_UART_Transmit(&huart2, (uint8_t* )buffer, len, HAL_MAX_DELAY);

		if (chip_id == BME280_CHIP_ID)
		{
			char ok_msg[] = "Sensor detected: BME280\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t* )ok_msg, strlen(ok_msg), HAL_MAX_DELAY);
		}

		else if (chip_id == BMP280_CHIP_ID)
		{
			char ok_msg[] = "Sensor detected: BMP280\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t* )ok_msg, strlen(ok_msg), HAL_MAX_DELAY);
		}

		else
		{
			char error_msg[] = "Sensor detected: UNKNOWN\r\n";
			HAL_UART_Transmit(&huart2, (uint8_t* )error_msg, strlen(error_msg), HAL_MAX_DELAY);
		}
	}

	else
	{
		char failed_msg[] = "BME280 chip ID read failed\r\n";
		HAL_UART_Transmit(&huart2, (uint8_t* )failed_msg, strlen(failed_msg), HAL_MAX_DELAY);
	}
}

void send_uart_head(void)
{
	char header[] = "timestamp_ms,adc_raw,adc_mv,button,status\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t*)header, sizeof(header) - 1, 100);
}

void send_sensor_log(uint32_t timestamp_t, uint16_t adc_raw, uint32_t adc_mv, uint8_t button, AdcSituation_t situation)
{
	char buffer[100];

	const char *situation_char = get_situation_char(situation);

	int len = snprintf(
			buffer,
			sizeof(buffer),
			"%lu,%u,%lu,%u,%s\r\n",
			(unsigned long)timestamp_t,
			adc_raw,
			(unsigned long)adc_mv,
			button,
			situation_char
			);

	HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, HAL_MAX_DELAY);
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
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  uint32_t last_log_time = 0;

  char boot_msg[] = "\r\n--- BOOT ---\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)boot_msg, strlen(boot_msg), HAL_MAX_DELAY);

  scan_i2c_devices();

  print_bme280_chip_id();

  if (bmp280_soft_reset() == HAL_OK)
  {
      char msg[] = "BMP280 soft reset: OK\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      HAL_Delay(100);
  }
  else
  {
      char msg[] = "BMP280 soft reset: ERROR\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
  }

  if (bmp280_read_calibration(&bmp280_calib) == HAL_OK)
  {
      print_bmp280_calibration(&bmp280_calib);
  }
  else
  {
      char msg[] = "BMP280 calibration read failed\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
  }

  if (bmp280_configure() == HAL_OK)
  {
      char msg[] = "BMP280 configure: OK\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
  }
  else
  {
      char msg[] = "BMP280 configure: ERROR\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
  }

  send_uart_head();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  uint32_t now = HAL_GetTick();

	  if ((now - last_log_time) >= UART_LOG_INTERVAL_MS)
	  {
	      uint8_t button = read_user_button();
		  uint16_t adc_raw = read_adc_average();
		  uint32_t adc_mv = convert_mv(adc_raw);
		  AdcSituation_t current_situation = adc_situation(adc_mv);

		  update_situation_led(current_situation);

		  send_sensor_log(now, adc_raw, adc_mv, button, current_situation);

		  BMP280_RawData_t bmp280_raw;

		  if (bmp280_trigger_forced_measurement() == HAL_OK)
		  {
		      print_bmp280_debug_registers();

		      HAL_Delay(50);

		      print_bmp280_debug_registers();

		      if (bmp280_read_raw_data(&bmp280_raw) == HAL_OK)
		      {
		          print_bmp280_raw_data(&bmp280_raw);
		      }
		      else
		      {
		          char msg[] = "BMP280 raw read failed\r\n";
		          HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		      }
		  }
		  else
		  {
		      char msg[] = "BMP280 forced measurement failed\r\n";
		      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		  }

		  last_log_time = now;
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
