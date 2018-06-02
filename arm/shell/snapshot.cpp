#include "snapshot.h"
#include "dialog.h"
#include "screen.h"
#include "../system.h"
#include "../specConfig.h"
#include "../specSnapshot.h"

//---------------------------------------------------------------------------------------
void Shell_SaveSnapshot()
{
	CPU_Stop();
	SystemBus_Write(0xc00020, 0); // use bank 0

	byte specPortFe = SystemBus_Read(0xc00016);
	byte specPort7ffd = SystemBus_Read(0xc00017);
	/*TODO scorpion*/

	byte page = (specPort7ffd & (1 << 3)) != 0 ? 7 : 5;

	dword src = 0x800000 | (page << 13);
	dword dst = 0x800000 | (VIDEO_PAGE << 13);

	word data;

	for (int i = 0x0000; i < 0x1b00; i += 2) {
		data = SystemBus_Read(src++);
		SystemBus_Write(dst++, data);
	}

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | (specPortFe & 0x07)); // Enable shell border

	CString name = UpdateSnaName();
	bool result = Shell_InputBox("Save snapshot", "Enter name :", name);

	SystemBus_Write(0xc00021, 0); //armVideoPage
	SystemBus_Write(0xc00022, 0); //armBorder

	if (result) {
		sniprintf(specConfig.snaName, sizeof(specConfig.snaName), "%s", name.String());
		SaveSnapshot(specConfig.snaName);
	}

	CPU_Start();
}
//---------------------------------------------------------------------------------------

