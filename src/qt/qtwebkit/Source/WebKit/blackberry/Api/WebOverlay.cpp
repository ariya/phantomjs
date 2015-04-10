/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "WebOverlay.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerWebKitThread.h"
#include "NotImplemented.h"
#include "TextureCacheCompositingThread.h"
#include "WebAnimation.h"
#include "WebAnimation_p.h"
#include "WebOverlayClient.h"
#include "WebOverlayOverride.h"
#include "WebOverlay_p.h"
#include "WebPageCompositorClient.h"
#include "WebPageCompositor_p.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformString.h>
#include <GLES2/gl2.h>

using namespace WebCore;
using BlackBerry::Platform::Graphics::GLES2Program;

namespace BlackBerry {
namespace WebKit {

WebOverlay::WebOverlay()
    : d(0)
{
    if (Platform::webKitThreadMessageClient()->isCurrentThread()) {
        d = new WebOverlayPrivateWebKitThread;
        d->q = this;
    } else if (Platform::userInterfaceThreadMessageClient()->isCurrentThread()) {
        d = new WebOverlayPrivateCompositingThread;
        d->q = this;
    }
}

WebOverlay::WebOverlay(GraphicsLayerClient* client)
    : d(0)
{
    d = new WebOverlayPrivateWebKitThread(client);
    d->q = this;
}

WebOverlay::~WebOverlay()
{
    delete d;
}

Platform::FloatPoint WebOverlay::position() const
{
    return d->position();
}

void WebOverlay::setPosition(const Platform::FloatPoint& position)
{
    d->setPosition(position);
}

Platform::FloatPoint WebOverlay::anchorPoint() const
{
    return d->anchorPoint();
}

void WebOverlay::setAnchorPoint(const Platform::FloatPoint& anchor)
{
    d->setAnchorPoint(anchor);
}

Platform::FloatSize WebOverlay::size() const
{
    return d->size();
}

void WebOverlay::setSize(const Platform::FloatSize& size)
{
    d->setSize(size);
}

bool WebOverlay::sizeIsScaleInvariant() const
{
    return d->sizeIsScaleInvariant();
}

void WebOverlay::setSizeIsScaleInvariant(bool invariant)
{
    d->setSizeIsScaleInvariant(invariant);
}

Platform::TransformationMatrix WebOverlay::transform() const
{
    // FIXME: There is no WebCore::TranformationMatrix interoperability
    // with Platform::TransformationMatrix
    TransformationMatrix transform = d->transform();
    return reinterpret_cast<const Platform::TransformationMatrix&>(transform);
}

void WebOverlay::setTransform(const Platform::TransformationMatrix& transform)
{
    d->setTransform(reinterpret_cast<const TransformationMatrix&>(transform));
}

float WebOverlay::opacity() const
{
    return d->opacity();
}

void WebOverlay::setOpacity(float opacity)
{
    d->setOpacity(opacity);
}

void WebOverlay::addAnimation(const WebAnimation& animation)
{
    d->addAnimation(animation.d->name, animation.d->animation.get(), animation.d->keyframes);
}

void WebOverlay::removeAnimation(const BlackBerry::Platform::String& name)
{
    d->removeAnimation(name);
}

Platform::IntRect WebOverlay::pixelViewportRect() const
{
    return d->pixelViewportRect();
}

WebOverlay* WebOverlay::parent() const
{
    return d->parent;
}

bool WebOverlay::addChild(WebOverlay* overlay)
{
    if (overlay->d->nativeThread != d->nativeThread)
        return false;

    overlay->d->parent = this;
    d->addChild(overlay->d);
    return true;
}

void WebOverlay::removeFromParent()
{
    d->removeFromParent();
    d->parent = 0;
}

void WebOverlay::setContentsToImage(const unsigned char* data, const Platform::IntSize& imageSize, ImageDataAdoptionType adoptionType)
{
    d->setContentsToImage(data, imageSize, adoptionType);
}

void WebOverlay::setContentsToColor(int r, int g, int b, int a)
{
    d->setContentsToColor(Color(r, g, b, a));
}

void WebOverlay::setDrawsContent(bool drawsContent)
{
    d->setDrawsContent(drawsContent);
}

void WebOverlay::invalidate()
{
    d->invalidate();
}

void WebOverlay::setClient(WebOverlayClient* client)
{
    d->setClient(client);
}

WebOverlayOverride* WebOverlay::override()
{
    // Must be called on UI thread
    if (!Platform::userInterfaceThreadMessageClient()->isCurrentThread())
        return 0;

    return d->override();
}

void WebOverlay::resetOverrides()
{
    d->resetOverrides();
}

WebPagePrivate* WebOverlayPrivate::page() const
{
    if (m_page)
        return m_page;

    if (parent)
        return parent->d->page();

    return 0;
}

WebOverlayOverride* WebOverlayPrivate::override()
{
    if (!m_override)
        m_override = adoptPtr(new WebOverlayOverride(this));

    // Page might have changed if we were removed from the page and added to
    // some other page.
    m_override->d->setPage(page());

    return m_override.get();
}

void WebOverlayPrivate::drawContents(Platform::Graphics::Drawable* drawable)
{
    if (!client)
        return;

    client->drawOverlayContents(q, drawable);
}

void WebOverlayPrivate::scheduleCompositingRun()
{
    if (!page())
        return;

    page()->scheduleCompositingRun();
}

WebOverlayPrivateWebKitThread::WebOverlayPrivateWebKitThread(GraphicsLayerClient* client)
    : m_layer(GraphicsLayer::create(client ? client : this))
{
    m_layerCompositingThread = m_layer->platformLayer()->layerCompositingThread();
}

FloatPoint WebOverlayPrivateWebKitThread::position() const
{
    return m_layer->position();
}

void WebOverlayPrivateWebKitThread::setPosition(const FloatPoint& position)
{
    m_layer->setPosition(position);
}

FloatPoint WebOverlayPrivateWebKitThread::anchorPoint() const
{
    FloatPoint3D anchor = m_layer->anchorPoint();
    return FloatPoint(anchor.x(), anchor.y());
}

void WebOverlayPrivateWebKitThread::setAnchorPoint(const FloatPoint& anchor)
{
    m_layer->setAnchorPoint(FloatPoint3D(anchor.x(), anchor.y(), 0));
}

FloatSize WebOverlayPrivateWebKitThread::size() const
{
    return m_layer->size();
}

void WebOverlayPrivateWebKitThread::setSize(const FloatSize& size)
{
    m_layer->setSize(size);
}

bool WebOverlayPrivateWebKitThread::sizeIsScaleInvariant() const
{
    return m_layer->platformLayer()->sizeIsScaleInvariant();
}

void WebOverlayPrivateWebKitThread::setSizeIsScaleInvariant(bool invariant)
{
    m_layer->platformLayer()->setSizeIsScaleInvariant(invariant);
}

TransformationMatrix WebOverlayPrivateWebKitThread::transform() const
{
    return m_layer->transform();
}

void WebOverlayPrivateWebKitThread::setTransform(const TransformationMatrix& transform)
{
    m_layer->setTransform(transform);
}

float WebOverlayPrivateWebKitThread::opacity() const
{
    return m_layer->opacity();
}

void WebOverlayPrivateWebKitThread::setOpacity(float opacity)
{
    m_layer->setOpacity(opacity);
}

void WebOverlayPrivateWebKitThread::addAnimation(const String& name, Animation* animation, const KeyframeValueList& keyframes)
{
    IntSize size(m_layer->size().width(), m_layer->size().height());
    m_layer->addAnimation(keyframes, size, animation, name, 0);
}

void WebOverlayPrivateWebKitThread::removeAnimation(const String& name)
{
    m_layer->removeAnimation(name);
}

Platform::IntRect WebOverlayPrivateWebKitThread::pixelViewportRect() const
{
    notImplemented();
    return Platform::IntRect();
}

void WebOverlayPrivateWebKitThread::addChild(WebOverlayPrivate* overlay)
{
    m_layer->addChild(static_cast<WebOverlayPrivateWebKitThread*>(overlay)->m_layer.get());
}

void WebOverlayPrivateWebKitThread::removeFromParent()
{
    m_layer->removeFromParent();
}

void WebOverlayPrivateWebKitThread::setContentsToImage(const unsigned char*, const WebCore::IntSize&, WebOverlay::ImageDataAdoptionType)
{
    notImplemented();
}

void WebOverlayPrivateWebKitThread::setContentsToColor(const Color&)
{
    notImplemented();
}

void WebOverlayPrivateWebKitThread::setDrawsContent(bool drawsContent)
{
    m_layer->setDrawsContent(drawsContent);
}

void WebOverlayPrivateWebKitThread::clear()
{
    setSize(FloatSize(0, 0));
}

void WebOverlayPrivateWebKitThread::invalidate()
{
    m_layer->setNeedsDisplay();
}

void WebOverlayPrivateWebKitThread::resetOverrides()
{
    if (Platform::webKitThreadMessageClient()->isCurrentThread())
        m_layer->platformLayer()->clearOverride();
    else if (Platform::userInterfaceThreadMessageClient()->isCurrentThread()) {
        m_layerCompositingThread->clearOverride();
        scheduleCompositingRun();
    }
}

void WebOverlayPrivateWebKitThread::notifyFlushRequired(const WebCore::GraphicsLayer*)
{
    if (WebPagePrivate* page = this->page())
        page->scheduleRootLayerCommit();
}

void WebOverlayPrivateWebKitThread::paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext& c, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&)
{
    drawContents(c.platformContext());
}

WebOverlayLayerCompositingThreadClient::WebOverlayLayerCompositingThreadClient(WebOverlayPrivate* overlay)
    : m_drawsContent(false)
    , m_overlay(overlay)
{
}

void WebOverlayLayerCompositingThreadClient::setDrawsContent(bool drawsContent)
{
    m_drawsContent = drawsContent;
}

void WebOverlayLayerCompositingThreadClient::invalidate()
{
    m_texture.clear();
    clearUploadedContents();
}

void WebOverlayLayerCompositingThreadClient::setContentsToImage(const BlackBerry::Platform::Graphics::TiledImage& image)
{
    m_image = image;
    m_color = Color();
    m_texture.clear();
    clearUploadedContents();
}

void WebOverlayLayerCompositingThreadClient::clearUploadedContents()
{
    m_uploadedImage = BlackBerry::Platform::Graphics::TiledImage();
}

void WebOverlayLayerCompositingThreadClient::setContentsToColor(const Color& color)
{
    m_image = BlackBerry::Platform::Graphics::TiledImage();
    m_color = color;
    m_texture.clear();
    clearUploadedContents();
}

void WebOverlayLayerCompositingThreadClient::layerCompositingThreadDestroyed(WebCore::LayerCompositingThread*)
{
    delete this;
}

void WebOverlayLayerCompositingThreadClient::layerVisibilityChanged(LayerCompositingThread*, bool)
{
}

void WebOverlayLayerCompositingThreadClient::uploadTexturesIfNeeded(LayerCompositingThread* layer)
{
    if (m_image.isNull() && !m_color.isValid() && !m_drawsContent)
        return;

    if (m_texture && m_texture->buffer())
        return;

    if (m_color.isValid()) {
        m_texture = textureCacheCompositingThread()->textureForColor(m_color);
        return;
    }

    BlackBerry::Platform::Graphics::Buffer* textureContents = 0;
    IntSize textureSize;

    if (m_drawsContent) {
        if (!m_overlay || !m_overlay->client)
            return;

        textureSize = layer->bounds();
        textureContents = BlackBerry::Platform::Graphics::createBuffer(IntSize(0, 0), BlackBerry::Platform::Graphics::NeverBacked);
        if (!textureContents)
            return;

        clearBuffer(textureContents, 0, 0, 0, 0);
        PlatformGraphicsContext* platformContext = lockBufferDrawable(textureContents);
        if (!platformContext) {
            destroyBuffer(textureContents);
            return;
        }
        double transform[] = {
            1, 0,
            0, 1,
            -layer->bounds().width() / 2.0, -layer->bounds().height() / 2.0
        };
        platformContext->setTransform(transform);
        m_overlay->client->drawOverlayContents(m_overlay->q, platformContext);

        releaseBufferDrawable(textureContents);
    } else if (!m_image.isNull()) {
        textureSize = IntSize(m_image.width(), m_image.height());
        textureContents = BlackBerry::Platform::Graphics::createBuffer(IntSize(0, 0), BlackBerry::Platform::Graphics::NeverBacked);
        if (!textureContents)
            return;

        PlatformGraphicsContext* platformContext = BlackBerry::Platform::Graphics::lockBufferDrawable(textureContents);
        if (!platformContext) {
            destroyBuffer(textureContents);
            return;
        }

        AffineTransform transform;
        platformContext->getTransform(reinterpret_cast<double*>(&transform));
        transform.translate(-m_image.width() / 2.0, -m_image.height() / 2.0);
        platformContext->setTransform(reinterpret_cast<double*>(&transform));

        FloatRect rect(0, 0, m_image.width(), m_image.height());
        platformContext->addImage(rect, rect, &m_image);
        releaseBufferDrawable(textureContents);
        m_uploadedImage = m_image;
    }

    m_texture = LayerTexture::create();
    m_texture->protect(IntSize(), BlackBerry::Platform::Graphics::BackedWhenNecessary);
    m_texture->updateContents(textureContents);
}

void WebOverlayLayerCompositingThreadClient::drawTextures(LayerCompositingThread* layer, const GLES2Program&, double scale, const FloatRect& /*clipRect*/)
{
    if (!m_texture || !m_texture->buffer())
        return;

    TransformationMatrix matrix = layer->drawTransform();
    if (!m_image.isNull()) {
        if (m_image.size().isEmpty())
            return;

        matrix.scaleNonUniform(static_cast<double>(layer->bounds().width()) / m_image.width(), static_cast<double>(layer->bounds().height()) / m_image.height());
    }
    matrix.scale(layer->sizeIsScaleInvariant() ? 1.0 / scale : 1.0);
    blitToBuffer(0, m_texture->buffer(), reinterpret_cast<BlackBerry::Platform::TransformationMatrix&>(matrix),
        BlackBerry::Platform::Graphics::SourceOver, static_cast<unsigned char>(layer->drawOpacity() * 255));
}

void WebOverlayLayerCompositingThreadClient::deleteTextures(LayerCompositingThread*)
{
    m_texture.clear();
    clearUploadedContents();
}

WebOverlayPrivateCompositingThread::WebOverlayPrivateCompositingThread(PassRefPtr<LayerCompositingThread> layerCompositingThread)
    : m_layerCompositingThreadClient(0)
    , m_data(0)
{
    m_layerCompositingThread = layerCompositingThread;
}

WebOverlayPrivateCompositingThread::WebOverlayPrivateCompositingThread()
    : m_layerCompositingThreadClient(new WebOverlayLayerCompositingThreadClient(this))
{
    m_layerCompositingThread = LayerCompositingThread::create(LayerData::CustomLayer, m_layerCompositingThreadClient);
}

WebOverlayPrivateCompositingThread::~WebOverlayPrivateCompositingThread()
{
    if (m_layerCompositingThreadClient)
        m_layerCompositingThreadClient->overlayDestroyed();
}

FloatPoint WebOverlayPrivateCompositingThread::position() const
{
    return m_layerCompositingThread->position();
}

void WebOverlayPrivateCompositingThread::setPosition(const FloatPoint& position)
{
    m_layerCompositingThread->setPosition(position);
    scheduleCompositingRun();
}

FloatPoint WebOverlayPrivateCompositingThread::anchorPoint() const
{
    return m_layerCompositingThread->anchorPoint();
}

void WebOverlayPrivateCompositingThread::setAnchorPoint(const FloatPoint& anchor)
{
    m_layerCompositingThread->setAnchorPoint(anchor);
    scheduleCompositingRun();
}

FloatSize WebOverlayPrivateCompositingThread::size() const
{
    IntSize bounds = m_layerCompositingThread->bounds();
    return FloatSize(bounds.width(), bounds.height());
}

void WebOverlayPrivateCompositingThread::setSize(const FloatSize& size)
{
    m_layerCompositingThread->setBounds(IntSize(size.width(), size.height()));
    scheduleCompositingRun();
}

bool WebOverlayPrivateCompositingThread::sizeIsScaleInvariant() const
{
    return m_layerCompositingThread->sizeIsScaleInvariant();
}

void WebOverlayPrivateCompositingThread::setSizeIsScaleInvariant(bool invariant)
{
    m_layerCompositingThread->setSizeIsScaleInvariant(invariant);
    scheduleCompositingRun();
}

TransformationMatrix WebOverlayPrivateCompositingThread::transform() const
{
    return m_layerCompositingThread->transform();
}

void WebOverlayPrivateCompositingThread::setTransform(const TransformationMatrix& transform)
{
    m_layerCompositingThread->setTransform(transform);
    scheduleCompositingRun();
}

float WebOverlayPrivateCompositingThread::opacity() const
{
    return m_layerCompositingThread->opacity();
}

void WebOverlayPrivateCompositingThread::setOpacity(float opacity)
{
    m_layerCompositingThread->setOpacity(opacity);
    scheduleCompositingRun();
}

void WebOverlayPrivateCompositingThread::addAnimation(const String& name, Animation* animation, const KeyframeValueList& keyframes)
{
    IntSize boxSize = m_layerCompositingThread->bounds();
    RefPtr<LayerAnimation> layerAnimation = LayerAnimation::create(keyframes, boxSize, animation, name, 0.0);

    // FIXME: Unfortunately WebPageCompositorClient::requestAnimationFrame uses a different time coordinate system
    // than accelerated animations, so we can't use the time returned by WebPageCompositorClient::requestAnimationFrame()
    // for starttime.
    layerAnimation->setStartTime(currentTime());

    m_layerCompositingThread->addAnimation(layerAnimation.get());
    scheduleCompositingRun();
}

void WebOverlayPrivateCompositingThread::removeAnimation(const String& name)
{
    m_layerCompositingThread->removeAnimation(name);
    scheduleCompositingRun();
}

Platform::IntRect WebOverlayPrivateCompositingThread::pixelViewportRect() const
{
    if (LayerRenderer* renderer = m_layerCompositingThread->layerRenderer())
        return renderer->toPixelViewportCoordinates(m_layerCompositingThread->boundingBox());

    return Platform::IntRect();
}

void WebOverlayPrivateCompositingThread::addChild(WebOverlayPrivate* overlay)
{
    m_layerCompositingThread->addSublayer(overlay->layerCompositingThread());
    scheduleCompositingRun();
}

void WebOverlayPrivateCompositingThread::removeFromParent()
{
    if (page() && m_layerCompositingThread->superlayer() == page()->compositor()->compositingThreadOverlayLayer())
        page()->m_webPage->removeCompositingThreadOverlay(q);
    else
        m_layerCompositingThread->removeFromSuperlayer();
    scheduleCompositingRun();
}

void WebOverlayPrivateCompositingThread::setContentsToImage(const unsigned char* data, const IntSize& imageSize, WebOverlay::ImageDataAdoptionType)
{
    if (!m_layerCompositingThreadClient)
        return;

    if (data == m_data)
        return;

    m_data = data;

    BlackBerry::Platform::Graphics::TiledImage image = BlackBerry::Platform::Graphics::TiledImage(imageSize, reinterpret_cast_ptr<const unsigned*>(data), true, BlackBerry::Platform::Graphics::TiledImage::Hardware);

    m_layerCompositingThreadClient->setContentsToImage(image);
    m_layerCompositingThread->setNeedsTexture(true);
}

void WebOverlayPrivateCompositingThread::setContentsToColor(const Color& color)
{
    if (!m_layerCompositingThreadClient)
        return;

    m_layerCompositingThreadClient->setContentsToColor(color);
    m_layerCompositingThread->setNeedsTexture(true);
}

void WebOverlayPrivateCompositingThread::setDrawsContent(bool drawsContent)
{
    if (!m_layerCompositingThreadClient)
        return;

    m_layerCompositingThreadClient->setDrawsContent(drawsContent);
    m_layerCompositingThread->setNeedsTexture(true);
}

void WebOverlayPrivateCompositingThread::clear()
{
    m_layerCompositingThread->deleteTextures();
}

void WebOverlayPrivateCompositingThread::invalidate()
{
    if (!m_layerCompositingThreadClient || !m_layerCompositingThreadClient->drawsContent())
        return;

    m_layerCompositingThreadClient->invalidate();
    scheduleCompositingRun();
}

void WebOverlayPrivateCompositingThread::resetOverrides()
{
    m_layerCompositingThread->clearOverride();
    scheduleCompositingRun();
}

}
}
#else // USE(ACCELERATED_COMPOSITING)
namespace BlackBerry {
namespace WebKit {

WebOverlay::WebOverlay()
{
}

WebOverlay::~WebOverlay()
{
}

Platform::FloatPoint WebOverlay::position() const
{
    return Platform::FloatPoint();
}

void WebOverlay::setPosition(const Platform::FloatPoint&)
{
}

Platform::FloatPoint WebOverlay::anchorPoint() const
{
    return Platform::FloatPoint();
}

void WebOverlay::setAnchorPoint(const Platform::FloatPoint&)
{
}

Platform::FloatSize WebOverlay::size() const
{
    return Platform::FloatSize();
}

void WebOverlay::setSize(const Platform::FloatSize&)
{
}

Platform::TransformationMatrix WebOverlay::transform() const
{
    return Platform::TransformationMatrix();
}

void WebOverlay::setTransform(const Platform::TransformationMatrix&)
{
}

float WebOverlay::opacity() const
{
    return 1.0f;
}

void WebOverlay::setOpacity(float)
{
}

Platform::IntRect WebOverlay::pixelViewportRect() const
{
    return Platform::IntRect();
}

WebOverlay* WebOverlay::parent() const
{
    return 0;
}

bool WebOverlay::addChild(WebOverlay*)
{
    return false;
}

void WebOverlay::removeFromParent()
{
}

void WebOverlay::addAnimation(const WebAnimation&)
{
}

void WebOverlay::removeAnimation(const BlackBerry::Platform::String&)
{
}

void WebOverlay::setContentsToImage(const unsigned char*, const Platform::IntSize&, ImageDataAdoptionType)
{
}

void WebOverlay::setContentsToColor(int, int, int, int)
{
}

void WebOverlay::setDrawsContent(bool)
{
}

void WebOverlay::invalidate()
{
}

void WebOverlay::setClient(WebOverlayClient*)
{
}

WebOverlayOverride* WebOverlay::override()
{
}

void WebOverlay::resetOverrides()
{
}

}
}
#endif // USE(ACCELERATED_COMPOSITING)
