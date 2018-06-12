#ifndef SPEC_CONFIG_H_INCLUDED
#define SPEC_CONFIG_H_INCLUDED

#include "system.h"
#include "settings.h"

#define ADVANCED_BETADISK 1

enum SpecCfg_Group { PGRP_GENERAL = 0, PGRP_TRDOS, PGRP_ROMS };

enum SpecRom_Type { SpecRom_Classic48, SpecRom_Classic128, SpecRom_Pentagon128, SpecRom_Pentagon1024, SpecRom_Scorpion };
enum SpecDiskIf_Type { SpecDiskIf_Betadisk, SpecDiskIf_DivMMC };
enum SpecTurbo_Type { SpecTurbo_None, SpecTurbo_x2, SpecTurbo_x4, SpecTurbo_None_x8 };
enum SpecJoy_Type { SpecJoy_Kempston, SpecJoy_Sinclair1, SpecJoy_Sinclair2, SpecJoy_Cursor, SpecJoy_Qaopm };

struct CDiskImage {
	char name[PATH_SIZE];
	int writeProtect;
};

struct CSpecConfig {
	char fpgaConfigName[PATH_SIZE];

	int specMachine;
	int specSync;
	int specDiskIf;
	int specUseBank0;
	int specTurbo;
	int specVideoMode;
	int specVideoAspectRatio;
	int specVideoInterlace;
	int specDacMode;
	int specAyMode;
	int specTurboSound;
	int specCovox;
	int specAyYm;
	int specBdiMode;
	int specJoyModeEmulation;
	int specJoyMode1;
	int specJoyMode2;
	int specMouseSensitivity;
	int specMouseSwap;
	int specFont;

	CDiskImage specImages[4];

	char specRomFile_Classic48[PATH_SIZE];
	char specRomFile_Classic128[PATH_SIZE];
	char specRomFile_Pentagon[PATH_SIZE];
	char specRomFile_Scorpion[PATH_SIZE];
	char specRomFile_TRD_ROM[PATH_SIZE];
	char specRomFile_TRD_Service[PATH_SIZE];
	char specRomFile_DivMMC_FW[PATH_SIZE];

	char snaName[PATH_SIZE];
};

extern CSpecConfig specConfig;
extern const CParameter iniParameters[];

void RestoreConfig();
void SaveConfig();

#endif
