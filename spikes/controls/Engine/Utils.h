#pragma once
class Utils
{
public:
    static std::wstring StringToWide( std::string to_convert );

    static inline std::string CharFromWideChar( std::wstring string )
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;

        //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
        return( converter.to_bytes( string ) );
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

