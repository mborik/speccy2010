#ifndef UTILS_FILE_H_INCLUDED
#define UTILS_FILE_H_INCLUDED

#include "../system.h"

bool FileExists(const char *str);
bool ReadLine(FIL *file, CString &str);
bool WriteText(FIL *file, const char *str);
bool WriteLine(FIL *file, const char *str);

#endif
