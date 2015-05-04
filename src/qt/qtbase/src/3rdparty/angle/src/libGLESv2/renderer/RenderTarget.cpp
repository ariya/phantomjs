//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget.cpp: Implements serial handling for rx::RenderTarget

#include "libGLESv2/renderer/RenderTarget.h"

namespace rx
{
unsigned int RenderTarget::mCurrentSerial = 1;

RenderTarget::RenderTarget()
    : mSerial(issueSerials(1))
{
}

RenderTarget::~RenderTarget()
{
}

unsigned int RenderTarget::getSerial() const
{
    return mSerial;
}

unsigned int RenderTarget::issueSerials(unsigned int count)
{
    unsigned int firstSerial = mCurrentSerial;
    mCurrentSerial += count;
    return firstSerial;
}

}
