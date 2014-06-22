#include "precompiled.h"
//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// HandleAllocator.cpp: Implements the gl::HandleAllocator class, which is used
// to allocate GL handles.

#include "libGLESv2/HandleAllocator.h"

#include "libGLESv2/main.h"

namespace gl
{

HandleAllocator::HandleAllocator() : mBaseValue(1), mNextValue(1)
{
}

HandleAllocator::~HandleAllocator()
{
}

void HandleAllocator::setBaseHandle(GLuint value)
{
    ASSERT(mBaseValue == mNextValue);
    mBaseValue = value;
    mNextValue = value;
}

GLuint HandleAllocator::allocate()
{
    if (mFreeValues.size())
    {
        GLuint handle = mFreeValues.back();
        mFreeValues.pop_back();
        return handle;
    }
    return mNextValue++;
}

void HandleAllocator::release(GLuint handle)
{
    if (handle == mNextValue - 1)
    {
        // Don't drop below base value
        if(mNextValue > mBaseValue)
        {
            mNextValue--;
        }
    }
    else
    {
        // Only free handles that we own - don't drop below the base value
        if (handle >= mBaseValue)
        {
            mFreeValues.push_back(handle);
        }
    }
}

}
