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

typedef enum
{
	BMP280_WAIT,
	BMP280_ADRESS,
	BMP280_CHIPID,
	BMP280_RESET,
	BMP280_CALIB_DATA,
	BMP280_READY,
	ERROR_BMP280_NULL_POINTER,
}BMP280_Init_t;

typedef enum
{
	ERROR_NO				= 0x00U,
	ERROR_I2C_ADRESS		= 0xA0U,
	ERROR_INVALID_CHIP_ID	= 0xA1U,
	ERROR_PARSE_CALIB		= 0xA3U,
	ERROR_NULL_POINTER		= 0xA4U,
	ERROR_READ_REGISTER		= 0xB0U,
	ERROR_WRITE_REGISTER	= 0xB1U,
	ERROR_TIMEOUT			= 0xB2U,
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
	BMP280_MESURE_SKIPPED	= 0b000U,
	BMP280_MESURE_X1		= 0b001U,
	BMP280_MESURE_X2		= 0b010U,
	BMP280_MESURE_X4		= 0b011U,
	BMP280_MESURE_X8		= 0b100U,
	BMP280_MESURE_X16		= 0b101U,
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
	uint32_t adc_pressure;
}BMP280_UncompensatedData;

typedef enum
{
	BMP280_STATUS_WAIT			= 0x00U,
	BMP280_SATUS_INIT			= 0x01U,
	BMP280_STSTUS_OK			= 0xAAU,
	BMP280_STATUS_FAST_TEMP		= 0xB0U,
	BMP280_STATUS_FAST_PRESS	= 0xB1U,
	BMP280_STATUS_ERROR			= 0xFFU,
}BMP280_Status_t;

typedef struct
{
	I2C_HandleTypeDef 		*hi2c;
	uint8_t 				BMP280_Adress;
	uint8_t					BMP280_ChipId;
	BMP280_CalibrationData 	CalibData;
	BMP280_Parameters_t		Params;
	BMP280_Status_t			Status;
	BMP280_Error_t			Error;
	BMP280_UncompensatedData uc;
}BMP280_t;


bool readChipId(BMP280_t *BMP280, uint8_t *chip_id);

BMP280_Init_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t adress);

bool fastPress(BMP280_t *BMP280, uint32_t *press);

bool fastTemp(BMP280_t *BMP280, int32_t *temp);

#endif /* INC_BMP280_H_ */











