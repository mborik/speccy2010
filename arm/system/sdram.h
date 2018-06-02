#ifndef SYSTEM_SDRAM_H_INCLUDED
#define SYSTEM_SDRAM_H_INCLUDED

#include "../types.h"

const dword SDRAM_PAGES = 0x200;
const dword SDRAM_PAGE_SIZE = 0x4000;

const dword VIDEO_PAGE = 0x108;
const dword VIDEO_PAGE_PTR = VIDEO_PAGE * SDRAM_PAGE_SIZE;

dword Malloc(word recordCount, word recordSize);
void Free(dword sdramPtr);
bool Read(void *rec, dword sdramPtr, word pos);
bool Write(void *rec, dword sdramPtr, word pos);

#endif
