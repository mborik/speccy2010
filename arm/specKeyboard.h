#ifndef SPEC_KEYBOARD_H_INCLUDED
#define SPEC_KEYBOARD_H_INCLUDED

#include "types.h"

void ResetKeyboard();
void UpdateKeyPort();

void DecodeKeySpec( word keyCode, word keyFlags );
void DecodeKey( word keyCode, word keyFlags );

char CodeToChar( word keyCode );

const char K_ESC = 0x1b;
const char K_RETURN = 0x0d;
const char K_BACKSPACE = 0x08;
const char K_TAB = 0x09;

const char K_UP = 0x01;
const char K_DOWN = 0x02;
const char K_LEFT = 0x03;
const char K_RIGHT = 0x04;

const char K_F1 = 0x05;
const char K_F2 = 0x06;
const char K_F3 = 0x07;
const char K_F4 = 0x0a;
const char K_F5 = 0x0b;
const char K_F6 = 0x0c;
const char K_F7 = 0x0e;
const char K_F8 = 0x10;
const char K_F9 = 0x11;
const char K_F10 = 0x12;
const char K_F11 = 0x13;
const char K_F12 = 0x14;

const char K_PAGEUP = 0x15;
const char K_PAGEDOWN = 0x16;
const char K_HOME = 0x17;
const char K_END = 0x18;
const char K_DELETE = 0x19;
const char K_INSERT = 0x1a;

#endif


