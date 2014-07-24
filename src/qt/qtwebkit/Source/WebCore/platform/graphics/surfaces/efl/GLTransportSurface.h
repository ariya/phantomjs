/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef GLTransportSurface_h
#define GLTransportSurface_h

#if USE(ACCELERATED_COMPOSITING)

#include "GLPlatformSurface.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class TextureMapperShaderProgram;

class GLTransportSurface : public GLPlatformSurface {

public:
    // Creates a GL surface whose results can be transported to the UI process for display.
    static PassOwnPtr<GLTransportSurface> createTransportSurface(const IntSize&, SurfaceAttributes = GLPlatformSurface::Default);
    virtual ~GLTransportSurface();
    virtual void updateContents(const uint32_t) OVERRIDE;
    virtual void setGeometry(const IntRect&) OVERRIDE;
    virtual void destroy() OVERRIDE;

protected:
    GLTransportSurface(const IntSize&, SurfaceAttributes);
    void updateTransformationMatrix();
    void bindArrayBuffer() const;
    void initializeShaderProgram();
    void draw(const uint32_t);

    RefPtr<GraphicsContext3D> m_context3D;
    RefPtr<TextureMapperShaderProgram> m_shaderProgram;
    Platform3DObject m_vbo;
    Platform3DObject m_vertexHandle;
    GLuint m_boundTexture;
};

class GLTransportSurfaceClient {

public:
    static PassOwnPtr<GLTransportSurfaceClient> createTransportSurfaceClient(const PlatformBufferHandle, const IntSize&, bool);
    virtual ~GLTransportSurfaceClient();
    virtual void prepareTexture();
    virtual void destroy();
    GLuint texture() const { return m_texture; }

protected:
    GLTransportSurfaceClient();
    void createTexture();
    GLuint m_texture;
};

}

#endif

#endif
