#ifndef MEM_H_
#define MEM_H_

#include "types.h"
#include "fatfs/ff.h"

#define PATH_SIZE 96
#define SHORT_PATH 32

typedef struct FRECORD {
	dword size;
	byte attr;
	byte sel;
	word date;
	word time;
	char name[_MAX_LFN + 1];
} FRECORD;

struct MemManager {
	char srcName[PATH_SIZE];
	char dstName[PATH_SIZE];
	char fullName[PATH_SIZE];
	char shortName[0x30];
	char sname[0x30];
	char trace[0x80];
	byte sector[0x100];

	FIL fsrc, fdst;
	FILINFO finfo;
	FRECORD ra, rb;
};
//-----------------------------------------------------------------------------
extern struct MemManager mem;
//-----------------------------------------------------------------------------
#endif
