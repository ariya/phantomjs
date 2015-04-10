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

#ifndef EvasGLSurface_h
#define EvasGLSurface_h
#if USE(ACCELERATED_COMPOSITING)

#include <Evas_GL.h>
#include <WebCore/IntSize.h>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

class EvasGLSurface {
public:
    static PassOwnPtr<EvasGLSurface> create(Evas_GL* evasGL, Evas_GL_Config* cfg, const WebCore::IntSize& size)
    {
        ASSERT(evasGL);
        ASSERT(cfg);

        Evas_GL_Surface* surface = 0;

        // Ensure that the surface is created with valid size.
        if (size.width() && size.height())
            surface = evas_gl_surface_create(evasGL, cfg, size.width(), size.height());

        if (!surface)
            return nullptr;

        // Ownership of surface is passed to EvasGLSurface.
        return adoptPtr(new EvasGLSurface(evasGL, surface));
    }
    ~EvasGLSurface();

    Evas_GL_Surface* surface() { return m_surface; }

private:
    EvasGLSurface(Evas_GL* evasGL, Evas_GL_Surface* passSurface);

    Evas_GL* m_evasGL;
    Evas_GL_Surface* m_surface;
};

} // namespace WebKit

#endif // USE(ACCELERATED_COMPOSITING)
#endif // EvasGLSurface_h
