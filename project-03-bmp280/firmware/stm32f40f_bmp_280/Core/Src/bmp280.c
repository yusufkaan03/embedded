/*
 * bmp280.c
 *
 *  Created on: Aug 26, 2026
 *      Author: yusuf
 */

#include "bmp280.h"

static bool readRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var, uint8_t length)
{
	if(HAL_I2C_Mem_Read(BMP280->hi2c, BMP280->BMP280_Address, reg_id, I2C_MEMADD_SIZE_8BIT, var, length, 100U) != HAL_OK)
	{
		BMP280->Error = ERROR_READ_REGISTER;
		return false;
	}

	return true;
}

static bool writeRegister(BMP280_t *BMP280, uint8_t reg_id, uint8_t *var, uint8_t length)
{
	if(HAL_I2C_Mem_Write(BMP280->hi2c, BMP280->BMP280_Address, reg_id, I2C_MEMADD_SIZE_8BIT, var, length, 100U) != HAL_OK)
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
	int32_t var1, var2;
	var1 = ((((adc_T >> 3) - ((int32_t)BMP280->CalibData.dig_T1 << 1))) * ((int32_t)BMP280->CalibData.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1)) * ((adc_T>>4) - ((int32_t)BMP280->CalibData.dig_T1))) >> 12) *
	((int32_t)BMP280->CalibData.dig_T3)) >> 14;

	BMP280->Measure_Data.t_fine = var1 + var2;

	BMP280->Measure_Data.temperature_x100 = (BMP280->Measure_Data.t_fine * 5 + 128) >> 8;

	return BMP280->Measure_Data.temperature_x100;
}

static uint32_t BMP280_compensate_P(BMP280_t *BMP280, uint32_t adc_P)
{
	int32_t var1, var2;
	uint32_t p;

	var1 = (((int32_t)BMP280->Measure_Data.t_fine) >> 1) - (int32_t)64000;
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
	BMP280->Measure_Data.pressure_pa = (uint32_t)((int32_t)p + ((var1 + var2 + BMP280->CalibData.dig_P7) >> 4));

	return BMP280->Measure_Data.pressure_pa;
}

static bool BMP280_ParseRawData(const BMP280_DataRaw *data_raw, BMP280_UncompensatedData *uncompensated_data)
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

static bool BMP280_Workflow(BMP280_t *BMP280, uint8_t wait_time)
{
	uint8_t status_reg_value;
	uint8_t measuring;
	uint8_t im_update;

	HAL_Delay(5);

	uint32_t last_time = HAL_GetTick();

	do
	{
		if(readRegister(BMP280, BMP280_REG_STATUS, &status_reg_value, 1U) == true)
		{
			measuring = status_reg_value & 0x08U;
			im_update = status_reg_value & 0x01U;
		}
		else
		{
			measuring = 0xFFU;
			im_update = 0xFFU;
		}
	}while((measuring == 0x8U || im_update == 0x1U) && (HAL_GetTick() - last_time) < wait_time);

	if(measuring == 0U && im_update == 0U)
	{
		return true;
	}
	else if(measuring == 0xFFU || im_update == 0xFFU)
	{
		return false;
	}
	else
	{
		BMP280->Error = ERROR_TIMEOUT;
		return false;
	}
}

static BMP280_Measure_Status_t MeasuringHandler(BMP280_t *BMP280, BMP280_Measure_Status_t *status)
{
	uint8_t f4_reg_value;
	uint8_t f5_reg_value;

	f4_reg_value = ((uint8_t)(BMP280->Params.osrs_t) << 5U) | ((uint8_t)(BMP280->Params.osrs_p) << 2U)|
					((uint8_t)(BMP280->Params.mode));
	f5_reg_value = (uint8_t)(BMP280->Params.filter) << 2U;

	*status = CONFIGURE_MEASUREMENT;

	if(writeRegister(BMP280, BMP280_REG_CONFIG, &f5_reg_value, 1U) != true)
		{
		return *status;
		}
	else
	{
		if(writeRegister(BMP280, BMP280_REG_CTRL_MEAS, &f4_reg_value, 1U) != true)
		{
			return *status;
		}
		else
		{
			*status = WAIT_FINISH_MEASURING;
		}
	}

	if(*status == WAIT_FINISH_MEASURING)
	{
		uint8_t wait_time = ((BMP280->Params.osrs_p) * 8U) + ((BMP280->Params.osrs_t) * 8U);
		if(BMP280_Workflow(BMP280, wait_time) != true)
		{
			return 	*status;
		}
		else
		{
			*status = READ_RAW_DATA;
		}
	}

	return *status;
}

static BMP280_Measure_Status_t Measuring(BMP280_t *BMP280, int32_t *temp, uint32_t *press, BMP280_Measure_Status_t *status)
{

	if(MeasuringHandler(BMP280, status) != READ_RAW_DATA)
	{
		return *status;
	}

	BMP280_DataRaw data_raw;
	BMP280_UncompensatedData uncompensated_data;

	if(readRegister(BMP280, BMP280_REG_PRESS_MSB, data_raw.bytes, BMP280_RAW_DATA_LENGTH) != true)
	{
		return READ_RAW_DATA;
	}
	else
	{
		*status = PARSE_RAW_DATA;
	}

	if(*status == PARSE_RAW_DATA)
	{
		if(BMP280_ParseRawData(&data_raw, &uncompensated_data) != true)
		{
			return *status;
		}
		else
		{
			*status = COMPENSATE_T;
		}
	}
	else
	{
		return *status;
	}

	BMP280->Measure_Data.Uncompensated_Data.adc_pressure = uncompensated_data.adc_pressure;
	BMP280->Measure_Data.Uncompensated_Data.adc_temperature = uncompensated_data.adc_temperature;

	if(*status == COMPENSATE_T)
	{
		*temp = BMP280_compensate_T(BMP280, uncompensated_data.adc_temperature);

		if(BMP280->Params.osrs_p == BMP280_MEASURE_SKIPPED)
		{
			*status = MEASURE_OK;
		}
		else
		{
			*status = COMPENSATE_P;
		}
	}

	if(*status == COMPENSATE_P)
	{
		*press = BMP280_compensate_P(BMP280, uncompensated_data.adc_pressure);
		*status = MEASURE_OK;
	}

	return *status;
}

bool BMP280_Reset(BMP280_t *BMP280)
{
	uint8_t reset_value = BMP280_RESET_VALUE;
	if(writeRegister(BMP280, BMP280_REG_RESET, &reset_value, 1U) != true)
	{
		return false;
	}

	if(BMP280_Workflow(BMP280, BMP280_RESET_TIME) != true)
	{
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
	}
}


bool readChipId(BMP280_t *BMP280, uint8_t *chip_id)
{
	return readRegister(BMP280, BMP280_REG_CHIP_ID, chip_id, 1U);
}

BMP280_Init_Status_t BMP280_Init(BMP280_t *BMP280, I2C_HandleTypeDef *p_hi2c, uint8_t Address)
{
	if(BMP280 == NULL)
	{
		return ERROR_BMP280_NULL_POINTER_I;
	}
	if(p_hi2c == NULL)
	{
		return ERROR_I2C_NULL_POINTER;
	}
	BMP280_Init_Status_t initStatus = BMP280_START;

	BMP280->Status = BMP280_STATUS_INIT;
	BMP280->Error = ERROR_NO;
	BMP280->hi2c = p_hi2c;
	BMP280->BMP280_Address = (Address << 1U);

	HAL_StatusTypeDef i2c_ready_status;

	initStatus = BMP280_ADDRESS;

	i2c_ready_status =
	    HAL_I2C_IsDeviceReady(BMP280->hi2c,
	                          BMP280->BMP280_Address,
	                          3U,
	                          100U);

	if(i2c_ready_status == HAL_OK)
	{
		initStatus = BMP280_CHIPID;
	}

	else
	{
		BMP280->Error = ERROR_I2C_ADDRESS;
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

		initStatus = INIT_OK;
		BMP280->Status = BMP280_STATUS_WAIT;
	}

	return initStatus;
}


BMP280_Measure_Status_t fastPress(BMP280_t *BMP280, uint32_t *press)
{

	if(BMP280 == NULL)
	{
		return ERROR_BMP280_NULL_POINTER_M;
	}

	if(press == NULL)
	{
		return ERROR_VARIABLE_NULL_POINTER;
	}

	if(BMP280->Error != ERROR_NO)
	{
		return ERROR_M;
	}

	BMP280_Measure_Status_t measure_Status = START_MEASURE;

	int32_t temp = 0U;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MEASURE_X1;
	BMP280->Params.osrs_p = BMP280_MEASURE_X2;
	BMP280->Params.filter = BMP280_FILTER_X2;
	BMP280->Status = BMP280_STATUS_FAST_PRESS;
	if(Measuring(BMP280, &temp, press, &measure_Status) == MEASURE_OK)
	{
		BMP280->Status = BMP280_STATUS_WAIT;
	}

	return measure_Status;
}

BMP280_Measure_Status_t fastTemp(BMP280_t *BMP280, int32_t *temp)
{

	if(BMP280 == NULL)
	{
		return ERROR_BMP280_NULL_POINTER_M;
	}

	if(temp == NULL)
	{
		return ERROR_VARIABLE_NULL_POINTER;
	}

	if(BMP280->Error != ERROR_NO)
	{
		return ERROR_M;
	}

	BMP280_Measure_Status_t measure_Status = START_MEASURE;

	uint32_t press = 0U;
	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MEASURE_X2;
	BMP280->Params.osrs_p = BMP280_MEASURE_SKIPPED;
	BMP280->Params.filter = BMP280_FILTER_X2;
	BMP280->Status = BMP280_STATUS_FAST_TEMP;
	if(Measuring(BMP280, temp, &press, &measure_Status) == MEASURE_OK)
	{
		BMP280->Status = BMP280_STATUS_WAIT;
	}

	return measure_Status;
}

BMP280_Measure_Status_t fastMeasure(BMP280_t *BMP280, int32_t *temp, uint32_t *press)
{
	if(BMP280 == NULL)
	{
		return ERROR_BMP280_NULL_POINTER_M;
	}

	if(temp == NULL || press == NULL)
	{
		return ERROR_VARIABLE_NULL_POINTER;
	}
	if(BMP280->Error != ERROR_NO)
	{
		return ERROR_M;
	}

	BMP280_Measure_Status_t measure_Status = START_MEASURE;



	BMP280->Params.mode = BMP280_MODE_FORCE;
	BMP280->Params.osrs_t = BMP280_MEASURE_X2;
	BMP280->Params.osrs_p = BMP280_MEASURE_X2;
	BMP280->Params.filter = BMP280_FILTER_X2;
	BMP280->Status = BMP280_STATUS_FAST_MEASURE;
	if(Measuring(BMP280, temp, press, &measure_Status) == MEASURE_OK)
	{
		BMP280->Status = BMP280_STATUS_WAIT;
	}

	return measure_Status;
}
