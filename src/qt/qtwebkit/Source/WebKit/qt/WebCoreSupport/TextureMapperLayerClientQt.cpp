/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TextureMapperLayerClientQt.h"

#if USE(ACCELERATED_COMPOSITING)

#include "FrameView.h"
#include "GraphicsLayerTextureMapper.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "TextureMapperLayer.h"

using namespace WebCore;

TextureMapperLayerClientQt::TextureMapperLayerClientQt(QWebFrameAdapter* frame)
    : m_frame(frame)
    , m_syncTimer(this, &TextureMapperLayerClientQt::syncLayers)
    , m_rootTextureMapperLayer(0)
{
}

TextureMapperLayerClientQt::~TextureMapperLayerClientQt()
{
    m_rootTextureMapperLayer = 0;
}

void TextureMapperLayerClientQt::syncRootLayer()
{
    m_rootGraphicsLayer->flushCompositingStateForThisLayerOnly();
}

void TextureMapperLayerClientQt::markForSync(bool scheduleSync)
{
    if (m_syncTimer.isActive())
        return;
    m_syncTimer.startOneShot(0);
}

TextureMapperLayer* TextureMapperLayerClientQt::rootLayer()
{
    return toTextureMapperLayer(m_rootGraphicsLayer.get());
}

void TextureMapperLayerClientQt::setRootGraphicsLayer(GraphicsLayer* layer)
{
    if (layer) {
        m_rootGraphicsLayer = GraphicsLayer::create(0);
        m_rootTextureMapperLayer = toTextureMapperLayer(m_rootGraphicsLayer.get());
        m_rootGraphicsLayer->addChild(layer);
        m_rootGraphicsLayer->setDrawsContent(false);
        m_rootGraphicsLayer->setMasksToBounds(false);
        m_rootGraphicsLayer->setSize(IntSize(1, 1));
        TextureMapper::AccelerationMode mode = TextureMapper::SoftwareMode;
        if (m_frame->pageAdapter->client->makeOpenGLContextCurrentIfAvailable())
            mode = TextureMapper::OpenGLMode;
        m_textureMapper = TextureMapper::create(mode);
        m_rootTextureMapperLayer->setTextureMapper(m_textureMapper.get());
        syncRootLayer();
    } else {
        m_rootGraphicsLayer.clear();
        m_rootTextureMapperLayer = 0;
    }
}

void TextureMapperLayerClientQt::syncLayers(Timer<TextureMapperLayerClientQt>*)
{
    if (m_rootGraphicsLayer)
        syncRootLayer();

    m_frame->frame->view()->flushCompositingStateIncludingSubframes();

    if (!m_rootGraphicsLayer)
        return;

    if (rootLayer()->descendantsOrSelfHaveRunningAnimations() && !m_syncTimer.isActive())
        m_syncTimer.startOneShot(1.0 / 60.0);

    m_frame->pageAdapter->client->repaintViewport();
}

void TextureMapperLayerClientQt::renderCompositedLayers(GraphicsContext* context, const IntRect& clip)
{
    if (!m_rootTextureMapperLayer || !m_textureMapper)
        return;

    m_textureMapper->setGraphicsContext(context);
    // GraphicsContext::imageInterpolationQuality is always InterpolationDefault here,
    // but 'default' may be interpreted differently due to a different backend QPainter,
    // so we need to set an explicit imageInterpolationQuality.
    if (context->platformContext()->renderHints() & QPainter::SmoothPixmapTransform)
        m_textureMapper->setImageInterpolationQuality(WebCore::InterpolationMedium);
    else
        m_textureMapper->setImageInterpolationQuality(WebCore::InterpolationNone);

    m_textureMapper->setTextDrawingMode(context->textDrawingMode());
    QPainter* painter = context->platformContext();
    QTransform transform;
    if (m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode) {
        // TextureMapperGL needs to duplicate the entire transform QPainter would do,
        // including the transforms QPainter would normally do behind the scenes.
        transform = painter->deviceTransform();
    } else {
        // TextureMapperImageBuffer needs a transform that can be used
        // with QPainter::setWorldTransform.
        transform = painter->worldTransform();
    }
    const TransformationMatrix matrix(
        transform.m11(), transform.m12(), 0, transform.m13(),
        transform.m21(), transform.m22(), 0, transform.m23(),
        0, 0, 1, 0,
        transform.m31(), transform.m32(), 0, transform.m33()
        );
    if (m_rootGraphicsLayer->opacity() != painter->opacity() || m_rootGraphicsLayer->transform() != matrix) {
        m_rootGraphicsLayer->setOpacity(painter->opacity());
        m_rootGraphicsLayer->setTransform(matrix);
        m_rootGraphicsLayer->flushCompositingStateForThisLayerOnly();
    }
    m_textureMapper->beginPainting();
    m_textureMapper->beginClip(matrix, clip);
    m_rootTextureMapperLayer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), IntPoint::zero(), matrix);
    m_textureMapper->endClip();
    m_textureMapper->endPainting();
}

#endif
