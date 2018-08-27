#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <sal.h> // SAL runtime checking annotations
#include <atlbase.h>
#include <atldef.h>
#include <comdef.h>
#include <ppltasks.h>	// For create_task
//#include <wrl.h>
//#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
//#include <d2d1effects_2.h>
#include <dwrite_3.h>
//#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
//#include <memory>
//#include <agile.h>
//#include <concrt.h>
#include <sodium/include/sodium.h>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <map>
#include <array>
#include <queue>
#include <list>

#undef max
/* registry base path */
static const wchar_t *BASE_REGISTRY_PATH = L"SOFTWARE\\WOW6432Node\\Umbrellas Required\\Sojourn\\Main\\";
