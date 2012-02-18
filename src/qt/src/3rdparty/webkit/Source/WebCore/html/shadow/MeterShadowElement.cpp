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
#if ENABLE(METER_TAG)
#include "MeterShadowElement.h"

#include "CSSMutableStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "RenderMeter.h"
#include "RenderTheme.h"

namespace WebCore {

using namespace HTMLNames;

MeterShadowElement::MeterShadowElement(Document* document) 
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

HTMLMeterElement* MeterShadowElement::meterElement() const
{
    Node* node = const_cast<MeterShadowElement*>(this)->shadowAncestorNode();
    ASSERT(!node || meterTag == toElement(node)->tagQName());
    return static_cast<HTMLMeterElement*>(node);
}

bool MeterShadowElement::rendererIsNeeded(RenderStyle* style)
{
    RenderMeter* meterRenderer = toRenderMeter(meterElement()->renderer());
    return meterRenderer && !meterRenderer->theme()->supportsMeter(meterRenderer->style()->appearance()) && HTMLDivElement::rendererIsNeeded(style);
}

const AtomicString& MeterBarElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, pseudId, ("-webkit-meter-bar"));
    return pseudId;
}


const AtomicString& MeterValueElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, optimumPseudId, ("-webkit-meter-optimum-value"));
    DEFINE_STATIC_LOCAL(AtomicString, suboptimumPseudId, ("-webkit-meter-suboptimum-value"));
    DEFINE_STATIC_LOCAL(AtomicString, evenLessGoodPseudId, ("-webkit-meter-even-less-good-value"));

    HTMLMeterElement* meter = meterElement();
    if (!meter)
        return optimumPseudId;

    switch (meter->gaugeRegion()) {
    case HTMLMeterElement::GaugeRegionOptimum:
        return optimumPseudId;
    case HTMLMeterElement::GaugeRegionSuboptimal:
        return suboptimumPseudId;
    case HTMLMeterElement::GaugeRegionEvenLessGood:
        return evenLessGoodPseudId;
    default:
        ASSERT_NOT_REACHED();
        return optimumPseudId;
    }
}

void MeterValueElement::setWidthPercentage(double width)
{
    getInlineStyleDecl()->setProperty(CSSPropertyWidth, width, CSSPrimitiveValue::CSS_PERCENTAGE);
}


}

#endif

