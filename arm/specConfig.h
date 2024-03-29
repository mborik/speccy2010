#ifndef SPEC_CONFIG_H_INCLUDED
#define SPEC_CONFIG_H_INCLUDED

#include "system.h"
#include "settings.h"

enum SpecCfg_Group { PGRP_GENERAL = 0, PGRP_TRDOS, PGRP_MB02, PGRP_ROMS };

enum SpecRom_Type { SpecRom_Classic48, SpecRom_Classic128, SpecRom_Pentagon128, SpecRom_Pentagon1024, SpecRom_Scorpion };
enum SpecDiskIf_Type { SpecDiskIf_Betadisk, SpecDiskIf_DivMMC, SpecDiskIf_MB02 };
enum SpecTurbo_Type { SpecTurbo_None, SpecTurbo_x2, SpecTurbo_x4, SpecTurbo_None_x8 };
enum SpecJoy_Type { SpecJoy_None, SpecJoy_Kempston, SpecJoy_Sinclair1, SpecJoy_Sinclair2, SpecJoy_Cursor, SpecJoy_Qaopm };

struct CDiskImage {
	char name[PATH_SIZE];
	int writeProtect;
};

struct CSpecConfig {
	char fpgaConfigName[SHORT_PATH];

	bool modified;
	int specMachine;
	int specSync;
	int specDiskIf;
	int specTurbo;
	int specTrdosFlag;
	int specVideoMode;
	int specVideoAspectRatio;
	int specVideoInterlace;
	int specDacMode;
	int specAyMode;
	int specTurboSound;
	int specCovox;
	int specAyYm;
	int specJoyModeEmulation;
	int specJoyMode1;
	int specJoyMode2;
	int specMouseSensitivity;
	int specMouseSwap;
	int specFont;

	CDiskImage specBdiImages[4];
	CDiskImage specMB2Images[4];

	char specRomFile_Classic48[SHORT_PATH];
	char specRomFile_Classic128[SHORT_PATH];
	char specRomFile_Pentagon[SHORT_PATH];
	char specRomFile_Scorpion[SHORT_PATH];
	char specRomFile_TRD_ROM[SHORT_PATH];
	char specRomFile_TRD_Service[SHORT_PATH];
	char specRomFile_DivMMC_FW[SHORT_PATH];
	char specRomFile_BSROM_BSDOS[SHORT_PATH];

	char snaName[PATH_SIZE];
};

extern CSpecConfig specConfig;
extern const CParameter iniParameters[];

void RestoreConfig();
void SaveConfig();

#endif
