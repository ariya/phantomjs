/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGTRefElement.h"

#include "ElementShadow.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "MutationEvent.h"
#include "NodeRenderingContext.h"
#include "RenderSVGInline.h"
#include "RenderSVGInlineText.h"
#include "RenderSVGResource.h"
#include "ShadowRoot.h"
#include "SVGDocument.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "StyleInheritedData.h"
#include "Text.h"
#include "XLinkNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGTRefElement, XLinkNames::hrefAttr, Href, href)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGTRefElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTextPositioningElement)
END_REGISTER_ANIMATED_PROPERTIES

PassRefPtr<SVGTRefElement> SVGTRefElement::create(const QualifiedName& tagName, Document* document)
{
    RefPtr<SVGTRefElement> element = adoptRef(new SVGTRefElement(tagName, document));
    element->ensureUserAgentShadowRoot();
    return element.release();
}

class SVGTRefTargetEventListener : public EventListener {
public:
    static PassRefPtr<SVGTRefTargetEventListener> create(SVGTRefElement* trefElement)
    {
        return adoptRef(new SVGTRefTargetEventListener(trefElement));
    }

    static const SVGTRefTargetEventListener* cast(const EventListener* listener)
    {
        return listener->type() == SVGTRefTargetEventListenerType
                ? static_cast<const SVGTRefTargetEventListener*>(listener) : 0;
    }

    void attach(PassRefPtr<Element> target);
    void detach();
    bool isAttached() const { return m_target.get(); }

private:
    SVGTRefTargetEventListener(SVGTRefElement* trefElement);

    virtual void handleEvent(ScriptExecutionContext*, Event*) OVERRIDE;
    virtual bool operator==(const EventListener&) OVERRIDE;

    SVGTRefElement* m_trefElement;
    RefPtr<Element> m_target;
};

SVGTRefTargetEventListener::SVGTRefTargetEventListener(SVGTRefElement* trefElement)
    : EventListener(SVGTRefTargetEventListenerType)
    , m_trefElement(trefElement)
    , m_target(0)
{
    ASSERT(m_trefElement);
}

void SVGTRefTargetEventListener::attach(PassRefPtr<Element> target)
{
    ASSERT(!isAttached());
    ASSERT(target.get());
    ASSERT(target->inDocument());

    target->addEventListener(eventNames().DOMSubtreeModifiedEvent, this, false);
    target->addEventListener(eventNames().DOMNodeRemovedFromDocumentEvent, this, false);
    m_target = target;
}

void SVGTRefTargetEventListener::detach()
{
    if (!isAttached())
        return;

    m_target->removeEventListener(eventNames().DOMSubtreeModifiedEvent, this, false);
    m_target->removeEventListener(eventNames().DOMNodeRemovedFromDocumentEvent, this, false);
    m_target.clear();
}

bool SVGTRefTargetEventListener::operator==(const EventListener& listener)
{
    if (const SVGTRefTargetEventListener* targetListener = SVGTRefTargetEventListener::cast(&listener))
        return m_trefElement == targetListener->m_trefElement;
    return false;
}

void SVGTRefTargetEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    ASSERT(isAttached());

    if (event->type() == eventNames().DOMSubtreeModifiedEvent && m_trefElement != event->target())
        m_trefElement->updateReferencedText(m_target.get());
    else if (event->type() == eventNames().DOMNodeRemovedFromDocumentEvent)
        m_trefElement->detachTarget();
}

inline SVGTRefElement::SVGTRefElement(const QualifiedName& tagName, Document* document)
    : SVGTextPositioningElement(tagName, document)
    , m_targetListener(SVGTRefTargetEventListener::create(this))
{
    ASSERT(hasTagName(SVGNames::trefTag));
    registerAnimatedPropertiesForSVGTRefElement();
}

SVGTRefElement::~SVGTRefElement()
{
    m_targetListener->detach();
}

void SVGTRefElement::updateReferencedText(Element* target)
{
    String textContent;
    if (target)
        textContent = target->textContent();

    ASSERT(shadow());
    ShadowRoot* root = shadow()->shadowRoot();
    if (!root->firstChild())
        root->appendChild(Text::create(document(), textContent), ASSERT_NO_EXCEPTION);
    else {
        ASSERT(root->firstChild()->isTextNode());
        root->firstChild()->setTextContent(textContent, ASSERT_NO_EXCEPTION);
    }
}

void SVGTRefElement::detachTarget()
{
    // Remove active listeners and clear the text content.
    m_targetListener->detach();

    String emptyContent;

    ASSERT(shadow());
    Node* container = shadow()->shadowRoot()->firstChild();
    if (container)
        container->setTextContent(emptyContent, IGNORE_EXCEPTION);

    if (!inDocument())
        return;

    // Mark the referenced ID as pending.
    String id;
    SVGURIReference::targetElementFromIRIString(href(), document(), &id);
    if (!id.isEmpty())
        document()->accessSVGExtensions()->addPendingResource(id, this);
}

bool SVGTRefElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty())
        SVGURIReference::addSupportedAttributes(supportedAttributes);
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGTRefElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGTextPositioningElement::parseAttribute(name, value);
        return;
    }

    if (SVGURIReference::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGTRefElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGTextPositioningElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (SVGURIReference::isKnownAttribute(attrName)) {
        buildPendingResource();
        if (RenderObject* renderer = this->renderer())
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    ASSERT_NOT_REACHED();
}

RenderObject* SVGTRefElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGInline(this);
}

bool SVGTRefElement::childShouldCreateRenderer(const NodeRenderingContext& childContext) const
{
    return childContext.node()->isInShadowTree();
}

bool SVGTRefElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (parentNode()
        && (parentNode()->hasTagName(SVGNames::aTag)
#if ENABLE(SVG_FONTS)
            || parentNode()->hasTagName(SVGNames::altGlyphTag)
#endif
            || parentNode()->hasTagName(SVGNames::textTag)
            || parentNode()->hasTagName(SVGNames::textPathTag)
            || parentNode()->hasTagName(SVGNames::tspanTag)))
        return StyledElement::rendererIsNeeded(context);

    return false;
}

void SVGTRefElement::buildPendingResource()
{
    // Remove any existing event listener.
    m_targetListener->detach();

    // If we're not yet in a document, this function will be called again from insertedInto().
    if (!inDocument())
        return;

    String id;
    RefPtr<Element> target = SVGURIReference::targetElementFromIRIString(href(), document(), &id);
    if (!target.get()) {
        if (id.isEmpty())
            return;

        document()->accessSVGExtensions()->addPendingResource(id, this);
        ASSERT(hasPendingResources());
        return;
    }

    // Don't set up event listeners if this is a shadow tree node.
    // SVGUseElement::transferEventListenersToShadowTree() handles this task, and addEventListener()
    // expects every element instance to have an associated shadow tree element - which is not the
    // case when we land here from SVGUseElement::buildShadowTree().
    if (!isInShadowTree())
        m_targetListener->attach(target);

    updateReferencedText(target.get());
}

Node::InsertionNotificationRequest SVGTRefElement::insertedInto(ContainerNode* rootParent)
{
    SVGStyledElement::insertedInto(rootParent);
    if (rootParent->inDocument())
        buildPendingResource();
    return InsertionDone;
}

void SVGTRefElement::removedFrom(ContainerNode* rootParent)
{
    SVGStyledElement::removedFrom(rootParent);
    if (rootParent->inDocument())
        m_targetListener->detach();
}

}

#endif // ENABLE(SVG)
