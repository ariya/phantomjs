/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
 */

#include "config.h"
#include "PainterOpenVG.h"

#include "AffineTransform.h"
#include "Color.h"
#include "DashArray.h"
#include "FloatPoint.h"
#include "FloatQuad.h"
#include "FloatRect.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "PlatformPathOpenVG.h"
#include "SurfaceOpenVG.h"
#include "TiledImageOpenVG.h"
#include "VGUtils.h"

#if PLATFORM(EGL)
#include "EGLUtils.h"
#endif

#include <vgu.h>

#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>

namespace WebCore {

static bool isNonRotatedAffineTransformation(const AffineTransform& t)
{
    return t.b() <= FLT_EPSILON && t.c() <= FLT_EPSILON;
}

static VGCapStyle toVGCapStyle(LineCap lineCap)
{
    switch (lineCap) {
    case RoundCap:
        return VG_CAP_ROUND;
    case SquareCap:
        return VG_CAP_SQUARE;
    case ButtCap:
    default:
        return VG_CAP_BUTT;
    }
}

static VGJoinStyle toVGJoinStyle(LineJoin lineJoin)
{
    switch (lineJoin) {
    case RoundJoin:
        return VG_JOIN_ROUND;
    case BevelJoin:
        return VG_JOIN_BEVEL;
    case MiterJoin:
    default:
        return VG_JOIN_MITER;
    }
}

static VGFillRule toVGFillRule(WindRule fillRule)
{
    return fillRule == RULE_EVENODD ? VG_EVEN_ODD : VG_NON_ZERO;
}

static VGuint colorToVGColor(const Color& color)
{
    VGuint vgColor = color.red();
    vgColor = (vgColor << 8) | color.green();
    vgColor = (vgColor << 8) | color.blue();
    vgColor = (vgColor << 8) | color.alpha();
    return vgColor;
}

static void setVGSolidColor(VGPaintMode paintMode, const Color& color)
{
    VGPaint paint = vgCreatePaint();
    vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetColor(paint, colorToVGColor(color));
    vgSetPaint(paint, paintMode);
    vgDestroyPaint(paint);
    ASSERT_VG_NO_ERROR();
}


struct PlatformPainterState {
    AffineTransform surfaceTransformation;
    CompositeOperator compositeOperation;
    float opacity;

    bool scissoringEnabled;
    FloatRect scissorRect;
#ifdef OPENVG_VERSION_1_1
    bool maskingChangedAndEnabled;
    VGMaskLayer mask;
#endif

    Color fillColor;
    StrokeStyle strokeStyle;
    Color strokeColor;
    float strokeThickness;
    LineCap strokeLineCap;
    LineJoin strokeLineJoin;
    float strokeMiterLimit;
    DashArray strokeDashArray;
    float strokeDashOffset;

    TextDrawingModeFlags textDrawingMode;
    bool antialiasingEnabled;

    PlatformPainterState()
        : compositeOperation(CompositeSourceOver)
        , opacity(1.0)
        , scissoringEnabled(false)
#ifdef OPENVG_VERSION_1_1
        , maskingChangedAndEnabled(false)
        , mask(VG_INVALID_HANDLE)
#endif
        , fillColor(Color::black)
        , strokeStyle(NoStroke)
        , strokeThickness(0.0)
        , strokeLineCap(ButtCap)
        , strokeLineJoin(MiterJoin)
        , strokeMiterLimit(4.0)
        , strokeDashOffset(0.0)
        , textDrawingMode(TextModeFill)
        , antialiasingEnabled(true)
    {
    }

    ~PlatformPainterState()
    {
#ifdef OPENVG_VERSION_1_1
        if (maskingChangedAndEnabled && mask != VG_INVALID_HANDLE) {
            vgDestroyMaskLayer(mask);
            ASSERT_VG_NO_ERROR();
            mask = VG_INVALID_HANDLE;
        }
#endif
    }

    PlatformPainterState(const PlatformPainterState& state)
    {
        surfaceTransformation = state.surfaceTransformation;

        scissoringEnabled = state.scissoringEnabled;
        scissorRect = state.scissorRect;
#ifdef OPENVG_VERSION_1_1
        maskingChangedAndEnabled = false;
        mask = state.mask;
#endif
        copyPaintState(&state);
    }

    inline bool maskingEnabled()
    {
        return maskingChangedAndEnabled || mask != VG_INVALID_HANDLE;
    }

    void copyPaintState(const PlatformPainterState* other)
    {
        compositeOperation = other->compositeOperation;
        opacity = other->opacity;

        fillColor = other->fillColor;
        strokeStyle = other->strokeStyle;
        strokeColor = other->strokeColor;
        strokeThickness = other->strokeThickness;
        strokeLineCap = other->strokeLineCap;
        strokeLineJoin = other->strokeLineJoin;
        strokeMiterLimit = other->strokeMiterLimit;
        strokeDashArray = other->strokeDashArray;
        strokeDashOffset = other->strokeDashOffset;

        textDrawingMode = other->textDrawingMode;
        antialiasingEnabled = other->antialiasingEnabled;
    }

    void applyState(PainterOpenVG* painter)
    {
        ASSERT(painter);

        setVGSolidColor(VG_FILL_PATH, fillColor);
        setVGSolidColor(VG_STROKE_PATH, strokeColor);

        vgSetf(VG_STROKE_LINE_WIDTH, strokeThickness);
        vgSeti(VG_STROKE_CAP_STYLE, toVGCapStyle(strokeLineCap));
        vgSeti(VG_STROKE_JOIN_STYLE, toVGJoinStyle(strokeLineJoin));
        vgSetf(VG_STROKE_MITER_LIMIT, strokeMiterLimit);

        if (antialiasingEnabled)
            vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_FASTER);
        else
            vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);

        applyBlending(painter);
        applyStrokeStyle();

        applyTransformation(painter);
        applyScissorRect();

#ifdef OPENVG_VERSION_1_1
        if (maskingEnabled()) {
            vgSeti(VG_MASKING, VG_TRUE);
            if (mask != VG_INVALID_HANDLE)
                vgMask(mask, VG_SET_MASK, 0, 0, painter->surface()->width(), painter->surface()->height());
        } else
            vgSeti(VG_MASKING, VG_FALSE);
#endif
        ASSERT_VG_NO_ERROR();
    }

    void applyBlending(PainterOpenVG* painter)
    {
        VGBlendMode blendMode = VG_BLEND_SRC_OVER;

        switch (compositeOperation) {
        case CompositeClear: {
            // Clear means "set to fully transparent regardless of SRC".
            // We implement that by multiplying DST with white color
            // (= no changes) and an alpha of 1.0 - opacity, so the destination
            // pixels will be fully transparent when opacity == 1.0 and
            // unchanged when opacity == 0.0.
            blendMode = VG_BLEND_DST_IN;
            const VGfloat values[] = { 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 - opacity };
            vgSetfv(VG_COLOR_TRANSFORM_VALUES, 8, values);
            vgSeti(VG_COLOR_TRANSFORM, VG_TRUE);
            ASSERT_VG_NO_ERROR();
            break;
        }
        case CompositeCopy:
            blendMode = VG_BLEND_SRC;
            break;
        case CompositeSourceOver:
            blendMode = VG_BLEND_SRC_OVER;
            break;
        case CompositeSourceIn:
            blendMode = VG_BLEND_SRC_IN;
            break;
        case CompositeSourceOut:
            notImplemented();
            break;
        case CompositeSourceAtop:
            notImplemented();
            break;
        case CompositeDestinationOver:
            blendMode = VG_BLEND_DST_OVER;
            break;
        case CompositeDestinationIn:
            blendMode = VG_BLEND_DST_IN;
            break;
        case CompositeDestinationOut:
            notImplemented();
            break;
        case CompositeDestinationAtop:
            notImplemented();
            break;
        case CompositeXOR:
            notImplemented();
            break;
        case CompositePlusDarker:
            blendMode = VG_BLEND_DARKEN;
            break;
        case CompositeHighlight:
            notImplemented();
            break;
        case CompositePlusLighter:
            blendMode = VG_BLEND_LIGHTEN;
            break;
        }

        if (compositeOperation != CompositeClear) {
            if (opacity >= (1.0 - FLT_EPSILON))
                vgSeti(VG_COLOR_TRANSFORM, VG_FALSE);
            else if (blendMode == VG_BLEND_SRC) {
                blendMode = VG_BLEND_SRC_OVER;
                VGfloat values[] = { 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, opacity };
                vgSetfv(VG_COLOR_TRANSFORM_VALUES, 8, values);
                vgSeti(VG_COLOR_TRANSFORM, VG_TRUE);
            } else {
                VGfloat values[] = { 1.0, 1.0, 1.0, opacity, 0.0, 0.0, 0.0, 0.0 };
                vgSetfv(VG_COLOR_TRANSFORM_VALUES, 8, values);
                vgSeti(VG_COLOR_TRANSFORM, VG_TRUE);
            }
            ASSERT_VG_NO_ERROR();
        }

        vgSeti(VG_BLEND_MODE, blendMode);
        ASSERT_VG_NO_ERROR();
    }

    void applyTransformation(PainterOpenVG* painter)
    {
        // There are *five* separate transforms that can be applied to OpenVG as of 1.1
        // but it is not clear that we need to set them separately.  Instead we set them
        // all right here and let this be a call to essentially set the world transformation!
        VGMatrix vgMatrix(surfaceTransformation);
        const VGfloat* vgFloatArray = vgMatrix.toVGfloat();

        vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
        vgLoadMatrix(vgFloatArray);
        ASSERT_VG_NO_ERROR();

        vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
        vgLoadMatrix(vgFloatArray);
        ASSERT_VG_NO_ERROR();

#ifdef OPENVG_VERSION_1_1
        vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
        vgLoadMatrix(vgFloatArray);
        ASSERT_VG_NO_ERROR();
#endif
    }

    void applyScissorRect()
    {
        if (scissoringEnabled) {
            vgSeti(VG_SCISSORING, VG_TRUE);
            vgSetfv(VG_SCISSOR_RECTS, 4, VGRect(scissorRect).toVGfloat());
        } else
            vgSeti(VG_SCISSORING, VG_FALSE);

        ASSERT_VG_NO_ERROR();
    }

    void applyStrokeStyle()
    {
        if (strokeStyle == DottedStroke) {
            VGfloat vgFloatArray[2] = { 1.0, 1.0 };
            vgSetfv(VG_STROKE_DASH_PATTERN, 2, vgFloatArray);
            vgSetf(VG_STROKE_DASH_PHASE, 0.0);
        } else if (strokeStyle == DashedStroke) {
            if (!strokeDashArray.size()) {
                VGfloat vgFloatArray[2] = { 4.0, 3.0 };
                vgSetfv(VG_STROKE_DASH_PATTERN, 2, vgFloatArray);
            } else {
                Vector<VGfloat> vgFloatArray(strokeDashArray.size());
                for (int i = 0; i < strokeDashArray.size(); ++i)
                    vgFloatArray[i] = strokeDashArray[i];

                vgSetfv(VG_STROKE_DASH_PATTERN, vgFloatArray.size(), vgFloatArray.data());
            }
            vgSetf(VG_STROKE_DASH_PHASE, strokeDashOffset);
        } else {
            vgSetfv(VG_STROKE_DASH_PATTERN, 0, 0);
            vgSetf(VG_STROKE_DASH_PHASE, 0.0);
        }

        ASSERT_VG_NO_ERROR();
    }

    inline bool strokeDisabled() const
    {
        return (compositeOperation == CompositeSourceOver
            && (strokeStyle == NoStroke || !strokeColor.alpha()));
    }

    inline bool fillDisabled() const
    {
        return (compositeOperation == CompositeSourceOver && !fillColor.alpha());
    }

    void saveMaskIfNecessary(PainterOpenVG* painter)
    {
#ifdef OPENVG_VERSION_1_1
        if (maskingChangedAndEnabled) {
            if (mask != VG_INVALID_HANDLE) {
                vgDestroyMaskLayer(mask);
                ASSERT_VG_NO_ERROR();
            }
            mask = vgCreateMaskLayer(painter->surface()->width(), painter->surface()->height());
            ASSERT(mask != VG_INVALID_HANDLE);
            vgCopyMask(mask, 0, 0, 0, 0, painter->surface()->width(), painter->surface()->height());
            ASSERT_VG_NO_ERROR();
        }
#endif
    }
};


PainterOpenVG::PainterOpenVG()
    : m_state(0)
    , m_surface(0)
{
}

PainterOpenVG::PainterOpenVG(SurfaceOpenVG* surface)
    : m_state(0)
    , m_surface(0)
{
    ASSERT(surface);
    begin(surface);
}

PainterOpenVG::~PainterOpenVG()
{
    end();
}

void PainterOpenVG::begin(SurfaceOpenVG* surface)
{
    if (surface == m_surface)
        return;

    ASSERT(surface);
    ASSERT(!m_state);

    m_surface = surface;

    m_stateStack.append(new PlatformPainterState());
    m_state = m_stateStack.last();

    m_surface->setActivePainter(this);
    m_surface->makeCurrent();
}

void PainterOpenVG::end()
{
    if (!m_surface)
        return;

    m_surface->setActivePainter(0);
    m_surface = 0;

    destroyPainterStates();
}

void PainterOpenVG::destroyPainterStates()
{
    PlatformPainterState* state = 0;
    while (!m_stateStack.isEmpty()) {
        state = m_stateStack.last();
        m_stateStack.removeLast();
        delete state;
    }
    m_state = 0;
}

// Called by friend SurfaceOpenVG, private otherwise.
void PainterOpenVG::applyState()
{
    ASSERT(m_state);
    m_state->applyState(this);
}

/**
 * Copy the current back buffer image onto the surface.
 *
 * Call this method when all painting operations have been completed,
 * otherwise the surface won't visibly change.
 */
void PainterOpenVG::blitToSurface()
{
    ASSERT(m_state); // implies m_surface
    m_surface->flush();
}

AffineTransform PainterOpenVG::transformation() const
{
    ASSERT(m_state);
    return m_state->surfaceTransformation;
}

void PainterOpenVG::concatTransformation(const AffineTransform& transformation)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    // We do the multiplication ourself using WebCore's AffineTransform rather
    // than offloading this to VG via vgMultMatrix() to keep things simple and
    // so we can maintain state ourselves.
    m_state->surfaceTransformation.multLeft(transformation);
    m_state->applyTransformation(this);
}

void PainterOpenVG::setTransformation(const AffineTransform& transformation)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->surfaceTransformation = transformation;
    m_state->applyTransformation(this);
}

void PainterOpenVG::transformPath(VGPath dst, VGPath src, const AffineTransform& transformation)
{
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);

    // Save the transform state
    VGfloat currentMatrix[9];
    vgGetMatrix(currentMatrix);
    ASSERT_VG_NO_ERROR();

    // Load the new transform
    vgLoadMatrix(VGMatrix(transformation).toVGfloat());
    ASSERT_VG_NO_ERROR();

    // Apply the new transform
    vgTransformPath(dst, src);
    ASSERT_VG_NO_ERROR();

    // Restore the transform state
    vgLoadMatrix(currentMatrix);
    ASSERT_VG_NO_ERROR();
}

CompositeOperator PainterOpenVG::compositeOperation() const
{
    ASSERT(m_state);
    return m_state->compositeOperation;
}

void PainterOpenVG::setCompositeOperation(CompositeOperator op)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->compositeOperation = op;
    m_state->applyBlending(this);
}

float PainterOpenVG::opacity() const
{
    ASSERT(m_state);
    return m_state->opacity;
}

void PainterOpenVG::setOpacity(float opacity)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->opacity = opacity;
    m_state->applyBlending(this);
}

float PainterOpenVG::strokeThickness() const
{
    ASSERT(m_state);
    return m_state->strokeThickness;
}

void PainterOpenVG::setStrokeThickness(float thickness)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeThickness = thickness;
    vgSetf(VG_STROKE_LINE_WIDTH, thickness);
    ASSERT_VG_NO_ERROR();
}

StrokeStyle PainterOpenVG::strokeStyle() const
{
    ASSERT(m_state);
    return m_state->strokeStyle;
}

void PainterOpenVG::setStrokeStyle(StrokeStyle style)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeStyle = style;
    m_state->applyStrokeStyle();
}

void PainterOpenVG::setLineDash(const DashArray& dashArray, float dashOffset)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeDashArray = dashArray;
    m_state->strokeDashOffset = dashOffset;
    m_state->applyStrokeStyle();
}

void PainterOpenVG::setLineCap(LineCap lineCap)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeLineCap = lineCap;
    vgSeti(VG_STROKE_CAP_STYLE, toVGCapStyle(lineCap));
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::setLineJoin(LineJoin lineJoin)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeLineJoin = lineJoin;
    vgSeti(VG_STROKE_JOIN_STYLE, toVGJoinStyle(lineJoin));
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::setMiterLimit(float miterLimit)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeMiterLimit = miterLimit;
    vgSetf(VG_STROKE_MITER_LIMIT, miterLimit);
    ASSERT_VG_NO_ERROR();
}

Color PainterOpenVG::strokeColor() const
{
    ASSERT(m_state);
    return m_state->strokeColor;
}

void PainterOpenVG::setStrokeColor(const Color& color)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->strokeColor = color;
    setVGSolidColor(VG_STROKE_PATH, color);
}

Color PainterOpenVG::fillColor() const
{
    ASSERT(m_state);
    return m_state->fillColor;
}

void PainterOpenVG::setFillColor(const Color& color)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->fillColor = color;
    setVGSolidColor(VG_FILL_PATH, color);
}

TextDrawingModeFlags PainterOpenVG::textDrawingMode() const
{
    ASSERT(m_state);
    return m_state->textDrawingMode;
}

void PainterOpenVG::setTextDrawingMode(TextDrawingModeFlags mode)
{
    ASSERT(m_state);
    m_state->textDrawingMode = mode;
}

bool PainterOpenVG::antialiasingEnabled() const
{
    ASSERT(m_state);
    return m_state->antialiasingEnabled;
}

void PainterOpenVG::setAntialiasingEnabled(bool enabled)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    m_state->antialiasingEnabled = enabled;

    if (enabled)
        vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_FASTER);
    else
        vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);
}

void PainterOpenVG::scale(const FloatSize& scaleFactors)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    AffineTransform transformation = m_state->surfaceTransformation;
    transformation.scaleNonUniform(scaleFactors.width(), scaleFactors.height());
    setTransformation(transformation);
}

void PainterOpenVG::rotate(float radians)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    AffineTransform transformation = m_state->surfaceTransformation;
    transformation.rotate(rad2deg(radians));
    setTransformation(transformation);
}

void PainterOpenVG::translate(float dx, float dy)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    AffineTransform transformation = m_state->surfaceTransformation;
    transformation.translate(dx, dy);
    setTransformation(transformation);
}

void PainterOpenVG::drawPath(const Path& path, VGbitfield specifiedPaintModes, WindRule fillRule)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    vgSeti(VG_FILL_RULE, toVGFillRule(fillRule));
    vgDrawPath(path.platformPath()->vgPath(), paintModes);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::intersectScissorRect(const FloatRect& rect)
{
    // Scissor rectangles are defined by float values, but e.g. painting
    // something red to a float-clipped rectangle and then painting something
    // white to the same rectangle will leave some red remnants as it is
    // rendered to full pixels in between. Also, some OpenVG implementations
    // are likely to clip to integer coordinates anyways because of the above
    // effect. So considering the above (and confirming through tests) the
    // visual result is better if we clip to the enclosing integer rectangle
    // rather than the exact float rectangle for scissoring.
    if (m_state->scissoringEnabled)
        m_state->scissorRect.intersect(FloatRect(enclosingIntRect(rect)));
    else {
        m_state->scissoringEnabled = true;
        m_state->scissorRect = FloatRect(enclosingIntRect(rect));
    }

    m_state->applyScissorRect();
}

void PainterOpenVG::intersectClipRect(const FloatRect& rect)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    if (m_state->surfaceTransformation.isIdentity()) {
        // No transformation required, skip all the complex stuff.
        intersectScissorRect(rect);
        return;
    }

    // Check if the actual destination rectangle is still rectilinear (can be
    // represented as FloatRect) so we could apply scissoring instead of
    // (potentially more expensive) path clipping. Note that scissoring is not
    // subject to transformations, so we need to do the transformation to
    // surface coordinates by ourselves.
    FloatQuad effectiveScissorQuad = m_state->surfaceTransformation.mapQuad(FloatQuad(rect));

    if (effectiveScissorQuad.isRectilinear())
        intersectScissorRect(effectiveScissorQuad.boundingBox());
    else {
        // The transformed scissorRect cannot be represented as FloatRect
        // anymore, so we need to perform masking instead.
        Path scissorRectPath;
        scissorRectPath.addRect(rect);
        clipPath(scissorRectPath, PainterOpenVG::IntersectClip);
    }
}

void PainterOpenVG::clipPath(const Path& path, PainterOpenVG::ClipOperation maskOp, WindRule clipRule)
{
#ifdef OPENVG_VERSION_1_1
    ASSERT(m_state);
    m_surface->makeCurrent();

    if (m_state->mask != VG_INVALID_HANDLE && !m_state->maskingChangedAndEnabled) {
        // The parent's mask has been inherited - dispose the handle so that
        // it won't be overwritten.
        m_state->maskingChangedAndEnabled = true;
        m_state->mask = VG_INVALID_HANDLE;
    } else if (!m_state->maskingEnabled()) {
        // None of the parent painter states had a mask enabled yet.
        m_state->maskingChangedAndEnabled = true;
        vgSeti(VG_MASKING, VG_TRUE);
        // Make sure not to inherit previous mask state from previously written
        // (but disabled) masks. For VG_FILL_MASK the first argument is ignored,
        // we pass VG_INVALID_HANDLE which is what the OpenVG spec suggests.
        vgMask(VG_INVALID_HANDLE, VG_FILL_MASK, 0, 0, m_surface->width(), m_surface->height());
    }

    // Intersect the path from the mask, or subtract it from there.
    // (In either case we always decrease the visible area, never increase it,
    // which means masking never has to modify scissor rectangles.)
    vgSeti(VG_FILL_RULE, toVGFillRule(clipRule));
    vgRenderToMask(path.platformPath()->vgPath(), VG_FILL_PATH, (VGMaskOperation) maskOp);
    ASSERT_VG_NO_ERROR();
#else
    notImplemented();
#endif
}

void PainterOpenVG::drawRect(const FloatRect& rect, VGbitfield specifiedPaintModes)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        5 /* expected number of segments */,
        5 /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    if (vguRect(path, rect.x(), rect.y(), rect.width(), rect.height()) == VGU_NO_ERROR) {
        vgDrawPath(path, paintModes);
        ASSERT_VG_NO_ERROR();
    }

    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawRoundedRect(const FloatRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, VGbitfield specifiedPaintModes)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        10 /* expected number of segments */,
        25 /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    // clamp corner arc sizes
    FloatSize clampedTopLeft = FloatSize(topLeft).shrunkTo(rect.size()).expandedTo(FloatSize());
    FloatSize clampedTopRight = FloatSize(topRight).shrunkTo(rect.size()).expandedTo(FloatSize());
    FloatSize clampedBottomLeft = FloatSize(bottomLeft).shrunkTo(rect.size()).expandedTo(FloatSize());
    FloatSize clampedBottomRight = FloatSize(bottomRight).shrunkTo(rect.size()).expandedTo(FloatSize());

    // As OpenVG's coordinate system is flipped in comparison to WebKit's,
    // we have to specify the opposite value for the "clockwise" value.
    static const VGubyte pathSegments[] = {
        VG_MOVE_TO_ABS,
        VG_HLINE_TO_REL,
        VG_SCCWARC_TO_REL,
        VG_VLINE_TO_REL,
        VG_SCCWARC_TO_REL,
        VG_HLINE_TO_REL,
        VG_SCCWARC_TO_REL,
        VG_VLINE_TO_REL,
        VG_SCCWARC_TO_REL,
        VG_CLOSE_PATH
    };
    // Also, the rounded rectangle path proceeds from the top to the bottom,
    // requiring height distances and clamped radius sizes to be flipped.
    const VGfloat pathData[] = {
        rect.x() + clampedTopLeft.width(), rect.y(),
        rect.width() - clampedTopLeft.width() - clampedTopRight.width(),
        clampedTopRight.width(), clampedTopRight.height(), 0, clampedTopRight.width(), clampedTopRight.height(),
        rect.height() - clampedTopRight.height() - clampedBottomRight.height(),
        clampedBottomRight.width(), clampedBottomRight.height(), 0, -clampedBottomRight.width(), clampedBottomRight.height(),
        -(rect.width() - clampedBottomLeft.width() - clampedBottomRight.width()),
        clampedBottomLeft.width(), clampedBottomLeft.height(), 0, -clampedBottomLeft.width(), -clampedBottomLeft.height(),
        -(rect.height() - clampedTopLeft.height() - clampedBottomLeft.height()),
        clampedTopLeft.width(), clampedTopLeft.height(), 0, clampedTopLeft.width(), -clampedTopLeft.height(),
    };

    vgAppendPathData(path, 10, pathSegments, pathData);
    vgDrawPath(path, paintModes);
    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawLine(const IntPoint& from, const IntPoint& to)
{
    ASSERT(m_state);

    if (m_state->strokeDisabled())
        return;

    m_surface->makeCurrent();

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        2 /* expected number of segments */,
        4 /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    VGUErrorCode errorCode;

    // Try to align lines to pixels, centering them between pixels for odd thickness values.
    if (fmod(m_state->strokeThickness + 0.5, 2.0) < 1.0)
        errorCode = vguLine(path, from.x(), from.y(), to.x(), to.y());
    else if ((to.y() - from.y()) > (to.x() - from.x())) // more vertical than horizontal
        errorCode = vguLine(path, from.x() + 0.5, from.y(), to.x() + 0.5, to.y());
    else
        errorCode = vguLine(path, from.x(), from.y() + 0.5, to.x(), to.y() + 0.5);

    if (errorCode == VGU_NO_ERROR) {
        vgDrawPath(path, VG_STROKE_PATH);
        ASSERT_VG_NO_ERROR();
    }

    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawArc(const IntRect& rect, int startAngle, int angleSpan, VGbitfield specifiedPaintModes)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        2 /* expected number of segments */,
        4 /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    if (vguArc(path, rect.x() + rect.width() / 2.0, rect.y() + rect.height() / 2.0, rect.width(), rect.height(), -startAngle, -angleSpan, VGU_ARC_OPEN) == VGU_NO_ERROR) {
        vgDrawPath(path, VG_STROKE_PATH);
        ASSERT_VG_NO_ERROR();
    }

    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawEllipse(const IntRect& rect, VGbitfield specifiedPaintModes)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        4 /* expected number of segments */,
        12 /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    if (vguEllipse(path, rect.x() + rect.width() / 2.0, rect.y() + rect.height() / 2.0, rect.width(), rect.height()) == VGU_NO_ERROR) {
        vgDrawPath(path, paintModes);
        ASSERT_VG_NO_ERROR();
    }

    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawPolygon(size_t numPoints, const FloatPoint* points, VGbitfield specifiedPaintModes)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;
    if (!m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;
    if (!m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;

    paintModes &= specifiedPaintModes;

    if (!paintModes)
        return;

    m_surface->makeCurrent();

    // Path segments: all points + "close path".
    const VGint numSegments = numPoints + 1;
    const VGint numCoordinates = numPoints * 2;

    VGPath path = vgCreatePath(
        VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
        1.0 /* scale */, 0.0 /* bias */,
        numSegments /* expected number of segments */,
        numCoordinates /* expected number of total coordinates */,
        VG_PATH_CAPABILITY_APPEND_TO);
    ASSERT_VG_NO_ERROR();

    Vector<VGfloat> vgPoints(numCoordinates);
    for (int i = 0; i < numPoints; ++i) {
        vgPoints[i*2]     = points[i].x();
        vgPoints[i*2 + 1] = points[i].y();
    }

    if (vguPolygon(path, vgPoints.data(), numPoints, VG_TRUE /* closed */) == VGU_NO_ERROR) {
        vgDrawPath(path, paintModes);
        ASSERT_VG_NO_ERROR();
    }

    vgDestroyPath(path);
    ASSERT_VG_NO_ERROR();
}

void PainterOpenVG::drawImage(TiledImageOpenVG* tiledImage, const FloatRect& dst, const FloatRect& src)
{
    ASSERT(m_state);
    m_surface->makeCurrent();

    // If buffers can be larger than the maximum OpenVG image sizes,
    // we split them into tiles.
    IntRect drawnTiles = tiledImage->tilesInRect(src);
    AffineTransform srcToDstTransformation = makeMapBetweenRects(
        FloatRect(FloatPoint(0.0, 0.0), src.size()), dst);
    srcToDstTransformation.translate(-src.x(), -src.y());

    for (int yIndex = drawnTiles.y(); yIndex < drawnTiles.bottom(); ++yIndex) {
        for (int xIndex = drawnTiles.x(); xIndex < drawnTiles.right(); ++xIndex) {
            // The srcTile rectangle is an aligned tile cropped by the src rectangle.
            FloatRect tile(tiledImage->tileRect(xIndex, yIndex));
            FloatRect srcTile = intersection(src, tile);

            save();

            // If the image is drawn in full, all we need is the proper transformation
            // in order to get it drawn at the right spot on the surface.
            concatTransformation(AffineTransform(srcToDstTransformation).translate(tile.x(), tile.y()));

            // If only a part of the tile is drawn, we also need to clip the surface.
            if (srcTile != tile) {
                // Put boundaries relative to tile origin, as we already
                // translated to (x, y) with the transformation matrix.
                srcTile.move(-tile.x(), -tile.y());
                intersectClipRect(srcTile);
            }

            VGImage image = tiledImage->tile(xIndex, yIndex);
            if (image != VG_INVALID_HANDLE) {
                vgDrawImage(image);
                ASSERT_VG_NO_ERROR();
            }

            restore();
        }
    }
}

#ifdef OPENVG_VERSION_1_1
void PainterOpenVG::drawText(VGFont vgFont, Vector<VGuint>& characters, VGfloat* adjustmentsX, VGfloat* adjustmentsY, const FloatPoint& point)
{
    ASSERT(m_state);

    VGbitfield paintModes = 0;

    if (m_state->textDrawingMode & TextModeClip)
        return; // unsupported for every port except CG at the time of writing
    if (m_state->textDrawingMode & TextModeFill && !m_state->fillDisabled())
        paintModes |= VG_FILL_PATH;
    if (m_state->textDrawingMode & TextModeStroke && !m_state->strokeDisabled())
        paintModes |= VG_STROKE_PATH;

    m_surface->makeCurrent();

    FloatPoint effectivePoint = m_state->surfaceTransformation.mapPoint(point);
    FloatPoint p = point;
    AffineTransform* originalTransformation = 0;

    // In case the font isn't drawn at a pixel-exact baseline and we can easily
    // fix that (which is the case for non-rotated affine transforms), let's
    // align the starting point to the pixel boundary in order to prevent
    // font rendering issues such as glyphs that appear off by a pixel.
    // This causes us to have inconsistent spacing between baselines in a
    // larger paragraph, but that seems to be the least of all evils.
    if ((fmod(effectivePoint.x() + 0.01, 1.0) > 0.02 || fmod(effectivePoint.y() + 0.01, 1.0) > 0.02)
        && isNonRotatedAffineTransformation(m_state->surfaceTransformation))
    {
        originalTransformation = new AffineTransform(m_state->surfaceTransformation);
        setTransformation(AffineTransform(
            m_state->surfaceTransformation.a(), 0,
            0, m_state->surfaceTransformation.d(),
            roundf(effectivePoint.x()), roundf(effectivePoint.y())));
        p = FloatPoint();
    }

    const VGfloat vgPoint[2] = { p.x(), p.y() };
    vgSetfv(VG_GLYPH_ORIGIN, 2, vgPoint);
    ASSERT_VG_NO_ERROR();

    vgDrawGlyphs(vgFont, characters.size(), characters.data(),
        adjustmentsX, adjustmentsY, paintModes, VG_TRUE /* allow autohinting */);
    ASSERT_VG_NO_ERROR();

    if (originalTransformation) {
        setTransformation(*originalTransformation);
        delete originalTransformation;
    }
}
#endif

TiledImageOpenVG* PainterOpenVG::asNewNativeImage(const IntRect& src, VGImageFormat format)
{
    ASSERT(m_state);
    m_surface->sharedSurface()->makeCurrent();

    const IntSize vgMaxImageSize(vgGeti(VG_MAX_IMAGE_WIDTH), vgGeti(VG_MAX_IMAGE_HEIGHT));
    ASSERT_VG_NO_ERROR();

    const IntRect rect = intersection(src, IntRect(0, 0, m_surface->width(), m_surface->height()));
    TiledImageOpenVG* tiledImage = new TiledImageOpenVG(rect.size(), vgMaxImageSize);

    const int numColumns = tiledImage->numColumns();
    const int numRows = tiledImage->numRows();

    // Create the images as resources of the shared surface/context.
    for (int yIndex = 0; yIndex < numRows; ++yIndex) {
        for (int xIndex = 0; xIndex < numColumns; ++xIndex) {
            IntRect tileRect = tiledImage->tileRect(xIndex, yIndex);
            VGImage image = vgCreateImage(format, tileRect.width(), tileRect.height(), VG_IMAGE_QUALITY_FASTER);
            ASSERT_VG_NO_ERROR();

            tiledImage->setTile(xIndex, yIndex, image);
        }
    }

    // Fill the image contents with our own surface/context being current.
    m_surface->makeCurrent();

    for (int yIndex = 0; yIndex < numRows; ++yIndex) {
        for (int xIndex = 0; xIndex < numColumns; ++xIndex) {
            IntRect tileRect = tiledImage->tileRect(xIndex, yIndex);

            vgGetPixels(tiledImage->tile(xIndex, yIndex), 0, 0,
                rect.x() + tileRect.x(), rect.y() + tileRect.y(),
                tileRect.width(), tileRect.height());
            ASSERT_VG_NO_ERROR();
        }
    }

    return tiledImage;
}

void PainterOpenVG::save(PainterOpenVG::SaveMode saveMode)
{
    ASSERT(m_state);

    // If the underlying context/surface was switched away by someone without
    // telling us, it might not correspond to the one assigned to this painter.
    // Switch back so we can save the state properly. (Should happen rarely.)
    // Use DontSaveOrApplyPainterState mode in order to avoid recursion.
    m_surface->makeCurrent(SurfaceOpenVG::DontSaveOrApplyPainterState);

    if (saveMode == PainterOpenVG::CreateNewState) {
        m_state->saveMaskIfNecessary(this);
        PlatformPainterState* state = new PlatformPainterState(*m_state);
        m_stateStack.append(state);
        m_state = m_stateStack.last();
    } else if (saveMode == PainterOpenVG::CreateNewStateWithPaintStateOnly) {
        m_state->saveMaskIfNecessary(this);
        PlatformPainterState* state = new PlatformPainterState();
        state->copyPaintState(m_state);
        m_stateStack.append(state);
        m_state = m_stateStack.last();
    } else // if (saveMode == PainterOpenVG::KeepCurrentState)
        m_state->saveMaskIfNecessary(this);
}

void PainterOpenVG::restore()
{
    ASSERT(m_stateStack.size() >= 2);
    m_surface->makeCurrent(SurfaceOpenVG::DontApplyPainterState);

    PlatformPainterState* state = m_stateStack.last();
    m_stateStack.removeLast();
    delete state;

    m_state = m_stateStack.last();
    m_state->applyState(this);
}

}
