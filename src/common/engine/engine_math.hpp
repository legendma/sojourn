#pragma once

namespace Engine
{
    struct vec2
    {
    vec2() : x( 0.0f ), y( 0.0f ) {};
    vec2( float _x, float _y ) : x( _x ), y( _y ) {};

    bool operator == ( vec2 &other ) const { return( ( x == other.x ) && ( y == other.y ) ) ; }
    bool operator != ( vec2 &other ) const { return( !( *this == other ) ); }

    float x;
    float y;
    };
}