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

#ifndef GLPlatformSurface_h
#define GLPlatformSurface_h

#if USE(ACCELERATED_COMPOSITING)

#include "GLDefs.h"
#include "IntRect.h"
#include <wtf/Noncopyable.h>

// Encapsulates a surface that can be rendered to with GL, hiding platform
// specific management.
namespace WebCore {

class GLPlatformSurface {
    WTF_MAKE_NONCOPYABLE(GLPlatformSurface);

public:
    enum Attributes {
        Default = 0x00, // No Alpha channel. Only R,G,B values set.
        SupportAlpha = 0x01,
        DoubleBuffered = 0x02
    };

    typedef unsigned SurfaceAttributes;
    // Creates a GL surface used for offscreen rendering.
    static PassOwnPtr<GLPlatformSurface> createOffScreenSurface(SurfaceAttributes = GLPlatformSurface::Default);

    virtual ~GLPlatformSurface();

    const IntRect& geometry() const;

    // Get the underlying platform specific buffer handle.
    // The handle will be null if surface doesn't support
    // buffer sharing.
    PlatformBufferHandle handle() const;

    PlatformDrawable drawable() const;

    PlatformDisplay sharedDisplay() const;

    virtual SurfaceAttributes attributes() const;

    virtual void swapBuffers();

    virtual bool isCurrentDrawable() const;
    virtual void onMakeCurrent();

    // Convenience Function to update surface backbuffer with texture contents.
    // Note that the function doesn't track or restore any GL states.
    // Function does the following(in order):
    // a) Blits texture contents to back buffer.
    // b) Calls Swap Buffers.
    virtual void updateContents(const uint32_t);

    virtual void setGeometry(const IntRect&);

    virtual PlatformSurfaceConfig configuration();

    virtual void destroy();

protected:
    GLPlatformSurface(SurfaceAttributes);

    PlatformDisplay m_sharedDisplay;
    PlatformDrawable m_drawable;
    PlatformBufferHandle m_bufferHandle;
    IntRect m_rect;
};

}

#endif

#endif

