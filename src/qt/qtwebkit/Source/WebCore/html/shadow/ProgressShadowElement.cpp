/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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
#if ENABLE(PROGRESS_ELEMENT)
#include "ProgressShadowElement.h"

#include "HTMLNames.h"
#include "HTMLProgressElement.h"
#include "RenderProgress.h"

namespace WebCore {

using namespace HTMLNames;

ProgressShadowElement::ProgressShadowElement(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

HTMLProgressElement* ProgressShadowElement::progressElement() const
{
    return toHTMLProgressElement(shadowHost());
}

bool ProgressShadowElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    RenderObject* progressRenderer = progressElement()->renderer();
    return progressRenderer && !progressRenderer->style()->hasAppearance() && HTMLDivElement::rendererIsNeeded(context);
}

ProgressInnerElement::ProgressInnerElement(Document* document)
    : ProgressShadowElement(document)
{
    DEFINE_STATIC_LOCAL(AtomicString, pseudoId, ("-webkit-progress-inner-element", AtomicString::ConstructFromLiteral));
    setPseudo(pseudoId);
}

PassRefPtr<ProgressInnerElement> ProgressInnerElement::create(Document* document)
{
    return adoptRef(new ProgressInnerElement(document));
}

RenderObject* ProgressInnerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderProgress(this);
}

bool ProgressInnerElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (progressElement()->hasAuthorShadowRoot())
        return HTMLDivElement::rendererIsNeeded(context);

    RenderObject* progressRenderer = progressElement()->renderer();
    return progressRenderer && !progressRenderer->style()->hasAppearance() && HTMLDivElement::rendererIsNeeded(context);    
}

void ProgressValueElement::setWidthPercentage(double width)
{
    setInlineStyleProperty(CSSPropertyWidth, width, CSSPrimitiveValue::CSS_PERCENTAGE);
}

}
#endif
