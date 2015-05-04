//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libEGL/main.h"

#include "common/debug.h"
#include "common/tls.h"

static TLSIndex currentTLS = TLS_OUT_OF_INDEXES;

namespace egl
{

Current *AllocateCurrent()
{
    ASSERT(currentTLS != TLS_OUT_OF_INDEXES);
    if (currentTLS == TLS_OUT_OF_INDEXES)
    {
        return NULL;
    }

    Current *current = new Current();
    current->error = EGL_SUCCESS;
    current->API = EGL_OPENGL_ES_API;
    current->display = EGL_NO_DISPLAY;
    current->drawSurface = EGL_NO_SURFACE;
    current->readSurface = EGL_NO_SURFACE;

    if (!SetTLSValue(currentTLS, current))
    {
        ERR("Could not set thread local storage.");
        return NULL;
    }

    return current;
}

void DeallocateCurrent()
{
    Current *current = reinterpret_cast<Current*>(GetTLSValue(currentTLS));
    SafeDelete(current);
    SetTLSValue(currentTLS, NULL);
}

}

#ifndef QT_OPENGL_ES_2_ANGLE_STATIC

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
#if defined(ANGLE_ENABLE_DEBUG_TRACE)
            FILE *debug = fopen(TRACE_OUTPUT_FILE, "rt");

            if (debug)
            {
                fclose(debug);
                debug = fopen(TRACE_OUTPUT_FILE, "wt");   // Erase
                
                if (debug)
                {
                    fclose(debug);
                }
            }
#endif

            currentTLS = CreateTLSIndex();
            if (currentTLS == TLS_OUT_OF_INDEXES)
            {
                return FALSE;
            }

#ifdef ANGLE_ENABLE_DEBUG_ANNOTATIONS
            gl::InitializeDebugAnnotations();
#endif
        }
        // Fall through to initialize index
      case DLL_THREAD_ATTACH:
        {
            egl::AllocateCurrent();
        }
        break;
      case DLL_THREAD_DETACH:
        {
            egl::DeallocateCurrent();
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            egl::DeallocateCurrent();
            DestroyTLSIndex(currentTLS);

#ifdef ANGLE_ENABLE_DEBUG_ANNOTATIONS
            gl::UninitializeDebugAnnotations();
#endif
        }
        break;
      default:
        break;
    }

    return TRUE;
}

#endif // !QT_OPENGL_ES_2_ANGLE_STATIC

namespace egl
{

Current *GetCurrentData()
{
#ifndef QT_OPENGL_ES_2_ANGLE_STATIC
    Current *current = reinterpret_cast<Current*>(GetTLSValue(currentTLS));

    // ANGLE issue 488: when the dll is loaded after thread initialization,
    // thread local storage (current) might not exist yet.
    return (current ? current : AllocateCurrent());
#else
    // No precautions for thread safety taken as ANGLE is used single-threaded in Qt.
    static Current current = { EGL_SUCCESS, EGL_OPENGL_ES_API, EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE };
    return &current;
#endif
}

void recordError(const Error &error)
{
    Current *current = GetCurrentData();

    current->error = error.getCode();
}

EGLint getCurrentError()
{
    Current *current = GetCurrentData();

    return current->error;
}

void setCurrentAPI(EGLenum API)
{
    Current *current = GetCurrentData();

    current->API = API;
}

EGLenum getCurrentAPI()
{
    Current *current = GetCurrentData();

    return current->API;
}

void setCurrentDisplay(EGLDisplay dpy)
{
    Current *current = GetCurrentData();

    current->display = dpy;
}

EGLDisplay getCurrentDisplay()
{
    Current *current = GetCurrentData();

    return current->display;
}

void setCurrentDrawSurface(EGLSurface surface)
{
    Current *current = GetCurrentData();

    current->drawSurface = surface;
}

EGLSurface getCurrentDrawSurface()
{
    Current *current = GetCurrentData();

    return current->drawSurface;
}

void setCurrentReadSurface(EGLSurface surface)
{
    Current *current = GetCurrentData();

    current->readSurface = surface;
}

EGLSurface getCurrentReadSurface()
{
    Current *current = GetCurrentData();

    return current->readSurface;
}

}
