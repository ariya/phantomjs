/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2007, 2012 Apple Inc. All rights reserved.
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
#include "CSSRule.h"

#include "CSSStyleSheet.h"
#include "NotImplemented.h"
#include "StyleRule.h"
#include "StyleSheetContents.h"

namespace WebCore {

struct SameSizeAsCSSRule : public RefCounted<SameSizeAsCSSRule> {
    virtual ~SameSizeAsCSSRule();
    unsigned char bitfields;
    void* pointerUnion;
};

COMPILE_ASSERT(sizeof(CSSRule) == sizeof(SameSizeAsCSSRule), CSSRule_should_stay_small);

#if ENABLE(CSS_REGIONS)
COMPILE_ASSERT(StyleRuleBase::Region == static_cast<StyleRuleBase::Type>(CSSRule::WEBKIT_REGION_RULE), enums_should_match);
#endif

#if ENABLE(CSS_DEVICE_ADAPTATION)
COMPILE_ASSERT(StyleRuleBase::Viewport == static_cast<StyleRuleBase::Type>(CSSRule::WEBKIT_VIEWPORT_RULE), enums_should_match);
#endif

void CSSRule::setCssText(const String& /*cssText*/, ExceptionCode& /*ec*/)
{
    notImplemented();
}

const CSSParserContext& CSSRule::parserContext() const
{
    CSSStyleSheet* styleSheet = parentStyleSheet();
    return styleSheet ? styleSheet->contents()->parserContext() : strictCSSParserContext();
}

} // namespace WebCore
