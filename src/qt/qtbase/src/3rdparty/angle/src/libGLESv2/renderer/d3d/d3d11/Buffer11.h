//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer11.h: Defines the rx::Buffer11 class which implements rx::BufferImpl via rx::BufferD3D.

#ifndef LIBGLESV2_RENDERER_BUFFER11_H_
#define LIBGLESV2_RENDERER_BUFFER11_H_

#include "libGLESv2/renderer/d3d/BufferD3D.h"
#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class Renderer11;

enum BufferUsage
{
    BUFFER_USAGE_STAGING,
    BUFFER_USAGE_VERTEX_OR_TRANSFORM_FEEDBACK,
    BUFFER_USAGE_INDEX,
    BUFFER_USAGE_PIXEL_UNPACK,
    BUFFER_USAGE_PIXEL_PACK,
    BUFFER_USAGE_UNIFORM,
};

struct PackPixelsParams
{
    PackPixelsParams();
    PackPixelsParams(const gl::Rectangle &area, GLenum format, GLenum type, GLuint outputPitch,
                     const gl::PixelPackState &pack, ptrdiff_t offset);

    gl::Rectangle area;
    GLenum format;
    GLenum type;
    GLuint outputPitch;
    gl::Buffer *packBuffer;
    gl::PixelPackState pack;
    ptrdiff_t offset;
};

typedef size_t DataRevision;

class Buffer11 : public BufferD3D
{
  public:
    Buffer11(Renderer11 *renderer);
    virtual ~Buffer11();

    static Buffer11 *makeBuffer11(BufferImpl *buffer);

    ID3D11Buffer *getBuffer(BufferUsage usage);
    ID3D11ShaderResourceView *getSRV(DXGI_FORMAT srvFormat);
    bool isMapped() const { return mMappedStorage != NULL; }
    gl::Error packPixels(ID3D11Texture2D *srcTexure, UINT srcSubresource, const PackPixelsParams &params);

    // BufferD3D implementation
    virtual size_t getSize() const { return mSize; }
    virtual bool supportsDirectBinding() const;
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
    DISALLOW_COPY_AND_ASSIGN(Buffer11);

    class BufferStorage11;
    class NativeBuffer11;
    class PackStorage11;

    Renderer11 *mRenderer;
    size_t mSize;

    BufferStorage11 *mMappedStorage;

    std::map<BufferUsage, BufferStorage11*> mBufferStorages;

    typedef std::pair<ID3D11Buffer *, ID3D11ShaderResourceView *> BufferSRVPair;
    std::map<DXGI_FORMAT, BufferSRVPair> mBufferResourceViews;

    MemoryBuffer mResolvedData;
    DataRevision mResolvedDataRevision;
    unsigned int mReadUsageCount;

    void markBufferUsage();
    NativeBuffer11 *getStagingBuffer();
    PackStorage11 *getPackStorage();

    BufferStorage11 *getBufferStorage(BufferUsage usage);
    BufferStorage11 *getLatestBufferStorage() const;
};

}

#endif // LIBGLESV2_RENDERER_BUFFER11_H_
