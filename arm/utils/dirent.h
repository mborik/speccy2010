#ifndef UTILS_DIRENT_H_INCLUDED
#define UTILS_DIRENT_H_INCLUDED

#include "../system.h"

struct FRECORD {
	dword size;
	byte attr;
	byte sel;
	word date;
	word time;
	char name[_MAX_LFN + 1];
};

const int FILES_PER_ROW = 18;
const int FILES_SIZE = 4096;

extern dword table_buffer;
extern int files_size;
extern int files_table_start;
extern int files_sel;
extern int files_sel_number;
extern byte files_sel_pos_x;
extern byte files_sel_pos_y;
extern char files_lastName[_MAX_LFN + 1];
extern bool too_many_files;

//---------------------------------------------------------------------------------------
const char *get_current_dir();
void read_dir();
void leave_dir();
void enter_dir(const char *name);
bool calc_sel();

#endif
