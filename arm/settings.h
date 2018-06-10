#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include "fatfs/ff.h"
#include "types.h"
#include "utils/cstring.h"

enum CParameterType { PTYPE_LIST = 0, PTYPE_STRING, PTYPE_INT, PTYPE_FLOAT, PTYPE_END = 0xff };

class CParameter {
	int type;
	int group;
	const char *name;
	const char *options;
	void *data;

public:
	CParameter(int _type, int _group = 0, const char *_name = NULL, const char *_options = 0, void *_data = 0)
	{
		type = _type;
		group = _group;
		name = _name;
		options = _options;
		data = _data;
	}

	int GetType() const { return type; }
	int GetGroup() const { return group; }
	const char *GetName() const { return name; }

	int GetValue() const;
	void SetValue(int _value) const;

	void GetValueText(CString &text) const;
	void SetValueText(const char *text) const;

	int GetValueMin() const;
	int GetValueMax() const;
	int GetValueDelta() const;

	float GetValueFloat() const;
	void SetValueFloat(float _value) const;

	float GetValueMinFloat() const;
	float GetValueMaxFloat() const;
	float GetValueDeltaFloat() const;
};

const CParameter *GetParam(const CParameter *iniParameters, const char *name, int group = 0);

class CSettingsFile {
	FIL file;
	bool fileIsOk;
	CString currentGroupName;

public:
	CSettingsFile(const char *name, bool createNew = false) { Open(name, createNew); }
	~CSettingsFile() { Close(); }

	bool Open(const char *name, bool createNew = false);
	void Close();

	bool ReadLine(CString &groupName, CString &name, CString &value);
	bool WriteLine(const char *groupName, const char *name, const char *value);
};

#endif
