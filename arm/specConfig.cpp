#include "specConfig.h"
#include <string.h>

CSpecConfig specConfig;

const CParameter *GetIniParameters()
{
    const static CParameter _iniParameters[] =
    {
        CParameter( PTYPE_LIST, "ROM/RAM", "Classic48|Pentagon128|Pentagon1024", &specConfig.specRom ),
        CParameter( PTYPE_LIST, "Timings", "Classic48|Pentagon", &specConfig.specSync ),
        CParameter( PTYPE_LIST, "Turbo", "None|x2|x4|x8", &specConfig.specTurbo ),
        CParameter( PTYPE_LIST, "AY mode", "None|ABC|ACB|Mono", &specConfig.specAyMode ),
        CParameter( PTYPE_LIST, "Video mode", "Composite/S-Video|PAL RGB", &specConfig.specVideoMode ),
        CParameter( PTYPE_INT, "Video subcarrier delta", "860000|900000|1", &specConfig.specVideoSubcarrierDelta ),
        CParameter( PTYPE_LIST, "Audio DAC mode", "R-2R|TDA1543|TDA1543A", &specConfig.specDacMode ),
        CParameter( PTYPE_LIST, "Joystick emulation", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyModeEmulation ),
        CParameter( PTYPE_LIST, "Joystick 1", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode1 ),
        CParameter( PTYPE_LIST, "Joystick 2", "Kempston|Sinclair I|Sinclair II|Cursor|QAOPM", &specConfig.specJoyMode2 ),
        CParameter( PTYPE_END )
    };

    return _iniParameters;
}


void RestreConfig()
{
    specConfig.specVideoMode = 0;
    specConfig.specVideoSubcarrierDelta = 885662;

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
