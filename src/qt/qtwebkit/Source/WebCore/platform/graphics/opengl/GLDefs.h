/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GLDefs_h
#define GLDefs_h

#if USE(ACCELERATED_COMPOSITING)

#define GL_GLEXT_PROTOTYPES 1

#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#include "OpenGLESShims.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include "Extensions3DOpenGL.h"
#include "OpenGLShims.h"
#include <GL/gl.h>
#include <GL/glext.h>
#if USE(GLX)
#define GLX_GLXEXT_PROTOTYPES 1
#include <GL/glx.h>
#include <GL/glxext.h>
#endif
#endif

#if USE(EGL)
#define EGL_EGLEXT_PROTOTYPES 1
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace WebCore {

typedef uint32_t PlatformBufferHandle;

#if USE(GLX)
typedef GLXContext PlatformContext;
typedef Display* PlatformDisplay;
typedef GLXFBConfig PlatformSurfaceConfig;
typedef GLXDrawable PlatformDrawable;
#elif USE(EGL)
#if USE(OPENGL_ES_2)
static const EGLenum eglAPIVersion = EGL_OPENGL_ES_API;
#else
static const EGLenum eglAPIVersion = EGL_OPENGL_API;
#endif
typedef EGLContext PlatformContext;
typedef EGLDisplay PlatformDisplay;
typedef EGLConfig PlatformSurfaceConfig;
typedef EGLSurface PlatformDrawable;
#else
typedef void* PlatformContext;
typedef void* PlatformDisplay;
typedef void* PlatformSurfaceConfig;
typedef void* PlatformDrawable;
#endif

}

#endif

#endif
