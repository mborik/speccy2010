#include "fpga.h"
#include "bus.h"
#include "cpu.h"
#include "../specConfig.h"
#include "../specKeyboard.h"

#define DCLK(x) GPIO_WriteBit(GPIO1, GPIO_Pin_6, x)
#define DATA(x) GPIO_WriteBit(GPIO1, GPIO_Pin_7, x)
#define nCONFIG(x) GPIO_WriteBit(GPIO1, GPIO_Pin_9, x)

#define CONFIG_DONE() GPIO_ReadBit(GPIO1, GPIO_Pin_5)
#define nSTATUS() GPIO_ReadBit(GPIO1, GPIO_Pin_8)

//---------------------------------------------------------------------------------------
byte timer_flag_1Hz = 0;
byte timer_flag_100Hz = 0;

dword fpgaConfigVersionPrev = 0;
dword fpgaStatus = FPGA_NONE;

dword romConfigPrev = -1;
//---------------------------------------------------------------------------------------
void FPGA_TestClock()
{
	if (fpgaStatus != FPGA_SPECCY2010)
		return;

	CPU_Stop();

	SystemBus_Write(0xc00050, 0);
	SystemBus_Write(0xc00050, 1);
	DelayMs(100);
	SystemBus_Write(0xc00050, 0);

	dword counter20 = SystemBus_Read(0xc00050) | (SystemBus_Read(0xc00051) << 16);
	dword counterMem = SystemBus_Read(0xc00052) | (SystemBus_Read(0xc00053) << 16);

	__TRACE("FPGA clock - %d.%.5d MHz\n", counter20 / 100000, counter20 % 100000);
	__TRACE("FPGA PLL clock - %d.%.5d MHz\n", counterMem / 100000, counterMem % 100000);

	CPU_Start();
}
//---------------------------------------------------------------------------------------
void FPGA_Config()
{
	if ((disk_status(0) & STA_NOINIT) != 0) {
		__TRACE("Cannot init SD card...\n");
		return;
	}

	FILINFO fpgaConfigInfo;
	char lfn[1];
	fpgaConfigInfo.lfname = lfn;
	fpgaConfigInfo.lfsize = 0;

	if (f_stat(specConfig.fpgaConfigName, &fpgaConfigInfo) != FR_OK) {
		__TRACE("FPGA_Config: '%s' not found, fallback to default...\n", specConfig.fpgaConfigName);
		strcpy(specConfig.fpgaConfigName, "speccy2010.rbf");

		if (f_stat(specConfig.fpgaConfigName, &fpgaConfigInfo) != FR_OK) {
			__TRACE("FPGA_Config: Cannot open '%s'!\n", specConfig.fpgaConfigName);
			return;
		}
	}

	dword fpgaConfigVersion = (fpgaConfigInfo.fdate << 16) | fpgaConfigInfo.ftime;

	__TRACE("FPGA_Config: chkver [current: %08x > new: %08x]\n", fpgaConfigVersionPrev, fpgaConfigVersion);

	if (fpgaConfigVersionPrev == fpgaConfigVersion) {
		__TRACE("FPGA_Config: The version of '%s' is match...\n", specConfig.fpgaConfigName);
		return;
	}

	FIL fpgaConfig;
	if (f_open(&fpgaConfig, specConfig.fpgaConfigName, FA_READ) != FR_OK) {
		__TRACE("FPGA_Config: Cannot open '%s'!\n", specConfig.fpgaConfigName);
		return;
	}
	else {
		__TRACE("FPGA_Config: '%s' is opened...\n", specConfig.fpgaConfigName);
	}
	//--------------------------------------------------------------------------

	__TRACE("FPGA_Config: Flashing started...\n");
	fpgaStatus = FPGA_NONE;

	GPIO_InitTypeDef GPIO_InitStructure;

	//BOOT0 pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIO0, &GPIO_InitStructure);

	//Reset FPGA pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	nCONFIG(Bit_SET);
	DCLK(Bit_SET);
	DATA(Bit_SET);

	nCONFIG(Bit_RESET);
	DelayMs(2);
	nCONFIG(Bit_SET);

	int i = 10;
	while (i-- > 0 && nSTATUS() != Bit_RESET)
		DelayMs(10);

	if (nSTATUS() == Bit_RESET) {
		__TRACE("FPGA_Config: Status OK...\n");

		for (dword pos = 0; pos < fpgaConfig.fsize; pos++) {
			byte data8;

			UINT res;
			if (f_read(&fpgaConfig, &data8, 1, &res) != FR_OK)
				break;
			if (res == 0)
				break;

			for (byte j = 0; j < 8; j++) {
				DCLK(Bit_RESET);

				DATA((data8 & 0x01) == 0 ? Bit_RESET : Bit_SET);
				data8 >>= 1;

				DCLK(Bit_SET);
			}

			if ((pos & 0xfff) == 0) {
				WDT_Kick();
				__TRACE(".");
			}
		}
	}

	f_close(&fpgaConfig);
	__TRACE("FPGA_Config: Flashing done...\n");

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	i = 10;
	while (i-- > 0 && !CONFIG_DONE())
		DelayMs(10);

	if (CONFIG_DONE()) {
		__TRACE("FPGA_Config: Finished...\n");
		fpgaConfigVersionPrev = fpgaConfigVersion;
	}
	else {
		__TRACE("FPGA_Config: Failed!\n");
		fpgaConfigVersionPrev = 0;
	}

	GPIO_WriteBit(GPIO1, GPIO_Pin_13, Bit_RESET); // FPGA RESET LOW
	DelayMs(100);
	GPIO_WriteBit(GPIO1, GPIO_Pin_13, Bit_SET); // FPGA RESET HIGH
	DelayMs(10);

	romConfigPrev = -1;
	SystemBus_TestConfiguration();

	if (fpgaStatus == FPGA_SPECCY2010) {
		__TRACE("Speccy2010 FPGA configuration found...\n");

		FPGA_TestClock();
		Keyboard_Reset();
		Mouse_Reset();

		Spectrum_UpdateConfig();
		Spectrum_UpdateDisks();
	}
	else {
		__TRACE("Wrong FPGA configuration...\n");
	}

	WDT_Kick();

	timer_flag_1Hz = 0;
	timer_flag_100Hz = 0;
}
//---------------------------------------------------------------------------------------
