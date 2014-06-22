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

#ifndef EGLConfigSelector_h
#define EGLConfigSelector_h

#if USE(EGL)

#include <opengl/GLDefs.h>
#include <opengl/GLPlatformSurface.h>
#include <wtf/Noncopyable.h>

namespace WebCore {
#if PLATFORM(X11)
typedef VisualID NativeVisualId;
#endif

class EGLConfigSelector {
    WTF_MAKE_NONCOPYABLE(EGLConfigSelector);
public:
    EGLConfigSelector(GLPlatformSurface::SurfaceAttributes);
    virtual ~EGLConfigSelector();
    virtual EGLConfig pixmapContextConfig();
    virtual EGLConfig surfaceContextConfig();
    EGLint nativeVisualId(const EGLConfig&) const;
    GLPlatformSurface::SurfaceAttributes attributes() const;
    void reset();
#if PLATFORM(X11)
    virtual EGLConfig surfaceClientConfig(NativeVisualId);
#endif

private:
#if PLATFORM(X11)
    EGLConfig findMatchingConfigWithVisualId(NativeVisualId);
#endif
    EGLConfig createConfig(EGLint expectedSurfaceType);

protected:
    EGLConfig m_pixmapFBConfig;
    EGLConfig m_surfaceContextFBConfig;
    unsigned m_attributes : 3;
    PlatformDisplay m_sharedDisplay;
};

}

#endif

#endif
