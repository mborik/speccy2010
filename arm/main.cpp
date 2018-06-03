#include <malloc.h>
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "system.h"
#include "specBetadisk.h"
#include "specConfig.h"
#include "specKeyboard.h"
#include "specRtc.h"
#include "specTape.h"

bool LOG_BDI_PORTS = false;
bool LOG_WAIT = false;

dword tickCounter = 0;

int mouseInited = 0;
const int MOUSE_OK = 4;
int mouseResetTimer = 0;

//---------------------------------------------------------------------------------------
DWORD get_fattime()
{
	tm currentTime;
	RTC_GetTime(&currentTime);

	return ((currentTime.tm_year - 80) << 25) |
		((currentTime.tm_mon + 1) << 21) |
		(currentTime.tm_mday << 16) |
		(currentTime.tm_hour << 11) |
		(currentTime.tm_min << 5) |
		(currentTime.tm_sec >> 1);
}

dword Timer_GetTickCounter() // 100 Hz
{
	return tickCounter;
}

void Mouse_Reset()
{
	word mouseCode = SystemBus_Read(0xc00033);
	while ((mouseCode & 0x8000) != 0)
		mouseCode = SystemBus_Read(0xc00033);

	mouseInited = 0;
	SystemBus_Write(0xc00033, 0xff);

	mouseResetTimer = 0;
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

			RestoreConfig();
			FPGA_Config();
		}
	}
}

//---------------------------------------------------------------------------------------
bool Spectrum_LoadRomPage(int page, const char *romFileName)
{
	FIL romImage;
	if (f_open(&romImage, romFileName, FA_READ) != FR_OK) {
		__TRACE("Cannot open rom image %s \n", romFileName);
		return false;
	}

	bool result = true;

	int writeErrors = 0;
	int readErrors = 0;

	CPU_Stop();
	SystemBus_Write(0xc00020, 0); // use bank 0
	dword addr = 0x800000 | (((0x100 + page) * 0x4000) >> 1);

	f_lseek(&romImage, 0);

	dword pos;
	for (pos = 0; pos < romImage.fsize; pos += 2) {
		word data;

		UINT res;
		if (f_read(&romImage, &data, 2, &res) != FR_OK)
			break;
		if (res == 0)
			break;

		word temp;
		//temp = SystemBus_Read(addr);
		//if (temp != data) __TRACE("a: 0x%.4x -> r: 0x%.4x w: 0x%.4x\n", addr << 1, temp, data);

		SystemBus_Write(addr, data);
		temp = SystemBus_Read(addr);

		if (temp != data) {
			for (int i = 0; i < 3; i++) {
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
		if ((addr & 0x0fff) == 0)
			WDT_Kick();
	}

	if (readErrors > 0 || writeErrors > 0) {
		__TRACE("Memory errors !\n");
		__TRACE(" - read errors : %d\n", readErrors);
		__TRACE(" - write errors : %d\n", writeErrors);

		result = false;
	}

	f_close(&romImage);
	CPU_Start();

	return result;
}

void Spectrum_InitRom()
{
	__TRACE("ROM configuration started...\n");

	specConfig.specUseBank0 = false;

	if (specConfig.specRom == SpecRom_Classic48) {
		Spectrum_LoadRomPage(1, "roms/trdos.rom");
		Spectrum_LoadRomPage(3, "roms/48.rom");
	}
	else if (specConfig.specRom == SpecRom_Scorpion) {
		Spectrum_LoadRomPage(0, "roms/scorpion.rom");
	}
	else {
		specConfig.specUseBank0 = Spectrum_LoadRomPage(0, "roms/system.rom");
		Spectrum_LoadRomPage(1, "roms/trdos.rom");
		Spectrum_LoadRomPage(2, "roms/pentagon.rom");
	}

	__TRACE("ROM configuration finished...\n");

	SystemBus_Write(0xc00021, 0x0000);
	SystemBus_Write(0xc00022, 0x0000);

	ResetKeyboard();
	RTC_Update();

	CPU_Reset(true);
	CPU_Reset(false);

	WDT_Kick();
	timer_flag_1Hz = 0;
	timer_flag_100Hz = 0;
}

void Spectrum_UpdateConfig()
{
	if (fpgaStatus != FPGA_SPECCY2010)
		return;

	SystemBus_Write(0xc00040, specConfig.specRom);
	SystemBus_Write(0xc00041, specConfig.specSync);
	SystemBus_Write(0xc00042, specConfig.specTurbo);

	SystemBus_Write(0xc00043, specConfig.specVideoMode);
	SystemBus_Write(0xc00048, specConfig.specVideoAspectRatio);

	SystemBus_Write(0xc00046, specConfig.specDacMode);
	SystemBus_Write(0xc00047, specConfig.specAyMode);
	SystemBus_Write(0xc00045, specConfig.specTurboSound | ((specConfig.specCovox << 1) & 0x0E) | (specConfig.specAyYm << 4));

	floppy_set_fast_mode(specConfig.specBdiMode);

	if (romConfigPrev != (dword)specConfig.specRom) {
		Spectrum_InitRom();
		romConfigPrev = specConfig.specRom;
	}
}

void Spectrum_UpdateDisks()
{
	for (int i = 0; i < 4; i++) {
		fdc_open_image(i, specConfig.specImages[i].name);
		floppy_disk_wp(i, &specConfig.specImages[i].readOnly);
	}
}

void SpectrumTimer_Routine()
{
	static dword joyCodePrev = 0;
	dword joyCode = SystemBus_Read(0xc00030) | (SystemBus_Read(0xc00031) << 16);

	if (joyCode != joyCodePrev) {
		//if (joyCode != 0) __TRACE("joyCode - 0x%.8x\n", joyCode);

		dword bit = 1;
		dword joyCodeDiff = joyCode ^ joyCodePrev;

		for (byte i = 0; i < 32; i++, bit <<= 1) {
			if ((joyCodeDiff & bit) != 0) {
				if ((joyCode & bit) != 0)
					joystick.WriteByte(i);
				else
					joystick.WriteByte(0x80 | i);
			}
		}

		joyCodePrev = joyCode;
	}

	//--------------------------------------------------------------------------

	word keyCode = SystemBus_Read(0xc00032);
	while ((keyCode & 0x8000) != 0) {
		//__TRACE( "keyCode : 0x%.2x, 0x%.2x\n", keyCode & 0xff, ( keyCode >> 8 ) & 0x0f );

		keyboard.WriteByte((byte)keyCode);
		keyCode = SystemBus_Read(0xc00032);
	}

	//--------------------------------------------------------------------------

	word mouseCode = SystemBus_Read(0xc00033);
	while ((mouseCode & 0x8000) != 0) {
		if (mouseCode & 0x4000) {
			//__TRACE( "Mouse buffer overflow\n" );
			Mouse_Reset();
			break;
		}

		mouseCode &= 0xff;
		//__TRACE( "mouse code - 0x%.2x\n", mouseCode );

		static byte mouseBuff[4];
		static byte mouseBuffPos = 0;

		if (mouseInited == 0 && mouseCode == 0xfa)
			mouseInited++;
		else if (mouseInited == 1 && mouseCode == 0xaa)
			mouseInited++;
		else if (mouseInited == 2 && mouseCode == 0x00) {
			mouseInited++;
			SystemBus_Write(0xc00033, 0xf4);
		}
		else if (mouseInited == 3 && mouseCode == 0xfa) {
			mouseInited++;
			mouseBuffPos = 0;

			__TRACE("PS/2 mouse init OK..\n");

			SystemBus_Write(0xc0001e, 0x5555);
			SystemBus_Write(0xc0001f, 0xff);
		}
		else if (mouseInited == MOUSE_OK) {
			static dword x = 0x55555555;
			static dword y = 0x55555555;

			mouseBuff[mouseBuffPos++] = mouseCode;

			if (mouseBuffPos >= 3) {
				int dx, dy;

				//__TRACE("mouse 0x%.2x.0x%.2x.0x%.2x, ", mouseBuff[0], mouseBuff[1], mouseBuff[2]);

				if ((mouseBuff[0] & (1 << 6)) == 0) {
					if ((mouseBuff[0] & (1 << 4)) == 0)
						dx = mouseBuff[1];
					else
						dx = mouseBuff[1] - 256;
				}
				else {
					if ((mouseBuff[0] & (1 << 4)) == 0)
						dx = 256;
					else
						dx = -256;
				}

				if ((mouseBuff[0] & (1 << 7)) == 0) {
					if ((mouseBuff[0] & (1 << 5)) == 0)
						dy = mouseBuff[2];
					else
						dy = mouseBuff[2] - 256;
				}
				else {
					if ((mouseBuff[0] & (1 << 5)) == 0)
						dy = 256;
					else
						dy = -256;
				}

				x += dx;
				y += dy;

				byte buttons = 0xff;
				if (!specConfig.specMouseSwap) {
					if ((mouseBuff[0] & (1 << 0)) != 0)
						buttons ^= (1 << 1);
					if ((mouseBuff[0] & (1 << 1)) != 0)
						buttons ^= (1 << 0);
				}
				else {
					if ((mouseBuff[0] & (1 << 0)) != 0)
						buttons ^= (1 << 0);
					if ((mouseBuff[0] & (1 << 1)) != 0)
						buttons ^= (1 << 1);
				}

				if ((mouseBuff[0] & (1 << 2)) != 0)
					buttons ^= (1 << 2);

				byte sx = x >> (byte)((6 - specConfig.specMouseSensitivity));
				byte sy = y >> (byte)((6 - specConfig.specMouseSensitivity));

				SystemBus_Write(0xc0001e, sx | (sy << 8));
				SystemBus_Write(0xc0001f, buttons);

				mouseBuffPos = 0;

				//__TRACE("x - 0x%.2x, y - 0x%.2x, b - 0x%.2x\n", sx, sy, buttons);
			}
		}

		mouseCode = SystemBus_Read(0xc00033);
	}

	static dword prevTimer = 0;

	if (prevTimer != Timer_GetTickCounter()) // 100Hz
	{
		int deltaTime = Timer_GetTickCounter() - prevTimer;

		if (kbdInited != KBD_OK) {
			kbdResetTimer += deltaTime;
			if (kbdResetTimer > 200)
				Keyboard_Reset();
		}

		if (mouseInited != MOUSE_OK) {
			mouseResetTimer += deltaTime;
			if (mouseResetTimer > 200)
				Mouse_Reset();
		}

		static byte leds_prev = 0;
		byte leds = (floppy_leds() << 2) | ((ReadKeyFlags() & fKeyPCEmu) ? 1 : 0) | 2;

		if (leds != leds_prev && kbdInited == KBD_OK) {
			byte data[2] = { 0xed, leds };
			Keyboard_Send(data, 2);
			leds_prev = leds;
		}

		static int rtcUpdateTime = 0;

		if (rtcUpdateTime <= 0) {
			RTC_Update();
			rtcUpdateTime += 100;
		}

		rtcUpdateTime -= deltaTime;

		prevTimer = Timer_GetTickCounter();
	}
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
			FPGA_Config();

		portENTER_CRITICAL();
		timer_flag_1Hz--;
		portEXIT_CRITICAL();
	}

	if (fpgaStatus == FPGA_SPECCY2010)
		SpectrumTimer_Routine();
}

void BDI_ResetWrite()
{
	SystemBus_Write(0xc00060, 0x8000);
}

void BDI_Write(byte data)
{
	SystemBus_Write(0xc00060, data);
}

void BDI_ResetRead(word counter)
{
	SystemBus_Write(0xc00061, 0x8000 | counter);
}

bool BDI_Read(byte *data)
{
	word result = SystemBus_Read(0xc00061);
	*data = (byte)result;

	return (result & 0x8000) != 0;
}

void BDI_Routine()
{
	if (fpgaStatus != FPGA_SPECCY2010)
		return;
	int ioCounter = 0x20;

	word trdosStatus = SystemBus_Read(0xc00019);

	while ((trdosStatus & 1) != 0) {
		bool trdosWr = (trdosStatus & 0x02) != 0;
		byte trdosAddr = SystemBus_Read(0xc0001a);

		static int counter = 0;

		if (trdosWr) {
			byte trdosData = SystemBus_Read(0xc0001b);
			fdc_write(trdosAddr, trdosData);

			if (LOG_BDI_PORTS) {
				if (trdosAddr == 0x7f) {
					if (counter == 0)
						__TRACE("Data write : ");

					__TRACE("%.2x.", trdosData);

					counter++;
					if (counter == 16) {
						counter = 0;
						__TRACE("\n");
					}
				}
				else //if( trdosAddr != 0xff )
				{
					if (counter != 0) {
						counter = 0;
						__TRACE("\n");
					}

					word specPc = SystemBus_Read(0xc00001);
					__TRACE("0x%.4x WR : 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData);
				}
			}
		}
		else {
			byte trdosData = fdc_read(trdosAddr);
			SystemBus_Write(0xc0001b, trdosData);

			if (LOG_BDI_PORTS) {
				if (trdosAddr == 0x7f) {
					if (counter == 0)
						__TRACE("Data read : ");

					__TRACE("%.2x.", trdosData);

					counter++;
					if (counter == 16) {
						counter = 0;
						__TRACE("\n");
					}
				}
				else if (trdosAddr != 0xff) {
					if (counter != 0) {
						counter = 0;
						__TRACE("\n");
					}

					word specPc = SystemBus_Read(0xc00001);
					__TRACE("0x%.4x RD : 0x%.2x, 0x%.2x\n", specPc, trdosAddr, trdosData);
				}
			}
		}

		SystemBus_Write(0xc0001d, fdc_read(0xff));
		SystemBus_Write(0xc00019, 0);

		fdc_dispatch();

		SystemBus_Write(0xc0001d, fdc_read(0xff));
		//SystemBus_Write( 0xc00019, 0 );

		if (--ioCounter == 0)
			break;

		trdosStatus = SystemBus_Read(0xc00019);
		if ((trdosStatus & 1) == 0)
			break;
	}

	fdc_dispatch();
	SystemBus_Write(0xc0001d, fdc_read(0xff));
}

const dword STACK_FILL = 0x37ACF177;
extern char *current_heap_end;

void InitStack()
{
	volatile dword *stackPos;
	{
		volatile dword stackStart;

		stackPos = &stackStart;
		while ((void *)stackPos >= (void *)current_heap_end) {
			*stackPos = STACK_FILL;
			stackPos--;
		}
	}
}

void TestStack()
{
	dword stackCurrent;
	dword stackMin;
	volatile dword *stackPos;

	{
		volatile dword stackStart;

		stackCurrent = (dword)&stackStart;
		stackMin = (dword)&stackStart;

		stackPos = &stackStart;
		while ((void *)stackPos >= (void *)current_heap_end) {
			if (*stackPos != STACK_FILL)
				stackMin = (dword)stackPos;
			stackPos--;
		}
	}

	__TRACE("stack current - 0x%.8x\n", stackCurrent);
	__TRACE("stack max - 0x%.8x\n", stackMin);
	__TRACE("heap max - 0x%.8x\n", (dword)current_heap_end);
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

			if (strcmp(cmd, "test stack") == 0)
				TestStack();
			else if (strcmp(cmd, "log bdi") == 0)
				LOG_BDI_PORTS = true;
			else if (strcmp(cmd, "update rom") == 0) {
				Spectrum_InitRom();
			}
			else if (strcmp(cmd, "log wait off") == 0)
				LOG_WAIT = false;
			else if (strcmp(cmd, "log wait") == 0)
				LOG_WAIT = true;
			//else if( strcmp( cmd, "test heap" ) == 0 ) TestHeap();
			else if (strcmp(cmd, "reset") == 0)
				while (true)
					;
			else if (strcmp(cmd, "pc") == 0) {
				SystemBus_TestConfiguration();
			}
			else {
				__TRACE("cmd: %s\n", cmd);
				LOG_BDI_PORTS = false;
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

void BDI_StopTimer()
{
	bdiTimerFlag = false;
}

void BDI_StartTimer()
{
	bdiTimerFlag = true;
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

	InitStack();

	__TRACE("\nSpeccy2010, v" VERSION "\n");

	if (!pllStatusOK)
		__TRACE("PLL initialization error !\n");
	DelayMs(100);

	fdc_init();
	RTC_Init();

	while (true) {
		Timer_Routine();
		Serial_Routine();

		Keyboard_Routine();
		Tape_Routine();
		BDI_Routine();
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
