/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "RenderSVGResourceFilter.h"

#include "AffineTransform.h"
#include "FilterEffect.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "IntRect.h"
#include "Page.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceFilterPrimitive.h"
#include "SVGElement.h"
#include "SVGFilter.h"
#include "SVGFilterElement.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGNames.h"
#include "SVGRenderingContext.h"
#include "SVGStyledElement.h"
#include "SVGUnitTypes.h"
#include "Settings.h"
#include "SourceAlpha.h"
#include "SourceGraphic.h"
#include <wtf/Vector.h>

using namespace std;

namespace WebCore {

RenderSVGResourceType RenderSVGResourceFilter::s_resourceType = FilterResourceType;

RenderSVGResourceFilter::RenderSVGResourceFilter(SVGFilterElement* node)
    : RenderSVGResourceContainer(node)
{
}

RenderSVGResourceFilter::~RenderSVGResourceFilter()
{
    if (m_filter.isEmpty())
        return;

    deleteAllValues(m_filter);
    m_filter.clear();
}

void RenderSVGResourceFilter::removeAllClientsFromCache(bool markForInvalidation)
{
    if (!m_filter.isEmpty()) {
        deleteAllValues(m_filter);
        m_filter.clear();
    }

    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceFilter::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);

    if (FilterData* filterData = m_filter.get(client)) {
        if (filterData->savedContext)
            filterData->state = FilterData::MarkedForRemoval;
        else
            delete m_filter.take(client);
    }

    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

PassRefPtr<SVGFilterBuilder> RenderSVGResourceFilter::buildPrimitives(SVGFilter* filter)
{
    SVGFilterElement* filterElement = toSVGFilterElement(node());
    FloatRect targetBoundingBox = filter->targetBoundingBox();

    // Add effects to the builder
    RefPtr<SVGFilterBuilder> builder = SVGFilterBuilder::create(SourceGraphic::create(filter), SourceAlpha::create(filter));
    for (Node* node = filterElement->firstChild(); node; node = node->nextSibling()) {
        if (!node->isSVGElement())
            continue;

        SVGElement* element = toSVGElement(node);
        if (!element->isFilterEffect())
            continue;

        SVGFilterPrimitiveStandardAttributes* effectElement = static_cast<SVGFilterPrimitiveStandardAttributes*>(element);
        RefPtr<FilterEffect> effect = effectElement->build(builder.get(), filter);
        if (!effect) {
            builder->clearEffects();
            return 0;
        }
        builder->appendEffectToEffectReferences(effect, effectElement->renderer());
        effectElement->setStandardAttributes(effect.get());
        effect->setEffectBoundaries(SVGLengthContext::resolveRectangle<SVGFilterPrimitiveStandardAttributes>(effectElement, filterElement->primitiveUnits(), targetBoundingBox));
        effect->setOperatingColorSpace(
            effectElement->renderer()->style()->svgStyle()->colorInterpolationFilters() == CI_LINEARRGB ? ColorSpaceLinearRGB : ColorSpaceDeviceRGB);
        builder->add(effectElement->result(), effect);
    }
    return builder.release();
}

bool RenderSVGResourceFilter::fitsInMaximumImageSize(const FloatSize& size, FloatSize& scale)
{
    bool matchesFilterSize = true;
    if (size.width() > kMaxFilterSize) {
        scale.setWidth(scale.width() * kMaxFilterSize / size.width());
        matchesFilterSize = false;
    }
    if (size.height() > kMaxFilterSize) {
        scale.setHeight(scale.height() * kMaxFilterSize / size.height());
        matchesFilterSize = false;
    }

    return matchesFilterSize;
}

bool RenderSVGResourceFilter::applyResource(RenderObject* object, RenderStyle*, GraphicsContext*& context, unsigned short resourceMode)
{
    ASSERT(object);
    ASSERT(context);
    ASSERT_UNUSED(resourceMode, resourceMode == ApplyToDefaultMode);

    if (m_filter.contains(object)) {
        FilterData* filterData = m_filter.get(object);
        if (filterData->state == FilterData::PaintingSource || filterData->state == FilterData::Applying)
            filterData->state = FilterData::CycleDetected;
        return false; // Already built, or we're in a cycle, or we're marked for removal. Regardless, just do nothing more now.
    }

    OwnPtr<FilterData> filterData(adoptPtr(new FilterData));
    FloatRect targetBoundingBox = object->objectBoundingBox();

    SVGFilterElement* filterElement = toSVGFilterElement(node());
    filterData->boundaries = SVGLengthContext::resolveRectangle<SVGFilterElement>(filterElement, filterElement->filterUnits(), targetBoundingBox);
    if (filterData->boundaries.isEmpty())
        return false;

    // Determine absolute transformation matrix for filter. 
    AffineTransform absoluteTransform;
    SVGRenderingContext::calculateTransformationToOutermostCoordinateSystem(object, absoluteTransform);
    if (!absoluteTransform.isInvertible())
        return false;

    // Eliminate shear of the absolute transformation matrix, to be able to produce unsheared tile images for feTile.
    filterData->shearFreeAbsoluteTransform = AffineTransform(absoluteTransform.xScale(), 0, 0, absoluteTransform.yScale(), 0, 0);

    // Determine absolute boundaries of the filter and the drawing region.
    FloatRect absoluteFilterBoundaries = filterData->shearFreeAbsoluteTransform.mapRect(filterData->boundaries);
    filterData->drawingRegion = object->strokeBoundingBox();
    filterData->drawingRegion.intersect(filterData->boundaries);
    FloatRect absoluteDrawingRegion = filterData->shearFreeAbsoluteTransform.mapRect(filterData->drawingRegion);

    // Create the SVGFilter object.
    bool primitiveBoundingBoxMode = filterElement->primitiveUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX;
    filterData->filter = SVGFilter::create(filterData->shearFreeAbsoluteTransform, absoluteDrawingRegion, targetBoundingBox, filterData->boundaries, primitiveBoundingBoxMode);

    // Create all relevant filter primitives.
    filterData->builder = buildPrimitives(filterData->filter.get());
    if (!filterData->builder)
        return false;

    // Calculate the scale factor for the use of filterRes.
    // Also see http://www.w3.org/TR/SVG/filters.html#FilterEffectsRegion
    FloatSize scale(1, 1);
    if (filterElement->hasAttribute(SVGNames::filterResAttr)) {
        scale.setWidth(filterElement->filterResX() / absoluteFilterBoundaries.width());
        scale.setHeight(filterElement->filterResY() / absoluteFilterBoundaries.height());
    }

    if (scale.isEmpty())
        return false;

    // Determine scale factor for filter. The size of intermediate ImageBuffers shouldn't be bigger than kMaxFilterSize.
    FloatRect tempSourceRect = absoluteDrawingRegion;
    tempSourceRect.scale(scale.width(), scale.height());
    fitsInMaximumImageSize(tempSourceRect.size(), scale);

    // Set the scale level in SVGFilter.
    filterData->filter->setFilterResolution(scale);

    FilterEffect* lastEffect = filterData->builder->lastEffect();
    if (!lastEffect)
        return false;

    RenderSVGResourceFilterPrimitive::determineFilterPrimitiveSubregion(lastEffect);
    FloatRect subRegion = lastEffect->maxEffectRect();
    // At least one FilterEffect has a too big image size,
    // recalculate the effect sizes with new scale factors.
    if (!fitsInMaximumImageSize(subRegion.size(), scale)) {
        filterData->filter->setFilterResolution(scale);
        RenderSVGResourceFilterPrimitive::determineFilterPrimitiveSubregion(lastEffect);
    }

    // If the drawingRegion is empty, we have something like <g filter=".."/>.
    // Even if the target objectBoundingBox() is empty, we still have to draw the last effect result image in postApplyResource.
    if (filterData->drawingRegion.isEmpty()) {
        ASSERT(!m_filter.contains(object));
        filterData->savedContext = context;
        m_filter.set(object, filterData.leakPtr());
        return false;
    }

    // Change the coordinate transformation applied to the filtered element to reflect the resolution of the filter.
    AffineTransform effectiveTransform;
    effectiveTransform.scale(scale.width(), scale.height());
    effectiveTransform.multiply(filterData->shearFreeAbsoluteTransform);

    OwnPtr<ImageBuffer> sourceGraphic;
    RenderingMode renderingMode = object->document()->page()->settings()->acceleratedFiltersEnabled() ? Accelerated : Unaccelerated;
    if (!SVGRenderingContext::createImageBuffer(filterData->drawingRegion, effectiveTransform, sourceGraphic, ColorSpaceLinearRGB, renderingMode)) {
        ASSERT(!m_filter.contains(object));
        filterData->savedContext = context;
        m_filter.set(object, filterData.leakPtr());
        return false;
    }
    
    // Set the rendering mode from the page's settings.
    filterData->filter->setRenderingMode(renderingMode);

    GraphicsContext* sourceGraphicContext = sourceGraphic->context();
    ASSERT(sourceGraphicContext);
  
    filterData->sourceGraphicBuffer = sourceGraphic.release();
    filterData->savedContext = context;

    context = sourceGraphicContext;

    ASSERT(!m_filter.contains(object));
    m_filter.set(object, filterData.leakPtr());

    return true;
}

void RenderSVGResourceFilter::postApplyResource(RenderObject* object, GraphicsContext*& context, unsigned short resourceMode, const Path*, const RenderSVGShape*)
{
    ASSERT(object);
    ASSERT(context);
    ASSERT_UNUSED(resourceMode, resourceMode == ApplyToDefaultMode);

    FilterData* filterData = m_filter.get(object);
    if (!filterData)
        return;

    switch (filterData->state) {
    case FilterData::MarkedForRemoval:
        delete m_filter.take(object);
        return;

    case FilterData::CycleDetected:
    case FilterData::Applying:
        // We have a cycle if we are already applying the data.
        // This can occur due to FeImage referencing a source that makes use of the FEImage itself.
        // This is the first place we've hit the cycle, so set the state back to PaintingSource so the return stack
        // will continue correctly.
        filterData->state = FilterData::PaintingSource;
        return;

    case FilterData::PaintingSource:
        if (!filterData->savedContext) {
            removeClientFromCache(object);
            return;
        }

        context = filterData->savedContext;
        filterData->savedContext = 0;
        break;

    case FilterData::Built: { } // Empty
    }

    FilterEffect* lastEffect = filterData->builder->lastEffect();

    if (lastEffect && !filterData->boundaries.isEmpty() && !lastEffect->filterPrimitiveSubregion().isEmpty()) {
        // This is the real filtering of the object. It just needs to be called on the
        // initial filtering process. We just take the stored filter result on a
        // second drawing.
        if (filterData->state != FilterData::Built)
            filterData->filter->setSourceImage(filterData->sourceGraphicBuffer.release());

        // Always true if filterData is just built (filterData->state == FilterData::Built).
        if (!lastEffect->hasResult()) {
            filterData->state = FilterData::Applying;
            lastEffect->applyAll();
            lastEffect->correctFilterResultIfNeeded();
            lastEffect->transformResultColorSpace(ColorSpaceDeviceRGB);
        }
        filterData->state = FilterData::Built;

        ImageBuffer* resultImage = lastEffect->asImageBuffer();
        if (resultImage) {
            context->concatCTM(filterData->shearFreeAbsoluteTransform.inverse());

            context->scale(FloatSize(1 / filterData->filter->filterResolution().width(), 1 / filterData->filter->filterResolution().height()));
            context->drawImageBuffer(resultImage, object->style()->colorSpace(), lastEffect->absolutePaintRect());
            context->scale(filterData->filter->filterResolution());

            context->concatCTM(filterData->shearFreeAbsoluteTransform);
        }
    }
    filterData->sourceGraphicBuffer.clear();
}

FloatRect RenderSVGResourceFilter::resourceBoundingBox(RenderObject* object)
{
    if (SVGFilterElement* element = toSVGFilterElement(node()))
        return SVGLengthContext::resolveRectangle<SVGFilterElement>(element, element->filterUnits(), object->objectBoundingBox());

    return FloatRect();
}

void RenderSVGResourceFilter::primitiveAttributeChanged(RenderObject* object, const QualifiedName& attribute)
{
    HashMap<RenderObject*, FilterData*>::iterator it = m_filter.begin();
    HashMap<RenderObject*, FilterData*>::iterator end = m_filter.end();
    SVGFilterPrimitiveStandardAttributes* primitve = static_cast<SVGFilterPrimitiveStandardAttributes*>(object->node());

    for (; it != end; ++it) {
        FilterData* filterData = it->value;
        if (filterData->state != FilterData::Built)
            continue;

        SVGFilterBuilder* builder = filterData->builder.get();
        FilterEffect* effect = builder->effectByRenderer(object);
        if (!effect)
            continue;
        // Since all effects shares the same attribute value, all
        // or none of them will be changed.
        if (!primitve->setFilterEffectAttribute(effect, attribute))
            return;
        builder->clearResultsRecursive(effect);

        // Repaint the image on the screen.
        markClientForInvalidation(it->key, RepaintInvalidation);
    }
    markAllClientLayersForInvalidation();
}

FloatRect RenderSVGResourceFilter::drawingRegion(RenderObject* object) const
{
    FilterData* filterData = m_filter.get(object);
    return filterData ? filterData->drawingRegion : FloatRect();
}

}
#endif
