/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopengltexture.h"
#include "qopengltexture_p.h"
#include "qopengltexturehelper_p.h"
#include "qopenglfunctions.h"
#include <QtGui/qcolor.h>
#include <QtGui/qopenglcontext.h>
#include <private/qobject_p.h>
#include <private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

//this is to work around GL_TEXTURE_WRAP_R_OES which also has 0x8072 as value
#if !defined(GL_TEXTURE_WRAP_R)
    #define GL_TEXTURE_WRAP_R 0x8072
#endif

QOpenGLTexturePrivate::QOpenGLTexturePrivate(QOpenGLTexture::Target textureTarget,
                                             QOpenGLTexture *qq)
    : q_ptr(qq),
      context(0),
      target(textureTarget),
      textureId(0),
      format(QOpenGLTexture::NoFormat),
      formatClass(QOpenGLTexture::NoFormatClass),
      requestedMipLevels(1),
      mipLevels(-1),
      layers(1),
      faces(1),
      samples(1),
      fixedSamplePositions(true),
      baseLevel(0),
      maxLevel(1000),
      depthStencilMode(QOpenGLTexture::DepthMode),
      minFilter(QOpenGLTexture::Nearest),
      magFilter(QOpenGLTexture::Nearest),
      maxAnisotropy(1.0f),
      minLevelOfDetail(-1000.0f),
      maxLevelOfDetail(1000.0f),
      levelOfDetailBias(0.0f),
      textureView(false),
      autoGenerateMipMaps(true),
      storageAllocated(false),
      texFuncs(0)
{
    dimensions[0] = dimensions[1] = dimensions[2] = 1;

    switch (target) {
    case QOpenGLTexture::Target1D:
        bindingTarget = QOpenGLTexture::BindingTarget1D;
        break;
    case QOpenGLTexture::Target1DArray:
        bindingTarget = QOpenGLTexture::BindingTarget1DArray;
        break;
    case QOpenGLTexture::Target2D:
        bindingTarget = QOpenGLTexture::BindingTarget2D;
        break;
    case QOpenGLTexture::Target2DArray:
        bindingTarget = QOpenGLTexture::BindingTarget2DArray;
        break;
    case QOpenGLTexture::Target3D:
        bindingTarget = QOpenGLTexture::BindingTarget3D;
        break;
    case QOpenGLTexture::TargetCubeMap:
        bindingTarget = QOpenGLTexture::BindingTargetCubeMap;
        break;
    case QOpenGLTexture::TargetCubeMapArray:
        bindingTarget = QOpenGLTexture::BindingTargetCubeMapArray;
        break;
    case QOpenGLTexture::Target2DMultisample:
        bindingTarget = QOpenGLTexture::BindingTarget2DMultisample;
        break;
    case QOpenGLTexture::Target2DMultisampleArray:
        bindingTarget = QOpenGLTexture::BindingTarget2DMultisampleArray;
        break;
    case QOpenGLTexture::TargetRectangle:
        bindingTarget = QOpenGLTexture::BindingTargetRectangle;
        break;
    case QOpenGLTexture::TargetBuffer:
        bindingTarget = QOpenGLTexture::BindingTargetBuffer;
        break;
    }

    swizzleMask[0] = QOpenGLTexture::RedValue;
    swizzleMask[1] = QOpenGLTexture::GreenValue;
    swizzleMask[2] = QOpenGLTexture::BlueValue;
    swizzleMask[3] = QOpenGLTexture::AlphaValue;

    wrapModes[0] = wrapModes[1] = wrapModes[2] = QOpenGLTexture::ClampToEdge;
}

QOpenGLTexturePrivate::~QOpenGLTexturePrivate()
{
    destroy();
}

void QOpenGLTexturePrivate::initializeOpenGLFunctions()
{
    // If we already have a functions object, there is nothing to do
    if (texFuncs)
        return;

    // See if the context already has a suitable resource we can use.
    // If not create a functions object and add it to the context in case
    // others wish to use it too
    texFuncs = context->textureFunctions();
    if (!texFuncs) {
        texFuncs = new QOpenGLTextureHelper(context);
        context->setTextureFunctions(texFuncs);
    }
}

bool QOpenGLTexturePrivate::create()
{
    if (textureId != 0)
        return true;

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("Requires a valid current OpenGL context.\n"
                 "Texture has not been created");
        return false;
    }
    context = ctx;

    // Resolve any functions we will need based upon context version and create the texture
    initializeOpenGLFunctions();

    // What features do we have?
    QOpenGLTexture::Feature feature = QOpenGLTexture::ImmutableStorage;
    while (feature != QOpenGLTexture::MaxFeatureFlag) {
        if (QOpenGLTexture::hasFeature(feature))
            features |= feature;
        feature = static_cast<QOpenGLTexture::Feature>(feature << 1);
    }

    texFuncs->glGenTextures(1, &textureId);
    return textureId != 0;
}

void QOpenGLTexturePrivate::destroy()
{
    if (QOpenGLContext::currentContext() != context) {
        qWarning("Requires a valid current OpenGL context.\n"
                 "Texture has not been destroyed");
        return;
    }

    texFuncs->glDeleteTextures(1, &textureId);

    context = 0;
    textureId = 0;
    format = QOpenGLTexture::NoFormat;
    formatClass = QOpenGLTexture::NoFormatClass;
    requestedMipLevels = 1;
    mipLevels = -1;
    layers = 1;
    faces = 1;
    samples = 1;
    fixedSamplePositions = true,
    baseLevel = 0;
    maxLevel = 1000;
    depthStencilMode = QOpenGLTexture::DepthMode;
    minFilter = QOpenGLTexture::Nearest;
    magFilter = QOpenGLTexture::Nearest;
    maxAnisotropy = 1.0f;
    minLevelOfDetail = -1000.0f;
    maxLevelOfDetail = 1000.0f;
    levelOfDetailBias = 0.0f;
    textureView = false;
    autoGenerateMipMaps = true;
    storageAllocated = false;
    texFuncs = 0;

    swizzleMask[0] = QOpenGLTexture::RedValue;
    swizzleMask[1] = QOpenGLTexture::GreenValue;
    swizzleMask[2] = QOpenGLTexture::BlueValue;
    swizzleMask[3] = QOpenGLTexture::AlphaValue;

    wrapModes[0] = wrapModes[1] = wrapModes[2] = QOpenGLTexture::ClampToEdge;
}

void QOpenGLTexturePrivate::bind()
{
    texFuncs->glBindTexture(target, textureId);
}

void QOpenGLTexturePrivate::bind(uint unit, QOpenGLTexture::TextureUnitReset reset)
{
    GLint oldTextureUnit = 0;
    if (reset == QOpenGLTexture::ResetTextureUnit)
        texFuncs->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTextureUnit);

    texFuncs->glActiveTexture(GL_TEXTURE0 + unit);
    texFuncs->glBindTexture(target, textureId);

    if (reset == QOpenGLTexture::ResetTextureUnit)
        texFuncs->glActiveTexture(GL_TEXTURE0 + oldTextureUnit);
}

void QOpenGLTexturePrivate::release()
{
    texFuncs->glBindTexture(target, 0);
}

void QOpenGLTexturePrivate::release(uint unit, QOpenGLTexture::TextureUnitReset reset)
{
    GLint oldTextureUnit = 0;
    if (reset == QOpenGLTexture::ResetTextureUnit)
        texFuncs->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTextureUnit);

    texFuncs->glActiveTexture(GL_TEXTURE0 + unit);
    texFuncs->glBindTexture(target, 0);

    if (reset == QOpenGLTexture::ResetTextureUnit)
        texFuncs->glActiveTexture(GL_TEXTURE0 + oldTextureUnit);
}

bool QOpenGLTexturePrivate::isBound() const
{
    GLint boundTextureId = 0;
    texFuncs->glGetIntegerv(bindingTarget, &boundTextureId);
    return (static_cast<GLuint>(boundTextureId) == textureId);
}

bool QOpenGLTexturePrivate::isBound(uint unit) const
{
    GLint oldTextureUnit = 0;
    texFuncs->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTextureUnit);

    GLint boundTextureId = 0;
    texFuncs->glActiveTexture(GL_TEXTURE0 + unit);
    texFuncs->glGetIntegerv(bindingTarget, &boundTextureId);
    bool result = (static_cast<GLuint>(boundTextureId) == textureId);

    texFuncs->glActiveTexture(GL_TEXTURE0 + oldTextureUnit);
    return result;
}

int QOpenGLTexturePrivate::evaluateMipLevels() const
{
    switch (target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::Target3D:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
        return qMin(maximumMipLevelCount(), qMax(1, requestedMipLevels));

    case QOpenGLTexture::TargetRectangle:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetBuffer:
    default:
        return 1;
    }
}

void QOpenGLTexturePrivate::allocateStorage()
{
    // Resolve the actual number of mipmap levels we can use
    mipLevels = evaluateMipLevels();

    // Use immutable storage whenever possible, falling back to mutable when not available
    if (features.testFlag(QOpenGLTexture::ImmutableStorage))
        allocateImmutableStorage();
    else
        allocateMutableStorage();
}

void QOpenGLTexturePrivate::allocateMutableStorage()
{
    switch (target) {
    case QOpenGLTexture::TargetBuffer:
        // Buffer textures get their storage from an external OpenGL buffer
        qWarning("Buffer textures do not allocate storage");
        return;

    case QOpenGLTexture::Target1D:
        if (features.testFlag(QOpenGLTexture::Texture1D)) {
            for (int level = 0; level < mipLevels; ++level)
                texFuncs->glTextureImage1D(textureId, target, bindingTarget, level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        } else {
            qWarning("1D textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target1DArray:
        if (features.testFlag(QOpenGLTexture::Texture1D)
                && features.testFlag(QOpenGLTexture::TextureArrays)) {
            for (int level = 0; level < mipLevels; ++level)
                texFuncs->glTextureImage2D(textureId, target, bindingTarget, level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           layers,
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        } else {
            qWarning("1D array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::TargetRectangle:
        for (int level = 0; level < mipLevels; ++level)
            texFuncs->glTextureImage2D(textureId, target, bindingTarget, level, format,
                                       mipLevelSize(level, dimensions[0]),
                                       mipLevelSize(level, dimensions[1]),
                                       0,
                                       QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        break;

    case QOpenGLTexture::TargetCubeMap: {
        // Cubemaps are the odd one out. We have to allocate storage for each
        // face and miplevel using the special cubemap face targets rather than
        // GL_TARGET_CUBEMAP.
        const QOpenGLTexture::CubeMapFace faceTargets[] = {
            QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::CubeMapNegativeX,
            QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::CubeMapNegativeY,
            QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::CubeMapNegativeZ
        };

        for (int faceTarget = 0; faceTarget < 6; ++faceTarget) {
            for (int level = 0; level < mipLevels; ++level) {
                texFuncs->glTextureImage2D(textureId, faceTargets[faceTarget], bindingTarget,
                                           level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           mipLevelSize(level, dimensions[1]),
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
            }
        }
        break;
    }

    case QOpenGLTexture::Target2DArray:
        if (features.testFlag(QOpenGLTexture::TextureArrays)) {
            for (int level = 0; level < mipLevels; ++level)
                texFuncs->glTextureImage3D(textureId, target, bindingTarget, level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           mipLevelSize(level, dimensions[1]),
                                           layers,
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        } else {
            qWarning("Array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::TargetCubeMapArray:
        // Cubemap arrays must specify number of layer-faces (6 * layers) as depth parameter
        if (features.testFlag(QOpenGLTexture::TextureCubeMapArrays)) {
            for (int level = 0; level < mipLevels; ++level)
                texFuncs->glTextureImage3D(textureId, target, bindingTarget, level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           mipLevelSize(level, dimensions[1]),
                                           6 * layers,
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        } else {
            qWarning("Cubemap Array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target3D:
        if (features.testFlag(QOpenGLTexture::Texture3D)) {
            for (int level = 0; level < mipLevels; ++level)
                texFuncs->glTextureImage3D(textureId, target, bindingTarget, level, format,
                                           mipLevelSize(level, dimensions[0]),
                                           mipLevelSize(level, dimensions[1]),
                                           mipLevelSize(level, dimensions[2]),
                                           0,
                                           QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, 0);
        } else {
            qWarning("3D textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2DMultisample:
        if (features.testFlag(QOpenGLTexture::TextureMultisample)) {
            texFuncs->glTextureImage2DMultisample(textureId, target, bindingTarget, samples, format,
                                                  dimensions[0], dimensions[1],
                                                  fixedSamplePositions);
        } else {
            qWarning("Multisample textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2DMultisampleArray:
        if (features.testFlag(QOpenGLTexture::TextureMultisample)
                && features.testFlag(QOpenGLTexture::TextureArrays)) {
            texFuncs->glTextureImage3DMultisample(textureId, target, bindingTarget, samples, format,
                                                  dimensions[0], dimensions[1], layers,
                                                  fixedSamplePositions);
        } else {
            qWarning("Multisample array textures are not supported");
            return;
        }
        break;
    }

    storageAllocated = true;
}

void QOpenGLTexturePrivate::allocateImmutableStorage()
{
    switch (target) {
    case QOpenGLTexture::TargetBuffer:
        // Buffer textures get their storage from an external OpenGL buffer
        qWarning("Buffer textures do not allocate storage");
        return;

    case QOpenGLTexture::Target1D:
        if (features.testFlag(QOpenGLTexture::Texture1D)) {
            texFuncs->glTextureStorage1D(textureId, target, bindingTarget, mipLevels, format,
                                         dimensions[0]);
        } else {
            qWarning("1D textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target1DArray:
        if (features.testFlag(QOpenGLTexture::Texture1D)
                && features.testFlag(QOpenGLTexture::TextureArrays)) {
            texFuncs->glTextureStorage2D(textureId, target, bindingTarget, mipLevels, format,
                                         dimensions[0], layers);
        } else {
            qWarning("1D array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetRectangle:
        texFuncs->glTextureStorage2D(textureId, target, bindingTarget, mipLevels, format,
                                     dimensions[0], dimensions[1]);
        break;

    case QOpenGLTexture::Target2DArray:
        if (features.testFlag(QOpenGLTexture::TextureArrays)) {
            texFuncs->glTextureStorage3D(textureId, target, bindingTarget, mipLevels, format,
                                         dimensions[0], dimensions[1], layers);
        } else {
            qWarning("Array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::TargetCubeMapArray:
        // Cubemap arrays must specify number of layer-faces (6 * layers) as depth parameter
        if (features.testFlag(QOpenGLTexture::TextureCubeMapArrays)) {
            texFuncs->glTextureStorage3D(textureId, target, bindingTarget, mipLevels, format,
                                         dimensions[0], dimensions[1], 6 * layers);
        } else {
            qWarning("Cubemap Array textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target3D:
        if (features.testFlag(QOpenGLTexture::Texture3D)) {
            texFuncs->glTextureStorage3D(textureId, target, bindingTarget, mipLevels, format,
                                         dimensions[0], dimensions[1], dimensions[2]);
        } else {
            qWarning("3D textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2DMultisample:
        if (features.testFlag(QOpenGLTexture::TextureMultisample)) {
            texFuncs->glTextureStorage2DMultisample(textureId, target, bindingTarget, samples, format,
                                                    dimensions[0], dimensions[1],
                                                    fixedSamplePositions);
        } else {
            qWarning("Multisample textures are not supported");
            return;
        }
        break;

    case QOpenGLTexture::Target2DMultisampleArray:
        if (features.testFlag(QOpenGLTexture::TextureMultisample)
                && features.testFlag(QOpenGLTexture::TextureArrays)) {
            texFuncs->glTextureStorage3DMultisample(textureId, target, bindingTarget, samples, format,
                                                    dimensions[0], dimensions[1], layers,
                                                    fixedSamplePositions);
        } else {
            qWarning("Multisample array textures are not supported");
            return;
        }
        break;
    }

    storageAllocated = true;
}

void QOpenGLTexturePrivate::setData(int mipLevel, int layer, QOpenGLTexture::CubeMapFace cubeFace,
                                    QOpenGLTexture::PixelFormat sourceFormat, QOpenGLTexture::PixelType sourceType,
                                    const void *data, const QOpenGLPixelTransferOptions * const options)
{
    switch (target) {
    case QOpenGLTexture::Target1D:
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage1D(textureId, target, bindingTarget, mipLevel,
                                      0, mipLevelSize( mipLevel, dimensions[0] ),
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::Target1DArray:
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage2D(textureId, target, bindingTarget, mipLevel,
                                      0, layer,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      1,
                                      sourceFormat, sourceType, data, options);

    case QOpenGLTexture::Target2D:
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage2D(textureId, target, bindingTarget, mipLevel,
                                      0, 0,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      mipLevelSize(mipLevel, dimensions[1]),
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::Target2DArray:
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                      0, 0, layer,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      mipLevelSize(mipLevel, dimensions[1]),
                                      1,
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::Target3D:
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                      0, 0, layer,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      mipLevelSize(mipLevel, dimensions[1]),
                                      mipLevelSize(mipLevel, dimensions[2]),
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::TargetCubeMap:
        Q_UNUSED(layer);
        texFuncs->glTextureSubImage2D(textureId, cubeFace, bindingTarget, mipLevel,
                                      0, 0,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      mipLevelSize(mipLevel, dimensions[1]),
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::TargetCubeMapArray: {
        int faceIndex = cubeFace - QOpenGLTexture::CubeMapPositiveX;
        int layerFace = 6 * layer + faceIndex;
        texFuncs->glTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                      0, 0, layerFace,
                                      mipLevelSize(mipLevel, dimensions[0]),
                                      mipLevelSize(mipLevel, dimensions[1]),
                                      1,
                                      sourceFormat, sourceType, data, options);
        break;
    }

    case QOpenGLTexture::TargetRectangle:
        Q_UNUSED(mipLevel);
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glTextureSubImage2D(textureId, target, bindingTarget, 0,
                                      0, 0,
                                      dimensions[0],
                                      dimensions[1],
                                      sourceFormat, sourceType, data, options);
        break;

    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetBuffer:
        // We don't upload pixel data for these targets
        qWarning("QOpenGLTexture::setData(): Texture target does not support pixel data upload");
        break;
    }

    // If requested perform automatic mip map generation
    if (mipLevel == 0 && autoGenerateMipMaps && mipLevels > 1) {
        Q_Q(QOpenGLTexture);
        q->generateMipMaps();
    }
}

void QOpenGLTexturePrivate::setCompressedData(int mipLevel, int layer, QOpenGLTexture::CubeMapFace cubeFace,
                                              int dataSize, const void *data,
                                              const QOpenGLPixelTransferOptions * const options)
{
    switch (target) {
    case QOpenGLTexture::Target1D:
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage1D(textureId, target, bindingTarget, mipLevel,
                                                0, mipLevelSize( mipLevel, dimensions[0] ),
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::Target1DArray:
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage2D(textureId, target, bindingTarget, mipLevel,
                                                0, layer,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                1,
                                                format, dataSize, data, options);

    case QOpenGLTexture::Target2D:
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage2D(textureId, target, bindingTarget, mipLevel,
                                                0, 0,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                mipLevelSize(mipLevel, dimensions[1]),
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::Target2DArray:
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                                0, 0, layer,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                mipLevelSize(mipLevel, dimensions[1]),
                                                1,
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::Target3D:
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                                0, 0, layer,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                mipLevelSize(mipLevel, dimensions[1]),
                                                mipLevelSize(mipLevel, dimensions[2]),
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::TargetCubeMap:
        Q_UNUSED(layer);
        texFuncs->glCompressedTextureSubImage2D(textureId, cubeFace, bindingTarget, mipLevel,
                                                0, 0,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                mipLevelSize(mipLevel, dimensions[1]),
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::TargetCubeMapArray: {
        int faceIndex = cubeFace - QOpenGLTexture::CubeMapPositiveX;
        int layerFace = 6 * layer + faceIndex;
        texFuncs->glCompressedTextureSubImage3D(textureId, target, bindingTarget, mipLevel,
                                                0, 0, layerFace,
                                                mipLevelSize(mipLevel, dimensions[0]),
                                                mipLevelSize(mipLevel, dimensions[1]),
                                                1,
                                                format, dataSize, data, options);
        break;
    }

    case QOpenGLTexture::TargetRectangle:
        Q_UNUSED(mipLevel);
        Q_UNUSED(layer);
        Q_UNUSED(cubeFace);
        texFuncs->glCompressedTextureSubImage2D(textureId, target, bindingTarget, 0,
                                                0, 0,
                                                dimensions[0],
                                                dimensions[1],
                                                format, dataSize, data, options);
        break;

    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetBuffer:
        // We don't upload pixel data for these targets
        qWarning("QOpenGLTexture::setCompressedData(): Texture target does not support pixel data upload");
        break;
    }

    // If requested perform automatic mip map generation
    if (mipLevel == 0 && autoGenerateMipMaps && mipLevels > 1) {
        Q_Q(QOpenGLTexture);
        q->generateMipMaps();
    }
}

void QOpenGLTexturePrivate::setWrapMode(QOpenGLTexture::WrapMode mode)
{
    switch (target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::TargetBuffer:
        wrapModes[0] = mode;
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_S, mode);
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetRectangle:
        wrapModes[0] = wrapModes[1] = mode;
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_S, mode);
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_T, mode);
        break;

    case QOpenGLTexture::Target3D:
        wrapModes[0] = wrapModes[1] = wrapModes[2] = mode;
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_S, mode);
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_T, mode);
        texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_R, mode);
        break;
    }
}

void QOpenGLTexturePrivate::setWrapMode(QOpenGLTexture::CoordinateDirection direction, QOpenGLTexture::WrapMode mode)
{
    switch (target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::TargetBuffer:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            wrapModes[0] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_S, mode);
            break;

        case QOpenGLTexture::DirectionT:
        case QOpenGLTexture::DirectionR:
            qWarning("QOpenGLTexture::setWrapMode() direction not valid for this texture target");
            break;
        }
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetRectangle:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            wrapModes[0] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_S, mode);
            break;

        case QOpenGLTexture::DirectionT:
            wrapModes[1] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, GL_TEXTURE_WRAP_T, mode);
            break;

        case QOpenGLTexture::DirectionR:
            qWarning("QOpenGLTexture::setWrapMode() direction not valid for this texture target");
            break;
        }
        break;

    case QOpenGLTexture::Target3D:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            wrapModes[0] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, direction, mode);
            break;

        case QOpenGLTexture::DirectionT:
            wrapModes[1] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, direction, mode);
            break;

        case QOpenGLTexture::DirectionR:
            wrapModes[2] = mode;
            texFuncs->glTextureParameteri(textureId, target, bindingTarget, direction, mode);
            break;
        }
        break;
    }
}

QOpenGLTexture::WrapMode QOpenGLTexturePrivate::wrapMode(QOpenGLTexture::CoordinateDirection direction) const
{
    switch (target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::TargetBuffer:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            return wrapModes[0];

        case QOpenGLTexture::DirectionT:
        case QOpenGLTexture::DirectionR:
            qWarning("QOpenGLTexture::setWrapMode() direction not valid for this texture target");
            return QOpenGLTexture::Repeat;
        }
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
    case QOpenGLTexture::TargetRectangle:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            return wrapModes[0];

        case QOpenGLTexture::DirectionT:
            return wrapModes[1];

        case QOpenGLTexture::DirectionR:
            qWarning("QOpenGLTexture::setWrapMode() direction not valid for this texture target");
            return QOpenGLTexture::Repeat;
        }
        break;

    case QOpenGLTexture::Target3D:
        switch (direction) {
        case QOpenGLTexture::DirectionS:
            return wrapModes[0];

        case QOpenGLTexture::DirectionT:
            return wrapModes[1];

        case QOpenGLTexture::DirectionR:
            return wrapModes[2];
        }
        break;
    }
    // Should never get here
    Q_ASSERT(false);
    return QOpenGLTexture::Repeat;
}

QOpenGLTexture *QOpenGLTexturePrivate::createTextureView(QOpenGLTexture::Target viewTarget,
                                                         QOpenGLTexture::TextureFormat viewFormat,
                                                         int minimumMipmapLevel, int maximumMipmapLevel,
                                                         int minimumLayer, int maximumLayer) const
{
    // Do sanity checks - see http://www.opengl.org/wiki/GLAPI/glTextureView

    // Check the targets are compatible
    bool viewTargetCompatible = false;
    switch (target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::Target1D
                             || viewTarget == QOpenGLTexture::Target1DArray);
        break;


    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::Target2D
                             || viewTarget == QOpenGLTexture::Target2DArray);
        break;

    case QOpenGLTexture::Target3D:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::Target3D);
        break;

    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::TargetCubeMap
                             || viewTarget == QOpenGLTexture::Target2D
                             || viewTarget == QOpenGLTexture::Target2DArray
                             || viewTarget == QOpenGLTexture::TargetCubeMapArray);
        break;

    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::Target2DMultisample
                             || viewTarget == QOpenGLTexture::Target2DMultisampleArray);
        break;

    case QOpenGLTexture::TargetRectangle:
        viewTargetCompatible = (viewTarget == QOpenGLTexture::TargetRectangle);
        break;

    case QOpenGLTexture::TargetBuffer:
        // Cannot be used with texture views
        break;
    }

    if (!viewTargetCompatible) {
        qWarning("QOpenGLTexture::createTextureView(): Incompatible source and view targets");
        return 0;
    }

    // Check the formats are compatible
    bool viewFormatCompatible = false;
    switch (formatClass) {
    case QOpenGLTexture::NoFormatClass:
        break;

    case QOpenGLTexture::FormatClass_128Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGBA32F
                             || viewFormat == QOpenGLTexture::RGBA32U
                             || viewFormat == QOpenGLTexture::RGBA32I);
        break;

    case QOpenGLTexture::FormatClass_96Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB32F
                             || viewFormat == QOpenGLTexture::RGB32U
                             || viewFormat == QOpenGLTexture::RGB32I);
        break;

    case QOpenGLTexture::FormatClass_64Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGBA16F
                             || viewFormat == QOpenGLTexture::RG32F
                             || viewFormat == QOpenGLTexture::RGBA16U
                             || viewFormat == QOpenGLTexture::RG32U
                             || viewFormat == QOpenGLTexture::RGBA16I
                             || viewFormat == QOpenGLTexture::RG32I
                             || viewFormat == QOpenGLTexture::RGBA16_UNorm
                             || viewFormat == QOpenGLTexture::RGBA16_SNorm);
        break;

    case QOpenGLTexture::FormatClass_48Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB16_UNorm
                             || viewFormat == QOpenGLTexture::RGB16_SNorm
                             || viewFormat == QOpenGLTexture::RGB16F
                             || viewFormat == QOpenGLTexture::RGB16U
                             || viewFormat == QOpenGLTexture::RGB16I);
        break;

    case QOpenGLTexture::FormatClass_32Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RG16F
                             || viewFormat == QOpenGLTexture::RG11B10F
                             || viewFormat == QOpenGLTexture::R32F
                             || viewFormat == QOpenGLTexture::RGB10A2
                             || viewFormat == QOpenGLTexture::RGBA8U
                             || viewFormat == QOpenGLTexture::RG16U
                             || viewFormat == QOpenGLTexture::R32U
                             || viewFormat == QOpenGLTexture::RGBA8I
                             || viewFormat == QOpenGLTexture::RG16I
                             || viewFormat == QOpenGLTexture::R32I
                             || viewFormat == QOpenGLTexture::RGBA8_UNorm
                             || viewFormat == QOpenGLTexture::RG16_UNorm
                             || viewFormat == QOpenGLTexture::RGBA8_SNorm
                             || viewFormat == QOpenGLTexture::RG16_SNorm
                             || viewFormat == QOpenGLTexture::SRGB8_Alpha8
                             || viewFormat == QOpenGLTexture::RGB9E5);
        break;

    case QOpenGLTexture::FormatClass_24Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB8_UNorm
                             || viewFormat == QOpenGLTexture::RGB8_SNorm
                             || viewFormat == QOpenGLTexture::SRGB8
                             || viewFormat == QOpenGLTexture::RGB8U
                             || viewFormat == QOpenGLTexture::RGB8I);
        break;

    case QOpenGLTexture::FormatClass_16Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::R16F
                             || viewFormat == QOpenGLTexture::RG8U
                             || viewFormat == QOpenGLTexture::R16U
                             || viewFormat == QOpenGLTexture::RG8I
                             || viewFormat == QOpenGLTexture::R16I
                             || viewFormat == QOpenGLTexture::RG8_UNorm
                             || viewFormat == QOpenGLTexture::R16_UNorm
                             || viewFormat == QOpenGLTexture::RG8_SNorm
                             || viewFormat == QOpenGLTexture::R16_SNorm);
        break;

    case QOpenGLTexture::FormatClass_8Bit:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::R8U
                             || viewFormat == QOpenGLTexture::R8I
                             || viewFormat == QOpenGLTexture::R8_UNorm
                             || viewFormat == QOpenGLTexture::R8_SNorm);
        break;

    case QOpenGLTexture::FormatClass_RGTC1_R:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::R_ATI1N_UNorm
                             || viewFormat == QOpenGLTexture::R_ATI1N_SNorm);
        break;

    case QOpenGLTexture::FormatClass_RGTC2_RG:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RG_ATI2N_UNorm
                             || viewFormat == QOpenGLTexture::RG_ATI2N_SNorm);
        break;

    case QOpenGLTexture::FormatClass_BPTC_Unorm:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB_BP_UNorm
                             || viewFormat == QOpenGLTexture::SRGB_BP_UNorm);
        break;

    case QOpenGLTexture::FormatClass_BPTC_Float:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB_BP_UNSIGNED_FLOAT
                             || viewFormat == QOpenGLTexture::RGB_BP_SIGNED_FLOAT);
        break;

    case QOpenGLTexture::FormatClass_S3TC_DXT1_RGB:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGB_DXT1
                             || viewFormat == QOpenGLTexture::SRGB_DXT1);
        break;

    case QOpenGLTexture::FormatClass_S3TC_DXT1_RGBA:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGBA_DXT1
                             || viewFormat == QOpenGLTexture::SRGB_Alpha_DXT1);
        break;

    case QOpenGLTexture::FormatClass_S3TC_DXT3_RGBA:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGBA_DXT3
                             || viewFormat == QOpenGLTexture::SRGB_Alpha_DXT3);
        break;

    case QOpenGLTexture::FormatClass_S3TC_DXT5_RGBA:
        viewFormatCompatible = (viewFormat == QOpenGLTexture::RGBA_DXT5
                             || viewFormat == QOpenGLTexture::SRGB_Alpha_DXT5);
        break;

    case QOpenGLTexture::FormatClass_Unique:
        viewFormatCompatible = (viewFormat == format);
        break;
    }

    if (!viewFormatCompatible) {
        qWarning("QOpenGLTexture::createTextureView(): Incompatible source and view formats");
        return 0;
    }


    // Create a view
    QOpenGLTexture *view = new QOpenGLTexture(viewTarget);
    view->setFormat(viewFormat);
    view->create();
    view->d_ptr->textureView = true;
    texFuncs->glTextureView(view->textureId(), viewTarget, textureId, viewFormat,
                            minimumMipmapLevel, maximumMipmapLevel - minimumMipmapLevel + 1,
                            minimumLayer, maximumLayer - minimumLayer + 1);
    return view;
}


/*!
    \class QOpenGLTexture
    \inmodule QtGui
    \since 5.2
    \wrapper
    \brief The QOpenGLTexture class encapsulates an OpenGL texture object.

    QOpenGLTexture makes it easy to work with OpenGL textures and the myriad features
    and targets that they offer depending upon the capabilities of your OpenGL implementation.

    The typical usage pattern for QOpenGLTexture is
    \list
        \li Instantiate the object specifying the texture target type
        \li Set properties that affect the storage requirements e.g. storage format, dimensions
        \li Allocate the server-side storage
        \li Optionally upload pixel data
        \li Optionally set any additional properties e.g. filtering and border options
        \li Render with texture or render to texture
    \endlist

    In the common case of simply using a QImage as the source of texture pixel data
    most of the above steps are performed automatically.

    \code
    // Prepare texture
    QOpenGLTexture *texture = new QOpenGLTexture(QImage(fileName).mirrored());
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    ...
    // Render with texture
    texture->bind();
    glDrawArrays(...);
    \endcode

    Note that the QImage is mirrored vertically to account for the fact that
    OpenGL and QImage use opposite directions for the y axis. Another option
    would be to transform your texture coordinates.
*/

/*!
    \enum QOpenGLTexture::Filter
    This enum defines the filtering parameters for a QOpenGLTexture object.
    \value Nearest Equivalent to GL_NEAREST
    \value Linear Equivalent to GL_LINEAR
    \value NearestMipMapNearest Equivalent to GL_NEAREST_MIPMAP_NEAREST
    \value NearestMipMapLinear Equivalent to GL_NEAREST_MIPMAP_LINEAR
    \value LinearMipMapNearest Equivalent to GL_LINEAR_MIPMAP_NEAREST
    \value LinearMipMapLinear Equivalent to GL_LINEAR_MIPMAP_LINEAR
*/

/*!
    \enum QOpenGLTexture::Target
    This enum defines the texture target of a QOpenGLTexture object.

    \value Target1D A 1-dimensional texture.
           Equivalent to GL_TEXTURE_1D.
    \value Target1DArray An array of 1-dimensional textures.
           Equivalent to GL_TEXTURE_1D_ARRAY
    \value Target2D A 2-dimensional texture.
           Equivalent to GL_TEXTURE_2D
    \value Target2DArray An array of 1-dimensional textures.
           Equivalent to GL_TEXTURE_2D_ARRAY
    \value Target3D A 3-dimensional texture.
           Equivalent to GL_TEXTURE_3D
    \value TargetCubeMap A cubemap texture.
           Equivalent to GL_TEXTURE_CUBE_MAP
    \value TargetCubeMapArray An array of cubemap textures.
           Equivalent to GL_TEXTURE_CUBE_MAP_ARRAY
    \value Target2DMultisample A 2-dimensional texture with multisample support.
           Equivalent to GL_TEXTURE_2D_MULTISAMPLE
    \value Target2DMultisampleArray An array of 2-dimensional textures with multisample support.
           Equivalent to GL_TEXTURE_2D_MULTISAMPLE_ARRAY
    \value TargetRectangle A rectangular 2-dimensional texture.
           Equivalent to GL_TEXTURE_RECTANGLE
    \value TargetBuffer A texture with data from an OpenGL buffer object.
           Equivalent to GL_TEXTURE_BUFFER
*/

/*!
    \enum QOpenGLTexture::BindingTarget
    This enum defines the possible binding targets of texture units.

    \value BindingTarget1D Equivalent to GL_TEXTURE_BINDING_1D
    \value BindingTarget1DArray Equivalent to GL_TEXTURE_BINDING_1D_ARRAY
    \value BindingTarget2D Equivalent to GL_TEXTURE_BINDING_2D
    \value BindingTarget2DArray Equivalent to GL_TEXTURE_BINDING_2D_ARRAY
    \value BindingTarget3D Equivalent to GL_TEXTURE_BINDING_3D
    \value BindingTargetCubeMap Equivalent to GL_TEXTURE_BINDING_CUBE_MAP
    \value BindingTargetCubeMapArray Equivalent to GL_TEXTURE_BINDING_CUBE_MAP_ARRAY
    \value BindingTarget2DMultisample Equivalent to GL_TEXTURE_BINDING_2D_MULTISAMPLE
    \value BindingTarget2DMultisampleArray Equivalent to GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY
    \value BindingTargetRectangle Equivalent to GL_TEXTURE_BINDING_RECTANGLE
    \value BindingTargetBuffer Equivalent to GL_TEXTURE_BINDING_BUFFER
*/

/*!
    \enum QOpenGLTexture::MipMapGeneration
    This enum defines the options to control mipmap generation.

    \value GenerateMipMaps Mipmaps should be generated
    \value DontGenerateMipMaps Mipmaps should not be generated
*/

/*!
    \enum QOpenGLTexture::TextureUnitReset
    This enum defines options ot control texture unit activation.

    \value ResetTextureUnit The previous active texture unit will be reset
    \value DontResetTextureUnit The previous active texture unit will not be rest
*/

/*!
    \enum QOpenGLTexture::TextureFormat
    This enum defines the possible texture formats. Depending upon your OpenGL
    implementation only a subset of these may be supported.

    \value NoFormat Equivalent to GL_NONE

    \value R8_UNorm Equivalent to GL_R8
    \value RG8_UNorm Equivalent to GL_RG8
    \value RGB8_UNorm Equivalent to GL_RGB8
    \value RGBA8_UNorm Equivalent to GL_RGBA8

    \value R16_UNorm Equivalent to GL_R16
    \value RG16_UNorm Equivalent to GL_RG16
    \value RGB16_UNorm Equivalent to GL_RGB16
    \value RGBA16_UNorm Equivalent to GL_RGBA16

    \value R8_SNorm Equivalent to GL_R8_SNORM
    \value RG8_SNorm Equivalent to GL_RG8_SNORM
    \value RGB8_SNorm Equivalent to GL_RGB8_SNORM
    \value RGBA8_SNorm Equivalent to GL_RGBA8_SNORM

    \value R16_SNorm Equivalent to GL_R16_SNORM
    \value RG16_SNorm Equivalent to GL_RG16_SNORM
    \value RGB16_SNorm Equivalent to GL_RGB16_SNORM
    \value RGBA16_SNorm Equivalent to GL_RGBA16_SNORM

    \value R8U Equivalent to GL_R8UI
    \value RG8U Equivalent to GL_RG8UI
    \value RGB8U Equivalent to GL_RGB8UI
    \value RGBA8U Equivalent to GL_RGBA8UI

    \value R16U Equivalent to GL_R16UI
    \value RG16U Equivalent to GL_RG16UI
    \value RGB16U Equivalent to GL_RGB16UI
    \value RGBA16U Equivalent to GL_RGBA16UI

    \value R32U Equivalent to GL_R32UI
    \value RG32U Equivalent to GL_RG32UI
    \value RGB32U Equivalent to GL_RGB32UI
    \value RGBA32U Equivalent to GL_RGBA32UI

    \value R8I Equivalent to GL_R8I
    \value RG8I Equivalent to GL_RG8I
    \value RGB8I Equivalent to GL_RGB8I
    \value RGBA8I Equivalent to GL_RGBA8I

    \value R16I Equivalent to GL_R16I
    \value RG16I Equivalent to GL_RG16I
    \value RGB16I Equivalent to GL_RGB16I
    \value RGBA16I Equivalent to GL_RGBA16I

    \value R32I Equivalent to GL_R32I
    \value RG32I Equivalent to GL_RG32I
    \value RGB32I Equivalent to GL_RGB32I
    \value RGBA32I Equivalent to GL_RGBA32I

    \value R16F Equivalent to GL_R16F
    \value RG16F Equivalent to GL_RG16F
    \value RGB16F Equivalent to GL_RGB16F
    \value RGBA16F Equivalent to GL_RGBA16F

    \value R32F Equivalent to GL_R32F
    \value RG32F Equivalent to GL_RG32F
    \value RGB32F Equivalent to GL_RGB32F
    \value RGBA32F Equivalent to GL_RGBA32F

    \value RGB9E5 Equivalent to GL_RGB9_E5
    \value RG11B10F Equivalent to GL_R11F_G11F_B10F
    \value RG3B2 Equivalent to GL_R3_G3_B2
    \value R5G6B5 Equivalent to GL_RGB565
    \value RGB5A1 Equivalent to GL_RGB5_A1
    \value RGBA4 Equivalent to GL_RGBA4
    \value RGB10A2 Equivalent to GL_RGB10_A2UI

    \value D16 Equivalent to GL_DEPTH_COMPONENT16
    \value D24 Equivalent to GL_DEPTH_COMPONENT24
    \value D24S8 Equivalent to GL_DEPTH24_STENCIL8
    \value D32 Equivalent to GL_DEPTH_COMPONENT32
    \value D32F Equivalent to GL_DEPTH_COMPONENT32F
    \value D32FS8X24 Equivalent to GL_DEPTH32F_STENCIL8

    \value RGB_DXT1 Equivalent to GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    \value RGBA_DXT1 Equivalent to GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    \value RGBA_DXT3 Equivalent to GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    \value RGBA_DXT5 Equivalent to GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    \value R_ATI1N_UNorm Equivalent to GL_COMPRESSED_RED_RGTC1
    \value R_ATI1N_SNorm Equivalent to GL_COMPRESSED_SIGNED_RED_RGTC1
    \value RG_ATI2N_UNorm Equivalent to GL_COMPRESSED_RG_RGTC2
    \value RG_ATI2N_SNorm Equivalent to GL_COMPRESSED_SIGNED_RG_RGTC2
    \value RGB_BP_UNSIGNED_FLOAT Equivalent to GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
    \value RGB_BP_SIGNED_FLOAT Equivalent to GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
    \value RGB_BP_UNorm Equivalent to GL_COMPRESSED_RGBA_BPTC_UNORM_ARB

    \value SRGB8 Equivalent to GL_SRGB8
    \value SRGB8_Alpha8 Equivalent to GL_SRGB8_ALPHA8
    \value SRGB_DXT1 Equivalent to GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
    \value SRGB_Alpha_DXT1 Equivalent to GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
    \value SRGB_Alpha_DXT3 Equivalent to GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
    \value SRGB_Alpha_DXT5 Equivalent to GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
    \value SRGB_BP_UNorm Equivalent to GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB

    \value DepthFormat Equivalent to GL_DEPTH_COMPONENT (OpenGL ES 2 only and  when OES_depth_texture is present)
    \value AlphaFormat Equivalent to GL_ALPHA (OpenGL ES 2 only)
    \value RGBFormat Equivalent to GL_RGB (OpenGL ES 2 only)
    \value RGBAFormat Equivalent to GL_RGBA (OpenGL ES 2 only)
    \value LuminanceFormat Equivalent to GL_LUMINANCE (OpenGL ES 2 only)
    \value LuminanceAlphaFormat Equivalent to GL_LUMINANCE_ALPHA (OpenGL ES 2 only)
*/

/*!
    \enum QOpenGLTexture::CubeMapFace
    This enum defines the possible CubeMap faces.

    \value CubeMapPositiveX Equivalent to GL_TEXTURE_CUBE_MAP_POSITIVE_X
    \value CubeMapNegativeX Equivalent to GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    \value CubeMapPositiveY Equivalent to GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    \value CubeMapNegativeY Equivalent to GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    \value CubeMapPositiveZ Equivalent to GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    \value CubeMapNegativeZ Equivalent to GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
*/

/*!
    \enum QOpenGLTexture::PixelFormat
    This enum defines the possible client-side pixel formats for a pixel
    transfer operation.

    \value NoSourceFormat Equivalent to GL_NONE
    \value Red Equivalent to GL_RED
    \value RG Equivalent to GL_RG
    \value RGB Equivalent to GL_RGB
    \value BGR Equivalent to GL_BGR
    \value RGBA Equivalent to GL_RGBA
    \value BGRA Equivalent to GL_BGRA
    \value Red_Integer Equivalent to GL_RED_INTEGER
    \value RG_Integer Equivalent to GL_RG_INTEGER
    \value RGB_Integer Equivalent to GL_RGB_INTEGER
    \value BGR_Integer Equivalent to GL_BGR_INTEGER
    \value RGBA_Integer Equivalent to GL_RGBA_INTEGER
    \value BGRA_Integer Equivalent to GL_BGRA_INTEGER
    \value Depth Equivalent to GL_DEPTH_COMPONENT
    \value DepthStencil Equivalent to GL_DEPTH_STENCIL
    \value Alpha Equivalent to GL_ALPHA (OpenGL ES 2 only)
    \value Luminance Equivalent to GL_LUMINANCE (OpenGL ES 2 only)
    \value LuminanceAlpha Equivalent to GL_LUMINANCE_ALPHA (OpenGL ES 2 only)

*/

/*!
    \enum QOpenGLTexture::PixelType
    This enum defines the possible pixel data types for a pixel transfer operation

    \value NoPixelType Equivalent to GL_NONE
    \value Int8 Equivalent to GL_BYTE
    \value UInt8 Equivalent to GL_UNSIGNED_BYTE
    \value Int16 Equivalent to GL_SHORT
    \value UInt16 Equivalent to GL_UNSIGNED_SHORT
    \value Int32 Equivalent to GL_INT
    \value UInt32 Equivalent to GL_UNSIGNED_INT
    \value Float16 Equivalent to GL_HALF_FLOAT
    \value Float16OES Equivalent to GL_HALF_FLOAT_OES
    \value Float32 Equivalent to GL_FLOAT
    \value UInt32_RGB9_E5 Equivalent to GL_UNSIGNED_INT_5_9_9_9_REV
    \value UInt32_RG11B10F Equivalent to GL_UNSIGNED_INT_10F_11F_11F_REV
    \value UInt8_RG3B2 Equivalent to GL_UNSIGNED_BYTE_3_3_2
    \value UInt8_RG3B2_Rev Equivalent to GL_UNSIGNED_BYTE_2_3_3_REV
    \value UInt16_RGB5A1 Equivalent to GL_UNSIGNED_SHORT_5_5_5_1
    \value UInt16_RGB5A1_Rev Equivalent to GL_UNSIGNED_SHORT_1_5_5_5_REV
    \value UInt16_R5G6B5 Equivalent to GL_UNSIGNED_SHORT_5_6_5
    \value UInt16_R5G6B5_Rev Equivalent to GL_UNSIGNED_SHORT_5_6_5_REV
    \value UInt16_RGBA4 Equivalent to GL_UNSIGNED_SHORT_4_4_4_4
    \value UInt16_RGBA4_Rev Equivalent to GL_UNSIGNED_SHORT_4_4_4_4_REV
    \value UInt32_RGB10A2 Equivalent to GL_UNSIGNED_INT_10_10_10_2
    \value UInt32_RGB10A2_Rev Equivalent to GL_UNSIGNED_INT_2_10_10_10_REV
*/

/*!
    \enum QOpenGLTexture::Feature
    This enum defines the OpenGL texture-related features that can be tested for.

    \value ImmutableStorage Support for immutable texture storage
    \value ImmutableMultisampleStorage Support for immutable texture storage with
           multisample targets
    \value TextureRectangle Support for the GL_TEXTURE_RECTANGLE target
    \value TextureArrays Support for texture targets with array layers
    \value Texture3D Support for the 3 dimensional texture target
    \value TextureMultisample Support for texture targets that have multisample capabilities
    \value TextureBuffer Support for textures that use OpenGL buffer objects
           as their data source
    \value TextureCubeMapArrays Support for cubemap array texture target
    \value Swizzle Support for texture component swizzle masks
    \value StencilTexturing Support for stencil texturing (i.e. looking up depth or stencil
           components of a combined depth/stencil format texture in GLSL shaders).
    \value AnisotropicFiltering Support for anisotropic texture filtering
    \value NPOTTextures Basic support for non-power-of-two textures
    \value NPOTTextureRepeat Full support for non-power-of-two textures including texture
           repeat modes
    \value Texture1D Support for the 1 dimensional texture target
*/

/*!
    \enum QOpenGLTexture::SwizzleComponent
    This enum defines the texture color components that can be assigned a swizzle mask.

    \value SwizzleRed The red component. Equivalent to GL_TEXTURE_SWIZZLE_R
    \value SwizzleGreen The green component. Equivalent to GL_TEXTURE_SWIZZLE_G
    \value SwizzleBlue The blue component. Equivalent to GL_TEXTURE_SWIZZLE_B
    \value SwizzleAlpha The alpha component. Equivalent to GL_TEXTURE_SWIZZLE_A
*/

/*!
    \enum QOpenGLTexture::SwizzleValue
    This enum defines the possible mask values for texture swizzling.

    \value RedValue Maps the component to the red channel. Equivalent to GL_RED
    \value GreenValue Maps the component to the green channel. Equivalent to GL_GREEN
    \value BlueValue Maps the component to the blue channel. Equivalent to GL_BLUE
    \value AlphaValue Maps the component to the alpha channel. Equivalent to GL_ALPHA
    \value ZeroValue Maps the component to a fixed value of 0. Equivalent to GL_ZERO
    \value OneValue Maps the component to a fixed value of 1. Equivalent to GL_ONE
*/

/*!
    \enum QOpenGLTexture::WrapMode
    This enum defines the possible texture coordinate wrapping modes.

    \value Repeat Texture coordinate is repeated. Equivalent to GL_REPEAT
    \value MirroredRepeat Texture coordinate is reflected about 0 and 1. Equivalent to GL_MIRRORED_REPEAT
    \value ClampToEdge Clamps the texture coordinates to [0,1]. Equivalent to GL_CLAMP_TO_EDGE
    \value ClampToBorder As for ClampToEdge but also blends samples at 0 and 1 with a
           fixed border color. Equivalent to GL_CLAMP_TO_BORDER
*/

/*!
    \enum QOpenGLTexture::CoordinateDirection
    This enum defines the possible texture coordinate directions

    \value DirectionS The horizontal direction. Equivalent to GL_TEXTURE_WRAP_S
    \value DirectionT The vertical direction. Equivalent to GL_TEXTURE_WRAP_T
    \value DirectionR The depth direction. Equivalent to GL_TEXTURE_WRAP_R
*/

/*!
    Creates a QOpenGLTexture object that can later be bound to \a target.

    This does not create the underlying OpenGL texture object. Therefore,
    construction using this constructor does not require a valid current
    OpenGL context.
*/
QOpenGLTexture::QOpenGLTexture(Target target)
    : d_ptr(new QOpenGLTexturePrivate(target, this))
{
}

/*!
    Creates a QOpenGLTexture object that can later be bound to the 2D texture
    target and contains the pixel data contained in \a image. If you wish
    to have a chain of mipmaps generated then set \a genMipMaps to \c true (this
    is the default).

    This does create the underlying OpenGL texture object. Therefore,
    construction using this constructor does require a valid current
    OpenGL context.
*/
QOpenGLTexture::QOpenGLTexture(const QImage& image, MipMapGeneration genMipMaps)
    : d_ptr(new QOpenGLTexturePrivate(QOpenGLTexture::Target2D, this))
{
    setData(image, genMipMaps);
}

QOpenGLTexture::~QOpenGLTexture()
{
}

/*!
    Creates the underlying OpenGL texture object. This requires a current valid
    OpenGL context. If the texture object already exists, this function does
    nothing.

    Once the texture object is created you can obtain the object
    name from the textureId() function. This may be useful if you wish to make
    some raw OpenGL calls related to this texture.

    Normally it should not be necessary to call this function directly as all
    functions that set properties of the texture object implicitly call create()
    on your behalf.

    Returns \c true if the creation succeeded, otherwise returns \c false.

    \sa destroy(), isCreated(), textureId()
*/
bool QOpenGLTexture::create()
{
    Q_D(QOpenGLTexture);
    return d->create();
}

/*!
    Destroys the underlying OpenGL texture object. This requires a current valid
    OpenGL context.

    \sa create(), isCreated(), textureId()
*/
void QOpenGLTexture::destroy()
{
    Q_D(QOpenGLTexture);
    return d->destroy();
}

/*!
    Returns \c true if the underlying OpenGL texture object has been created.

    \sa create(), destroy(), textureId()
*/
bool QOpenGLTexture::isCreated() const
{
    Q_D(const QOpenGLTexture);
    return d->textureId != 0;
}

/*!
    Returns the name of the underlying OpenGL texture object or 0 if it has
    not yet been created.

    \sa create(), destroy(), isCreated()
*/
GLuint QOpenGLTexture::textureId() const
{
    Q_D(const QOpenGLTexture);
    return d->textureId;
}

/*!
    Binds this texture to the currently active texture unit ready for
    rendering. Note that you do not need to bind QOpenGLTexture objects
    in order to modify them as the implementation makes use of the
    EXT_direct_state_access extension where available and simulates it
    where it is not.

    \sa release()
*/
void QOpenGLTexture::bind()
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->bind();
}

/*!
    Binds this texture to texture unit \a unit ready for
    rendering. Note that you do not need to bind QOpenGLTexture objects
    in order to modify them as the implementation makes use of the
    EXT_direct_state_access extension where available and simulates it
    where it is not.

    If parameter \a reset is \c true then this function will restore
    the active unit to the texture unit that was active upon entry.

    \sa release()
*/
void QOpenGLTexture::bind(uint unit, TextureUnitReset reset)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->bind(unit, reset);
}

/*!
    Unbinds this texture from the currently active texture unit.

    \sa bind()
*/
void QOpenGLTexture::release()
{
    Q_D(QOpenGLTexture);
    d->release();
}

/*!
    Unbinds this texture from texture unit \a unit.

    If parameter \a reset is \c true then this function
    will restore the active unit to the texture unit that was active
    upon entry.
*/
void QOpenGLTexture::release(uint unit, TextureUnitReset reset)
{
    Q_D(QOpenGLTexture);
    d->release(unit, reset);
}

/*!
    Returns \c true if this texture is bound to the corresponding target
    of the currently active texture unit.

    \sa bind(), release()
*/
bool QOpenGLTexture::isBound() const
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(d->textureId);
    return d->isBound();
}

/*!
    Returns \c true if this texture is bound to the corresponding target
    of texture unit \a unit.

    \sa bind(), release()
*/
bool QOpenGLTexture::isBound(uint unit)
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(d->textureId);
    return d->isBound(unit);
}

/*!
    Returns the textureId of the texture that is bound to the \a target
    of the currently active texture unit.
*/
GLuint QOpenGLTexture::boundTextureId(BindingTarget target)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLTexture::boundTextureId() requires a valid current context");
        return 0;
    }

    GLint textureId = 0;
    ctx->functions()->glGetIntegerv(target, &textureId);
    return static_cast<GLuint>(textureId);
}

/*!
    Returns the textureId of the texture that is bound to the \a target
    of the texture unit \a unit.
*/
GLuint QOpenGLTexture::boundTextureId(uint unit, BindingTarget target)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLTexture::boundTextureId() requires a valid current context");
        return 0;
    }

    QOpenGLFunctions *funcs = ctx->functions();
    funcs->initializeOpenGLFunctions();

    GLint oldTextureUnit = 0;
    funcs->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTextureUnit);

    funcs->glActiveTexture(unit);
    GLint textureId = 0;
    funcs->glGetIntegerv(target, &textureId);
    funcs->glActiveTexture(oldTextureUnit);

    return static_cast<GLuint>(textureId);
}

/*!
    Sets the format of this texture object to \a format. This function
    must be called before texture storage is allocated.

    Note that all formats may not be supported. The exact set of supported
    formats is dependent upon your OpenGL implementation and version.

    \sa format(), allocateStorage()
*/
void QOpenGLTexture::setFormat(TextureFormat format)
{
    Q_D(QOpenGLTexture);
    d->create();
    if (isStorageAllocated()) {
        qWarning("QOpenGLTexture::setFormat(): Cannot change format once storage has been allocated");
        return;
    }

    d->format = format;

    switch (format) {
    case NoFormat:
        d->formatClass = NoFormatClass;
        break;

    case RGBA32F:
    case RGBA32U:
    case RGBA32I:
        d->formatClass = FormatClass_128Bit;
        break;

    case RGB32F:
    case RGB32U:
    case RGB32I:
        d->formatClass = FormatClass_96Bit;
        break;

    case RGBA16F:
    case RG32F:
    case RGBA16U:
    case RG32U:
    case RGBA16I:
    case RG32I:
    case RGBA16_UNorm:
    case RGBA16_SNorm:
        d->formatClass = FormatClass_64Bit;
        break;

    case RGB16_UNorm:
    case RGB16_SNorm:
    case RGB16F:
    case RGB16U:
    case RGB16I:
        d->formatClass = FormatClass_48Bit;
        break;

    case RG16F:
    case RG11B10F:
    case R32F:
    case RGB10A2:
    case RGBA8U:
    case RG16U:
    case R32U:
    case RGBA8I:
    case RG16I:
    case R32I:
    case RGBA8_UNorm:
    case RG16_UNorm:
    case RGBA8_SNorm:
    case RG16_SNorm:
    case SRGB8_Alpha8:
    case RGB9E5:
        d->formatClass = FormatClass_32Bit;
        break;

    case RGB8_UNorm:
    case RGB8_SNorm:
    case SRGB8:
    case RGB8U:
    case RGB8I:
        d->formatClass = FormatClass_24Bit;
        break;

    case R16F:
    case RG8U:
    case R16U:
    case RG8I:
    case R16I:
    case RG8_UNorm:
    case R16_UNorm:
    case RG8_SNorm:
    case R16_SNorm:
        d->formatClass = FormatClass_16Bit;
        break;

    case R8U:
    case R8I:
    case R8_UNorm:
    case R8_SNorm:
        d->formatClass = FormatClass_8Bit;
        break;

    case R_ATI1N_UNorm:
    case R_ATI1N_SNorm:
        d->formatClass = FormatClass_RGTC1_R;
        break;

    case RG_ATI2N_UNorm:
    case RG_ATI2N_SNorm:
        d->formatClass = FormatClass_RGTC2_RG;
        break;

    case RGB_BP_UNorm:
    case SRGB_BP_UNorm:
        d->formatClass = FormatClass_BPTC_Unorm;
        break;

    case RGB_BP_UNSIGNED_FLOAT:
    case RGB_BP_SIGNED_FLOAT:
        d->formatClass = FormatClass_BPTC_Float;
        break;

    case RGB_DXT1:
    case SRGB_DXT1:
        d->formatClass = FormatClass_S3TC_DXT1_RGB;
        break;

    case RGBA_DXT1:
    case SRGB_Alpha_DXT1:
        d->formatClass = FormatClass_S3TC_DXT1_RGBA;
        break;

    case RGBA_DXT3:
    case SRGB_Alpha_DXT3:
        d->formatClass = FormatClass_S3TC_DXT3_RGBA;
        break;

    case RGBA_DXT5:
    case SRGB_Alpha_DXT5:
        d->formatClass = FormatClass_S3TC_DXT5_RGBA;
        break;

    case RG3B2:
    case R5G6B5:
    case RGB5A1:
    case RGBA4:
    case D16:
    case D24:
    case D24S8:
    case D32:
    case D32F:
    case D32FS8X24:
    case DepthFormat:
    case AlphaFormat:
    case RGBFormat:
    case RGBAFormat:
    case LuminanceFormat:
    case LuminanceAlphaFormat:
        d->formatClass = FormatClass_Unique;
        break;
    }
}

/*!
    Returns the format of this texture object.

    \sa setFormat()
*/
QOpenGLTexture::TextureFormat QOpenGLTexture::format() const
{
    Q_D(const QOpenGLTexture);
    return d->format;
}

/*!
    Sets the dimensions of this texture object to \a width,
    \a height, and \a depth. The default for each dimension is 1.
    The maximum allowable texture size is dependent upon your OpenGL
    implementation. Allocating storage for a texture less than the
    maximum size can still fail if your system is low on resources.

    \sa width(), height(), depth()
*/
void QOpenGLTexture::setSize(int width, int height, int depth)
{
    Q_D(QOpenGLTexture);
    d->create();
    if (isStorageAllocated()) {
        qWarning("Cannot resize a texture that already has storage allocated.\n"
                 "To do so, destroy() the texture and then create() and setSize()");
        return;
    }

    switch (d->target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::TargetBuffer:
        d->dimensions[0] = width;
        Q_UNUSED(height);
        Q_UNUSED(depth);
        break;

    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetRectangle:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
        d->dimensions[0] = width;
        d->dimensions[1] = height;
        Q_UNUSED(depth);
        break;

    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
        if (width != height)
            qWarning("QAbstractOpenGLTexture::setSize(): Cube map textures must be square");
        d->dimensions[0] = d->dimensions[1] = width;
        Q_UNUSED(depth);
        break;

    case QOpenGLTexture::Target3D:
        d->dimensions[0] = width;
        d->dimensions[1] = height;
        d->dimensions[2] = depth;
        break;
    }
}

/*!
    Returns the width of a 1D, 2D or 3D texture.

    \sa height(), depth(), setSize()
*/
int QOpenGLTexture::width() const
{
    Q_D(const QOpenGLTexture);
    return d->dimensions[0];
}

/*!
    Returns the height of a 2D or 3D texture.

    \sa width(), depth(), setSize()
*/
int QOpenGLTexture::height() const
{
    Q_D(const QOpenGLTexture);
    return d->dimensions[1];
}

/*!
    Returns the depth of a 3D texture.

    \sa width(), height(), setSize()
*/
int QOpenGLTexture::depth() const
{
    Q_D(const QOpenGLTexture);
    return d->dimensions[2];
}

/*!
    For texture targets that support mipmaps, this function
    sets the requested number of mipmap \a levels to allocate storage
    for. This function should be called before storage is allocated
    for the texture.

    If the texture target does not support mipmaps this function
    has no effect.

    \sa mipLevels(), maximumMipLevels(), isStorageAllocated()
*/
void QOpenGLTexture::setMipLevels(int levels)
{
    Q_D(QOpenGLTexture);
    d->create();
    if (isStorageAllocated()) {
        qWarning("Cannot set mip levels on a texture that already has storage allocated.\n"
                 "To do so, destroy() the texture and then create() and setMipLevels()");
        return;
    }

    switch (d->target) {
    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetCubeMapArray:
    case QOpenGLTexture::Target3D:
        d->requestedMipLevels = levels;
        break;

    case QOpenGLTexture::TargetBuffer:
    case QOpenGLTexture::TargetRectangle:
    case QOpenGLTexture::Target2DMultisample:
    case QOpenGLTexture::Target2DMultisampleArray:
        qWarning("QAbstractOpenGLTexture::setMipLevels(): This texture target does not support mipmaps");
        break;
    }
}

/*!
    Returns the number of mipmap levels for this texture. If storage
    has not yet been allocated for this texture it returns the
    requested number of mipmap levels.

    \sa setMipLevels(), maximumMipLevels(), isStorageAllocated()
*/
int QOpenGLTexture::mipLevels() const
{
    Q_D(const QOpenGLTexture);
    return isStorageAllocated() ? d->mipLevels : d->requestedMipLevels;
}

/*!
    Returns the maximum number of mipmap levels that this texture
    can have given the current dimensions.

    \sa setMipLevels(), mipLevels(), setSize()
*/
int QOpenGLTexture::maximumMipLevels() const
{
    Q_D(const QOpenGLTexture);
    return d->maximumMipLevelCount();
}

/*!
    Sets the number of array \a layers to allocate storage for. This
    function should be called before storage is allocated for the texture.

    For targets that do not support array layers this function has
    no effect.

    \sa layers(), isStorageAllocated()
*/
void QOpenGLTexture::setLayers(int layers)
{
    Q_D(QOpenGLTexture);
    d->create();
    if (isStorageAllocated()) {
        qWarning("Cannot set layers on a texture that already has storage allocated.\n"
                 "To do so, destroy() the texture and then create() and setLayers()");
        return;
    }

    switch (d->target) {
    case QOpenGLTexture::Target1DArray:
    case QOpenGLTexture::Target2DArray:
    case QOpenGLTexture::TargetCubeMapArray:
    case QOpenGLTexture::Target2DMultisampleArray:
        d->layers = layers;
        break;

    case QOpenGLTexture::Target1D:
    case QOpenGLTexture::Target2D:
    case QOpenGLTexture::Target3D:
    case QOpenGLTexture::TargetCubeMap:
    case QOpenGLTexture::TargetBuffer:
    case QOpenGLTexture::TargetRectangle:
    case QOpenGLTexture::Target2DMultisample:
        qWarning("Texture target does not support array layers");
        break;
    }
}

/*!
    Returns the number of array layers for this texture. If
    storage has not yet been allocated for this texture then
    this function returns the requested number of array layers.

    For texture targets that do not support array layers this
    will return 1.

    \sa setLayers(), isStorageAllocated()
*/
int QOpenGLTexture::layers() const
{
    Q_D(const QOpenGLTexture);
    return d->layers;
}

/*!
    Returns the number of faces for this texture. For cubemap
    and cubemap array type targets this will be 6.

    For non-cubemap type targets this will return 1.
*/
int QOpenGLTexture::faces() const
{
    Q_D(const QOpenGLTexture);
    return d->faces;
}

/*!
    Allocates server-side storage for this texture object taking
    into account, the format, dimensions, mipmap levels, array
    layers and cubemap faces.

    Once storage has been allocated it is no longer possible to change
    these properties.

    If supported QOpenGLTexture makes use of immutable texture
    storage.

    Once storage has been allocated for the texture then pixel data
    can be uploaded via one of the setData() overloads.

    \sa isStorageAllocated(), setData()
*/
void QOpenGLTexture::allocateStorage()
{
    Q_D(QOpenGLTexture);
    d->allocateStorage();
}

/*!
    Returns \c true if server-side storage for this texture as been
    allocated.

    The texture format, dimensions, mipmap levels and array layers
    cannot be altered once storage ihas been allocated.

    \sa allocateStorage(), setSize(), setMipLevels(), setLayers(), setFormat()
*/
bool QOpenGLTexture::isStorageAllocated() const
{
    Q_D(const QOpenGLTexture);
    return d->storageAllocated;
}

/*!
    Attempts to create a texture view onto this texture. A texture
    view is somewhat analogous to a view in SQL in that it presents
    a restricted or reinterpreted view of the original data. Texture
    views do not allocate any more server-side storage, insted relying
    on the storage buffer of the source texture.

    Texture views are only available when using immutable storage. For
    more information on texture views see
    http://www.opengl.org/wiki/Texture_Storage#Texture_views.

    The \a target argument specifies the target to use for the view.
    Only some targets can be used depending upon the target of the original
    target. For e.g. a view onto a Target1DArray texture can specify
    either Target1DArray or Target1D but for the latter the number of
    array layers specified with \a minimumLayer and \a maximumLayer must
    be exactly 1.

    Simpliar constraints apply for the \a viewFormat. See the above link
    and the specification for more details.

    The \a minimumMipmapLevel, \a maximumMipmapLevel, \a minimumLayer,
    and \a maximumLayer arguments serve to restrict the parts of the
    texture accessible by the texture view.

    If creation of the texture view fails this function will return
    0. If the function succeeds it will return a pointer to a new
    QOpenGLTexture object that will return \c true from its isTextureView()
    function.

    \sa isTextureView()
*/
QOpenGLTexture *QOpenGLTexture::createTextureView(Target target,
                                                  TextureFormat viewFormat,
                                                  int minimumMipmapLevel, int maximumMipmapLevel,
                                                  int minimumLayer, int maximumLayer) const
{
    Q_D(const QOpenGLTexture);
    if (!isStorageAllocated()) {
        qWarning("Cannot set create a texture view of a texture that does not have storage allocated.");
        return 0;
    }
    Q_ASSERT(maximumMipmapLevel >= minimumMipmapLevel);
    Q_ASSERT(maximumLayer >= minimumLayer);
    return d->createTextureView(target, viewFormat,
                                minimumMipmapLevel, maximumMipmapLevel,
                                minimumLayer, maximumLayer);
}

/*!
    Returns \c true if this texture object is actually a view onto another
    texture object.

    \sa createTextureView()
*/
bool QOpenGLTexture::isTextureView() const
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(d->textureId);
    return d->textureView;
}

/*!
    Uploads pixel \a data for this texture object \a mipLevel, array \a layer, and \a cubeFace.
    Storage must have been allocated before uploading pixel data. Some overloads of setData()
    will set appropriate dimensions, mipmap levels, and array layers and then allocate storage
    for you if they have enough information to do so. This will be noted in the function
    documentation.

    The structure of the pixel data pointed to by \a data is specified by \a sourceFormat
    and \a sourceType. The pixel data upload can optionally be controlled by \a options.

    If using a compressed format() then you should use setCompressedData() instead of this
    function.

    \since 5.3
    \sa setCompressedData()
*/
void QOpenGLTexture::setData(int mipLevel, int layer, CubeMapFace cubeFace,
                             PixelFormat sourceFormat, PixelType sourceType,
                             const void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    if (!isStorageAllocated()) {
        qWarning("Cannot set data on a texture that does not have storage allocated.\n"
                 "To do so call allocate() before this function");
        return;
    }
    d->setData(mipLevel, layer, cubeFace, sourceFormat, sourceType, data, options);
}

/*!
    \since 5.3
    \overload
*/
void QOpenGLTexture::setData(int mipLevel, int layer,
                             PixelFormat sourceFormat, PixelType sourceType,
                             const void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(mipLevel, layer, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    \since 5.3
    \overload
*/
void QOpenGLTexture::setData(int mipLevel,
                             PixelFormat sourceFormat, PixelType sourceType,
                             const void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(mipLevel, 0, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    \since 5.3
    \overload
*/
void QOpenGLTexture::setData(PixelFormat sourceFormat, PixelType sourceType,
                             const void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    \obsolete
    \overload

    \sa setCompressedData()
*/
void QOpenGLTexture::setData(int mipLevel, int layer, CubeMapFace cubeFace,
                             PixelFormat sourceFormat, PixelType sourceType,
                             void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    if (!isStorageAllocated()) {
        qWarning("Cannot set data on a texture that does not have storage allocated.\n"
                 "To do so call allocate() before this function");
        return;
    }
    d->setData(mipLevel, layer, cubeFace, sourceFormat, sourceType, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setData(int mipLevel, int layer,
                             PixelFormat sourceFormat, PixelType sourceType,
                             void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(mipLevel, layer, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setData(int mipLevel,
                             PixelFormat sourceFormat, PixelType sourceType,
                             void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(mipLevel, 0, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setData(PixelFormat sourceFormat, PixelType sourceType,
                             void *data, const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, sourceFormat, sourceType, data, options);
}

/*!
    This overload of setData() will allocate storage for you.
    The pixel data is contained in \a image. Mipmaps are generated by default.
    Set \a genMipMaps to \l DontGenerateMipMaps to turn off mipmap generation.

    \overload
*/
void QOpenGLTexture::setData(const QImage& image, MipMapGeneration genMipMaps)
{
    setFormat(QOpenGLTexture::RGBA8_UNorm);
    setSize(image.width(), image.height());
    setMipLevels(genMipMaps == GenerateMipMaps ? maximumMipLevels() : 1);
    allocateStorage();

    // Upload pixel data and generate mipmaps
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888);
    QOpenGLPixelTransferOptions uploadOptions;
    uploadOptions.setAlignment(1);
    setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, glImage.constBits(), &uploadOptions);
}

/*!
    Uploads compressed pixel \a data to \a mipLevel, array \a layer, and \a cubeFace.
    The pixel transfer can optionally be controlled with \a options. The \a dataSize
    argument should specify the size of the data pointed to by \a data.

    If not using a compressed format() then you should use setData() instead of this
    function.

    \since 5.3
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int layer, CubeMapFace cubeFace,
                                       int dataSize, const void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    if (!isStorageAllocated()) {
        qWarning("Cannot set data on a texture that does not have storage allocated.\n"
                 "To do so call allocate() before this function");
        return;
    }
    d->setCompressedData(mipLevel, layer, cubeFace, dataSize, data, options);
}

/*!
    \overload
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int layer, int dataSize, const void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(mipLevel, layer, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    \overload
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int dataSize, const void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(mipLevel, 0, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    \overload
*/
void QOpenGLTexture::setCompressedData(int dataSize, const void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(0, 0, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int layer, CubeMapFace cubeFace,
                                       int dataSize, void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    if (!isStorageAllocated()) {
        qWarning("Cannot set data on a texture that does not have storage allocated.\n"
                 "To do so call allocate() before this function");
        return;
    }
    d->setCompressedData(mipLevel, layer, cubeFace, dataSize, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int layer, int dataSize, void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(mipLevel, layer, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setCompressedData(int mipLevel, int dataSize, void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(mipLevel, 0, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    \obsolete
    \overload
*/
void QOpenGLTexture::setCompressedData(int dataSize, void *data,
                                       const QOpenGLPixelTransferOptions * const options)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->textureId);
    d->setCompressedData(0, 0, QOpenGLTexture::CubeMapPositiveX, dataSize, data, options);
}

/*!
    Returns \c true if your OpenGL implementation and version supports the texture
    feature \a feature.
*/
bool QOpenGLTexture::hasFeature(Feature feature)
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLTexture::hasFeature() requires a valid current context");
        return false;
    }

    QSurfaceFormat f = ctx->format();

    bool supported = false;

#if !defined(QT_OPENGL_ES_2)
    if (!ctx->isOpenGLES()) {
        switch (feature) {
        case ImmutableMultisampleStorage:
        case TextureBuffer:
        case StencilTexturing:
            supported = f.version() >= qMakePair(4, 3);
            break;

        case ImmutableStorage:
            supported = f.version() >= qMakePair(4, 2);
            break;

        case TextureCubeMapArrays:
            supported = f.version() >= qMakePair(4, 0);
            break;

        case Swizzle:
            supported = f.version() >= qMakePair(3, 3);
            break;

        case TextureMultisample:
            supported = f.version() >= qMakePair(3, 2);
            break;

        case TextureArrays:
            supported = f.version() >= qMakePair(3, 0);
            break;

        case TextureRectangle:
            supported = f.version() >= qMakePair(2, 1);
            break;

        case Texture3D:
            supported = f.version() >= qMakePair(1, 3);
            break;

        case AnisotropicFiltering:
            supported = ctx->hasExtension(QByteArrayLiteral("GL_EXT_texture_filter_anisotropic"));
            break;

        case NPOTTextures:
        case NPOTTextureRepeat:
            supported = ctx->hasExtension(QByteArrayLiteral("GL_ARB_texture_non_power_of_two"));
            break;

        case Texture1D:
            supported = f.version() >= qMakePair(1, 1);
            break;

        case MaxFeatureFlag:
            break;

        default:
            break;
        }
    }

    if (ctx->isOpenGLES())
#endif
    {
        switch (feature) {
        case Texture3D:
            supported = ctx->hasExtension(QByteArrayLiteral("GL_OES_texture_3D"));
            break;
        case AnisotropicFiltering:
            supported = ctx->hasExtension(QByteArrayLiteral("GL_EXT_texture_filter_anisotropic"));
            break;
        case NPOTTextures:
        case NPOTTextureRepeat:
            supported = f.version() >= qMakePair(3,0);
            if (!supported) {
                supported = ctx->hasExtension(QByteArrayLiteral("GL_OES_texture_npot"));
                if (!supported)
                    supported = ctx->hasExtension(QByteArrayLiteral("GL_ARB_texture_non_power_of_two"));
            }
        default:
            break;
        }
    }

    return supported;
}

/*!
    Sets the base mipmap level used for all texture lookups with this texture to \a baseLevel.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa mipBaseLevel(), setMipMaxLevel(), setMipLevelRange()
*/
void QOpenGLTexture::setMipBaseLevel(int baseLevel)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->textureId);
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(baseLevel <= d->maxLevel);
        d->baseLevel = baseLevel;
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BASE_LEVEL, baseLevel);
        return;
    }
#else
    Q_UNUSED(baseLevel);
#endif
    qWarning("QOpenGLTexture: Mipmap base level is not supported");
}

/*!
    Returns the mipmap base level used for all texture lookups with this texture.
    The default is 0.

    \sa setMipBaseLevel(), mipMaxLevel(), mipLevelRange()
*/
int QOpenGLTexture::mipBaseLevel() const
{
    Q_D(const QOpenGLTexture);
    return d->baseLevel;
}

/*!
    Sets the maximum mipmap level used for all texture lookups with this texture to \a maxLevel.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa mipMaxLevel(), setMipBaseLevel(), setMipLevelRange()
*/
void QOpenGLTexture::setMipMaxLevel(int maxLevel)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->textureId);
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->baseLevel <= maxLevel);
        d->maxLevel = maxLevel;
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAX_LEVEL, maxLevel);
        return;
    }
#else
    Q_UNUSED(maxLevel);
#endif
    qWarning("QOpenGLTexture: Mipmap max level is not supported");
}

/*!
    Returns the mipmap maximum level used for all texture lookups with this texture.

    \sa setMipMaxLevel(), mipBaseLevel(), mipLevelRange()
*/
int QOpenGLTexture::mipMaxLevel() const
{
    Q_D(const QOpenGLTexture);
    return d->maxLevel;
}

/*!
    Sets the range of mipmap levels that can be used for texture lookups with this texture
    to range from \a baseLevel to \a maxLevel.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa setMipBaseLevel(), setMipMaxLevel(), mipLevelRange()
*/
void QOpenGLTexture::setMipLevelRange(int baseLevel, int maxLevel)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->textureId);
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(baseLevel <= maxLevel);
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BASE_LEVEL, baseLevel);
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAX_LEVEL, maxLevel);
        return;
    }
#else
    Q_UNUSED(baseLevel);
    Q_UNUSED(maxLevel);
#endif
    qWarning("QOpenGLTexture: Mipmap level range is not supported");
}

/*!
    Returns the range of mipmap levels that can be used for texture lookups with this texture.

    \sa mipBaseLevel(), mipMaxLevel()
*/
QPair<int, int> QOpenGLTexture::mipLevelRange() const
{
    Q_D(const QOpenGLTexture);
    return qMakePair(d->baseLevel, d->maxLevel);
}

/*!
    If \a enabled is \c true, enables automatic mipmap generation for this texture object
    to occur whenever the level 0 mipmap data is set via setData().

    The automatic mipmap generation is enabled by default.

    \sa isAutoMipMapGenerationEnabled(), generateMipMaps()
*/
void QOpenGLTexture::setAutoMipMapGenerationEnabled(bool enabled)
{
    Q_D(QOpenGLTexture);
    d->autoGenerateMipMaps = enabled;
}

/*!
    Returns whether auto mipmap generation is enabled for this texture object.

    \sa setAutoMipMapGenerationEnabled(), generateMipMaps()
*/
bool QOpenGLTexture::isAutoMipMapGenerationEnabled() const
{
    Q_D(const QOpenGLTexture);
    return d->autoGenerateMipMaps;
}

/*!
    Generates mipmaps for this texture object from mipmap level 0. If you are
    using a texture target and filtering option that requires mipmaps and you
    have disabled automatic mipmap generation then you need to call this function
    or the overload to create the mipmap chain.

    \sa setAutoMipMapGenerationEnabled(), setMipLevels(), mipLevels()
*/
void QOpenGLTexture::generateMipMaps()
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->texFuncs->glGenerateTextureMipmap(d->textureId, d->target, d->bindingTarget);
}

/*!
    Generates mipmaps for this texture object from mipmap level \a baseLevel. If you are
    using a texture target and filtering option that requires mipmaps and you
    have disabled automatic mipmap generation then you need to call this function
    or the overload to create the mipmap chain.

    The generation of mipmaps to above \a baseLevel is achieved by setting the mipmap
    base level to \a baseLevel and then generating the mipmap chain. If \a resetBaseLevel
    is \c true, then the baseLevel of the texture will be reset to its previous value.

    \sa setAutoMipMapGenerationEnabled(), setMipLevels(), mipLevels()
*/
void QOpenGLTexture::generateMipMaps(int baseLevel, bool resetBaseLevel)
{
    Q_D(QOpenGLTexture);
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    int oldBaseLevel;
    if (resetBaseLevel)
        oldBaseLevel = mipBaseLevel();
    setMipBaseLevel(baseLevel);
    d->texFuncs->glGenerateTextureMipmap(d->textureId, d->target, d->bindingTarget);
    if (resetBaseLevel)
        setMipBaseLevel(oldBaseLevel);
}

/*!
    GLSL shaders are able to reorder the components of the vec4 returned by texture
    functions. It is also desirable to be able to control this reordering from CPU
    side code. This is made possible by swizzle masks since OpenGL 3.3.

    Each component of the texture can be mapped to one of the SwizzleValue options.

    This function maps \a component to the output \a value.

    \note This function has no effect on Mac and Qt built for OpenGL ES 2.
    \sa swizzleMask()
*/
void QOpenGLTexture::setSwizzleMask(SwizzleComponent component, SwizzleValue value)
{
#if !defined(Q_OS_MAC) && !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        if (!d->features.testFlag(Swizzle)) {
            qWarning("QOpenGLTexture::setSwizzleMask() requires OpenGL >= 3.3");
            return;
        }
        d->swizzleMask[component - SwizzleRed] = value;
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, component, value);
        return;
    }
#else
    Q_UNUSED(component);
    Q_UNUSED(value);
#endif
    qWarning("QOpenGLTexture: Texture swizzling is not supported");
}

/*!
    Parameters \a {r}, \a {g}, \a {b}, and \a {a}  are values used for setting
    the colors red, green, blue, and the alpha value.
    \overload
*/
void QOpenGLTexture::setSwizzleMask(SwizzleValue r, SwizzleValue g,
                                    SwizzleValue b, SwizzleValue a)
{
#if !defined(Q_OS_MAC) && !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        if (!d->features.testFlag(Swizzle)) {
            qWarning("QOpenGLTexture::setSwizzleMask() requires OpenGL >= 3.3");
            return;
        }
        GLint swizzleMask[] = {GLint(r), GLint(g), GLint(b), GLint(a)};
        d->swizzleMask[0] = r;
        d->swizzleMask[1] = g;
        d->swizzleMask[2] = b;
        d->swizzleMask[3] = a;
        d->texFuncs->glTextureParameteriv(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        return;
    }
#else
    Q_UNUSED(r);
    Q_UNUSED(g);
    Q_UNUSED(b);
    Q_UNUSED(a);
#endif
    qWarning("QOpenGLTexture: Texture swizzling is not supported");
}

/*!
    Returns the swizzle mask for texture \a component.
*/
QOpenGLTexture::SwizzleValue QOpenGLTexture::swizzleMask(SwizzleComponent component) const
{
    Q_D(const QOpenGLTexture);
    return d->swizzleMask[component - SwizzleRed];
}

/*!
    If using a texture that has a combined depth/stencil format this function sets
    which component of the texture is accessed to \a mode.

    When the parameter is set to DepthMode, then accessing it from the
    shader will access the depth component as a single float, as normal. But when
    the parameter is set to StencilMode, the shader will access the stencil component.

    \note This function has no effect on Mac and Qt built for OpenGL ES 2.
    \sa depthStencilMode()
*/
void QOpenGLTexture::setDepthStencilMode(QOpenGLTexture::DepthStencilMode mode)
{
#if !defined(Q_OS_MAC) && !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        if (!d->features.testFlag(StencilTexturing)) {
            qWarning("QOpenGLTexture::setDepthStencilMode() requires OpenGL >= 4.3");
            return;
        }
        d->depthStencilMode = mode;
        d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_DEPTH_STENCIL_TEXTURE_MODE, mode);
        return;
    }
#else
    Q_UNUSED(mode);
#endif
    qWarning("QOpenGLTexture: DepthStencil Mode is not supported");
}

/*!
    Returns the depth stencil mode for textures using a combined depth/stencil format.

    \sa setDepthStencilMode()
*/
QOpenGLTexture::DepthStencilMode QOpenGLTexture::depthStencilMode() const
{
    Q_D(const QOpenGLTexture);
    return d->depthStencilMode;
}

/*!
    Sets the filter used for minification to \a filter.

    \sa minificationFilter(), setMagnificationFilter(), setMinMagFilters()
*/
void QOpenGLTexture::setMinificationFilter(QOpenGLTexture::Filter filter)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->minFilter = filter;
    d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MIN_FILTER, filter);
}

/*!
    Returns the minification filter.

    \sa setMinificationFilter()
*/
QOpenGLTexture::Filter QOpenGLTexture::minificationFilter() const
{
    Q_D(const QOpenGLTexture);
    return d->minFilter;
}

/*!
    Sets the magnification filter to \a filter.

    \sa magnificationFilter(), setMinificationFilter(), setMinMagFilters()
*/
void QOpenGLTexture::setMagnificationFilter(QOpenGLTexture::Filter filter)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->magFilter = filter;
    d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAG_FILTER, filter);
}

/*!
    Returns the magnification filter.

    \sa setMagnificationFilter()
*/
QOpenGLTexture::Filter QOpenGLTexture::magnificationFilter() const
{
    Q_D(const QOpenGLTexture);
    return d->magFilter;
}

/*!
    Sets the minification filter to \a minificationFilter and the magnification filter
    to \a magnificationFilter.

    \sa minMagFilters(), setMinificationFilter(), setMagnificationFilter()
*/
void QOpenGLTexture::setMinMagFilters(QOpenGLTexture::Filter minificationFilter,
                                      QOpenGLTexture::Filter magnificationFilter)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->minFilter = minificationFilter;
    d->magFilter = magnificationFilter;
    d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MIN_FILTER, minificationFilter);
    d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAG_FILTER, magnificationFilter);
}

/*!
    Returns the current minification and magnification filters.

    \sa setMinMagFilters()
*/
QPair<QOpenGLTexture::Filter, QOpenGLTexture::Filter> QOpenGLTexture::minMagFilters() const
{
    Q_D(const QOpenGLTexture);
    return QPair<QOpenGLTexture::Filter, QOpenGLTexture::Filter>(d->minFilter, d->magFilter);
}

/*!
    If your OpenGL implementation supports the GL_EXT_texture_filter_anisotropic extension
    this function sets the maximum anisotropy level to \a anisotropy.

    \sa maximumAnisotropy()
*/
void QOpenGLTexture::setMaximumAnisotropy(float anisotropy)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    if (!d->features.testFlag(StencilTexturing)) {
        qWarning("QOpenGLTexture::setMaximumAnisotropy() requires GL_EXT_texture_filter_anisotropic");
        return;
    }
    d->maxAnisotropy = anisotropy;
    d->texFuncs->glTextureParameteri(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
}

/*!
    Returns the maximum level of anisotropy to be accounted for when performing texture lookups.
    This requires the GL_EXT_texture_filter_anisotropic extension.

    \sa setMaximumAnisotropy()
*/
float QOpenGLTexture::maximumAnisotropy() const
{
    Q_D(const QOpenGLTexture);
    return d->maxAnisotropy;
}

/*!
    Sets the wrap (or repeat mode) for all texture dimentions to \a mode.

    \sa wrapMode()
*/
void QOpenGLTexture::setWrapMode(QOpenGLTexture::WrapMode mode)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->setWrapMode(mode);
}

/*!
    Holds the texture dimension \a direction.
    \overload
*/
void QOpenGLTexture::setWrapMode(QOpenGLTexture::CoordinateDirection direction, QOpenGLTexture::WrapMode mode)
{
    Q_D(QOpenGLTexture);
    d->create();
    Q_ASSERT(d->texFuncs);
    Q_ASSERT(d->textureId);
    d->setWrapMode(direction, mode);
}

/*!
    Returns the wrap mode for the texture dimension \a direction.

    \sa setWrapMode()
*/
QOpenGLTexture::WrapMode QOpenGLTexture::wrapMode(QOpenGLTexture::CoordinateDirection direction) const
{
    Q_D(const QOpenGLTexture);
    return d->wrapMode(direction);
}

/*!
    Sets the border color of the texture to \a color.

    \note This function has no effect on Mac and Qt built for OpenGL ES 2.
    \sa borderColor()
*/
void QOpenGLTexture::setBorderColor(QColor color)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        float values[4];
        values[0] = color.redF();
        values[1] = color.greenF();
        values[2] = color.blueF();
        values[3] = color.alphaF();
        d->borderColor.clear();
        for (int i = 0; i < 4; ++i)
            d->borderColor.append(QVariant(values[i]));
        d->texFuncs->glTextureParameterfv(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BORDER_COLOR, values);
        return;
    }
#else
    Q_UNUSED(color);
#endif
    qWarning("QOpenGLTexture: Border color is not supported");
}

/*!
    Sets the color red to \a {r}, green to \a {g}, blue to \a {b}, and \a {a} to the
    alpha value.
    \overload
*/
void QOpenGLTexture::setBorderColor(float r, float g, float b, float a)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        float values[4];
        values[0] = r;
        values[1] = g;
        values[2] = b;
        values[3] = a;
        d->borderColor.clear();
        for (int i = 0; i < 4; ++i)
            d->borderColor.append(QVariant(values[i]));
        d->texFuncs->glTextureParameterfv(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BORDER_COLOR, values);
        return;
    }
#else
    Q_UNUSED(r);
    Q_UNUSED(g);
    Q_UNUSED(b);
    Q_UNUSED(a);
#endif
    qWarning("QOpenGLTexture: Border color is not supported");
}

/*!
    Sets the color red to \a {r}, green to \a {g}, blue to \a {b}, and the alpha
    value to \a {a}.
    \overload
*/
void QOpenGLTexture::setBorderColor(int r, int g, int b, int a)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        int values[4];
        values[0] = r;
        values[1] = g;
        values[2] = b;
        values[3] = a;
        d->borderColor.clear();
        for (int i = 0; i < 4; ++i)
            d->borderColor.append(QVariant(values[i]));
        d->texFuncs->glTextureParameteriv(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BORDER_COLOR, values);
        return;
    }
#else
    Q_UNUSED(r);
    Q_UNUSED(g);
    Q_UNUSED(b);
    Q_UNUSED(a);
#endif
    qWarning("QOpenGLTexture: Border color is not supported");

    // TODO Handle case of using glTextureParameterIiv() based on format
}

/*!
    Sets the color red to \a {r}, green to \a {g}, blue to \a {b}, and the alpha
    value to \a {a}.
    \overload
*/
void QOpenGLTexture::setBorderColor(uint r, uint g, uint b, uint a)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        int values[4];
        values[0] = int(r);
        values[1] = int(g);
        values[2] = int(b);
        values[3] = int(a);
        d->borderColor.clear();
        for (int i = 0; i < 4; ++i)
            d->borderColor.append(QVariant(values[i]));
        d->texFuncs->glTextureParameteriv(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_BORDER_COLOR, values);
        return;
    }
#else
    Q_UNUSED(r);
    Q_UNUSED(g);
    Q_UNUSED(b);
    Q_UNUSED(a);
#endif
    qWarning("QOpenGLTexture: Border color is not supported");

    // TODO Handle case of using glTextureParameterIuiv() based on format
}

/*!
    Returns the borderColor of this texture.

    \sa setBorderColor()
*/
QColor QOpenGLTexture::borderColor() const
{
    Q_D(const QOpenGLTexture);
    QColor c(0.0f, 0.0f, 0.0f, 0.0f);
    if (!d->borderColor.isEmpty()) {
        c.setRedF(d->borderColor.at(0).toFloat());
        c.setGreenF(d->borderColor.at(1).toFloat());
        c.setBlueF(d->borderColor.at(2).toFloat());
        c.setAlphaF(d->borderColor.at(3).toFloat());
    }
    return c;
}

/*!
    Writes the texture border color into the first four elements
    of the array pointed to by \a border.

    \sa setBorderColor()
*/
void QOpenGLTexture::borderColor(float *border) const
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(border);
    if (d->borderColor.isEmpty()) {
        for (int i = 0; i < 4; ++i)
            border[i] = 0.0f;
    } else {
        for (int i = 0; i < 4; ++i)
            border[i] = d->borderColor.at(i).toFloat();
    }
}

/*!
    Writes the texture border color into the first four elements
    of the array pointed to by \a border.

    \overload
*/
void QOpenGLTexture::borderColor(int *border) const
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(border);
    if (d->borderColor.isEmpty()) {
        for (int i = 0; i < 4; ++i)
            border[i] = 0;
    } else {
        for (int i = 0; i < 4; ++i)
            border[i] = d->borderColor.at(i).toInt();
    }
}

/*!
    Writes the texture border color into the first four elements
    of the array pointed to by \a border.

    \overload
*/
void QOpenGLTexture::borderColor(unsigned int *border) const
{
    Q_D(const QOpenGLTexture);
    Q_ASSERT(border);
    if (d->borderColor.isEmpty()) {
        for (int i = 0; i < 4; ++i)
            border[i] = 0;
    } else {
        for (int i = 0; i < 4; ++i)
            border[i] = d->borderColor.at(i).toUInt();
    }
}

/*!
    Sets the minimum level of detail to \a value. This limits the selection of highest
    resolution mipmap (lowest mipmap level). The default value is -1000.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa minimumLevelOfDetail(), setMaximumLevelOfDetail(), setLevelOfDetailRange()
*/
void QOpenGLTexture::setMinimumLevelOfDetail(float value)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        Q_ASSERT(value < d->maxLevelOfDetail);
        d->minLevelOfDetail = value;
        d->texFuncs->glTextureParameterf(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MIN_LOD, value);
        return;
    }
#else
    Q_UNUSED(value);
#endif
    qWarning("QOpenGLTexture: Detail level is not supported");
}

/*!
    Returns the minimum level of detail parameter.

    \sa setMinimumLevelOfDetail(), maximumLevelOfDetail(), levelOfDetailRange()
*/
float QOpenGLTexture::minimumLevelOfDetail() const
{
    Q_D(const QOpenGLTexture);
    return d->minLevelOfDetail;
}

/*!
    Sets the maximum level of detail to \a value. This limits the selection of lowest
    resolution mipmap (highest mipmap level). The default value is 1000.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa maximumLevelOfDetail(), setMinimumLevelOfDetail(), setLevelOfDetailRange()
*/
void QOpenGLTexture::setMaximumLevelOfDetail(float value)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        Q_ASSERT(value > d->minLevelOfDetail);
        d->maxLevelOfDetail = value;
        d->texFuncs->glTextureParameterf(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAX_LOD, value);
        return;
    }
#else
    Q_UNUSED(value);
#endif
    qWarning("QOpenGLTexture: Detail level is not supported");
}

/*!
    Returns the maximum level of detail parameter.

    \sa setMaximumLevelOfDetail(), minimumLevelOfDetail(), levelOfDetailRange()
*/
float QOpenGLTexture::maximumLevelOfDetail() const
{
    Q_D(const QOpenGLTexture);
    return d->maxLevelOfDetail;
}

/*!
    Sets the minimum level of detail parameters to \a min and the maximum level
    to \a max.
    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa levelOfDetailRange(), setMinimumLevelOfDetail(), setMaximumLevelOfDetail()
*/
void QOpenGLTexture::setLevelOfDetailRange(float min, float max)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        Q_ASSERT(min < max);
        d->minLevelOfDetail = min;
        d->maxLevelOfDetail = max;
        d->texFuncs->glTextureParameterf(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MIN_LOD, min);
        d->texFuncs->glTextureParameterf(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_MAX_LOD, max);
        return;
    }
#else
    Q_UNUSED(min);
    Q_UNUSED(max);
#endif
    qWarning("QOpenGLTexture: Detail level is not supported");
}

/*!
    Returns the minimum and maximum level of detail parameters.

    \sa setLevelOfDetailRange(), minimumLevelOfDetail(), maximumLevelOfDetail()
*/
QPair<float, float> QOpenGLTexture::levelOfDetailRange() const
{
    Q_D(const QOpenGLTexture);
    return qMakePair(d->minLevelOfDetail, d->maxLevelOfDetail);
}

/*!
    Sets the level of detail bias to \a bias.
    Level of detail bias affects the point at which mipmapping levels change.
    Increasing values for level of detail bias makes the overall images blurrier
    or smoother. Decreasing values make the overall images sharper.

    \note This function has no effect on Qt built for OpenGL ES 2.
    \sa levelofDetailBias()
*/
void QOpenGLTexture::setLevelofDetailBias(float bias)
{
#if !defined(QT_OPENGL_ES_2)
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGLTexture);
        d->create();
        Q_ASSERT(d->texFuncs);
        Q_ASSERT(d->textureId);
        d->levelOfDetailBias = bias;
        d->texFuncs->glTextureParameterf(d->textureId, d->target, d->bindingTarget, GL_TEXTURE_LOD_BIAS, bias);
        return;
    }
#else
    Q_UNUSED(bias);
#endif
    qWarning("QOpenGLTexture: Detail level is not supported");
}

/*!
    Returns the level of detail bias parameter.

    \sa setLevelofDetailBias()
*/
float QOpenGLTexture::levelofDetailBias() const
{
    Q_D(const QOpenGLTexture);
    return d->levelOfDetailBias;
}

QT_END_NAMESPACE
