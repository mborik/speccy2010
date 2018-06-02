#include "sdram.h"
#include "../system.h"

dword sdramHeapPtr = VIDEO_PAGE_PTR + SDRAM_PAGE_SIZE;
//---------------------------------------------------------------------------------------
dword Malloc(word recordCount, word recordSize)
{
	dword size = recordSize;
	if ((size & 1) != 0)
		size++;

	size *= recordCount;
	if (sdramHeapPtr + 4 + size > SDRAM_PAGES * SDRAM_PAGE_SIZE)
		return 0;

	dword result = sdramHeapPtr;
	sdramHeapPtr += 4 + size;

	dword addr = 0x800000 | (result >> 1);
	SystemBus_Write(addr++, recordSize);
	SystemBus_Write(addr++, recordCount);

	return result;
}
//---------------------------------------------------------------------------------------
void Free(dword sdramPtr)
{
	if (sdramPtr != 0)
		sdramHeapPtr = sdramPtr;
}
//---------------------------------------------------------------------------------------
bool Read(void *rec, dword sdramPtr, word pos)
{
	if (sdramPtr == 0)
		return false;

	dword addr = 0x800000 | (sdramPtr >> 1);
	word recordSize = SystemBus_Read(addr++);
	word recordCount = SystemBus_Read(addr++);

	if (pos >= recordCount)
		return false;

	addr += ((recordSize + 1) >> 1) * pos;
	byte *ptr = (byte *) rec;

	while (recordSize > 0) {
		word data = SystemBus_Read(addr++);

		*ptr++ = (byte) data;
		recordSize--;

		if (recordSize > 0) {
			*ptr++ = (byte)(data >> 8);
			recordSize--;
		}
	}

	return true;
}
//---------------------------------------------------------------------------------------
bool Write(void *rec, dword sdramPtr, word pos)
{
	if (sdramPtr == 0)
		return false;

	dword addr = 0x800000 | (sdramPtr >> 1);
	word recordSize = SystemBus_Read(addr++);
	word recordCount = SystemBus_Read(addr++);

	if (pos >= recordCount)
		return false;

	addr += ((recordSize + 1) >> 1) * pos;
	byte *ptr = (byte *) rec;

	for (word i = 0; i < recordSize; i += 2) {
		SystemBus_Write(addr++, ptr[0] | (ptr[1] << 8));
		ptr += 2;
	}

	return true;
}
//---------------------------------------------------------------------------------------
