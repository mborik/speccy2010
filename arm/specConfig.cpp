#include "specConfig.h"
#include <string.h>

CSpecConfig specConfig;
//---------------------------------------------------------------------------------------
const CParameter iniParameters[] = {
	CParameter(PTYPE_STRING, PGRP_GENERAL, "FPGA config", (char *) sizeof(specConfig.fpgaConfigName), specConfig.fpgaConfigName),

	CParameter(PTYPE_LIST, PGRP_GENERAL, "Machine", "ZX Spectrum 48|ZX Spectrum 128|Pentagon 128|Pentagon 1024|Scorpion 256", &specConfig.specMachine),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Timings", "ZX Spectrum 48|ZX Spectrum 128|Pentagon|Scorpion", &specConfig.specSync),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Disk Interface", "Betadisk|DivMMC|MB-02", &specConfig.specDiskIf),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Turbo", "None|x2|x4|x8", &specConfig.specTurbo),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "AY mode", "None|ABC|ACB|Mono", &specConfig.specAyMode),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Turbo Sound", "Off|On", &specConfig.specTurboSound),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Covox", "Off|SD Mode1|SD Mode2|GS-#B3|Pentagon-#FB|Scorpion-#DD|Profi/BB55", &specConfig.specCovox),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "AY Chip", "YM|AY", &specConfig.specAyYm),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Video mode", "Composite/S-Video|PAL RGB|VGA 50Hz|VGA 60Hz|VGA 75Hz", &specConfig.specVideoMode),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Video aspect ratio", "4:3|5:4|16:9", &specConfig.specVideoAspectRatio),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Video interlace", "Off|On", &specConfig.specVideoInterlace),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Audio DAC mode", "R-2R|TDA1543|TDA1543A", &specConfig.specDacMode),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Joystick emulation", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyModeEmulation),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Joystick 1", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode1),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Joystick 2", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode2),
	CParameter(PTYPE_INT,  PGRP_GENERAL, "Mouse Sensitivity", "1|6|1", &specConfig.specMouseSensitivity),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Swap mouse buttons", "Off|On", &specConfig.specMouseSwap),
	CParameter(PTYPE_LIST, PGRP_GENERAL, "Font", "Speccy|Alt|Bold", &specConfig.specFont),

	CParameter(PTYPE_LIST,   PGRP_TRDOS, "BDI mode", "Slow|Fast", &specConfig.specBdiMode),
	CParameter(PTYPE_STRING, PGRP_TRDOS, "Disk A", (char *) sizeof(specConfig.specBdiImages[0].name), specConfig.specBdiImages[0].name),
	CParameter(PTYPE_LIST,   PGRP_TRDOS, "Disk A WP", "No|Yes", &specConfig.specBdiImages[0].writeProtect),
	CParameter(PTYPE_STRING, PGRP_TRDOS, "Disk B", (char *) sizeof(specConfig.specBdiImages[1].name), specConfig.specBdiImages[1].name),
	CParameter(PTYPE_LIST,   PGRP_TRDOS, "Disk B WP", "No|Yes", &specConfig.specBdiImages[1].writeProtect),
	CParameter(PTYPE_STRING, PGRP_TRDOS, "Disk C", (char *) sizeof(specConfig.specBdiImages[2].name), specConfig.specBdiImages[2].name),
	CParameter(PTYPE_LIST,   PGRP_TRDOS, "Disk C WP", "No|Yes", &specConfig.specBdiImages[2].writeProtect),
	CParameter(PTYPE_STRING, PGRP_TRDOS, "Disk D", (char *) sizeof(specConfig.specBdiImages[3].name), specConfig.specBdiImages[3].name),
	CParameter(PTYPE_LIST,   PGRP_TRDOS, "Disk D WP", "No|Yes", &specConfig.specBdiImages[3].writeProtect),

	CParameter(PTYPE_STRING, PGRP_MB02, "Disk A", (char *) sizeof(specConfig.specMB2Images[0].name), specConfig.specMB2Images[0].name),
	CParameter(PTYPE_LIST,   PGRP_MB02, "Disk A WP", "No|Yes", &specConfig.specMB2Images[0].writeProtect),
	CParameter(PTYPE_STRING, PGRP_MB02, "Disk B", (char *) sizeof(specConfig.specMB2Images[1].name), specConfig.specMB2Images[1].name),
	CParameter(PTYPE_LIST,   PGRP_MB02, "Disk B WP", "No|Yes", &specConfig.specMB2Images[1].writeProtect),
	CParameter(PTYPE_STRING, PGRP_MB02, "Disk C", (char *) sizeof(specConfig.specMB2Images[2].name), specConfig.specMB2Images[2].name),
	CParameter(PTYPE_LIST,   PGRP_MB02, "Disk C WP", "No|Yes", &specConfig.specMB2Images[2].writeProtect),
	CParameter(PTYPE_STRING, PGRP_MB02, "Disk D", (char *) sizeof(specConfig.specMB2Images[3].name), specConfig.specMB2Images[3].name),
	CParameter(PTYPE_LIST,   PGRP_MB02, "Disk D WP", "No|Yes", &specConfig.specMB2Images[3].writeProtect),

	CParameter(PTYPE_STRING, PGRP_ROMS, "48", (char *) sizeof(specConfig.specRomFile_Classic48), specConfig.specRomFile_Classic48),
	CParameter(PTYPE_STRING, PGRP_ROMS, "128", (char *) sizeof(specConfig.specRomFile_Classic128), specConfig.specRomFile_Classic128),
	CParameter(PTYPE_STRING, PGRP_ROMS, "Pentagon", (char *) sizeof(specConfig.specRomFile_Pentagon), specConfig.specRomFile_Pentagon),
	CParameter(PTYPE_STRING, PGRP_ROMS, "Scorpion", (char *) sizeof(specConfig.specRomFile_Scorpion), specConfig.specRomFile_Scorpion),
	CParameter(PTYPE_STRING, PGRP_ROMS, "TR-DOS", (char *) sizeof(specConfig.specRomFile_TRD_ROM), specConfig.specRomFile_TRD_ROM),
	CParameter(PTYPE_STRING, PGRP_ROMS, "EVO Reset Service", (char *) sizeof(specConfig.specRomFile_TRD_Service), specConfig.specRomFile_TRD_Service),
	CParameter(PTYPE_STRING, PGRP_ROMS, "DivMMC Firmware", (char *) sizeof(specConfig.specRomFile_DivMMC_FW), specConfig.specRomFile_DivMMC_FW),
	CParameter(PTYPE_STRING, PGRP_ROMS, "BS-DOS", (char *) sizeof(specConfig.specRomFile_BSROM_BSDOS), specConfig.specRomFile_BSROM_BSDOS),

	CParameter(PTYPE_END)
};
//---------------------------------------------------------------------------------------
int GroupFromName(CString name)
{
	int group = -1;
	if (name == "General")
		group = PGRP_GENERAL;
	else if (name == "TR-DOS")
		group = PGRP_TRDOS;
	else if (name == "MB-02")
		group = PGRP_MB02;
	else if (name == "ROM")
		group = PGRP_ROMS;

	return group;
}
//---------------------------------------------------------------------------------------
CString GetGroupName(int group)
{
	switch (group) {
		default:
		case PGRP_GENERAL:
			return "General";
		case PGRP_TRDOS:
			return "TR-DOS";
		case PGRP_MB02:
			return "MB-02";
		case PGRP_ROMS:
			return "ROM";
	}
}
//---------------------------------------------------------------------------------------
void RestoreConfig()
{
	strcpy(specConfig.fpgaConfigName, "speccy2010.rbf");

	specConfig.specMachine = SpecRom_Pentagon128;
	specConfig.specSync = SpecRom_Classic128;
	specConfig.specDiskIf = SpecDiskIf_Betadisk;
	specConfig.specTurbo = SpecTurbo_None;
	specConfig.specTrdosFlag = 0;
	specConfig.specVideoMode = 0;
	specConfig.specVideoAspectRatio = 0;
	specConfig.specVideoInterlace = 0;
	specConfig.specDacMode = 1;
	specConfig.specAyMode = 1;
	specConfig.specTurboSound = 1;
	specConfig.specCovox = 0;
	specConfig.specAyYm = 0;
	specConfig.specBdiMode = 1;
	specConfig.specJoyModeEmulation = SpecJoy_Cursor;
	specConfig.specJoyMode1 = SpecJoy_Kempston;
	specConfig.specJoyMode2 = SpecJoy_Kempston;
	specConfig.specMouseSensitivity = 4;
	specConfig.specMouseSwap = 0;
	specConfig.specFont = 1;

	strcpy(specConfig.specRomFile_Classic48, "roms/48.rom");
	strcpy(specConfig.specRomFile_Classic128, "roms/128.rom");
	strcpy(specConfig.specRomFile_Pentagon, "roms/pentagon.rom");
	strcpy(specConfig.specRomFile_Scorpion, "roms/scorpion.rom");
	strcpy(specConfig.specRomFile_TRD_ROM, "roms/trdos.rom");
	strcpy(specConfig.specRomFile_TRD_Service, "roms/system.rom");
	strcpy(specConfig.specRomFile_DivMMC_FW, "");
	strcpy(specConfig.specRomFile_BSROM_BSDOS, "");

	strcpy(specConfig.snaName, "");

	for (int i = 0; i < 4; i++) {
		strcpy(specConfig.specBdiImages[i].name, "");
		specConfig.specBdiImages[i].writeProtect = false;
		strcpy(specConfig.specMB2Images[i].name, "");
		specConfig.specMB2Images[i].writeProtect = false;
	}

	CSettingsFile file("speccy2010.ini");

	CString groupName;
	CString name;
	CString value;
	int group;

	while (file.ReadLine(groupName, name, value)) {
		//__TRACE("file.ReadLine: %s > %s > %s\n", (const char *)groupName, (const char *)name, (const char *)value);

		group = GroupFromName(groupName);
		if (group >= PGRP_GENERAL) {
			const CParameter *param = GetParam(iniParameters, name, group);

			if (param) {
				param->SetValueText(value);
			}
		}
	}

	file.Close();
}
//---------------------------------------------------------------------------------------
void SaveConfig()
{
	CSettingsFile file("speccy2010.ini", true);

	for (const CParameter *param = iniParameters; param->GetType() != PTYPE_END; param++) {
		CString value;
		param->GetValueText(value);
		CString group = GetGroupName(param->GetGroup());
		file.WriteLine(group, param->GetName(), value);
	}

	file.Close();
}
//---------------------------------------------------------------------------------------
