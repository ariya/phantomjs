/*
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

#if ENABLE(SVG)
#include "RenderSVGShadowTreeRootContainer.h"

#include "MouseEvent.h"
#include "SVGShadowTreeElements.h"
#include "SVGUseElement.h"

namespace WebCore {

RenderSVGShadowTreeRootContainer::RenderSVGShadowTreeRootContainer(SVGUseElement* node)
    : RenderSVGTransformableContainer(node)
    , m_recreateTree(false)
{
}

RenderSVGShadowTreeRootContainer::~RenderSVGShadowTreeRootContainer()
{
    if (m_shadowRoot)
        m_shadowRoot->clearSVGShadowHost();
}

void RenderSVGShadowTreeRootContainer::updateStyle(Node::StyleChange change)
{
    if (m_shadowRoot && m_shadowRoot->attached())
        m_shadowRoot->recalcStyle(change);
}

void RenderSVGShadowTreeRootContainer::updateFromElement()
{
    bool hadExistingTree = m_shadowRoot;

    SVGUseElement* useElement = static_cast<SVGUseElement*>(node());
    if (!m_shadowRoot) {
        ASSERT(!m_recreateTree);
        m_shadowRoot = SVGShadowTreeRootElement::create(document(), useElement);
        useElement->buildPendingResource();
    }

    ASSERT(m_shadowRoot->svgShadowHost() == useElement);

    bool shouldRecreateTree = m_recreateTree;
    if (m_recreateTree) {
        ASSERT(hadExistingTree);

        if (m_shadowRoot->attached())
            m_shadowRoot->detach();

        m_shadowRoot->removeAllChildren();
        m_recreateTree = false;
    }

    // Only rebuild the shadow tree, if we a) never had a tree or b) we were specifically asked to do so
    // If the use element is a pending resource, and a) or b) is true, do nothing, and wait for the use
    // element to be asked to buildPendingResource(), this will call us again, with m_recreateTrue=true.
    if ((shouldRecreateTree || !hadExistingTree) && !useElement->hasPendingResources()) {
        useElement->buildShadowAndInstanceTree(m_shadowRoot.get());

        // Attach shadow root element
        m_shadowRoot->attachElement(style(), renderArena());

        // Attach subtree, as if it was a regular non-shadow tree
        for (Node* child = m_shadowRoot->firstChild(); child; child = child->nextSibling())
            child->attach();
    }

    ASSERT(!m_recreateTree);
    RenderSVGTransformableContainer::updateFromElement();
}

void RenderSVGShadowTreeRootContainer::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderSVGTransformableContainer::styleDidChange(diff, oldStyle);

    if (RenderObject* shadowRootRenderer = m_shadowRoot ? m_shadowRoot->renderer() : 0)
        shadowRootRenderer->setStyle(style());
}

Node* RenderSVGShadowTreeRootContainer::rootElement() const
{
    return m_shadowRoot.get();
}

}

#endif
