/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#include "LayerCompositingThread.h"

#include "LayerCompositingThreadClient.h"
#include "LayerMessage.h"
#include "LayerRenderer.h"
#include "LayerRendererClient.h"
#include "LayerUtilities.h"
#include "LayerWebKitThread.h"
#if ENABLE(VIDEO)
#include "MediaPlayer.h"
#include "MediaPlayerPrivateBlackBerry.h"
#endif
#include "PluginView.h"
#include "StdLibExtras.h"
#include "TextureCacheCompositingThread.h"

#include <BlackBerryPlatformGLES2ContextState.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <wtf/Assertions.h>

using BlackBerry::Platform::Graphics::GLES2Program;

namespace WebCore {

void LayerOverride::removeAnimation(const String& name)
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i]->name() == name) {
            m_animations.remove(i);
            return;
        }
    }
}

PassRefPtr<LayerCompositingThread> LayerCompositingThread::create(LayerType type, LayerCompositingThreadClient* client)
{
    return adoptRef(new LayerCompositingThread(type, client));
}

LayerCompositingThread::LayerCompositingThread(LayerType type, LayerCompositingThreadClient* client)
    : LayerData(type)
    , m_layerRenderer(0)
    , m_superlayer(0)
    , m_centerW(0)
    , m_pluginBuffer(0)
    , m_drawOpacity(0)
    , m_visible(false)
    , m_commitScheduled(false)
    , m_client(client)
#if ENABLE(CSS_FILTERS)
    , m_filterOperationsChanged(false)
#endif
{
}

LayerCompositingThread::~LayerCompositingThread()
{
    ASSERT(isCompositingThread());

    ASSERT(!superlayer());

    // Remove the superlayer reference from all sublayers.
    while (m_sublayers.size())
        m_sublayers[0]->removeFromSuperlayer();

    // Delete all allocated textures
    deleteTextures();

    // We just deleted all our textures, no need for the
    // layer renderer to track us anymore
    if (m_layerRenderer)
        m_layerRenderer->removeLayer(this);

    if (m_client)
        m_client->layerCompositingThreadDestroyed(this);
}

void LayerCompositingThread::setLayerRenderer(LayerRenderer* renderer)
{
    // It's not expected that layers will ever switch renderers.
    ASSERT(!renderer || !m_layerRenderer || renderer == m_layerRenderer);

    m_layerRenderer = renderer;
    if (m_layerRenderer)
        m_layerRenderer->addLayer(this);
}

void LayerCompositingThread::deleteTextures()
{
    releaseTextureResources();

    if (m_client)
        m_client->deleteTextures(this);
}

void LayerCompositingThread::setDrawTransform(double scale, const TransformationMatrix& matrix, const TransformationMatrix& projectionMatrix)
{
    m_drawTransform = projectionMatrix * matrix;

    FloatRect boundsRect(-origin(), bounds());

    if (sizeIsScaleInvariant())
        boundsRect.scale(1 / scale);

    m_centerW = 0;
    m_transformedBounds.clear();
    m_ws.clear();
    m_textureCoordinates.clear();
    if (matrix.hasPerspective() && !m_layerRendererSurface) {
        // Perform processing according to http://www.w3.org/TR/css3-transforms 6.2
        // If w < 0 for all four corners of the transformed box, the box is not rendered.
        // If w < 0 for one to three corners of the transformed box, the box
        // must be replaced by a polygon that has any parts with w < 0 cut out.
        // If w = 0, (x′, y′, z′) = (x ⋅ n, y ⋅ n, z ⋅ n)
        // We implement this by intersecting with the image plane, i.e. the last row of the column-major matrix.
        // To avoid problems with w close to 0, we use w = epsilon as the near plane by subtracting epsilon from matrix.m44().
        const float epsilon = 1e-3;
        Vector<FloatPoint3D, 4> quad = toVector<FloatPoint3D, 4>(boundsRect);
        Vector<FloatPoint3D, 4> polygon = intersect(quad, LayerClipPlane(FloatPoint3D(matrix.m14(), matrix.m24(), matrix.m34()), matrix.m44() - epsilon));

        // Compute the clipped texture coordinates.
        if (polygon != quad) {
            for (size_t i = 0; i < polygon.size(); ++i) {
                FloatPoint3D& p = polygon[i];
                m_textureCoordinates.append(FloatPoint(p.x() / boundsRect.width() + 0.5f, p.y() / boundsRect.height() + 0.5f));
            }
        }

        // If w > 0, (x′, y′, z′) = (x/w, y/w, z/w)
        for (size_t i = 0; i < polygon.size(); ++i) {
            float w;
            FloatPoint3D p = multVecMatrix(matrix, polygon[i], w);
            if (w != 1) {
                p.setX(p.x() / w);
                p.setY(p.y() / w);
                p.setZ(p.z() / w);
            }

            FloatPoint3D q = projectionMatrix.mapPoint(p);
            m_transformedBounds.append(FloatPoint(q.x(), q.y()));
            m_ws.append(w);
        }

        m_centerW = matrix.m44();
    } else
        m_transformedBounds = toVector<FloatPoint, 4>(m_drawTransform.mapQuad(boundsRect));

    m_boundingBox = WebCore::boundingBox(m_transformedBounds);
}

const Vector<FloatPoint>& LayerCompositingThread::textureCoordinates(TextureCoordinateOrientation orientation) const
{
    if (m_textureCoordinates.size()) {
        if (orientation == UpsideDown) {
            static Vector<FloatPoint> upsideDownCoordinates;
            upsideDownCoordinates = m_textureCoordinates;
            for (size_t i = 0; i < upsideDownCoordinates.size(); ++i)
                upsideDownCoordinates[i].setY(1 - upsideDownCoordinates[i].y());
            return upsideDownCoordinates;
        }

        return m_textureCoordinates;
    }

    if (orientation == UpsideDown) {
        static FloatPoint data[4] = { FloatPoint(0, 1),  FloatPoint(1, 1),  FloatPoint(1, 0),  FloatPoint(0, 0) };
        static Vector<FloatPoint>* upsideDownCoordinates = 0;
        if (!upsideDownCoordinates) {
            upsideDownCoordinates = new Vector<FloatPoint>();
            upsideDownCoordinates->append(data, 4);
        }
        return *upsideDownCoordinates;
    }

    static FloatPoint data[4] = { FloatPoint(0, 0),  FloatPoint(1, 0),  FloatPoint(1, 1),  FloatPoint(0, 1) };
    static Vector<FloatPoint>* coordinates = 0;
    if (!coordinates) {
        coordinates = new Vector<FloatPoint>();
        coordinates->append(data, 4);
    }
    return *coordinates;
}

FloatQuad LayerCompositingThread::transformedHolePunchRect() const
{
    FloatRect holePunchRect(m_holePunchRect);
    holePunchRect.moveBy(-origin());
    return m_drawTransform.mapQuad(holePunchRect);
}

void LayerCompositingThread::drawTextures(const GLES2Program& program, double scale, const FloatRect& visibleRect, const FloatRect& clipRect)
{
    if (m_pluginView) {
        if (m_isVisible) {
            // The layer contains Flash, video, or other plugin contents.
            m_pluginBuffer = m_pluginView->lockFrontBufferForRead();

            if (!m_pluginBuffer)
                return;

            if (!BlackBerry::Platform::Graphics::lockAndBindBufferGLTexture(m_pluginBuffer, GL_TEXTURE_2D)) {
                m_pluginView->unlockFrontBuffer();
                return;
            }

            m_layerRenderer->addLayerToReleaseTextureResourcesList(this);

            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glUniform1f(program.opacityLocation(), drawOpacity());
            glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, m_transformedBounds.data());
            glVertexAttribPointer(program.texCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates().data());
            glDrawArrays(GL_TRIANGLE_FAN, 0, m_transformedBounds.size());
        }
        return;
    }
#if ENABLE(VIDEO)
    if (m_mediaPlayer) {
        if (m_isVisible) {
            IntRect paintRect;
            if (m_layerRenderer->client()->shouldChildWindowsUseDocumentCoordinates()) {
                // We need to specify the media player location in contents coordinates. The 'visibleRect'
                // specifies the content region covered by our viewport. So we transform from our
                // normalized device coordinates [-1, 1] to the 'visibleRect'.
                float vrw2 = visibleRect.width() / 2.0;
                float vrh2 = visibleRect.height() / 2.0;
                FloatPoint p(m_transformedBounds[0].x() * vrw2 + vrw2 + visibleRect.x(),
                    -m_transformedBounds[0].y() * vrh2 + vrh2 + visibleRect.y());
                paintRect = IntRect(roundedIntPoint(p), m_bounds);
            } else
                paintRect = m_layerRenderer->toWindowCoordinates(m_boundingBox);

            m_mediaPlayer->paint(0, paintRect);
            MediaPlayerPrivate* mpp = static_cast<MediaPlayerPrivate*>(m_mediaPlayer->platformMedia().media.qnxMediaPlayer);
            mpp->drawBufferingAnimation(m_drawTransform, program);
        }
        return;
    }
#endif

    if (m_client)
        m_client->drawTextures(this, program, scale, clipRect);
}

void LayerCompositingThread::drawSurface(const GLES2Program& program, const TransformationMatrix& drawTransform, LayerCompositingThread* mask)
{
    using namespace BlackBerry::Platform::Graphics;

    if (m_layerRenderer->layerAlreadyOnSurface(this)) {
        LayerTexture* surfaceTexture = layerRendererSurface()->texture();
        if (!surfaceTexture) {
            ASSERT_NOT_REACHED();
            return;
        }
        textureCacheCompositingThread()->textureAccessed(layerRendererSurface()->texture());
        GLuint surfaceTexID = surfaceTexture->platformTexture();

        if (!surfaceTexID) {
            ASSERT_NOT_REACHED();
            return;
        }

        if (mask) {
            if (LayerTexture* maskTexture = mask->contentsTexture()) {
                GLuint maskTexID = maskTexture->platformTexture();

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, maskTexID);
                glActiveTexture(GL_TEXTURE0);
            }
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, surfaceTexID);

        FloatQuad surfaceQuad = drawTransform.mapQuad(FloatRect(-origin(), bounds()));
        glUniform1f(program.opacityLocation(), layerRendererSurface()->drawOpacity());
        glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, &surfaceQuad);

        static float texcoords[4 * 2] = { 0, 0,  1, 0,  1, 1,  0, 1 };
        glVertexAttribPointer(program.texCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, texcoords);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void LayerCompositingThread::releaseTextureResources()
{
    if (m_pluginView && m_pluginBuffer) {
        BlackBerry::Platform::Graphics::releaseBufferGLTexture(m_pluginBuffer);
        m_pluginBuffer = 0;
        m_pluginView->unlockFrontBuffer();
    }
}

void LayerCompositingThread::setPluginView(PluginView* pluginView)
{
    if (!isCompositingThread()) {
        dispatchSyncCompositingMessage(BlackBerry::Platform::createMethodCallMessage(
            &LayerCompositingThread::setPluginView,
            this,
            pluginView));
        return;
    }

    m_pluginView = pluginView;
}

#if ENABLE(VIDEO)
void LayerCompositingThread::setMediaPlayer(MediaPlayer* mediaPlayer)
{
    if (!isCompositingThread()) {
        dispatchSyncCompositingMessage(BlackBerry::Platform::createMethodCallMessage(
            &LayerCompositingThread::setMediaPlayer,
            this,
            mediaPlayer));
        return;
    }

    m_mediaPlayer = mediaPlayer;
}
#endif

void LayerCompositingThread::removeSublayer(LayerCompositingThread* sublayer)
{
    ASSERT(isCompositingThread());

    int foundIndex = indexOfSublayer(sublayer);
    if (foundIndex == -1)
        return;

    sublayer->setSuperlayer(0);
    m_sublayers.remove(foundIndex);
}

int LayerCompositingThread::indexOfSublayer(const LayerCompositingThread* reference)
{
    for (size_t i = 0; i < m_sublayers.size(); i++) {
        if (m_sublayers[i] == reference)
            return i;
    }
    return -1;
}

const LayerCompositingThread* LayerCompositingThread::rootLayer() const
{
    const LayerCompositingThread* layer = this;
    for (LayerCompositingThread* superlayer = layer->superlayer(); superlayer; layer = superlayer, superlayer = superlayer->superlayer()) { }
    return layer;
}

void LayerCompositingThread::addSublayer(LayerCompositingThread* layer)
{
    layer->removeFromSuperlayer();
    layer->setSuperlayer(this);
    m_sublayers.append(layer);
}

void LayerCompositingThread::removeFromSuperlayer()
{
    if (m_superlayer)
        m_superlayer->removeSublayer(this);
}

void LayerCompositingThread::setSublayers(const Vector<RefPtr<LayerCompositingThread> >& sublayers)
{
    if (sublayers == m_sublayers)
        return;

    while (m_sublayers.size()) {
        RefPtr<LayerCompositingThread> layer = m_sublayers[0].get();
        ASSERT(layer->superlayer());
        layer->removeFromSuperlayer();
    }

    m_sublayers.clear();

    size_t listSize = sublayers.size();
    for (size_t i = 0; i < listSize; i++) {
        RefPtr<LayerCompositingThread> sublayer = sublayers[i];
        sublayer->removeFromSuperlayer();
        sublayer->setSuperlayer(this);
        m_sublayers.insert(i, sublayer);
    }
}

void LayerCompositingThread::updateTextureContentsIfNeeded()
{
    if (m_client)
        m_client->uploadTexturesIfNeeded(this);
}

LayerTexture* LayerCompositingThread::contentsTexture()
{
    if (m_client)
        return m_client->contentsTexture(this);

    return 0;
}

void LayerCompositingThread::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

    m_visible = visible;

    if (m_client)
        m_client->layerVisibilityChanged(this, visible);
}

void LayerCompositingThread::setNeedsCommit()
{
    if (m_layerRenderer)
        m_layerRenderer->setNeedsCommit();
}

void LayerCompositingThread::scheduleCommit()
{
    if (!m_client)
        return;

    if (!isWebKitThread()) {
        if (m_commitScheduled)
            return;

        m_commitScheduled = true;

        dispatchWebKitMessage(BlackBerry::Platform::createMethodCallMessage(&LayerCompositingThread::scheduleCommit, this));
        return;
    }

    m_commitScheduled = false;

    m_client->scheduleCommit();
}

void LayerCompositingThread::commitPendingTextureUploads()
{
    if (m_client)
        m_client->commitPendingTextureUploads(this);
}

bool LayerCompositingThread::updateAnimations(double currentTime)
{
    // The commit mechanism always overwrites our state with state from the
    // WebKit thread. This means we have to restore the last animated value for
    // suspended animations.
    for (size_t i = 0; i < m_suspendedAnimations.size(); ++i) {
        LayerAnimation* animation = m_suspendedAnimations[i].get();
        // From looking at the WebCore code, it appears that when the animation
        // is paused, the timeOffset is modified so it will be an appropriate
        // elapsedTime.
        double elapsedTime = animation->timeOffset();
        animation->apply(this, elapsedTime);
    }

    bool allAnimationsFinished = true;
    for (size_t i = 0; i < m_runningAnimations.size(); ++i) {
        LayerAnimation* animation = m_runningAnimations[i].get();
        double elapsedTime = (m_suspendTime ? m_suspendTime : currentTime) - animation->startTime() + animation->timeOffset();
        animation->apply(this, elapsedTime);
        if (!animation->finished())
            allAnimationsFinished = false;
    }

    // If there are any overrides, apply them
    if (m_override) {
        if (m_override->isPositionSet())
            m_position = m_override->position();
        if (m_override->isAnchorPointSet())
            m_anchorPoint = m_override->anchorPoint();
        if (m_override->isBoundsSet())
            m_bounds = m_override->bounds();
        if (m_override->isTransformSet())
            m_transform = m_override->transform();
        if (m_override->isOpacitySet())
            m_opacity = m_override->opacity();

        for (size_t i = 0; i < m_override->animations().size(); ++i) {
            LayerAnimation* animation = m_override->animations()[i].get();
            double elapsedTime = (m_suspendTime ? m_suspendTime : currentTime) - animation->startTime() + animation->timeOffset();
            animation->apply(this, elapsedTime);
            if (!animation->finished())
                allAnimationsFinished = false;
        }
    }

    return !allAnimationsFinished;
}

bool LayerCompositingThread::hasVisibleHolePunchRect() const
{
    if (m_pluginView && !m_isVisible)
        return false;

#if ENABLE(VIDEO)
    if (m_mediaPlayer && !m_isVisible)
        return false;
#endif

    return hasHolePunchRect();
}

void LayerCompositingThread::createLayerRendererSurface()
{
    ASSERT(!m_layerRendererSurface);
    m_layerRendererSurface = adoptPtr(new LayerRendererSurface(m_layerRenderer, this));
}

void LayerCompositingThread::removeAnimation(const String& name)
{
    for (size_t i = 0; i < m_runningAnimations.size(); ++i) {
        if (m_runningAnimations[i]->name() == name) {
            m_runningAnimations.remove(i);
            return;
        }
    }
}

LayerOverride* LayerCompositingThread::override()
{
    if (!m_override)
        m_override = LayerOverride::create();
    return m_override.get();
}

void LayerCompositingThread::clearOverride()
{
    m_override.clear();
}

}

#endif // USE(ACCELERATED_COMPOSITING)
