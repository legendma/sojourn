#include "pch.hpp"
#include "engine_utilities.hpp"

static Engine::LogLevel s_log_level = Engine::LOG_LEVEL_INFO;

void Engine::ReportWindowsError( std::wstring message )
{
    LPTSTR error_text = NULL;
    DWORD error_num = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_num,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPTSTR)&error_text,
        0, NULL );

    if( error_text )
    {
        Log( Engine::LOG_LEVEL_ERROR, L"%s: %d- %s", message.c_str(), error_num, error_text );
        LocalFree( error_text );
        error_text = NULL;
    }
}

void Engine::ReportWinsockError( std::wstring message, int given_error /*= 0*/ )
{   
    int error_num = given_error;
    if( given_error == NO_ERROR )
    {
    error_num = WSAGetLastError();
    }

    LPTSTR error_text = NULL;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_num,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPTSTR)&error_text,
        0, NULL );

    if( error_text )
    {
        Log( Engine::LOG_LEVEL_ERROR, L"%s: %d- %s", message.c_str(), error_num, error_text );
        LocalFree( error_text );
        error_text = NULL;
    }
}

void Engine::Log( const LogLevel level, std::wstring format, ... )
{
    if( level < s_log_level )
    {
        return;
    }

    if( level == LOG_LEVEL_ERROR )
    {
        format = std::wstring( L"[Error] " ).append( format );
    }
    else if( level == LOG_LEVEL_WARNING )
    {
        format = std::wstring( L"[Warning] " ).append( format );
    }
    else if( level == LOG_LEVEL_DEBUG )
    {
        format = std::wstring( L"[Debug] " ).append( format );
    }
    else
    {
        format = std::wstring( L"[Info] " ).append( format );
    }

    format.append( L"\n" );
    
    static wchar_t buffer[4096];
    va_list args;
    va_start( args, format );

    _vsnwprintf_s( buffer, 4096, 4096, format.c_str(), args );

    OutputDebugString( buffer );

#if defined SERVER
    wprintf( buffer );
#endif
}

void Engine::SetLogLevel( const Engine::LogLevel min_level )
{
    s_log_level = min_level;
}