#pragma once
#include <type_traits>

class Utils
{
public:
    struct SourceLocation
    {
        const char *file_name;
        unsigned line_number;
        const char *function_name;
    };
#define CUR_SOURCE_LOCATION SourceLocation{__FILE__, __LINE__, __func__}

    static void _DebugAssert( bool expr, const SourceLocation& location, const char* expression )
    {
        if( !expr )
        {
            // handle failed assertion
            std::abort();
        }
    }

#ifdef DEBUG_ASSERT_ENABLED
#define DebugAssert( expr ) \
        Utils::_DebugAssert( expr, CUR_SOURCE_LOCATION, #expr )
#else
#define DebugAssert( expr )
#endif

    static std::wstring StringToWide( std::string to_convert );

    static inline void WindowsError( LPCTSTR calling_function )
    {
        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        DWORD dw = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
            (LPTSTR)&lpMsgBuf,
            0, NULL );

        // Display the error message and exit the process

        lpDisplayBuf = (LPVOID)LocalAlloc( LMEM_ZEROINIT,
            ( lstrlen( (LPCTSTR)lpMsgBuf ) + lstrlen( calling_function ) + 40 ) * sizeof( TCHAR ) );
        StringCchPrintf( (LPTSTR)lpDisplayBuf,
            LocalSize( lpDisplayBuf ) / sizeof( TCHAR ),
            TEXT( "%s failed with error %d: %s" ),
            calling_function, dw, lpMsgBuf );
        MessageBox( NULL, (LPCTSTR)lpDisplayBuf, TEXT( "Error" ), MB_OK );

        LocalFree( lpMsgBuf );
        LocalFree( lpDisplayBuf );
        ExitProcess( dw );
    }

    static inline std::string CharFromWideChar( std::wstring in )
    {
        std::string ret;
        if( in.empty() )
        {
            return ret;
        }

        auto required_chars = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, in.c_str(), (int)in.size(), nullptr, 0, nullptr, 0 );
        ret.reserve( required_chars );

        if( required_chars != WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, in.c_str(), (int)in.size(), ret.data(), (int)ret.size(), nullptr, 0 ) )
        {
            WindowsError( L"CharFromWideChar" );
        }

        return ret;
    }

    static inline void ComThrow( HRESULT hr )
    {
        if( FAILED( hr ) )
        {
            __debugbreak();
            _com_error error( hr );
            auto error_text = error.ErrorMessage();
            throw std::runtime_error( CharFromWideChar( error_text ) );
        }
    }
};

