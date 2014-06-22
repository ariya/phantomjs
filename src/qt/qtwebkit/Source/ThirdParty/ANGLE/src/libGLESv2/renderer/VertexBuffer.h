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

namespace gl
{
class VertexAttribute;
}

namespace rx
{
class Renderer;

class VertexBuffer
{
  public:
    VertexBuffer();
    virtual ~VertexBuffer();

    virtual bool initialize(unsigned int size, bool dynamicUsage) = 0;

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count,
                                       GLsizei instances, unsigned int offset) = 0;
    virtual bool storeRawData(const void* data, unsigned int size, unsigned int offset) = 0;

    virtual unsigned int getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count,
                                          GLsizei instances) const = 0;

    virtual bool requiresConversion(const gl::VertexAttribute &attrib) const = 0;

    virtual unsigned int getBufferSize() const = 0;
    virtual bool setBufferSize(unsigned int size) = 0;
    virtual bool discard() = 0;

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
    VertexBufferInterface(rx::Renderer *renderer, bool dynamic);
    virtual ~VertexBufferInterface();

    bool reserveVertexSpace(const gl::VertexAttribute &attribute, GLsizei count, GLsizei instances);
    bool reserveRawDataSpace(unsigned int size);

    unsigned int getBufferSize() const;

    unsigned int getSerial() const;

    virtual int storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances);
    virtual int storeRawData(const void* data, unsigned int size);

    VertexBuffer* getVertexBuffer() const;

  protected:
    virtual bool reserveSpace(unsigned int size) = 0;

    unsigned int getWritePosition() const;
    void setWritePosition(unsigned int writePosition);

    bool discard();

    bool setBufferSize(unsigned int size);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBufferInterface);

    rx::Renderer *const mRenderer;

    VertexBuffer* mVertexBuffer;

    unsigned int mWritePosition;
    unsigned int mReservedSpace;
    bool mDynamic;
};

class StreamingVertexBufferInterface : public VertexBufferInterface
{
  public:
    StreamingVertexBufferInterface(rx::Renderer *renderer, std::size_t initialSize);
    ~StreamingVertexBufferInterface();

  protected:
    bool reserveSpace(unsigned int size);
};

class StaticVertexBufferInterface : public VertexBufferInterface
{
  public:
    explicit StaticVertexBufferInterface(rx::Renderer *renderer);
    ~StaticVertexBufferInterface();

    int storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count, GLsizei instances);

    // Returns the offset into the vertex buffer, or -1 if not found
    int lookupAttribute(const gl::VertexAttribute &attribute);

  protected:
    bool reserveSpace(unsigned int size);

  private:
    struct VertexElement
    {
        GLenum type;
        GLint size;
        GLsizei stride;
        bool normalized;
        int attributeOffset;

        unsigned int streamOffset;
    };

    std::vector<VertexElement> mCache;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER_H_