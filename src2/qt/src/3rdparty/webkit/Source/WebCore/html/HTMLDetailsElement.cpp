/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#include "config.h"
#include "HTMLDetailsElement.h"

#if ENABLE(DETAILS)

#include "HTMLNames.h"
#include "HTMLSummaryElement.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "RenderDetails.h"
#include "ShadowContentElement.h"
#include "ShadowRoot.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

class DetailsContentElement : public ShadowContentElement {
public:
    static PassRefPtr<DetailsContentElement> create(Document*);

private:
    DetailsContentElement(Document* document)
        : ShadowContentElement(document)
    {
    }

    virtual bool shouldInclude(Node*);
};

PassRefPtr<DetailsContentElement> DetailsContentElement::create(Document* document)
{
    return adoptRef(new DetailsContentElement(document));
}

bool DetailsContentElement::shouldInclude(Node* node)
{
    HTMLDetailsElement* details = static_cast<HTMLDetailsElement*>(shadowAncestorNode());
    return details->mainSummary() != node;
}


class DetailsSummaryElement : public ShadowContentElement {
public:
    static PassRefPtr<DetailsSummaryElement> create(Document*);

private:
    DetailsSummaryElement(Document* document)
        : ShadowContentElement(document)
    {
    }

    virtual bool shouldInclude(Node*);
};

PassRefPtr<DetailsSummaryElement> DetailsSummaryElement::create(Document* document)
{
    return adoptRef(new DetailsSummaryElement(document));
}

bool DetailsSummaryElement::shouldInclude(Node* node)
{
    HTMLDetailsElement* details = static_cast<HTMLDetailsElement*>(shadowAncestorNode());
    return details->mainSummary() == node;
}


PassRefPtr<HTMLDetailsElement> HTMLDetailsElement::create(const QualifiedName& tagName, Document* document)
{
    RefPtr<HTMLDetailsElement> result = adoptRef(new HTMLDetailsElement(tagName, document));
    result->ensureShadowSubtreeOf(ForwardingSummary);
    return result;
}

HTMLDetailsElement::HTMLDetailsElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_summaryType(NoSummary)
    , m_mainSummary(0)
    , m_isOpen(false)
{
    ASSERT(hasTagName(detailsTag));
}

RenderObject* HTMLDetailsElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderDetails(this);
}

void HTMLDetailsElement::ensureShadowSubtreeOf(SummaryType type)
{
    if (type == m_summaryType)
        return;
    m_summaryType = type;
    removeShadowRoot();
    createShadowSubtree();
}

static Node* findSummaryFor(PassRefPtr<ContainerNode> container)
{
    for (Node* child = container->firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(summaryTag))
            return child;
    }

    return 0;
}

Node* HTMLDetailsElement::ensureMainSummary()
{
    Node* summary = findSummaryFor(this);
    if (summary) {
        ensureShadowSubtreeOf(ForwardingSummary);
        return summary;
    }

    ensureShadowSubtreeOf(DefaultSummary);
    return findSummaryFor(shadowRoot());
}

void HTMLDetailsElement::refreshMainSummary(RefreshRenderer refreshRenderer)
{
    RefPtr<Node> oldSummary = m_mainSummary;
    m_mainSummary = ensureMainSummary();

    if (oldSummary == m_mainSummary || !attached())
        return;

    if (oldSummary && oldSummary->parentNodeForRenderingAndStyle()) {
        oldSummary->detach();
        oldSummary->attach();
    }
        
    if (m_mainSummary && refreshRenderer == RefreshRendererAllowed) {
        m_mainSummary->detach();
        m_mainSummary->attach();
    }
}

void HTMLDetailsElement::createShadowSubtree()
{
    ASSERT(!shadowRoot());
    ExceptionCode ec = 0;
    if (m_summaryType == DefaultSummary) {
        RefPtr<HTMLSummaryElement> defaultSummary = HTMLSummaryElement::create(summaryTag, document());
        defaultSummary->appendChild(Text::create(document(), defaultDetailsSummaryText()), ec);
        ensureShadowRoot()->appendChild(defaultSummary, ec, true);
        ensureShadowRoot()->appendChild(DetailsContentElement::create(document()), ec, true);
    } else {
        ASSERT(m_summaryType == ForwardingSummary);
        ensureShadowRoot()->appendChild(DetailsSummaryElement::create(document()), ec, true);
        ensureShadowRoot()->appendChild(DetailsContentElement::create(document()), ec, true);
    }
}


void HTMLDetailsElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    HTMLElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    // If childCountDelta is less then zero and the main summary has changed it must be because previous main
    // summary was removed. The new main summary was then inside the unrevealed content and needs to be
    // reattached to create its renderer. If childCountDelta is not less then zero then a new <summary> element
    // has been added and it will be attached without our help.
    if (!changedByParser)
        refreshMainSummary(childCountDelta < 0 ? RefreshRendererAllowed : RefreshRendererSupressed);
}

void HTMLDetailsElement::finishParsingChildren()
{
    HTMLElement::finishParsingChildren();
    refreshMainSummary(RefreshRendererAllowed);
}

void HTMLDetailsElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == openAttr) {
        bool oldValue = m_isOpen;
        m_isOpen =  !attr->value().isNull();
        if (attached() && oldValue != m_isOpen) {
            detach();
            attach();
        }
    } else
        HTMLElement::parseMappedAttribute(attr);
}

bool HTMLDetailsElement::childShouldCreateRenderer(Node* child) const
{
    return m_isOpen || child == m_mainSummary;
}

void HTMLDetailsElement::toggleOpen()
{
    setAttribute(openAttr, m_isOpen ? nullAtom : emptyAtom);
}

}

#endif
