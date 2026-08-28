/*
 * bmp280.c
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#include "bmp280.h"

#include <stddef.h>

static uint16_t BMP280_BytesToUInt16(uint8_t lsb, uint8_t msb)
{
	return (uint16_t)(((uint16_t)msb << 8U) | (uint16_t)lsb);
}

static int16_t BMP280_BytesToInt16(uint8_t lsb, uint8_t msb)
{
	return (int16_t)BMP280_BytesToUInt16(lsb, msb);
}

BMP280_Status BMP280_ParseCalibration(const BMP280_CalibrationRaw *raw_calibration, BMP280_CalibrationData *calibration_data)
{
	if(raw_calibration == NULL || calibration_data == NULL)
	{
		return BMP280_STATUS_NULL_POINTER;
	}

	calibration_data->dig_T1 = BMP280_BytesToUInt16(raw_calibration->bytes[0], raw_calibration->bytes[1]);
	calibration_data->dig_T2 = BMP280_BytesToInt16(raw_calibration->bytes[2], raw_calibration->bytes[3]);
	calibration_data->dig_T3 = BMP280_BytesToInt16(raw_calibration->bytes[4], raw_calibration->bytes[5]);

	calibration_data->dig_P1 = BMP280_BytesToUInt16(raw_calibration->bytes[6], raw_calibration->bytes[7]);
	calibration_data->dig_P2 = BMP280_BytesToInt16(raw_calibration->bytes[8], raw_calibration->bytes[9]);
	calibration_data->dig_P3 = BMP280_BytesToInt16(raw_calibration->bytes[10], raw_calibration->bytes[11]);
	calibration_data->dig_P4 = BMP280_BytesToInt16(raw_calibration->bytes[12], raw_calibration->bytes[13]);
	calibration_data->dig_P5 = BMP280_BytesToInt16(raw_calibration->bytes[14], raw_calibration->bytes[15]);
	calibration_data->dig_P6 = BMP280_BytesToInt16(raw_calibration->bytes[16], raw_calibration->bytes[17]);
	calibration_data->dig_P7 = BMP280_BytesToInt16(raw_calibration->bytes[18], raw_calibration->bytes[19]);
	calibration_data->dig_P8 = BMP280_BytesToInt16(raw_calibration->bytes[20], raw_calibration->bytes[21]);
	calibration_data->dig_P9 = BMP280_BytesToInt16(raw_calibration->bytes[22], raw_calibration->bytes[23]);

	return BMP280_STATUS_OK;
}

BMP280_Status BMP280_ParseRawData(const BMP280_DataRaw *data_raw, BMP280_UncompensatedData *uncompensated_data)
{
	if(data_raw == NULL || uncompensated_data == NULL)
	{
		return BMP280_STATUS_NULL_POINTER;
	}

	uncompensated_data->adc_pressure = (uint32_t)((uint32_t)data_raw->bytes[0] << 12U |
			(uint32_t)data_raw->bytes[1] << 4U | (uint32_t)data_raw->bytes[2] >> 4U);

	uncompensated_data->adc_temperature = (uint32_t)((uint32_t)data_raw->bytes[3] << 12U |
				(uint32_t)data_raw->bytes[4] << 4U | (uint32_t)data_raw->bytes[5] >> 4U);

	return BMP280_STATUS_OK;
}


