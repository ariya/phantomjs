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
#if ENABLE(DETAILS_ELEMENT)
#include "DetailsMarkerControl.h"

#include "HTMLNames.h"
#include "HTMLSummaryElement.h"
#include "RenderDetailsMarker.h"

namespace WebCore {

using namespace HTMLNames;

DetailsMarkerControl::DetailsMarkerControl(Document* document) 
    : HTMLDivElement(divTag, document)
{
}

RenderObject* DetailsMarkerControl::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderDetailsMarker(this);
}

bool DetailsMarkerControl::rendererIsNeeded(const NodeRenderingContext& context)
{
    return summaryElement()->isMainSummary() && HTMLDivElement::rendererIsNeeded(context);
}

const AtomicString& DetailsMarkerControl::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, pseudId, ("-webkit-details-marker", AtomicString::ConstructFromLiteral));
    return pseudId;
}

HTMLSummaryElement* DetailsMarkerControl::summaryElement()
{
    Element* element = shadowHost();
    ASSERT_WITH_SECURITY_IMPLICATION(!element || element->hasTagName(summaryTag));
    return static_cast<HTMLSummaryElement*>(element);
}

}

#endif
