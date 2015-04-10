/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerWebKitThread.h"

#include "GraphicsContext.h"
#include "LayerCompositingThread.h"
#include "LayerMessage.h"
#include "RenderLayerBacking.h"
#include "TransformationMatrix.h"

#include <BlackBerryPlatformGraphics.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

using namespace std;

PassRefPtr<LayerWebKitThread> LayerWebKitThread::create(LayerType type, GraphicsLayerBlackBerry* owner)
{
    return adoptRef(new LayerWebKitThread(type, owner));
}

LayerWebKitThread::LayerWebKitThread(LayerType type, GraphicsLayerBlackBerry* owner)
    : LayerData(type)
    , m_owner(owner)
    , m_superlayer(0)
    , m_contents(0)
    , m_isDrawable(false)
    , m_isMask(false)
    , m_animationsChanged(false)
    , m_clearOverrideOnCommit(false)
#if ENABLE(CSS_FILTERS)
    , m_filtersChanged(false)
#endif
    , m_didStartAnimations(false)
{
    if (type == Layer)
        m_tiler = LayerTiler::create(this);
    m_layerCompositingThread = LayerCompositingThread::create(type, m_tiler.get());
}

LayerWebKitThread::~LayerWebKitThread()
{
    if (m_tiler)
        m_tiler->layerWebKitThreadDestroyed();

    // Our superlayer should be holding a reference to us so there should be no
    // way for us to be destroyed while we still have a superlayer.
    ASSERT(!superlayer());

    // Remove the superlayer reference from all sublayers.
    removeAll(m_sublayers);
    removeAll(m_overlays);
}

void LayerWebKitThread::paintContents(BlackBerry::Platform::Graphics::Buffer* buffer, const IntRect& contentsRect, double scale)
{
    if (!drawsContent() && !contents())
        return;

    if (!buffer)
        return;

    IntRect untransformedContentsRect = contentsRect;
    FloatRect clipRect = contentsRect;
    if (scale != 1.0) {
        TransformationMatrix matrix;
        matrix.scale(1.0 / scale);
        untransformedContentsRect = matrix.mapRect(contentsRect);
        clipRect = matrix.mapRect(clipRect);

        // We extract from the contentsRect but draw a slightly larger region than
        // we were told to, in order to avoid pixels being rendered only partially.
        const int atLeastOneDevicePixel = static_cast<int>(ceilf(1.0 / scale));
        untransformedContentsRect.inflate(atLeastOneDevicePixel);
    }

    PlatformGraphicsContext* platformContext = lockBufferDrawable(buffer);
    GraphicsContext graphicsContext(platformContext);
    if (contents()) {
        // Images needs to be centered and will be scaled to fit the bounds on the compositing thread
        if (!contents()->size().isEmpty())
            graphicsContext.drawImage(contents(), ColorSpaceDeviceRGB, IntPoint(0, 0));
    } else {
        graphicsContext.translate(-contentsRect.x(), -contentsRect.y());
        graphicsContext.scale(FloatSize(scale, scale));
        graphicsContext.clip(clipRect);
        m_owner->paintGraphicsLayerContents(graphicsContext, untransformedContentsRect);
    }

    releaseBufferDrawable(buffer);
}

void LayerWebKitThread::updateTextureContentsIfNeeded()
{
    if (m_tiler)
        m_tiler->updateTextureContentsIfNeeded(m_isMask ? 1.0 : contentsScale());
}

void LayerWebKitThread::commitPendingTextureUploads()
{
    layerCompositingThread()->commitPendingTextureUploads();
}

void LayerWebKitThread::setContents(Image* contents)
{
    // Check if the image has changed.
    if (m_contents == contents) {
        // Set needs display for animated images.
        if (contents)
            setNeedsDisplay();
        return;
    }
    m_contents = contents;
    setNeedsTexture(m_isDrawable && (this->contents() || drawsContent() || pluginView()));

    if (m_contents)
        setNeedsDisplay();
    else
        setNeedsCommit();

    // If this layer contains a bitmap image it isn't rerendered at different scale (it is resolution independent)
    m_contentsResolutionIndependent = static_cast<bool>(m_contents);
}

void LayerWebKitThread::setDrawable(bool isDrawable)
{
    if (m_isDrawable == isDrawable)
        return;

    m_isDrawable = isDrawable;

    setNeedsTexture(m_isDrawable && (drawsContent() || contents() || pluginView() || mediaPlayer()));
    setNeedsCommit();
}

void LayerWebKitThread::setNeedsCommit()
{
    // Call notifyFlushRequired(), which in this implementation plumbs through to
    // call scheduleRootLayerCommit() on the WebView, which will cause us to commit
    // changes done on the WebKit thread for display on the Compositing thread.
    if (m_owner)
        m_owner->notifyFlushRequired();
}

void LayerWebKitThread::notifyAnimationsStarted(double time)
{
    if (m_didStartAnimations) {
        m_didStartAnimations = false;
        if (m_owner)
            m_owner->notifyAnimationStarted(time);
    }

    size_t listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        m_sublayers[i]->notifyAnimationsStarted(time);

    listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        m_overlays[i]->notifyAnimationsStarted(time);
}

void LayerWebKitThread::commitOnWebKitThread(double scale)
{
    // Updating texture contents require the latest visibility info.
    updateTextureContents(scale);
}

bool LayerWebKitThread::startAnimations(double time)
{
    bool didStartAnimations = false;
    for (size_t i = 0; i < m_runningAnimations.size(); ++i) {
        if (!m_runningAnimations[i]->startTime()) {
            m_runningAnimations[i]->setStartTime(time);
            m_didStartAnimations = didStartAnimations = true;
        }
    }

    size_t listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        didStartAnimations |= m_sublayers[i]->startAnimations(time);

    listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        didStartAnimations |= m_overlays[i]->startAnimations(time);

    return didStartAnimations;
}

void LayerWebKitThread::updateTextureContents(double scale)
{
    if (m_contentsScale != scale) {
        m_contentsScale = scale;

        // Only web content can redraw at the new scale.
        // Canvas, images, video etc can't.
        if (drawsContent())
            setNeedsDisplay();
    }

    updateTextureContentsIfNeeded();

    if (includeVisibility()) {
        // The RenderLayerBacking cast looks unsafe given that there are two classes
        // derived from GraphicsLayerClient but this code is only reachable for
        // things that produce RenderLayerBacking derivatives; i.e., plugins and media.
        RenderLayer* renderLayer(static_cast<RenderLayerBacking*>(m_owner->client())->owningLayer());
        bool isVisible(renderLayer->hasVisibleContent() || renderLayer->hasVisibleDescendant());
        if (m_isVisible != isVisible) {
            m_isVisible = isVisible;
            setNeedsCommit();
        }
    }

    size_t listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        m_sublayers[i]->updateTextureContents(scale);

    listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        m_overlays[i]->updateTextureContents(scale);

    if (maskLayer())
        maskLayer()->updateTextureContents(scale);

    if (replicaLayer())
        replicaLayer()->updateTextureContents(scale);
}

void LayerWebKitThread::commitOnCompositingThread()
{
    FloatPoint oldPosition = m_position;
    m_position += m_absoluteOffset;
    // Copy the base variables from this object into m_layerCompositingThread
    replicate(m_layerCompositingThread.get());
#if ENABLE(CSS_FILTERS)
    if (m_filtersChanged) {
        m_filtersChanged = false;
        m_layerCompositingThread->setFilterOperationsChanged(true);
    }
#endif
    if (m_animationsChanged) {
        m_layerCompositingThread->setRunningAnimations(m_runningAnimations);
        m_layerCompositingThread->setSuspendedAnimations(m_suspendedAnimations);
        m_animationsChanged = false;
    }
    if (m_clearOverrideOnCommit) {
        m_layerCompositingThread->clearOverride();
        m_clearOverrideOnCommit = false;
    }
    m_position = oldPosition;
    updateLayerHierarchy();

    commitPendingTextureUploads();

    size_t listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        m_sublayers[i]->commitOnCompositingThread();

    listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        m_overlays[i]->commitOnCompositingThread();

    if (maskLayer()) {
        maskLayer()->commitOnCompositingThread();
        layerCompositingThread()->setMaskLayer(maskLayer()->layerCompositingThread());
    } else
        layerCompositingThread()->setMaskLayer(0);

    if (replicaLayer()) {
        replicaLayer()->commitOnCompositingThread();
        layerCompositingThread()->setReplicaLayer(replicaLayer()->layerCompositingThread());
    } else
        layerCompositingThread()->setReplicaLayer(0);
}

void LayerWebKitThread::addSublayer(PassRefPtr<LayerWebKitThread> sublayer)
{
    insert(m_sublayers, sublayer, m_sublayers.size());
}

void LayerWebKitThread::addOverlay(PassRefPtr<LayerWebKitThread> overlay)
{
    insert(m_overlays, overlay, m_overlays.size());
}

void LayerWebKitThread::insert(Vector<RefPtr<LayerWebKitThread> >& list, PassRefPtr<LayerWebKitThread> sublayer, size_t index)
{
    sublayer->removeFromSuperlayer();
    index = min(index, list.size());
    sublayer->setSuperlayer(this);
    list.insert(index, sublayer);

    setNeedsCommit();
}

void LayerWebKitThread::removeFromSuperlayer()
{
    if (m_superlayer)
        m_superlayer->removeSublayerOrOverlay(this);
}

void LayerWebKitThread::removeSublayerOrOverlay(LayerWebKitThread* sublayer)
{
    remove(m_sublayers, sublayer);
    remove(m_overlays, sublayer);
}

void LayerWebKitThread::remove(Vector<RefPtr<LayerWebKitThread> >& vector, LayerWebKitThread* sublayer)
{
    size_t foundIndex = vector.find(sublayer);
    if (foundIndex == notFound)
        return;

    sublayer->setSuperlayer(0);
    vector.remove(foundIndex);

    setNeedsCommit();
}

void LayerWebKitThread::replaceSublayer(LayerWebKitThread* reference, PassRefPtr<LayerWebKitThread> newLayer)
{
    ASSERT_ARG(reference, reference);
    ASSERT_ARG(reference, reference->superlayer() == this);

    if (reference == newLayer)
        return;

    size_t referenceIndex = m_sublayers.find(reference);
    if (referenceIndex == notFound) {
        ASSERT_NOT_REACHED();
        return;
    }

    reference->removeFromSuperlayer();

    if (newLayer) {
        newLayer->removeFromSuperlayer();
        insertSublayer(newLayer, referenceIndex);
    }
}

void LayerWebKitThread::setBounds(const IntSize& size)
{
    if (m_bounds == size)
        return;

    bool firstResize = !m_bounds.width() && !m_bounds.height() && size.width() && size.height();

    m_bounds = size;

    boundsChanged();

    if (firstResize)
        setNeedsDisplay();
    else
        setNeedsCommit();
}

void LayerWebKitThread::setFrame(const FloatRect& rect)
{
    if (rect == m_frame)
        return;

    m_frame = rect;
    setNeedsDisplay();
}

#if ENABLE(CSS_FILTERS)
bool LayerWebKitThread::filtersCanBeComposited(const FilterOperations& filters)
{
    // There is work associated with compositing filters, even if there are zero filters,
    // so if there are no filters, claim we can't composite them.
    if (!filters.size())
        return false;

    for (unsigned i = 0; i < filters.size(); ++i) {
        const FilterOperation* filterOperation = filters.at(i);
        switch (filterOperation->getOperationType()) {
        case FilterOperation::REFERENCE:
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
#endif
            return false;
        default:
            break;
        }
    }

    return true;
}
#endif

const LayerWebKitThread* LayerWebKitThread::rootLayer() const
{
    const LayerWebKitThread* layer = this;
    LayerWebKitThread* superlayer = layer->superlayer();

    while (superlayer) {
        layer = superlayer;
        superlayer = superlayer->superlayer();
    }
    return layer;
}

void LayerWebKitThread::removeAll(Vector<RefPtr<LayerWebKitThread> >& vector)
{
    if (!vector.size())
        return;

    while (vector.size()) {
        RefPtr<LayerWebKitThread> layer = vector[0].get();
        ASSERT(layer->superlayer() == this);
        layer->removeFromSuperlayer();
    }

    setNeedsCommit();
}

void LayerWebKitThread::setSublayers(const Vector<RefPtr<LayerWebKitThread> >& sublayers)
{
    if (sublayers == m_sublayers)
        return;

    removeAllSublayers();
    size_t listSize = sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        addSublayer(sublayers[i]);
}

void LayerWebKitThread::setNeedsDisplayInRect(const FloatRect& dirtyRect)
{
    if (m_tiler)
        m_tiler->setNeedsDisplay(dirtyRect);
    setNeedsCommit(); // FIXME: Replace this with a more targeted message for dirty rect handling with plugin content?
}

void LayerWebKitThread::setNeedsDisplay()
{
    if (m_tiler)
        m_tiler->setNeedsDisplay();
    setNeedsCommit(); // FIXME: Replace this with a more targeted message for dirty rect handling with plugin content?
}

void LayerWebKitThread::updateLayerHierarchy()
{
    m_layerCompositingThread->setSuperlayer(superlayer() ? superlayer()->m_layerCompositingThread.get() : 0);

    Vector<RefPtr<LayerCompositingThread> > sublayers;
    size_t listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        sublayers.append(m_overlays[i]->m_layerCompositingThread.get());
    listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        sublayers.append(m_sublayers[i]->m_layerCompositingThread.get());
    m_layerCompositingThread->setSublayers(sublayers);
}

void LayerWebKitThread::setIsMask(bool isMask)
{
    m_isMask = isMask;
    if (isMask && m_tiler)
        m_tiler->setNeedsBacking(true);
}

void LayerWebKitThread::setRunningAnimations(const Vector<RefPtr<LayerAnimation> >& animations)
{
    m_runningAnimations = animations;
    m_animationsChanged = true;
    setNeedsCommit();
}

void LayerWebKitThread::setSuspendedAnimations(const Vector<RefPtr<LayerAnimation> >& animations)
{
    m_suspendedAnimations = animations;
    m_animationsChanged = true;
    setNeedsCommit();
}

void LayerWebKitThread::releaseLayerResources()
{
    deleteTextures();

    size_t listSize = m_sublayers.size();
    for (size_t i = 0; i < listSize; ++i)
        m_sublayers[i]->releaseLayerResources();

    listSize = m_overlays.size();
    for (size_t i = 0; i < listSize; ++i)
        m_overlays[i]->releaseLayerResources();

    if (maskLayer())
        maskLayer()->releaseLayerResources();

    if (replicaLayer())
        replicaLayer()->releaseLayerResources();
}

IntRect LayerWebKitThread::mapFromTransformed(const IntRect& contentsRect, double scale)
{
    IntRect untransformedContentsRect = contentsRect;

    if (scale != 1.0) {
        TransformationMatrix matrix;
        matrix.scale(1.0 / scale);
        untransformedContentsRect = matrix.mapRect(contentsRect);

        // We extract from the contentsRect but draw a slightly larger region than
        // we were told to, in order to avoid pixels being rendered only partially.
        const int atLeastOneDevicePixel = static_cast<int>(ceilf(1.0 / scale));
        untransformedContentsRect.inflate(atLeastOneDevicePixel);
    }

    return untransformedContentsRect;
}

}

#endif // USE(ACCELERATED_COMPOSITING)
