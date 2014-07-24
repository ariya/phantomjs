/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "ViewportStyleResolver.h"

#if ENABLE(CSS_DEVICE_ADAPTATION)

#include "CSSValueKeywords.h"
#include "Document.h"
#include "Page.h"
#include "RenderView.h"
#include "StylePropertySet.h"
#include "StyleRule.h"
#include "ViewportArguments.h"

namespace WebCore {

ViewportStyleResolver::ViewportStyleResolver(Document* document)
    : m_document(document)
{
    ASSERT(m_document);
}

ViewportStyleResolver::~ViewportStyleResolver()
{
}

void ViewportStyleResolver::addViewportRule(StyleRuleViewport* viewportRule)
{
    StylePropertySet* propertySet = viewportRule->mutableProperties();

    unsigned propertyCount = propertySet->propertyCount();
    if (!propertyCount)
        return;

    if (!m_propertySet) {
        m_propertySet = propertySet->mutableCopy();
        return;
    }

    m_propertySet->mergeAndOverrideOnConflict(propertySet);
}

void ViewportStyleResolver::clearDocument()
{
    m_document = 0;
}

void ViewportStyleResolver::resolve()
{
    if (!m_document || !m_propertySet)
        return;

    ViewportArguments arguments(ViewportArguments::CSSDeviceAdaptation);

    arguments.userZoom = getViewportArgumentValue(CSSPropertyUserZoom);
    arguments.zoom = getViewportArgumentValue(CSSPropertyZoom);
    arguments.minZoom = getViewportArgumentValue(CSSPropertyMinZoom);
    arguments.maxZoom = getViewportArgumentValue(CSSPropertyMaxZoom);
    arguments.minWidth = getViewportArgumentValue(CSSPropertyMinWidth);
    arguments.maxWidth = getViewportArgumentValue(CSSPropertyMaxWidth);
    arguments.minHeight = getViewportArgumentValue(CSSPropertyMinHeight);
    arguments.maxHeight = getViewportArgumentValue(CSSPropertyMaxHeight);
    arguments.orientation = getViewportArgumentValue(CSSPropertyOrientation);

    m_document->setViewportArguments(arguments);
    m_document->updateViewportArguments();

    m_propertySet = 0;
}

float ViewportStyleResolver::getViewportArgumentValue(CSSPropertyID id) const
{
    float defaultValue = ViewportArguments::ValueAuto;

    // UserZoom default value is CSSValueZoom, which maps to true, meaning that
    // yes, it is user scalable. When the value is set to CSSValueFixed, we
    // return false.
    if (id == CSSPropertyUserZoom)
        defaultValue = 1;

    RefPtr<CSSValue> value = m_propertySet->getPropertyCSSValue(id);
    if (!value || !value->isPrimitiveValue())
        return defaultValue;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value.get());

    if (primitiveValue->isNumber() || primitiveValue->isPx())
        return primitiveValue->getFloatValue();

    if (primitiveValue->isFontRelativeLength())
        return primitiveValue->getFloatValue() * m_document->documentElement()->renderStyle()->fontDescription().computedSize();

    if (primitiveValue->isPercentage()) {
        float percentValue = primitiveValue->getFloatValue() / 100.0f;
        switch (id) {
        case CSSPropertyMaxHeight:
        case CSSPropertyMinHeight:
            ASSERT(m_document->initialViewportSize().height() > 0);
            return percentValue * m_document->initialViewportSize().height();
        case CSSPropertyMaxWidth:
        case CSSPropertyMinWidth:
            ASSERT(m_document->initialViewportSize().width() > 0);
            return percentValue * m_document->initialViewportSize().width();
        case CSSPropertyMaxZoom:
        case CSSPropertyMinZoom:
        case CSSPropertyZoom:
            return percentValue;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    switch (primitiveValue->getValueID()) {
    case CSSValueAuto:
        return defaultValue;
    case CSSValueDeviceHeight:
        return ViewportArguments::ValueDeviceHeight;
    case CSSValueDeviceWidth:
        return ViewportArguments::ValueDeviceWidth;
    case CSSValueLandscape:
        return ViewportArguments::ValueLandscape;
    case CSSValuePortrait:
        return ViewportArguments::ValuePortrait;
    case CSSValueZoom:
        return defaultValue;
    case CSSValueFixed:
        return 0;
    default:
        return defaultValue;
    }
}

} // namespace WebCore

#endif // ENABLE(CSS_DEVICE_ADAPTATION)
