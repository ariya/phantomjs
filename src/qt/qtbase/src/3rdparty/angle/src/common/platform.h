//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// platform.h: Operating system specific includes and defines.

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

#if defined(_WIN32) || defined(_WIN64)
#   define ANGLE_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#   define ANGLE_PLATFORM_APPLE 1
#   define ANGLE_PLATFORM_POSIX 1
#elif defined(__linux__)
#   define ANGLE_PLATFORM_LINUX 1
#   define ANGLE_PLATFORM_POSIX 1
#elif defined(ANDROID)
#   define ANGLE_PLATFORM_ANDROID 1
#   define ANGLE_PLATFORM_POSIX 1
#elif defined(__FreeBSD__) || \
      defined(__OpenBSD__) || \
      defined(__NetBSD__) || \
      defined(__DragonFly__) || \
      defined(__sun) || \
      defined(__GLIBC__) || \
      defined(__GNU__) || \
      defined(__QNX__)
#   define ANGLE_PLATFORM_POSIX 1
#else
#   error Unsupported platform.
#endif

#ifdef ANGLE_PLATFORM_WINDOWS
#   if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP || WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#       define ANGLE_ENABLE_WINDOWS_STORE 1
#   endif
#   ifndef STRICT
#       define STRICT 1
#   endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN 1
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX 1
#   endif

#   include <windows.h>
#   include <intrin.h>

#   if defined(ANGLE_ENABLE_D3D9)
#       include <d3d9.h>
#       include <dxgi.h>
#      if !defined(COMPILER_IMPLEMENTATION)
#       include <d3dcompiler.h>
#      endif
#   endif

#   if defined(ANGLE_ENABLE_D3D11)
#       include <d3d10_1.h>
#       include <d3d10.h>
#       include <d3d11.h>
#       include <dxgi.h>
#      if defined(_MSC_VER) && (_MSC_VER >= 1700)
#       include <d3d11_1.h>
#       include <dxgi1_2.h>
#      endif
#      if !defined(COMPILER_IMPLEMENTATION)
#       include <d3dcompiler.h>
#      endif
#   endif

#   if defined(ANGLE_ENABLE_WINDOWS_STORE)
#       include <dxgi1_3.h>
#       if defined(_DEBUG)
#          if (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
#           include <DXProgrammableCapture.h>
#          endif
#           include <dxgidebug.h>
#       endif
#   endif

#  if defined(__MINGW32__) // Missing defines on MinGW
typedef enum D3D11_MAP_FLAG
{
    D3D11_MAP_FLAG_DO_NOT_WAIT = 0x100000L
} D3D11_MAP_FLAG;
typedef struct D3D11_QUERY_DATA_SO_STATISTICS
{
    UINT64 NumPrimitivesWritten;
    UINT64 PrimitivesStorageNeeded;
} D3D11_QUERY_DATA_SO_STATISTICS;
typedef HRESULT (WINAPI *PFN_D3D11_CREATE_DEVICE)(
        IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, CONST D3D_FEATURE_LEVEL *,
        UINT FeatureLevels, UINT, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
#define D3D11_MESSAGE_CATEGORY UINT
#define D3D11_MESSAGE_SEVERITY UINT
#define D3D11_MESSAGE_ID UINT
struct D3D11_MESSAGE;
typedef struct D3D11_INFO_QUEUE_FILTER_DESC
{
    UINT NumCategories;
    D3D11_MESSAGE_CATEGORY *pCategoryList;
    UINT NumSeverities;
    D3D11_MESSAGE_SEVERITY *pSeverityList;
    UINT NumIDs;
    D3D11_MESSAGE_ID *pIDList;
} D3D11_INFO_QUEUE_FILTER_DESC;
typedef struct D3D11_INFO_QUEUE_FILTER
{
    D3D11_INFO_QUEUE_FILTER_DESC AllowList;
    D3D11_INFO_QUEUE_FILTER_DESC DenyList;
} D3D11_INFO_QUEUE_FILTER;
static const IID IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, 0xab, 0x82, 0xe9, 0x7e, 0xc7, 0x43, 0x26, 0xf6 };
MIDL_INTERFACE("6543dbb6-1b48-42f5-ab82-e97ec74326f6") ID3D11InfoQueue : public IUnknown
{
public:
    virtual HRESULT __stdcall SetMessageCountLimit(UINT64) = 0;
    virtual void __stdcall ClearStoredMessages() = 0;
    virtual HRESULT __stdcall GetMessage(UINT64, D3D11_MESSAGE *, SIZE_T *) = 0;
    virtual UINT64 __stdcall GetNumMessagesAllowedByStorageFilter() = 0;
    virtual UINT64 __stdcall GetNumMessagesDeniedByStorageFilter() = 0;
    virtual UINT64 __stdcall GetNumStoredMessages() = 0;
    virtual UINT64 __stdcall GetNumStoredMessagesAllowedByRetrievalFilter() = 0;
    virtual UINT64 __stdcall GetNumMessagesDiscardedByMessageCountLimit() = 0;
    virtual UINT64 __stdcall GetMessageCountLimit() = 0;
    virtual HRESULT __stdcall AddStorageFilterEntries(D3D11_INFO_QUEUE_FILTER *) = 0;
    virtual HRESULT __stdcall GetStorageFilter(D3D11_INFO_QUEUE_FILTER *, SIZE_T *) = 0;
    virtual void __stdcall ClearStorageFilter() = 0;
    virtual HRESULT __stdcall PushEmptyStorageFilter() = 0;
    virtual HRESULT __stdcall PushCopyOfStorageFilter() = 0;
    virtual HRESULT __stdcall PushStorageFilter(D3D11_INFO_QUEUE_FILTER *) = 0;
    virtual void __stdcall PopStorageFilter() = 0;
    virtual UINT __stdcall GetStorageFilterStackSize() = 0;
    virtual HRESULT __stdcall AddRetrievalFilterEntries(D3D11_INFO_QUEUE_FILTER *) = 0;
    virtual HRESULT __stdcall GetRetrievalFilter(D3D11_INFO_QUEUE_FILTER *, SIZE_T *) = 0;
    virtual void __stdcall ClearRetrievalFilter() = 0;
    virtual HRESULT __stdcall PushEmptyRetrievalFilter() = 0;
    virtual HRESULT __stdcall PushCopyOfRetrievalFilter() = 0;
    virtual HRESULT __stdcall PushRetrievalFilter(D3D11_INFO_QUEUE_FILTER *) = 0;
    virtual void __stdcall PopRetrievalFilter() = 0;
    virtual UINT __stdcall GetRetrievalFilterStackSize() = 0;
    virtual HRESULT __stdcall AddMessage(D3D11_MESSAGE_CATEGORY, D3D11_MESSAGE_SEVERITY, D3D11_MESSAGE_ID, LPCSTR) = 0;
    virtual HRESULT __stdcall AddApplicationMessage(D3D11_MESSAGE_SEVERITY, LPCSTR) = 0;
    virtual HRESULT __stdcall SetBreakOnCategory(D3D11_MESSAGE_CATEGORY, BOOL) = 0;
    virtual HRESULT __stdcall SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY, BOOL) = 0;
    virtual HRESULT __stdcall SetBreakOnID(D3D11_MESSAGE_ID, BOOL) = 0;
    virtual BOOL __stdcall GetBreakOnCategory(D3D11_MESSAGE_CATEGORY) = 0;
    virtual BOOL __stdcall GetBreakOnSeverity(D3D11_MESSAGE_SEVERITY) = 0;
    virtual BOOL __stdcall GetBreakOnID(D3D11_MESSAGE_ID) = 0;
    virtual void __stdcall SetMuteDebugOutput(BOOL) = 0;
    virtual BOOL __stdcall GetMuteDebugOutput() = 0;
};
#endif // __MINGW32__

#   undef near
#   undef far
#endif

#endif // COMMON_PLATFORM_H_
