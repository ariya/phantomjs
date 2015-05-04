//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.h: Management of thread-local data.

#ifndef LIBGLESV2_MAIN_H_
#define LIBGLESV2_MAIN_H_

#include "common/debug.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace egl
{
class Display;
class Surface;
class AttributeMap;
}

namespace gl
{
class Context;
    
struct Current
{
    Context *context;
    egl::Display *display;
};

void makeCurrent(Context *context, egl::Display *display, egl::Surface *surface);

Context *getContext();
Context *getNonLostContext();
egl::Display *getDisplay();

}

namespace rx
{
class Renderer;
}

extern "C"
{
// Exported functions for use by EGL
gl::Context *glCreateContext(int clientVersion, const gl::Context *shareContext, rx::Renderer *renderer, bool notifyResets, bool robustAccess);
void glDestroyContext(gl::Context *context);
void glMakeCurrent(gl::Context *context, egl::Display *display, egl::Surface *surface);
gl::Context *glGetCurrentContext();
rx::Renderer *glCreateRenderer(egl::Display *display, EGLNativeDisplayType nativeDisplay, const egl::AttributeMap &attribMap);
void glDestroyRenderer(rx::Renderer *renderer);

__eglMustCastToProperFunctionPointerType EGLAPIENTRY glGetProcAddress(const char *procname);
bool EGLAPIENTRY glBindTexImage(egl::Surface *surface);
}

#endif   // LIBGLESV2_MAIN_H_
