/*
 * lis3dsh.c
 *
 *  Created on: Oct 19, 2026
 *      Author: yusuf
 */

#include <lis3dsh.h>

/* Function prototypes ---------------------------*/

static Lis3dsh_StatusTypeDef spiWriteRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, const uint8_t *data, const uint8_t size);
static Lis3dsh_StatusTypeDef spiReadRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, uint8_t *response, const uint8_t size);
static Lis3dsh_StatusTypeDef writeRegisterAndVerify(Lis3dsh_t *sensor, const uint8_t reg_adress,
		const uint8_t *data, uint8_t *response, const uint8_t size);

/*------------------------------------------------*/


static Lis3dsh_StatusTypeDef spiTransmit(Lis3dsh_t *sensor, const uint8_t *command_data, uint8_t size)
{
	if((sensor == NULL) || (command_data == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	HAL_StatusTypeDef status;
	HAL_GPIO_WritePin(sensor->gpıo.gpio_port, sensor->gpıo.gpio_pin, RESET);
	status = HAL_SPI_Transmit(sensor->hspi, command_data, size+1, 100);
	HAL_GPIO_WritePin(sensor->gpıo.gpio_port, sensor->gpıo.gpio_pin, SET);

	if (status == HAL_OK)
	{
		return LIS3DSH_OK;
	}
	else if (status == HAL_BUSY)
	{
		return LIS3DSH_ERROR_SPI_BUSY;
	}
	else if(status == HAL_TIMEOUT)
	{
		return LIS3DSH_ERROR_SPI_TIMEOUT;
	}
	else
	{
		return LIS3DSH_ERROR_SPI_WRITE;
	}
}

static Lis3dsh_StatusTypeDef spiTransmitReceive(Lis3dsh_t *sensor, const uint8_t *command_data, uint8_t *rx_msg, uint8_t size)
{
	if((sensor == NULL) || (command_data == NULL) || (rx_msg == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	HAL_StatusTypeDef status;
	HAL_GPIO_WritePin(sensor->gpıo.gpio_port, sensor->gpıo.gpio_pin, RESET);
	status = HAL_SPI_TransmitReceive(sensor->hspi, command_data, rx_msg, size+1, 100);
	HAL_GPIO_WritePin(sensor->gpıo.gpio_port, sensor->gpıo.gpio_pin, SET);


	if (status == HAL_OK)
	{
		return LIS3DSH_OK;
	}
	else if (status == HAL_BUSY)
	{
		return LIS3DSH_ERROR_SPI_BUSY;
	}
	else if(status == HAL_TIMEOUT)
	{
		return LIS3DSH_ERROR_SPI_TIMEOUT;
	}
	else
	{
		return LIS3DSH_ERROR_SPI_READ;
	}
}

static Lis3dsh_StatusTypeDef buildSpiPacket(uint8_t *tx_data, const uint8_t mode, const uint8_t adress, const uint8_t *data, const uint8_t size)
{
	if((tx_data == NULL) || (data == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	tx_data[0] = (mode << 7) | (adress & 0x7FU);

	for(uint8_t i = 0; i<size; i++)
	{
		tx_data[i+1] = data[i];
	}

	return LIS3DSH_OK;
}

static Lis3dsh_StatusTypeDef blockDataRegAddressUpdate(Lis3dsh_t *sensor)
{
	uint8_t ctrl_reg6 = 0xFF;
	uint8_t read_data[2] = {0};
	uint8_t write_data[2] = {0};

	Lis3dsh_StatusTypeDef status;

	status = buildSpiPacket(write_data, SPI_READ_DATA, CTRL_REG6_REG, read_data, 1U);
	if(status != LIS3DSH_OK)
	{
		return status;
	}

	status = spiTransmitReceive(sensor, write_data, read_data, 1U);
	if(status != LIS3DSH_OK)
	{
		return status;
	}

	ctrl_reg6 = read_data[1] | (1 << 4);

	status = writeRegisterAndVerify(sensor, CTRL_REG6_REG, &ctrl_reg6, read_data, 1U);
	if(status != LIS3DSH_OK)
	{
		return status;
	}

	return LIS3DSH_OK;
}

static Lis3dsh_StatusTypeDef rawToMg(const int16_t *values, int16_t *value_x, int16_t *value_y, int16_t *value_z)
{
	if((values == NULL) || (value_x == NULL) || (value_y == NULL) || (value_z == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	*value_x = (int16_t)((((int32_t)values[0]) * 6) / 100);
	*value_y = (int16_t)((((int32_t)values[1]) * 6) / 100);
	*value_z = (int16_t)((((int32_t)values[2]) * 6) / 100);

	return LIS3DSH_OK;
}

static Lis3dsh_StatusTypeDef readRawValues(Lis3dsh_t *sensor, const uint8_t axis, const uint8_t size, int16_t *values)
{
	if((sensor == NULL) || (values == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	Lis3dsh_StatusTypeDef status;

	uint8_t raw_values[6] = {0};
	uint8_t buffer_size;
	uint8_t first_axis;

	bool size_ok = false;

	if(axis != X_AXIS && axis != Y_AXIS && axis != Z_AXIS)
	{
		return LIS3DSH_ERROR_AXIS;
	}

	if(size == 1U)
	{
		size_ok = true;
	}
	else if((size + axis) == 3U)
	{
		size_ok = true;
	}
	else
	{
		size_ok = false;
	}

	if(size_ok == true)
	{
		first_axis = OUT_AXIS_REG + axis;
		buffer_size = (size*2U);
	}
	else
	{
		return LIS3DSH_ERROR_OVERFLOW;
	}

	status = spiReadRegister(sensor, first_axis, raw_values, buffer_size);
	if(status != LIS3DSH_OK)
	{
		return status;
	}

	for(uint8_t i = 0; i < size; i++)
	{
		values[i] = (int16_t)((uint16_t)(raw_values[(i*2)+1] << 8) | (uint16_t)(raw_values[i*2]));
	}

	return status;
}

static Lis3dsh_StatusTypeDef spiWriteRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, const uint8_t *data, const uint8_t size)
{
	if((sensor == NULL) || (data == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	if(size>MAX_SPI_SIZE)
	{
		return LIS3DSH_ERROR_OVERFLOW;
	}
	else if(size == 0)
	{
		return LIS3DSH_ERROR_SPI_ZERO_LENGTH;
	}

	Lis3dsh_StatusTypeDef status;
	uint8_t command_data[MAX_SPI_SIZE + 1] = {0};

	status = buildSpiPacket(command_data, SPI_WRITE_DATA, reg_adress, data, size);
	if(status != LIS3DSH_OK)
	{
		return status;
	}

	status = spiTransmit(sensor, command_data, size);
	if(status != LIS3DSH_OK)
	{
		return LIS3DSH_ERROR_SPI_WRITE;
	}

	return LIS3DSH_OK;
}

static Lis3dsh_StatusTypeDef spiReadRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, uint8_t *response, const uint8_t size)
{
	if((sensor == NULL) || (response == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	uint8_t command_data[MAX_SPI_SIZE + 1] = {0};
	uint8_t data[MAX_SPI_SIZE] = {0};

	Lis3dsh_StatusTypeDef status;

	if (size > MAX_SPI_SIZE)
	{
		return LIS3DSH_ERROR_OVERFLOW;
	}
	else if(size == 0)
	{
		return LIS3DSH_ERROR_SPI_ZERO_LENGTH;
	}

	uint8_t rx_msg[size+1];

	for(uint8_t i = 0; i<size+1; i++)
	{
		rx_msg[i] = 0xFFU;
	}

	buildSpiPacket(command_data, SPI_READ_DATA, reg_adress, data, size);

	status = spiTransmitReceive(sensor, command_data, rx_msg, size);

	if(status == LIS3DSH_OK)
	{
		for(uint8_t i = 0; i<size; i++)
		{
			response[i] = rx_msg[i+1];
		}
	}
	else
	{
		return status;
	}

	return LIS3DSH_OK;
}

static Lis3dsh_StatusTypeDef writeRegisterAndVerify(Lis3dsh_t *sensor, const uint8_t reg_adress, const uint8_t *data, uint8_t *response, const uint8_t size)
{
	if((sensor == NULL) || (data == NULL) || (response == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	Lis3dsh_StatusTypeDef status;

	status = spiWriteRegister(sensor, reg_adress, data, size);

	if(status != LIS3DSH_OK)
	{
		return status;
	}

	status = spiReadRegister(sensor, reg_adress, response, size);


	if(status != LIS3DSH_OK)
	{
		return status;
	}

	for(uint8_t i = 0; i<size; i++)
	{
		if(response[i] != data[i])
		{
			return LIS3DSH_ERROR_DATA_MISMATCH;
		}
	}

		return LIS3DSH_OK;
}

Lis3dsh_StatusTypeDef sensorInit(Lis3dsh_t *sensor, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin)
{
	if((sensor == NULL) || (hspi == NULL) || (port == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	memset(sensor, 0, sizeof(Lis3dsh_t));

	Lis3dsh_StatusTypeDef status;

	Init_state init_state = INIT_SETUP;

	do
	{

	if (init_state == INIT_SETUP)
	{
		sensor->hspi = hspi;
		sensor->gpıo.gpio_port = port;
		sensor->gpıo.gpio_pin = pin;

		init_state = INIT_WHO_AM_I;
	}

	if(init_state == INIT_WHO_AM_I)
	{
		__HAL_SPI_ENABLE(sensor->hspi);

		uint8_t WHO_AM_I = 0xFF;

		status = spiReadRegister(sensor, WHO_AM_I_REG, &WHO_AM_I, 1U);

		if(status != LIS3DSH_OK)
		{
			break;
		}

		if(WHO_AM_I != WHO_AM_I_VALUE)
		{
			status = LIS3DSH_ERROR_WHO_AM_I;
			break;
		}

	sensor->WHO_AM_I = WHO_AM_I;

	init_state = INIT_MEASUR_SETUP;

	}

	if(init_state == INIT_MEASUR_SETUP)
	{
		uint8_t data = 0x6F;
		uint8_t response = 0xFF;

		status = writeRegisterAndVerify(sensor, CTRL_REG4_REG, &data, &response, 1U);

		if(status != LIS3DSH_OK)
		{
			break;
		}

		init_state = INIT_OTO_REG_UPDATE;

	}

	if(init_state == INIT_OTO_REG_UPDATE)
	{
		status = blockDataRegAddressUpdate(sensor);
		if(status != LIS3DSH_OK)
		{
			break;
		}

		init_state = INIT_CHECK_UPDATE;
	}

	if(init_state == INIT_CHECK_UPDATE)
	{
		uint8_t zyxda = 0x00U;
		uint32_t start_time = HAL_GetTick();

		do
		{
			status = spiReadRegister(sensor, STATUS_REG, &zyxda, 1U);
			zyxda = zyxda & (1 << 3);

		}while(((HAL_GetTick() - start_time) < 200U) && (zyxda == 0x00U) && (status == LIS3DSH_OK));

		if(status != LIS3DSH_OK)
		{
			break;
		}
		else if(zyxda == 0x00U)
		{
			status = LIS3DSH_ERROR_TIMEOUT;
			break;
		}
		else if(zyxda == 0x08)
		{
			init_state = INIT_OK;
		}
	}

	}while(0);

	sensor->init_state = init_state;
	if(sensor->init_state == INIT_OK)
	{
		sensor->init_status = LIS3DSH_INIT_OK;
	}
	else
	{
		sensor->init_status = LIS3DSH_INIT_ERROR;
	}

	return status;
}



Lis3dsh_StatusTypeDef readAxisRaw(Lis3dsh_t *sensor, const uint8_t axis, int16_t *value)
{
	if((sensor == NULL) || (value == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	if(sensor->init_status != LIS3DSH_INIT_OK)
	{
		return LIS3DSH_ERROR_NO_INIT;
	}

	Lis3dsh_StatusTypeDef status;

	status = readRawValues(sensor, axis, 1U, value);

	return status;
}

Lis3dsh_StatusTypeDef readAxesRaw(Lis3dsh_t *sensor, int16_t *values)
{
	if((sensor == NULL) || (values == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	if(sensor->init_status != LIS3DSH_INIT_OK)
	{
		return LIS3DSH_ERROR_NO_INIT;
	}

	Lis3dsh_StatusTypeDef status;

	status = readRawValues(sensor, X_AXIS, 3U, values);

	return status;
}

Lis3dsh_StatusTypeDef readAxesMg(Lis3dsh_t *sensor, int16_t *value_x, int16_t *value_y, int16_t *value_z)
{
	if((sensor == NULL) || (value_x == NULL) || (value_y == NULL) || (value_z == NULL))
	{
		return LIS3DSH_ERROR_NULL_POINTER;
	}

	if(sensor->init_status != LIS3DSH_INIT_OK)
	{
		return LIS3DSH_ERROR_NO_INIT;
	}

	Lis3dsh_StatusTypeDef status;

	int16_t values[3] = {0};

	status = readRawValues(sensor, X_AXIS, 3U, values);

	if(status != LIS3DSH_OK)
	{
		return status;
	}

	rawToMg(values, value_x, value_y, value_z);

	return status;
}




