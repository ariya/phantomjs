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
#include "PseudoElement.h"

#include "ContentData.h"
#include "InspectorInstrumentation.h"
#include "NodeRenderingContext.h"
#include "RenderObject.h"
#include "RenderQuote.h"

namespace WebCore {

const QualifiedName& pseudoElementTagName()
{
    DEFINE_STATIC_LOCAL(QualifiedName, name, (nullAtom, "<pseudo>", nullAtom));
    return name;
}

String PseudoElement::pseudoElementNameForEvents(PseudoId pseudoId)
{
    DEFINE_STATIC_LOCAL(const String, after, (ASCIILiteral("::after")));
    DEFINE_STATIC_LOCAL(const String, before, (ASCIILiteral("::before")));
    switch (pseudoId) {
    case AFTER:
        return after;
    case BEFORE:
        return before;
    default:
        return emptyString();
    }
}

PseudoElement::PseudoElement(Element* parent, PseudoId pseudoId)
    : Element(pseudoElementTagName(), parent->document(), CreatePseudoElement)
    , m_pseudoId(pseudoId)
{
    ASSERT(pseudoId != NOPSEUDO);
    setParentOrShadowHostNode(parent);
    setHasCustomStyleCallbacks();
}

PseudoElement::~PseudoElement()
{
#if USE(ACCELERATED_COMPOSITING)
    InspectorInstrumentation::pseudoElementDestroyed(document()->page(), this);
#endif
}

PassRefPtr<RenderStyle> PseudoElement::customStyleForRenderer()
{
    return parentOrShadowHostElement()->renderer()->getCachedPseudoStyle(m_pseudoId);
}

void PseudoElement::attach(const AttachContext& context)
{
    ASSERT(!renderer());

    Element::attach(context);

    RenderObject* renderer = this->renderer();
    if (!renderer || !renderer->style()->regionThread().isEmpty())
        return;

    RenderStyle* style = renderer->style();
    ASSERT(style->contentData());

    for (const ContentData* content = style->contentData(); content; content = content->next()) {
        RenderObject* child = content->createRenderer(document(), style);
        if (renderer->isChildAllowed(child, style)) {
            renderer->addChild(child);
            if (child->isQuote())
                toRenderQuote(child)->attachQuote();
        } else
            child->destroy();
    }
}

bool PseudoElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    return pseudoElementRendererIsNeeded(context.style());
}

void PseudoElement::didRecalcStyle(StyleChange)
{
    if (!renderer())
        return;

    // The renderers inside pseudo elements are anonymous so they don't get notified of recalcStyle and must have
    // the style propagated downward manually similar to RenderObject::propagateStyleToAnonymousChildren.
    RenderObject* renderer = this->renderer();
    for (RenderObject* child = renderer->nextInPreOrder(renderer); child; child = child->nextInPreOrder(renderer)) {
        // We only manage the style for the generated content which must be images or text.
        if (!child->isText() && !child->isImage())
            continue;

        // The style for the RenderTextFragment for first letter is managed by an enclosing block, not by us.
        if (child->style()->styleType() == FIRST_LETTER)
            continue;

        child->setPseudoStyle(renderer->style());
    }
}

} // namespace
