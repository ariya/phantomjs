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


#ifndef LayerData_h
#define LayerData_h

#include "Color.h"
#include "FilterOperations.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "IntRect.h"
#include "TransformationMatrix.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

#if USE(ACCELERATED_COMPOSITING)

namespace WebCore {

class HTMLCanvasElement;
class PluginView;
#if ENABLE(VIDEO)
class MediaPlayer;
#endif

class LayerData {
public:
    enum LayerType { Layer, TransformLayer, WebGLLayer, CanvasLayer, CustomLayer };
    enum FilterType { Linear, Nearest, Trilinear, Lanczos };
    enum LayerProgram {
        LayerProgramRGBA = 0,
        LayerProgramBGRA,
        NumberOfLayerPrograms
    };

#if ENABLE(CSS_FILTERS)
    enum CSSFilterShaders {
        CSSFilterShaderGrayscale = 0,
        CSSFilterShaderSepia,
        CSSFilterShaderSaturate,
        CSSFilterShaderHueRotate,
        CSSFilterShaderInvert,
        CSSFilterShaderBrightness,
        CSSFilterShaderContrast,
        CSSFilterShaderOpacity,
        CSSFilterShaderBlurY,
        CSSFilterShaderBlurX,
        CSSFilterShaderShadow,
        CSSFilterShaderPassthrough,
#if ENABLE(CSS_SHADERS)
        CSSFilterShaderCustom,
#endif
        NumberOfCSSFilterShaders
    };
#endif

    LayerData(LayerType type)
        : m_layerType(type)
        , m_anchorPoint(0.5, 0.5)
        , m_backgroundColor(0, 0, 0, 0)
        , m_borderColor(0, 0, 0, 0)
        , m_opacity(1.0)
        , m_anchorPointZ(0.0)
        , m_borderWidth(0.0)
        , m_layerProgram(LayerProgramBGRA)
        , m_pluginView(0)
#if ENABLE(VIDEO)
        , m_mediaPlayer(0)
#endif
        , m_suspendTime(0)
        , m_contentsScale(1.0)
        , m_doubleSided(true)
        , m_masksToBounds(false)
        , m_isOpaque(false)
        , m_preserves3D(false)
        , m_needsDisplayOnBoundsChange(false)
        , m_needsTexture(false)
        , m_isFixedPosition(false)
        , m_hasFixedContainer(false)
        , m_hasFixedAncestorInDOMTree(false)
        , m_isContainerForFixedPositionLayers(false)
        , m_sizeIsScaleInvariant(false)
        , m_contentsResolutionIndependent(false)
        , m_isVisible(true)
    {
    }

    virtual ~LayerData()
    {
    }

    FloatPoint anchorPoint() const { return m_anchorPoint; }

    float anchorPointZ() const { return m_anchorPointZ; }

    Color backgroundColor() const { return m_backgroundColor; }

    Color borderColor() const { return m_borderColor; }

    float borderWidth() const { return m_borderWidth; }

    IntSize bounds() const { return m_bounds; }

    bool sizeIsScaleInvariant() const { return m_sizeIsScaleInvariant; }

    bool contentsResolutionIndependent() const { return m_contentsResolutionIndependent; }

    bool doubleSided() const { return m_doubleSided; }

    FloatRect frame() const { return m_frame; }

    bool masksToBounds() const { return m_masksToBounds; }

    float opacity() const { return m_opacity; }

#if ENABLE(CSS_FILTERS)
    FilterOperations filters() const { return m_filters; }
#endif

    bool isOpaque() const { return m_isOpaque; }

    FloatPoint position() const { return m_position; }

    // This is currently only used for perspective transform, see GraphicsLayer::setChildrenTransform()
    const TransformationMatrix& sublayerTransform() const { return m_sublayerTransform; }

    const TransformationMatrix& transform() const { return m_transform; }

    bool preserves3D() const { return m_preserves3D; }

    bool needsTexture() const { return m_layerType == WebGLLayer || m_layerType == CanvasLayer || m_needsTexture; }

    LayerProgram layerProgram() const { return m_layerProgram; }

    bool isFixedPosition() const { return m_isFixedPosition; }
    bool hasFixedContainer() const { return m_hasFixedContainer; }
    bool hasFixedAncestorInDOMTree() const { return m_hasFixedAncestorInDOMTree; }
    bool isContainerForFixedPositionLayers() const { return m_isContainerForFixedPositionLayers; }
    bool isFixedToTop() const { return m_isFixedToTop; }
    bool isFixedToLeft() const { return m_isFixedToLeft; }

    IntRect frameVisibleRect() const { return m_frameVisibleRect; }
    IntSize frameContentsSize() const { return m_frameContentsSize; }

    PluginView* pluginView() const { return m_pluginView; }

    IntRect holePunchRect() const { return m_holePunchRect; }
    bool hasHolePunchRect() const { return !m_holePunchRect.isEmpty(); }

    double contentsScale() const { return m_contentsScale; }

#if ENABLE(VIDEO)
    MediaPlayer* mediaPlayer() const { return m_mediaPlayer; }
#endif

    void replicate(LayerData *to) const { *to = *this; }

    LayerType layerType() const { return m_layerType; }

    bool includeVisibility() const
    {
        if (pluginView())
            return true;

#if ENABLE(VIDEO)
        if (mediaPlayer())
            return true;
#endif

        return false;
    }

protected:
    LayerType m_layerType;

    IntSize m_bounds;
    FloatPoint m_position;
    FloatPoint m_anchorPoint;
    Color m_backgroundColor;
    Color m_borderColor;

    FloatRect m_frame;
    TransformationMatrix m_transform;
    TransformationMatrix m_sublayerTransform;

    float m_opacity;
#if ENABLE(CSS_FILTERS)
    FilterOperations m_filters;
#endif
    float m_anchorPointZ;
    float m_borderWidth;

    LayerProgram m_layerProgram;

    PluginView* m_pluginView;
#if ENABLE(VIDEO)
    MediaPlayer* m_mediaPlayer;
#endif
    IntRect m_holePunchRect;

    IntRect m_frameVisibleRect;
    IntSize m_frameContentsSize;

    double m_suspendTime;
    double m_contentsScale;

    unsigned m_doubleSided : 1;
    unsigned m_masksToBounds : 1;
    unsigned m_isOpaque : 1;
    unsigned m_preserves3D : 1;
    unsigned m_needsDisplayOnBoundsChange : 1;

    unsigned m_needsTexture : 1;
    unsigned m_isFixedPosition : 1;
    unsigned m_hasFixedContainer : 1;
    unsigned m_hasFixedAncestorInDOMTree : 1;
    unsigned m_isContainerForFixedPositionLayers: 1;
    unsigned m_isFixedToTop : 1;
    unsigned m_isFixedToLeft : 1;

    unsigned m_sizeIsScaleInvariant : 1;
    unsigned m_contentsResolutionIndependent : 1;

    // The following is only available for media (video) and plugin layers.
    unsigned m_isVisible : 1;

    // CAUTION: all the data members are copied from one instance to another
    // i.e. from one thread to another in the replicate method.
    // Beware of adding any member data e.g. of type String whose default
    // assignment operator doesn't behave properly if the two String instances
    // are owned by different threads.
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerData_h
