#pragma once

#include "engine_math.hpp"

namespace Engine
{
    enum class HexType
    {
        LOCATION,
        DIFFERNCE
    };

    template<typename T, HexType VARIANT>
    struct _Hex
    {
        _Hex( int _q, int _r, int _s ) : q( _q ), r( _r ), s( _s ) {}
        _Hex( int _q, int _r ) : q( _q ), r( _r ), s( -_r - _s ) {}

        bool operator ==( _Hex &other ) { return q == other.q && r == other.r && s == other.s; }
        bool operator !=( _Hex &other ) { return !( *this == other ); }

        union
        {
            T v[ 3 ];
            struct { T q; T r; T s; };
        };
    };

    typedef _Hex<int, LOCATION> Hex;
    typedef _Hex< int, DIFFERENCE> HexDifference;
    typedef _Hex<double, LOCATION> HexReal;
    typedef _Hex<double, DIFFERENCE> HexRealDifference;
}