#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage.cpp Defines the abstract BufferStorage class.

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{

unsigned int BufferStorage::mNextSerial = 1;

BufferStorage::BufferStorage()
{
    updateSerial();
}

BufferStorage::~BufferStorage()
{
}

unsigned int BufferStorage::getSerial() const
{
    return mSerial;
}

void BufferStorage::updateSerial()
{
    mSerial = mNextSerial++;
}

void BufferStorage::markBufferUsage()
{
}

}
