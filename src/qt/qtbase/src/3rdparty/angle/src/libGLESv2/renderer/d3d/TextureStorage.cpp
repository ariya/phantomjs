//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Shared members of abstract rx::TextureStorage class.

#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"

#include "common/debug.h"
#include "common/mathutil.h"

namespace rx
{

TextureStorage::TextureStorage()
    : mFirstRenderTargetSerial(0),
      mRenderTargetSerialsLayerStride(0)
{}

void TextureStorage::initializeSerials(unsigned int rtSerialsToReserve, unsigned int rtSerialsLayerStride)
{
    mFirstRenderTargetSerial = RenderTarget::issueSerials(rtSerialsToReserve);
    mRenderTargetSerialsLayerStride = rtSerialsLayerStride;
}

unsigned int TextureStorage::getRenderTargetSerial(const gl::ImageIndex &index) const
{
    unsigned int layerOffset = (index.hasLayer() ? (static_cast<unsigned int>(index.layerIndex) * mRenderTargetSerialsLayerStride) : 0);
    return mFirstRenderTargetSerial + static_cast<unsigned int>(index.mipIndex) + layerOffset;
}

}
