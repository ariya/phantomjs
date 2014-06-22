//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer9.h: Defines the D3D9 VertexBuffer implementation.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER9_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER9_H_

#include "libGLESv2/renderer/VertexBuffer.h"

namespace rx
{
class Renderer9;

class VertexBuffer9 : public VertexBuffer
{
  public:
    explicit VertexBuffer9(rx::Renderer9 *const renderer);
    virtual ~VertexBuffer9();

    virtual bool initialize(unsigned int size, bool dynamicUsage);

    static VertexBuffer9 *makeVertexBuffer9(VertexBuffer *vertexBuffer);

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances,
                                       unsigned int offset);
    virtual bool storeRawData(const void* data, unsigned int size, unsigned int offset);

    virtual bool getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances, unsigned int *outSpaceRequired) const;

    virtual bool requiresConversion(const gl::VertexAttribute &attrib) const;

    unsigned int getVertexSize(const gl::VertexAttribute &attrib) const;
    D3DDECLTYPE getDeclType(const gl::VertexAttribute &attrib) const;

    virtual unsigned int getBufferSize() const;
    virtual bool setBufferSize(unsigned int size);
    virtual bool discard();

    IDirect3DVertexBuffer9 *getBuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer9);

    rx::Renderer9 *const mRenderer;

    IDirect3DVertexBuffer9 *mVertexBuffer;
    unsigned int mBufferSize;
    bool mDynamicUsage;

    // Attribute format conversion
    enum { NUM_GL_VERTEX_ATTRIB_TYPES = 6 };

    struct FormatConverter
    {
        bool identity;
        std::size_t outputElementSize;
        void (*convertArray)(const void *in, std::size_t stride, std::size_t n, void *out);
        D3DDECLTYPE d3dDeclType;
    };

    static bool mTranslationsInitialized;
    static void initializeTranslations(DWORD declTypes);

    // [GL types as enumerated by typeIndex()][normalized][size - 1]
    static FormatConverter mFormatConverters[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];

    struct TranslationDescription
    {
        DWORD capsFlag;
        FormatConverter preferredConversion;
        FormatConverter fallbackConversion;
    };

    // This table is used to generate mFormatConverters.
    // [GL types as enumerated by typeIndex()][normalized][size - 1]
    static const TranslationDescription mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];

    static unsigned int typeIndex(GLenum type);
    static const FormatConverter &formatConverter(const gl::VertexAttribute &attribute);

    static bool spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances,
                              unsigned int *outSpaceRequired);
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER9_H_
