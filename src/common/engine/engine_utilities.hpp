#pragma once

namespace Engine
{
    typedef enum
    {
        LOG_LEVEL_ERROR,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO
    } LogLevel;

    void ReportError( std::wstring message );
    void Log( const LogLevel level, std::wstring format, ... );
    void SetLogLevel( const LogLevel min_level );

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
            Log( LOG_LEVEL_ERROR, std::wstring( L":COM: " ).append( error_text ).c_str() );
            throw std::runtime_error( CharFromWideChar( error_text ) );
        }
    }

    inline uint64_t Random64()
    {
        return
        (
            ( (uint64_t) rand() <<  0 ) ^
            ( (uint64_t) rand() << 16 ) ^
            ( (uint64_t) rand() << 32 ) ^
            ( (uint64_t) rand() << 48 )
        );
    }


    template <typename fromT, typename toT>
    class TypeAlias
    {
    public:
        TypeAlias( fromT from ) { m_internal.from = from; }
        toT As() { return m_internal.to; }

    private:
        union
        {
            fromT from;
            toT   to;
        } m_internal;
    };
}