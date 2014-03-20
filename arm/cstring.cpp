#include <string.h>

#include "cstring.h"

CString::CString( int _size )
{
    str = 0;
    size = 0;

    if ( SetBufferSize( _size ) ) str[0] = '\0';
    length = 0;
}

CString::CString( const char *_str )
{
    str = 0;
    size = 0;

    *this = _str;
}

CString::CString( const CString &s )
{
    str = 0;
    size = 0;

    *this = s.str;
}

CString::~CString()
{
    if ( str ) delete str;
}

//-----------------------------------------------------------------------------------------

CString &CString::operator+=( const CString & _s )
{
    if ( str )
    {
		//portENTER_CRITICAL();

        int slen = _s.length;

        if ( SetBufferSize( length + slen + 1 ) )
        {
            strcpy( str + length, _s );
            length += slen;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::operator+=( const char * _str )
{
    if ( str && _str )
    {
		//portENTER_CRITICAL();

        int slen = strlen( _str );

        if ( SetBufferSize( length + slen + 1 ) )
        {
            strcpy( str + length, _str );
            length += slen;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::operator+=( char c )
{
    if ( c )
    {
		//portENTER_CRITICAL();

        if ( SetBufferSize( length + 2 ) )
        {
            str[ length++ ] = c;
            str[ length ] = 0;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::TrimRight( int i )
{
    if ( str )
    {
		//portENTER_CRITICAL();

        if ( i >= length )
        {
            length = 0;
            str[0] = '\0';
        }
        else if ( i > 0 )
        {
            length -= i;
            str[ length ] = '\0';
        }

		//portENTER_CRITICAL();
    }

    return *this;
}

CString &CString::TrimLeft( int i )
{
    if ( str )
    {
		//portENTER_CRITICAL();

        if ( i >= length )
        {
            length = 0;
            str[0] = '\0';
        }
        else if ( i > 0 )
        {
            length -= i;
            for ( int j = 0; j < length + 1; j++ )
                str[ j ] = str[ i++ ];
        }

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::Delete( int i, int num )
{
    if ( str )
    {
		//portENTER_CRITICAL();

        if ( i < 0 )
        {
            num += i;
            i = 0;
        }
        else if ( i > length )
        {
            num = 0;
            i = length;
        }

        if ( num > ( length - i ) ) num = length - i;

        if ( num > 0 )
        {
            length -= num;

            for ( int j = i; j <= length; j++ )
                str[ j ] = str[ j + num ];
        }

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::Insert( int i, const CString &_s )
{
    if ( str )
    {
		//portENTER_CRITICAL();

        if ( i < 0 ) i = 0;
        else if ( i > length ) i = length;

        int slen = _s.length;

        if ( SetBufferSize( length + slen + 1 ) )
        {
            for ( int j = length; j >= i; j-- )
                str[ j + slen ] = str[ j ];

            memcpy( &str[ i ], _s.str, slen );
            length += slen;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::Insert( int i, const char *_str )
{
    if ( str && _str )
    {
		//portENTER_CRITICAL();

        if ( i < 0 ) i = 0;
        else if ( i > length ) i = length;

        int slen = strlen( _str );

        if ( SetBufferSize( length + slen + 1 ) )
        {
            for ( int j = length; j >= i; j-- )
                str[ j + slen ] = str[ j ];

            memcpy( &str[ i ], _str, slen );
            length += slen;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}

CString &CString::Insert( int i, char c )
{
    if ( str && c )
    {
		//portENTER_CRITICAL();

        if ( i < 0 ) i = 0;
        else if ( i > length ) i = length;

        if ( SetBufferSize( length + 1 + 1 ) )
        {
            for ( int j = length; j >= i; j-- )
                str[ j + 1 ] = str[ j ];

            str[ i ] = c;
            length++;
        }
        else length = 0;

		//portEXIT_CRITICAL();
    }

    return *this;
}



