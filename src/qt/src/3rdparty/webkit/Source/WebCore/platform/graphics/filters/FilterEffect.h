/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef FilterEffect_h
#define FilterEffect_h

#if ENABLE(FILTERS)
#include "FloatRect.h"
#include "IntRect.h"

#include <wtf/ByteArray.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Filter;
class FilterEffect;
class ImageBuffer;
class TextStream;

typedef Vector<RefPtr<FilterEffect> > FilterEffectVector;

enum FilterEffectType {
    FilterEffectTypeUnknown,
    FilterEffectTypeImage,
    FilterEffectTypeTile,
    FilterEffectTypeSourceInput
};

class FilterEffect : public RefCounted<FilterEffect> {
public:
    virtual ~FilterEffect();

    bool hasResult() const { return m_imageBufferResult || m_unmultipliedImageResult || m_premultipliedImageResult; }
    void clearResult();
    ImageBuffer* asImageBuffer();
    PassRefPtr<ByteArray> asUnmultipliedImage(const IntRect&);
    PassRefPtr<ByteArray> asPremultipliedImage(const IntRect&);
    void copyUnmultipliedImage(ByteArray* destination, const IntRect&);
    void copyPremultipliedImage(ByteArray* destination, const IntRect&);

    FilterEffectVector& inputEffects() { return m_inputEffects; }
    FilterEffect* inputEffect(unsigned) const;
    unsigned numberOfEffectInputs() const { return m_inputEffects.size(); }

    IntRect drawingRegionOfInputImage(const IntRect&) const;
    IntRect requestedRegionOfInputImageData(const IntRect&) const;

    // Solid black image with different alpha values.
    bool isAlphaImage() const { return m_alphaImage; }
    void setIsAlphaImage(bool alphaImage) { m_alphaImage = alphaImage; }

    IntRect absolutePaintRect() const { return m_absolutePaintRect; }
    void setAbsolutePaintRect(const IntRect& absolutePaintRect) { m_absolutePaintRect = absolutePaintRect; }

    IntRect maxEffectRect() const { return m_maxEffectRect; }
    void setMaxEffectRect(const IntRect& maxEffectRect) { m_maxEffectRect = maxEffectRect; } 

    virtual void apply() = 0;
    virtual void dump() = 0;

    virtual void determineAbsolutePaintRect();

    virtual FilterEffectType filterEffectType() const { return FilterEffectTypeUnknown; }

    virtual TextStream& externalRepresentation(TextStream&, int indention = 0) const;

public:
    // The following functions are SVG specific and will move to RenderSVGResourceFilterPrimitive.
    // See bug https://bugs.webkit.org/show_bug.cgi?id=45614.
    bool hasX() const { return m_hasX; }
    void setHasX(bool value) { m_hasX = value; }

    bool hasY() const { return m_hasY; }
    void setHasY(bool value) { m_hasY = value; }

    bool hasWidth() const { return m_hasWidth; }
    void setHasWidth(bool value) { m_hasWidth = value; }

    bool hasHeight() const { return m_hasHeight; }
    void setHasHeight(bool value) { m_hasHeight = value; }

    FloatRect filterPrimitiveSubregion() const { return m_filterPrimitiveSubregion; }
    void setFilterPrimitiveSubregion(const FloatRect& filterPrimitiveSubregion) { m_filterPrimitiveSubregion = filterPrimitiveSubregion; }

    FloatRect effectBoundaries() const { return m_effectBoundaries; }
    void setEffectBoundaries(const FloatRect& effectBoundaries) { m_effectBoundaries = effectBoundaries; }

    Filter* filter() { return m_filter; }

protected:
    FilterEffect(Filter*);

    ImageBuffer* createImageBufferResult();
    ByteArray* createUnmultipliedImageResult();
    ByteArray* createPremultipliedImageResult();

private:
    OwnPtr<ImageBuffer> m_imageBufferResult;
    RefPtr<ByteArray> m_unmultipliedImageResult;
    RefPtr<ByteArray> m_premultipliedImageResult;
    FilterEffectVector m_inputEffects;

    bool m_alphaImage;

    IntRect m_absolutePaintRect;
    
    // The maximum size of a filter primitive. In SVG this is the primitive subregion in absolute coordinate space.
    // The absolute paint rect should never be bigger than m_maxEffectRect.
    IntRect m_maxEffectRect;
    Filter* m_filter;

private:
    inline void copyImageBytes(ByteArray* source, ByteArray* destination, const IntRect&);

    // The following member variables are SVG specific and will move to RenderSVGResourceFilterPrimitive.
    // See bug https://bugs.webkit.org/show_bug.cgi?id=45614.

    // The subregion of a filter primitive according to the SVG Filter specification in local coordinates.
    // This is SVG specific and needs to move to RenderSVGResourceFilterPrimitive.
    FloatRect m_filterPrimitiveSubregion;

    // x, y, width and height of the actual SVGFE*Element. Is needed to determine the subregion of the
    // filter primitive on a later step.
    FloatRect m_effectBoundaries;
    bool m_hasX;
    bool m_hasY;
    bool m_hasWidth;
    bool m_hasHeight;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FilterEffect_h
