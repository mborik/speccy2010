#ifndef SYSTEM_BUS_H_INCLUDED
#define SYSTEM_BUS_H_INCLUDED

#include "../system.h"

bool SystemBus_TestConfiguration();
void SystemBus_SetAddress(dword address);
word SystemBus_Read();
word SystemBus_Read(dword address);
void SystemBus_Write(word data);
void SystemBus_Write(dword address, word data);

#endif
