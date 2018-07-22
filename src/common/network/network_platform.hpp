#pragma once

/* define platform endianness */
#define LITTLE_ENDIAN                    ( 0 )
#define BIG_ENDIAN                       ( 1 )

#define NETWORK_ENDIANNESS               LITTLE_ENDIAN

static int get_platform_endian()
{
    union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };

    if( bint.c[0] == 1 )
    {
        return BIG_ENDIAN;
    }

    return LITTLE_ENDIAN;
}

#define PLATFORM_ENDIANNESS get_platform_endian()

#include "common/engine/engine_utilities.hpp"

namespace Engine
{
    inline static uint16_t SwapBytes16( const uint16_t value )
    {
        return( ( value << 8 )
              | ( value >> 8 ) );
    }

    inline static uint32_t SwapBytes32( const uint32_t value )
    {
        return(   ( value                << 24 )
              | ( ( value & 0x0000ff00 ) << 8  )
              | ( ( value & 0x00ff0000 ) >> 8  )
              |   ( value                >> 24 ) );
    }

    inline static uint64_t SwapBytes64( const uint64_t value )
    {
        return(   ( value                        << 56 )
              | ( ( value & 0x000000000000ff00 ) << 40 )
              | ( ( value & 0x0000000000ff0000 ) << 24 )
              | ( ( value & 0x00000000ff000000 ) << 8  )
              | ( ( value & 0x000000ff00000000 ) >> 8  )
              | ( ( value & 0x0000ff0000000000 ) >> 24 )
              | ( ( value & 0x00ff000000000000 ) >> 40 )
              |   ( value                        >> 56 ) );
    }

    template <typename T, size_t size> class ByteSwapper;

    template <typename T>
    class ByteSwapper<T, 1>
    {
    public:
        static T Swap( T &input )
        {
            return input;
        }
    };

    template <typename T>
    class ByteSwapper<T, 2>
    {
    public:
        static T Swap( T &input )
        {
            if( PLATFORM_ENDIANNESS == NETWORK_ENDIANNESS )
            {
                return input;
            }

            /* do a two byte swap on a 16 bit value */
            return( TypeAlias<uint16_t, T>( SwapBytes16( TypeAlias<T, uint16_t>( input ).As() ) ).As() );
        }
    };

    template <typename T>
    class ByteSwapper<T, 4>
    {
    public:
        static T Swap( T &input )
        {
            if( PLATFORM_ENDIANNESS == NETWORK_ENDIANNESS )
            {
                return input;
            }

            /* do a four byte swap on a 32 bit value */
            return( TypeAlias<uint32_t, T>( SwapBytes32( TypeAlias<T, uint32_t>( input ).As() ) ).As() );
        }
    };
    
    template <typename T>
    class ByteSwapper<T, 8>
    {
    public:
        static T Swap( T &input )
        {
            if( PLATFORM_ENDIANNESS == NETWORK_ENDIANNESS )
            {
                return input;
            }

            /* do a eight byte swap on a 64 bit value */
            return( TypeAlias<uint64_t, T>( SwapBytes64( TypeAlias<T, uint64_t>( input ).As() ) ).As() );
        }
    };

    template <typename T>
    T ByteSwap( T input )
    {
        return( ByteSwapper<T, sizeof(T)>::Swap( input ) );
    }

}
