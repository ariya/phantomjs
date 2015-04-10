/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if ENABLE(CSS_FILTERS)
#include "FilterOperation.h"

#include "AnimationUtilities.h"

#if ENABLE(SVG)
#include "CachedSVGDocumentReference.h"
#endif

namespace WebCore {

ReferenceFilterOperation::ReferenceFilterOperation(const String& url, const String& fragment, OperationType type)
    : FilterOperation(type)
    , m_url(url)
    , m_fragment(fragment)
{
}

ReferenceFilterOperation::~ReferenceFilterOperation()
{
}

#if ENABLE(SVG)
void ReferenceFilterOperation::setCachedSVGDocumentReference(PassOwnPtr<CachedSVGDocumentReference> cachedSVGDocumentReference)
{
    m_cachedSVGDocumentReference = cachedSVGDocumentReference;
}
#endif

PassRefPtr<FilterOperation> BasicColorMatrixFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToPassthrough)
        return BasicColorMatrixFilterOperation::create(WebCore::blend(m_amount, passthroughAmount(), progress), m_type);
        
    const BasicColorMatrixFilterOperation* fromOp = static_cast<const BasicColorMatrixFilterOperation*>(from);
    double fromAmount = fromOp ? fromOp->amount() : passthroughAmount();
    return BasicColorMatrixFilterOperation::create(WebCore::blend(fromAmount, m_amount, progress), m_type);
}

double BasicColorMatrixFilterOperation::passthroughAmount() const
{
    switch (m_type) {
    case GRAYSCALE:
    case SEPIA:
    case HUE_ROTATE:
        return 0;
    case SATURATE:
        return 1;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

PassRefPtr<FilterOperation> BasicComponentTransferFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToPassthrough)
        return BasicComponentTransferFilterOperation::create(WebCore::blend(m_amount, passthroughAmount(), progress), m_type);
        
    const BasicComponentTransferFilterOperation* fromOp = static_cast<const BasicComponentTransferFilterOperation*>(from);
    double fromAmount = fromOp ? fromOp->amount() : passthroughAmount();
    return BasicComponentTransferFilterOperation::create(WebCore::blend(fromAmount, m_amount, progress), m_type);
}

double BasicComponentTransferFilterOperation::passthroughAmount() const
{
    switch (m_type) {
    case OPACITY:
        return 1;
    case INVERT:
        return 0;
    case CONTRAST:
        return 1;
    case BRIGHTNESS:
        return 1;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

PassRefPtr<FilterOperation> GammaFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToPassthrough)
        return GammaFilterOperation::create(
            WebCore::blend(m_amplitude, 1.0, progress),
            WebCore::blend(m_exponent, 1.0, progress),
            WebCore::blend(m_offset, 0.0, progress), m_type);
        
    const GammaFilterOperation* fromOp = static_cast<const GammaFilterOperation*>(from);
    double fromAmplitude = fromOp ? fromOp->amplitude() : 1;
    double fromExponent = fromOp ? fromOp->exponent() : 1;
    double fromOffset = fromOp ? fromOp->offset() : 0;
    return GammaFilterOperation::create(
        WebCore::blend(fromAmplitude, m_amplitude, progress),
        WebCore::blend(fromExponent, m_exponent, progress),
        WebCore::blend(fromOffset, m_offset, progress), m_type);
}

PassRefPtr<FilterOperation> BlurFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;

    LengthType lengthType = m_stdDeviation.type();

    if (blendToPassthrough)
        return BlurFilterOperation::create(Length(lengthType).blend(m_stdDeviation, progress), m_type);

    const BlurFilterOperation* fromOp = static_cast<const BlurFilterOperation*>(from);
    Length fromLength = fromOp ? fromOp->m_stdDeviation : Length(lengthType);
    return BlurFilterOperation::create(m_stdDeviation.blend(fromLength, progress), m_type);
}

PassRefPtr<FilterOperation> DropShadowFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;

    if (blendToPassthrough)
        return DropShadowFilterOperation::create(
            WebCore::blend(m_location, IntPoint(), progress),
            WebCore::blend(m_stdDeviation, 0, progress),
            WebCore::blend(m_color, Color(Color::transparent), progress),
            m_type);

    const DropShadowFilterOperation* fromOp = static_cast<const DropShadowFilterOperation*>(from);
    IntPoint fromLocation = fromOp ? fromOp->location() : IntPoint();
    int fromStdDeviation = fromOp ? fromOp->stdDeviation() : 0;
    Color fromColor = fromOp ? fromOp->color() : Color(Color::transparent);
    
    return DropShadowFilterOperation::create(
        WebCore::blend(fromLocation, m_location, progress),
        WebCore::blend(fromStdDeviation, m_stdDeviation, progress),
        WebCore::blend(fromColor, m_color, progress), m_type);
}

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)
