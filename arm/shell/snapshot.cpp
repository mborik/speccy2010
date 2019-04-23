#include "snapshot.h"
#include "debugger.h"
#include "dialog.h"
#include "screen.h"
#include "../system.h"
#include "../specConfig.h"
#include "../utils/fifo.h"
#include "../utils/dirent.h"

#define SNA_48K_LENGTH 49179

FIL snaFile;
byte header[0x20];
int progressStep, progressTotal;
extern char dstName[PATH_SIZE], shortName[45];
//---------------------------------------------------------------------------------------
#define RegLoad(R, H, L) \
		SystemBus_Write(0xc001f0 + R, (header[H] << 8) | header[L]);

#define RegStore(R, H, L) \
		data = SystemBus_Read(0xc001f0 + R); \
		header[H] = (data >> 8); header[L] = (data & 0xff);
//---------------------------------------------------------------------------------------
void IncrementName(char *name)
{
	int p = strlen(name) - 1;

	while (p >= 0) {
		if (name[p] >= '0' && name[p] < '9') {
			name[p]++;
			while (name[++p] != 0)
				if (name[p] == '9')
					name[p] = '0';
			return;
		}
		else
			p--;
	}

	strcpy(name, "");
}

const char *UpdateSnaName()
{
	dstName[0] = '\0';

	if (specConfig.snaName[0] != 0) {
		if (specConfig.snaName[0] == '/')
			sniprintf(dstName, sizeof(dstName), "%s", specConfig.snaName + 1);
		else
			sniprintf(dstName, sizeof(dstName), "%s%s", get_current_dir(), specConfig.snaName);

		while (dstName[0] != 0 && FileExists(dstName)) {
			IncrementName(dstName);
			IncrementName(specConfig.snaName);
		}
	}

	return specConfig.snaName;
}
//---------------------------------------------------------------------------------------
void LoadPage(FIL *file, byte page)
{
	dword addr = 0x800000 | (page << 13);
	word data;
	UINT res;

	for (int i = 0; i < 0x4000; i += 2) {
		if (f_read(file, &data, 2, &res) != FR_OK)
			break;
		if (res == 0)
			break;

		SystemBus_Write(addr, data);
		addr++;

		if ((i & 0x3ff) == 0) {
			WDT_Kick();

			float progress = progressStep++;
			progress /= progressTotal;
			Shell_UpdateProgress(progress);
		}
	}
}

void SavePage(FIL *file, byte page)
{
	dword addr = 0x800000 | (page << 13);
	word data;
	UINT res;

	for (int i = 0; i < 0x4000; i += 2) {
		data = SystemBus_Read(addr);
		addr++;

		if (f_write(file, &data, 2, &res) != FR_OK)
			break;
		if (res == 0)
			break;

		if ((i & 0x3ff) == 0) {
			WDT_Kick();

			float progress = progressStep++;
			progress /= progressTotal;
			Shell_UpdateProgress(progress);
		}
	}
}
//---------------------------------------------------------------------------------------
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//---------------------------------------------------------------------------------------
bool LoadSnapshot(const char *fileName, const char *title)
{
	bool result = false;
	bool stopped = CPU_Stopped();
	if (!stopped)
		CPU_Stop();

	word specPc = 0;
	SystemBus_Write(0xc00020, 0); // use bank 0

	if (f_open(&snaFile, fileName, FA_READ) == FR_OK && snaFile.fsize >= SNA_48K_LENGTH) {
		dword addr;
		word data;
		int i;

		UINT res;
		f_lseek(&snaFile, 0);
		f_read(&snaFile, header, 0x1B, &res);

		progressStep = 0;
		progressTotal = (snaFile.fsize >> 14) * 16;

		make_short_name(shortName, 32, title);
		Shell_ProgressBar("Loading snapshot", shortName);

		SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
		SystemBus_Write(0xc00022, 0x8000); // Enable shell border

		byte specPortFe, specPort7ffd, specTrdosFlag;
		specPortFe = header[0x1A] & 0x07;

		if (snaFile.fsize == SNA_48K_LENGTH) {
			specPort7ffd = 0x30; // disable 128k
			specTrdosFlag = 0;
		}
		else {
			f_lseek(&snaFile, SNA_48K_LENGTH);
			f_read(&snaFile, header + 0x1B, 0x04, &res);

			specPc = (header[0x1C] << 8) | header[0x1B];
			specPort7ffd = header[0x1D];
			specTrdosFlag = header[0x1E];
		}

		f_lseek(&snaFile, 0x1B);

		LoadPage(&snaFile, 0x05);
		LoadPage(&snaFile, 0x02);
		LoadPage(&snaFile, specPort7ffd & 0x07);

		if (snaFile.fsize > SNA_48K_LENGTH) {
			f_lseek(&snaFile, 0xC01F);

			for (byte page = 0; page < 8; page++) {
				if (page != 0x05 && page != 0x02 && page != (specPort7ffd & 0x07))
					LoadPage(&snaFile, page);
			}
		}
		else {
			byte ROMpage = 0;
			if ((specPort7ffd & 0x10) != 0)
				ROMpage |= 0x01;
			if ((specTrdosFlag & 0x01) == 0)
				ROMpage |= 0x02;

			addr = (0x100 + ROMpage) * 0x4000;

			for (i = 0; i < 0x4000; i++) {
				data = SystemBus_Read(addr++);
				if (data == 0xC9) {
					specPc = i;
					break;
				}
			}
		}

		f_close(&snaFile);

		SystemBus_Write(0xc00016, specPortFe);
		SystemBus_Write(0xc00017, specPort7ffd);
		SystemBus_Write(0xc00018, specTrdosFlag & 1);

		RegLoad(REG_IR,  0x00, 0x14);
		RegLoad(REG_HL_, 0x02, 0x01);
		RegLoad(REG_DE_, 0x04, 0x03);
		RegLoad(REG_BC_, 0x06, 0x05);
		RegLoad(REG_AF_, 0x08, 0x07);
		RegLoad(REG_HL,  0x0A, 0x09);
		RegLoad(REG_DE,  0x0C, 0x0B);
		RegLoad(REG_BC,  0x0E, 0x0D);
		RegLoad(REG_IY,  0x10, 0x0F);
		RegLoad(REG_IX,  0x12, 0x11);
		RegLoad(REG_AF,  0x16, 0x15);
		RegLoad(REG_SP,  0x18, 0x17);

		SystemBus_Write(0xc00021, 0);
		SystemBus_Write(0xc00022, 0);

		CPU_ModifyPC(specPc, (header[0x19] & 0x03) | 0x04 | ((header[0x13] & 0x04) << 1), stopped);
		result = true;
	}

	if (!stopped)
		CPU_Start();
	return result;
}
//---------------------------------------------------------------------------------------
void SaveSnapshot(const char *name)
{
	bool stopped = CPU_Stopped();
	if (!stopped)
		CPU_Stop();

	dstName[0] = '\0';
	if (name != NULL && name[0] != '\0') {
		if (name[0] == '/')
			sniprintf(dstName, sizeof(dstName), "%s", name + 1);
		else
			sniprintf(dstName, sizeof(dstName), "%s%s", get_current_dir(), name);
	}
	else {
		sniprintf(dstName, sizeof(dstName), "%sshot0000.sna", get_current_dir());
		while (dstName[0] != '\0' && FileExists(dstName))
			IncrementName(dstName);
	}

	SystemBus_Write(0xc00020, 0); // use bank 0

	byte specPortFe = SystemBus_Read(0xc00016);
	byte specPort7ffd = SystemBus_Read(0xc00017);
	byte specTrdosFlag = SystemBus_Read(0xc00018);

	byte page = (specPort7ffd & 0x08) != 0 ? 7 : 5;

	dword src = 0x800000 | (page << 13);
	dword dst = 0x800000 | (VIDEO_PAGE << 13);

	word data;
	for (int i = 0x0000; i < 0x1b00; i += 2) {
		data = SystemBus_Read(src++);
		SystemBus_Write(dst++, data);
	}

	progressStep = 0;
	progressTotal = 8 * 16;
	make_short_name(shortName, 32, dstName);
	Shell_ProgressBar("Saving snapshot", shortName);

	page = specPort7ffd & 7;
	if (page == 2 || page == 5)
		progressTotal += 16;

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE); // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | (specPortFe & 0x07)); // Enable shell border

	if (f_open(&snaFile, dstName, FA_CREATE_ALWAYS | FA_READ | FA_WRITE) == FR_OK) {
		UINT res;

		RegStore(REG_IR,  0x00, 0x14);
		RegStore(REG_HL_, 0x02, 0x01);
		RegStore(REG_DE_, 0x04, 0x03);
		RegStore(REG_BC_, 0x06, 0x05);
		RegStore(REG_AF_, 0x08, 0x07);
		RegStore(REG_HL,  0x0A, 0x09);
		RegStore(REG_DE,  0x0C, 0x0B);
		RegStore(REG_BC,  0x0E, 0x0D);
		RegStore(REG_IY,  0x10, 0x0F);
		RegStore(REG_IX,  0x12, 0x11);
		RegStore(REG_AF,  0x16, 0x15);
		RegStore(REG_SP,  0x18, 0x17);
		RegStore(REG_PC,  0x1C, 0x1B);

		data = SystemBus_Read(0xc001f0 + REG_IM);
		header[0x13] = (data & 0x08) >> 1;
		header[0x19] = (data & 0x03);
		header[0x1A] = specPortFe & 0x07;
		header[0x1D] = specPort7ffd & 0x1F;
		header[0x1E] = specTrdosFlag & 1;

		f_write(&snaFile, header, 0x1B, &res);

		SavePage(&snaFile, 0x05);
		SavePage(&snaFile, 0x02);
		SavePage(&snaFile, specPort7ffd & 0x07);

		f_write(&snaFile, header + 0x1B, 4, &res);

		for (page = 0; page < 8; page++) {
			if (page != 0x05 && page != 0x02 && page != (specPort7ffd & 0x07))
				SavePage(&snaFile, page);
		}

		f_close(&snaFile);

		SystemBus_Write(0xc00021, 0);
		SystemBus_Write(0xc00022, 0);
	}

	if (!stopped)
		CPU_Start();
}
//---------------------------------------------------------------------------------------
void SaveSnapshotAs()
{
	CPU_Stop();
	SystemBus_Write(0xc00020, 0); // use bank 0

	byte specPortFe = SystemBus_Read(0xc00016);
	byte specPort7ffd = SystemBus_Read(0xc00017);

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
