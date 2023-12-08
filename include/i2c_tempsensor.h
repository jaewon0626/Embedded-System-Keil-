#ifndef __i2c_tempsensor_H__
#define __i2c_tempsensor_H__
#include "main.h"

extern void I2C_Init(I2C_TypeDef * I2Cx);
void I2C_WaitLineIdle(I2C_TypeDef * I2Cx);
void I2C_Start(I2C_TypeDef * I2Cx, uint16_t DevAddress);
void I2C_Stop(I2C_TypeDef * I2Cx);
void I2C_ReceiveData(I2C_TypeDef * I2Cx, uint16_t DevAddress, uint8_t *pdata, uint16_t Size);
void I2C_SendData(I2C_TypeDef * I2Cx, uint16_t DevAddress, uint8_t *pdata, uint16_t Size);
void TCM75A_Write(uint8_t Register, uint8_t Value);
int TCM75A_Read(uint8_t Register);
void display_temp(int temp);
#endif
