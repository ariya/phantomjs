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
#if ENABLE(METER_ELEMENT)
#include "MeterShadowElement.h"

#include "CSSPropertyNames.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "RenderMeter.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"
#include "StylePropertySet.h"

namespace WebCore {

using namespace HTMLNames;

MeterShadowElement::MeterShadowElement(Document* document) 
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

HTMLMeterElement* MeterShadowElement::meterElement() const
{
    return toHTMLMeterElement(shadowHost());
}

bool MeterShadowElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    RenderObject* render = meterElement()->renderer();
    return render && !render->theme()->supportsMeter(render->style()->appearance()) && HTMLDivElement::rendererIsNeeded(context);
}

MeterInnerElement::MeterInnerElement(Document* document)
    : MeterShadowElement(document)
{
    DEFINE_STATIC_LOCAL(AtomicString, pseudoId, ("-webkit-meter-inner-element", AtomicString::ConstructFromLiteral));
    setPseudo(pseudoId);
}

bool MeterInnerElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (meterElement()->hasAuthorShadowRoot())
        return HTMLDivElement::rendererIsNeeded(context);

    RenderObject* render = meterElement()->renderer();
    return render && !render->theme()->supportsMeter(render->style()->appearance()) && HTMLDivElement::rendererIsNeeded(context);
}

RenderObject* MeterInnerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMeter(this);
}

const AtomicString& MeterValueElement::valuePseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, optimumPseudoId, ("-webkit-meter-optimum-value", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, suboptimumPseudoId, ("-webkit-meter-suboptimum-value", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, evenLessGoodPseudoId, ("-webkit-meter-even-less-good-value", AtomicString::ConstructFromLiteral));

    HTMLMeterElement* meter = meterElement();
    if (!meter)
        return optimumPseudoId;

    switch (meter->gaugeRegion()) {
    case HTMLMeterElement::GaugeRegionOptimum:
        return optimumPseudoId;
    case HTMLMeterElement::GaugeRegionSuboptimal:
        return suboptimumPseudoId;
    case HTMLMeterElement::GaugeRegionEvenLessGood:
        return evenLessGoodPseudoId;
    default:
        ASSERT_NOT_REACHED();
        return optimumPseudoId;
    }
}

void MeterValueElement::setWidthPercentage(double width)
{
    setInlineStyleProperty(CSSPropertyWidth, width, CSSPrimitiveValue::CSS_PERCENTAGE);
}

}

#endif

