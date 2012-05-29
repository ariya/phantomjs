/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "SVGSVGElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "CSSHelper.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "RenderSVGResource.h"
#include "RenderSVGRoot.h"
#include "RenderSVGViewportContainer.h"
#include "SMILTimeContainer.h"
#include "SVGAngle.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGViewElement.h"
#include "SVGViewSpec.h"
#include "SVGZoomEvent.h"
#include "ScriptEventListener.h"
#include "SelectionController.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGSVGElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGSVGElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGSVGElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGSVGElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_BOOLEAN(SVGSVGElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGSVGElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)
DEFINE_ANIMATED_RECT(SVGSVGElement, SVGNames::viewBoxAttr, ViewBox, viewBox)

inline SVGSVGElement::SVGSVGElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledLocatableElement(tagName, doc)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth, "100%")
    , m_height(LengthModeHeight, "100%") 
    , m_useCurrentView(false)
    , m_timeContainer(SMILTimeContainer::create(this))
    , m_scale(1)
    , m_containerSize(300, 150)
    , m_hasSetContainerSize(false)
{
    doc->registerForDocumentActivationCallbacks(this);
}

PassRefPtr<SVGSVGElement> SVGSVGElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGSVGElement(tagName, document));
}

SVGSVGElement::~SVGSVGElement()
{
    document()->unregisterForDocumentActivationCallbacks(this);
    // There are cases where removedFromDocument() is not called.
    // see ContainerNode::removeAllChildren, called by its destructor.
    document()->accessSVGExtensions()->removeTimeContainer(this);
}

void SVGSVGElement::willMoveToNewOwnerDocument()
{
    document()->unregisterForDocumentActivationCallbacks(this);
    SVGStyledLocatableElement::willMoveToNewOwnerDocument();
}

void SVGSVGElement::didMoveToNewOwnerDocument()
{
    document()->registerForDocumentActivationCallbacks(this);
    SVGStyledLocatableElement::didMoveToNewOwnerDocument();
}

const AtomicString& SVGSVGElement::contentScriptType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("text/ecmascript"));
    const AtomicString& n = getAttribute(SVGNames::contentScriptTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentScriptType(const AtomicString& type)
{
    setAttribute(SVGNames::contentScriptTypeAttr, type);
}

const AtomicString& SVGSVGElement::contentStyleType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("text/css"));
    const AtomicString& n = getAttribute(SVGNames::contentStyleTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentStyleType(const AtomicString& type)
{
    setAttribute(SVGNames::contentStyleTypeAttr, type);
}

FloatRect SVGSVGElement::viewport() const
{
    FloatRect viewRectangle;
    if (!isOutermostSVG())
        viewRectangle.setLocation(FloatPoint(x().value(this), y().value(this)));

    viewRectangle.setSize(FloatSize(width().value(this), height().value(this)));    
    return viewBoxToViewTransform(viewRectangle.width(), viewRectangle.height()).mapRect(viewRectangle);
}

int SVGSVGElement::relativeWidthValue() const
{
    SVGLength w = width();
    if (w.unitType() != LengthTypePercentage)
        return 0;

    return static_cast<int>(w.valueAsPercentage() * m_containerSize.width());
}

int SVGSVGElement::relativeHeightValue() const
{
    SVGLength h = height();
    if (h.unitType() != LengthTypePercentage)
        return 0;

    return static_cast<int>(h.valueAsPercentage() * m_containerSize.height());
}

float SVGSVGElement::pixelUnitToMillimeterX() const
{
    // 2.54 / cssPixelsPerInch gives CM.
    return (2.54f / cssPixelsPerInch) * 10.0f;
}

float SVGSVGElement::pixelUnitToMillimeterY() const
{
    // 2.54 / cssPixelsPerInch gives CM.
    return (2.54f / cssPixelsPerInch) * 10.0f;
}

float SVGSVGElement::screenPixelToMillimeterX() const
{
    return pixelUnitToMillimeterX();
}

float SVGSVGElement::screenPixelToMillimeterY() const
{
    return pixelUnitToMillimeterY();
}

bool SVGSVGElement::useCurrentView() const
{
    return m_useCurrentView;
}

void SVGSVGElement::setUseCurrentView(bool currentView)
{
    m_useCurrentView = currentView;
}

SVGViewSpec* SVGSVGElement::currentView() const
{
    if (!m_viewSpec)
        m_viewSpec = adoptPtr(new SVGViewSpec(const_cast<SVGSVGElement*>(this)));
    return m_viewSpec.get();
}

float SVGSVGElement::currentScale() const
{
    // Only the page zoom factor is relevant for SVG
    if (Frame* frame = document()->frame())
        return frame->pageZoomFactor();
    return m_scale;
}

void SVGSVGElement::setCurrentScale(float scale)
{
    if (Frame* frame = document()->frame()) {
        // Calling setCurrentScale() on the outermost <svg> element in a standalone SVG document
        // is allowed to change the page zoom factor, influencing the document size, scrollbars etc.
        if (parentNode() == document())
            frame->setPageZoomFactor(scale);
        return;
    }

    m_scale = scale;
    if (RenderObject* object = renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(object);
}

void SVGSVGElement::setCurrentTranslate(const FloatPoint& translation)
{
    m_translation = translation;
    updateCurrentTranslate();
}

void SVGSVGElement::updateCurrentTranslate()
{
    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);

    if (parentNode() == document() && document()->renderer())
        document()->renderer()->repaint();
}

void SVGSVGElement::parseMappedAttribute(Attribute* attr)
{
    if (!nearestViewportElement()) {
        bool setListener = true;

        // Only handle events if we're the outermost <svg> element
        if (attr->name() == HTMLNames::onunloadAttr)
            document()->setWindowAttributeEventListener(eventNames().unloadEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == HTMLNames::onresizeAttr)
            document()->setWindowAttributeEventListener(eventNames().resizeEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == HTMLNames::onscrollAttr)
            document()->setWindowAttributeEventListener(eventNames().scrollEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == SVGNames::onzoomAttr)
            document()->setWindowAttributeEventListener(eventNames().zoomEvent, createAttributeEventListener(document()->frame(), attr));
        else
            setListener = false;
 
        if (setListener)
            return;
    }

    if (attr->name() == HTMLNames::onabortAttr)
        document()->setWindowAttributeEventListener(eventNames().abortEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == HTMLNames::onerrorAttr)
        document()->setWindowAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        addCSSProperty(attr, CSSPropertyWidth, attr->value());
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        addCSSProperty(attr, CSSPropertyHeight, attr->value());
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGFitToViewBox::parseMappedAttribute(document(), attr))
            return;
        if (SVGZoomAndPan::parseMappedAttribute(attr))
            return;

        SVGStyledLocatableElement::parseMappedAttribute(attr);
    }
}

// This hack will not handle the case where we're setting a width/height
// on a root <svg> via svg.width.baseValue = when it has none.
static void updateCSSForAttribute(SVGSVGElement* element, const QualifiedName& attrName, CSSPropertyID property, const SVGLength& value)
{
    Attribute* attribute = element->attributes(false)->getAttributeItem(attrName);
    if (!attribute || !attribute->isMappedAttribute())
        return;
    element->addCSSProperty(attribute, property, value.valueAsString());
}

void SVGSVGElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    // FIXME: Ugly, ugly hack to around that parseMappedAttribute is not called
    // when svg.width.baseValue = 100 is evaluated.
    // Thus the CSS length value for width is not updated, and width() computeLogicalWidth()
    // calculations on RenderSVGRoot will be wrong.
    // https://bugs.webkit.org/show_bug.cgi?id=25387
    bool updateRelativeLengths = false;
    if (attrName == SVGNames::widthAttr) {
        updateCSSForAttribute(this, attrName, CSSPropertyWidth, widthBaseValue());
        updateRelativeLengths = true;
    } else if (attrName == SVGNames::heightAttr) {
        updateCSSForAttribute(this, attrName, CSSPropertyHeight, heightBaseValue());
        updateRelativeLengths = true;
    }

    if (updateRelativeLengths
        || attrName == SVGNames::xAttr
        || attrName == SVGNames::yAttr
        || SVGFitToViewBox::isKnownAttribute(attrName)) {
        updateRelativeLengths = true;
        updateRelativeLengthsInformation();
    }

    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    if (!renderer())
        return;

    if (updateRelativeLengths
        || SVGLangSpace::isKnownAttribute(attrName)
        || SVGExternalResourcesRequired::isKnownAttribute(attrName)
        || SVGZoomAndPan::isKnownAttribute(attrName)
        || SVGStyledLocatableElement::isKnownAttribute(attrName))
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer());
}

void SVGSVGElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeExternalResourcesRequired();
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (attrName == SVGNames::viewBoxAttr)
        synchronizeViewBox();
    else if (attrName == SVGNames::preserveAspectRatioAttr)
        synchronizePreserveAspectRatio();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

AttributeToPropertyTypeMap& SVGSVGElement::attributeToPropertyTypeMap()
{
    DEFINE_STATIC_LOCAL(AttributeToPropertyTypeMap, s_attributeToPropertyTypeMap, ());
    return s_attributeToPropertyTypeMap;
}

void SVGSVGElement::fillAttributeToPropertyTypeMap()
{
    AttributeToPropertyTypeMap& attributeToPropertyTypeMap = this->attributeToPropertyTypeMap();
    attributeToPropertyTypeMap.set(SVGNames::xAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::yAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::widthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::heightAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::viewBoxAttr, AnimatedRect);
    attributeToPropertyTypeMap.set(SVGNames::preserveAspectRatioAttr, AnimatedPreserveAspectRatio);
}

unsigned SVGSVGElement::suspendRedraw(unsigned /* maxWaitMilliseconds */)
{
    // FIXME: Implement me (see bug 11275)
    return 0;
}

void SVGSVGElement::unsuspendRedraw(unsigned /* suspendHandleId */)
{
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::unsuspendRedrawAll()
{
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::forceRedraw()
{
    // FIXME: Implement me (see bug 11275)
}

NodeList* SVGSVGElement::getIntersectionList(const FloatRect&, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

NodeList* SVGSVGElement::getEnclosureList(const FloatRect&, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

bool SVGSVGElement::checkIntersection(SVGElement*, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.intersects(getBBox());
}

bool SVGSVGElement::checkEnclosure(SVGElement*, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.contains(getBBox());
}

void SVGSVGElement::deselectAll()
{
    if (Frame* frame = document()->frame())
        frame->selection()->clear();
}

float SVGSVGElement::createSVGNumber()
{
    return 0.0f;
}

SVGLength SVGSVGElement::createSVGLength()
{
    return SVGLength();
}

SVGAngle SVGSVGElement::createSVGAngle()
{
    return SVGAngle();
}

FloatPoint SVGSVGElement::createSVGPoint()
{
    return FloatPoint();
}

SVGMatrix SVGSVGElement::createSVGMatrix()
{
    return SVGMatrix();
}

FloatRect SVGSVGElement::createSVGRect()
{
    return FloatRect();
}

SVGTransform SVGSVGElement::createSVGTransform()
{
    return SVGTransform(SVGTransform::SVG_TRANSFORM_MATRIX);
}

SVGTransform SVGSVGElement::createSVGTransformFromMatrix(const SVGMatrix& matrix)
{
    return SVGTransform(static_cast<const AffineTransform&>(matrix));
}

AffineTransform SVGSVGElement::localCoordinateSpaceTransform(SVGLocatable::CTMScope mode) const
{
    AffineTransform viewBoxTransform;
    if (attributes()->getAttributeItem(SVGNames::viewBoxAttr))
        viewBoxTransform = viewBoxToViewTransform(width().value(this), height().value(this));

    AffineTransform transform;
    if (!isOutermostSVG())
        transform.translate(x().value(this), y().value(this));
    else if (mode == SVGLocatable::ScreenScope) {
        if (RenderObject* renderer = this->renderer()) {
            // Translate in our CSS parent coordinate space
            // FIXME: This doesn't work correctly with CSS transforms.
            FloatPoint location = renderer->localToAbsolute(FloatPoint(), false, true);

            // Be careful here! localToAbsolute() includes the x/y offset coming from the viewBoxToViewTransform(), because
            // RenderSVGRoot::localToBorderBoxTransform() (called through mapLocalToContainer(), called from localToAbsolute())
            // also takes the viewBoxToViewTransform() into account, so we have to subtract it here (original cause of bug #27183)
            transform.translate(location.x() - viewBoxTransform.e(), location.y() - viewBoxTransform.f());

            // Respect scroll offset.
            if (FrameView* view = document()->view()) {
                IntSize scrollOffset = view->scrollOffset();
                transform.translate(-scrollOffset.width(), -scrollOffset.height());
            }
        }
    }

    return transform.multiply(viewBoxTransform);
}

RenderObject* SVGSVGElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    if (isOutermostSVG())
        return new (arena) RenderSVGRoot(this);

    return new (arena) RenderSVGViewportContainer(this);
}

void SVGSVGElement::insertedIntoDocument()
{
    document()->accessSVGExtensions()->addTimeContainer(this);
    SVGStyledLocatableElement::insertedIntoDocument();
}

void SVGSVGElement::removedFromDocument()
{
    document()->accessSVGExtensions()->removeTimeContainer(this);
    SVGStyledLocatableElement::removedFromDocument();
}

void SVGSVGElement::pauseAnimations()
{
    if (!m_timeContainer->isPaused())
        m_timeContainer->pause();
}

void SVGSVGElement::unpauseAnimations()
{
    if (m_timeContainer->isPaused())
        m_timeContainer->resume();
}

bool SVGSVGElement::animationsPaused() const
{
    return m_timeContainer->isPaused();
}

float SVGSVGElement::getCurrentTime() const
{
    return narrowPrecisionToFloat(m_timeContainer->elapsed().value());
}

void SVGSVGElement::setCurrentTime(float /* seconds */)
{
    // FIXME: Implement me, bug 12073
}

bool SVGSVGElement::selfHasRelativeLengths() const
{
    return x().isRelative()
        || y().isRelative()
        || width().isRelative()
        || height().isRelative()
        || hasAttribute(SVGNames::viewBoxAttr);
}

bool SVGSVGElement::isOutermostSVG() const
{
    // Element may not be in the document, pretend we're outermost for viewport(), getCTM(), etc.
    if (!parentNode())
        return true;

#if ENABLE(SVG_FOREIGN_OBJECT)
    // We act like an outermost SVG element, if we're a direct child of a <foreignObject> element.
    if (parentNode()->hasTagName(SVGNames::foreignObjectTag))
        return true;
#endif

    // This is true whenever this is the outermost SVG, even if there are HTML elements outside it
    return !parentNode()->isSVGElement();
}

AffineTransform SVGSVGElement::viewBoxToViewTransform(float viewWidth, float viewHeight) const
{
    FloatRect viewBoxRect;
    if (useCurrentView()) {
        if (currentView()) // what if we should use it but it is not set?
            viewBoxRect = currentView()->viewBox();
    } else
        viewBoxRect = viewBox();

    AffineTransform ctm = SVGFitToViewBox::viewBoxToViewTransform(viewBoxRect, preserveAspectRatio(), viewWidth, viewHeight);

    if (useCurrentView() && currentView()) {
        AffineTransform transform;
        if (currentView()->transform().concatenate(transform))
            ctm *= transform;
    }

    return ctm;
}

void SVGSVGElement::inheritViewAttributes(SVGViewElement* viewElement)
{
    setUseCurrentView(true);
    if (viewElement->hasAttribute(SVGNames::viewBoxAttr))
        currentView()->setViewBoxBaseValue(viewElement->viewBox());
    else
        currentView()->setViewBoxBaseValue(viewBox());

    SVGPreserveAspectRatio aspectRatio;
    if (viewElement->hasAttribute(SVGNames::preserveAspectRatioAttr))
        aspectRatio = viewElement->preserveAspectRatioBaseValue();
    else
        aspectRatio = preserveAspectRatioBaseValue();
    currentView()->setPreserveAspectRatioBaseValue(aspectRatio);

    if (viewElement->hasAttribute(SVGNames::zoomAndPanAttr))
        currentView()->setZoomAndPan(viewElement->zoomAndPan());
    
    if (RenderObject* object = renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(object);
}
    
void SVGSVGElement::documentWillBecomeInactive()
{
    pauseAnimations();
}

void SVGSVGElement::documentDidBecomeActive()
{
    unpauseAnimations();
}

// getElementById on SVGSVGElement is restricted to only the child subtree defined by the <svg> element.
// See http://www.w3.org/TR/SVG11/struct.html#InterfaceSVGSVGElement
Element* SVGSVGElement::getElementById(const AtomicString& id) const
{
    Element* element = treeScope()->getElementById(id);
    if (element && element->isDescendantOf(this))
        return element;

    // Fall back to traversing our subtree. Duplicate ids are allowed, the first found will
    // be returned.
    for (Node* node = traverseNextNode(this); node; node = node->traverseNextNode(this)) {
        if (!node->isElementNode())
            continue;

        Element* element = static_cast<Element*>(node);
        if (element->hasID() && element->getIdAttribute() == id)
            return element;
    }
    return 0;
}

}

#endif // ENABLE(SVG)
