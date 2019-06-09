#include <malloc.h>
#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "system.h"
#include "betadisk/fdc.h"
#include "betadisk/floppy.h"
#include "specConfig.h"
#include "specKeyboard.h"
#include "specMB02.h"
#include "specRtc.h"
#include "specTape.h"
#include "shell/screen.h"
#include "shell/debugger.h"


dword tickCounter = 0;

word lastCpuConfig = 0;
int diskIfWasActive = 0;

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
			FRESULT result = f_mount(fatfs, "", 1);

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
void Spectrum_CleanupSDRAM()
{
	__TRACE("Cleanup of SD-RAM...\n");

	SystemBus_Write(0xc00020, 0); // use bank 0

	// 512kB of DiskIF SRAM pages  (32 x #4000)
	// 128kB of ZX-Spectrum RAM     (8 x #4000)
	dword addr, bases[2] = { 0x840000, 0x800000 };

	for (int i, j = 0, amount = 0x80000; j < 2; j++) {
		for (addr = bases[j], i = 0; i < amount; i += 2) {
			SystemBus_Write(addr, 0);

			addr++;
			if ((addr & 0x0fff) == 0)
				WDT_Kick();
			if ((addr & 0x3fff) == 0)
				__TRACE(".");
		}

		amount >>= 2;
	}

	__TRACE("\nCleanup done...\n");
}

bool Spectrum_LoadRomPage(int page, const char *romFileName, const char *fallbackRomFile = NULL)
{
	if (!(romFileName != NULL && strlen(romFileName) > 0))
		return false;

	const char *romImageFileName = romFileName;

	if (f_open(&mem.fsrc, romFileName, FA_READ) != FR_OK) {
		__TRACE("Cannot open rom image '%s'\n", romFileName);

		if (fallbackRomFile == NULL)
			return false;

		romImageFileName = fallbackRomFile;
		if (f_open(&mem.fsrc, fallbackRomFile, FA_READ) != FR_OK) {
			__TRACE("Cannot open rom image '%s'\n", fallbackRomFile);
			return false;
		}
	}

	SystemBus_Write(0xc00020, 0); // use bank 0
	f_lseek(&mem.fsrc, 0);

	bool result = true;
	int writeErrors = 0;
	int readErrors = 0;
	dword pos, addr = 0x800000 | (((0x100 + page) * 0x4000) >> 1);
	word data, temp;
	UINT res;

	__TRACE("Loading '%s' into page %d (0x%x)\n", romImageFileName, page, addr << 1);

	for (pos = 0; pos < mem.fsrc.fsize; pos += 2) {
		if (f_read(&mem.fsrc, &data, 2, &res) != FR_OK)
			break;
		if (res == 0)
			break;

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

	f_close(&mem.fsrc);
	return result;
}

void Spectrum_InitRom()
{
	__TRACE("ROM configuration started...\n");

	specConfig.specTrdosFlag = 0;

	if (specConfig.specMachine == SpecRom_Classic48) {
		Spectrum_LoadRomPage(1, specConfig.specRomFile_Classic48, "roms/48.rom");
	}
	else if (specConfig.specMachine == SpecRom_Classic128) {
		Spectrum_LoadRomPage(0, specConfig.specRomFile_Classic128, "roms/128.rom");
	}
	else if (specConfig.specMachine == SpecRom_Scorpion) {
		Spectrum_LoadRomPage(0, specConfig.specRomFile_Scorpion, "roms/scorpion.rom");
	}
	else {
		if (specConfig.specDiskIf == SpecDiskIf_Betadisk)
			specConfig.specTrdosFlag = Spectrum_LoadRomPage(2, specConfig.specRomFile_TRD_Service);

		Spectrum_LoadRomPage(0, specConfig.specRomFile_Pentagon, "roms/pentagon.rom");
	}

	if (specConfig.specDiskIf == SpecDiskIf_Betadisk) {
		Spectrum_LoadRomPage(3, specConfig.specRomFile_TRD_ROM, "roms/trdos.rom");
	}
	else if (specConfig.specDiskIf == SpecDiskIf_DivMMC) {
		// DivMMC banks [00-3F] are mapped in RAM section of memspace [0x080000-0x100000]
		// and DivMMC firmware is mapped on 3rd place of ROM memspace [0x40C000-0x40DFFF]
		if (!Spectrum_LoadRomPage(3, specConfig.specRomFile_DivMMC_FW))
			__TRACE("DivMMC firmware missing!\n");
	}
	else if (specConfig.specDiskIf == SpecDiskIf_MB02) {
		// MB-02 banks [00-1F] are mapped in RAM section of memspace [0x080000-0x100000]
		// BS-ROM (16k) and BS-DOS (16k) loaded at first 2 SRAM banks...
		if (!Spectrum_LoadRomPage(-0xE0, specConfig.specRomFile_BSROM_BSDOS))
			__TRACE("BS-ROM + BS-DOS for MB-02 missing!\n");

		// and MB-02 fixed EPROM mapped on 3rd place of ROM memspace [0x40C000-0x40C7FF]
		mb02_fill_eprom();
	}

	__TRACE("ROM configuration finished...\n");
}

void Spectrum_UpdateConfig(bool hardReset)
{
	word fpgaRomRamCfg, fpgaDiskIfCfg = 0;

	switch (specConfig.specMachine) {
		case SpecRom_Classic48:
			fpgaRomRamCfg = 0;
			break;

		default:
		case SpecRom_Classic128:
		case SpecRom_Pentagon128:
			fpgaRomRamCfg = 1;
			break;

		case SpecRom_Pentagon1024:
			fpgaRomRamCfg = 2;
			break;

		case SpecRom_Scorpion:
			fpgaRomRamCfg = 3;
			break;
	}

	switch (specConfig.specDiskIf) {
		case SpecDiskIf_Betadisk:
			fpgaDiskIfCfg = 1;
			break;
		case SpecDiskIf_DivMMC:
			fpgaDiskIfCfg = 2;
			break;
		case SpecDiskIf_MB02:
			fpgaDiskIfCfg = 4;
			break;
	}

	// machine, RAM, ROM and Turbo configuration...
	lastCpuConfig = fpgaRomRamCfg |
		((specConfig.specSync << 3) & 0x38) | ((specConfig.specTurbo << 6) & 0xC0);
	SystemBus_Write(0xc00040, lastCpuConfig);

	// video mode and aspect ratio configuration...
	SystemBus_Write(0xc00041, specConfig.specVideoMode  |
		((specConfig.specVideoAspectRatio << 4) & 0x70) |
		(specConfig.specVideoInterlace << 7));

	// disk interface configuration...
	SystemBus_Write(0xc00042, fpgaDiskIfCfg | (hardReset ? 0x8000 : 0));
	diskIfWasActive = 0;

	// audio configuration (AY, YM, TurboSound)...
	SystemBus_Write(0xc00045, specConfig.specTurboSound |
		((specConfig.specAyMode << 1) & 0x0E) | (specConfig.specAyYm << 4));

	// Covox and D/A converter configuration...
	SystemBus_Write(0xc00046, specConfig.specCovox | (specConfig.specDacMode << 3));

	// ROMs initialization...
	dword newMachineConfig = specConfig.specMachine | (fpgaDiskIfCfg << 4);
	if (prevMachineConfig != newMachineConfig || hardReset) {
		prevMachineConfig = newMachineConfig;

		if (hardReset) {
			SystemBus_Write(0xc00000, 8);
			CPU_Stop();

			SystemBus_Write(0xc00020, 0);
			ResetScreen(true);

			Spectrum_CleanupSDRAM();
		}
		else
			CPU_Stop();

		Spectrum_InitRom();
		Spectrum_UpdateDisks();

		ResetKeyboard();
		RTC_Update();

		CPU_Start();
		CPU_Reset_Seq();

		WDT_Kick();
		timer_flag_1Hz = 0;
		timer_flag_100Hz = 0;
	}

	if (hardReset)
		ResetScreen(false);
}

void Spectrum_UpdateDisks()
{
	if (specConfig.specDiskIf == SpecDiskIf_Betadisk) {
		floppy_set_fast_mode(specConfig.specBdiMode);

		for (int i = 0; i < 4; i++) {
			fdc_open_image(i, specConfig.specBdiImages[i].name);
			floppy_disk_wp(i, &specConfig.specBdiImages[i].writeProtect);
		}
	}
	else if (specConfig.specDiskIf == SpecDiskIf_MB02) {
		for (int i = 0; i < 4; i++) {
			if (mb02_open_image(i, specConfig.specMB2Images[i].name) > 0)
				mb02_close_image(i);
			else
				mb02_set_disk_wp(i, specConfig.specMB2Images[i].writeProtect);
		}
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
		bool activity = Tape_Started();

		if (specConfig.specDiskIf == SpecDiskIf_Betadisk)
			activity ^= floppy_leds();
		else
			activity ^= (diskIfWasActive > 0);

		byte leds = ((ReadKeyFlags() & fKeyPCEmu) ? 1 : 0) | 2 | (activity ? 4 : 0);

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

	SpectrumTimer_Routine();
}

void DiskIF_Routine()
{
	if (specConfig.specDiskIf == SpecDiskIf_Betadisk)
		BDI_Routine();
	else if (specConfig.specDiskIf == SpecDiskIf_DivMMC)
		DivMMC_Routine();
	else if (specConfig.specDiskIf == SpecDiskIf_MB02)
		MB02_Routine();
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
	int ioCounter = 0x20;

	word trdosStatus = SystemBus_Read(0xc00019);

	while ((trdosStatus & 1) != 0) {
		bool trdosWr = (trdosStatus & 0x02) != 0;
		byte trdosAddr = SystemBus_Read(0xc0001a);

		if (trdosWr) {
			byte trdosData = SystemBus_Read(0xc0001b);
			fdc_write(trdosAddr, trdosData);
		}
		else {
			byte trdosData = fdc_read(trdosAddr);
			SystemBus_Write(0xc0001b, trdosData);
		}

		SystemBus_Write(0xc0001d, fdc_read(0xff));
		SystemBus_Write(0xc00019, 0);

		fdc_dispatch();

		SystemBus_Write(0xc0001d, fdc_read(0xff));
		//SystemBus_Write( 0xc00019, 0 );

		if (--ioCounter == 0)
			break;

		trdosStatus = SystemBus_Read(0xc00019);
	}

	fdc_dispatch();
	SystemBus_Write(0xc0001d, fdc_read(0xff));
}

void DivMMC_Routine()
{
	int ioCounter = 0x40;

	word divmmcStatus = SystemBus_Read(0xc00019);
	bool isActiveNow = (divmmcStatus & 1) != 0;

	if (isActiveNow && diskIfWasActive == 0) {
		SystemBus_Write(0xc00040, (lastCpuConfig & 0x3F) | 0x80); // near full speed
		GPIO_WriteBit(GPIO1, GPIO_Pin_10, Bit_RESET);
	}

	while ((divmmcStatus & 1) != 0) {
		bool divmmcWr = (divmmcStatus & 0x02) != 0;

		if (divmmcWr) {
			byte data = SystemBus_Read(0xc0001b);
			xmit_spi(data);
		}
		else {
			byte data = rcvr_spi();
			SystemBus_Write(0xc0001b, data);
		}

		SystemBus_Write(0xc00019, 0);

		if (--ioCounter == 0)
			break;

		WDT_Kick();
		divmmcStatus = SystemBus_Read(0xc00019);
	}

	if (isActiveNow) {
		if (diskIfWasActive == 0)
			diskIfWasActive = 1023;
		else if (diskIfWasActive < 1024)
			diskIfWasActive++;
	}
	else {
		if (diskIfWasActive > 0)
			diskIfWasActive--;
		else
			SystemBus_Write(0xc00040, lastCpuConfig);
	}
}

void MB02_Routine()
{
	int ioCounter = 0x40;

	word mb02Status = SystemBus_Read(0xc00019);
	bool isActiveNow = (mb02Status & 1) != 0;

	if (isActiveNow && diskIfWasActive == 0)
		SystemBus_Write(0xc00040, (lastCpuConfig & 0x3F) | 0x80); // near full speed

	while ((mb02Status & 1) != 0) {
		bool divmmcWr = (mb02Status & 0x02) != 0;

		if (divmmcWr)
			mb02_received(SystemBus_Read(0xc0001b));
		else
			SystemBus_Write(0xc0001b, mb02_transmit());

		SystemBus_Write(0xc00019, 0);

		if (--ioCounter == 0)
			break;

		DelayUs(1);
		mb02Status = SystemBus_Read(0xc00019);
	}

	if (isActiveNow) {
		if (diskIfWasActive == 0)
			diskIfWasActive = 1023;
		else if (diskIfWasActive < 1024)
			diskIfWasActive++;
	}
	else {
		if (diskIfWasActive > 0)
			diskIfWasActive--;
		else
			SystemBus_Write(0xc00040, lastCpuConfig);
	}
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


volatile dword delayTimer = 0;
volatile dword tapeTimer = 0;

volatile bool diskIfTimerFlag = true;
volatile dword diskIfTimer = 0;

void TIM0_UP_IRQHandler() //40kHz
{
	static dword tick = 0;

	if (delayTimer >= 25)
		delayTimer -= 25;
	else
		delayTimer = 0;

	if (diskIfTimerFlag)
		diskIfTimer += 25;

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
	return diskIfTimer;
}

void DiskIF_StopTimer()
{
	diskIfTimerFlag = false;
}

void DiskIF_StartTimer()
{
	diskIfTimerFlag = true;
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

	__TRACE("\n%s\n", "Speccy2010 v" VERSION);

	if (!pllStatusOK)
		__TRACE("PLL initialization error !\n");
	DelayMs(100);

	fdc_init();
	mb02_init();
	RTC_Init();
	Debugger_Init();

	while (true) {
		Timer_Routine();
		Serial_Routine();

		Debugger_HandleBreakpoint();
		Keyboard_Routine();
		Tape_Routine();
		DiskIF_Routine();
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
