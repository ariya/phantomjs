//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.cpp: Debugging utilities.

#include "common/debug.h"
#include "common/platform.h"
#include "common/angleutils.h"

#include <stdarg.h>
#include <vector>
#include <fstream>
#include <cstdio>

namespace gl
{
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
// Wraps the D3D9/D3D11 debug annotation functions.
class DebugAnnotationWrapper
{
  public:
    DebugAnnotationWrapper() { };
    virtual ~DebugAnnotationWrapper() { };
    virtual void beginEvent(const std::wstring &eventName) = 0;
    virtual void endEvent() = 0;
    virtual void setMarker(const std::wstring &markerName) = 0;
    virtual bool getStatus() = 0;
};

#if defined(ANGLE_ENABLE_D3D9)
class D3D9DebugAnnotationWrapper : public DebugAnnotationWrapper
{
  public:
    void beginEvent(const std::wstring &eventName)
    {
        D3DPERF_BeginEvent(0, eventName.c_str());
    }

    void endEvent()
    {
        D3DPERF_EndEvent();
    }

    void setMarker(const std::wstring &markerName)
    {
        D3DPERF_SetMarker(0, markerName.c_str());
    }

    bool getStatus()
    {
        return !!D3DPERF_GetStatus();
    }
};
#endif // ANGLE_ENABLE_D3D9

#if defined(ANGLE_ENABLE_D3D11)
class D3D11DebugAnnotationWrapper : public DebugAnnotationWrapper
{
  public:

    D3D11DebugAnnotationWrapper()
      : mInitialized(false),
        mD3d11Module(NULL),
        mUserDefinedAnnotation(NULL)
    {
        // D3D11 devices can't be created during DllMain.
        // We defer device creation until the object is actually used.
    }

    ~D3D11DebugAnnotationWrapper()
    {
        if (mInitialized)
        {
            SafeRelease(mUserDefinedAnnotation);
            FreeLibrary(mD3d11Module);
        }
    }

    virtual void beginEvent(const std::wstring &eventName)
    {
        initializeDevice();

        mUserDefinedAnnotation->BeginEvent(eventName.c_str());
    }

    virtual void endEvent()
    {
        initializeDevice();

        mUserDefinedAnnotation->EndEvent();
    }

    virtual void setMarker(const std::wstring &markerName)
    {
        initializeDevice();

        mUserDefinedAnnotation->SetMarker(markerName.c_str());
    }

    virtual bool getStatus()
    {
        // ID3DUserDefinedAnnotation::GetStatus doesn't work with the Graphics Diagnostics tools in Visual Studio 2013.

#if defined(_DEBUG) && defined(ANGLE_ENABLE_WINDOWS_STORE)
        // In the Windows Store, we can use IDXGraphicsAnalysis. The call to GetDebugInterface1 only succeeds if the app is under capture.
        // This should only be called in DEBUG mode.
        // If an app links against DXGIGetDebugInterface1 in release mode then it will fail Windows Store ingestion checks.
        IDXGraphicsAnalysis* graphicsAnalysis;
        DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphicsAnalysis));
        bool underCapture = (graphicsAnalysis != NULL);
        SafeRelease(graphicsAnalysis);
        return underCapture;
#endif

        // Otherwise, we have to return true here.
        return true;
    }

  protected:

    void initializeDevice()
    {
        if (!mInitialized)
        {
#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
            mD3d11Module = LoadLibrary(TEXT("d3d11.dll"));
            ASSERT(mD3d11Module);

            PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(mD3d11Module, "D3D11CreateDevice");
            ASSERT(D3D11CreateDevice != NULL);
#endif // !ANGLE_ENABLE_WINDOWS_STORE

            ID3D11Device* device = NULL;
            ID3D11DeviceContext* context = NULL;

            HRESULT hr = E_FAIL;

            // Create a D3D_DRIVER_TYPE_NULL device, which is much cheaper than other types of device.
            hr =  D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device, NULL, &context);
            ASSERT(SUCCEEDED(hr));

            hr = context->QueryInterface(__uuidof(mUserDefinedAnnotation), reinterpret_cast<void**>(&mUserDefinedAnnotation));
            ASSERT(SUCCEEDED(hr) && mUserDefinedAnnotation != NULL);

            SafeRelease(device);
            SafeRelease(context);

            mInitialized = true;
        }
    }

    bool mInitialized;
    HMODULE mD3d11Module;
    ID3DUserDefinedAnnotation* mUserDefinedAnnotation;
};
#endif // ANGLE_ENABLE_D3D11

static DebugAnnotationWrapper* g_DebugAnnotationWrapper = NULL;

void InitializeDebugAnnotations()
{
#if defined(ANGLE_ENABLE_D3D9)
    g_DebugAnnotationWrapper = new D3D9DebugAnnotationWrapper();
#elif defined(ANGLE_ENABLE_D3D11)
    // If the project uses D3D9 then we can use the D3D9 debug annotations, even with the D3D11 renderer.
    // However, if D3D9 is unavailable (e.g. in Windows Store), then we use D3D11 debug annotations.
    // The D3D11 debug annotations are methods on ID3DUserDefinedAnnotation, which is implemented by the DeviceContext.
    // This doesn't have to be the same DeviceContext that the renderer uses, though.
    g_DebugAnnotationWrapper = new D3D11DebugAnnotationWrapper();
#endif
}

void UninitializeDebugAnnotations()
{
    if (g_DebugAnnotationWrapper != NULL)
    {
        SafeDelete(g_DebugAnnotationWrapper);
    }
}

#endif // ANGLE_ENABLE_DEBUG_ANNOTATIONS

enum DebugTraceOutputType
{
   DebugTraceOutputTypeNone,
   DebugTraceOutputTypeSetMarker,
   DebugTraceOutputTypeBeginEvent
};

static void output(bool traceInDebugOnly, DebugTraceOutputType outputType, const char *format, va_list vararg)
{
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
    static std::vector<char> buffer(512);

    if (perfActive())
    {
        size_t len = FormatStringIntoVector(format, vararg, buffer);
        std::wstring formattedWideMessage(buffer.begin(), buffer.begin() + len);

        switch (outputType)
        {
            case DebugTraceOutputTypeNone:
                break;
            case DebugTraceOutputTypeBeginEvent:
                g_DebugAnnotationWrapper->beginEvent(formattedWideMessage);
                break;
            case DebugTraceOutputTypeSetMarker:
                g_DebugAnnotationWrapper->setMarker(formattedWideMessage);
                break;
        }
    }
#endif // ANGLE_ENABLE_DEBUG_ANNOTATIONS

#if defined(ANGLE_ENABLE_DEBUG_TRACE)
#if defined(NDEBUG)
    if (traceInDebugOnly)
    {
        return;
    }
#endif // NDEBUG
    std::string formattedMessage = FormatString(format, vararg);

    static std::ofstream file(TRACE_OUTPUT_FILE, std::ofstream::app);
    if (file)
    {
        file.write(formattedMessage.c_str(), formattedMessage.length());
        file.flush();
    }

#if defined(ANGLE_ENABLE_DEBUG_TRACE_TO_DEBUGGER)
    OutputDebugStringA(formattedMessage.c_str());
#endif // ANGLE_ENABLE_DEBUG_TRACE_TO_DEBUGGER

#endif // ANGLE_ENABLE_DEBUG_TRACE
}

void trace(bool traceInDebugOnly, const char *format, ...)
{
    va_list vararg;
    va_start(vararg, format);
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
    output(traceInDebugOnly, DebugTraceOutputTypeSetMarker, format, vararg);
#else
    output(traceInDebugOnly, DebugTraceOutputTypeNone, format, vararg);
#endif
    va_end(vararg);
}

bool perfActive()
{
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
    static bool active = g_DebugAnnotationWrapper->getStatus();
    return active;
#else
    return false;
#endif
}

ScopedPerfEventHelper::ScopedPerfEventHelper(const char* format, ...)
{
#if !defined(ANGLE_ENABLE_DEBUG_TRACE)
    if (!perfActive())
    {
        return;
    }
#endif // !ANGLE_ENABLE_DEBUG_TRACE
    va_list vararg;
    va_start(vararg, format);
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
    output(true, DebugTraceOutputTypeBeginEvent, format, vararg);
#else
    output(true, DebugTraceOutputTypeNone, format, vararg);
#endif // ANGLE_ENABLE_DEBUG_ANNOTATIONS
    va_end(vararg);
}

ScopedPerfEventHelper::~ScopedPerfEventHelper()
{
#if defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
    if (perfActive())
    {
        g_DebugAnnotationWrapper->endEvent();
    }
#endif
}
}
