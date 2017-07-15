#include <stdio.h>
#include <string.h>
#include <new>
#include <malloc.h>

#include "system.h"

#include "specKeyboard.h"
#include "specKeyMap.h"
#include "specTape.h"
#include "specShell.h"
#include "specSnapshot.h"
#include "specConfig.h"

static byte keyPort[ SPEC_KEYS_NUMBER ];

void UpdateKeyPort()
{
    byte *keyPortPtr = keyPort;
    word temp;

    for( int i = 0; i < 4; i++ )
    {
        temp = 0xFFFF;

        if( *keyPortPtr++ ) temp &= ~0x0001;
        if( *keyPortPtr++ ) temp &= ~0x0002;
        if( *keyPortPtr++ ) temp &= ~0x0004;
        if( *keyPortPtr++ ) temp &= ~0x0008;
        if( *keyPortPtr++ ) temp &= ~0x0010;

        if( *keyPortPtr++ ) temp &= ~0x0100;
        if( *keyPortPtr++ ) temp &= ~0x0200;
        if( *keyPortPtr++ ) temp &= ~0x0400;
        if( *keyPortPtr++ ) temp &= ~0x0800;
        if( *keyPortPtr++ ) temp &= ~0x1000;

        SystemBus_Write( 0xc00010 + i, temp );
    }

    temp = 0x0000;

    if( *keyPortPtr++ ) temp |= 0x0001;
    if( *keyPortPtr++ ) temp |= 0x0002;
    if( *keyPortPtr++ ) temp |= 0x0004;
    if( *keyPortPtr++ ) temp |= 0x0008;
    if( *keyPortPtr++ ) temp |= 0x0010;

    SystemBus_Write( 0xc00014, temp );
}

void ResetKeyboard()
{
    for( int i = 0; i < SPEC_KEYS_NUMBER; i++ ) keyPort[i] = 0;
    UpdateKeyPort();
}

void SetSpecKey( byte code, bool set )
{
    if( code < SPEC_KEYS_NUMBER )
    {
        if( set && keyPort[ code ] != 0xff ) keyPort[ code ]++;
        else if( !set && keyPort[ code ] != 0 ) keyPort[ code ]--;
    }

    UpdateKeyPort();
}

void DecodeKeyMatrix( word keyCode, bool flagKeyRelease, const CMatrixRecord *matrix )
{
    word code;

    for ( ; ( code = matrix->keyCode ) != KEY_NONE; matrix++ )
    {
        if ( code == keyCode )
        {
            if ( !flagKeyRelease )
            {
                SetSpecKey( matrix->specKeyCode0, true );

                if( matrix->specKeyCode1 != SPEC_KEY_NONE )
                {
                    DelayMs( 2 );
                    SetSpecKey( matrix->specKeyCode1, true );
                }
            }
            else
            {
                if( matrix->specKeyCode1 != SPEC_KEY_NONE )
                {
                    SetSpecKey( matrix->specKeyCode1, false );
                    DelayMs( 2 );
                }

                SetSpecKey( matrix->specKeyCode0, false );
            }

            break;
        }
    }
}

bool DecodeSymbolShortcut( word keyCode, word keyFlags, const CSymbolShortcutKeyRecord *map )
{
    if ( ( keyFlags & fKeyPCEmu ) != 0 )
        return false;

    bool flagKeyRelease = ( keyFlags & fKeyRelease );
    word code, shifts = (keyFlags & (fKeyShift | fKeyCtrl));

    for ( ; ( code = map->keyCode ) != KEY_NONE; map++ )
    {
        if ( code == keyCode )
        {
            __TRACE( "# keyCode - 0x%04x, 0x%04x[m:0x%04x], ext: %d\n", keyCode, keyFlags, map->ctrlMask, map->extend);

            if ( ( map->ctrlMask && (shifts & map->ctrlMask)) ||
                 ( map->ctrlMask == 0 && shifts == 0 ) )
            {
                if ( map->extend )
                {
                    if ( !flagKeyRelease )
                    {
                        SetSpecKey( SPEC_KEY_CAPS_SHIFT, true );
                        DelayMs( 2 );
                        SetSpecKey( SPEC_KEY_SYMBOL_SHIFT, true );
                    }
                    else
                    {
                        SetSpecKey( SPEC_KEY_SYMBOL_SHIFT, false );
                        DelayMs( 2 );
                        SetSpecKey( SPEC_KEY_CAPS_SHIFT, false );
                    }

                    DelayMs( 10 );
                }

                if ( !flagKeyRelease )
                {
                    SetSpecKey( SPEC_KEY_SYMBOL_SHIFT, true );
                    DelayMs( 3 );
                    SetSpecKey( map->specKeyCode, true );
                }
                else
                {
                    SetSpecKey( map->specKeyCode, false );
                    DelayMs( 3 );
                    SetSpecKey( SPEC_KEY_SYMBOL_SHIFT, false );
                }

                return true;
            }
        }
    }

    return false;
}

void DecodeKeySpec( word keyCode, word keyFlags )
{
    bool flagKeyRelease = ( keyFlags & fKeyRelease ) != 0;

    if ( !DecodeSymbolShortcut( keyCode, keyFlags, keyMatrixSymbolShortcuts ) )
        DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixMain );

    int joyMode = specConfig.specJoyModeEmulation;

    if( ( keyFlags & fKeyJoy1 ) != 0 ) joyMode = specConfig.specJoyMode1;
    else if( ( keyFlags & fKeyJoy2 ) != 0 ) joyMode = specConfig.specJoyMode2;

    switch( joyMode )
    {
        case SpecJoy_Kempston : DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixKempston );
                                break;
        case SpecJoy_Cursor : DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixCursor );
                                break;
        case SpecJoy_Sinclair1 : DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixSinclair1 );
                                break;
        case SpecJoy_Sinclair2 : DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixSinclair2 );
                                break;
        case SpecJoy_Qaopm : DecodeKeyMatrix( keyCode, flagKeyRelease, keyMatrixQaopm );
                                break;
    }
}

void DecodeKey( word keyCode, word keyFlags )
{
    static bool resetState = false;

    bool flagKeyRelease = ( keyFlags & fKeyRelease ) != 0;

    bool reset1 = ( keyFlags & fKeyAlt ) == fKeyAlt;

    static bool reset2 = false;
    if( keyCode == KEY_POWER && !flagKeyRelease ) reset2 = true;
    if( keyCode == KEY_POWER && flagKeyRelease ) reset2 = false;

    static bool reset3 = false;
    if( keyCode == KEY_PAUSE && !flagKeyRelease ) reset3 = true;
    if( keyCode == KEY_PAUSE && flagKeyRelease ) reset3 = false;

    if( ( reset1 || reset2 || reset3 ) != resetState )
    {
        resetState = reset1 || reset2 || reset3;
        CPU_Reset( resetState );
        DelayMs( 100 );
    }

    //------------------------------------------------------

    if( !flagKeyRelease )
    {
        if( ( keyFlags & fKeyAlt ) )
        {
            switch( keyCode )
            {
                case KEY_1 :
                    specConfig.specVideoMode = 0;
                    Spectrum_UpdateConfig();
                    SaveConfig();
                    break;
                case KEY_2 :
                    specConfig.specVideoMode = 1;
                    Spectrum_UpdateConfig();
                    SaveConfig();
                    break;
                case KEY_3 :
                    specConfig.specVideoMode = 2;
                    Spectrum_UpdateConfig();
                    SaveConfig();
                    break;
                case KEY_4 :
                    specConfig.specVideoMode = 3;
                    Spectrum_UpdateConfig();
                    SaveConfig();
                    break;
                case KEY_5 :
                    specConfig.specVideoMode = 4;
                    Spectrum_UpdateConfig();
                    SaveConfig();
                    break;
                case KEY_F12 :
                    CPU_NMI();
                    break;
            }
        }
        else if ( keyFlags & ( fKeyAlt | fKeyCtrl ) )
        {
            int kc;

            switch ( keyCode )
            {
                case KEY_F1:
                    kc = 0;
                    break;
                case KEY_F2:
                    kc = 1;
                    break;
                case KEY_F3:
                    kc = 2;
                    break;
                case KEY_F4:
                    kc = 3;
                    break;
                case KEY_F5:
                    kc = 4;
                    break;
                case KEY_F6:
                    kc = 5;
                    break;
                case KEY_F7:
                    kc = 6;
                    break;
                case KEY_F8:
                    kc = 7;
                    break;
                case KEY_F9:
                    kc = 8;
                    break;
                case KEY_F10:
                    kc = 9;
                    break;
                default:
                    kc = -1;
                    break;
            }
            if ( kc >= 0 )
            {
                char snaName[ 0x10 ];
                sniprintf( snaName, sizeof(snaName), "!slot_%.1d.sna", kc );
                SaveSnapshot( snaName );
            }
        }

        else
        {
            switch( keyCode )
            {
                case KEY_PAUSE :
                    Shell_Pause();
                    break;

                case KEY_F1 :
                    specConfig.specTurbo ^= 3;
                    Spectrum_UpdateConfig();
                    //SaveConfig();
                    break;
                case KEY_F2 :
                    specConfig.specTurbo = 1;
                    Spectrum_UpdateConfig();
                    //SaveConfig();
                    break;
                case KEY_F3 :
                    specConfig.specTurbo = 2;
                    Spectrum_UpdateConfig();
                    //SaveConfig();
                    break;
                case KEY_F4 :
                    if( ( keyFlags & fKeyAlt ) != 0 )
                        SystemBus_Write( 0xc00000, 0x00004 );
                    else {
                        specConfig.specTurbo ^= 3;
                        Spectrum_UpdateConfig();
                        //SaveConfig();
                    }
                    break;

                case KEY_F5 :
                    CPU_NMI();
                    break;

                case KEY_F6 :
                    Shell_DisksMenu();
                    break;

                case KEY_F7 :
                    if( ( keyFlags & fKeyAlt ) != 0 ) Shell_SaveSnapshot();
                    else SaveSnapshot( UpdateSnaName() );
                   break;

                case KEY_F8 :
                    {
                        CPU_Stop();

                        byte specPort7ffd = SystemBus_Read( 0xc00017 );

                        byte page = ( specPort7ffd & ( 1 << 3 ) ) != 0 ? 7 : 5;
                        dword addr = 0x800000 | ( page << 13 );

                        SystemBus_Write( 0xc00020, 0 );  // use bank 0

                        for( int i = 0x1800; i < 0x1b00; i += 2 )
                        {
                            SystemBus_Write( addr + ( i >> 1 ), 0x3838 );
                        }

                        CPU_Start();
                    }
                    break;

                case KEY_F9 :
                    Shell_SettingsMenu();
                    break;

                case KEY_F10 :
                case KEY_KP_PLUS :
                    if( !Tape_Started() ) Tape_Restart();
                    break;

                case KEY_F11 :
                case KEY_KP_MINUS :
                    if( !Tape_Started() ) Tape_Start();
                    else Tape_Stop();
                    break;

                case KEY_F12 :
                    if( ( keyFlags & fKeyAlt ) != 0 ) Debugger_Enter();
                    else Shell_Browser();
                    break;
            }
        }
    }
}

char CodeToChar( word keyCode )
{
    word keyFlags = ReadKeyFlags();

    bool shift = ( keyFlags & fKeyShift ) != 0;
    bool caps = ( keyFlags & fKeyCaps ) != 0;
    if( shift ) caps = !caps;

    switch( keyCode )
    {
        case KEY_NONE : return 0;

        case KEY_ESC : return K_ESC;
        case KEY_RETURN : return K_RETURN;
        case KEY_BACKSPACE : return K_BACKSPACE;

        case KEY_UP : return K_UP;
        case KEY_DOWN : return K_DOWN;
        case KEY_LEFT : return K_LEFT;
        case KEY_RIGHT : return K_RIGHT;

        case KEY_PAGEUP : return K_PAGEUP;
        case KEY_PAGEDOWN : return K_PAGEDOWN;
        case KEY_HOME : return K_HOME;
        case KEY_END : return K_END;

        case KEY_DELETE : return K_DELETE;
        case KEY_INSERT : return K_INSERT;

        case KEY_F1 : return K_F1;
        case KEY_F2 : return K_F2;
        case KEY_F3 : return K_F3;
        case KEY_F4 : return K_F4;
        case KEY_F5 : return K_F5;
        case KEY_F6 : return K_F6;
        case KEY_F7 : return K_F7;
        case KEY_F8 : return K_F8;
        case KEY_F9 : return K_F9;
        case KEY_F10 : return K_F10;
        case KEY_F11 : return K_F11;
        case KEY_F12 : return K_F12;

        case KEY_SPACE : return ' ';

        case KEY_1 : return !shift ? '1' : '!';
        case KEY_2 : return !shift ? '2' : '@';
        case KEY_3 : return !shift ? '3' : '#';
        case KEY_4 : return !shift ? '4' : '$';
        case KEY_5 : return !shift ? '5' : '%';
        case KEY_6 : return !shift ? '6' : '^';
        case KEY_7 : return !shift ? '7' : '&';
        case KEY_8 : return !shift ? '8' : '*';
        case KEY_9 : return !shift ? '9' : '(';
        case KEY_0 : return !shift ? '0' : ')';

        case KEY_MINUS : return !shift ? '-' : '_';
        case KEY_EQUALS : return !shift ? '=' : '+';

        case KEY_SLASH : return !shift ? '/' : '?';
        case KEY_BACKSLASH : return !shift ? '\\' : '|';

        case KEY_A : return !caps ? 'a' : 'A';
        case KEY_B : return !caps ? 'b' : 'B';
        case KEY_C : return !caps ? 'c' : 'C';
        case KEY_D : return !caps ? 'd' : 'D';
        case KEY_E : return !caps ? 'e' : 'E';
        case KEY_F : return !caps ? 'f' : 'F';
        case KEY_G : return !caps ? 'g' : 'G';
        case KEY_H : return !caps ? 'h' : 'H';
        case KEY_I : return !caps ? 'i' : 'I';
        case KEY_J : return !caps ? 'j' : 'J';
        case KEY_K : return !caps ? 'k' : 'K';
        case KEY_L : return !caps ? 'l' : 'L';
        case KEY_M : return !caps ? 'm' : 'M';
        case KEY_N : return !caps ? 'n' : 'N';
        case KEY_O : return !caps ? 'o' : 'O';
        case KEY_P : return !caps ? 'p' : 'P';
        case KEY_Q : return !caps ? 'q' : 'Q';
        case KEY_R : return !caps ? 'r' : 'R';
        case KEY_S : return !caps ? 's' : 'S';
        case KEY_T : return !caps ? 't' : 'T';
        case KEY_U : return !caps ? 'u' : 'U';
        case KEY_V : return !caps ? 'v' : 'V';
        case KEY_W : return !caps ? 'w' : 'W';
        case KEY_X : return !caps ? 'x' : 'X';
        case KEY_Y : return !caps ? 'y' : 'Y';
        case KEY_Z : return !caps ? 'z' : 'Z';

        case KEY_BACKQUOTE :    return !shift ? '`' : '~';
        case KEY_LEFTBRACKET :  return !shift ? '[' : '{';
        case KEY_RIGHTBRACKET : return !shift ? ']' : '}';
        case KEY_COMMA :        return !shift ? ',' : '<';
        case KEY_PERIOD :       return !shift ? '.' : '>';
        case KEY_SEMICOLON :    return !shift ? ';' : ':';
        case KEY_QUOTE :        return !shift ? '\'' : '\"';
    }

    return 0;
}
