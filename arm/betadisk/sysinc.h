#define TRACE( x )      __TRACE x
//#define TRACE( x )

#define sysword_t word

#define FDC_PORTABLE			0
#define FAST_FLOPPY				0
#define FLOPPY_SET_SEC			1			// установка сектора, не дожидаясь прокрутки диска
#define TRD_ON_ODI				0

#define BDI_TRACE_FLAG          0
#define WD_TRACE_FLAG           0
#define WDH_TRACE_FLAG          0
#define SCL_TRACE_FLAG          0
#define FLP_TRACE_FLAG          0
#define FLPh_TRACE_FLAG         0

#define FDI_TRACE_FLAG          0
#define TRD_TRACE_FLAG          0

#include "types.h"

char *wd_stat_decode( word data, word data2 );
char *wd_command_decode( word data );
char *ticks_str( word data );
char *ticks_str1( word data );
char *ticks_str2( word data );

char *bdi_sys_ctl_decode( word data );
char *bdi_port_desc( word data );
char *bdi_sys_stat_decode( word data );



