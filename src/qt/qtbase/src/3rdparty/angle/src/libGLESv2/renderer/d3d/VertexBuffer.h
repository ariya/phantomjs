//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.h: Defines the abstract VertexBuffer class and VertexBufferInterface
// class with derivations, classes that perform graphics API agnostic vertex buffer operations.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER_H_

#include "common/angleutils.h"
#include "libGLESv2/Error.h"

#include <GLES2/gl2.h>

#include <cstddef>
#include <vector>

namespace gl
{
struct VertexAttribute;
struct VertexAttribCurrentValueData;
}

namespace rx
{
class RendererD3D;

class VertexBuffer
{
  public:
    VertexBuffer();
    virtual ~VertexBuffer();

    virtual gl::Error initialize(unsigned int size, bool dynamicUsage) = 0;

    virtual gl::Error storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                            GLint start, GLsizei count, GLsizei instances, unsigned int offset) = 0;
    virtual gl::Error getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances,
                                       unsigned int *outSpaceRequired) const = 0;

    virtual unsigned int getBufferSize() const = 0;
    virtual gl::Error setBufferSize(unsigned int size) = 0;
    virtual gl::Error discard() = 0;

    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

class VertexBufferInterface
{
  public:
    VertexBufferInterface(RendererD3D *renderer, bool dynamic);
    virtual ~VertexBufferInterface();

    gl::Error reserveVertexSpace(const gl::VertexAttribute &attribute, GLsizei count, GLsizei instances);

    unsigned int getBufferSize() const;

    unsigned int getSerial() const;

    virtual gl::Error storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                            GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset);

    bool directStoragePossible(const gl::VertexAttribute &attrib,
                               const gl::VertexAttribCurrentValueData &currentValue) const;

    VertexBuffer* getVertexBuffer() const;

  protected:
    virtual gl::Error reserveSpace(unsigned int size) = 0;

    unsigned int getWritePosition() const;
    void setWritePosition(unsigned int writePosition);

    gl::Error discard();

    gl::Error setBufferSize(unsigned int size);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBufferInterface);

    RendererD3D *const mRenderer;

    VertexBuffer* mVertexBuffer;

    unsigned int mWritePosition;
    unsigned int mReservedSpace;
    bool mDynamic;
};

class StreamingVertexBufferInterface : public VertexBufferInterface
{
  public:
    StreamingVertexBufferInterface(RendererD3D *renderer, std::size_t initialSize);
    ~StreamingVertexBufferInterface();

  protected:
    gl::Error reserveSpace(unsigned int size);
};

class StaticVertexBufferInterface : public VertexBufferInterface
{
  public:
    explicit StaticVertexBufferInterface(RendererD3D *renderer);
    ~StaticVertexBufferInterface();

    gl::Error storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                    GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset);

    bool lookupAttribute(const gl::VertexAttribute &attribute, unsigned int* outStreamFffset);

  protected:
    gl::Error reserveSpace(unsigned int size);

  private:
    struct VertexElement
    {
        GLenum type;
        GLuint size;
        GLuint stride;
        bool normalized;
        bool pureInteger;
        size_t attributeOffset;

        unsigned int streamOffset;
    };

    std::vector<VertexElement> mCache;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER_H_