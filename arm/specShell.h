#ifndef SPEC_SHELL_H_INCLUDED
#define SPEC_SHELL_H_INCLUDED

#include "settings.h"
#include "system.h"

class CMenuItem {
	int x, y, state;
	const char *name;

	CString data;

	const CParameter *param;

public:
	CMenuItem(int _x, int _y, const char *_name, const CParameter *_param);
	CMenuItem(int _x, int _y, const char *_name, const char *_data);

	void Redraw();
	void UpdateData();
	void UpdateData(const char *_data);
	void UpdateState(int _state);

	const char *GetName() { return name; }
	const CParameter *GetParam() { return param; }

	void OnKey(byte key);
};

void Debugger_Enter();

bool Shell_Browser();

bool Shell_Menu(CMenuItem *menu, int menuSize);
bool Shell_SettingsMenu();
bool Shell_DisksMenu();
void Shell_Pause();
void Shell_SaveSnapshot();

const char *Shell_GetPath();
const char *GetFatErrorMsg(int id);

#endif
