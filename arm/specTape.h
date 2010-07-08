#ifndef SPEC_TAPE_H_INCLUDED
#define SPEC_TAPE_H_INCLUDED

#include "types.h"

void Tape_SelectFile( const char *name );

void Tape_Start();
void Tape_Stop();
void Tape_Restart();
bool Tape_Started();

void Tape_Routine();
void Tape_TimerHandler();

#endif


