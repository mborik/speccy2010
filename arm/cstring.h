#ifndef STRING_H_INCLUDED
#define STRING_H_INCLUDED

#include "types.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

const int CSTRING_DEF_MIN_SIZE = 0x10;

class CString
{
    char *str;

    int size;
    int length;

public:
    CString( int _size = CSTRING_DEF_MIN_SIZE );
    CString( const char *_str );
    CString( const CString &s );

    ~CString();

    //-----------------------------------------------------------------------------------------

    CString &operator=( const CString &s );
    CString &operator=( const char *_str );

    CString &Set( const char *_str, unsigned int maxSize = 0x100 );
    CString &Format( const char *_str, ... );

    CString &operator+=( const CString &_s );
    CString &operator+=( const char *_str );
    CString &operator+=( char c );

    CString &TrimRight( int i );
    CString &TrimLeft( int i );

    CString &Delete( int i, int num );

    CString &Insert( int i, const CString &_s );
    CString &Insert( int i, const char *_str );
    CString &Insert( int i, char c );

    //-----------------------------------------------------------------------------------------

    bool operator==( const CString &s );
    bool operator==( const char *_str );

    bool operator!=( const CString &s );
    bool operator!=( const char *_str );

    //-----------------------------------------------------------------------------------------

    operator const char*() const;

    void SetSymbol( int position, char symbol );
    char GetSymbol( int position ) const;

    const char *String() const;
    int Length() const;

    //-----------------------------------------------------------------------------------------

    int SetBufferSize( int _size );
    int GetBufferSize() const;
};

//-----------------------------------------------------------------------------------------

//#include "system.h"

inline CString &CString::operator=( const CString & s )
{
    *this = s.str;
    return *this;
}

inline CString &CString::operator=( const char * _str )
{
	//portENTER_CRITICAL();

    if ( _str )
    {
        if( _str != str )
        {
            length = strlen( _str );

            if ( SetBufferSize( length + 1 ) ) strcpy( str, _str );
            else length = 0;
        }
    }
    else
    {
        length = 0;
        if ( SetBufferSize( length + 1 ) ) str[0] = '\0';
    }

	//portEXIT_CRITICAL();

    return *this;
}

inline CString &CString::Set( const char *_str, unsigned int maxSize )
{
	//portENTER_CRITICAL();

    if ( _str )
    {
        if( _str != str )
        {
            length = min( strlen( _str ), maxSize );

            if ( SetBufferSize( length + 1 ) ) sniprintf( str, length + 1, "%s", _str );
            else length = 0;
        }
    }
    else
    {
        length = 0;
        if ( SetBufferSize( length + 1 ) ) str[0] = '\0';
    }

	//portEXIT_CRITICAL();

    return *this;
}

inline CString &CString::Format( const char *format, ... )
{
    va_list ap;
    va_start( ap, format );

	//portENTER_CRITICAL();

    while( true )
    {
        length = vsniprintf( str, GetBufferSize(), format, ap );

        if( length == -1 ) length = 0;
        if( length < GetBufferSize() ) break;

        if ( !SetBufferSize( length + 1 ) )
        {
            length = 0;
            break;
        }
    }

	//portEXIT_CRITICAL();

    va_end(ap);
    return *this;
}

//-----------------------------------------------------------------------------------------

inline bool CString::operator==( const CString &s )
{
    if( str && s.str ) return strcmp( str, s.str ) == 0;
    return false;
}

inline bool CString::operator==( const char *_str )
{
    if( str && _str ) return strcmp( str, _str ) == 0;
    return false;
}

inline bool CString::operator!=( const CString &s )
{
    if( str && s.str ) return strcmp( str, s.str ) != 0;
    return true;
}

inline bool CString::operator!=( const char *_str )
{
    if( str && _str ) return strcmp( str, _str ) != 0;
    return true;
}


//-----------------------------------------------------------------------------------------

inline CString::operator const char*() const
{
    if ( str ) return str;
    else return "";
}

inline const char *CString::String() const
{
    if ( str ) return str;
    else return "";
}

inline int CString::Length() const
{
    return length;
}

//-----------------------------------------------------------------------------------------

inline void CString::SetSymbol( int position, char symbol )
{
    if ( str && position < length )
    {
        str[position] = symbol;
    }
}

inline char CString::GetSymbol( int position ) const
{
    if ( str && position < length )
    {
        return str[position];
    }

    return 0;
}


//-----------------------------------------------------------------------------------------

inline int CString::GetBufferSize() const
{
    return size;
}

inline int CString::SetBufferSize( int _size )
{
	//portENTER_CRITICAL();

    if ( _size < CSTRING_DEF_MIN_SIZE ) _size = CSTRING_DEF_MIN_SIZE;

    if ( _size > size )
    {
        _size = ( _size + CSTRING_DEF_MIN_SIZE - 1 );
        _size = _size - _size % CSTRING_DEF_MIN_SIZE;

        char *_str = new char[ _size ];

        if ( str )
        {
            if ( _str ) memcpy( _str, str, size );
            delete str;
        }

        str = _str;


        if ( str ) size = _size;
        else size = 0;
    }

	//portEXIT_CRITICAL();

    return size;
}

#endif
