#ifndef UTILS_FILE_H_INCLUDED
#define UTILS_FILE_H_INCLUDED

#include "../system.h"

bool FileExists(const char *str);
bool ReadLine(FIL *file, CString &str);
bool WriteText(FIL *file, const char *str);
bool WriteLine(FIL *file, const char *str);

//---------------------------------------------------------------------------------------

byte read_file(FIL *f, byte *dst, byte sz);
byte write_file(FIL *f, byte *dst, byte sz);
byte read_le_byte(FIL *f, byte *dst);
byte read_le_word(FIL *f, word *dst);
byte read_le_dword(FIL *f, dword *dst);

#endif
