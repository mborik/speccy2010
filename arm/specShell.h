#ifndef SPEC_SHELL_H_INCLUDED
#define SPEC_SHELL_H_INCLUDED

#include "system.h"
#include "settings.h"

const byte K_ESC = 0x1b;
const byte K_RETURN = 0x0d;
const byte K_BACKSPACE = 0x08;
const byte K_TAB = 0x09;

const byte K_UP = 0x01;
const byte K_DOWN = 0x02;
const byte K_LEFT = 0x03;
const byte K_RIGHT = 0x04;

const byte K_F1 = 0x05;
const byte K_F2 = 0x06;
const byte K_F3 = 0x07;
const byte K_F4 = 0x0a;
const byte K_F5 = 0x0b;
const byte K_F6 = 0x0c;
const byte K_F7 = 0x0e;
const byte K_F8 = 0x10;
const byte K_F9 = 0x11;
const byte K_F10 = 0x12;
const byte K_F11 = 0x13;
const byte K_F12 = 0x14;

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
void Shell_Pause();

const char *Shell_GetPath();

#endif





