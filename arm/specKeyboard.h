#ifndef SPEC_KEYBOARD_H_INCLUDED
#define SPEC_KEYBOARD_H_INCLUDED

#include "types.h"

void ResetKeyboard();
void UpdateKeyPort();

void DecodeKey( word keyCode, word keyFlags );

#endif


