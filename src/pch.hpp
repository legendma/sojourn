#pragma once

#include <sal.h> // SAL runtime checking annotations
#include <atlbase.h>
#include <atldef.h>
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
#include <windows.h>
#include <fstream>
#include <iostream>
//#include <memory>
//#include <agile.h>
//#include <concrt.h>
#include <map>

#undef max
/* registry base path */
static const wchar_t *BASE_REGISTRY_PATH = L"SOFTWARE\\WOW6432Node\\Umbrellas Required\\Sojourn\\Main\\";

#define cnt_of_array( _arr ) \
    ( sizeof( _arr ) / sizeof( _arr[0] ) )