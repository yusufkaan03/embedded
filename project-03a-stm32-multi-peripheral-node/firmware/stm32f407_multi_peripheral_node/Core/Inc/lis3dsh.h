/*
 * lis3dsh.h
 *
 *  Created on: Oct 19, 2026
 *      Author: yusuf
 */

#ifndef INC_LIS3DSH_H_
#define INC_LIS3DSH_H_

#include "stm32f4xx_hal.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define SPI_READ_DATA	1U
#define SPI_WRITE_DATA	0U

#define WHO_AM_I_VALUE	0x3FU

#define INFO_01_REG		0x0DU
#define INFO_02_REG		0x0EU
#define WHO_AM_I_REG	0x0FU
#define CTRL_REG4_REG	0x20U
#define CTRL_REG6_REG	0x25U
#define STATUS_REG		0x27U
#define OUT_X_L_REG		0x28U
#define OUT_X_H_REG		0x29U
#define OUT_Y_L_REG		0x2AU
#define OUT_Y_H_REG		0x2BU
#define OUT_Z_L_REG		0x2CU
#define OUT_Z_H_REG		0x2DU

#define OUT_AXIS_REG	0x28U
#define X_AXIS			0x00U
#define Y_AXIS			0x02U
#define Z_AXIS			0x04U

#define MAX_SPI_SIZE	64U

typedef enum
{
	LIS3DSH_OK,
	LIS3DSH_ERROR_SPI_READ,
	LIS3DSH_ERROR_SPI_WRITE,
	LIS3DSH_ERROR_OVERFLOW,
	LIS3DSH_ERROR_SPI_ZERO_LENGTH,
	LIS3DSH_ERROR_AXIS,
	LIS3DSH_ERROR_DATA_MISMATCH,
	LIS3DSH_ERROR_NULL_POINTER,
	LIS3DSH_ERROR_TIMEOUT,
	LIS3DSH_ERROR_NO_INIT,
	LIS3DSH_ERROR_WHO_AM_I,
	LIS3DSH_ERROR_SPI_BUSY,
	LIS3DSH_ERROR_SPI_TIMEOUT,
}Lis3dsh_StatusTypeDef;

typedef struct
{
	GPIO_TypeDef	*gpio_port;
	uint16_t		gpio_pin;
}SpiCsPins_t;

typedef enum
{
	LIS3DSH_INIT_WAIT,
	LIS3DSH_INIT_OK,
	LIS3DSH_INIT_ERROR,
}Lis3dsh_InitStatus_t;

typedef enum
{
	INIT_SETUP,
	INIT_WHO_AM_I,
	INIT_MEASUR_SETUP,
	INIT_OTO_REG_UPDATE,
	INIT_CHECK_UPDATE,
	INIT_OK,
}Init_state;


typedef struct
{
	SPI_HandleTypeDef 		*hspi;
	uint8_t					WHO_AM_I;
	SpiCsPins_t				gpıo;
	Lis3dsh_InitStatus_t	init_status;
	Init_state				init_state;
}Lis3dsh_t;


/*Lis3dsh_StatusTypeDef spiWriteRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, const uint8_t *data, const uint8_t size);

Lis3dsh_StatusTypeDef spiReadRegister(Lis3dsh_t *sensor, const uint8_t reg_adress, uint8_t *response, const uint8_t size);

Lis3dsh_StatusTypeDef writeRegisterAndVerify(Lis3dsh_t *sensor, const uint8_t reg_adress,
	const uint8_t *data, uint8_t *response, const uint8_t size);*/

Lis3dsh_StatusTypeDef readAxisRaw(Lis3dsh_t *sensor, const uint8_t axis, int16_t *value);

Lis3dsh_StatusTypeDef readAxesRaw(Lis3dsh_t *sensor, int16_t *values);

Lis3dsh_StatusTypeDef readAxesMg(Lis3dsh_t *sensor, int16_t *value_x, int16_t *value_y, int16_t *value_z);

Lis3dsh_StatusTypeDef sensorInit(Lis3dsh_t *sensor, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin);


#endif /* INC_LIS3DSH_H_ */
