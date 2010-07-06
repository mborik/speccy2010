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

void DecodeKey( word keyCode, word keyFlags )
{
    static bool resetState = false;

    bool flagKeyRelease = ( keyFlags & fKeyRelease ) != 0;

    bool reset1 = ( keyFlags & ( fKeyCtrlLeft | fKeyCtrlRight ) ) == ( fKeyCtrlLeft | fKeyCtrlRight );

    static bool reset2 = false;
    if( keyCode == KEY_POWER && !flagKeyRelease ) reset2 = true;
    if( keyCode == KEY_POWER && flagKeyRelease ) reset2 = false;

    if( ( reset1 || reset2 ) != resetState )
    {
        resetState = reset1 || reset2;

        if( resetState ) ResetKeyboard();
        CPU_Reset( resetState );
    }

    //------------------------------------------------------

    if( !flagKeyRelease )
    {
        if( ( keyFlags & fKeyCtrl ) != 0 )
        {
            if( keyCode == KEY_1 )
            {
                specConfig.specVideoMode = 0;
                Spectrum_UpdateConfig();
                SaveConfig();
            }
            else if( keyCode == KEY_2 )
            {
                specConfig.specVideoMode = 1;
                Spectrum_UpdateConfig();
                SaveConfig();
            }
        }
        else
        {
            if( keyCode == KEY_F12 ) Shell_Enter();
            if( keyCode == KEY_F11 ) SaveSnapshot( Shell_GetPath(), 0 );

            if ( keyCode == KEY_EQUALS || keyCode == KEY_KP_PLUS )
            {
                if( !Tape_Started() ) Tape_Restart();
            }

            if ( keyCode == KEY_MINUS || keyCode == KEY_KP_MINUS )
            {
                if( !Tape_Started() ) Tape_Start();
                else Tape_Stop();
            }

            if ( keyCode == KEY_F1 )
            {
                SystemBus_Write( 0xc00000, 0x00004 );
            }

            if( keyCode == KEY_F2 )
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
        }
    }

    //------------------------------------------------------

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







