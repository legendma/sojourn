#pragma once
// // Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define __WRL_CLASSIC_COM_STRICT__ 
#define NOMINMAX
// Windows Header Files
#include <windows.h>
#include <strsafe.h>
#include <comdef.h>
#include <d3d11_3.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
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

#define SOJOURN_PAGE_SIZE 32768 // 32 kilobytes

#ifdef _DEBUG
#define DEBUG_ASSERT_ENABLED
#endif
