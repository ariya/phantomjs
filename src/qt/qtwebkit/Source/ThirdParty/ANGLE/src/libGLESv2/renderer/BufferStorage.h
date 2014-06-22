//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage.h Defines the abstract BufferStorage class.

#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE_H_

#include "common/angleutils.h"

namespace rx
{

class BufferStorage
{
  public:
    BufferStorage();
    virtual ~BufferStorage();

    // The data returned is only guaranteed valid until next non-const method.
    virtual void *getData() = 0;
    virtual void setData(const void* data, unsigned int size, unsigned int offset) = 0;
    virtual void clear() = 0;
    virtual unsigned int getSize() const = 0;
    virtual bool supportsDirectBinding() const = 0;
    virtual void markBufferUsage();
    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferStorage);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE_H_
