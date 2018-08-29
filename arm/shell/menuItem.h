#ifndef SHELL_MENUITEM_H_INCLUDED
#define SHELL_MENUITEM_H_INCLUDED

#include "../settings.h"
#include "../utils/cstring.h"

class CMenuItem {
private:
	int x, y, state, size, origY, maxLen;
	const char *origName;
	byte colors[4];

	CString data;
	const CParameter *param;

	void Init(int _x, int _y, const char *_name, const CParameter *_param);
	void Redraw(bool justInit = false);

public:
	CMenuItem(int _x, int _y, const char *_name, const CParameter *_param);
	CMenuItem(int _x, int _y, const char *_name, const char *_data);

	void UpdateData(bool force = false);
	void UpdateData(const char *_data);
	void UpdateState(int _state);

	inline const char *GetName() { return origName; }
	inline const CParameter *GetParam() { return param; }
};

#endif
