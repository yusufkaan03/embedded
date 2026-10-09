/*
 * bmp280.h
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include "stm32f4xx_hal.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BMP280_CALIBRATION_DATA_LENGTH 	24U
#define BMP280_RAW_DATA_LENGTH			6U

#define BMP280_CHIP_ID 0x58U

#define BMP280_REG_CHIP_ID		0xD0
#define BMP280_REG_RESET		0xE0
#define BMP280_REG_STATUS		0xF3
#define BMP280_REG_CTRL_MEAS	0xF4
#define BMP280_REG_CONFIG		0xF5
#define BMP280_REG_PRESS_MSB	0xF7
#define BMP280_REG_PRESS_LSB	0xF8
#define BMP280_REG_PRESS_XLSB	0xF9
#define BMP280_REG_TEMP_MSB		0xFA
#define BMP280_REG_TEMP_LSB		0xFB
#define BMP280_REG_TEMP_XLSB	0xFC

#define BMP280_CALIB_DATA_00	0X88

#define BMP280_RESET_VALUE		0xB6
#define BMP280_RESET_TIME		0x1E

typedef enum
{
	BMP280_START				= 0xA1U,
	BMP280_ADDRESS				= 0xA2U,
	BMP280_CHIPID				= 0xB1U,
	BMP280_RESET				= 0xC1U,
	BMP280_CALIB_DATA			= 0xD1U,
	INIT_OK						= 0x00U,
	ERROR_BMP280_NULL_POINTER_I	= 0xFAU,
	ERROR_I2C_NULL_POINTER		= 0xFBU,
}BMP280_Init_Status_t;

typedef enum
{
	START_MEASURE				= 0xA1U,
	CONFIGURE_MEASUREMENT		= 0xB1U,
	WAIT_FINISH_MEASURING		= 0xC1U,
	READ_RAW_DATA				= 0xA2U,
	PARSE_RAW_DATA				= 0xD1U,
	COMPENSATE_T				= 0xE1U,
	COMPENSATE_P				= 0xE2U,
	MEASURE_OK					= 0X00U,
	ERROR_M						= 0xF1U,
	ERROR_BMP280_NULL_POINTER_M	= 0xF2U,
	ERROR_VARABLE_NULL_POINTER	= 0xF3U
}BMP280_Measure_Status_t;

typedef enum
{
	ERROR_NO				= 0x00U,
	ERROR_I2C_ADDRESS		= 0xB1U,
	ERROR_INVALID_CHIP_ID	= 0xB2U,
	ERROR_PARSE_CALIB		= 0xB3U,
	ERROR_NULL_POINTER		= 0xB4U,
	ERROR_READ_REGISTER		= 0xC1U,
	ERROR_WRITE_REGISTER	= 0xC2U,
	ERROR_TIMEOUT			= 0xC3U,
}BMP280_Error_t;

typedef struct
{
	uint16_t	dig_T1;
	int16_t 	dig_T2;
	int16_t 	dig_T3;

	uint16_t 	dig_P1;
	int16_t 	dig_P2;
	int16_t 	dig_P3;
	int16_t 	dig_P4;
	int16_t	 	dig_P5;
	int16_t 	dig_P6;
	int16_t		dig_P7;
	int16_t 	dig_P8;
	int16_t 	dig_P9;
}BMP280_CalibrationData;

typedef enum
{
	BMP280_STANDBY_0_5	= 0b000U,
	BMP280_STANDBY_62_5	= 0b001U,
	BMP280_STANDBY_125	= 0b010U,
	BMP280_STANDBY_250	= 0b011U,
	BMP280_STANDBY_500 	= 0b100U,
	BMP280_STANDBY_1000	= 0b101U,
	BMP280_STANDBY_2000	= 0b110U,
	BMP280_STANDBY_4000	= 0b111U,
}BMP280_StandbTime;

typedef enum
{
	BMP280_FILTER_OFF	= 0b000U,
	BMP280_FILTER_X2	= 0b001U,
	BMP280_FILTER_X4	= 0b010U,
	BMP280_FILTER_X8	= 0b011U,
	BMP280_FILTER_X16	= 0b100U,
}BMP280_Filter;

typedef enum
{
	BMP280_MEASURE_SKIPPED	= 0b000U,
	BMP280_MEASURE_X1		= 0b001U,
	BMP280_MEASURE_X2		= 0b010U,
	BMP280_MEASURE_X4		= 0b011U,
	BMP280_MEASURE_X8		= 0b100U,
	BMP280_MEASURE_X16		= 0b101U,
}BMP280_Oversampling;

typedef enum
{
	BMP280_MODE_SLEEP	= 0b00U,
	BMP280_MODE_FORCE	= 0b01U,
	BMP280_MODE_NORMAL	= 0b11U,
}BMP280_Mode;

typedef struct
{
	BMP280_Mode 		mode;
	BMP280_Filter		filter;
	BMP280_Oversampling	osrs_t;
	BMP280_Oversampling osrs_p;
	BMP280_StandbTime	stby_t;
}BMP280_Parameters_t;

typedef struct
{
	uint8_t bytes[BMP280_CALIBRATION_DATA_LENGTH];
}BMP280_CalibrationRaw;

typedef struct
{
	uint8_t bytes[BMP280_RAW_DATA_LENGTH];
}BMP280_DataRaw;

typedef struct
{
	uint32_t adc_temperature;
	uint32_t adc_pressure_pa;
}BMP280_UncompensatedData;

typedef enum
{
	BMP280_STATUS_WAIT			= 0x00U,
	BMP280_STATUS_INIT			= 0x01U,
	BMP280_STATUS_FAST_TEMP		= 0xB0U,
	BMP280_STATUS_FAST_PRESS	= 0xB1U,
	BMP280_STATUS_FAST_MEASURE	= 0xB2U,
}BMP280_Status_t;

typedef struct
{
	int32_t 					temperature_x100;
	uint32_t 					pressure_pa;
	BMP280_UncompensatedData	Uncompensated_Data;
	int32_t 					t_fine;

}BMP280_Measure_t;

typedef struct
{
	I2C_HandleTypeDef 		*hi2c;
	uint8_t 				BMP280_Address;
	uint8_t					BMP280_ChipId;
	BMP280_CalibrationData 	CalibData;
	BMP280_Parameters_t		Params;
	BMP280_Status_t			Status;
	BMP280_Error_t			Error;
	BMP280_Measure_t		Measure_Data;
}BMP280_t;


bool readChipId(BMP280_t *BMP280, uint8_t *chip_id);

BMP280_Init_Status_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t address);

BMP280_Measure_Status_t fastPress(BMP280_t *BMP280, uint32_t *press);

BMP280_Measure_Status_t fastTemp(BMP280_t *BMP280, int32_t *temp);

BMP280_Measure_Status_t fastMeasure(BMP280_t *BMP280, int32_t *temp, uint32_t *press);

uint16_t error_Handler(BMP280_t *BMP280, uint8_t status);

#endif /* INC_BMP280_H_ */











