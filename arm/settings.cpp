#include "settings.h"
#include "system.h"
#include <string.h>

void IntToStr( CString &str, int number )
{
    str.Format( "%d", number );
}

int StrToInt( const char *str, int base = 10 )
{
    int len = strlen( str );

    int result = 0;
    int sign = 1;

    if( len > 1 && str[0] == '-' )
    {
        sign = -1;
        str += 1;
    }

    if( len > 2 && str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) )
    {
        base = 16;
        str += 2;
    }

    while( *str != 0 )
    {
        if( *str >= '0' && *str <= '9' )
        {
            result *= base;
            result += *str - '0';
        }
        else if( base == 16 && *str >= 'a' && *str <= 'f' )
        {
            result *= base;
            result += 10 + ( *str - 'a' );
        }
        else if( base == 16 && *str >= 'A' && *str <= 'F' )
        {
            result *= base;
            result += 10 + ( *str - 'A' );
        }
        else break;

        str++;
    }

    return sign * result;
}

int GetSubstringsCount( const char *str )
{
    int result = 0;

    while( *str != 0 )
    {
        if( *str++ == '|' ) result++;
    }

    return result + 1;
}

const char *FindSubstring( const char *str, int pos )
{
    while( *str != 0 && pos > 0 )
    {
        if( *str++ == '|' ) pos--;
    }

    return str;
}

int GetSubstringLength( const char *str )
{
    int length = 0;

    while( *str != 0 && *str != '|' )
    {
        length++;
        str++;
    }

    return length;
}

int CopySubstring( char *buff, int buffSize, const char *str )
{
    if( buffSize <= 0 ) return 0;

    int length = 0;

    while( *str != 0 && *str != '|' && buffSize > 1 )
    {
        *buff++ = *str++;
        length++;
        buffSize--;
    }

    *buff = 0;

    return length;
}

int CParameter::GetValue() const
{
    if( type == PTYPE_LIST || type == PTYPE_INT )
    {
        if( data != 0 ) return *(int*) data;
    }

    return 0;
}

void CParameter::SetValue( int _value ) const
{
    if( type == PTYPE_LIST || type == PTYPE_INT )
    {
        if( data != 0 ) *(int*) data = _value;
    }
}

void CParameter::GetValueText( CString &text ) const
{
    text = "";

    if( type == PTYPE_LIST )
    {
        if( data != 0 && options != 0 )
        {
            const char *str = FindSubstring( options, *(int*) data );
            text.Set( str, GetSubstringLength( str ) );
        }
    }
    else if( type == PTYPE_INT )
    {
        if( data != 0 )
        {
            IntToStr( text, *(int*) data );
        }
    }
}

void CParameter::SetValueText( const char *text ) const
{
    if( type == PTYPE_LIST )
    {
        if( data != 0 && options != 0 )
        {
            *(int*) data = 0;
            int textLen = strlen( text );

            for( int i = 0; i < GetSubstringsCount( options ); i++ )
            {
                const char *str = FindSubstring( options, i );
                int strLen = GetSubstringLength( str );
                if( strLen == textLen && strncmp( str, text, strLen ) == 0 )
                {
                    *(int*) data = i;
                    break;
                }
            }
        }
    }
    else if( type == PTYPE_INT )
    {
        if( data != 0 )
        {
            *(int*) data = StrToInt( text );
        }
    }
}

int CParameter::GetValueMin() const
{
    if( data == 0 || options == 0 ) return 0;

    if( type == PTYPE_LIST )
    {
        return 0;
    }
    else if( type == PTYPE_INT )
    {
        char buff[16];
        CopySubstring( buff, sizeof( buff ), FindSubstring( options, 0 ) );
        return StrToInt( buff );
    }

    return 0;
}

int CParameter::GetValueMax() const
{
    if( data == 0 || options == 0 ) return 0;

    if( type == PTYPE_LIST )
    {
        return GetSubstringsCount( options ) - 1;
    }
    else if( type == PTYPE_INT )
    {
        char buff[16];
        CopySubstring( buff, sizeof( buff ), FindSubstring( options, 1 ) );
        return StrToInt( buff );
    }

    return 0;
}

int CParameter::GetValueDelta() const
{
    if( data == 0 || options == 0 ) return 1;

    if( type == PTYPE_LIST )
    {
        return 1;
    }
    else if( type == PTYPE_INT )
    {
        char buff[16];
        CopySubstring( buff, sizeof( buff ), FindSubstring( options, 2 ) );
        return StrToInt( buff );
    }

    return 1;
}

bool CSettingsFile::Open( const char *name, bool createNew )
{
    currentGroupName = "";

    int result;

    if( createNew ) result = f_open( &file, name, FA_WRITE | FA_CREATE_ALWAYS );
    else result = f_open( &file, name, FA_READ | FA_OPEN_EXISTING );

    //char str[0x10];
    //snprintf( str, sizeof( str ), "result - %d\n", result );
    //UART0_WriteText( str );

    fileIsOk = ( result == FR_OK );
    return fileIsOk;
}

void CSettingsFile::Close()
{
    if( fileIsOk ) f_close( &file );
    fileIsOk = false;
}

bool CSettingsFile::ReadLine( CString &groupName, CString &name, CString &value )
{
    if( !fileIsOk ) return false;

    CString line;

    while( ::ReadLine( &file, line ) )
    {
        const char *a, *b, *c;

        a = strchr( line, '[' );
        b = strchr( line, ']' );

        if( a && b && a < b )
        {
            a++; b--;
            currentGroupName.Set( a, b - a + 1 );
            continue;
        }

        c = strchr( line, '=' );
        if( c )
        {
            groupName = currentGroupName;

            a = line;
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\"' ) a++;

            b = c - 1;
            while( b >= a && ( *b == ' ' || *b == '\t' ) ) b--;
            if( b >= a && *b == '\"' ) b--;

            name.Set( a, b - a + 1 );

            a = c + 1;
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\"' ) a++;

            b = a + strlen( a ) - 1;
            while( b >= a && ( *b == ' ' || *b == '\t' ) ) b--;
            if( b >= a && *b == '\"' ) b--;

            value.Set( a, b - a + 1 );

            return true;
        }
    }

    return false;
}

bool CSettingsFile::WriteLine( const char *groupName, const char *name, const char *value )
{
    if( !fileIsOk ) return false;

    bool result = true;

    if( currentGroupName != groupName )
    {
        if( file.fptr != 0 ) result = result && ::WriteLine( &file, "" );

        result = result && ::WriteText( &file, "[" );
        result = result && ::WriteText( &file, groupName );
        result = result && ::WriteLine( &file, "]" );

        currentGroupName = groupName;
    }

    result = result && ::WriteText( &file, name );
    result = result && ::WriteText( &file, " = " );
    result = result && ::WriteLine( &file, value );

    return result;
}

const CParameter *GetParam( const CParameter *params, const char *name )
{
    while( params->GetType() != PTYPE_END )
    {
        if( strcmp( params->GetName(), name ) == 0 ) return params;
        params++;
    }

    return 0;
}
