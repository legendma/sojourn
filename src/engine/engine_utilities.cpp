#include "pch.hpp"
#include "engine\engine_utilities.hpp"

void Engine::ReportError( std::wstring message )
{
    LPTSTR errorText = NULL;
    DWORD errorNum = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorNum,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPTSTR)&errorText,
        0, NULL );

    if( errorText )
    {
        Log( L"Error %s: %d- %s", message, errorNum, errorText );
        LocalFree( errorText );
        errorText = NULL;
    }
}

void Engine::Log( const wchar_t* format, ... )
{
    static wchar_t buffer[4096];

    va_list args;
    va_start( args, format );

    _vsnwprintf_s( buffer, 4096, 4096, format, args );

    OutputDebugString( buffer );
    OutputDebugString( L"\n" );
}
