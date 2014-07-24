/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Dirk Schulze <krit@webkit.org>
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

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEImage.h"

#include "AffineTransform.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderObject.h"
#include "RenderTreeAsText.h"
#include "SVGFilter.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGRenderingContext.h"
#include "SVGStyledElement.h"
#include "SVGURIReference.h"
#include "TextStream.h"

namespace WebCore {

FEImage::FEImage(Filter* filter, PassRefPtr<Image> image, const SVGPreserveAspectRatio& preserveAspectRatio)
    : FilterEffect(filter)
    , m_image(image)
    , m_document(0)
    , m_preserveAspectRatio(preserveAspectRatio)
{
}

FEImage::FEImage(Filter* filter, Document* document, const String& href, const SVGPreserveAspectRatio& preserveAspectRatio)
    : FilterEffect(filter)
    , m_document(document)
    , m_href(href)
    , m_preserveAspectRatio(preserveAspectRatio)
{
}

PassRefPtr<FEImage> FEImage::createWithImage(Filter* filter, PassRefPtr<Image> image, const SVGPreserveAspectRatio& preserveAspectRatio)
{
    return adoptRef(new FEImage(filter, image, preserveAspectRatio));
}

PassRefPtr<FEImage> FEImage::createWithIRIReference(Filter* filter, Document* document, const String& href, const SVGPreserveAspectRatio& preserveAspectRatio)
{
    return adoptRef(new FEImage(filter, document, href, preserveAspectRatio));
}

void FEImage::determineAbsolutePaintRect()
{
    SVGFilter* svgFilter = static_cast<SVGFilter*>(filter());

    FloatRect paintRect = svgFilter->absoluteTransform().mapRect(filterPrimitiveSubregion());
    FloatRect srcRect;
    if (m_image) {
        srcRect.setSize(m_image->size());
        m_preserveAspectRatio.transformRect(paintRect, srcRect);
    } else if (RenderObject* renderer = referencedRenderer())
        srcRect = svgFilter->absoluteTransform().mapRect(renderer->repaintRectInLocalCoordinates());

    if (clipsToBounds())
        paintRect.intersect(maxEffectRect());
    else
        paintRect.unite(maxEffectRect());
    setAbsolutePaintRect(enclosingIntRect(paintRect));
}

RenderObject* FEImage::referencedRenderer() const
{
    if (!m_document)
        return 0;
    Element* hrefElement = SVGURIReference::targetElementFromIRIString(m_href, m_document);
    if (!hrefElement || !hrefElement->isSVGElement())
        return 0;
    return hrefElement->renderer();
}

void FEImage::platformApplySoftware()
{
    RenderObject* renderer = referencedRenderer();
    if (!m_image && !renderer)
        return;

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;

    SVGFilter* svgFilter = static_cast<SVGFilter*>(filter());
    FloatRect destRect = svgFilter->absoluteTransform().mapRect(filterPrimitiveSubregion());

    FloatRect srcRect;
    if (renderer)
        srcRect = svgFilter->absoluteTransform().mapRect(renderer->repaintRectInLocalCoordinates());
    else {
        srcRect = FloatRect(FloatPoint(), m_image->size());
        m_preserveAspectRatio.transformRect(destRect, srcRect);
    }

    IntPoint paintLocation = absolutePaintRect().location();
    destRect.move(-paintLocation.x(), -paintLocation.y());

    // FEImage results are always in ColorSpaceDeviceRGB
    setResultColorSpace(ColorSpaceDeviceRGB);

    if (renderer) {
        const AffineTransform& absoluteTransform = svgFilter->absoluteTransform();
        resultImage->context()->concatCTM(absoluteTransform);

        SVGElement* contextNode = toSVGElement(renderer->node());
        if (contextNode->isSVGStyledElement() && toSVGStyledElement(contextNode)->hasRelativeLengths()) {
            SVGLengthContext lengthContext(contextNode);
            float width = 0;
            float height = 0;

            // If we're referencing an element with percentage units, eg. <rect with="30%"> those values were resolved against the viewport.
            // Build up a transformation that maps from the viewport space to the filter primitive subregion.
            if (lengthContext.determineViewport(width, height))
                resultImage->context()->concatCTM(makeMapBetweenRects(FloatRect(0, 0, width, height), destRect));
        }

        AffineTransform contentTransformation;
        SVGRenderingContext::renderSubtreeToImageBuffer(resultImage, renderer, contentTransformation);
        return;
    }

    resultImage->context()->drawImage(m_image.get(), ColorSpaceDeviceRGB, destRect, srcRect);
}

void FEImage::dump()
{
}

TextStream& FEImage::externalRepresentation(TextStream& ts, int indent) const
{
    IntSize imageSize;
    if (m_image)
        imageSize = m_image->size();
    else if (RenderObject* renderer = referencedRenderer())
        imageSize = enclosingIntRect(renderer->repaintRectInLocalCoordinates()).size();
    writeIndent(ts, indent);
    ts << "[feImage";
    FilterEffect::externalRepresentation(ts);
    ts << " image-size=\"" << imageSize.width() << "x" << imageSize.height() << "\"]\n";
    // FIXME: should this dump also object returned by SVGFEImage::image() ?
    return ts;
}

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
