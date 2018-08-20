// pch.hpp : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#define NOMINMAX
#define SERVER

#include <SDKDDKVer.h>

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <memory>
#include <stdexcept>
#include <codecvt>
#include <comdef.h>
#include <ppltasks.h>	// For create_task
#include <array>
#include <map>
#include <queue>
#include <sodium/include/sodium.h>
