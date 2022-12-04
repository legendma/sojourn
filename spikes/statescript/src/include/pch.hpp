#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#include <array>
#include <deque>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
namespace gui = ImGui;

#undef max
#undef min

#if !defined( FALSE )
#define FALSE 0
#endif
#if !defined( TRUE )
#define TRUE 1
#endif

#define BUILD_WIN32 1

#define CURRENT_BUILD BUILD_WIN32

#if( CURRENT_BUILD == BUILD_WIN32 )
#define ARE_BUILDING_WIN32
#endif

namespace math
{
union Vec2
{
    Vec2() { this->v.x = 0.0f; this->v.y = 0.0f; }
    Vec2( float x, float y ) { this->v.x = x; this->v.y = y; }

    static inline Vec2 Add( const Vec2 &a, const Vec2 &b ) { return Vec2( a.v.x + b.v.x, a.v.y + b.v.y ); }
    static inline Vec2 Sub( const Vec2 &a, const Vec2 &b ) { return Vec2( a.v.x - b.v.x, a.v.y - b.v.y ); }
    static inline Vec2 Scale( const float scalar, const Vec2 &vec ) { return Vec2( scalar * vec.v.x, scalar * vec.v.y ); }
    static inline Vec2 Scale( const float x, const float y, const Vec2& vec ) { return Vec2( x * vec.v.x, y * vec.v.y ); }
    static inline float Magnitude( const Vec2 &vec ) { return std::sqrtf( vec.v.x * vec.v.x + vec.v.y * vec.v.y ); }

    struct
    {
        float x;
        float y;
    } v;
    float p[ 2 ];
};
using Pos2 = Vec2;

union Vec3
{
    Vec3() { this->v.x = 0.0f; this->v.y = 0.0f; this->v.z = 0.0f; }
    Vec3( float x, float y, float z ) { this->v.x = x; this->v.y = y; this->v.z = z; }
    struct
    {
        float x;
        float y;
        float z;
    } v;
    float p[3];
};
using Pos3 = Vec3;

union Vec4
{
    Vec4() { this->v.x = 0.0f; this->v.y = 0.0f; this->v.z = 0.0f; this->v.w = 0.0f; }
    Vec4( float x, float y, float z, float w ) { this->v.x = x; this->v.y = y; this->v.z = z; this->v.w = w; }
    struct
    {
        float x;
        float y;
        float z;
        float w;
    } v;
    float p[4];
};
using Pos4 = Vec4;


struct ABB2
{
    ABB2() { this->position = Pos2( std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN() ); this->extent = Vec2( 0.0f, 0.0f ); }
    ABB2( Pos2 position, Vec2 extent ) { this->position = position; this->extent = extent; }

    static inline boolean IsValid( const ABB2 &abb ) { return !std::isnan( abb.position.v.x ) && !std::isnan( abb.position.v.y ); }

    static inline Pos2 BottomRight( const ABB2 &abb )
        {
        if( std::isnan( abb.position.v.x ) 
         || std::isnan( abb.position.v.y ) )
            return abb.position;

        return Vec2::Add( abb.position, abb.extent );
        }

    static inline ABB2 Union( const ABB2 &a, const ABB2 &b )
    {
        if( !IsValid( a ) ) return b;
        if( !IsValid( b ) ) return a;

        auto ret = ABB2( a.position, a.extent );
        if( b.position.v.x < ret.position.v.x )
        {
            ret.extent.v.x += ret.position.v.x - b.position.v.x;
            ret.position.v.x = b.position.v.x;
        }

        if( b.position.v.y < ret.position.v.y )
        {
            ret.extent.v.y += ret.position.v.y - b.position.v.y;
            ret.position.v.y = b.position.v.y;
        }

        ret.extent.v.x = std::max( ret.extent.v.x, b.position.v.x - ret.position.v.x + b.extent.v.x );
        ret.extent.v.y = std::max( ret.extent.v.y, b.position.v.y - ret.position.v.y + b.extent.v.y );

        return ret;
    }

    static inline Vec2 Sub( const Vec2& a, const Vec2& b ) { return Vec2( a.v.x - b.v.x, a.v.y - b.v.y ); }

    // interval is [ pos, pos + extent )
    Pos2 position;
    Vec2 extent;
};



}; /* namespace math */
