#ifndef SPEC_SNAPSHOT_H_INCLUDED
#define SPEC_SNAPSHOT_H_INCLUDED

#include "system.h"

void SaveSnapshot( const char *name );
bool LoadSnapshot( const char *fileName );

const char *UpdateSnaName();

#endif





