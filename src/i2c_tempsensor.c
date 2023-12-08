#include "stm32f446xx.h"  
#include "main.h"
#include "i2c_tempsensor.h"
#include "fnd.h"
#include "uart.h"
void I2C_Init(I2C_TypeDef * I2Cx)
{
	uint32_t Slaveaddr = (0x4C<<1);//100_1100 << 1, OAR1 7~1bit = slave_addr
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;//Enable Clock GPIOB
	RCC->APB1ENR |= (1 << 21);//Enable Clock I2C1
	GPIOB->MODER &= ~( 3UL<<(2*TEMP_SDA_Pin) | 3UL<<(2*TEMP_SCL_Pin) );  
	GPIOB->MODER |=  ( 2UL<<(2*TEMP_SDA_Pin)) | (2UL<<(2*TEMP_SCL_Pin));  //alternate function mode(10)
	GPIOB->AFR[1]|=((4UL<<0|(4UL<<4*1)));	//  in AF4, pin PB8 functions as I2C1_SCL and pin PB9 function as I2C1_SDA.
	//AFR[0]: GPIO 0~7, AFR[1] : GPIO 8~15, we use GPIO 8,9  
	GPIOB->OTYPER |=  ( 1UL<<(TEMP_SDA_Pin)) | (1UL<<(TEMP_SCL_Pin));//Open drain
	GPIOB->PUPDR |=  ( 1UL<<(2*TEMP_SDA_Pin)) | (1UL<<(2*TEMP_SCL_Pin)); //Enable Pull-up
	
	
	I2Cx->CR1 |= (I2C_CR1_SWRST);//soft reset
	I2Cx->CR1 &= ~(I2C_CR1_SWRST);
  I2Cx->CR1 &= ~I2C_CR1_PE; //Disable the selected I2C peripheral
	
	
//Noise Filter
	I2Cx->FLTR &= ~I2C_FLTR_ANOFF;
	I2Cx->FLTR &= ~I2C_FLTR_DNF;

	I2Cx->CR2 |= (1U<<4); //USE 16Mhz Clock Frequency
	
	I2Cx->TRISE = 17; //Configure Rise Time, Rise_Time = Clock_Speed<= 100000 ? (freq + 1) : (((freq * 300) / 1000) + 1), We use I2C Clock speed 100KHz
	
	I2Cx->CCR |= 80;//Set I2C clock to standard mode 100KHz, T_high = CCR * clock_T, T_low = CCR * clock_T 
	
	I2Cx->OAR1 |= I2C_OAR1_ADDMODE | Slaveaddr; //Own address 1 configuration for slave mode
	
/*
  I2Cx->CR2 |= I2C_CR2_ITERREN; //Error interrupt enable
	I2Cx->CR1 &= ~I2C_CR1_NOSTRETCH;
*/

	I2Cx->CR1 |= I2C_CR1_PE; //Enable I2C
};

void I2C_WaitLineIdle(I2C_TypeDef * I2Cx)
{
	while((I2Cx->SR2) & (I2C_SR2_BUSY)); // Wait until I2C no busy
}

void I2C_Start(I2C_TypeDef * I2Cx, uint16_t DevAddress)
{
	volatile int tmp;
	I2Cx->CR1 &= ~I2C_CR1_POS; //Disable Pos
	I2Cx->CR1 |= I2C_CR1_ACK; //Enable Ack
	I2Cx->CR1 |= I2C_CR1_START; //Generate Start Signal
	while(!((I2Cx->SR1) & (I2C_SR1_SB))); // Wait until Start condition generate
	I2Cx->DR = (DevAddress);//Write Device Address + Write bit
	while(!((I2Cx->SR1) & (I2C_SR1_ADDR))); //Wait until ADDR flag is set
	// Clear Addr Flag
	tmp = I2Cx->SR2; 
	tmp = I2Cx->SR1;
}
void I2C_Stop(I2C_TypeDef * I2Cx)
{
	I2Cx->CR1 |= I2C_CR1_STOP;//Generate Stop Signal
}

void I2C_ReceiveData(I2C_TypeDef * I2Cx, uint16_t DevAddress, uint8_t *pdata, uint16_t Size)
{
	int i;
	if (Size <= 0 || pdata == 0) return;
	I2C_WaitLineIdle(I2Cx);
	I2C_Start(I2Cx, ((DevAddress << 1) | 1));
	if(Size==2)I2Cx->CR1 |= I2C_CR1_POS; //Enable Pos
	for(i=0;i<Size;i++)
	{
		while(!((I2Cx->SR1) & (I2C_SR1_RXNE)));
		pdata[i]=I2Cx->DR;
	}
	I2C_Stop(I2Cx);
}	

void I2C_SendData(I2C_TypeDef * I2Cx, uint16_t DevAddress, uint8_t *pdata, uint16_t Size)
{
	int i;
	if (Size <= 0 || pdata == 0) return;
	I2C_WaitLineIdle(I2Cx);
	I2C_Start(I2Cx, ((DevAddress <<1 ) | 0));
	I2Cx->CR1 &= ~I2C_CR1_ACK; //Disable Ack
	for(i=0;i<Size;i++)
	{
		while(!((I2Cx->SR1) & (I2C_SR1_TXE)));
		I2Cx->DR = pdata[i];
	}
	I2C_Stop(I2Cx);
}

void TCM75A_Write(uint8_t Register, uint8_t Value)
{
	uint8_t SendData[10];
	SendData[0] = Register;
	SendData[1] = Value;
	I2C_SendData(I2C1, LM75A_ADDR, SendData, 2);
}
int TCM75A_Read(uint8_t Register)
{
	int Value;
	uint8_t SendData[10];
	uint8_t RevData[10];
	SendData[0] = Register;
	I2C_SendData(I2C1, LM75A_ADDR, SendData, 1);
	I2C_ReceiveData(I2C1,LM75A_ADDR,RevData, 2);
	Value = (*RevData<<8) | *(RevData+1);//Value={RevData[0],RevData[1])
	return Value;
}

void display_temp(int temp)
{
	int sign, decimal;
	if((temp&(1<<15)))//if negative value
	{
		sign = 0; //sign bit clear (negative)
		temp = (~temp & 0x7fe0) + 0x20;
	}
	else sign = 1;//sign bit set (positve)
	
	if(temp&(1<<7))
	{
		decimal=1;
		temp = temp>>8;
	}
	else
	{
		decimal=0;
		temp= temp>>8;
	}
	if(temp>99) temp=99;
	if(sign==1)
		digit3 = 10;
	else	
		digit3 = 11;
	  digit2 = (temp/10);
		digit1 = (temp%10);
	if(decimal == 1)
		digit0 = 5;
	else
		digit0 = 0;
}




