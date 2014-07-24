/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGStyledElement.h"

#include "Attr.h"
#include "CSSParser.h"
#include "Document.h"
#include "EventNames.h"
#include "HTMLNames.h"
#include "NodeTraversal.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMasker.h"
#include "SVGElement.h"
#include "SVGElementInstance.h"
#include "SVGElementRareData.h"
#include "SVGNames.h"
#include "SVGRenderStyle.h"
#include "SVGRenderSupport.h"
#include "SVGSVGElement.h"
#include "SVGUseElement.h"
#include "ShadowRoot.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGStyledElement, HTMLNames::classAttr, ClassName, className)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGStyledElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(className)
END_REGISTER_ANIMATED_PROPERTIES

using namespace SVGNames;

void mapAttributeToCSSProperty(HashMap<AtomicStringImpl*, CSSPropertyID>* propertyNameToIdMap, const QualifiedName& attrName)
{
    // FIXME: when CSS supports "transform-origin" the special case for transform_originAttr can be removed.
    CSSPropertyID propertyId = cssPropertyID(attrName.localName());
    if (!propertyId && attrName == transform_originAttr)
        propertyId = CSSPropertyWebkitTransformOrigin; // cssPropertyID("-webkit-transform-origin")
    ASSERT(propertyId > 0);
    propertyNameToIdMap->set(attrName.localName().impl(), propertyId);
}

SVGStyledElement::SVGStyledElement(const QualifiedName& tagName, Document* document, ConstructionType constructionType)
    : SVGElement(tagName, document, constructionType)
{
    registerAnimatedPropertiesForSVGStyledElement();
}

String SVGStyledElement::title() const
{
    // According to spec, we should not return titles when hovering over root <svg> elements (those
    // <title> elements are the title of the document, not a tooltip) so we instantly return.
    if (isOutermostSVGSVGElement())
        return String();

    // Walk up the tree, to find out whether we're inside a <use> shadow tree, to find the right title.
    if (isInShadowTree()) {
        Element* shadowHostElement = toShadowRoot(treeScope()->rootNode())->host();
        // At this time, SVG nodes are not allowed in non-<use> shadow trees, so any shadow root we do
        // have should be a use. The assert and following test is here to catch future shadow DOM changes
        // that do enable SVG in a shadow tree.
        ASSERT(!shadowHostElement || shadowHostElement->hasTagName(SVGNames::useTag));
        if (shadowHostElement && shadowHostElement->hasTagName(SVGNames::useTag)) {
            SVGUseElement* useElement = toSVGUseElement(shadowHostElement);
 
            // If the <use> title is not empty we found the title to use.
            String useTitle(useElement->title());
            if (!useTitle.isEmpty())
               return useTitle;
        }
    }

    // If we aren't an instance in a <use> or the <use> title was not found, then find the first
    // <title> child of this element.
    Element* titleElement = ElementTraversal::firstWithin(this);
    for (; titleElement; titleElement = ElementTraversal::nextSkippingChildren(titleElement, this)) {
        if (titleElement->hasTagName(SVGNames::titleTag) && titleElement->isSVGElement())
            break;
    }

    // If a title child was found, return the text contents.
    if (titleElement)
        return titleElement->innerText();
    
    // Otherwise return a null/empty string.
    return String();
}

bool SVGStyledElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    // http://www.w3.org/TR/SVG/extend.html#PrivateData
    // Prevent anything other than SVG renderers from appearing in our render tree
    // Spec: SVG allows inclusion of elements from foreign namespaces anywhere
    // with the SVG content. In general, the SVG user agent will include the unknown
    // elements in the DOM but will otherwise ignore unknown elements. 
    if (!parentOrShadowHostElement() || parentOrShadowHostElement()->isSVGElement())
        return StyledElement::rendererIsNeeded(context);

    return false;
}

CSSPropertyID SVGStyledElement::cssPropertyIdForSVGAttributeName(const QualifiedName& attrName)
{
    if (!attrName.namespaceURI().isNull())
        return CSSPropertyInvalid;
    
    static HashMap<AtomicStringImpl*, CSSPropertyID>* propertyNameToIdMap = 0;
    if (!propertyNameToIdMap) {
        propertyNameToIdMap = new HashMap<AtomicStringImpl*, CSSPropertyID>;
        // This is a list of all base CSS and SVG CSS properties which are exposed as SVG XML attributes
        mapAttributeToCSSProperty(propertyNameToIdMap, alignment_baselineAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, baseline_shiftAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, buffered_renderingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, clipAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, clip_pathAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, clip_ruleAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, SVGNames::colorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, color_interpolationAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, color_interpolation_filtersAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, color_profileAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, color_renderingAttr); 
        mapAttributeToCSSProperty(propertyNameToIdMap, cursorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, SVGNames::directionAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, displayAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, dominant_baselineAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, enable_backgroundAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, fillAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, fill_opacityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, fill_ruleAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, filterAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, flood_colorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, flood_opacityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_familyAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_sizeAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_stretchAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_styleAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_variantAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, font_weightAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, glyph_orientation_horizontalAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, glyph_orientation_verticalAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, image_renderingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, kerningAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, letter_spacingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, lighting_colorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, marker_endAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, marker_midAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, marker_startAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, maskAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, mask_typeAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, opacityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, overflowAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, pointer_eventsAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, shape_renderingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stop_colorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stop_opacityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, strokeAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_dasharrayAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_dashoffsetAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_linecapAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_linejoinAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_miterlimitAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_opacityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, stroke_widthAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, text_anchorAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, text_decorationAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, text_renderingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, transform_originAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, unicode_bidiAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, vector_effectAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, visibilityAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, word_spacingAttr);
        mapAttributeToCSSProperty(propertyNameToIdMap, writing_modeAttr);
    }
    
    return propertyNameToIdMap->get(attrName.localName().impl());
}

typedef HashMap<QualifiedName, AnimatedPropertyType> AttributeToPropertyTypeMap;
static inline AttributeToPropertyTypeMap& cssPropertyToTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_cssPropertyMap, ());

    if (!s_cssPropertyMap.isEmpty())
        return s_cssPropertyMap;

    // Fill the map for the first use.
    s_cssPropertyMap.set(alignment_baselineAttr, AnimatedString);
    s_cssPropertyMap.set(baseline_shiftAttr, AnimatedString);
    s_cssPropertyMap.set(buffered_renderingAttr, AnimatedString);
    s_cssPropertyMap.set(clipAttr, AnimatedRect);
    s_cssPropertyMap.set(clip_pathAttr, AnimatedString);
    s_cssPropertyMap.set(clip_ruleAttr, AnimatedString);
    s_cssPropertyMap.set(SVGNames::colorAttr, AnimatedColor);
    s_cssPropertyMap.set(color_interpolationAttr, AnimatedString);
    s_cssPropertyMap.set(color_interpolation_filtersAttr, AnimatedString);
    s_cssPropertyMap.set(color_profileAttr, AnimatedString);
    s_cssPropertyMap.set(color_renderingAttr, AnimatedString); 
    s_cssPropertyMap.set(cursorAttr, AnimatedString);
    s_cssPropertyMap.set(displayAttr, AnimatedString);
    s_cssPropertyMap.set(dominant_baselineAttr, AnimatedString);
    s_cssPropertyMap.set(fillAttr, AnimatedColor);
    s_cssPropertyMap.set(fill_opacityAttr, AnimatedNumber);
    s_cssPropertyMap.set(fill_ruleAttr, AnimatedString);
    s_cssPropertyMap.set(filterAttr, AnimatedString);
    s_cssPropertyMap.set(flood_colorAttr, AnimatedColor);
    s_cssPropertyMap.set(flood_opacityAttr, AnimatedNumber);
    s_cssPropertyMap.set(font_familyAttr, AnimatedString);
    s_cssPropertyMap.set(font_sizeAttr, AnimatedLength);
    s_cssPropertyMap.set(font_stretchAttr, AnimatedString);
    s_cssPropertyMap.set(font_styleAttr, AnimatedString);
    s_cssPropertyMap.set(font_variantAttr, AnimatedString);
    s_cssPropertyMap.set(font_weightAttr, AnimatedString);
    s_cssPropertyMap.set(image_renderingAttr, AnimatedString);
    s_cssPropertyMap.set(kerningAttr, AnimatedLength);
    s_cssPropertyMap.set(letter_spacingAttr, AnimatedLength);
    s_cssPropertyMap.set(lighting_colorAttr, AnimatedColor);
    s_cssPropertyMap.set(marker_endAttr, AnimatedString);
    s_cssPropertyMap.set(marker_midAttr, AnimatedString);
    s_cssPropertyMap.set(marker_startAttr, AnimatedString);
    s_cssPropertyMap.set(maskAttr, AnimatedString);
    s_cssPropertyMap.set(mask_typeAttr, AnimatedString);
    s_cssPropertyMap.set(opacityAttr, AnimatedNumber);
    s_cssPropertyMap.set(overflowAttr, AnimatedString);
    s_cssPropertyMap.set(pointer_eventsAttr, AnimatedString);
    s_cssPropertyMap.set(shape_renderingAttr, AnimatedString);
    s_cssPropertyMap.set(stop_colorAttr, AnimatedColor);
    s_cssPropertyMap.set(stop_opacityAttr, AnimatedNumber);
    s_cssPropertyMap.set(strokeAttr, AnimatedColor);
    s_cssPropertyMap.set(stroke_dasharrayAttr, AnimatedLengthList);
    s_cssPropertyMap.set(stroke_dashoffsetAttr, AnimatedLength);
    s_cssPropertyMap.set(stroke_linecapAttr, AnimatedString);
    s_cssPropertyMap.set(stroke_linejoinAttr, AnimatedString);
    s_cssPropertyMap.set(stroke_miterlimitAttr, AnimatedNumber);
    s_cssPropertyMap.set(stroke_opacityAttr, AnimatedNumber);
    s_cssPropertyMap.set(stroke_widthAttr, AnimatedLength);
    s_cssPropertyMap.set(text_anchorAttr, AnimatedString);
    s_cssPropertyMap.set(text_decorationAttr, AnimatedString);
    s_cssPropertyMap.set(text_renderingAttr, AnimatedString);
    s_cssPropertyMap.set(vector_effectAttr, AnimatedString);
    s_cssPropertyMap.set(visibilityAttr, AnimatedString);
    s_cssPropertyMap.set(word_spacingAttr, AnimatedLength);
    return s_cssPropertyMap;
}

void SVGStyledElement::animatedPropertyTypeForAttribute(const QualifiedName& attrName, Vector<AnimatedPropertyType>& propertyTypes)
{
    SVGElement::animatedPropertyTypeForAttribute(attrName, propertyTypes);
    if (!propertyTypes.isEmpty())
        return;

    AttributeToPropertyTypeMap& cssPropertyTypeMap = cssPropertyToTypeMap();
    if (cssPropertyTypeMap.contains(attrName))
        propertyTypes.append(cssPropertyTypeMap.get(attrName));
}

bool SVGStyledElement::isAnimatableCSSProperty(const QualifiedName& attrName)
{
    return cssPropertyToTypeMap().contains(attrName);
}

bool SVGStyledElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (SVGStyledElement::cssPropertyIdForSVGAttributeName(name) > 0)
        return true;
    return SVGElement::isPresentationAttribute(name);
}

void SVGStyledElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    CSSPropertyID propertyID = SVGStyledElement::cssPropertyIdForSVGAttributeName(name);
    if (propertyID > 0)
        addPropertyToPresentationAttributeStyle(style, propertyID, value);
}

void SVGStyledElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    // SVG animation has currently requires special storage of values so we set
    // the className here.  svgAttributeChanged actually causes the resulting
    // style updates (instead of StyledElement::parseAttribute). We don't
    // tell StyledElement about the change to avoid parsing the class list twice
    if (name == HTMLNames::classAttr) {
        setClassNameBaseValue(value);
        return;
    }

    // id is handled by StyledElement which SVGElement inherits from
    SVGElement::parseAttribute(name, value);
}

bool SVGStyledElement::isKnownAttribute(const QualifiedName& attrName)
{
    return isIdAttributeName(attrName);
}

void SVGStyledElement::svgAttributeChanged(const QualifiedName& attrName)
{
    CSSPropertyID propId = SVGStyledElement::cssPropertyIdForSVGAttributeName(attrName);
    if (propId > 0) {
        SVGElementInstance::invalidateAllInstancesOfElement(this);
        return;
    }

    if (attrName == HTMLNames::classAttr) {
        classAttributeChanged(className());
        SVGElementInstance::invalidateAllInstancesOfElement(this);
        return;
    }

    if (isIdAttributeName(attrName)) {
        RenderObject* object = renderer();
        // Notify resources about id changes, this is important as we cache resources by id in SVGDocumentExtensions
        if (object && object->isSVGResourceContainer())
            object->toRenderSVGResourceContainer()->idChanged();
        if (inDocument())
            buildPendingResourcesIfNeeded();
        SVGElementInstance::invalidateAllInstancesOfElement(this);
        return;
    }
}

Node::InsertionNotificationRequest SVGStyledElement::insertedInto(ContainerNode* rootParent)
{
    SVGElement::insertedInto(rootParent);
    updateRelativeLengthsInformation();
    buildPendingResourcesIfNeeded();
    return InsertionDone;
}

void SVGStyledElement::buildPendingResourcesIfNeeded()
{
    Document* document = this->document();
    if (!needsPendingResourceHandling() || !document || !inDocument() || isInShadowTree())
        return;

    SVGDocumentExtensions* extensions = document->accessSVGExtensions();
    String resourceId = getIdAttribute();
    if (!extensions->hasPendingResource(resourceId))
        return;

    // Mark pending resources as pending for removal.
    extensions->markPendingResourcesForRemoval(resourceId);

    // Rebuild pending resources for each client of a pending resource that is being removed.
    while (Element* clientElement = extensions->removeElementFromPendingResourcesForRemoval(resourceId)) {
        ASSERT(clientElement->hasPendingResources());
        if (clientElement->hasPendingResources()) {
            clientElement->buildPendingResource();
            extensions->clearHasPendingResourcesIfPossible(clientElement);
        }
    }
}

void SVGStyledElement::removedFrom(ContainerNode* rootParent)
{
    if (rootParent->inDocument())
        updateRelativeLengthsInformation(false, this);
    SVGElement::removedFrom(rootParent);
    SVGElementInstance::invalidateAllInstancesOfElement(this);
}

void SVGStyledElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    // Invalidate all SVGElementInstances associated with us
    if (!changedByParser)
        SVGElementInstance::invalidateAllInstancesOfElement(this);
}

PassRefPtr<CSSValue> SVGStyledElement::getPresentationAttribute(const String& name)
{
    if (!hasAttributesWithoutUpdate())
        return 0;

    QualifiedName attributeName(nullAtom, name, nullAtom);
    const Attribute* attr = getAttributeItem(attributeName);
    if (!attr)
        return 0;

    RefPtr<MutableStylePropertySet> style = MutableStylePropertySet::create(SVGAttributeMode);
    CSSPropertyID propertyID = SVGStyledElement::cssPropertyIdForSVGAttributeName(attr->name());
    style->setProperty(propertyID, attr->value());
    RefPtr<CSSValue> cssValue = style->getPropertyCSSValue(propertyID);
    return cssValue ? cssValue->cloneForCSSOM() : 0;
}

bool SVGStyledElement::instanceUpdatesBlocked() const
{
    return hasSVGRareData() && svgRareData()->instanceUpdatesBlocked();
}

void SVGStyledElement::setInstanceUpdatesBlocked(bool value)
{
    if (hasSVGRareData())
        svgRareData()->setInstanceUpdatesBlocked(value);
}

AffineTransform SVGStyledElement::localCoordinateSpaceTransform(SVGLocatable::CTMScope) const
{
    // To be overriden by SVGGraphicsElement (or as special case SVGTextElement and SVGPatternElement)
    return AffineTransform();
}

void SVGStyledElement::updateRelativeLengthsInformation(bool hasRelativeLengths, SVGStyledElement* element)
{
    // If we're not yet in a document, this function will be called again from insertedInto(). Do nothing now.
    if (!inDocument())
        return;

    // An element wants to notify us that its own relative lengths state changed.
    // Register it in the relative length map, and register us in the parent relative length map.
    // Register the parent in the grandparents map, etc. Repeat procedure until the root of the SVG tree.

    if (hasRelativeLengths)
        m_elementsWithRelativeLengths.add(element);
    else {
        if (!m_elementsWithRelativeLengths.contains(element)) {
            // We were never registered. Do nothing.
            return;
        }

        m_elementsWithRelativeLengths.remove(element);
    }

    // Find first styled parent node, and notify it that we've changed our relative length state.
    ContainerNode* node = parentNode();
    while (node) {
        if (!node->isSVGElement())
            break;

        SVGElement* element = toSVGElement(node);
        if (!element->isSVGStyledElement()) {
            node = node->parentNode();
            continue;
        }

        // Register us in the parent element map.
        toSVGStyledElement(element)->updateRelativeLengthsInformation(hasRelativeLengths, this);
        break;
    }
}

bool SVGStyledElement::isMouseFocusable() const
{
    if (!isFocusable())
        return false;
    Element* eventTarget = const_cast<SVGStyledElement *>(this);
    return eventTarget->hasEventListeners(eventNames().focusinEvent) || eventTarget->hasEventListeners(eventNames().focusoutEvent);
}

bool SVGStyledElement::isKeyboardFocusable(KeyboardEvent*) const
{
    return isMouseFocusable();
}

}

#endif // ENABLE(SVG)
