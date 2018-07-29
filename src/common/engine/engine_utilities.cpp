#include "pch.hpp"
#include "engine_utilities.hpp"

static Engine::LogLevel s_log_level = Engine::LOG_LEVEL_ERROR;

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
        Log( Engine::LOG_LEVEL_ERROR, L"%s: %d- %s", message, errorNum, errorText );
        LocalFree( errorText );
        errorText = NULL;
    }
}

void Engine::Log( const LogLevel level, const wchar_t* format, ... )
{
    if( level < s_log_level )
    {
        return;
    }

    std::wstring str;
    if( level == LOG_LEVEL_ERROR )
    {
        str = std::wstring( L"Error " ).append( format );
    }
    else if( level == LOG_LEVEL_WARNING )
    {
        str = std::wstring( L"Warning " ).append( format );
    }
    else if( level == LOG_LEVEL_DEBUG )
    {
        str = std::wstring( L"Debug " ).append( format );
    }
    else
    {
        str = std::wstring( L"Info " ).append( format );
    }

    static wchar_t buffer[4096];

    va_list args;
    va_start( args, format );

    _vsnwprintf_s( buffer, 4096, 4096, str.c_str(), args );

    OutputDebugString( buffer );
    OutputDebugString( L"\n" );
}

void Engine::SetLogLevel( const Engine::LogLevel min_level )
{
    s_log_level = min_level;
}