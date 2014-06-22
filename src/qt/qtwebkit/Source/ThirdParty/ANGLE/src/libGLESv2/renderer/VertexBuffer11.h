//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer11.h: Defines the D3D11 VertexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER11_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER11_H_

#include "libGLESv2/renderer/VertexBuffer.h"

namespace rx
{
class Renderer11;

class VertexBuffer11 : public VertexBuffer
{
  public:
    explicit VertexBuffer11(rx::Renderer11 *const renderer);
    virtual ~VertexBuffer11();

    virtual bool initialize(unsigned int size, bool dynamicUsage);

    static VertexBuffer11 *makeVertexBuffer11(VertexBuffer *vetexBuffer);

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances,
                                 unsigned int offset);
    virtual bool storeRawData(const void* data, unsigned int size, unsigned int offset);

    virtual unsigned int getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances) const;

    virtual bool requiresConversion(const gl::VertexAttribute &attrib) const;

    virtual unsigned int getBufferSize() const;
    virtual bool setBufferSize(unsigned int size);
    virtual bool discard();

    unsigned int getVertexSize(const gl::VertexAttribute &attrib) const;
    DXGI_FORMAT getDXGIFormat(const gl::VertexAttribute &attrib) const;

    ID3D11Buffer *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer11);

    rx::Renderer11 *const mRenderer;

    ID3D11Buffer *mBuffer;
    unsigned int mBufferSize;
    bool mDynamicUsage;

    typedef void (*VertexConversionFunction)(const void *, unsigned int, unsigned int, void *);
    struct VertexConverter
    {
        VertexConversionFunction conversionFunc;
        bool identity;
        DXGI_FORMAT dxgiFormat;
        unsigned int outputElementSize;
    };

    enum { NUM_GL_VERTEX_ATTRIB_TYPES = 6 };

    // This table is used to generate mAttributeTypes.
    static const VertexConverter mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4]; // [GL types as enumerated by typeIndex()][normalized][size - 1]

    static const VertexConverter &getVertexConversion(const gl::VertexAttribute &attribute);
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER11_H_