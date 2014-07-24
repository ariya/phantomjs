//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Config.cpp: Implements the egl::Config class, describing the format, type
// and size for an egl::Surface. Implements EGLConfig and related functionality.
// [EGL 1.4] section 3.4 page 15.

#include "libEGL/Config.h"

#include <algorithm>
#include <vector>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "common/debug.h"

using namespace std;

namespace egl
{
Config::Config(rx::ConfigDesc desc, EGLint minInterval, EGLint maxInterval, EGLint texWidth, EGLint texHeight)
    : mRenderTargetFormat(desc.renderTargetFormat), mDepthStencilFormat(desc.depthStencilFormat), mMultiSample(desc.multiSample)
{
    mBindToTextureRGB = EGL_FALSE;
    mBindToTextureRGBA = EGL_FALSE;
    switch (desc.renderTargetFormat)
    {
      case GL_RGB5_A1:
        mBufferSize = 16;
        mRedSize = 5;
        mGreenSize = 5;
        mBlueSize = 5;
        mAlphaSize = 1;
        break;
      case GL_RGBA8_OES:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 8;
        mBindToTextureRGBA = true;
        break;
      case GL_RGB565:
        mBufferSize = 16;
        mRedSize = 5;
        mGreenSize = 6;
        mBlueSize = 5;
        mAlphaSize = 0;
        break;
      case GL_RGB8_OES:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 0;
        mBindToTextureRGB = true;
        break;
      case GL_BGRA8_EXT:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 8;
        mBindToTextureRGBA = true;
        break;
      default:
        UNREACHABLE();   // Other formats should not be valid
    }

    mLuminanceSize = 0;
    mAlphaMaskSize = 0;
    mColorBufferType = EGL_RGB_BUFFER;
    mConfigCaveat = (desc.fastConfig) ? EGL_NONE : EGL_SLOW_CONFIG;
    mConfigID = 0;
    mConformant = EGL_OPENGL_ES2_BIT;

    switch (desc.depthStencilFormat)
    {
      case GL_NONE:
        mDepthSize = 0;
        mStencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT32_OES:
        mDepthSize = 32;
        mStencilSize = 0;
        break;
      case GL_DEPTH24_STENCIL8_OES:
        mDepthSize = 24;
        mStencilSize = 8;
        break;
      case GL_DEPTH_COMPONENT24_OES:
        mDepthSize = 24;
        mStencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT16:
        mDepthSize = 16;
        mStencilSize = 0;
        break;
      default:
        UNREACHABLE();
    }

    mLevel = 0;
    mMatchNativePixmap = EGL_NONE;
    mMaxPBufferWidth = texWidth;
    mMaxPBufferHeight = texHeight;
    mMaxPBufferPixels = texWidth*texHeight;
    mMaxSwapInterval = maxInterval;
    mMinSwapInterval = minInterval;
    mNativeRenderable = EGL_FALSE;
    mNativeVisualID = 0;
    mNativeVisualType = 0;
    mRenderableType = EGL_OPENGL_ES2_BIT;
    mSampleBuffers = desc.multiSample ? 1 : 0;
    mSamples = desc.multiSample;
    mSurfaceType = EGL_PBUFFER_BIT | EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
    mTransparentType = EGL_NONE;
    mTransparentRedValue = 0;
    mTransparentGreenValue = 0;
    mTransparentBlueValue = 0;
}

EGLConfig Config::getHandle() const
{
    return (EGLConfig)(size_t)mConfigID;
}

SortConfig::SortConfig(const EGLint *attribList)
    : mWantRed(false), mWantGreen(false), mWantBlue(false), mWantAlpha(false), mWantLuminance(false)
{
    scanForWantedComponents(attribList);
}

void SortConfig::scanForWantedComponents(const EGLint *attribList)
{
    // [EGL] section 3.4.1 page 24
    // Sorting rule #3: by larger total number of color bits, not considering
    // components that are 0 or don't-care.
    for (const EGLint *attr = attribList; attr[0] != EGL_NONE; attr += 2)
    {
        if (attr[1] != 0 && attr[1] != EGL_DONT_CARE)
        {
            switch (attr[0])
            {
              case EGL_RED_SIZE:       mWantRed = true; break;
              case EGL_GREEN_SIZE:     mWantGreen = true; break;
              case EGL_BLUE_SIZE:      mWantBlue = true; break;
              case EGL_ALPHA_SIZE:     mWantAlpha = true; break;
              case EGL_LUMINANCE_SIZE: mWantLuminance = true; break;
            }
        }
    }
}

EGLint SortConfig::wantedComponentsSize(const Config &config) const
{
    EGLint total = 0;

    if (mWantRed)       total += config.mRedSize;
    if (mWantGreen)     total += config.mGreenSize;
    if (mWantBlue)      total += config.mBlueSize;
    if (mWantAlpha)     total += config.mAlphaSize;
    if (mWantLuminance) total += config.mLuminanceSize;

    return total;
}

bool SortConfig::operator()(const Config *x, const Config *y) const
{
    return (*this)(*x, *y);
}

bool SortConfig::operator()(const Config &x, const Config &y) const
{
    #define SORT(attribute)                        \
        if (x.attribute != y.attribute)            \
        {                                          \
            return x.attribute < y.attribute;      \
        }

    META_ASSERT(EGL_NONE < EGL_SLOW_CONFIG && EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG);
    SORT(mConfigCaveat);

    META_ASSERT(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER);
    SORT(mColorBufferType);

    // By larger total number of color bits, only considering those that are requested to be > 0.
    EGLint xComponentsSize = wantedComponentsSize(x);
    EGLint yComponentsSize = wantedComponentsSize(y);
    if (xComponentsSize != yComponentsSize)
    {
        return xComponentsSize > yComponentsSize;
    }

    SORT(mBufferSize);
    SORT(mSampleBuffers);
    SORT(mSamples);
    SORT(mDepthSize);
    SORT(mStencilSize);
    SORT(mAlphaMaskSize);
    SORT(mNativeVisualType);
    SORT(mConfigID);

    #undef SORT

    return false;
}

// We'd like to use SortConfig to also eliminate duplicate configs.
// This works as long as we never have two configs with different per-RGB-component layouts,
// but the same total.
// 5551 and 565 are different because R+G+B is different.
// 5551 and 555 are different because bufferSize is different.
const EGLint ConfigSet::mSortAttribs[] =
{
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_LUMINANCE_SIZE, 1,
    // BUT NOT ALPHA
    EGL_NONE
};

ConfigSet::ConfigSet()
    : mSet(SortConfig(mSortAttribs))
{
}

void ConfigSet::add(rx::ConfigDesc desc, EGLint minSwapInterval, EGLint maxSwapInterval, EGLint texWidth, EGLint texHeight)
{
    Config config(desc, minSwapInterval, maxSwapInterval, texWidth, texHeight);
    mSet.insert(config);
}

size_t ConfigSet::size() const
{
    return mSet.size();
}

bool ConfigSet::getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig)
{
    vector<const Config*> passed;
    passed.reserve(mSet.size());

    for (Iterator config = mSet.begin(); config != mSet.end(); config++)
    {
        bool match = true;
        const EGLint *attribute = attribList;

        while (attribute[0] != EGL_NONE)
        {
            switch (attribute[0])
            {
              case EGL_BUFFER_SIZE:               match = config->mBufferSize >= attribute[1];                      break;
              case EGL_ALPHA_SIZE:                match = config->mAlphaSize >= attribute[1];                       break;
              case EGL_BLUE_SIZE:                 match = config->mBlueSize >= attribute[1];                        break;
              case EGL_GREEN_SIZE:                match = config->mGreenSize >= attribute[1];                       break;
              case EGL_RED_SIZE:                  match = config->mRedSize >= attribute[1];                         break;
              case EGL_DEPTH_SIZE:                match = config->mDepthSize >= attribute[1];                       break;
              case EGL_STENCIL_SIZE:              match = config->mStencilSize >= attribute[1];                     break;
              case EGL_CONFIG_CAVEAT:             match = config->mConfigCaveat == (EGLenum) attribute[1];          break;
              case EGL_CONFIG_ID:                 match = config->mConfigID == attribute[1];                        break;
              case EGL_LEVEL:                     match = config->mLevel >= attribute[1];                           break;
              case EGL_NATIVE_RENDERABLE:         match = config->mNativeRenderable == (EGLBoolean) attribute[1];   break;
              case EGL_NATIVE_VISUAL_TYPE:        match = config->mNativeVisualType == attribute[1];                break;
              case EGL_SAMPLES:                   match = config->mSamples >= attribute[1];                         break;
              case EGL_SAMPLE_BUFFERS:            match = config->mSampleBuffers >= attribute[1];                   break;
              case EGL_SURFACE_TYPE:              match = (config->mSurfaceType & attribute[1]) == attribute[1];    break;
              case EGL_TRANSPARENT_TYPE:          match = config->mTransparentType == (EGLenum) attribute[1];       break;
              case EGL_TRANSPARENT_BLUE_VALUE:    match = config->mTransparentBlueValue == attribute[1];            break;
              case EGL_TRANSPARENT_GREEN_VALUE:   match = config->mTransparentGreenValue == attribute[1];           break;
              case EGL_TRANSPARENT_RED_VALUE:     match = config->mTransparentRedValue == attribute[1];             break;
              case EGL_BIND_TO_TEXTURE_RGB:       match = config->mBindToTextureRGB == (EGLBoolean) attribute[1];   break;
              case EGL_BIND_TO_TEXTURE_RGBA:      match = config->mBindToTextureRGBA == (EGLBoolean) attribute[1];  break;
              case EGL_MIN_SWAP_INTERVAL:         match = config->mMinSwapInterval == attribute[1];                 break;
              case EGL_MAX_SWAP_INTERVAL:         match = config->mMaxSwapInterval == attribute[1];                 break;
              case EGL_LUMINANCE_SIZE:            match = config->mLuminanceSize >= attribute[1];                   break;
              case EGL_ALPHA_MASK_SIZE:           match = config->mAlphaMaskSize >= attribute[1];                   break;
              case EGL_COLOR_BUFFER_TYPE:         match = config->mColorBufferType == (EGLenum) attribute[1];       break;
              case EGL_RENDERABLE_TYPE:           match = (config->mRenderableType & attribute[1]) == attribute[1]; break;
              case EGL_MATCH_NATIVE_PIXMAP:       match = false; UNIMPLEMENTED();                                   break;
              case EGL_CONFORMANT:                match = (config->mConformant & attribute[1]) == attribute[1];     break;
              case EGL_MAX_PBUFFER_WIDTH:         match = config->mMaxPBufferWidth >= attribute[1];                 break;
              case EGL_MAX_PBUFFER_HEIGHT:        match = config->mMaxPBufferHeight >= attribute[1];                break;
              case EGL_MAX_PBUFFER_PIXELS:        match = config->mMaxPBufferPixels >= attribute[1];                break;
              default:
                return false;
            }

            if (!match)
            {
                break;
            }

            attribute += 2;
        }

        if (match)
        {
            passed.push_back(&*config);
        }
    }

    if (configs)
    {
        sort(passed.begin(), passed.end(), SortConfig(attribList));

        EGLint index;
        for (index = 0; index < configSize && index < static_cast<EGLint>(passed.size()); index++)
        {
            configs[index] = passed[index]->getHandle();
        }

        *numConfig = index;
    }
    else
    {
        *numConfig = passed.size();
    }

    return true;
}

const egl::Config *ConfigSet::get(EGLConfig configHandle)
{
    for (Iterator config = mSet.begin(); config != mSet.end(); config++)
    {
        if (config->getHandle() == configHandle)
        {
            return &(*config);
        }
    }

    return NULL;
}
}
