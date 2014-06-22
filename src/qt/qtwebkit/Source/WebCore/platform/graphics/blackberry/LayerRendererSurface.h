/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef LayerRendererSurface_h
#define LayerRendererSurface_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatRect.h"
#include "IntRect.h"
#include "IntSize.h"
#include "LayerTexture.h"
#include "TransformationMatrix.h"

namespace WebCore {

class LayerCompositingThread;
class LayerRenderer;

class LayerRendererSurface {
    WTF_MAKE_NONCOPYABLE(LayerRendererSurface);
public:
    LayerRendererSurface(LayerRenderer*, LayerCompositingThread* owner);
    ~LayerRendererSurface();

    IntSize size() const { return m_size; }

    FloatRect contentRect() const { return m_contentRect; }
    void setContentRect(const IntRect&);
    FloatRect clipRect() const { return m_clipRect; }
    void setClipRect(const FloatRect& rect) { m_clipRect = rect; }

    FloatPoint origin() const { return FloatPoint(m_size.width() / 2.0f, m_size.height() / 2.0f); }

    void setDrawTransform(const TransformationMatrix& matrix, const TransformationMatrix& projectionMatrix) { m_drawTransform = projectionMatrix * matrix; }
    const TransformationMatrix& drawTransform() const { return m_drawTransform; }
    void setReplicaDrawTransform(const TransformationMatrix& matrix, const TransformationMatrix& projectionMatrix) { m_replicaDrawTransform = projectionMatrix * matrix; }
    const TransformationMatrix& replicaDrawTransform() const { return m_replicaDrawTransform; }

    // These use normalized device coordinates
    FloatRect boundingBox() const;
    Vector<FloatPoint, 4> transformedBounds() const;

    bool ensureTexture();
    void releaseTexture();
    LayerTexture* texture() const { return m_texture.get(); }

    float drawOpacity() { return m_opacity; }
    void setDrawOpacity(float opacity) { m_opacity = opacity; }

private:
    RefPtr<LayerTexture> m_texture;

    FloatRect m_contentRect;
    FloatRect m_clipRect;

    TransformationMatrix m_surfaceMatrix;
    TransformationMatrix m_drawTransform;
    TransformationMatrix m_replicaDrawTransform;

    LayerCompositingThread* m_ownerLayer;
    LayerRenderer* m_layerRenderer;

    float m_opacity;

    IntSize m_size;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerRendererSurface_h
