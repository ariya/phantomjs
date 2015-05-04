//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Sampler.h : Defines the Sampler class, which represents a GLES 3
// sampler object. Sampler objects store some state needed to sample textures.

#ifndef LIBGLESV2_SAMPLER_H_
#define LIBGLESV2_SAMPLER_H_

#include "common/RefCountObject.h"

namespace gl
{
struct SamplerState;

class Sampler : public RefCountObject
{
  public:
    Sampler(GLuint id);

    void setMinFilter(GLenum minFilter) { mMinFilter = minFilter; }
    void setMagFilter(GLenum magFilter) { mMagFilter = magFilter; }
    void setWrapS(GLenum wrapS) { mWrapS = wrapS; }
    void setWrapT(GLenum wrapT) { mWrapT = wrapT; }
    void setWrapR(GLenum wrapR) { mWrapR = wrapR; }
    void setMinLod(GLfloat minLod) { mMinLod = minLod; }
    void setMaxLod(GLfloat maxLod) { mMaxLod = maxLod; }
    void setComparisonMode(GLenum comparisonMode) { mComparisonMode = comparisonMode; }
    void setComparisonFunc(GLenum comparisonFunc) { mComparisonFunc = comparisonFunc; }

    GLenum getMinFilter() const { return mMinFilter; }
    GLenum getMagFilter() const { return mMagFilter; }
    GLenum getWrapS() const { return mWrapS; }
    GLenum getWrapT() const { return mWrapT; }
    GLenum getWrapR() const { return mWrapR; }
    GLfloat getMinLod() const { return mMinLod; }
    GLfloat getMaxLod() const { return mMaxLod; }
    GLenum getComparisonMode() const { return mComparisonMode; }
    GLenum getComparisonFunc() const { return mComparisonFunc; }

    void getState(SamplerState *samplerState) const;

  private:
    GLenum mMinFilter;
    GLenum mMagFilter;
    GLenum mWrapS;
    GLenum mWrapT;
    GLenum mWrapR;
    GLfloat mMinLod;
    GLfloat mMaxLod;
    GLenum mComparisonMode;
    GLenum mComparisonFunc;
};

}

#endif // LIBGLESV2_SAMPLER_H_
