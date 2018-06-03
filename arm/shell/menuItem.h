#ifndef SHELL_MENUITEM_H_INCLUDED
#define SHELL_MENUITEM_H_INCLUDED

#include "../settings.h"
#include "../utils/cstring.h"

class CMenuItem {
	int x, y, state;
	const char *name;
	byte colors[4];

	CString data;
	const CParameter *param;

	void Init(int _x, int _y, const char *_name, const CParameter *_param);

public:
	CMenuItem(int _x, int _y, const char *_name, const CParameter *_param);
	CMenuItem(int _x, int _y, const char *_name, const char *_data);

	void Redraw();
	void UpdateData();
	void UpdateData(const char *_data);
	void UpdateState(int _state);

	inline const char *GetName() { return name; }
	inline const CParameter *GetParam() { return param; }
};

#endif
