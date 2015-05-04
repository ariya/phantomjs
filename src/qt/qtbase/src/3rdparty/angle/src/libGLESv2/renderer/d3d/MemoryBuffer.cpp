//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/renderer/d3d/MemoryBuffer.h"
#include "common/debug.h"

#include <algorithm>
#include <cstdlib>

namespace rx
{

MemoryBuffer::MemoryBuffer()
    : mSize(0),
      mData(NULL)
{
}

MemoryBuffer::~MemoryBuffer()
{
    free(mData);
    mData = NULL;
}

bool MemoryBuffer::resize(size_t size)
{
    if (size == 0)
    {
        free(mData);
        mData = NULL;
        mSize = 0;
    }
    else
    {
        uint8_t *newMemory = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * size));
        if (newMemory == NULL)
        {
            return false;
        }

        if (mData)
        {
            // Copy the intersection of the old data and the new data
            std::copy(mData, mData + std::min(mSize, size), newMemory);
            free(mData);
        }

        mData = newMemory;
        mSize = size;
    }

    return true;
}

size_t MemoryBuffer::size() const
{
    return mSize;
}

const uint8_t *MemoryBuffer::data() const
{
    return mData;
}

uint8_t *MemoryBuffer::data()
{
    ASSERT(mData);
    return mData;
}

}
