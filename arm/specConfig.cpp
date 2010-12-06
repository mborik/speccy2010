#include "specConfig.h"
#include <string.h>

CSpecConfig specConfig;

const CParameter *GetIniParameters()
{
    const static CParameter _iniParameters[] =
    {
        CParameter( PTYPE_LIST, "ROM/RAM", "ZX Spectrum 48|Pentagon 128|Pentagon 1024", &specConfig.specRom ),
        CParameter( PTYPE_LIST, "Timings", "ZX Spectrum 48|ZX Spectrum 128|Pentagon|Scorpion", &specConfig.specSync ),
        CParameter( PTYPE_LIST, "Turbo", "None|x2|x4|x8", &specConfig.specTurbo ),
        CParameter( PTYPE_LIST, "AY mode", "None|ABC|ACB|Mono", &specConfig.specAyMode ),
        CParameter( PTYPE_LIST, "BDI mode", "Slow|Fast", &specConfig.specBdiMode ),
        CParameter( PTYPE_LIST, "Video mode", "Composite/S-Video|PAL RGB|VGA 50Hz|VGA 60Hz|VGA 75Hz", &specConfig.specVideoMode ),
        CParameter( PTYPE_LIST, "Video aspect ratio", "4:3|5:4|16:9", &specConfig.specVideoAspectRatio ),
        CParameter( PTYPE_LIST, "Audio DAC mode", "R-2R|TDA1543|TDA1543A", &specConfig.specDacMode ),
        CParameter( PTYPE_LIST, "Joystick emulation", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyModeEmulation ),
        CParameter( PTYPE_LIST, "Joystick 1", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode1 ),
        CParameter( PTYPE_LIST, "Joystick 2", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode2 ),
        CParameter( PTYPE_INT, "Mouse Sensitivity", "1|6|1", &specConfig.specMouseSensitivity ),

        CParameter( PTYPE_STRING, "Disk A", (char*) sizeof(specConfig.specImages[0].name), &specConfig.specImages[0].name ),
        CParameter( PTYPE_LIST, "Disk A read only", "No|Yes", &specConfig.specImages[0].readOnly ),
        CParameter( PTYPE_STRING, "Disk B", (char*) sizeof(specConfig.specImages[1].name), &specConfig.specImages[1].name ),
        CParameter( PTYPE_LIST, "Disk B read only", "No|Yes", &specConfig.specImages[1].readOnly ),
        CParameter( PTYPE_STRING, "Disk C", (char*) sizeof(specConfig.specImages[2].name), &specConfig.specImages[2].name ),
        CParameter( PTYPE_LIST, "Disk C read only", "No|Yes", &specConfig.specImages[2].readOnly ),
        CParameter( PTYPE_STRING, "Disk D", (char*) sizeof(specConfig.specImages[3].name), &specConfig.specImages[3].name ),
        CParameter( PTYPE_LIST, "Disk D read only", "No|Yes", &specConfig.specImages[3].readOnly ),

        CParameter( PTYPE_END )
    };

    return _iniParameters;
}


void RestreConfig()
{
    specConfig.specVideoMode = 0;
    specConfig.specMouseSensitivity = 4;
    specConfig.specBdiMode = 1;

    for( int i = 0; i < 4; i++ )
    {
        strcpy( specConfig.specImages[i].name, "" );
        specConfig.specImages[i].readOnly = false;
    }

    CSettingsFile file( "speccy2010.ini" );

    CString groupName;
    CString name;
    CString value;

    while( file.ReadLine( groupName, name, value ) )
    {
        if( groupName == "General" )
        {
            const CParameter *param = GetParam( iniParameters, name );

            if( param != 0 )
            {
                param->SetValueText( value );
            }
        }
    }

    file.Close();
}

void SaveConfig()
{
    CSettingsFile file( "speccy2010.ini", true );

    for( const CParameter *param = iniParameters; param->GetType() != PTYPE_END; param++ )
    {
        CString value;
        param->GetValueText( value );
        file.WriteLine( "General", param->GetName(), value );
    }

    file.Close();
}
