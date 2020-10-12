#pragma once
// // Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define __WRL_CLASSIC_COM_STRICT__ 
// Windows Header Files
#include <windows.h>
#include <comdef.h>
#include <d3d11.h>
#include <wrl/client.h>
// C RunTime Header Files
#include <stdlib.h>
#include <codecvt>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cstdint>
#include <string>
#include <queue>
#include <bitset>
#include <list>

using Microsoft::WRL::ComPtr;

inline void DebugAssert( bool check )
{
    if( !check )
    {
        __debugbreak();
    }
}