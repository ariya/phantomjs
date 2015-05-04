//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferImpl.h: Defines the abstract rx::BufferImpl class.

#ifndef LIBGLESV2_RENDERER_BUFFERD3D_H_
#define LIBGLESV2_RENDERER_BUFFERD3D_H_

#include "libGLESv2/renderer/BufferImpl.h"
#include "libGLESv2/angletypes.h"

#include <cstdint>

namespace rx
{
class RendererD3D;
class StaticIndexBufferInterface;
class StaticVertexBufferInterface;

class BufferD3D : public BufferImpl
{
  public:
    BufferD3D();
    virtual ~BufferD3D();

    static BufferD3D *makeBufferD3D(BufferImpl *buffer);
    static BufferD3D *makeFromBuffer(gl::Buffer *buffer);

    unsigned int getSerial() const { return mSerial; }

    virtual gl::Error getData(const uint8_t **outData) = 0;
    virtual size_t getSize() const = 0;
    virtual bool supportsDirectBinding() const = 0;
    virtual RendererD3D *getRenderer() = 0;

    StaticVertexBufferInterface *getStaticVertexBuffer() { return mStaticVertexBuffer; }
    StaticIndexBufferInterface *getStaticIndexBuffer() { return mStaticIndexBuffer; }

    void initializeStaticData();
    void invalidateStaticData();
    void promoteStaticUsage(int dataSize);

  protected:
    unsigned int mSerial;
    static unsigned int mNextSerial;

    void updateSerial();

    StaticVertexBufferInterface *mStaticVertexBuffer;
    StaticIndexBufferInterface *mStaticIndexBuffer;
    unsigned int mUnmodifiedDataUse;
};

}

#endif // LIBGLESV2_RENDERER_BUFFERIMPLD3D_H_
