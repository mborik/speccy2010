#include "menu.h"
#include "screen.h"
#include "../system.h"
#include "../specConfig.h"
#include "../specKeyboard.h"
#include "../specRtc.h"

//---------------------------------------------------------------------------------------
CMenuItem mainMenu[] = {
	CMenuItem(1, 2, "Date: ", "00"),
	CMenuItem(9, 2, ".", "00"),
	CMenuItem(12, 2, ".", "00"),

	CMenuItem(1, 3, "Time: ", "00"),
	CMenuItem(9, 3, ":", "00"),
	CMenuItem(12, 3, ":", "0000"),

	CMenuItem(1, 5, "ROM/RAM: ", GetParam(iniParameters, "ROM/RAM")),
	CMenuItem(1, 6, "Timings: ", GetParam(iniParameters, "Timings")),
	CMenuItem(1, 7, "Turbo: ", GetParam(iniParameters, "Turbo")),
	CMenuItem(1, 8, "AY mode: ", GetParam(iniParameters, "AY mode")),
	CMenuItem(14, 8, "Covox:", GetParam(iniParameters, "Covox")),
	CMenuItem(1, 9, "AY Chip:", GetParam(iniParameters, "AY Chip")),
	CMenuItem(14, 9, "Turbo Sound: ", GetParam(iniParameters, "Turbo Sound")),
	CMenuItem(1, 10, "BDI mode: ", GetParam(iniParameters, "BDI mode")),

	CMenuItem(1, 11, "Joystick emulation: ", GetParam(iniParameters, "Joystick emulation")),
	CMenuItem(1, 12, "Joystick 1: ", GetParam(iniParameters, "Joystick 1")),
	CMenuItem(1, 13, "Joystick 2: ", GetParam(iniParameters, "Joystick 2")),
	CMenuItem(1, 14, "Mouse Sensitivity: ", GetParam(iniParameters, "Mouse Sensitivity")),
	CMenuItem(1, 15, "Swap mouse buttons: ", GetParam(iniParameters, "Swap mouse buttons")),

	CMenuItem(1, 16, "Video mode: ", GetParam(iniParameters, "Video mode")),
	CMenuItem(1, 17, "Video aspect ratio: ", GetParam(iniParameters, "Video aspect ratio")),
	CMenuItem(1, 18, "Audio DAC mode: ", GetParam(iniParameters, "Audio DAC mode")),
	CMenuItem(1, 19, "Font: ", GetParam(iniParameters, "Font")),
};
//---------------------------------------------------------------------------------------
CMenuItem disksMenu[] = {
	CMenuItem(1, 3, "A: ", GetParam(iniParameters, "Disk A")),
	CMenuItem(3, 4, "read only :", GetParam(iniParameters, "Disk A read only")),
	CMenuItem(1, 6, "B: ", GetParam(iniParameters, "Disk B")),
	CMenuItem(3, 7, "read only :", GetParam(iniParameters, "Disk B read only")),
	CMenuItem(1, 9, "C: ", GetParam(iniParameters, "Disk C")),
	CMenuItem(3, 10, "read only :", GetParam(iniParameters, "Disk C read only")),
	CMenuItem(1, 12, "D: ", GetParam(iniParameters, "Disk D")),
	CMenuItem(3, 13, "read only :", GetParam(iniParameters, "Disk D read only")),
};
//---------------------------------------------------------------------------------------
void Shell_SettingsMenu()
{
	Shell_Menu(mainMenu, sizeof(mainMenu) / sizeof(CMenuItem));
}
//---------------------------------------------------------------------------------------
void Shell_DisksMenu()
{
	Shell_Menu(disksMenu, sizeof(disksMenu) / sizeof(CMenuItem));
	Spectrum_UpdateDisks();
}
//---------------------------------------------------------------------------------------
//=======================================================================================
//---------------------------------------------------------------------------------------
void InitScreen()
{
	byte attr = 0007;
	ClrScr(attr);

	SystemBus_Write(0xc00021, 0x8000 | VIDEO_PAGE);           // Enable shell videopage
	SystemBus_Write(0xc00022, 0x8000 | ((attr >> 3) & 0x03)); // Enable shell border

	char str[33];
	sniprintf(str, sizeof(str), "    -= Speccy2010, v" VERSION " =-    ");

	WriteStr(0, 0, str);
	WriteAttr(0, 0, 0104, strlen(str));

	WriteAttr(0, 1, 0006, 32);
	WriteLine(1, 3);
	WriteLine(1, 5);

	WriteAttr(0, 20, 0006, 32);
	WriteLine(20, 3);
	WriteLine(20, 5);
}
//---------------------------------------------------------------------------------------
void Shell_Menu(CMenuItem *menu, int menuSize)
{
	static tm time;
	static dword tickCounter = 0;

	CPU_Stop();

	InitScreen();

	for (int i = 0; i < menuSize; i++) {
		menu[i].UpdateData();
		menu[i].UpdateState(0);
		menu[i].Redraw();
	}

	int menuPos = 0;
	if (menu == mainMenu)
		menuPos = 6;

	menu[menuPos].UpdateState(1);
	bool editMode = false;

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
						time.tm_mday += delta;
					if (menuPos == 1)
						time.tm_mon += delta;
					if (menuPos == 2)
						time.tm_year += delta;
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
					if (param != 0) {
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
				if (param != 0) {
					if (param->GetType() == PTYPE_LIST) {
						param->SetValue((param->GetValue() + 1) % (param->GetValueMax() + 1));

						if (param == GetParam(iniParameters, "Font")) {
							InitScreen();
							for (int i = 0; i < menuSize; i++)
								menu[i].Redraw();
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
				}
			}
		}
		else if (key == K_BACKSPACE) {
			const CParameter *param = menu[menuPos].GetParam();
			if (param != 0) {
				if (param->GetType() == PTYPE_STRING) {
					param->SetValueText("");
					menu[menuPos].UpdateData();
				}
			}
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
			mainMenu[0].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.2d", time.tm_mon + 1);
			mainMenu[1].UpdateData(temp);
			sniprintf(temp, sizeof(temp), "%.4d", time.tm_year + 1900);
			mainMenu[2].UpdateData(temp);

			tickCounter = Timer_GetTickCounter();
		}
	}

	SaveConfig();

	SystemBus_Write(0xc00021, 0); // Enable Video
	SystemBus_Write(0xc00022, 0);

	CPU_Start();
	Spectrum_UpdateConfig();
}
//---------------------------------------------------------------------------------------
