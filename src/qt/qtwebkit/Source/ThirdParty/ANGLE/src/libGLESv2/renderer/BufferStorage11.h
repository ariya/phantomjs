//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage11.h Defines the BufferStorage11 class.

#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{
class Renderer11;

class BufferStorage11 : public BufferStorage
{
  public:
    explicit BufferStorage11(Renderer11 *renderer);
    virtual ~BufferStorage11();

    static BufferStorage11 *makeBufferStorage11(BufferStorage *bufferStorage);

    virtual void *getData();
    virtual void setData(const void* data, unsigned int size, unsigned int offset);
    virtual void clear();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;
    virtual void markBufferUsage();

    ID3D11Buffer *getBuffer() const;

  private:
    Renderer11 *mRenderer;

    ID3D11Buffer *mStagingBuffer;
    unsigned int mStagingBufferSize;

    ID3D11Buffer *mBuffer;
    unsigned int mBufferSize;

    unsigned int mSize;

    void *mResolvedData;
    unsigned int mResolvedDataSize;
    bool mResolvedDataValid;

    unsigned int mReadUsageCount;
    unsigned int mWriteUsageCount;
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_
