#include "core.h"

//---------------------------------------------------------------------------------------
bool MRCC_Config(void)
{
	MRCC_DeInit();

	int i = 16;
	while (i > 0 && MRCC_WaitForOSC4MStartUp() != SUCCESS)
		i--;

	if (i != 0) {
		/* Set HCLK to 60 MHz */
		MRCC_HCLKConfig(MRCC_CKSYS_Div1);
		/* Set CKTIM to 60 MHz */
		MRCC_CKTIMConfig(MRCC_HCLK_Div1);
		/* Set PCLK to 30 MHz */
		MRCC_PCLKConfig(MRCC_CKTIM_Div2);

		CFG_FLASHBurstConfig(CFG_FLASHBurst_Enable);
		MRCC_CKSYSConfig(MRCC_CKSYS_OSC4MPLL, MRCC_PLL_Mul_15);
	}

	/* GPIO pins optimized for 3V3 operation */
	MRCC_IOVoltageRangeConfig(MRCC_IOVoltageRange_3V3);

	MRCC_PeripheralClockConfig(MRCC_Peripheral_GPIO, ENABLE);
	MRCC_PeripheralClockConfig(MRCC_Peripheral_TIM0, ENABLE);
	MRCC_PeripheralClockConfig(MRCC_Peripheral_I2C, ENABLE);
	MRCC_PeripheralClockConfig(MRCC_Peripheral_SSP0, ENABLE);

	return i > 0;
}
//---------------------------------------------------------------------------------------
void TIM0_Config()
{
	TIM_InitTypeDef TIM_InitStructure;
	TIM_InitStructure.TIM_Mode = TIM_Mode_OCTiming;
	TIM_InitStructure.TIM_Prescaler = 60 - 1;
	TIM_InitStructure.TIM_ClockSource = TIM_ClockSource_Internal;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Channel = TIM_Channel_2;
	TIM_InitStructure.TIM_Period = 25 - 1;
	TIM_Init(TIM0, &TIM_InitStructure);

	TIM_ClearFlag(TIM0, TIM_FLAG_OC1 | TIM_FLAG_OC2 | TIM_FLAG_Update);
	TIM_ITConfig(TIM0, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM0, ENABLE);

	EIC_IRQInitTypeDef EIC_IRQInitStructure;

	EIC_IRQInitStructure.EIC_IRQChannel = TIM0_UP_IRQChannel;
	EIC_IRQInitStructure.EIC_IRQChannelPriority = 1;
	EIC_IRQInitStructure.EIC_IRQChannelCmd = ENABLE;
	EIC_IRQInit(&EIC_IRQInitStructure);

	EIC_IRQCmd(ENABLE);
}
//---------------------------------------------------------------------------------------
void I2C_Config()
{
	/* SCL and SDA I2C pins configuration */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIO0, &GPIO_InitStructure);

	/* I2C configuration */
	I2C_InitTypeDef I2C_InitStructure;
	I2C_InitStructure.I2C_GeneralCall = I2C_GeneralCall_Disable;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_CLKSpeed = 100000;
	I2C_InitStructure.I2C_OwnAddress = 0xA0;

	/* I2C Peripheral Enable */
	I2C_Cmd(ENABLE);
	/* Apply I2C configuration after enabling it */
	I2C_Init(&I2C_InitStructure);
}
//---------------------------------------------------------------------------------------
void SPI_Config()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIO0, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	GPIO_WriteBit(GPIO1, GPIO_Pin_10, Bit_SET);
	GPIO_WriteBit(GPIO0, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7, Bit_SET);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_18 | GPIO_Pin_19;
	GPIO_Init(GPIO2, &GPIO_InitStructure);


	SSP_InitTypeDef SSP_InitStructure;
	SSP_InitStructure.SSP_FrameFormat = SSP_FrameFormat_Motorola;
	SSP_InitStructure.SSP_Mode = SSP_Mode_Master;
	SSP_InitStructure.SSP_CPOL = SSP_CPOL_Low;
	SSP_InitStructure.SSP_CPHA = SSP_CPHA_1Edge;
	SSP_InitStructure.SSP_DataSize = SSP_DataSize_8b;
	SSP_InitStructure.SSP_NSS = SSP_NSS_Soft;
	SSP_InitStructure.SSP_ClockRate = 0;
	SSP_InitStructure.SSP_ClockPrescaler = 2; /* SSP baud rate : 30 MHz/(4*(2+1))= 2.5MHz */
	SSP_Init(SSP0, &SSP_InitStructure);

	SSP_Cmd(SSP0, ENABLE);
}
//---------------------------------------------------------------------------------------
