//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage9.h Defines the BufferStorage9 class.

#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE9_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE9_H_

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{

class BufferStorage9 : public BufferStorage
{
  public:
    BufferStorage9();
    virtual ~BufferStorage9();

    static BufferStorage9 *makeBufferStorage9(BufferStorage *bufferStorage);

    virtual void *getData();
    virtual void setData(const void* data, unsigned int size, unsigned int offset);
    virtual void clear();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferStorage9);

    void *mMemory;
    unsigned int mAllocatedSize;

    unsigned int mSize;
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE9_H_
