#include <stdio.h>
#include <string.h>

#include "fatfs/diskio.h"
#include "fatfs/ff.h"

#include "system.h"

#define MAIN_PROG_START 0x20008000
#define RESET_WRITE_PROTECTION_BITS 0


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

	//----------------------------------------------------------------------------------------------------

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

void UART0_SetBaudRate(dword baudRate)
{
	UART_InitTypeDef UART_InitStructure;
	UART_InitStructure.UART_WordLength = UART_WordLength_8D;
	UART_InitStructure.UART_StopBits = UART_StopBits_1;
	UART_InitStructure.UART_Parity = UART_Parity_No;
	UART_InitStructure.UART_BaudRate = baudRate;
	UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
	UART_InitStructure.UART_Mode = UART_Mode_Tx_Rx;
	UART_InitStructure.UART_FIFO = UART_FIFO_Disable;

	UART_Cmd(UART0, DISABLE);
	UART_DeInit(UART0);
	UART_Init(UART0, &UART_InitStructure);

	UART_Cmd(UART0, ENABLE);
}

void UART0_Init(GPIO_TypeDef *tx_gpio, dword tx_pin, GPIO_TypeDef *rx_gpio, dword rx_pin)
{
	MRCC_PeripheralClockConfig(MRCC_Peripheral_UART0 | MRCC_Peripheral_GPIO, ENABLE);

	//---------------------------------------------

	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure the UART0_Tx as Alternate function Push-Pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = tx_pin;
	GPIO_Init(tx_gpio, &GPIO_InitStructure);

	/* Configure the UART0_Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = rx_pin;
	GPIO_Init(rx_gpio, &GPIO_InitStructure);

	UART0_SetBaudRate(115200);
}

void UART0_WriteFile(const byte *s, dword cnt)
{
	while (cnt) {
		if (UART_GetFlagStatus(UART0, UART_FLAG_TxFIFOFull) == RESET) {
			UART_SendData(UART0, *s++);
			cnt--;
		}
	}

	while (UART_GetFlagStatus(UART0, UART_FLAG_Busy) == SET);
}

void SD_Init()
{
	if ((disk_status(0) & STA_NODISK) == 0 && (disk_status(0) & STA_NOINIT) != 0) {
		disk_initialize(0);

		__TRACE("SD card init ");
		if ((disk_status(0) & STA_NOINIT) != 0) {
			__TRACE("error\n");
		}
		else {
			__TRACE("OK...\n");

			static FATFS fatfs;
			f_mount(0, &fatfs);
		}
	}
}

static char str[10];
char *NumToHex(dword num, char *str, int len)
{
	str[len] = '\0';

	while (--len >= 0) {
		dword rem = num % 16;
		str[len] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num /= 16;
	}

	return str;
}

void FlashPage(byte secnum, dword sector, dword startAddr, dword endAddr, FIL newFirmware)
{
	char isError = 0;
	dword data4;
	dword storedData4;
	int pos;
	UINT res;
	byte *flashData;

	if (newFirmware.fsize < startAddr - 0x20008000)
		return;

	if (newFirmware.fsize <= endAddr - 0x20008000)
		endAddr = newFirmware.fsize + 0x20008000;
	else
		endAddr = endAddr;

	do {
		__TRACE("\nFlashing sector #");
		__TRACE(NumToHex(secnum, str, 1));
		__TRACE(" [START:0x");
		__TRACE(NumToHex(startAddr, str, 8));
#if RESET_WRITE_PROTECTION_BITS
		__TRACE(", WP:");
		__TRACE(FLASH_GetWriteProtectionStatus(sector) ? "on": "off");
#endif
		__TRACE("]\n");

		__TRACE("- erasing...");
		FLASH_ClearFlag();
		FLASH_EraseSector(sector);
		FLASH_WaitForLastOperation();

		__TRACE("\n- flashing.");

#if RESET_WRITE_PROTECTION_BITS
		FLASH_WriteProtectionCmd(sector, DISABLE);
#endif

		flashData = (byte *) startAddr;
		f_lseek(&newFirmware, startAddr - 0x20008000);

		for (pos = 0; pos < endAddr - startAddr; pos += 4, flashData += 4) {
			if (((dword) flashData & 0xfff) == 0)
				__TRACE(".");

			f_read(&newFirmware, &data4, 4, &res);

			FLASH_ClearFlag();
			FLASH_WriteWord((dword) flashData - 0x20000000, data4);
			FLASH_WaitForLastOperation();

			storedData4 = *flashData + (*(flashData + 1) << 8) + (*(flashData + 2) << 16) + (*(flashData + 3) << 24);
			if (data4 != storedData4) {
				__TRACE("\n! error at 0x");
				__TRACE(NumToHex((dword) flashData, str, 8));
				__TRACE(" [RD:");
				__TRACE(NumToHex(data4, str, 8));
				__TRACE(" > WR:");
				__TRACE(NumToHex(storedData4, str, 8));
#if !RESET_WRITE_PROTECTION_BITS
				__TRACE(", STATUS:");
				__TRACE(NumToHex(FLASH_GetFlagStatus(), str, 3));
#endif
				__TRACE("]");
				isError++;
				break;
			}
		}

		__TRACE("\n");
	} while (isError && isError <= 3);
}

void UpdateFirmware()
{
	if ((disk_status(0) & STA_NOINIT) != 0)
		return;

	FIL newFirmware;
	if (f_open(&newFirmware, "speccy2010.bin", FA_READ) != FR_OK) {
		__TRACE("Missing speccy2010.bin. ");
		__TRACE("Skipping firmware upgrade...\n");
		return;
	}

	dword pos;
	byte data;
	UINT res;
	byte olddata, *flashData = (byte *) MAIN_PROG_START;

	for (pos = 0; pos < newFirmware.fsize; pos++) {
		f_read(&newFirmware, &data, 1, &res);
		olddata = *flashData++;
		if (data != olddata) {
			__TRACE("Firmware differ at 0x");
			__TRACE(NumToHex(pos, str, 6));
			__TRACE(" [SD:");
			__TRACE(NumToHex(data, str, 2));
			__TRACE(" > ROM:");
			__TRACE(NumToHex(olddata, str, 2));
			__TRACE("]\n");
			break;
		}
	}

	if (pos >= newFirmware.fsize) {
		__TRACE("Skipping firmware upgrade...\n");
		return;
	}

	__TRACE("Firmware upgrade started...\n");

	FLASH_DeInit();

	FlashPage(4, FLASH_BANK0_SECTOR4, 0x20008000, 0x20010000, newFirmware);
	FlashPage(5, FLASH_BANK0_SECTOR5, 0x20010000, 0x20020000, newFirmware);
	FlashPage(6, FLASH_BANK0_SECTOR6, 0x20020000, 0x20030000, newFirmware);
	FlashPage(7, FLASH_BANK0_SECTOR7, 0x20030000, 0x20040000, newFirmware);

	__TRACE("Firmware upgrade finished...\n");
}

void JumpToMainProg()
{
	void (*MainProg)() = (void *) MAIN_PROG_START;
	MainProg();
}

int main()
{
	MRCC_Config();

	UART0_Init(GPIO0, GPIO_Pin_11, GPIO0, GPIO_Pin_10);
	__TRACE("\nSpeccy2010 booter: v1.2.0\n");

	SPI_Config();
	SD_Init();

	UpdateFirmware();
	JumpToMainProg();

	return 0;
}

void __TRACE(const char *str)
{
	char lastChar = 0;
	const char *strPos = str;

	while (*strPos != 0) {
		lastChar = *strPos++;

		if (lastChar == '\n')
			UART0_WriteFile((byte *) "\r\n", 2);
		else
			UART0_WriteFile((byte *) &lastChar, 1);
	}
}

void WDT_Kick() {}
