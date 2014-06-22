//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.h: Management of thread-local data.

#ifndef LIBEGL_MAIN_H_
#define LIBEGL_MAIN_H_

#define EGLAPI
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

void setCurrentError(EGLint error);
EGLint getCurrentError();

void setCurrentAPI(EGLenum API);
EGLenum getCurrentAPI();

void setCurrentDisplay(EGLDisplay dpy);
EGLDisplay getCurrentDisplay();

void setCurrentDrawSurface(EGLSurface surface);
EGLSurface getCurrentDrawSurface();

void setCurrentReadSurface(EGLSurface surface);
EGLSurface getCurrentReadSurface();

void error(EGLint errorCode);

template<class T>
const T &error(EGLint errorCode, const T &returnValue)
{
    error(errorCode);

    return returnValue;
}

template<class T>
const T &success(const T &returnValue)
{
    egl::setCurrentError(EGL_SUCCESS);

    return returnValue;
}

}

#endif  // LIBEGL_MAIN_H_
