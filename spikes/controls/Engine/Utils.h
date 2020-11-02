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

    /* Sparse Set class */
    template <typename Key>
    class SparseSet
    {
    public:
        void Add( Key k )
        {
            DebugAssert( Contains( k ) );
            auto &page = GrowPage( ToPageIndex( k ) );
            page[ ToPageSubindex( k ) ] = Key( dense.size() );
            dense.push_back( k );
        }

        virtual void Clear()
        {
            dense.clear();
            sparse.clear();
        }

        bool Contains( Key k ) const
        {
            auto page_index = ToPageIndex( k );

            return page_index < sparse.size()
                && sparse[ page_index ] != nullptr
                && sparse[ page_index ][ ToPageSubindex( k ) ] != Key::null_value;
        }

        size_t Get( Key k ) const
        {
            auto &page = sparse.at( ToPageIndex( k ) );
            return page.at( ToPageSubindex( k ) );
        }

        void Remove( Key k )
        {
            DebugAssert( Contains( k ) );
            auto &remove_page = GrowPage( ToPageIndex( k ) );
            auto &remove_dense_index = remove_page[ ToPageSubindex( k ) ];

            auto swap_value = dense.back();
            auto &swap_page = GrowPage( ToPageIndex( swap_value ) );

            dense[ remove_dense_index ] = swap_value;
            swap_page[ ToPageSubindex( swap_value ) ] = remove_dense_index;
            remove_dense_index = Key::null_value;
            dense.pop_back();
        }

        size_t Size()
        {
            return dense.size();
        }
        
    private:
        static constexpr auto indices_per_page = SOJOURN_PAGE_SIZE / sizeof( Key );
        using Page = std::unique_ptr<std::array<Key, indices_per_page>>;

        std::vector<Key> dense;
        std::vector<Page> sparse;

    private:
        size_t ToPageIndex( Key k ) const
        {
            return size_t( k ) / indices_per_page;
        }

        size_t ToPageSubindex( Key k ) const
        {
            return k & ( indices_per_page - 1 );
        }

        Page & GrowPage( std::size_t page_index )
        {
            if( !( sparse.size() > page_index ) )
            {
                sparse.resize( page_index + 1 );
            }

            if( sparse[ page_index ] == nullptr )
            {
                sparse[ page_index ].reset( new std::array<Key, indices_per_page> );
                for( auto &value : sparse[ page_index ] )
                {
                    value = Key::null_value;
                }
            }

            return sparse[ page_index ];
        }
    };

    template <typename Key, typename Type>
    class SparseMap : public SparseSet<Key>
    {
    using base_class = SparseSet<Key>;
    public:
        template<typename... Args>
        Type & Add( const Key k, Args &&... args )
        {
            if constexpr( std::is_aggregate_v<Type> )
            {
                values.push_back( Type{ std::forward<Args>( args )... } );
            }
            else
            {
                values.emplace_back( std::forward<Args>( args )... );
            }

            base_class::Add( k );
            return values.back();
        }

        virtual void Clear() override
        {
            base_class::Clear();
            values.clear();
        }

        void Remove( Key k )
        {
            auto swap_value = std::move( values.back() );
            values[ base_class::Get( k ) ] = std::move( swap_value );
            base_class::Remove( k );
            values.pop_back();
        }

    private:
        std::vector<Type> values;
    };
};

