/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "NodeRenderingContext.h"

#include "ContainerNode.h"
#include "ContentDistributor.h"
#include "ElementShadow.h"
#include "FlowThreadController.h"
#include "HTMLContentElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "Node.h"
#include "PseudoElement.h"
#include "RenderFullScreen.h"
#include "RenderNamedFlowThread.h"
#include "RenderObject.h"
#include "RenderText.h"
#include "RenderView.h"
#include "ShadowRoot.h"
#include "StyleInheritedData.h"
#include "StyleResolver.h"
#include "Text.h"

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

namespace WebCore {

using namespace HTMLNames;

NodeRenderingContext::NodeRenderingContext(Node* node)
    : m_node(node)
    , m_parentFlowRenderer(0)
{
    m_renderingParent = NodeRenderingTraversal::parent(node, &m_parentDetails);
}

NodeRenderingContext::NodeRenderingContext(Node* node, RenderStyle* style)
    : m_node(node)
    , m_renderingParent(0)
    , m_style(style)
    , m_parentFlowRenderer(0)
{
}

NodeRenderingContext::NodeRenderingContext(Node* node, const Node::AttachContext& context)
    : m_node(node)
    , m_style(context.resolvedStyle)
    , m_parentFlowRenderer(0)
{
    m_renderingParent = NodeRenderingTraversal::parent(node, &m_parentDetails);
}

NodeRenderingContext::~NodeRenderingContext()
{
}

static bool isRendererReparented(const RenderObject* renderer)
{
    if (!renderer->node()->isElementNode())
        return false;
    if (renderer->style() && !renderer->style()->flowThread().isEmpty())
        return true;
#if ENABLE(DIALOG_ELEMENT)
    if (toElement(renderer->node())->isInTopLayer())
        return true;
#endif
    return false;
}

RenderObject* NodeRenderingContext::nextRenderer() const
{
    if (RenderObject* renderer = m_node->renderer())
        return renderer->nextSibling();

#if ENABLE(DIALOG_ELEMENT)
    Element* element = m_node->isElementNode() ? toElement(m_node) : 0;
    if (element && element->isInTopLayer()) {
        const Vector<RefPtr<Element> >& topLayerElements = element->document()->topLayerElements();
        size_t position = topLayerElements.find(element);
        ASSERT(position != notFound);
        for (size_t i = position + 1; i < topLayerElements.size(); ++i) {
            if (RenderObject* renderer = topLayerElements[i]->renderer())
                return renderer;
        }
        return 0;
    }
#endif

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer->nextRendererForNode(m_node);

    // Avoid an O(N^2) problem with this function by not checking for
    // nextRenderer() when the parent element hasn't attached yet.
    if (m_renderingParent && !m_renderingParent->attached())
        return 0;

    for (Node* sibling = NodeRenderingTraversal::nextSibling(m_node); sibling; sibling = NodeRenderingTraversal::nextSibling(sibling)) {
        RenderObject* renderer = sibling->renderer();
        if (renderer && !isRendererReparented(renderer))
            return renderer;
    }

    return 0;
}

RenderObject* NodeRenderingContext::previousRenderer() const
{
    if (RenderObject* renderer = m_node->renderer())
        return renderer->previousSibling();

#if ENABLE(DIALOG_ELEMENT)
    // FIXME: This doesn't work correctly for things in the top layer that are
    // display: none. We'd need to duplicate the logic in nextRenderer, but since
    // nothing needs that yet just assert.
    ASSERT(!m_node->isElementNode() || !toElement(m_node)->isInTopLayer());
#endif

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer->previousRendererForNode(m_node);

    // FIXME: We should have the same O(N^2) avoidance as nextRenderer does
    // however, when I tried adding it, several tests failed.
    for (Node* sibling = NodeRenderingTraversal::previousSibling(m_node); sibling; sibling = NodeRenderingTraversal::previousSibling(sibling)) {
        RenderObject* renderer = sibling->renderer();
        if (renderer && !isRendererReparented(renderer))
            return renderer;
    }

    return 0;
}

RenderObject* NodeRenderingContext::parentRenderer() const
{
    if (RenderObject* renderer = m_node->renderer())
        return renderer->parent();

#if ENABLE(DIALOG_ELEMENT)
    if (m_node->isElementNode() && toElement(m_node)->isInTopLayer()) {
        // The parent renderer of top layer elements is the RenderView, but only
        // if the normal parent would have had a renderer.
        // FIXME: This behavior isn't quite right as the spec for top layer
        // only talks about display: none ancestors so putting a <dialog> inside
        // an <optgroup> seems like it should still work even though this check
        // will prevent it.
        if (!m_renderingParent || !m_renderingParent->renderer())
            return 0;
        return m_node->document()->renderView();
    }
#endif

    if (m_parentFlowRenderer)
        return m_parentFlowRenderer;

    return m_renderingParent ? m_renderingParent->renderer() : 0;
}

bool NodeRenderingContext::shouldCreateRenderer() const
{
    if (!m_node->document()->shouldCreateRenderers())
        return false;
    if (!m_renderingParent)
        return false;
    RenderObject* parentRenderer = this->parentRenderer();
    if (!parentRenderer)
        return false;
    if (!parentRenderer->canHaveChildren() && !(m_node->isPseudoElement() && parentRenderer->canHaveGeneratedChildren()))
        return false;
    if (!m_renderingParent->childShouldCreateRenderer(*this))
        return false;
    return true;
}

// Check the specific case of elements that are children of regions but are flowed into a flow thread themselves.
bool NodeRenderingContext::elementInsideRegionNeedsRenderer()
{
    bool elementInsideRegionNeedsRenderer = false;

#if ENABLE(CSS_REGIONS)
    Element* element = toElement(m_node);
    RenderObject* parentRenderer = this->parentRenderer();
    if ((parentRenderer && !parentRenderer->canHaveChildren() && parentRenderer->isRenderRegion())
        || (!parentRenderer && element->parentElement() && element->parentElement()->isInsideRegion())) {

        if (!m_style)
            m_style = element->styleForRenderer();

        elementInsideRegionNeedsRenderer = element->shouldMoveToFlowThread(m_style.get());

        // Children of this element will only be allowed to be flowed into other flow-threads if display is NOT none.
        if (element->rendererIsNeeded(*this))
            element->setIsInsideRegion(true);
    }
#endif

    return elementInsideRegionNeedsRenderer;
}

void NodeRenderingContext::moveToFlowThreadIfNeeded()
{
#if ENABLE(CSS_REGIONS)
    Element* element = toElement(m_node);

    if (!element->shouldMoveToFlowThread(m_style.get()))
        return;

    ASSERT(m_node->document()->renderView());
    FlowThreadController* flowThreadController = m_node->document()->renderView()->flowThreadController();
    m_parentFlowRenderer = flowThreadController->ensureRenderFlowThreadWithName(m_style->flowThread());
    flowThreadController->registerNamedFlowContentNode(m_node, m_parentFlowRenderer);
#endif
}

bool NodeRenderingContext::isOnEncapsulationBoundary() const
{
    return isOnUpperEncapsulationBoundary() || isLowerEncapsulationBoundary(m_parentDetails.insertionPoint()) || isLowerEncapsulationBoundary(m_node->parentNode());
}

bool NodeRenderingContext::isOnUpperEncapsulationBoundary() const
{
    return m_node->parentNode() && m_node->parentNode()->isShadowRoot();
}

void NodeRenderingContext::createRendererForElementIfNeeded()
{
    ASSERT(!m_node->renderer());

    Element* element = toElement(m_node);

    element->setIsInsideRegion(false);

    if (!shouldCreateRenderer() && !elementInsideRegionNeedsRenderer())
        return;

    if (!m_style)
        m_style = element->styleForRenderer();
    ASSERT(m_style);

    moveToFlowThreadIfNeeded();

    if (!element->rendererIsNeeded(*this))
        return;

    RenderObject* parentRenderer = this->parentRenderer();
    RenderObject* nextRenderer = this->nextRenderer();

    Document* document = element->document();
    RenderObject* newRenderer = element->createRenderer(document->renderArena(), m_style.get());
    if (!newRenderer)
        return;
    if (!parentRenderer->isChildAllowed(newRenderer, m_style.get())) {
        newRenderer->destroy();
        return;
    }

    // Make sure the RenderObject already knows it is going to be added to a RenderFlowThread before we set the style
    // for the first time. Otherwise code using inRenderFlowThread() in the styleWillChange and styleDidChange will fail.
    newRenderer->setFlowThreadState(parentRenderer->flowThreadState());

    element->setRenderer(newRenderer);
    newRenderer->setAnimatableStyle(m_style.release()); // setAnimatableStyle() can depend on renderer() already being set.

#if ENABLE(FULLSCREEN_API)
    if (document->webkitIsFullScreen() && document->webkitCurrentFullScreenElement() == element) {
        newRenderer = RenderFullScreen::wrapRenderer(newRenderer, parentRenderer, document);
        if (!newRenderer)
            return;
    }
#endif
    // Note: Adding newRenderer instead of renderer(). renderer() may be a child of newRenderer.
    parentRenderer->addChild(newRenderer, nextRenderer);
}

void NodeRenderingContext::createRendererForTextIfNeeded()
{
    ASSERT(!m_node->renderer());

    Text* textNode = toText(m_node);

    if (!shouldCreateRenderer())
        return;

    RenderObject* parentRenderer = this->parentRenderer();
    ASSERT(parentRenderer);
    Document* document = textNode->document();

    if (resetStyleInheritance())
        m_style = document->ensureStyleResolver()->defaultStyleForElement();
    else
        m_style = parentRenderer->style();

    if (!textNode->textRendererIsNeeded(*this))
        return;
    RenderText* newRenderer = textNode->createTextRenderer(document->renderArena(), m_style.get());
    if (!newRenderer)
        return;
    if (!parentRenderer->isChildAllowed(newRenderer, m_style.get())) {
        newRenderer->destroy();
        return;
    }

    // Make sure the RenderObject already knows it is going to be added to a RenderFlowThread before we set the style
    // for the first time. Otherwise code using inRenderFlowThread() in the styleWillChange and styleDidChange will fail.
    newRenderer->setFlowThreadState(parentRenderer->flowThreadState());

    RenderObject* nextRenderer = this->nextRenderer();
    textNode->setRenderer(newRenderer);
    // Parent takes care of the animations, no need to call setAnimatableStyle.
    newRenderer->setStyle(m_style.release());
    parentRenderer->addChild(newRenderer, nextRenderer);
}

}
