//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable.h: Defines a renderer-agnostic class to contain shader
// executable implementation details.

#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_

#include "common/angleutils.h"
#include "common/debug.h"

#include <vector>
#include <cstdint>

namespace rx
{

class ShaderExecutable
{
  public:
    ShaderExecutable(const void *function, size_t length)
        : mFunctionBuffer(length)
    {
        memcpy(mFunctionBuffer.data(), function, length);
    }

    virtual ~ShaderExecutable() {}

    const uint8_t *getFunction() const
    {
        return mFunctionBuffer.data();
    }

    size_t getLength() const
    {
        return mFunctionBuffer.size();
    }

    const std::string &getDebugInfo() const
    {
        return mDebugInfo;
    }

    void appendDebugInfo(const std::string &info)
    {
        mDebugInfo += info;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable);

    std::vector<uint8_t> mFunctionBuffer;
    std::string mDebugInfo;
};

class UniformStorage
{
  public:
    UniformStorage(size_t initialSize)
        : mSize(initialSize)
    {
    }

    virtual ~UniformStorage() {}

    size_t size() const { return mSize; }

  private:
    size_t mSize;
};

}

#endif // LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_
