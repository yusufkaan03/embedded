/*
 * bmp280.c
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#include "bmp280.h"

static BMP280_Error_t error_status = ERROR_NO;


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

bool BMP280_Params(BMP280_t *BMP280, BMP280_Mode mode, BMP280_Filter filter,
		BMP280_Oversampling os_temp, BMP280_Oversampling os_press, BMP280_StandbTime time)
{
	uint8_t f3_reg;
	uint8_t f4_reg;

	bool f3_reg_ok = false;
	bool f4_reg_ok = false;

	BMP280->Params.mode = mode;
	BMP280->Params.filter = filter;
	BMP280->Params.osrs_t = os_temp;
	BMP280->Params.osrs_p = os_press;
	BMP280->Params.stby_t = time;


	f3_reg = (((uint8_t)BMP280->Params.stby_t) << 5U) | (((uint8_t)BMP280->Params.filter) << 2U);
	f4_reg = (((uint8_t)BMP280->Params.osrs_t) << 5U) | (((uint8_t)BMP280->Params.osrs_p) << 2U) | ((uint8_t)BMP280->Params.mode);

	if(writeRegister(BMP280, BMP280_REG_STATUS, &f3_reg))
	{
		uint8_t f3_reg_read;
		if(readRegister(BMP280, BMP280_REG_STATUS, &f3_reg_read, 1U))
		{
			f3_reg_read = f3_reg_read & 0xFCU;
			f3_reg = f3_reg & 0xFCU;
			if(f3_reg_read == f3_reg)
			{
				f3_reg_ok = true;
			}
		}
	}

	if(writeRegister(BMP280, BMP280_REG_STATUS, &f4_reg))
	{
		uint8_t f4_reg_read;
		if(readRegister(BMP280, BMP280_REG_STATUS, &f4_reg_read, 1U))
		{
			f4_reg_read = f4_reg_read & 0xFCU;
			f4_reg = f4_reg & 0xFCU;
			if(f4_reg_read == f4_reg)
			{
				f4_reg_ok = true;
			}
		}
	}

	if(f3_reg_ok && f4_reg_ok)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool BMP280_ForceMeasur(BMP280_t *BMP280, BMP280_Oversampling os_temp, BMP280_Oversampling os_press, BMP280_Filter filter)
{
	uint8_t f4_reg;
	uint8_t f5_reg;

	bool f4_reg_ok = false;
	bool f5_reg_ok = false;

	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.filter = filter;
	BMP280->Params.osrs_t = os_temp;
	BMP280->Params.osrs_p = os_press;

	f4_reg = ((uint8_t)BMP280->Params.filter) << 2U;
	f5_reg = (((uint8_t)BMP280->Params.osrs_t) << 5U) | (((uint8_t)BMP280->Params.osrs_p) << 2U) | ((uint8_t)BMP280->Params.mode);

	if(writeRegister(BMP280, BMP280_REG_CTRL_MEAS, &f4_reg))
	{
		uint8_t f4_reg_read;
		if(readRegister(BMP280, BMP280_REG_CTRL_MEAS, &f4_reg_read, 1U))
		{
			f4_reg_read &= 0x1CU;
			f4_reg &= 0x1CU;
			if(f4_reg_read == f4_reg)
			{
				f4_reg_ok = true;
			}
		}
	}

	if(writeRegister(BMP280, BMP280_REG_CONFIG, &f5_reg) && error_status == ERROR_NO)
	{
		uint8_t f5_reg_read;
		if(readRegister(BMP280, BMP280_REG_CONFIG, &f5_reg_read, 1U))
		{
			f5_reg_read &= 0xFCU;
			f5_reg &= 0xFCU;
			if(f5_reg_read == f5_reg)
			{
				f5_reg_ok = true;
			}
		}
	}

	if(f4_reg_ok && f5_reg_ok)
	{
		HAL_Delay(5);

		uint8_t f3_reg_read;

		uint8_t im_update = f3_reg_read & 0x01U;
		uint8_t mesuring = f3_reg_read & 0x08U;

		uint32_t last_time = HAL_GetTick();

		do
		{
			readRegister(BMP280, BMP280_REG_STATUS, &f3_reg_read, 1U);
			im_update = f3_reg_read & 0x01U;
			mesuring = f3_reg_read & 0x08U;

		}while((im_update != 0x00U) && (mesuring != 0x00U) && ((HAL_GetTick() - last_time) < 20U));
	}



}

BMP280_Init_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t adress)
{
	BMP280_Init_t status = BMP280_WAIT_INIT;
	error_status = ERROR_NO;

	BMP280_CalibrationRaw raw_calibration;
	BMP280_CalibrationData calibration_data;
	BMP280_Status parse_status;

	BMP280->hi2c = p_hi2c;
	BMP280->BMP280_Adress = (adress << 1);

	HAL_StatusTypeDef results = HAL_I2C_IsDeviceReady(BMP280->hi2c, BMP280->BMP280_Adress, 3U, 100);
	if(results == HAL_OK)
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
			parse_status = BMP280_ParseCalibration(&raw_calibration, &calibration_data);
			if(parse_status == BMP280_STATUS_OK)
			{
				BMP280->CalibData = calibration_data;
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

BMP280_Error_t BMP280_Error_Handler(void)
{
	return (uint8_t)error_status;

}

