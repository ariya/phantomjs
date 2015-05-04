//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer9.h: Defines the rx::Buffer9 class which implements rx::BufferImpl via rx::BufferD3D.

#ifndef LIBGLESV2_RENDERER_BUFFER9_H_
#define LIBGLESV2_RENDERER_BUFFER9_H_

#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class Renderer9;

class Buffer9 : public BufferD3D
{
  public:
    Buffer9(Renderer9 *renderer);
    virtual ~Buffer9();

    static Buffer9 *makeBuffer9(BufferImpl *buffer);

    // BufferD3D implementation
    virtual size_t getSize() const { return mSize; }
    virtual bool supportsDirectBinding() const { return false; }
    RendererD3D *getRenderer() override;

    // BufferImpl implementation
    virtual gl::Error setData(const void* data, size_t size, GLenum usage);
    gl::Error getData(const uint8_t **outData) override;
    virtual gl::Error setSubData(const void* data, size_t size, size_t offset);
    virtual gl::Error copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size);
    virtual gl::Error map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr);
    virtual gl::Error unmap();
    virtual void markTransformFeedbackUsage();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer9);

    Renderer9 *mRenderer;
    MemoryBuffer mMemory;
    size_t mSize;
};

}

#endif // LIBGLESV2_RENDERER_BUFFER9_H_
