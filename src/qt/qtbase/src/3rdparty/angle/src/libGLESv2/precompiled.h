//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// precompiled.h: Precompiled header file for libGLESv2.

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define EGLAPI
#include <EGL/egl.h>

#include <assert.h>
#include <cstddef>
#include <float.h>
#include <intrin.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <algorithm> // for std::min and std::max
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY==WINAPI_FAMILY_APP || WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
#  define ANGLE_OS_WINRT
#  if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
#    define ANGLE_OS_WINPHONE
#  endif
#endif

#if defined(ANGLE_ENABLE_D3D9)
#  include <d3d9.h>
#endif
#if defined(ANGLE_ENABLE_D3D11)
#  if !defined(ANGLE_OS_WINRT)
#    include <d3d11.h>
#  else
#    include <d3d11_1.h>
#    define Sleep(x) WaitForSingleObjectEx(GetCurrentThread(), x, FALSE)
#    define GetVersion() WINVER
#    define LoadLibrary(x) LoadPackagedLibrary(x, NULL)
#  endif
#  include <dxgi.h>
#endif
#if !defined(ANGLE_OS_WINPHONE)
#  include <d3dcompiler.h>
#endif

#ifndef D3DCOMPILE_OPTIMIZATION_LEVEL0
#define D3DCOMPILE_OPTIMIZATION_LEVEL0 (1 << 14)
#endif
#ifndef D3DCOMPILE_OPTIMIZATION_LEVEL1
#define D3DCOMPILE_OPTIMIZATION_LEVEL1 0
#endif
#ifndef D3DCOMPILE_OPTIMIZATION_LEVEL2
#define D3DCOMPILE_OPTIMIZATION_LEVEL2 ((1 << 14) | (1 << 15))
#endif
#ifndef D3DCOMPILE_OPTIMIZATION_LEVEL3
#define D3DCOMPILE_OPTIMIZATION_LEVEL3 (1 << 15)
#endif
#ifndef D3DCOMPILE_DEBUG
#define D3DCOMPILE_DEBUG (1 << 0)
#endif
#ifndef D3DCOMPILE_SKIP_OPTIMIZATION
#define D3DCOMPILE_SKIP_OPTIMIZATION (1 << 2)
#endif
#ifndef D3DCOMPILE_AVOID_FLOW_CONTROL
#define D3DCOMPILE_AVOID_FLOW_CONTROL (1 << 9)
#endif
#ifndef D3DCOMPILE_PREFER_FLOW_CONTROL
#define D3DCOMPILE_PREFER_FLOW_CONTROL (1 << 10)
#endif

#ifdef _MSC_VER
#include <hash_map>
#endif
