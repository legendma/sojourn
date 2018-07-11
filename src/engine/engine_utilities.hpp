#pragma once

namespace Engine
{
    void ReportError( std::wstring message );
    void Log( const wchar_t * format, ... );

    inline std::wstring WideCharFromChar( std::string string )
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;

        //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
        return(converter.from_bytes( string ));
    }

    inline std::string CharFromWideChar( std::wstring string )
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;

        //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
        return(converter.to_bytes( string ));
    }

    inline void ComThrow( HRESULT hr )
    {
        if( FAILED( hr ) )
        {
            // Set a breakpoint on this line to catch Win32 API errors.
           // AtlThrow( hr );
            //Platform::Exception::CreateException(hr);
            _com_error error( hr );
            auto error_text = error.ErrorMessage();
            Log( std::wstring( L"COM Error: " ).append( error_text ).c_str() );
            throw std::runtime_error( CharFromWideChar( error_text ) );
        }
    }

}