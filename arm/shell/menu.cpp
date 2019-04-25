#include "menu.h"
#include "screen.h"
#include "dialog.h"
#include "../system.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
#include "../specRtc.h"

//---------------------------------------------------------------------------------------
CMenuItem mainMenu[] = {
	CMenuItem(  8,  2, "Date:", "0000"),
	CMenuItem( 65,  2, "-", "00"),
	CMenuItem( 88,  2, "-", "00"),

	CMenuItem(  8,  3, "Time:", "00"),
	CMenuItem( 56,  3, ":", "00"),
	CMenuItem( 80,  3, ":", "00"),

	CMenuItem(  8,  5, "Machine :", GetParam(iniParameters, "Machine", PGRP_GENERAL)),
	CMenuItem(  8,  6, "Timings :", GetParam(iniParameters, "Timings", PGRP_GENERAL)),
	CMenuItem(  8,  7, "Disk IF :", GetParam(iniParameters, "Disk Interface", PGRP_GENERAL)),
	CMenuItem(128,  7, "Betadisk speed:", GetParam(iniParameters, "BDI mode", PGRP_TRDOS)),
	CMenuItem(  8,  8, "Turbo   :", GetParam(iniParameters, "Turbo", PGRP_GENERAL)),
	CMenuItem(  8,  9, "AY Chip :", GetParam(iniParameters, "AY Chip", PGRP_GENERAL)),
	CMenuItem(104,  9, "Covox:", GetParam(iniParameters, "Covox", PGRP_GENERAL)),
	CMenuItem(  8, 10, "AY mode :", GetParam(iniParameters, "AY mode", PGRP_GENERAL)),
	CMenuItem(104, 10, "Turbo Sound: ", GetParam(iniParameters, "Turbo Sound", PGRP_GENERAL)),
	CMenuItem(  8, 11, "Audio DAC mode:", GetParam(iniParameters, "Audio DAC mode", PGRP_GENERAL)),

	CMenuItem(  8, 12, "Joystick emulation:", GetParam(iniParameters, "Joystick emulation", PGRP_GENERAL)),
	CMenuItem(  8, 13, "Joystick 1:", GetParam(iniParameters, "Joystick 1", PGRP_GENERAL)),
	CMenuItem(  8, 14, "Joystick 2:", GetParam(iniParameters, "Joystick 2", PGRP_GENERAL)),
	CMenuItem(  8, 15, "Mouse sensitivity :", GetParam(iniParameters, "Mouse Sensitivity", PGRP_GENERAL)),
	CMenuItem(  8, 16, "Swap mouse buttons:", GetParam(iniParameters, "Swap mouse buttons", PGRP_GENERAL)),

	CMenuItem(  8, 17, "Video mode:", GetParam(iniParameters, "Video mode", PGRP_GENERAL)),
	CMenuItem(  8, 18, "Video aspect ratio:", GetParam(iniParameters, "Video aspect ratio", PGRP_GENERAL)),
	CMenuItem(  8, 19, "TV Interlace emulation:", GetParam(iniParameters, "Video interlace", PGRP_GENERAL)),

	CMenuItem(190, 19, "Font:", GetParam(iniParameters, "Font", PGRP_GENERAL))
};
//---------------------------------------------------------------------------------------
CMenuItem trdosDisksMenu[] = {
	CMenuItem(8,  3, "A: ", GetParam(iniParameters, "Disk A", PGRP_TRDOS)),
	CMenuItem(8,  4, "A  Write protect: ", GetParam(iniParameters, "Disk A WP", PGRP_TRDOS)),
	CMenuItem(8,  6, "B: ", GetParam(iniParameters, "Disk B", PGRP_TRDOS)),
	CMenuItem(8,  7, "B  Write protect: ", GetParam(iniParameters, "Disk B WP", PGRP_TRDOS)),
	CMenuItem(8,  9, "C: ", GetParam(iniParameters, "Disk C", PGRP_TRDOS)),
	CMenuItem(8, 10, "C  Write protect: ", GetParam(iniParameters, "Disk C WP", PGRP_TRDOS)),
	CMenuItem(8, 12, "D: ", GetParam(iniParameters, "Disk D", PGRP_TRDOS)),
	CMenuItem(8, 13, "D  Write protect: ", GetParam(iniParameters, "Disk D WP", PGRP_TRDOS))
};
//---------------------------------------------------------------------------------------
CMenuItem mb02DisksMenu[] = {
	CMenuItem(8,  3, "A: ", GetParam(iniParameters, "Disk A", PGRP_MB02)),
	CMenuItem(8,  4, "A  Write protect: ", GetParam(iniParameters, "Disk A WP", PGRP_MB02)),
	CMenuItem(8,  6, "B: ", GetParam(iniParameters, "Disk B", PGRP_MB02)),
	CMenuItem(8,  7, "B  Write protect: ", GetParam(iniParameters, "Disk B WP", PGRP_MB02)),
	CMenuItem(8,  9, "C: ", GetParam(iniParameters, "Disk C", PGRP_MB02)),
	CMenuItem(8, 10, "C  Write protect: ", GetParam(iniParameters, "Disk C WP", PGRP_MB02)),
	CMenuItem(8, 12, "D: ", GetParam(iniParameters, "Disk D", PGRP_MB02)),
	CMenuItem(8, 13, "D  Write protect: ", GetParam(iniParameters, "Disk D WP", PGRP_MB02))
};
//---------------------------------------------------------------------------------------
CMenuItem romCfgMenu[] = {
	CMenuItem(8,  3, "ZX 48    :", GetParam(iniParameters, "48", PGRP_ROMS)),
	CMenuItem(8,  4, "ZX 128   :", GetParam(iniParameters, "128", PGRP_ROMS)),
	CMenuItem(8,  5, "Pentagon :", GetParam(iniParameters, "Pentagon", PGRP_ROMS)),
	CMenuItem(8,  6, "Scorpion :", GetParam(iniParameters, "Scorpion", PGRP_ROMS)),
	CMenuItem(8,  8, "Gluk/EVO Reset Service\n>", GetParam(iniParameters, "EVO Reset Service", PGRP_ROMS)),
	CMenuItem(8, 11, "TR-DOS Firmware  (16k)\n>", GetParam(iniParameters, "TR-DOS", PGRP_ROMS)),
	CMenuItem(8, 13, "DivMMC Firmware   (8k)\n>", GetParam(iniParameters, "DivMMC Firmware", PGRP_ROMS)),
	CMenuItem(8, 15, "MB-02 Firmware   (32k)\n>", GetParam(iniParameters, "BS-DOS", PGRP_ROMS))
};
//---------------------------------------------------------------------------------------
void Shell_SettingsMenu()
{
	Shell_Menu("Settings", mainMenu, sizeof(mainMenu) / sizeof(CMenuItem));
}
//---------------------------------------------------------------------------------------
void Shell_DisksMenu()
{
	CMenuItem *disksMenu;

	if (specConfig.specDiskIf == SpecDiskIf_Betadisk)
		disksMenu = trdosDisksMenu;
	else if (specConfig.specDiskIf == SpecDiskIf_MB02)
		disksMenu = mb02DisksMenu;
	else return;

	Shell_Menu("Disk Mount", disksMenu, sizeof(trdosDisksMenu) / sizeof(CMenuItem));
	Spectrum_UpdateDisks();
}
//---------------------------------------------------------------------------------------
void Shell_RomCfgMenu()
{
	Shell_Menu("ROM Config", romCfgMenu, sizeof(romCfgMenu) / sizeof(CMenuItem));
}
//---------------------------------------------------------------------------------------
//=======================================================================================
//---------------------------------------------------------------------------------------
void InitScreen(const char *title)
{
	byte attr = 0007;
	ClrScr(attr);

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE);           // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | ((attr >> 3) & 0x03)); // Enable shell border

	static char str[43];
	sniprintf(str, sizeof(str), "Speccy2010 v" VERSION " \7 %s Dialog", title);

	size_t len = strlen(str);
	DrawStrAttr((256 - (len * 6)) / 2, 0, str, 0105, len);

	DrawAttr8(0, 1, 0002, 32);
	DrawLine(1, 2);
	DrawLine(1, 4);

	DrawAttr8(0, 20, 0002, 32);
	DrawLine(20, 3);
	DrawLine(20, 5);
}
//---------------------------------------------------------------------------------------
void Shell_Menu(const char *title, CMenuItem *menu, int menuSize)
{
	static tm time;
	static dword tickCounter = 0;

	CPU_Stop();

	InitScreen(title);

	for (int i = 0; i < menuSize; i++) {
		menu[i].UpdateData(true);
		menu[i].UpdateState(0);
	}

	bool editMode = false;
	bool hardReset = false;
	int mmLastMachineCfg = -1;

	int menuPos = 0;
	if (menu == mainMenu) {
		menuPos = 6;
		mmLastMachineCfg = specConfig.specMachine | (specConfig.specDiskIf << 8);
	}

	menu[menuPos].UpdateState(1);

	while (true) {
		byte key = GetKey(false);

		if (!editMode) {
			if (key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT) {
				menu[menuPos].UpdateState(0);

				if (key == K_LEFT || key == K_UP)
					menuPos = (menuPos - 1 + menuSize) % menuSize;
				else
					menuPos = (menuPos + 1) % menuSize;

				menu[menuPos].UpdateState(1);
			}
		}
		else {
			if (key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT) {
				int delta;
				if (key == K_LEFT || key == K_DOWN)
					delta = -1;
				else
					delta = 1;

				if (menu == mainMenu && menuPos >= 0 && menuPos < 6) {
					if (menuPos == 0)
						time.tm_year += delta;
					if (menuPos == 1)
						time.tm_mon += delta;
					if (menuPos == 2)
						time.tm_mday += delta;
					if (menuPos == 3)
						time.tm_hour += delta;
					if (menuPos == 4)
						time.tm_min += delta;
					if (menuPos == 5)
						time.tm_sec += delta;

					RTC_SetTime(&time);
					tickCounter = 0;
				}
				else {
					const CParameter *param = menu[menuPos].GetParam();
					if (param != NULL) {
						if (param->GetType() == PTYPE_INT) {
							int value = param->GetValue() + delta;
							if (value < param->GetValueMin())
								value = param->GetValueMin();
							if (value > param->GetValueMax())
								value = param->GetValueMax();
							param->SetValue(value);

							menu[menuPos].UpdateData();
						}
					}
				}
			}
		}
		if (key == K_RETURN) {
			if (menu == mainMenu && menuPos >= 0 && menuPos < 6) {
				editMode = !editMode;

				if (editMode)
					menu[menuPos].UpdateState(2);
				else
					menu[menuPos].UpdateState(1);
			}
			else {
				const CParameter *param = menu[menuPos].GetParam();
				if (param != NULL) {
					if (param->GetType() == PTYPE_LIST) {
						param->SetValue((param->GetValue() + 1) % (param->GetValueMax() + 1));

						if (param == GetParam(iniParameters, "Font", PGRP_GENERAL)) {
							InitScreen(title);
							for (int i = 0; i < menuSize; i++)
								menu[i].UpdateData(true);
						}
						else {
							const CParameter *machine = GetParam(iniParameters, "Machine", PGRP_GENERAL);
							if (param == machine && machine->GetValue() >= SpecRom_Pentagon128) {
								menu[8].GetParam()->SetValue(SpecDiskIf_Betadisk);
								menu[8].UpdateData();
							}
							else if (param == GetParam(iniParameters, "Disk Interface", PGRP_GENERAL)) {
								if (machine->GetValue() >= SpecRom_Pentagon128)
									param->SetValue(SpecDiskIf_Betadisk);
							}
						}

						menu[menuPos].UpdateData();
					}
					else if (param->GetType() == PTYPE_INT) {
						editMode = !editMode;

						if (editMode)
							menu[menuPos].UpdateState(2);
						else
							menu[menuPos].UpdateState(1);
					}
					else if (param->GetType() == PTYPE_STRING) {
						menu[menuPos].UpdateState(2);

						bool inROMCfg = (menu == romCfgMenu);

						CString value = "";
						param->GetValueText(value);
						if (Shell_InputBox(title, "Filename:", value)) {
							if (!FileExists(value.String()))
								Shell_MessageBox(title, "Invalid path or", "file not exists");

							param->SetValueText(value);
							menu[menuPos].UpdateData();

							if (inROMCfg)
								hardReset = true;
						}

						menu[menuPos].UpdateState(1);

						InitScreen(title);
						for (int i = 0; i < menuSize; i++)
							menu[i].UpdateData();
					}
				}
			}
		}
		else if (key == K_BACKSPACE) {
			const CParameter *param = menu[menuPos].GetParam();
			if (param != NULL) {
				if (param->GetType() == PTYPE_STRING) {
					param->SetValueText("");
					menu[menuPos].UpdateState(0);
					menu[menuPos].UpdateData();
					menu[menuPos].UpdateState(1);
				}
			}
		}
		else if (menu == mainMenu && key == K_F5) {
			hardReset = true;
			break;
		}
		else if (key == K_ESC)
			break;

		// main menu RTC auto-update...
		if (menu == mainMenu && (dword)(Timer_GetTickCounter() - tickCounter) >= 100) {
			RTC_GetTime(&time);
			char temp[5];

			sniprintf(temp, sizeof(temp), "%.2d", time.tm_sec);
			mainMenu[5].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.2d", time.tm_min);
			mainMenu[4].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.2d", time.tm_hour);
			mainMenu[3].UpdateData(temp);

			sniprintf(temp, sizeof(temp), "%.2d", time.tm_mday);
			mainMenu[2].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.2d", time.tm_mon + 1);
			mainMenu[1].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.4d", time.tm_year + 1900);
			mainMenu[0].UpdateData(temp);

			tickCounter = Timer_GetTickCounter();
		}
	}

	int mmNewMachineCfg = specConfig.specMachine | (specConfig.specDiskIf << 8);
	if (menu == mainMenu) {
		if (mmLastMachineCfg != mmNewMachineCfg)
			hardReset = true;
	}

	SaveConfig();

	SystemBus_Write(0xc00021, 0); // Enable Video
	SystemBus_Write(0xc00022, 0);

	Spectrum_UpdateConfig(hardReset);
	CPU_Start();
}
//---------------------------------------------------------------------------------------
