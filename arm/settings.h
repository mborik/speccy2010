#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include "types.h"
#include "cstring.h"
#include "fatfs/ff.h"

const int PTYPE_LIST = 0;
const int PTYPE_STRING = 1;
const int PTYPE_INT = 2;
const int PTYPE_FLOAT = 3;
const int PTYPE_END = 0xff;

class CParameter
{
    int type;
    const char *name;
    const char *options;
    void *data;

public:
    CParameter( int _type, const char *_name = 0, const char *_options = 0, void *_data = 0 )
    {
        type = _type;
        name = _name;
        options = _options;
        data = _data;
    }

    int GetType() const { return type; }
    const char *GetName() const { return name; }

    int GetValue() const;
    void SetValue( int _value ) const;

    void GetValueText( CString &text ) const;
    void SetValueText( const char *text ) const;

    int GetValueMin() const;
    int GetValueMax() const;
    int GetValueDelta() const;

    float GetValueFloat() const;
    void SetValueFloat( float _value ) const;

    float GetValueMinFloat() const;
    float GetValueMaxFloat() const;
    float GetValueDeltaFloat() const;
};

const CParameter *GetParam( const CParameter *iniParameters, const char *name );

class CSettingsFile
{
    FIL file;
    bool fileIsOk;
    CString currentGroupName;

public:
    CSettingsFile( const char *name = 0, bool createNew = false ) { Open( name, createNew ); }
    ~CSettingsFile() { Close(); }

    bool Open( const char *name, bool createNew = false );
    void Close();

    bool ReadLine( CString &groupName, CString &name, CString &value );
    bool WriteLine( const char *groupName, const char *name, const char *value );
};

#endif


