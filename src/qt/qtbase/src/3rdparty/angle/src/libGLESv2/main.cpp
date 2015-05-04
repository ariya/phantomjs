//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libGLESv2/main.h"
#include "libGLESv2/Context.h"

#include "common/tls.h"

static TLSIndex currentTLS = TLS_INVALID_INDEX;

namespace gl
{

// TODO(kbr): figure out how these are going to be managed on
// non-Windows platforms. These routines would need to be exported
// from ANGLE and called cooperatively when users create and destroy
// threads -- or the initialization of the TLS index, and allocation
// of thread-local data, will have to be done lazily. Will have to use
// destructor function with pthread_create_key on POSIX platforms to
// clean up thread-local data.

// Call this exactly once at process startup.
bool CreateThreadLocalIndex()
{
    currentTLS = CreateTLSIndex();
    if (currentTLS == TLS_INVALID_INDEX)
    {
        return false;
    }
    return true;
}

// Call this exactly once at process shutdown.
void DestroyThreadLocalIndex()
{
    DestroyTLSIndex(currentTLS);
    currentTLS = TLS_INVALID_INDEX;
}

// Call this upon thread startup.
Current *AllocateCurrent()
{
    ASSERT(currentTLS != TLS_INVALID_INDEX);
    if (currentTLS == TLS_INVALID_INDEX)
    {
        return NULL;
    }

    Current *current = new Current();
    current->context = NULL;
    current->display = NULL;

    if (!SetTLSValue(currentTLS, current))
    {
        ERR("Could not set thread local storage.");
        return NULL;
    }

    return current;
}

// Call this upon thread shutdown.
void DeallocateCurrent()
{
    Current *current = reinterpret_cast<Current*>(GetTLSValue(currentTLS));
    SafeDelete(current);
    SetTLSValue(currentTLS, NULL);
}

}

#if defined(ANGLE_PLATFORM_WINDOWS) && !defined(QT_OPENGL_ES_2_ANGLE_STATIC)
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
            if (!gl::CreateThreadLocalIndex())
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
            gl::DestroyThreadLocalIndex();

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
#endif // ANGLE_PLATFORM_WINDOWS && !QT_OPENGL_ES_2_ANGLE_STATIC

namespace gl
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
    static Current current = { 0, 0 };
    return &current;
#endif
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
            context->recordError(Error(GL_OUT_OF_MEMORY, "Context has been lost."));
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

}
