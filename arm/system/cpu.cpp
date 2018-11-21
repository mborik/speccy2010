#include "cpu.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
#include "../betadisk/fdc.h"

int cpuStopNesting = 0;

//---------------------------------------------------------------------------------------
void CPU_Start()
{
	if (cpuStopNesting > 0) {
		if (cpuStopNesting == 1) {
			ResetKeyboard();
			SystemBus_Write(0xc00000, 0x0000);
			SystemBus_Write(0xc00008, 0x0001);
			DiskIF_StartTimer();
		}

		cpuStopNesting--;
	}
}
//---------------------------------------------------------------------------------------
void CPU_Stop()
{
	if (cpuStopNesting == 0) {
		SystemBus_Write(0xc00000, 0x0001);
		DiskIF_StopTimer();

		while (true) {
			DiskIF_Routine();
			if ((SystemBus_Read(0xc00000) & 0x0001) != 0)
				break;
		}

		//SystemBus_Write( 0xc00000, 0x0000 );
	}

	cpuStopNesting++;
}
//---------------------------------------------------------------------------------------
bool CPU_Stopped()
{
	return cpuStopNesting > 0;
}
//---------------------------------------------------------------------------------------
void CPU_NMI()
{
	SystemBus_Write(0xc00000, 0x0010);
	DelayMs(100);
	SystemBus_Write(0xc00000, 0x0000);
}
//---------------------------------------------------------------------------------------
void CPU_Reset(bool res)
{
	if (res == false) {
		SystemBus_Write(0xC00018, specConfig.specTrdosFlag & 1); //specTrdosFlag

		if (specConfig.specMachine == SpecRom_Classic48) {
			SystemBus_Write(0xC00017, 0x30); //specPort7ffd
		}
		else {
			SystemBus_Write(0xC00017, 0x00); //specPort7ffd
			SystemBus_Write(0xC00023, 0x00); //specPort1ffd
		}
	}

	if (!CPU_Stopped()) {
		SystemBus_Write(0xc00000, res ? 0x0008 : 0);
	}

	fdc_reset();
}
//---------------------------------------------------------------------------------------
void CPU_Reset_Seq()
{
	CPU_Reset(true);
	DelayMs(10);
	CPU_Reset(false);
	DelayMs(100);
}
//---------------------------------------------------------------------------------------
void CPU_ModifyPC(word pc, byte istate)
{
	CPU_Stop();

	SystemBus_Write(0xc001f4, pc);
	DelayUs(1);
	SystemBus_Write(0xc001fd, istate);

	DelayUs(1);
	SystemBus_Write(0xc00000, 0x0001);

	CPU_Start();
	SystemBus_Write(0xc001ff, 0);
}
//---------------------------------------------------------------------------------------
