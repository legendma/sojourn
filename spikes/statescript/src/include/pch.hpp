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

inline Vec2 Add( const Vec2 &a, const Vec2 &b ) { return Vec2( a.v.x + b.v.x, a.v.y + b.v.y ); }
inline Vec2 Sub( const Vec2& a, const Vec2& b ) { return Vec2( a.v.x - b.v.x, a.v.y - b.v.y ); }

}; /* namespace math */
