#include <malloc.h>
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "system.h"

dword tickCounter = 0;

//---------------------------------------------------------------------------------------
DWORD get_fattime()
{
	return (34 << 25) | (10 << 21) | (11 << 16) | (16 << 11) | (30 << 5);
}

dword Timer_GetTickCounter() // 100 Hz
{
	return tickCounter;
}

void WDT_Config()
{
	WDG_InitTypeDef WDG_InitStructure;
	WDG_InitStructure.WDG_Mode = WDG_Mode_WDG;
	WDG_InitStructure.WDG_Preload = 0xffff;
	WDG_InitStructure.WDG_Prescaler = 0xff;
	WDG_Init(&WDG_InitStructure);
	WDG_Cmd(ENABLE);
}

void WDT_Kick()
{
	WDG_Cmd(ENABLE);
}

void FPGA_Init()
{
	FPGA_Config();

	SystemBus_Write(0xc00040, 0x12);
	SystemBus_Write(0xc00041, 0x23);
	SystemBus_Write(0xc00042, 0x8001);
	SystemBus_Write(0xc00045, 0);
	SystemBus_Write(0xc00046, 0);
	SystemBus_Write(0xc00000, 8);

	CPU_Start();
	CPU_Reset(true);
	DelayMs(10);
	CPU_Reset(false);
	DelayMs(100);
	CPU_Stop();
}

void SD_Init()
{
	DSTATUS state = disk_status(0);

	if ((state & (STA_NODISK | STA_NOINIT)) == STA_NOINIT) {
		__TRACE("SD card initialization...\n");
		state = disk_initialize(0);

		if ((state & STA_NOINIT) != 0) {
			__TRACE("SD card init error (%x)\n", state);
		}
		else {
			__TRACE("SD card init OK..\n");

			FATFS *fatfs = new FATFS;
			FRESULT result = f_mount(0, fatfs);

			if (result != FR_OK) {
				__TRACE("FAT mounting error (%d)\n", result);
				return;
			}

			FPGA_Init();
		}
	}
}


bool TestPage(unsigned page)
{
	bool result = true;
	int writeErrors = 0;
	int readErrors = 0;
	dword pos, addr;
	word data = 0xAAAA, temp;
	int i, k;

	__TRACE("\n\nTesting page %03X [range 0x%06x~0x%06x]:", page,
		(page * 0x4000), ((page + 1) * 0x4000) - 1);

	SystemBus_Write(0xc00020, 0); // use bank 0

	for (k = 0; k < 2; k++) {
		__TRACE("\n- pattern[0x%02X]  W", (data & 0xff));

		addr = 0x800000 | ((page * 0x4000) >> 1);
		for (pos = 0; pos < 0x4000; pos += 2) {
			SystemBus_Write(addr, data);

			addr++;
			if ((addr & 0x01ff) == 0) {
				WDT_Kick();
				__TRACE(".");
			}
		}

		__TRACE(" R");
		addr = 0x800000 | ((page * 0x4000) >> 1);
		for (pos = 0; pos < 0x4000; pos += 2) {
			temp = SystemBus_Read(addr);

			if (temp != data) {
				for (i = 0; i < 3; i++) {
					temp = SystemBus_Read(addr);
					if (temp == data)
						break;
				}

				if (temp == data)
					readErrors++;
				else
					writeErrors++;
			}

			addr++;
			if ((addr & 0x01ff) == 0) {
				WDT_Kick();
				__TRACE(".");
			}
		}

		data = data ^ 0xFFFF;
	}

	if (readErrors > 0 || writeErrors > 0) {
		__TRACE("\nMemory errors in page %d!\n", page);
		__TRACE(" - read errors : %d\n", readErrors);
		__TRACE(" - write errors : %d\n", writeErrors);

		result = false;
	}

	return result;
}

void Timer_PageTester()
{
	static unsigned page = 0;

	TestPage(page);

	page = (page + 1) % 0x200;
}

void Timer_Routine()
{
	static byte prev_status = STA_NODISK;

	byte status = (disk_status(0) & STA_NODISK);
	if (status != prev_status) {
		if (status == 0) {
			DelayMs(10);
			SD_Init();
		}
		else {
			__TRACE("SD card removed from slot!");
		}

		prev_status = status;
	}

	if (timer_flag_100Hz) // 100Hz
	{
		portENTER_CRITICAL();
		timer_flag_100Hz--;
		portEXIT_CRITICAL();

		tickCounter++;
		WDT_Kick();
	}

	if (timer_flag_1Hz > 0) // 1Hz
	{
		//__TRACE("disk status - %x\n", disk_status(0));

		SD_Init();
		if (fpgaConfigVersionPrev == 0 && (disk_status(0) & STA_NOINIT) == 0)
			FPGA_Init();
		else if (fpgaConfigVersionPrev != 0)
			Timer_PageTester();

		portENTER_CRITICAL();
		timer_flag_1Hz--;
		portEXIT_CRITICAL();
	}
}


const int MAX_CMD_SIZE = 0x20;
static char cmd[MAX_CMD_SIZE + 1];
static int cmdSize = 0;
static bool traceNewLine = false;

void Serial_Routine()
{
	while (uart0.GetRxCntr() > 0) {
		char temp = uart0.ReadByte();

		if (temp == 0x0a) {
			cmd[cmdSize] = 0;
			cmdSize = 0;

			if (strcmp(cmd, "reset") == 0)
				while (true);
			else {
				__TRACE("cmd: %s\n", cmd);
			}
		}
		else if (temp == 0x08) {
			if (cmdSize > 0) {
				cmdSize--;

				const char delStr[2] = { 0x20, 0x08 };
				uart0.WriteFile((byte *)delStr, 2);
			}
		}
		else if (temp != 0x0d && cmdSize < MAX_CMD_SIZE) {
			cmd[cmdSize++] = temp;
		}
	}
}

void __TRACE(const char *str, ...)
{
	static char fullStr[0x80];

	va_list ap;
	va_start(ap, str);
	vsniprintf(fullStr, sizeof(fullStr), str, ap);
	va_end(ap);

	if (traceNewLine) {
		Serial_Routine();
		const char delChar = 0x08;
		for (int i = 0; i <= cmdSize; i++)
			uart0.WriteFile((byte *) &delChar, 1);
	}

	char lastChar = 0;
	char *strPos = fullStr;

	while (*strPos != 0) {
		lastChar = *strPos++;

		if (lastChar == '\n')
			uart0.WriteFile((byte *) "\r\n", 2);
		else
			uart0.WriteFile((byte *) &lastChar, 1);

		WDT_Kick();
	}

	traceNewLine = (lastChar == '\n');

	if (traceNewLine) {
		uart0.WriteFile((byte *) ">", 1);

		WDT_Kick();
		Serial_Routine();

		if (cmdSize > 0)
			uart0.WriteFile((byte *)cmd, cmdSize);
	}
}

volatile dword delayTimer = 0;
volatile dword tapeTimer = 0;

volatile bool bdiTimerFlag = true;
volatile dword bdiTimer = 0;

void TIM0_UP_IRQHandler() //40kHz
{
	static dword tick = 0;

	if (delayTimer >= 25)
		delayTimer -= 25;
	else
		delayTimer = 0;

	if (bdiTimerFlag)
		bdiTimer += 25;

	tick++;
	if (tick >= 400) // 100Hz
	{
		tick = 0;

		static dword tick2 = 0;

		tick2++;
		if (tick2 == 100) // 1Hz
		{
			tick2 = 0;
			timer_flag_1Hz++;
		}

		disk_timerproc();
		timer_flag_100Hz++;
	}

	//---------------------------------------------------

	TIM_ClearITPendingBit(TIM0, TIM_IT_Update);
}

void DelayUs(dword delay)
{
	word prevValue = TIM0->CNT;
	delay++;

	while (delay) {
		if (TIM0->CNT != prevValue) {
			prevValue = TIM0->CNT;
			delay--;
		}
	}

	WDT_Kick();
}

void DelayMs(dword delay)
{
	delayTimer = delay * 1000;
	while (delayTimer)
		WDT_Kick();
}

dword get_ticks()
{
	return bdiTimer;
}


dword prevEIC_ICR = 0;
dword nesting = 0;

void portENTER_CRITICAL()
{
	if (nesting == 0) {
		dword temp = EIC->ICR & 3;
		EIC->ICR &= ~3;
		prevEIC_ICR = temp;
	}

	nesting++;
}

void portEXIT_CRITICAL()
{
	if (nesting) {
		nesting--;

		if (nesting == 0) {
			EIC->ICR |= prevEIC_ICR;
		}
	}
}

//---------------------------------------------------------------------------------------

int main()
{
	bool pllStatusOK = MRCC_Config();
	TIM0_Config();
	I2C_Config();
	SPI_Config();

	WDT_Config();

	uart0.Init(GPIO0, GPIO_Pin_11, GPIO0, GPIO_Pin_10, 2);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 1 << 14;
	GPIO_Init(GPIO2, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 0xffff0000;
	GPIO_Init(GPIO0, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = 0x07ff;
	GPIO_Init(GPIO2, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //reset
	GPIO_InitStructure.GPIO_Pin = 1 << 13;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // zet_fifo_ready
	GPIO_InitStructure.GPIO_Pin = 1 << 2;
	GPIO_Init(GPIO0, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // zet_fifo_full
	GPIO_InitStructure.GPIO_Pin = 1 << 12;
	GPIO_Init(GPIO1, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = 0x0800;
	GPIO_Init(GPIO2, &GPIO_InitStructure);

	__TRACE("\nSpeccy2010 MEMTESTER!\n");

	if (!pllStatusOK)
		__TRACE("PLL initialization error !\n");
	DelayMs(100);

	while (true) {
		Timer_Routine();
		Serial_Routine();
	}
}

//---------------------------------------------------------------------------------------
// override original libc overhead implementation of new/delete

void *operator new(std::size_t size) { return malloc(size); }
void *operator new[](std::size_t size) { return malloc(size); }
void operator delete(void *ptr) { free(ptr); }
void operator delete[](void *ptr) { free(ptr); }

//---------------------------------------------------------------------------------------

extern "C" void __cxa_guard_acquire() {}
extern "C" void __cxa_guard_release() {}
