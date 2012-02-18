/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "WebGLTexture.h"

#include "WebGLFramebuffer.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

PassRefPtr<WebGLTexture> WebGLTexture::create(WebGLRenderingContext* ctx)
{
    return adoptRef(new WebGLTexture(ctx));
}

WebGLTexture::WebGLTexture(WebGLRenderingContext* ctx)
    : WebGLObject(ctx)
    , m_target(0)
    , m_minFilter(GraphicsContext3D::NEAREST_MIPMAP_LINEAR)
    , m_magFilter(GraphicsContext3D::LINEAR)
    , m_wrapS(GraphicsContext3D::REPEAT)
    , m_wrapT(GraphicsContext3D::REPEAT)
    , m_isNPOT(false)
    , m_isComplete(false)
    , m_needToUseBlackTexture(false)
{
    setObject(context()->graphicsContext3D()->createTexture());
}

void WebGLTexture::setTarget(GC3Denum target, GC3Dint maxLevel)
{
    if (!object())
        return;
    // Target is finalized the first time bindTexture() is called.
    if (m_target)
        return;
    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        m_target = target;
        m_info.resize(1);
        m_info[0].resize(maxLevel);
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP:
        m_target = target;
        m_info.resize(6);
        for (int ii = 0; ii < 6; ++ii)
            m_info[ii].resize(maxLevel);
        break;
    }
}

void WebGLTexture::setParameteri(GC3Denum pname, GC3Dint param)
{
    if (!object() || !m_target)
        return;
    switch (pname) {
    case GraphicsContext3D::TEXTURE_MIN_FILTER:
        switch (param) {
        case GraphicsContext3D::NEAREST:
        case GraphicsContext3D::LINEAR:
        case GraphicsContext3D::NEAREST_MIPMAP_NEAREST:
        case GraphicsContext3D::LINEAR_MIPMAP_NEAREST:
        case GraphicsContext3D::NEAREST_MIPMAP_LINEAR:
        case GraphicsContext3D::LINEAR_MIPMAP_LINEAR:
            m_minFilter = param;
            break;
        }
        break;
    case GraphicsContext3D::TEXTURE_MAG_FILTER:
        switch (param) {
        case GraphicsContext3D::NEAREST:
        case GraphicsContext3D::LINEAR:
            m_magFilter = param;
            break;
        }
        break;
    case GraphicsContext3D::TEXTURE_WRAP_S:
        switch (param) {
        case GraphicsContext3D::CLAMP_TO_EDGE:
        case GraphicsContext3D::MIRRORED_REPEAT:
        case GraphicsContext3D::REPEAT:
            m_wrapS = param;
            break;
        }
        break;
    case GraphicsContext3D::TEXTURE_WRAP_T:
        switch (param) {
        case GraphicsContext3D::CLAMP_TO_EDGE:
        case GraphicsContext3D::MIRRORED_REPEAT:
        case GraphicsContext3D::REPEAT:
            m_wrapT = param;
            break;
        }
        break;
    default:
        return;
    }
    update();
}

void WebGLTexture::setParameterf(GC3Denum pname, GC3Dfloat param)
{
    if (!object() || !m_target)
        return;
    GC3Dint iparam = static_cast<GC3Dint>(param);
    setParameteri(pname, iparam);
}

void WebGLTexture::setLevelInfo(GC3Denum target, GC3Dint level, GC3Denum internalFormat, GC3Dsizei width, GC3Dsizei height, GC3Denum type)
{
    if (!object() || !m_target)
        return;
    // We assume level, internalFormat, width, height, and type have all been
    // validated already.
    int index = mapTargetToIndex(target);
    if (index < 0)
        return;
    m_info[index][level].setInfo(internalFormat, width, height, type);
    update();
}

void WebGLTexture::generateMipmapLevelInfo()
{
    if (!object() || !m_target)
        return;
    if (!canGenerateMipmaps())
        return;
    if (!m_isComplete) {
        for (size_t ii = 0; ii < m_info.size(); ++ii) {
            const LevelInfo& info0 = m_info[ii][0];
            GC3Dsizei width = info0.width;
            GC3Dsizei height = info0.height;
            GC3Dint levelCount = computeLevelCount(width, height);
            for (GC3Dint level = 1; level < levelCount; ++level) {
                width = std::max(1, width >> 1);
                height = std::max(1, height >> 1);
                LevelInfo& info = m_info[ii][level];
                info.setInfo(info0.internalFormat, width, height, info0.type);
            }
        }
        m_isComplete = true;
    }
    m_needToUseBlackTexture = false;
}

GC3Denum WebGLTexture::getInternalFormat(GC3Denum target, GC3Dint level) const
{
    const LevelInfo* info = getLevelInfo(target, level);
    if (!info)
        return 0;
    return info->internalFormat;
}

GC3Denum WebGLTexture::getType(GC3Denum target, GC3Dint level) const
{
    const LevelInfo* info = getLevelInfo(target, level);
    if (!info)
        return 0;
    return info->type;
}

GC3Dsizei WebGLTexture::getWidth(GC3Denum target, GC3Dint level) const
{
    const LevelInfo* info = getLevelInfo(target, level);
    if (!info)
        return 0;
    return info->width;
}

GC3Dsizei WebGLTexture::getHeight(GC3Denum target, GC3Dint level) const
{
    const LevelInfo* info = getLevelInfo(target, level);
    if (!info)
        return 0;
    return info->height;
}

bool WebGLTexture::isNPOT(GC3Dsizei width, GC3Dsizei height)
{
    ASSERT(width >= 0 && height >= 0);
    if (!width || !height)
        return false;
    if ((width & (width - 1)) || (height & (height - 1)))
        return true;
    return false;
}

bool WebGLTexture::isNPOT() const
{
    if (!object())
        return false;
    return m_isNPOT;
}

bool WebGLTexture::needToUseBlackTexture() const
{
    if (!object())
        return false;
    return m_needToUseBlackTexture;
}

void WebGLTexture::deleteObjectImpl(Platform3DObject object)
{
    context()->graphicsContext3D()->deleteTexture(object);
}

int WebGLTexture::mapTargetToIndex(GC3Denum target) const
{
    if (m_target == GraphicsContext3D::TEXTURE_2D) {
        if (target == GraphicsContext3D::TEXTURE_2D)
            return 0;
    } else if (m_target == GraphicsContext3D::TEXTURE_CUBE_MAP) {
        switch (target) {
        case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
            return 0;
        case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
            return 1;
        case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
            return 2;
        case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
            return 3;
        case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
            return 4;
        case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return 5;
        }
    }
    return -1;
}

bool WebGLTexture::canGenerateMipmaps()
{
    if (isNPOT())
        return false;
    const LevelInfo& first = m_info[0][0];
    for (size_t ii = 0; ii < m_info.size(); ++ii) {
        const LevelInfo& info = m_info[ii][0];
        if (!info.valid
            || info.width != first.width || info.height != first.height
            || info.internalFormat != first.internalFormat || info.type != first.type)
            return false;
    }
    return true;
}

GC3Dint WebGLTexture::computeLevelCount(GC3Dsizei width, GC3Dsizei height)
{
    // return 1 + log2Floor(std::max(width, height));
    GC3Dsizei n = std::max(width, height);
    if (n <= 0)
        return 0;
    GC3Dint log = 0;
    GC3Dsizei value = n;
    for (int ii = 4; ii >= 0; --ii) {
        int shift = (1 << ii);
        GC3Dsizei x = (value >> shift);
        if (x) {
            value = x;
            log += shift;
        }
    }
    ASSERT(value == 1);
    return log + 1;
}

void WebGLTexture::update()
{
    m_isNPOT = false;
    for (size_t ii = 0; ii < m_info.size(); ++ii) {
        if (isNPOT(m_info[ii][0].width, m_info[ii][0].height)) {
            m_isNPOT = true;
            break;
        }
    }
    m_isComplete = true;
    const LevelInfo& first = m_info[0][0];
    GC3Dint levelCount = computeLevelCount(first.width, first.height);
    if (levelCount < 1)
        m_isComplete = false;
    else {
        for (size_t ii = 0; ii < m_info.size() && m_isComplete; ++ii) {
            const LevelInfo& info0 = m_info[ii][0];
            if (!info0.valid
                || info0.width != first.width || info0.height != first.height
                || info0.internalFormat != first.internalFormat || info0.type != first.type) {
                m_isComplete = false;
                break;
            }
            GC3Dsizei width = info0.width;
            GC3Dsizei height = info0.height;
            for (GC3Dint level = 1; level < levelCount; ++level) {
                width = std::max(1, width >> 1);
                height = std::max(1, height >> 1);
                const LevelInfo& info = m_info[ii][level];
                if (!info.valid
                    || info.width != width || info.height != height
                    || info.internalFormat != info0.internalFormat || info.type != info0.type) {
                    m_isComplete = false;
                    break;
                }

            }
        }
    }

    m_needToUseBlackTexture = false;
    // NPOT
    if (m_isNPOT && ((m_minFilter != GraphicsContext3D::NEAREST && m_minFilter != GraphicsContext3D::LINEAR)
                     || m_wrapS != GraphicsContext3D::CLAMP_TO_EDGE || m_wrapT != GraphicsContext3D::CLAMP_TO_EDGE))
        m_needToUseBlackTexture = true;
    // Completeness
    if (!m_isComplete && m_minFilter != GraphicsContext3D::NEAREST && m_minFilter != GraphicsContext3D::LINEAR)
        m_needToUseBlackTexture = true;
}

const WebGLTexture::LevelInfo* WebGLTexture::getLevelInfo(GC3Denum target, GC3Dint level) const
{
    if (!object() || !m_target)
        return 0;
    int targetIndex = mapTargetToIndex(target);
    if (targetIndex < 0 || targetIndex >= static_cast<int>(m_info.size()))
        return 0;
    if (level < 0 || level >= static_cast<GC3Dint>(m_info[targetIndex].size()))
        return 0;
    return &(m_info[targetIndex][level]);
}

}

#endif // ENABLE(WEBGL)
