/*
 * bmp280.c
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#include "bmp280.h"

static volatile int32_t t_fine;

/*static BMP280_Error_t error_status = ERROR_NO;

static bool readRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var, uint8_t length)
{
	if(HAL_I2C_Mem_Read(BMP280->hi2c, BMP280->BMP280_Adress, reg_id, I2C_MEMADD_SIZE_8BIT, var, length, 100) == HAL_OK)
	{
		return true;
	}
	else
	{
		error_status = ERROR_READ_REGISTER;
		return false;
	}
}

static bool writeRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var)
{
	if(HAL_I2C_Mem_Write(BMP280->hi2c, BMP280->BMP280_Adress, reg_id, I2C_MEMADD_SIZE_8BIT, var, 1U, 100) == HAL_OK)
	{
		return true;
	}
	else
	{
		error_status = ERROR_WRITE_REGISTER;
		return false;
	}
}

static int32_t BMP280_compensate_T(BMP280_t *BMP280, int32_t adc_T)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T >> 3) - ((int32_t)BMP280->CalibData.dig_T1 << 1))) * ((int32_t)BMP280->CalibData.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1)) * ((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1))) >> 12) *
	((int32_t)BMP280->CalibData.dig_T3)) >> 14;

	t_fine = var1 + var2;

	T = (t_fine * 5 + 128) >> 8;

	return T;
}

static uint32_t BMP280_compensate_P(BMP280_t *BMP280, uint32_t adc_P)
{
	int32_t var1, var2;
	uint32_t p;

	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11 ) * ((int32_t)BMP280->CalibData.dig_P6);
	var2 = var2 + ((var1 * ((int32_t)BMP280->CalibData.dig_P5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)BMP280->CalibData.dig_P4) << 16);
	var1 = (((BMP280->CalibData.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13 )) >> 3) +
			((((int32_t)BMP280->CalibData.dig_P2) * var1) >> 1)) >> 18;
	var1 = ((((32768 + var1)) * ((int32_t)BMP280->CalibData.dig_P1)) >> 15);

	if(var1 == 0U)
	{
		return 0U;
	}

	p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;

	if (p < 0x80000000)
	{
		p = (p << 1) / ((uint32_t)var1);
	}
	else
	{
		p = (p / (uint32_t)var1) * 2;
	}

	var1 = (((int32_t)BMP280->CalibData.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * ((int32_t)BMP280->CalibData.dig_P8)) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + BMP280->CalibData.dig_P7) >> 4));

	return p;
}

static bool forceMeasur_Handle(BMP280_t *BMP280, int32_t temp, uint32_t press)
{
	uint8_t f4_value = BMP280_REG_CTRL_MEAS;
	uint8_t f5_value = BMP280_REG_CONFIG;

	f4_value = ((uint8_t)(BMP280->Params.osrs_t) << 5U) | ((uint8_t)(BMP280->Params.osrs_p) << 2U) | ((uint8_t)(BMP280->Params.mode));
	f5_value = (uint8_t)(BMP280->Params.filter) << 2U;

	if(writeRegister(BMP280, BMP280_REG_CONFIG, &f5_value) == false)
	{
		return false;
	}

	HAL_Delay(5);

	if(writeRegister(BMP280, BMP280_REG_CTRL_MEAS, &f4_value))
	{
		HAL_Delay(5);

		uint8_t f3_read = 0x00U;
		uint8_t measuring = f3_read & 0x08U;

		uint32_t last_time = HAL_GetTick();

		do
		{
			readRegister(BMP280, BMP280_REG_STATUS, &f3_read, 1U);
			measuring = f3_read & 0x08U;
		}while((measuring != 0U) && (HAL_GetTick()-last_time) <= 20U);

		if(measuring != 0U)
		{
			error_status = ERROR_MEASURING_TIMEOUT;
			return false;
		}
	}

	if(error_status == ERROR_NO)
	{
		BMP280_DataRaw data_raw;
		BMP280_UncompensatedData uncompensated_data;
		BMP280_CalibrationRaw raw_calibration;

		if(readRegister(BMP280, BMP280_REG_PRESS_MSB, data_raw.bytes, BMP280_RAW_DATA_LENGTH))
		{
			if(BMP280_ParseRawData(&data_raw, &uncompensated_data) == BMP280_STATUS_OK)
			{

				if(readRegister(BMP280, BMP280_REG_PRESS_MSB, raw_calibration.bytes, BMP280_CALIBRATION_DATA_LENGTH))
				{
					if(BMP280_ParseCalibration(BMP280, &raw_calibration) == BMP280_STATUS_OK)
					{
						if(BMP280->Params.osrs_t != BMP280_MESURE_SKIPPED)
						{
							temp = BMP280_compensate_T(BMP280, uncompensated_data.adc_temperature);
						}
						if(BMP280->Params.osrs_p != BMP280_MESURE_SKIPPED)
						{
							press = BMP280_compensate_P(BMP280, uncompensated_data.adc_pressure);
						}

						return true;
					}
				}
			}
		}
	}

	return false;
}

bool forcedTemp(BMP280_t *BMP280, int32_t temp)
{
	uint32_t press = 0U;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MESURE_X2;
	BMP280->Params.osrs_p = BMP280_MESURE_SKIPPED;
	BMP280->Params.filter = BMP280_FILTER_X2;
	if(forceMeasur_Handle(BMP280, temp, press))
	{
		return true;
	}

	return false;
}

bool forcedPress(BMP280_t *BMP280, uint32_t press)
{
	int32_t temp = 0U;;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MESURE_SKIPPED;
	BMP280->Params.osrs_p = BMP280_MESURE_X2;
	BMP280->Params.filter = BMP280_FILTER_X2;
	if(forceMeasur_Handle(BMP280, temp, press))
	{
		return true;
	}

	return false;
}

bool forcedMeasure(BMP280_t *BMP280, int32_t temp, uint32_t press)
{
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MESURE_X2;
	BMP280->Params.osrs_p = BMP280_MESURE_X2;
	BMP280->Params.filter = BMP280_FILTER_X2;
	if(forceMeasur_Handle(BMP280, temp, press))
		{
			return true;
		}

		return false;
}

bool resetSensor(BMP280_t *BMP280)
{
	uint8_t reset_reg = BMP280_REG_RESET;
	uint8_t reset_value = BMP280_RESET_VALUE;

	uint8_t f3 = 0x00;
	uint8_t im_update = f3 & 0x01;

	if(writeRegister(BMP280, reset_reg, &reset_value))
	{
		HAL_Delay(5);

		uint32_t last_time = HAL_GetTick();

		do
		{
			readRegister(BMP280, BMP280_REG_STATUS, &f3, 1U);
			im_update = f3 & 0x01U;

		}while((im_update != 0x00U) && ((HAL_GetTick() - last_time) < 20U));

		if(im_update == 0x00U)
		{
			uint8_t press_msb;
			uint8_t temp_msb;

			readRegister(BMP280, BMP280_REG_PRESS_MSB, &press_msb, 1U);
			readRegister(BMP280, BMP280_REG_TEMP_MSB, &temp_msb, 1U);

			if((press_msb == 0x80) && (temp_msb == 0x80))
			{
				return true;
			}
		}
	}

	error_status = ERROR_INIT_RESET;
	return false;
}


BMP280_Init_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t adress)
{
	BMP280_Init_t status = BMP280_WAIT_INIT;
	error_status = ERROR_NO;

	BMP280_CalibrationRaw raw_calibration;

	BMP280->hi2c = p_hi2c;
	BMP280->BMP280_Adress = (adress << 1);

	if(HAL_I2C_IsDeviceReady(BMP280->hi2c, BMP280->BMP280_Adress, 3U, 100) == HAL_OK)
	{
		status = BMP280_ADRESS_OK;
	}
	else
	{
		error_status = ERROR_INIT_I2C_ADRESS;
	}

	if(status == BMP280_ADRESS_OK)
	{
		uint8_t chip_id_read;

		if(readRegister(BMP280, BMP280_REG_CHIP_ID, &chip_id_read, 1U))
		{
			if(chip_id_read == BMP280_CHIP_ID)
			{
				status = BMP280_CHIP_ID_OK;
			}
			else
			{
				error_status = ERROR_INIT_INVALID_CHIP_ID;
			}
		}
	}

	if(status == BMP280_CHIP_ID_OK)
	{

		if(resetSensor(BMP280))
		{
			status = BMP280_RESET_OK;
		}
		else
		{
			error_status = ERROR_INIT_RESET;
		}
	}

	if(status == BMP280_RESET_OK)
	{
		if(readRegister(BMP280, BMP280_REG_PRESS_MSB, raw_calibration.bytes, BMP280_CALIBRATION_DATA_LENGTH))
		{
			if(BMP280_ParseCalibration(BMP280, &raw_calibration) == BMP280_STATUS_OK)
			{
				status = BMP280_CALIB_DATA_OK;
			}
			else
			{
				error_status = ERROR_INIT_PARSE_CALIB;
			}
		}
	}



	if(status == BMP280_CALIB_DATA_OK)
	{
		return BMP280_READY;
	}

	else
	{
		return error_status;
	}

}

static uint16_t BMP280_BytesToUInt16(uint8_t lsb, uint8_t msb)
{
	return (uint16_t)(((uint16_t)msb << 8U) | (uint16_t)lsb);
}

static int16_t BMP280_BytesToInt16(uint8_t lsb, uint8_t msb)
{
	return (int16_t)BMP280_BytesToUInt16(lsb, msb);
}

BMP280_Status BMP280_ParseCalibration(BMP280_t *BMP280, const BMP280_CalibrationRaw *raw_calibration)
{
	if(raw_calibration == NULL || BMP280 == NULL)
	{
		return BMP280_STATUS_NULL_POINTER;
	}

	BMP280->CalibData.dig_T1 = BMP280_BytesToUInt16(raw_calibration->bytes[0], raw_calibration->bytes[1]);
	BMP280->CalibData.dig_T2 = BMP280_BytesToInt16(raw_calibration->bytes[2], raw_calibration->bytes[3]);
	BMP280->CalibData.dig_T3 = BMP280_BytesToInt16(raw_calibration->bytes[4], raw_calibration->bytes[5]);

	BMP280->CalibData.dig_P1 = BMP280_BytesToUInt16(raw_calibration->bytes[6], raw_calibration->bytes[7]);
	BMP280->CalibData.dig_P2 = BMP280_BytesToInt16(raw_calibration->bytes[8], raw_calibration->bytes[9]);
	BMP280->CalibData.dig_P3 = BMP280_BytesToInt16(raw_calibration->bytes[10], raw_calibration->bytes[11]);
	BMP280->CalibData.dig_P4 = BMP280_BytesToInt16(raw_calibration->bytes[12], raw_calibration->bytes[13]);
	BMP280->CalibData.dig_P5 = BMP280_BytesToInt16(raw_calibration->bytes[14], raw_calibration->bytes[15]);
	BMP280->CalibData.dig_P6 = BMP280_BytesToInt16(raw_calibration->bytes[16], raw_calibration->bytes[17]);
	BMP280->CalibData.dig_P7 = BMP280_BytesToInt16(raw_calibration->bytes[18], raw_calibration->bytes[19]);
	BMP280->CalibData.dig_P8 = BMP280_BytesToInt16(raw_calibration->bytes[20], raw_calibration->bytes[21]);
	BMP280->CalibData.dig_P9 = BMP280_BytesToInt16(raw_calibration->bytes[22], raw_calibration->bytes[23]);

	return BMP280_STATUS_OK;
}

static BMP280_Status BMP280_ParseCalibration1(const BMP280_CalibrationRaw *raw_calibration, BMP280_CalibrationData *calibration_data)
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

BMP280_Error_t BMP280_Error_Handler(void)
{
	return (uint8_t)error_status;

}*/

static bool readRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var, uint8_t length)
{
	if(HAL_I2C_Mem_Read(BMP280->hi2c, BMP280->BMP280_Adress, reg_id, I2C_MEMADD_SIZE_8BIT, var, length, 100U) != HAL_OK)
	{
		BMP280->Error = ERROR_READ_REGISTER;
		return false;
	}

	return true;
}

static bool writeRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var, uint8_t length)
{
	if(HAL_I2C_Mem_Write(BMP280->hi2c, BMP280->BMP280_Adress, reg_id, I2C_MEMADD_SIZE_8BIT, var, length, 100U) != HAL_OK)
	{
		BMP280->Error = ERROR_WRITE_REGISTER;
		return false;
	}

	return true;
}

static uint16_t BMP280_BytesToUInt16(uint8_t lsb, uint8_t msb)
{
	return (uint16_t)(((uint16_t)msb << 8U) | (uint16_t)lsb);
}

static int16_t BMP280_BytesToInt16(uint8_t lsb, uint8_t msb)
{
	return (int16_t)BMP280_BytesToUInt16(lsb, msb);
}

static bool BMP280_ParseCalibration(BMP280_t *BMP280, const BMP280_CalibrationRaw *raw_calibration)
{
	if(raw_calibration == NULL || BMP280 == NULL)
	{
		return false;
	}

	BMP280->CalibData.dig_T1 = BMP280_BytesToUInt16(raw_calibration->bytes[0], raw_calibration->bytes[1]);
	BMP280->CalibData.dig_T2 = BMP280_BytesToInt16(raw_calibration->bytes[2], raw_calibration->bytes[3]);
	BMP280->CalibData.dig_T3 = BMP280_BytesToInt16(raw_calibration->bytes[4], raw_calibration->bytes[5]);

	BMP280->CalibData.dig_P1 = BMP280_BytesToUInt16(raw_calibration->bytes[6], raw_calibration->bytes[7]);
	BMP280->CalibData.dig_P2 = BMP280_BytesToInt16(raw_calibration->bytes[8], raw_calibration->bytes[9]);
	BMP280->CalibData.dig_P3 = BMP280_BytesToInt16(raw_calibration->bytes[10], raw_calibration->bytes[11]);
	BMP280->CalibData.dig_P4 = BMP280_BytesToInt16(raw_calibration->bytes[12], raw_calibration->bytes[13]);
	BMP280->CalibData.dig_P5 = BMP280_BytesToInt16(raw_calibration->bytes[14], raw_calibration->bytes[15]);
	BMP280->CalibData.dig_P6 = BMP280_BytesToInt16(raw_calibration->bytes[16], raw_calibration->bytes[17]);
	BMP280->CalibData.dig_P7 = BMP280_BytesToInt16(raw_calibration->bytes[18], raw_calibration->bytes[19]);
	BMP280->CalibData.dig_P8 = BMP280_BytesToInt16(raw_calibration->bytes[20], raw_calibration->bytes[21]);
	BMP280->CalibData.dig_P9 = BMP280_BytesToInt16(raw_calibration->bytes[22], raw_calibration->bytes[23]);

	return true;
}

static int32_t BMP280_compensate_T(BMP280_t *BMP280, int32_t adc_T)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T >> 3) - ((int32_t)BMP280->CalibData.dig_T1 << 1))) * ((int32_t)BMP280->CalibData.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1)) * ((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1))) >> 12) *
	((int32_t)BMP280->CalibData.dig_T3)) >> 14;

	t_fine = var1 + var2;

	T = (t_fine * 5 + 128) >> 8;

	return T;
}

static int32_t bmp280_compensate_T_int32(BMP280_t *BMP280, int32_t adc_T)
{
	int32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((int32_t)BMP280->CalibData.dig_T1<<1))) * ((int32_t)BMP280->CalibData.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1)) * ((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1)))
			>> 12) *
			((int32_t)BMP280->CalibData.dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}


static uint32_t BMP280_compensate_P(BMP280_t *BMP280, uint32_t adc_P)
{
	int32_t var1, var2;
	uint32_t p;

	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11 ) * ((int32_t)BMP280->CalibData.dig_P6);
	var2 = var2 + ((var1 * ((int32_t)BMP280->CalibData.dig_P5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)BMP280->CalibData.dig_P4) << 16);
	var1 = (((BMP280->CalibData.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13 )) >> 3) +
			((((int32_t)BMP280->CalibData.dig_P2) * var1) >> 1)) >> 18;
	var1 = ((((32768 + var1)) * ((int32_t)BMP280->CalibData.dig_P1)) >> 15);

	if(var1 == 0U)
	{
		return 0U;
	}

	p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;

	if (p < 0x80000000)
	{
		p = (p << 1) / ((uint32_t)var1);
	}
	else
	{
		p = (p / (uint32_t)var1) * 2;
	}

	var1 = (((int32_t)BMP280->CalibData.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * ((int32_t)BMP280->CalibData.dig_P8)) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + BMP280->CalibData.dig_P7) >> 4));

	return p;
}

static bool BMP280_ParseRawData(BMP280_t *BMP280, const BMP280_DataRaw *data_raw, BMP280_UncompensatedData *uncompensated_data)
{
	if(data_raw == NULL || uncompensated_data == NULL)
	{
		return false;
	}

	uncompensated_data->adc_pressure = (uint32_t)((uint32_t)data_raw->bytes[0] << 12U |
			(uint32_t)data_raw->bytes[1] << 4U | (uint32_t)data_raw->bytes[2] >> 4U);

	uncompensated_data->adc_temperature = (uint32_t)((uint32_t)data_raw->bytes[3] << 12U |
				(uint32_t)data_raw->bytes[4] << 4U | (uint32_t)data_raw->bytes[5] >> 4U);

	return true;
}

static bool BMP280_Workflow(BMP280_t *BMP280, uint8_t reset)
{
	if(reset == BMP280_RESET_VALUE)
	{
		if(writeRegister(BMP280, BMP280_REG_RESET, &reset, 1U) != true)
		{
			return false;
		}
	}
	else
	{
		uint8_t f4_reg_value;
		uint8_t f5_reg_value;

		f4_reg_value = ((uint8_t)(BMP280->Params.osrs_t) << 5U) | ((uint8_t)(BMP280->Params.osrs_p) << 2U)|
				((uint8_t)(BMP280->Params.mode));
		f5_reg_value = (uint8_t)(BMP280->Params.filter) << 2U;


		if(writeRegister(BMP280, BMP280_REG_CTRL_MEAS, &f4_reg_value, 1U) != true)
		{
			return false;
		}
		else
		{
			if(writeRegister(BMP280, BMP280_REG_CONFIG, &f5_reg_value, 1U) != true)
			{
				return false;
			}
		}

		uint8_t status_reg_value;
		uint8_t measuring;
		uint8_t im_update;

		HAL_Delay(5);

		uint32_t last_time = HAL_GetTick();

		do
		{
			readRegister(BMP280, BMP280_REG_STATUS, &status_reg_value, 1U);
			measuring = status_reg_value & 0x08U;
			im_update = status_reg_value & 0x01U;
		}while((measuring == 1U || im_update == 1U) && (HAL_GetTick() - last_time) < 20U);

		if(measuring != 0U && im_update != 0U)
		{
			BMP280->Error = ERROR_TIMEOUT;
		}
			return false;
		}

		uint8_t press_msb;
		uint8_t temp_msb;

		readRegister(BMP280, BMP280_REG_PRESS_MSB, &press_msb, 1U);
		readRegister(BMP280, BMP280_REG_TEMP_MSB, &temp_msb, 1U);

		if(reset == BMP280_RESET_VALUE)
		{
			if(press_msb == 0x80 && temp_msb == 0x80)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		else
		{
			return true;
		}

}

static bool MeasuringHandler(BMP280_t *BMP280)
{
	if(BMP280_Workflow(BMP280, 0U) != true)
	{
		return false;
	}

	return true;
}

static bool Measuring(BMP280_t *BMP280, int32_t *temp, uint32_t *press)
{
	if(MeasuringHandler(BMP280) != true)
	{
		return false;
	}

	BMP280_DataRaw data_raw;
	BMP280_UncompensatedData uncompensated_data;

	if(readRegister(BMP280, BMP280_REG_PRESS_MSB, data_raw.bytes, BMP280_RAW_DATA_LENGTH) != true)
	{
		return false;
	}

	if(BMP280_ParseRawData(BMP280, &data_raw, &uncompensated_data) != true)
	{
		return false;
	}

	BMP280->uc.adc_pressure = uncompensated_data.adc_pressure;
	BMP280->uc.adc_temperature = uncompensated_data.adc_temperature;

	*temp = BMP280_compensate_T(BMP280, uncompensated_data.adc_temperature);

	if(BMP280->Params.osrs_p != BMP280_MESURE_SKIPPED)
	{
		*press = BMP280_compensate_P(BMP280, uncompensated_data.adc_pressure);
	}

	return true;

}



bool BMP280_Reset(BMP280_t *BMP280)
{
	/*uint8_t reset_value = BMP280_RESET_VALUE;
	if(writeRegister(BMP280, BMP280_REG_RESET, &reset_value, 1U) != true)
	{
		return false;
	}

	uint8_t status_reg_value;
	uint8_t measuring;
	uint8_t im_update;

	HAL_Delay(5);

	uint32_t last_time = HAL_GetTick();

	do
	{
		readRegister(BMP280, BMP280_REG_STATUS, &status_reg_value, 1U);
		measuring = status_reg_value & 0x08U;
		im_update = status_reg_value & 0x01U;
	}while((measuring == 1U || im_update == 1U) && (HAL_GetTick() - last_time) < 20U);

	if(measuring != 0U && im_update != 0U)
	{
		BMP280->Error = ERROR_TIMEOUT;
		return false;
	}

	uint8_t press_msb;
	uint8_t temp_msb;

	readRegister(BMP280, BMP280_REG_PRESS_MSB, &press_msb, 1U);
	readRegister(BMP280, BMP280_REG_TEMP_MSB, &temp_msb, 1U);

	if(press_msb == 0x80 && temp_msb == 0x80)
	{
		return true;
	}
	else
	{
		return false;
	}*/

	return BMP280_Workflow(BMP280, BMP280_RESET_VALUE);
}


bool readChipId(BMP280_t *BMP280, uint8_t *chip_id)
{
	return readRegister(BMP280, BMP280_REG_CHIP_ID, chip_id, 1U);
}

BMP280_Init_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t adress)
{
	BMP280_Init_t initStatus = BMP280_WAIT;

	if(BMP280 == NULL)
	{
		return ERROR_BMP280_NULL_POINTER;
	}

	BMP280->Status = BMP280_SATUS_INIT;
	BMP280->hi2c = p_hi2c;
	BMP280->BMP280_Adress = (adress << 1U);

	initStatus = BMP280_ADRESS;

	if(HAL_I2C_IsDeviceReady(BMP280->hi2c, BMP280->BMP280_Adress, 3U, 100) == HAL_OK)
	{
		initStatus = BMP280_CHIPID;
	}

	else
	{
		BMP280->Error = ERROR_I2C_ADRESS;
		return initStatus;
	}

	if(initStatus == BMP280_CHIPID)
	{
		uint8_t chip_id;
		if(readChipId(BMP280, &chip_id) == true)
		{
			if(chip_id == BMP280_CHIP_ID)
			{
				BMP280->BMP280_ChipId = chip_id;
				initStatus = BMP280_RESET;
			}
			else
			{
				BMP280->Error = ERROR_INVALID_CHIP_ID;
				return initStatus;
			}
		}
	}

	if(initStatus == BMP280_RESET)
	{
		if(BMP280_Reset(BMP280) == true)
		{
			initStatus = BMP280_CALIB_DATA;
		}
		else
		{
			return initStatus;
		}
	}

	if(initStatus == BMP280_CALIB_DATA)
	{
		BMP280_CalibrationRaw raw_calibration;

		if(readRegister(BMP280, BMP280_CALIB_DATA_00, raw_calibration.bytes, BMP280_CALIBRATION_DATA_LENGTH) != true)
		{
			return initStatus;
		}

		if(BMP280_ParseCalibration(BMP280, &raw_calibration) != true)
		{
			BMP280->Error = ERROR_NULL_POINTER;
			return initStatus;
		}

		initStatus = BMP280_READY;
		BMP280->Status = BMP280_STSTUS_OK;
	}

	return initStatus;
}


bool fastPress(BMP280_t *BMP280, uint32_t *press)
{
	int32_t temp = 0U;;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MESURE_X1;
	BMP280->Params.osrs_p = BMP280_MESURE_X2;
	BMP280->Params.filter = BMP280_FILTER_X2;
	BMP280->Status = BMP280_STATUS_FAST_PRESS;
	if(Measuring(BMP280, &temp, press))
	{
		BMP280->Status = BMP280_STSTUS_OK;
		return true;
	}

	return false;
}

bool fastTemp(BMP280_t *BMP280, int32_t *temp)
{
	uint32_t press = 0U;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MESURE_X16;
	BMP280->Params.osrs_p = BMP280_MESURE_SKIPPED;
	BMP280->Params.filter = BMP280_FILTER_X2;
	BMP280->Status = BMP280_STATUS_FAST_TEMP;
	if(Measuring(BMP280, temp, &press))
	{
		BMP280->Status = BMP280_STSTUS_OK;
		return true;
	}

	return false;
}


























