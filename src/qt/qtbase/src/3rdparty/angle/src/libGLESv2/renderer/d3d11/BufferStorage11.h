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
class Renderer;
class Renderer11;
class DirectBufferStorage11;

enum BufferUsage
{
    BUFFER_USAGE_VERTEX,
    BUFFER_USAGE_INDEX,
};

class BufferStorage11 : public BufferStorage
{
  public:
    explicit BufferStorage11(Renderer11 *renderer);
    virtual ~BufferStorage11();

    static BufferStorage11 *makeBufferStorage11(BufferStorage *bufferStorage);

    virtual void *getData();
    virtual void setData(const void* data, unsigned int size, unsigned int offset);
    virtual void copyData(BufferStorage* sourceStorage, unsigned int size,
                          unsigned int sourceOffset, unsigned int destOffset);
    virtual void clear();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;

    ID3D11Buffer *getBuffer(BufferUsage usage);

  private:
    Renderer11 *mRenderer;

    ID3D11Buffer *mStagingBuffer;
    unsigned int mStagingBufferSize;

    std::map<BufferUsage, DirectBufferStorage11*> mDirectBuffers;

    unsigned int mSize;

    void *mResolvedData;
    unsigned int mResolvedDataSize;
    bool mResolvedDataValid;

    unsigned int mReadUsageCount;
    unsigned int mWriteUsageCount;

    void markBufferUsage();
};

// Each instance of BufferStorageD3DBuffer11 is specialized for a class of D3D binding points
// - vertex buffers
// - index buffers
class DirectBufferStorage11
{
  public:
    DirectBufferStorage11(Renderer11 *renderer, BufferUsage usage);
    ~DirectBufferStorage11();

    BufferUsage getUsage() const;
    bool updateFromStagingBuffer(ID3D11Buffer *stagingBuffer, size_t size, size_t offset);

    ID3D11Buffer *getD3DBuffer() { return mDirectBuffer; }
    bool isDirty() const { return mDirty; }
    void markDirty() { mDirty = true; }

  private:
    Renderer11 *mRenderer;
    const BufferUsage mUsage;
    ID3D11Buffer *mDirectBuffer;
    size_t mBufferSize;
    bool mDirty;

    static void fillBufferDesc(D3D11_BUFFER_DESC* bufferDesc, Renderer *renderer, BufferUsage usage, unsigned int bufferSize);
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_
