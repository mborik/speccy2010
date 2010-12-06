#ifndef SPEC_SHELL_H_INCLUDED
#define SPEC_SHELL_H_INCLUDED

#include "system.h"
#include "settings.h"

const byte K_ESC = 0x1b;
const byte K_RETURN = 0x0d;
const byte K_BACKSPACE = 0x08;

const byte K_UP = 0x01;
const byte K_DOWN = 0x02;
const byte K_LEFT = 0x03;
const byte K_RIGHT = 0x04;

class CMenuItem
{
    int x, y, state;

    const char *name;

    CString data;

    const CParameter *param;

public:
    CMenuItem( int _x, int _y, const char *_name, const CParameter *_param );
    CMenuItem( int _x, int _y, const char *_name, const char *_data );

    void Redraw();
    void UpdateData();
    void UpdateData( const char *_data );
    void UpdateState( int _state );

    const char *GetName() { return name; }
    const CParameter *GetParam() { return param; }

    void OnKey( byte key );
};


void CPU_Start();
void CPU_Stop();
bool CPU_Stopped();

void CPU_Reset( bool res );
void CPU_ModifyPC( word pc, byte istate );

void Debugger_Enter();

bool Shell_Browser();

bool Shell_Menu( CMenuItem *menu, int menuSize );
bool Shell_SettingsMenu();
bool Shell_DisksMenu();

const char *Shell_GetPath();

#endif





