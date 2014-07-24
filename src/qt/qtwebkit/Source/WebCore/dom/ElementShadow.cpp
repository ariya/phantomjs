/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ElementShadow.h"

#include "ContainerNodeAlgorithms.h"
#include "InspectorInstrumentation.h"

namespace WebCore {

ShadowRoot* ElementShadow::addShadowRoot(Element* shadowHost, ShadowRoot::ShadowRootType type)
{
    ASSERT(!m_shadowRoot);
    m_shadowRoot = ShadowRoot::create(shadowHost->document(), type);

    m_shadowRoot->setParentOrShadowHostNode(shadowHost);
    m_shadowRoot->setParentTreeScope(shadowHost->treeScope());
    m_distributor.didShadowBoundaryChange(shadowHost);
    ChildNodeInsertionNotifier(shadowHost).notify(m_shadowRoot.get());

    // Existence of shadow roots requires the host and its children to do traversal using ComposedShadowTreeWalker.
    shadowHost->setNeedsShadowTreeWalker();

    // FIXME(94905): ShadowHost should be reattached during recalcStyle.
    // Set some flag here and recreate shadow hosts' renderer in
    // Element::recalcStyle.
    if (shadowHost->attached())
        shadowHost->lazyReattach();

    InspectorInstrumentation::didPushShadowRoot(shadowHost, m_shadowRoot.get());

    return m_shadowRoot.get();
}

void ElementShadow::removeShadowRoot()
{
    // Dont protect this ref count.
    Element* shadowHost = host();

    if (RefPtr<ShadowRoot> oldRoot = m_shadowRoot) {
        InspectorInstrumentation::willPopShadowRoot(shadowHost, oldRoot.get());
        shadowHost->document()->removeFocusedNodeOfSubtree(oldRoot.get());

        if (oldRoot->attached())
            oldRoot->detach();

        m_shadowRoot = 0;
        oldRoot->setParentOrShadowHostNode(0);
        oldRoot->setParentTreeScope(shadowHost->document());
        ChildNodeRemovalNotifier(shadowHost).notify(oldRoot.get());
    }

    m_distributor.invalidateDistribution(shadowHost);
}

void ElementShadow::attach(const Node::AttachContext& context)
{
    ContentDistributor::ensureDistribution(shadowRoot());

    Node::AttachContext childrenContext(context);
    childrenContext.resolvedStyle = 0;

    if (ShadowRoot* root = shadowRoot()) {
        if (!root->attached())
            root->attach(childrenContext);
    }
}

void ElementShadow::detach(const Node::AttachContext& context)
{
    Node::AttachContext childrenContext(context);
    childrenContext.resolvedStyle = 0;

    if (ShadowRoot* root = shadowRoot()) {
        if (root->attached())
            root->detach(childrenContext);
    }
}

bool ElementShadow::childNeedsStyleRecalc() const
{
    ASSERT(shadowRoot());
    return shadowRoot()->childNeedsStyleRecalc();
}

bool ElementShadow::needsStyleRecalc() const
{
    ASSERT(shadowRoot());
    return shadowRoot()->needsStyleRecalc();
}

void ElementShadow::recalcStyle(Node::StyleChange change)
{
    if (ShadowRoot* root = shadowRoot())
        root->recalcStyle(change);
}

void ElementShadow::removeAllEventListeners()
{
    if (ShadowRoot* root = shadowRoot()) {
        for (Node* node = root; node; node = NodeTraversal::next(node))
            node->removeAllEventListeners();
    }
}

} // namespace
