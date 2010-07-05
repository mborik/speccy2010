#ifndef RAM_FILE_H_INCLUDED
#define RAM_FILE_H_INCLUDED

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _RFIL_
{
	byte	flag;
	dword	fptr;
	dword	fsize;

	int     ramBank;
} RFIL;

int rf_open ( RFIL*, const char*, byte );		        /* Open or create a file */
int rf_read ( RFIL*, void*, dword, dword* );			/* Read data from a file */
int rf_write ( RFIL*, const void*, dword, dword* );	/* Write data to a file */
int rf_lseek ( RFIL*, dword );			    	    /* Move file pointer of a file object */
int rf_close (RFIL* );								/* Close an open file object */

#ifdef __cplusplus
}
#endif

#endif


