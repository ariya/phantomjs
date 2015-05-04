//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Sampler.cpp : Implements the Sampler class, which represents a GLES 3
// sampler object. Sampler objects store some state needed to sample textures.

#include "libGLESv2/Sampler.h"
#include "libGLESv2/angletypes.h"

namespace gl
{

Sampler::Sampler(GLuint id)
    : RefCountObject(id),
      mMinFilter(GL_NEAREST_MIPMAP_LINEAR),
      mMagFilter(GL_LINEAR),
      mWrapS(GL_REPEAT),
      mWrapT(GL_REPEAT),
      mWrapR(GL_REPEAT),
      mMinLod(-1000.0f),
      mMaxLod(1000.0f),
      mComparisonMode(GL_NONE),
      mComparisonFunc(GL_LEQUAL)
{
}

void Sampler::getState(SamplerState *samplerState) const
{
    samplerState->minFilter   = mMinFilter;
    samplerState->magFilter   = mMagFilter;
    samplerState->wrapS       = mWrapS;
    samplerState->wrapT       = mWrapT;
    samplerState->wrapR       = mWrapR;
    samplerState->minLod      = mMinLod;
    samplerState->maxLod      = mMaxLod;
    samplerState->compareMode = mComparisonMode;
    samplerState->compareFunc = mComparisonFunc;
}

}
