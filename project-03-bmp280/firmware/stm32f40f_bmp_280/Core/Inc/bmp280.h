/*
 * bmp280.h
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include <stdint.h>

#define BMP280_CALIBRATION_DATA_LENGTH 	24U
#define BMP280_RAW_DATA_LENGTH			6U

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

typedef struct
{
	uint8_t bytes[BMP280_CALIBRATION_DATA_LENGTH];
}BMP280_CalibrationRaw;

typedef enum
{
	BMP280_STATUS_OK 			= 0,
	BMP280_STATUS_NULL_POINTER
}BMP280_Status;

typedef struct
{
	uint8_t bytes[BMP280_RAW_DATA_LENGTH];
}BMP280_DataRaw;

typedef struct
{
	uint32_t adc_temperature;
	uint32_t adc_pressure;
}BMP280_UncompensatedData;


BMP280_Status BMP280_ParseCalibration(const BMP280_CalibrationRaw *raw_calibration, BMP280_CalibrationData *calibration_data);

BMP280_Status BMP280_ParseRawData(const BMP280_DataRaw *data_raw, BMP280_UncompensatedData *uncompensated_data);


#endif /* INC_BMP280_H_ */











