#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libGLESv2/main.h"

#include "libGLESv2/Context.h"

#if !defined(ANGLE_OS_WINRT)
static DWORD currentTLS = TLS_OUT_OF_INDEXES;
#else
static __declspec(thread) void *currentTLS = 0;
#endif

namespace gl
{

Current *AllocateCurrent()
{
#if !defined(ANGLE_OS_WINRT)
    Current *current = (Current*)LocalAlloc(LPTR, sizeof(Current));
#else
    currentTLS = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Current));
    Current *current = (Current*)currentTLS;
#endif

    if (!current)
    {
        ERR("Could not allocate thread local storage.");
        return NULL;
    }

#if !defined(ANGLE_OS_WINRT)
    ASSERT(currentTLS != TLS_OUT_OF_INDEXES);
    TlsSetValue(currentTLS, current);
#endif

    current->context = NULL;
    current->display = NULL;

    return current;
}

void DeallocateCurrent()
{
#if !defined(ANGLE_OS_WINRT)
    void *current = TlsGetValue(currentTLS);

    if (current)
    {
        LocalFree((HLOCAL)current);
    }
#else
    if (currentTLS)
    {
        HeapFree(GetProcessHeap(), 0, currentTLS);
        currentTLS = 0;
    }
#endif
}

}

#ifndef QT_OPENGL_ES_2_ANGLE_STATIC

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
#if !defined(ANGLE_OS_WINRT)
            currentTLS = TlsAlloc();

            if (currentTLS == TLS_OUT_OF_INDEXES)
            {
                return FALSE;
            }
#endif
        }
        // Fall throught to initialize index
      case DLL_THREAD_ATTACH:
        {
            gl::AllocateCurrent();
        }
        break;
      case DLL_THREAD_DETACH:
        {
            gl::DeallocateCurrent();
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            gl::DeallocateCurrent();
#if !defined(ANGLE_OS_WINRT)
            TlsFree(currentTLS);
#endif
        }
        break;
      default:
        break;
    }

    return TRUE;
}

#endif // !QT_OPENGL_ES_2_ANGLE_STATIC

namespace gl
{

Current *GetCurrentData()
{
#ifndef QT_OPENGL_ES_2_ANGLE_STATIC
#if !defined(ANGLE_OS_WINRT)
    Current *current = (Current*)TlsGetValue(currentTLS);
#else
    Current *current = (Current*)currentTLS;
#endif
#else
    // No precautions for thread safety taken as ANGLE is used single-threaded in Qt.
    static Current s_current = { 0, 0 };
    Current *current = &s_current;
#endif

    // ANGLE issue 488: when the dll is loaded after thread initialization,
    // thread local storage (current) might not exist yet.
    return (current ? current : AllocateCurrent());
}

void makeCurrent(Context *context, egl::Display *display, egl::Surface *surface)
{
    Current *current = GetCurrentData();

    current->context = context;
    current->display = display;

    if (context && display && surface)
    {
        context->makeCurrent(surface);
    }
}

Context *getContext()
{
    Current *current = GetCurrentData();

    return current->context;
}

Context *getNonLostContext()
{
    Context *context = getContext();
    
    if (context)
    {
        if (context->isContextLost())
        {
            gl::error(GL_OUT_OF_MEMORY);
            return NULL;
        }
        else
        {
            return context;
        }
    }
    return NULL;
}

egl::Display *getDisplay()
{
    Current *current = GetCurrentData();

    return current->display;
}

// Records an error code
void error(GLenum errorCode)
{
    gl::Context *context = glGetCurrentContext();

    if (context)
    {
        switch (errorCode)
        {
          case GL_INVALID_ENUM:
            context->recordInvalidEnum();
            TRACE("\t! Error generated: invalid enum\n");
            break;
          case GL_INVALID_VALUE:
            context->recordInvalidValue();
            TRACE("\t! Error generated: invalid value\n");
            break;
          case GL_INVALID_OPERATION:
            context->recordInvalidOperation();
            TRACE("\t! Error generated: invalid operation\n");
            break;
          case GL_OUT_OF_MEMORY:
            context->recordOutOfMemory();
            TRACE("\t! Error generated: out of memory\n");
            break;
          case GL_INVALID_FRAMEBUFFER_OPERATION:
            context->recordInvalidFramebufferOperation();
            TRACE("\t! Error generated: invalid framebuffer operation\n");
            break;
          default: UNREACHABLE();
        }
    }
}

}

