/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "ShadowRoot.h"

#include "ContentDistributor.h"
#include "ElementShadow.h"
#include "HistogramSupport.h"
#include "InsertionPoint.h"
#include "RuntimeEnabledFeatures.h"
#include "StyleResolver.h"
#include "Text.h"
#include "markup.h"

namespace WebCore {

struct SameSizeAsShadowRoot : public DocumentFragment, public TreeScope {
    unsigned countersAndFlags[1];
};

COMPILE_ASSERT(sizeof(ShadowRoot) == sizeof(SameSizeAsShadowRoot), shadowroot_should_stay_small);

enum ShadowRootUsageOriginType {
    ShadowRootUsageOriginWeb = 0,
    ShadowRootUsageOriginNotWeb,
    ShadowRootUsageOriginMax
};

ShadowRoot::ShadowRoot(Document* document, ShadowRootType type)
    : DocumentFragment(0, CreateShadowRoot)
    , TreeScope(this, document)
    , m_numberOfStyles(0)
    , m_applyAuthorStyles(false)
    , m_resetStyleInheritance(false)
    , m_type(type)
{
    ASSERT(document);
}

ShadowRoot::~ShadowRoot()
{
    // We cannot let ContainerNode destructor call willBeDeletedFrom()
    // for this ShadowRoot instance because TreeScope destructor
    // clears Node::m_treeScope thus ContainerNode is no longer able
    // to access it Document reference after that.
    willBeDeletedFrom(documentInternal());

    // We must remove all of our children first before the TreeScope destructor
    // runs so we don't go through TreeScopeAdopter for each child with a
    // destructed tree scope in each descendant.
    removeDetachedChildren();

    // We must call clearRareData() here since a ShadowRoot class inherits TreeScope
    // as well as Node. See a comment on TreeScope.h for the reason.
    if (hasRareData())
        clearRareData();
}

void ShadowRoot::dispose()
{
    removeDetachedChildren();
}

PassRefPtr<Node> ShadowRoot::cloneNode(bool, ExceptionCode& ec)
{
    ec = DATA_CLONE_ERR;
    return 0;
}

String ShadowRoot::innerHTML() const
{
    return createMarkup(this, ChildrenOnly);
}

void ShadowRoot::setInnerHTML(const String& markup, ExceptionCode& ec)
{
    if (isOrphan()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    if (RefPtr<DocumentFragment> fragment = createFragmentForInnerOuterHTML(markup, host(), AllowScriptingContent, ec))
        replaceChildrenWithFragment(this, fragment.release(), ec);
}

bool ShadowRoot::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ELEMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case COMMENT_NODE:
    case TEXT_NODE:
    case CDATA_SECTION_NODE:
    case ENTITY_REFERENCE_NODE:
        return true;
    default:
        return false;
    }
}

void ShadowRoot::recalcStyle(StyleChange change)
{
    // ShadowRoot doesn't support custom callbacks.
    ASSERT(!hasCustomStyleCallbacks());

    StyleResolver* styleResolver = document()->ensureStyleResolver();
    styleResolver->pushParentShadowRoot(this);

    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isElementNode())
            toElement(child)->recalcStyle(change);
        else if (child->isTextNode())
            toText(child)->recalcTextStyle(change);
    }

    styleResolver->popParentShadowRoot(this);
    clearNeedsStyleRecalc();
    clearChildNeedsStyleRecalc();
}

void ShadowRoot::setApplyAuthorStyles(bool value)
{
    if (isOrphan())
        return;

    if (m_applyAuthorStyles != value) {
        m_applyAuthorStyles = value;
        host()->setNeedsStyleRecalc();
    }
}

void ShadowRoot::setResetStyleInheritance(bool value)
{
    if (isOrphan())
        return;

    if (value != m_resetStyleInheritance) {
        m_resetStyleInheritance = value;
        if (attached() && owner())
            owner()->recalcStyle(Force);
    }
}

void ShadowRoot::attach(const AttachContext& context)
{
    StyleResolver* styleResolver = document()->ensureStyleResolver();
    styleResolver->pushParentShadowRoot(this);
    DocumentFragment::attach(context);
    styleResolver->popParentShadowRoot(this);
}

void ShadowRoot::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    if (isOrphan())
        return;

    ContainerNode::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    owner()->invalidateDistribution();
}

void ShadowRoot::registerScopedHTMLStyleChild()
{
    ++m_numberOfStyles;
    setHasScopedHTMLStyleChild(true);
}

void ShadowRoot::unregisterScopedHTMLStyleChild()
{
    ASSERT(hasScopedHTMLStyleChild() && m_numberOfStyles > 0);
    --m_numberOfStyles;
    setHasScopedHTMLStyleChild(m_numberOfStyles > 0);
}

}
