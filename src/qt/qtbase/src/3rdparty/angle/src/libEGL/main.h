//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.h: Management of thread-local data.

#ifndef LIBEGL_MAIN_H_
#define LIBEGL_MAIN_H_

#include "libEGL/Error.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace egl
{
struct Current
{
    EGLint error;
    EGLenum API;
    EGLDisplay display;
    EGLSurface drawSurface;
    EGLSurface readSurface;
};

void recordError(const Error &error);
EGLint getCurrentError();

void setCurrentAPI(EGLenum API);
EGLenum getCurrentAPI();

void setCurrentDisplay(EGLDisplay dpy);
EGLDisplay getCurrentDisplay();

void setCurrentDrawSurface(EGLSurface surface);
EGLSurface getCurrentDrawSurface();

void setCurrentReadSurface(EGLSurface surface);
EGLSurface getCurrentReadSurface();

}

#endif  // LIBEGL_MAIN_H_
