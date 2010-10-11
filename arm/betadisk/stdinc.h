#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "../system.h"

#define USE_RAM_FILE 0
#if USE_RAM_FILE

    #include "../ramFile.h"

    #define FIL        RFIL
    #define f_open     rf_open
    #define f_read     rf_read
    #define f_write    rf_write
    #define f_lseek    rf_lseek
    #define f_close    rf_close

#endif
