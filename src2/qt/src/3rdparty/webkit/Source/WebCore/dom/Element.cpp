/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
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
#include "Element.h"

#include "AXObjectCache.h"
#include "Attr.h"
#include "CSSParser.h"
#include "CSSSelectorList.h"
#include "CSSStyleSelector.h"
#include "ClassList.h"
#include "ClientRect.h"
#include "ClientRectList.h"
#include "DOMTokenList.h"
#include "DatasetDOMStringMap.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "ElementRareData.h"
#include "ExceptionCode.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "InspectorInstrumentation.h"
#include "NodeList.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "TextIterator.h"
#include "WebKitAnimationList.h"
#include "XMLNames.h"
#include "htmlediting.h"
#include <wtf/text/CString.h>

#if ENABLE(SVG)
#include "SVGElement.h"
#include "SVGNames.h"
#endif

namespace WebCore {

using namespace HTMLNames;
using namespace XMLNames;
    
class StyleSelectorParentPusher {
public:
    StyleSelectorParentPusher(Element* parent)
        : m_parent(parent)
        , m_pushedStyleSelector(0)
    {
    }
    void push()
    {
        if (m_pushedStyleSelector)
            return;
        m_pushedStyleSelector = m_parent->document()->styleSelector();
        m_pushedStyleSelector->pushParent(m_parent);
    }
    ~StyleSelectorParentPusher() 
    {

        if (!m_pushedStyleSelector)
            return;

        // This tells us that our pushed style selector is in a bad state,
        // so we should just bail out in that scenario.
        ASSERT(m_pushedStyleSelector == m_parent->document()->styleSelector());
        if (m_pushedStyleSelector != m_parent->document()->styleSelector())
            return;

        m_pushedStyleSelector->popParent(m_parent); 
    }

private:
    Element* m_parent;
    CSSStyleSelector* m_pushedStyleSelector;
};

PassRefPtr<Element> Element::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new Element(tagName, document, CreateElement));
}

Element::~Element()
{
    removeShadowRoot();
    if (m_attributeMap)
        m_attributeMap->detachFromElement();
}

inline ElementRareData* Element::rareData() const
{
    ASSERT(hasRareData());
    return static_cast<ElementRareData*>(NodeRareData::rareDataFromMap(this));
}
    
inline ElementRareData* Element::ensureRareData()
{
    return static_cast<ElementRareData*>(Node::ensureRareData());
}
    
NodeRareData* Element::createRareData()
{
    return new ElementRareData;
}

DEFINE_VIRTUAL_ATTRIBUTE_EVENT_LISTENER(Element, blur);
DEFINE_VIRTUAL_ATTRIBUTE_EVENT_LISTENER(Element, error);
DEFINE_VIRTUAL_ATTRIBUTE_EVENT_LISTENER(Element, focus);
DEFINE_VIRTUAL_ATTRIBUTE_EVENT_LISTENER(Element, load);

PassRefPtr<DocumentFragment> Element::deprecatedCreateContextualFragment(const String& markup, FragmentScriptingPermission scriptingPermission)
{
    RefPtr<DocumentFragment> fragment = document()->createDocumentFragment();

    if (document()->isHTMLDocument())
        fragment->parseHTML(markup, this, scriptingPermission);
    else {
        if (!fragment->parseXML(markup, this, scriptingPermission))
            // FIXME: We should propagate a syntax error exception out here.
            return 0;
    }

    // Exceptions are ignored because none ought to happen here.
    ExceptionCode ignoredExceptionCode;

    // We need to pop <html> and <body> elements and remove <head> to
    // accommodate folks passing complete HTML documents to make the
    // child of an element.

    RefPtr<Node> nextNode;
    for (RefPtr<Node> node = fragment->firstChild(); node; node = nextNode) {
        nextNode = node->nextSibling();
        if (node->hasTagName(htmlTag) || node->hasTagName(headTag) || node->hasTagName(bodyTag)) {
            HTMLElement* element = toHTMLElement(node.get());
            Node* firstChild = element->firstChild();
            if (firstChild)
                nextNode = firstChild;
            RefPtr<Node> nextChild;
            for (RefPtr<Node> child = firstChild; child; child = nextChild) {
                nextChild = child->nextSibling();
                element->removeChild(child.get(), ignoredExceptionCode);
                ASSERT(!ignoredExceptionCode);
                fragment->insertBefore(child, element, ignoredExceptionCode);
                ASSERT(!ignoredExceptionCode);
            }
            fragment->removeChild(element, ignoredExceptionCode);
            ASSERT(!ignoredExceptionCode);
        }
    }
    return fragment.release();
}

PassRefPtr<Node> Element::cloneNode(bool deep)
{
    return deep ? cloneElementWithChildren() : cloneElementWithoutChildren();
}

PassRefPtr<Element> Element::cloneElementWithChildren()
{
    RefPtr<Element> clone = cloneElementWithoutChildren();
    cloneChildNodes(clone.get());
    return clone.release();
}

PassRefPtr<Element> Element::cloneElementWithoutChildren()
{
    RefPtr<Element> clone = cloneElementWithoutAttributesAndChildren();
    // This will catch HTML elements in the wrong namespace that are not correctly copied.
    // This is a sanity check as HTML overloads some of the DOM methods.
    ASSERT(isHTMLElement() == clone->isHTMLElement());

    // Call attributes(true) to force attribute synchronization to occur for SVG and style attributes.
    if (NamedNodeMap* attributeMap = attributes(true))
        clone->attributes()->setAttributes(*attributeMap);

    clone->copyNonAttributeProperties(this);

    return clone.release();
}

PassRefPtr<Element> Element::cloneElementWithoutAttributesAndChildren() const
{
    return document()->createElement(tagQName(), false);
}

void Element::copyNonAttributeProperties(const Element* source)
{
    ShadowRoot* sourceShadow = source->shadowRoot();
    removeShadowRoot();
    if (sourceShadow) {
        ShadowRoot* clonedShadow = ensureShadowRoot();
        sourceShadow->cloneChildNodes(clonedShadow);
    }
}

void Element::removeAttribute(const QualifiedName& name, ExceptionCode& ec)
{
    if (m_attributeMap) {
        ec = 0;
        m_attributeMap->removeNamedItem(name, ec);
        if (ec == NOT_FOUND_ERR)
            ec = 0;
    }
}

void Element::setAttribute(const QualifiedName& name, const AtomicString& value)
{
    ExceptionCode ec;
    setAttribute(name, value, ec);
}
    
void Element::setCStringAttribute(const QualifiedName& name, const char* cStringValue)
{
    ExceptionCode ec;
    setAttribute(name, AtomicString(cStringValue), ec);
}

void Element::setBooleanAttribute(const QualifiedName& name, bool b)
{
    if (b)
        setAttribute(name, emptyAtom);
    else {
        ExceptionCode ex;
        removeAttribute(name, ex);
    }
}

Node::NodeType Element::nodeType() const
{
    return ELEMENT_NODE;
}

bool Element::hasAttribute(const QualifiedName& name) const
{
    return hasAttributeNS(name.namespaceURI(), name.localName());
}

const AtomicString& Element::getAttribute(const QualifiedName& name) const
{
    if (UNLIKELY(name == styleAttr) && !isStyleAttributeValid())
        updateStyleAttribute();

#if ENABLE(SVG)
    if (UNLIKELY(!areSVGAttributesValid()))
        updateAnimatedSVGAttribute(name);
#endif

    return fastGetAttribute(name);
}

void Element::scrollIntoView(bool alignToTop) 
{
    document()->updateLayoutIgnorePendingStylesheets();
    IntRect bounds = getRect();    
    if (renderer()) {
        // Align to the top / bottom and to the closest edge.
        if (alignToTop)
            renderer()->enclosingLayer()->scrollRectToVisible(bounds, false, ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignTopAlways);
        else
            renderer()->enclosingLayer()->scrollRectToVisible(bounds, false, ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignBottomAlways);
    }
}

void Element::scrollIntoViewIfNeeded(bool centerIfNeeded)
{
    document()->updateLayoutIgnorePendingStylesheets();
    IntRect bounds = getRect();    
    if (renderer()) {
        if (centerIfNeeded)
            renderer()->enclosingLayer()->scrollRectToVisible(bounds, false, ScrollAlignment::alignCenterIfNeeded, ScrollAlignment::alignCenterIfNeeded);
        else
            renderer()->enclosingLayer()->scrollRectToVisible(bounds, false, ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignToEdgeIfNeeded);
    }
}

void Element::scrollByUnits(int units, ScrollGranularity granularity)
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderObject *rend = renderer()) {
        if (rend->hasOverflowClip()) {
            ScrollDirection direction = ScrollDown;
            if (units < 0) {
                direction = ScrollUp;
                units = -units;
            }
            toRenderBox(rend)->layer()->scroll(direction, granularity, units);
        }
    }
}

void Element::scrollByLines(int lines)
{
    scrollByUnits(lines, ScrollByLine);
}

void Element::scrollByPages(int pages)
{
    scrollByUnits(pages, ScrollByPage);
}

static float localZoomForRenderer(RenderObject* renderer)
{
    // FIXME: This does the wrong thing if two opposing zooms are in effect and canceled each
    // other out, but the alternative is that we'd have to crawl up the whole render tree every
    // time (or store an additional bit in the RenderStyle to indicate that a zoom was specified).
    float zoomFactor = 1;
    if (renderer->style()->effectiveZoom() != 1) {
        // Need to find the nearest enclosing RenderObject that set up
        // a differing zoom, and then we divide our result by it to eliminate the zoom.
        RenderObject* prev = renderer;
        for (RenderObject* curr = prev->parent(); curr; curr = curr->parent()) {
            if (curr->style()->effectiveZoom() != prev->style()->effectiveZoom()) {
                zoomFactor = prev->style()->zoom();
                break;
            }
            prev = curr;
        }
        if (prev->isRenderView())
            zoomFactor = prev->style()->zoom();
    }
    return zoomFactor;
}

static int adjustForLocalZoom(int value, RenderObject* renderer)
{
    float zoomFactor = localZoomForRenderer(renderer);
    if (zoomFactor == 1)
        return value;
    // Needed because computeLengthInt truncates (rather than rounds) when scaling up.
    if (zoomFactor > 1)
        value++;
    return static_cast<int>(value / zoomFactor);
}

int Element::offsetLeft()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = renderBoxModelObject())
        return adjustForLocalZoom(rend->offsetLeft(), rend);
    return 0;
}

int Element::offsetTop()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = renderBoxModelObject())
        return adjustForLocalZoom(rend->offsetTop(), rend);
    return 0;
}

int Element::offsetWidth()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = renderBoxModelObject())
        return adjustForAbsoluteZoom(rend->offsetWidth(), rend);
    return 0;
}

int Element::offsetHeight()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = renderBoxModelObject())
        return adjustForAbsoluteZoom(rend->offsetHeight(), rend);
    return 0;
}

Element* Element::offsetParent()
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderObject* rend = renderer())
        if (RenderObject* offsetParent = rend->offsetParent())
            return static_cast<Element*>(offsetParent->node());
    return 0;
}

int Element::clientLeft()
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->clientLeft(), rend);
    return 0;
}

int Element::clientTop()
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->clientTop(), rend);
    return 0;
}

int Element::clientWidth()
{
    document()->updateLayoutIgnorePendingStylesheets();

    // When in strict mode, clientWidth for the document element should return the width of the containing frame.
    // When in quirks mode, clientWidth for the body element should return the width of the containing frame.
    bool inQuirksMode = document()->inQuirksMode();
    if ((!inQuirksMode && document()->documentElement() == this) ||
        (inQuirksMode && isHTMLElement() && document()->body() == this)) {
        if (FrameView* view = document()->view()) {
            if (RenderView* renderView = document()->renderView())
                return adjustForAbsoluteZoom(view->layoutWidth(), renderView);
        }
    }
    
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->clientWidth(), rend);
    return 0;
}

int Element::clientHeight()
{
    document()->updateLayoutIgnorePendingStylesheets();

    // When in strict mode, clientHeight for the document element should return the height of the containing frame.
    // When in quirks mode, clientHeight for the body element should return the height of the containing frame.
    bool inQuirksMode = document()->inQuirksMode();     

    if ((!inQuirksMode && document()->documentElement() == this) ||
        (inQuirksMode && isHTMLElement() && document()->body() == this)) {
        if (FrameView* view = document()->view()) {
            if (RenderView* renderView = document()->renderView())
                return adjustForAbsoluteZoom(view->layoutHeight(), renderView);
        }
    }
    
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->clientHeight(), rend);
    return 0;
}

int Element::scrollLeft() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->scrollLeft(), rend);
    return 0;
}

int Element::scrollTop() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->scrollTop(), rend);
    return 0;
}

void Element::setScrollLeft(int newLeft)
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        rend->setScrollLeft(static_cast<int>(newLeft * rend->style()->effectiveZoom()));
}

void Element::setScrollTop(int newTop)
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        rend->setScrollTop(static_cast<int>(newTop * rend->style()->effectiveZoom()));
}

int Element::scrollWidth() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->scrollWidth(), rend);
    return 0;
}

int Element::scrollHeight() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    if (RenderBox* rend = renderBox())
        return adjustForAbsoluteZoom(rend->scrollHeight(), rend);
    return 0;
}

IntRect Element::boundsInWindowSpace() const
{
    document()->updateLayoutIgnorePendingStylesheets();

    FrameView* view = document()->view();
    if (!view)
        return IntRect();

    Vector<FloatQuad> quads;
#if ENABLE(SVG)
    if (isSVGElement() && renderer()) {
        // Get the bounding rectangle from the SVG model.
        const SVGElement* svgElement = static_cast<const SVGElement*>(this);
        FloatRect localRect;
        if (svgElement->boundingBox(localRect))
            quads.append(renderer()->localToAbsoluteQuad(localRect));
    } else
#endif
    {
        // Get the bounding rectangle from the box model.
        if (renderBoxModelObject())
            renderBoxModelObject()->absoluteQuads(quads);
    }

    if (quads.isEmpty())
        return IntRect();

    IntRect result = quads[0].enclosingBoundingBox();
    for (size_t i = 1; i < quads.size(); ++i)
        result.unite(quads[i].enclosingBoundingBox());

    result = view->contentsToWindow(result);
    return result;
}

PassRefPtr<ClientRectList> Element::getClientRects() const
{
    document()->updateLayoutIgnorePendingStylesheets();

    RenderBoxModelObject* renderBoxModelObject = this->renderBoxModelObject();
    if (!renderBoxModelObject)
        return ClientRectList::create();

    // FIXME: Handle SVG elements.
    // FIXME: Handle table/inline-table with a caption.

    Vector<FloatQuad> quads;
    renderBoxModelObject->absoluteQuads(quads);

    float pageScale = 1;
    if (Page* page = document()->page()) {
        if (Frame* frame = page->mainFrame())
            pageScale = frame->pageScaleFactor();
    }

    if (FrameView* view = document()->view()) {
        IntRect visibleContentRect = view->visibleContentRect();
        for (size_t i = 0; i < quads.size(); ++i) {
            quads[i].move(-visibleContentRect.x(), -visibleContentRect.y());
            adjustFloatQuadForAbsoluteZoom(quads[i], renderBoxModelObject);
            if (pageScale != 1)
                adjustFloatQuadForPageScale(quads[i], pageScale);
        }
    }

    return ClientRectList::create(quads);
}

PassRefPtr<ClientRect> Element::getBoundingClientRect() const
{
    document()->updateLayoutIgnorePendingStylesheets();

    Vector<FloatQuad> quads;
#if ENABLE(SVG)
    if (isSVGElement() && renderer()) {
        // Get the bounding rectangle from the SVG model.
        const SVGElement* svgElement = static_cast<const SVGElement*>(this);
        FloatRect localRect;
        if (svgElement->boundingBox(localRect))
            quads.append(renderer()->localToAbsoluteQuad(localRect));
    } else
#endif
    {
        // Get the bounding rectangle from the box model.
        if (renderBoxModelObject())
            renderBoxModelObject()->absoluteQuads(quads);
    }

    if (quads.isEmpty())
        return ClientRect::create();

    FloatRect result = quads[0].boundingBox();
    for (size_t i = 1; i < quads.size(); ++i)
        result.unite(quads[i].boundingBox());

    if (FrameView* view = document()->view()) {
        IntRect visibleContentRect = view->visibleContentRect();
        result.move(-visibleContentRect.x(), -visibleContentRect.y());
    }

    adjustFloatRectForAbsoluteZoom(result, renderer());
    if (Page* page = document()->page()) {
        if (Frame* frame = page->mainFrame())
            adjustFloatRectForPageScale(result, frame->pageScaleFactor());
    }

    return ClientRect::create(result);
}
    
IntRect Element::screenRect() const
{
    if (!renderer())
        return IntRect();
    return renderer()->view()->frameView()->contentsToScreen(renderer()->absoluteBoundingBoxRect());
}

static inline bool shouldIgnoreAttributeCase(const Element* e)
{
    return e && e->document()->isHTMLDocument() && e->isHTMLElement();
}

const AtomicString& Element::getAttribute(const String& name) const
{
    bool ignoreCase = shouldIgnoreAttributeCase(this);
    
    // Update the 'style' attribute if it's invalid and being requested:
    if (!isStyleAttributeValid() && equalPossiblyIgnoringCase(name, styleAttr.localName(), ignoreCase))
        updateStyleAttribute();

#if ENABLE(SVG)
    if (!areSVGAttributesValid()) {
        // We're not passing a namespace argument on purpose. SVGNames::*Attr are defined w/o namespaces as well.
        updateAnimatedSVGAttribute(QualifiedName(nullAtom, name, nullAtom));
    }
#endif

    if (m_attributeMap) {
        if (Attribute* attribute = m_attributeMap->getAttributeItem(name, ignoreCase))
            return attribute->value();
    }

    return nullAtom;
}

const AtomicString& Element::getAttributeNS(const String& namespaceURI, const String& localName) const
{
    return getAttribute(QualifiedName(nullAtom, localName, namespaceURI));
}

void Element::setAttribute(const AtomicString& name, const AtomicString& value, ExceptionCode& ec)
{
    if (!Document::isValidName(name)) {
        ec = INVALID_CHARACTER_ERR;
        return;
    }

#if ENABLE(INSPECTOR)
    if (!isSynchronizingStyleAttribute())
        InspectorInstrumentation::willModifyDOMAttr(document(), this);
#endif

    const AtomicString& localName = shouldIgnoreAttributeCase(this) ? name.lower() : name;
    QualifiedName attributeName(nullAtom, localName, nullAtom);
    
    // Allocate attribute map if necessary.
    Attribute* old = attributes(false)->getAttributeItem(localName, false);

    document()->incDOMTreeVersion();

    if (isIdAttributeName(old ? old->name() : attributeName))
        updateId(old ? old->value() : nullAtom, value);

    if (old && value.isNull())
        m_attributeMap->removeAttribute(old->name());
    else if (!old && !value.isNull())
        m_attributeMap->addAttribute(createAttribute(attributeName, value));
    else if (old && !value.isNull()) {
        if (Attr* attrNode = old->attr())
            attrNode->setValue(value);
        else
            old->setValue(value);
        attributeChanged(old);
    }

#if ENABLE(INSPECTOR)
    if (!isSynchronizingStyleAttribute())
        InspectorInstrumentation::didModifyDOMAttr(document(), this);
#endif
}

void Element::setAttribute(const QualifiedName& name, const AtomicString& value, ExceptionCode&)
{
#if ENABLE(INSPECTOR)
    if (!isSynchronizingStyleAttribute())
        InspectorInstrumentation::willModifyDOMAttr(document(), this);
#endif

    document()->incDOMTreeVersion();

    // Allocate attribute map if necessary.
    Attribute* old = attributes(false)->getAttributeItem(name);

    if (isIdAttributeName(name))
        updateId(old ? old->value() : nullAtom, value);

    if (old && value.isNull())
        m_attributeMap->removeAttribute(name);
    else if (!old && !value.isNull())
        m_attributeMap->addAttribute(createAttribute(name, value));
    else if (old) {
        if (Attr* attrNode = old->attr())
            attrNode->setValue(value);
        else
            old->setValue(value);
        attributeChanged(old);
    }

#if ENABLE(INSPECTOR)
    if (!isSynchronizingStyleAttribute())
        InspectorInstrumentation::didModifyDOMAttr(document(), this);
#endif
}

PassRefPtr<Attribute> Element::createAttribute(const QualifiedName& name, const AtomicString& value)
{
    return Attribute::create(name, value);
}

void Element::attributeChanged(Attribute* attr, bool)
{
    if (isIdAttributeName(attr->name()))
        idAttributeChanged(attr);
    recalcStyleIfNeededAfterAttributeChanged(attr);
    updateAfterAttributeChanged(attr);
}

void Element::updateAfterAttributeChanged(Attribute* attr)
{
    if (!AXObjectCache::accessibilityEnabled())
        return;

    const QualifiedName& attrName = attr->name();
    if (attrName == aria_activedescendantAttr) {
        // any change to aria-activedescendant attribute triggers accessibility focus change, but document focus remains intact
        document()->axObjectCache()->handleActiveDescendantChanged(renderer());
    } else if (attrName == roleAttr) {
        // the role attribute can change at any time, and the AccessibilityObject must pick up these changes
        document()->axObjectCache()->handleAriaRoleChanged(renderer());
    } else if (attrName == aria_valuenowAttr) {
        // If the valuenow attribute changes, AX clients need to be notified.
        document()->axObjectCache()->postNotification(renderer(), AXObjectCache::AXValueChanged, true);
    } else if (attrName == aria_labelAttr || attrName == aria_labeledbyAttr || attrName == altAttr || attrName == titleAttr) {
        // If the content of an element changes due to an attribute change, notify accessibility.
        document()->axObjectCache()->contentChanged(renderer());
    } else if (attrName == aria_selectedAttr)
        document()->axObjectCache()->selectedChildrenChanged(renderer());
    else if (attrName == aria_expandedAttr)
        document()->axObjectCache()->handleAriaExpandedChange(renderer());
    else if (attrName == aria_hiddenAttr)
        document()->axObjectCache()->childrenChanged(renderer());
    else if (attrName == aria_invalidAttr)
        document()->axObjectCache()->postNotification(renderer(), AXObjectCache::AXInvalidStatusChanged, true);
}
    
void Element::recalcStyleIfNeededAfterAttributeChanged(Attribute* attr)
{
    if (document()->attached() && document()->styleSelector()->hasSelectorForAttribute(attr->name().localName()))
        setNeedsStyleRecalc();
}

void Element::idAttributeChanged(Attribute* attr)
{
    setHasID(!attr->isNull());
    if (attributeMap()) {
        if (attr->isNull())
            attributeMap()->setIdForStyleResolution(nullAtom);
        else if (document()->inQuirksMode())
            attributeMap()->setIdForStyleResolution(attr->value().lower());
        else
            attributeMap()->setIdForStyleResolution(attr->value());
    }
    setNeedsStyleRecalc();
}
    
// Returns true is the given attribute is an event handler.
// We consider an event handler any attribute that begins with "on".
// It is a simple solution that has the advantage of not requiring any
// code or configuration change if a new event handler is defined.

static bool isEventHandlerAttribute(const QualifiedName& name)
{
    return name.namespaceURI().isNull() && name.localName().startsWith("on");
}

static bool isAttributeToRemove(const QualifiedName& name, const AtomicString& value)
{    
    return (name.localName().endsWith(hrefAttr.localName()) || name == srcAttr || name == actionAttr) && protocolIsJavaScript(stripLeadingAndTrailingHTMLSpaces(value));       
}

void Element::setAttributeMap(PassRefPtr<NamedNodeMap> list, FragmentScriptingPermission scriptingPermission)
{
    document()->incDOMTreeVersion();

    // If setting the whole map changes the id attribute, we need to call updateId.

    const QualifiedName& idName = document()->idAttributeName();
    Attribute* oldId = m_attributeMap ? m_attributeMap->getAttributeItem(idName) : 0;
    Attribute* newId = list ? list->getAttributeItem(idName) : 0;

    if (oldId || newId)
        updateId(oldId ? oldId->value() : nullAtom, newId ? newId->value() : nullAtom);

    if (m_attributeMap)
        m_attributeMap->m_element = 0;

    m_attributeMap = list;

    if (m_attributeMap) {
        m_attributeMap->m_element = this;
        // If the element is created as result of a paste or drag-n-drop operation
        // we want to remove all the script and event handlers.
        if (scriptingPermission == FragmentScriptingNotAllowed) {
            unsigned i = 0;
            while (i < m_attributeMap->length()) {
                const QualifiedName& attributeName = m_attributeMap->m_attributes[i]->name();
                if (isEventHandlerAttribute(attributeName)) {
                    m_attributeMap->m_attributes.remove(i);
                    continue;
                }

                if (isAttributeToRemove(attributeName, m_attributeMap->m_attributes[i]->value()))
                    m_attributeMap->m_attributes[i]->setValue(nullAtom);
                i++;
            }
        }
        // Store the set of attributes that changed on the stack in case
        // attributeChanged mutates m_attributeMap.
        Vector<RefPtr<Attribute> > attributes;
        m_attributeMap->copyAttributesToVector(attributes);
        for (Vector<RefPtr<Attribute> >::iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
            attributeChanged(iter->get());
        // FIXME: What about attributes that were in the old map that are not in the new map?
    }
}

bool Element::hasAttributes() const
{
    if (!isStyleAttributeValid())
        updateStyleAttribute();

#if ENABLE(SVG)
    if (!areSVGAttributesValid())
        updateAnimatedSVGAttribute(anyQName());
#endif

    return m_attributeMap && m_attributeMap->length();
}

String Element::nodeName() const
{
    return m_tagName.toString();
}

String Element::nodeNamePreservingCase() const
{
    return m_tagName.toString();
}

void Element::setPrefix(const AtomicString& prefix, ExceptionCode& ec)
{
    ec = 0;
    checkSetPrefix(prefix, ec);
    if (ec)
        return;

    m_tagName.setPrefix(prefix.isEmpty() ? AtomicString() : prefix);
}

KURL Element::baseURI() const
{
    const AtomicString& baseAttribute = getAttribute(baseAttr);
    KURL base(KURL(), baseAttribute);
    if (!base.protocol().isEmpty())
        return base;

    ContainerNode* parent = parentNode();
    if (!parent)
        return base;

    const KURL& parentBase = parent->baseURI();
    if (parentBase.isNull())
        return base;

    return KURL(parentBase, baseAttribute);
}

void Element::createAttributeMap() const
{
    m_attributeMap = NamedNodeMap::create(const_cast<Element*>(this));
}

bool Element::isURLAttribute(Attribute*) const
{
    return false;
}

const QualifiedName& Element::imageSourceAttributeName() const
{
    return srcAttr;
}

RenderObject* Element::createRenderer(RenderArena* arena, RenderStyle* style)
{
    if (document()->documentElement() == this && style->display() == NONE) {
        // Ignore display: none on root elements.  Force a display of block in that case.
        RenderBlock* result = new (arena) RenderBlock(this);
        if (result)
            result->setAnimatableStyle(style);
        return result;
    }
    return RenderObject::createObject(this, style);
}

bool Element::wasChangedSinceLastFormControlChangeEvent() const
{
    return false;
}

void Element::setChangedSinceLastFormControlChangeEvent(bool)
{
}

void Element::insertedIntoDocument()
{
    // need to do superclass processing first so inDocument() is true
    // by the time we reach updateId
    ContainerNode::insertedIntoDocument();
    if (ShadowRoot* shadow = shadowRoot())
        shadow->insertedIntoDocument();

    if (hasID()) {
        if (m_attributeMap) {
            Attribute* idItem = m_attributeMap->getAttributeItem(document()->idAttributeName());
            if (idItem && !idItem->isNull())
                updateId(nullAtom, idItem->value());
        }
    }
}

void Element::removedFromDocument()
{
    if (hasID()) {
        if (m_attributeMap) {
            Attribute* idItem = m_attributeMap->getAttributeItem(document()->idAttributeName());
            if (idItem && !idItem->isNull())
                updateId(idItem->value(), nullAtom);
        }
    }

    ContainerNode::removedFromDocument();
    if (ShadowRoot* shadow = shadowRoot())
        shadow->removedFromDocument();
}

void Element::insertedIntoTree(bool deep)
{
    ContainerNode::insertedIntoTree(deep);
    if (!deep)
        return;
    if (ShadowRoot* shadow = shadowRoot())
        shadow->insertedIntoTree(true);
}

void Element::removedFromTree(bool deep)
{
    ContainerNode::removedFromTree(deep);
    if (!deep)
        return;
    if (ShadowRoot* shadow = shadowRoot())
        shadow->removedFromTree(true);
}

void Element::attach()
{
    suspendPostAttachCallbacks();
    RenderWidget::suspendWidgetHierarchyUpdates();

    createRendererIfNeeded();
    
    StyleSelectorParentPusher parentPusher(this);

    if (ShadowRoot* shadow = shadowRoot()) {
        parentPusher.push();
        shadow->attach();
    }

    if (firstChild())
        parentPusher.push();
    ContainerNode::attach();

    if (hasRareData()) {   
        ElementRareData* data = rareData();
        if (data->needsFocusAppearanceUpdateSoonAfterAttach()) {
            if (isFocusable() && document()->focusedNode() == this)
                document()->updateFocusAppearanceSoon(false /* don't restore selection */);
            data->setNeedsFocusAppearanceUpdateSoonAfterAttach(false);
        }
    }

    RenderWidget::resumeWidgetHierarchyUpdates();
    resumePostAttachCallbacks();
}

void Element::detach()
{
    RenderWidget::suspendWidgetHierarchyUpdates();

    cancelFocusAppearanceUpdate();
    if (hasRareData())
        rareData()->resetComputedStyle();
    ContainerNode::detach();
    if (ShadowRoot* shadow = shadowRoot())
        shadow->detach();

    RenderWidget::resumeWidgetHierarchyUpdates();
}

bool Element::pseudoStyleCacheIsInvalid(const RenderStyle* currentStyle, RenderStyle* newStyle)
{
    ASSERT(currentStyle == renderStyle());

    if (!renderer() || !currentStyle)
        return false;

    const PseudoStyleCache* pseudoStyleCache = currentStyle->cachedPseudoStyles();
    if (!pseudoStyleCache)
        return false;

    size_t cacheSize = pseudoStyleCache->size();
    for (size_t i = 0; i < cacheSize; ++i) {
        RefPtr<RenderStyle> newPseudoStyle;
        PseudoId pseudoId = pseudoStyleCache->at(i)->styleType();
        if (pseudoId == VISITED_LINK) {
            newPseudoStyle =  newStyle->getCachedPseudoStyle(VISITED_LINK); // This pseudo-style was aggressively computed already when we first called styleForElement on the new style.
            if (!newPseudoStyle || *newPseudoStyle != *pseudoStyleCache->at(i))
                return true;
        } else if (pseudoId == FIRST_LINE || pseudoId == FIRST_LINE_INHERITED)
            newPseudoStyle = renderer()->uncachedFirstLineStyle(newStyle);
        else
            newPseudoStyle = renderer()->getUncachedPseudoStyle(pseudoId, newStyle, newStyle);
        if (!newPseudoStyle)
            return true;
        if (*newPseudoStyle != *pseudoStyleCache->at(i)) {
            if (pseudoId < FIRST_INTERNAL_PSEUDOID)
                newStyle->setHasPseudoStyle(pseudoId);
            newStyle->addCachedPseudoStyle(newPseudoStyle);
            if (pseudoId == FIRST_LINE || pseudoId == FIRST_LINE_INHERITED) {
                // FIXME: We should do an actual diff to determine whether a repaint vs. layout
                // is needed, but for now just assume a layout will be required.  The diff code
                // in RenderObject::setStyle would need to be factored out so that it could be reused.
                renderer()->setNeedsLayoutAndPrefWidthsRecalc();
            }
            return true;
        }
    }
    return false;
}

void Element::recalcStyle(StyleChange change)
{
    // Ref currentStyle in case it would otherwise be deleted when setRenderStyle() is called.
    RefPtr<RenderStyle> currentStyle(renderStyle());
    bool hasParentStyle = parentNodeForRenderingAndStyle() ? parentNodeForRenderingAndStyle()->renderStyle() : false;
    bool hasDirectAdjacentRules = currentStyle && currentStyle->childrenAffectedByDirectAdjacentRules();
    bool hasIndirectAdjacentRules = currentStyle && currentStyle->childrenAffectedByForwardPositionalRules();

    if ((change > NoChange || needsStyleRecalc())) {
        if (hasRareData())
            rareData()->resetComputedStyle();
    }
    if (hasParentStyle && (change >= Inherit || needsStyleRecalc())) {
        RefPtr<RenderStyle> newStyle = document()->styleSelector()->styleForElement(this);
        StyleChange ch = diff(currentStyle.get(), newStyle.get());
        if (ch == Detach || !currentStyle) {
            if (attached())
                detach();
            attach(); // FIXME: The style gets computed twice by calling attach. We could do better if we passed the style along.
            // attach recalulates the style for all children. No need to do it twice.
            clearNeedsStyleRecalc();
            clearChildNeedsStyleRecalc();
            return;
        }

        if (currentStyle) {
            // Preserve "affected by" bits that were propagated to us from descendants in the case where we didn't do a full
            // style change (e.g., only inline style changed).
            if (currentStyle->affectedByHoverRules())
                newStyle->setAffectedByHoverRules(true);
            if (currentStyle->affectedByActiveRules())
                newStyle->setAffectedByActiveRules(true);
            if (currentStyle->affectedByDragRules())
                newStyle->setAffectedByDragRules(true);
            if (currentStyle->childrenAffectedByForwardPositionalRules())
                newStyle->setChildrenAffectedByForwardPositionalRules();
            if (currentStyle->childrenAffectedByBackwardPositionalRules())
                newStyle->setChildrenAffectedByBackwardPositionalRules();
            if (currentStyle->childrenAffectedByFirstChildRules())
                newStyle->setChildrenAffectedByFirstChildRules();
            if (currentStyle->childrenAffectedByLastChildRules())
                newStyle->setChildrenAffectedByLastChildRules();
            if (currentStyle->childrenAffectedByDirectAdjacentRules())
                newStyle->setChildrenAffectedByDirectAdjacentRules();
        }

        if (ch != NoChange || pseudoStyleCacheIsInvalid(currentStyle.get(), newStyle.get()) || (change == Force && renderer() && renderer()->requiresForcedStyleRecalcPropagation())) {
            setRenderStyle(newStyle);
        } else if (needsStyleRecalc() && styleChangeType() != SyntheticStyleChange) {
            // Although no change occurred, we use the new style so that the cousin style sharing code won't get
            // fooled into believing this style is the same.
            if (renderer())
                renderer()->setStyleInternal(newStyle.get());
            else
                setRenderStyle(newStyle);
        } else if (styleChangeType() == SyntheticStyleChange)
             setRenderStyle(newStyle);

        if (change != Force) {
            // If "rem" units are used anywhere in the document, and if the document element's font size changes, then go ahead and force font updating
            // all the way down the tree.  This is simpler than having to maintain a cache of objects (and such font size changes should be rare anyway).
            if (document()->usesRemUnits() && ch != NoChange && currentStyle && newStyle && currentStyle->fontSize() != newStyle->fontSize() && document()->documentElement() == this)
                change = Force;
            else if (styleChangeType() >= FullStyleChange)
                change = Force;
            else
                change = ch;
        }
    }
    StyleSelectorParentPusher parentPusher(this);
    // FIXME: This check is good enough for :hover + foo, but it is not good enough for :hover + foo + bar.
    // For now we will just worry about the common case, since it's a lot trickier to get the second case right
    // without doing way too much re-resolution.
    bool forceCheckOfNextElementSibling = false;
    bool forceCheckOfAnyElementSibling = false;
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        bool childRulesChanged = n->needsStyleRecalc() && n->styleChangeType() == FullStyleChange;
        if ((forceCheckOfNextElementSibling || forceCheckOfAnyElementSibling) && n->isElementNode())
            n->setNeedsStyleRecalc();
        if (change >= Inherit || n->isTextNode() || n->childNeedsStyleRecalc() || n->needsStyleRecalc()) {
            parentPusher.push();
            n->recalcStyle(change);
        }
        if (n->isElementNode()) {
            forceCheckOfNextElementSibling = childRulesChanged && hasDirectAdjacentRules;
            forceCheckOfAnyElementSibling = forceCheckOfAnyElementSibling || (childRulesChanged && hasIndirectAdjacentRules);
        }
    }
    // FIXME: This does not care about sibling combinators. Will be necessary in XBL2 world.
    if (ShadowRoot* shadow = shadowRoot()) {
        if (change >= Inherit || shadow->childNeedsStyleRecalc() || shadow->needsStyleRecalc()) {
            parentPusher.push();
            shadow->recalcStyle(change);
        }
    }

    clearNeedsStyleRecalc();
    clearChildNeedsStyleRecalc();
}

ShadowRoot* Element::shadowRoot() const
{
    return hasRareData() ? rareData()->m_shadowRoot : 0;
}

ShadowRoot* Element::ensureShadowRoot()
{
    if (ShadowRoot* existingRoot = shadowRoot())
        return existingRoot;

    RefPtr<ShadowRoot> newRoot = ShadowRoot::create(document());
    ensureRareData()->m_shadowRoot = newRoot.get();
    InspectorInstrumentation::willInsertDOMNode(document(), newRoot.get(), this);
    newRoot->setShadowHost(this);
    if (inDocument())
        newRoot->insertedIntoDocument();
    if (attached())
        newRoot->lazyAttach();
    InspectorInstrumentation::didInsertDOMNode(document(), newRoot.get());
    return newRoot.get();
}

void Element::removeShadowRoot()
{
    if (!hasRareData())
        return;

    ElementRareData* data = rareData();
    if (RefPtr<Node> oldRoot = data->m_shadowRoot) {
        InspectorInstrumentation::willRemoveDOMNode(document(), this);
        data->m_shadowRoot = 0;
        document()->removeFocusedNodeOfSubtree(oldRoot.get());

        // Remove from rendering tree
        if (oldRoot->attached())
            oldRoot->detach();

        oldRoot->setShadowHost(0);
        oldRoot->setTreeScopeRecursively(document());
        if (oldRoot->inDocument())
            oldRoot->removedFromDocument();
        else
            oldRoot->removedFromTree(true);
    }
}

bool Element::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ELEMENT_NODE:
    case TEXT_NODE:
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case CDATA_SECTION_NODE:
    case ENTITY_REFERENCE_NODE:
        return true;
    default:
        break;
    }
    return false;
}

static void checkForEmptyStyleChange(Element* element, RenderStyle* style)
{
    if (!style)
        return;

    if (style->affectedByEmpty() && (!style->emptyState() || element->hasChildNodes()))
        element->setNeedsStyleRecalc();
}

static void checkForSiblingStyleChanges(Element* e, RenderStyle* style, bool finishedParsingCallback,
                                        Node* beforeChange, Node* afterChange, int childCountDelta)
{
    if (!style || (e->needsStyleRecalc() && style->childrenAffectedByPositionalRules()))
        return;

    // :first-child.  In the parser callback case, we don't have to check anything, since we were right the first time.
    // In the DOM case, we only need to do something if |afterChange| is not 0.
    // |afterChange| is 0 in the parser case, so it works out that we'll skip this block.
    if (style->childrenAffectedByFirstChildRules() && afterChange) {
        // Find our new first child.
        Node* newFirstChild = 0;
        for (newFirstChild = e->firstChild(); newFirstChild && !newFirstChild->isElementNode(); newFirstChild = newFirstChild->nextSibling()) {};
        
        // Find the first element node following |afterChange|
        Node* firstElementAfterInsertion = 0;
        for (firstElementAfterInsertion = afterChange;
             firstElementAfterInsertion && !firstElementAfterInsertion->isElementNode();
             firstElementAfterInsertion = firstElementAfterInsertion->nextSibling()) {};
        
        // This is the insert/append case.
        if (newFirstChild != firstElementAfterInsertion && firstElementAfterInsertion && firstElementAfterInsertion->attached() &&
            firstElementAfterInsertion->renderStyle() && firstElementAfterInsertion->renderStyle()->firstChildState())
            firstElementAfterInsertion->setNeedsStyleRecalc();
            
        // We also have to handle node removal.
        if (childCountDelta < 0 && newFirstChild == firstElementAfterInsertion && newFirstChild && newFirstChild->renderStyle() && !newFirstChild->renderStyle()->firstChildState())
            newFirstChild->setNeedsStyleRecalc();
    }

    // :last-child.  In the parser callback case, we don't have to check anything, since we were right the first time.
    // In the DOM case, we only need to do something if |afterChange| is not 0.
    if (style->childrenAffectedByLastChildRules() && beforeChange) {
        // Find our new last child.
        Node* newLastChild = 0;
        for (newLastChild = e->lastChild(); newLastChild && !newLastChild->isElementNode(); newLastChild = newLastChild->previousSibling()) {};
        
        // Find the last element node going backwards from |beforeChange|
        Node* lastElementBeforeInsertion = 0;
        for (lastElementBeforeInsertion = beforeChange;
             lastElementBeforeInsertion && !lastElementBeforeInsertion->isElementNode();
             lastElementBeforeInsertion = lastElementBeforeInsertion->previousSibling()) {};
        
        if (newLastChild != lastElementBeforeInsertion && lastElementBeforeInsertion && lastElementBeforeInsertion->attached() &&
            lastElementBeforeInsertion->renderStyle() && lastElementBeforeInsertion->renderStyle()->lastChildState())
            lastElementBeforeInsertion->setNeedsStyleRecalc();
            
        // We also have to handle node removal.  The parser callback case is similar to node removal as well in that we need to change the last child
        // to match now.
        if ((childCountDelta < 0 || finishedParsingCallback) && newLastChild == lastElementBeforeInsertion && newLastChild && newLastChild->renderStyle() && !newLastChild->renderStyle()->lastChildState())
            newLastChild->setNeedsStyleRecalc();
    }

    // The + selector.  We need to invalidate the first element following the insertion point.  It is the only possible element
    // that could be affected by this DOM change.
    if (style->childrenAffectedByDirectAdjacentRules() && afterChange) {
        Node* firstElementAfterInsertion = 0;
        for (firstElementAfterInsertion = afterChange;
             firstElementAfterInsertion && !firstElementAfterInsertion->isElementNode();
             firstElementAfterInsertion = firstElementAfterInsertion->nextSibling()) {};
        if (firstElementAfterInsertion && firstElementAfterInsertion->attached())
            firstElementAfterInsertion->setNeedsStyleRecalc();
    }

    // Forward positional selectors include the ~ selector, nth-child, nth-of-type, first-of-type and only-of-type.
    // Backward positional selectors include nth-last-child, nth-last-of-type, last-of-type and only-of-type.
    // We have to invalidate everything following the insertion point in the forward case, and everything before the insertion point in the
    // backward case.
    // |afterChange| is 0 in the parser callback case, so we won't do any work for the forward case if we don't have to.
    // For performance reasons we just mark the parent node as changed, since we don't want to make childrenChanged O(n^2) by crawling all our kids
    // here.  recalcStyle will then force a walk of the children when it sees that this has happened.
    if ((style->childrenAffectedByForwardPositionalRules() && afterChange) ||
        (style->childrenAffectedByBackwardPositionalRules() && beforeChange))
        e->setNeedsStyleRecalc();
    
    // :empty selector.
    checkForEmptyStyleChange(e, style);
}

void Element::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    ContainerNode::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    if (changedByParser)
        checkForEmptyStyleChange(this, renderStyle());
    else
        checkForSiblingStyleChanges(this, renderStyle(), false, beforeChange, afterChange, childCountDelta);
    if (ShadowRoot* shadow = shadowRoot())
        shadow->hostChildrenChanged();
}

void Element::beginParsingChildren()
{
    clearIsParsingChildrenFinished();
    CSSStyleSelector* styleSelector = document()->styleSelectorIfExists();
    if (styleSelector && attached())
        styleSelector->pushParent(this);
}

void Element::finishParsingChildren()
{
    ContainerNode::finishParsingChildren();
    setIsParsingChildrenFinished();
    checkForSiblingStyleChanges(this, renderStyle(), true, lastChild(), 0, 0);
    if (CSSStyleSelector* styleSelector = document()->styleSelectorIfExists())
        styleSelector->popParent(this);
}

void Element::dispatchAttrRemovalEvent(Attribute*)
{
    ASSERT(!eventDispatchForbidden());

#if 0
    if (!document()->hasListenerType(Document::DOMATTRMODIFIED_LISTENER))
        return;
    ExceptionCode ec = 0;
    dispatchScopedEvent(MutationEvent::create(DOMAttrModifiedEvent, true, attr, attr->value(),
        attr->value(), document()->attrName(attr->id()), MutationEvent::REMOVAL), ec);
#endif
}

void Element::dispatchAttrAdditionEvent(Attribute*)
{
    ASSERT(!eventDispatchForbidden());

#if 0
    if (!document()->hasListenerType(Document::DOMATTRMODIFIED_LISTENER))
        return;
    ExceptionCode ec = 0;
    dispatchScopedEvent(MutationEvent::create(DOMAttrModifiedEvent, true, attr, attr->value(),
        attr->value(), document()->attrName(attr->id()), MutationEvent::ADDITION), ec);
#endif
}

String Element::openTagStartToString() const
{
    String result = "<" + nodeName();

    NamedNodeMap* attrMap = attributes(true);

    if (attrMap) {
        unsigned numAttrs = attrMap->length();
        for (unsigned i = 0; i < numAttrs; i++) {
            result += " ";

            Attribute *attribute = attrMap->attributeItem(i);
            result += attribute->name().toString();
            if (!attribute->value().isNull()) {
                result += "=\"";
                // FIXME: substitute entities for any instances of " or '
                result += attribute->value();
                result += "\"";
            }
        }
    }

    return result;
}

#ifndef NDEBUG
void Element::formatForDebugger(char* buffer, unsigned length) const
{
    String result;
    String s;
    
    s = nodeName();
    if (s.length() > 0) {
        result += s;
    }
          
    s = getIdAttribute();
    if (s.length() > 0) {
        if (result.length() > 0)
            result += "; ";
        result += "id=";
        result += s;
    }
          
    s = getAttribute(classAttr);
    if (s.length() > 0) {
        if (result.length() > 0)
            result += "; ";
        result += "class=";
        result += s;
    }
          
    strncpy(buffer, result.utf8().data(), length - 1);
}
#endif

PassRefPtr<Attr> Element::setAttributeNode(Attr* attr, ExceptionCode& ec)
{
    if (!attr) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }
    return static_pointer_cast<Attr>(attributes(false)->setNamedItem(attr, ec));
}

PassRefPtr<Attr> Element::setAttributeNodeNS(Attr* attr, ExceptionCode& ec)
{
    if (!attr) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }
    return static_pointer_cast<Attr>(attributes(false)->setNamedItem(attr, ec));
}

PassRefPtr<Attr> Element::removeAttributeNode(Attr* attr, ExceptionCode& ec)
{
    if (!attr) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }
    if (attr->ownerElement() != this) {
        ec = NOT_FOUND_ERR;
        return 0;
    }

    ASSERT(document() == attr->document());

    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return 0;

    return static_pointer_cast<Attr>(attrs->removeNamedItem(attr->qualifiedName(), ec));
}

void Element::setAttributeNS(const AtomicString& namespaceURI, const AtomicString& qualifiedName, const AtomicString& value, ExceptionCode& ec, FragmentScriptingPermission scriptingPermission)
{
    String prefix, localName;
    if (!Document::parseQualifiedName(qualifiedName, prefix, localName, ec))
        return;

    if (namespaceURI.isNull() && !prefix.isNull()) {
        ec = NAMESPACE_ERR;
        return;
    }

    QualifiedName qName(prefix, localName, namespaceURI);

    if (scriptingPermission == FragmentScriptingNotAllowed && (isEventHandlerAttribute(qName) || isAttributeToRemove(qName, value)))
        return;

    setAttribute(qName, value, ec);
}

void Element::removeAttribute(const String& name, ExceptionCode& ec)
{
    InspectorInstrumentation::willModifyDOMAttr(document(), this);

    String localName = shouldIgnoreAttributeCase(this) ? name.lower() : name;

    if (m_attributeMap) {
        m_attributeMap->removeNamedItem(localName, ec);
        if (ec == NOT_FOUND_ERR)
            ec = 0;
    }
    
    InspectorInstrumentation::didModifyDOMAttr(document(), this);
}

void Element::removeAttributeNS(const String& namespaceURI, const String& localName, ExceptionCode& ec)
{
    removeAttribute(QualifiedName(nullAtom, localName, namespaceURI), ec);
}

PassRefPtr<Attr> Element::getAttributeNode(const String& name)
{
    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return 0;
    String localName = shouldIgnoreAttributeCase(this) ? name.lower() : name;
    return static_pointer_cast<Attr>(attrs->getNamedItem(localName));
}

PassRefPtr<Attr> Element::getAttributeNodeNS(const String& namespaceURI, const String& localName)
{
    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return 0;
    return static_pointer_cast<Attr>(attrs->getNamedItem(QualifiedName(nullAtom, localName, namespaceURI)));
}

bool Element::hasAttribute(const String& name) const
{
    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return false;

    // This call to String::lower() seems to be required but
    // there may be a way to remove it.
    String localName = shouldIgnoreAttributeCase(this) ? name.lower() : name;
    return attrs->getAttributeItem(localName, false);
}

bool Element::hasAttributeNS(const String& namespaceURI, const String& localName) const
{
    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return false;
    return attrs->getAttributeItem(QualifiedName(nullAtom, localName, namespaceURI));
}

CSSStyleDeclaration *Element::style()
{
    return 0;
}

void Element::focus(bool restorePreviousSelection)
{
    if (!inDocument())
        return;

    Document* doc = document();
    if (doc->focusedNode() == this)
        return;

    // If the stylesheets have already been loaded we can reliably check isFocusable.
    // If not, we continue and set the focused node on the focus controller below so
    // that it can be updated soon after attach. 
    if (doc->haveStylesheetsLoaded()) {
        doc->updateLayoutIgnorePendingStylesheets();
        if (!isFocusable())
            return;
    }

    if (!supportsFocus())
        return;

    RefPtr<Node> protect;
    if (Page* page = doc->page()) {
        // Focus and change event handlers can cause us to lose our last ref.
        // If a focus event handler changes the focus to a different node it
        // does not make sense to continue and update appearence.
        protect = this;
        if (!page->focusController()->setFocusedNode(this, doc->frame()))
            return;
    }

    // Setting the focused node above might have invalidated the layout due to scripts.
    doc->updateLayoutIgnorePendingStylesheets();

    if (!isFocusable()) {
        ensureRareData()->setNeedsFocusAppearanceUpdateSoonAfterAttach(true);
        return;
    }
        
    cancelFocusAppearanceUpdate();
    updateFocusAppearance(restorePreviousSelection);
}

void Element::updateFocusAppearance(bool /*restorePreviousSelection*/)
{
    if (this == rootEditableElement()) { 
        Frame* frame = document()->frame();
        if (!frame)
            return;
        
        // When focusing an editable element in an iframe, don't reset the selection if it already contains a selection.
        if (this == frame->selection()->rootEditableElement())
            return;

        // FIXME: We should restore the previous selection if there is one.
        VisibleSelection newSelection = VisibleSelection(firstPositionInOrBeforeNode(this), DOWNSTREAM);
        
        if (frame->selection()->shouldChangeSelection(newSelection)) {
            frame->selection()->setSelection(newSelection);
            frame->selection()->revealSelection();
        }
    } else if (renderer() && !renderer()->isWidget())
        renderer()->enclosingLayer()->scrollRectToVisible(getRect());
}

void Element::blur()
{
    cancelFocusAppearanceUpdate();
    Document* doc = document();
    if (doc->focusedNode() == this) {
        if (doc->frame())
            doc->frame()->page()->focusController()->setFocusedNode(0, doc->frame());
        else
            doc->setFocusedNode(0);
    }
}

String Element::innerText() const
{
    // We need to update layout, since plainText uses line boxes in the render tree.
    document()->updateLayoutIgnorePendingStylesheets();

    if (!renderer())
        return textContent(true);

    return plainText(rangeOfContents(const_cast<Element*>(this)).get());
}

String Element::outerText() const
{
    // Getting outerText is the same as getting innerText, only
    // setting is different. You would think this should get the plain
    // text for the outer range, but this is wrong, <br> for instance
    // would return different values for inner and outer text by such
    // a rule, but it doesn't in WinIE, and we want to match that.
    return innerText();
}

String Element::title() const
{
    return String();
}

IntSize Element::minimumSizeForResizing() const
{
    return hasRareData() ? rareData()->m_minimumSizeForResizing : defaultMinimumSizeForResizing();
}

void Element::setMinimumSizeForResizing(const IntSize& size)
{
    if (size == defaultMinimumSizeForResizing() && !hasRareData())
        return;
    ensureRareData()->m_minimumSizeForResizing = size;
}

RenderStyle* Element::computedStyle(PseudoId pseudoElementSpecifier)
{
    // FIXME: Find and use the renderer from the pseudo element instead of the actual element so that the 'length'
    // properties, which are only known by the renderer because it did the layout, will be correct and so that the
    // values returned for the ":selection" pseudo-element will be correct.
    if (RenderStyle* usedStyle = renderStyle())
        return pseudoElementSpecifier ? usedStyle->getCachedPseudoStyle(pseudoElementSpecifier) : usedStyle;

    if (!attached())
        // FIXME: Try to do better than this. Ensure that styleForElement() works for elements that are not in the
        // document tree and figure out when to destroy the computed style for such elements.
        return 0;

    ElementRareData* data = ensureRareData();
    if (!data->m_computedStyle)
        data->m_computedStyle = document()->styleForElementIgnoringPendingStylesheets(this);
    return pseudoElementSpecifier ? data->m_computedStyle->getCachedPseudoStyle(pseudoElementSpecifier) : data->m_computedStyle.get();
}

AtomicString Element::computeInheritedLanguage() const
{
    const Node* n = this;
    AtomicString value;
    // The language property is inherited, so we iterate over the parents to find the first language.
    while (n && value.isNull()) {
        if (n->isElementNode()) {
            // Spec: xml:lang takes precedence -- http://www.w3.org/TR/xhtml1/#C_7
            value = static_cast<const Element*>(n)->fastGetAttribute(XMLNames::langAttr);
            if (value.isNull())
                value = static_cast<const Element*>(n)->fastGetAttribute(HTMLNames::langAttr);
        } else if (n->isDocumentNode()) {
            // checking the MIME content-language
            value = static_cast<const Document*>(n)->contentLanguage();
        }

        n = n->parentNode();
    }

    return value;
}

void Element::cancelFocusAppearanceUpdate()
{
    if (hasRareData())
        rareData()->setNeedsFocusAppearanceUpdateSoonAfterAttach(false);
    if (document()->focusedNode() == this)
        document()->cancelFocusAppearanceUpdate();
}

void Element::normalizeAttributes()
{
    // Normalize attributes.
    NamedNodeMap* attrs = attributes(true);
    if (!attrs)
        return;

    if (attrs->isEmpty())
        return;

    Vector<RefPtr<Attribute> > attributeVector;
    attrs->copyAttributesToVector(attributeVector);
    size_t numAttrs = attributeVector.size();
    for (size_t i = 0; i < numAttrs; ++i) {
        if (Attr* attr = attributeVector[i]->attr())
            attr->normalize();
    }
}

// ElementTraversal API
Element* Element::firstElementChild() const
{
    return WebCore::firstElementChild(this);
}

Element* Element::lastElementChild() const
{
    Node* n = lastChild();
    while (n && !n->isElementNode())
        n = n->previousSibling();
    return static_cast<Element*>(n);
}

Element* Element::previousElementSibling() const
{
    Node* n = previousSibling();
    while (n && !n->isElementNode())
        n = n->previousSibling();
    return static_cast<Element*>(n);
}

Element* Element::nextElementSibling() const
{
    Node* n = nextSibling();
    while (n && !n->isElementNode())
        n = n->nextSibling();
    return static_cast<Element*>(n);
}

unsigned Element::childElementCount() const
{
    unsigned count = 0;
    Node* n = firstChild();
    while (n) {
        count += n->isElementNode();
        n = n->nextSibling();
    }
    return count;
}

bool Element::webkitMatchesSelector(const String& selector, ExceptionCode& ec)
{
    if (selector.isEmpty()) {
        ec = SYNTAX_ERR;
        return false;
    }

    bool strictParsing = !document()->inQuirksMode();
    CSSParser p(strictParsing);

    CSSSelectorList selectorList;
    p.parseSelector(selector, document(), selectorList);

    if (!selectorList.first()) {
        ec = SYNTAX_ERR;
        return false;
    }

    // Throw a NAMESPACE_ERR if the selector includes any namespace prefixes.
    if (selectorList.selectorsNeedNamespaceResolution()) {
        ec = NAMESPACE_ERR;
        return false;
    }

    CSSStyleSelector::SelectorChecker selectorChecker(document(), strictParsing);
    for (CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector)) {
        if (selectorChecker.checkSelector(selector, this))
            return true;
    }

    return false;
}

DOMTokenList* Element::classList()
{
    ElementRareData* data = ensureRareData();
    if (!data->m_classList)
        data->m_classList = ClassList::create(this);
    return data->m_classList.get();
}

DOMTokenList* Element::optionalClassList() const
{
    if (!hasRareData())
        return 0;
    return rareData()->m_classList.get();
}

DOMStringMap* Element::dataset()
{
    ElementRareData* data = ensureRareData();
    if (!data->m_datasetDOMStringMap)
        data->m_datasetDOMStringMap = DatasetDOMStringMap::create(this);
    return data->m_datasetDOMStringMap.get();
}

KURL Element::getURLAttribute(const QualifiedName& name) const
{
#if !ASSERT_DISABLED
    if (m_attributeMap) {
        if (Attribute* attribute = m_attributeMap->getAttributeItem(name))
            ASSERT(isURLAttribute(attribute));
    }
#endif
    return document()->completeURL(stripLeadingAndTrailingHTMLSpaces(getAttribute(name)));
}

KURL Element::getNonEmptyURLAttribute(const QualifiedName& name) const
{
#if !ASSERT_DISABLED
    if (m_attributeMap) {
        if (Attribute* attribute = m_attributeMap->getAttributeItem(name))
            ASSERT(isURLAttribute(attribute));
    }
#endif
    String value = stripLeadingAndTrailingHTMLSpaces(getAttribute(name));
    if (value.isEmpty())
        return KURL();
    return document()->completeURL(value);
}

int Element::getIntegralAttribute(const QualifiedName& attributeName) const
{
    return getAttribute(attributeName).string().toInt();
}

void Element::setIntegralAttribute(const QualifiedName& attributeName, int value)
{
    // FIXME: Need an AtomicString version of String::number.
    ExceptionCode ec;
    setAttribute(attributeName, String::number(value), ec);
}

unsigned Element::getUnsignedIntegralAttribute(const QualifiedName& attributeName) const
{
    return getAttribute(attributeName).string().toUInt();
}

void Element::setUnsignedIntegralAttribute(const QualifiedName& attributeName, unsigned value)
{
    // FIXME: Need an AtomicString version of String::number.
    ExceptionCode ec;
    setAttribute(attributeName, String::number(value), ec);
}

#if ENABLE(SVG)
bool Element::childShouldCreateRenderer(Node* child) const
{
    // Only create renderers for SVG elements whose parents are SVG elements, or for proper <svg xmlns="svgNS"> subdocuments.
    if (child->isSVGElement())
        return child->hasTagName(SVGNames::svgTag) || isSVGElement();

    return Node::childShouldCreateRenderer(child);
}
#endif
    
#if ENABLE(FULLSCREEN_API)
void Element::webkitRequestFullScreen(unsigned short flags)
{
    document()->requestFullScreenForElement(this, flags, Document::EnforceIFrameAllowFulScreenRequirement);
}
#endif    

SpellcheckAttributeState Element::spellcheckAttributeState() const
{
    if (!hasAttribute(HTMLNames::spellcheckAttr))
        return SpellcheckAttributeDefault;

    const AtomicString& value = getAttribute(HTMLNames::spellcheckAttr);
    if (equalIgnoringCase(value, "true") || equalIgnoringCase(value, ""))
        return SpellcheckAttributeTrue;
    if (equalIgnoringCase(value, "false"))
        return SpellcheckAttributeFalse;

    return SpellcheckAttributeDefault;
}

bool Element::isSpellCheckingEnabled() const
{
    const Element* element = this;
    while (element) {
        switch (element->spellcheckAttributeState()) {
        case SpellcheckAttributeTrue:
            return true;
        case SpellcheckAttributeFalse:
            return false;
        case SpellcheckAttributeDefault:
            break;
        }

        ContainerNode* parent = const_cast<Element*>(element)->parentOrHostNode();
        element = (parent && parent->isElementNode()) ? toElement(parent) : 0;
    }

    return true;
}

PassRefPtr<WebKitAnimationList> Element::webkitGetAnimations() const
{
    if (!renderer())
        return 0;

    AnimationController* animController = renderer()->animation();

    if (!animController)
        return 0;
    
    return animController->animationsForRenderer(renderer());
}

} // namespace WebCore
