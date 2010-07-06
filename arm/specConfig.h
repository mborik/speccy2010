#ifndef SPEC_CONFIG_H_INCLUDED
#define SPEC_CONFIG_H_INCLUDED

#include "types.h"
#include "string.h"
#include "settings.h"

#define ADVANCED_BETADISK 1
#define LOG_BDI_PORTS 0

enum SpecRom_Type { SpecRom_Classic48, SpecRom_Pentagon128, SpecRom_Pentagon1024 };
enum SpecSync_Type { SpecSync_Classic48, SpecSync_Pentagon };
enum SpecTurbo_Type { SpecTurbo_None, SpecTurbo_x2, SpecTurbo_x4, SpecTurbo_None_x8 };
enum SpecJoy_Type { SpecJoy_Kempston, SpecJoy_Sinclair1, SpecJoy_Sinclair2, SpecJoy_Cursor, SpecJoy_Qaopm };

struct CSpecConfig
{
	int specRom;
	int specSync;
	int specTurbo;
	int specVideoMode;
	int specVideoSubcarrierDelta;
	int specDacMode;
	int specAyMode;
	int specJoyModeEmulation;
	int specJoyMode1;
	int specJoyMode2;
	int specMouseSensitivity;
};

extern CSpecConfig specConfig;
//extern CParameter iniParameters[];

const CParameter *GetIniParameters();
#define iniParameters GetIniParameters()

void RestreConfig();
void SaveConfig();

#endif


