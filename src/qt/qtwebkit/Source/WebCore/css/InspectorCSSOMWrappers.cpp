/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "InspectorCSSOMWrappers.h"

#include "CSSDefaultStyleSheets.h"
#include "CSSHostRule.h"
#include "CSSImportRule.h"
#include "CSSMediaRule.h"
#include "CSSRule.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CSSSupportsRule.h"
#include "DocumentStyleSheetCollection.h"
#include "StyleSheetContents.h"
#include "WebKitCSSRegionRule.h"

namespace WebCore {

void InspectorCSSOMWrappers::collectFromStyleSheetIfNeeded(CSSStyleSheet* styleSheet)
{
    if (!m_styleRuleToCSSOMWrapperMap.isEmpty())
        collect(styleSheet);
}

template <class ListType>
void InspectorCSSOMWrappers::collect(ListType* listType)
{
    if (!listType)
        return;
    unsigned size = listType->length();
    for (unsigned i = 0; i < size; ++i) {
        CSSRule* cssRule = listType->item(i);
        switch (cssRule->type()) {
        case CSSRule::IMPORT_RULE:
            collect(static_cast<CSSImportRule*>(cssRule)->styleSheet());
            break;
        case CSSRule::MEDIA_RULE:
            collect(static_cast<CSSMediaRule*>(cssRule));
            break;
#if ENABLE(CSS3_CONDITIONAL_RULES)
        case CSSRule::SUPPORTS_RULE:
            collect(static_cast<CSSSupportsRule*>(cssRule));
            break;
#endif
#if ENABLE(CSS_REGIONS)
        case CSSRule::WEBKIT_REGION_RULE:
            collect(static_cast<WebKitCSSRegionRule*>(cssRule));
            break;
#endif
#if ENABLE(SHADOW_DOM)
        case CSSRule::HOST_RULE:
            collect(static_cast<CSSHostRule*>(cssRule));
            break;
#endif
        case CSSRule::STYLE_RULE:
            m_styleRuleToCSSOMWrapperMap.add(static_cast<CSSStyleRule*>(cssRule)->styleRule(), static_cast<CSSStyleRule*>(cssRule));
            break;
        default:
            break;
        }
    }
}

void InspectorCSSOMWrappers::collectFromStyleSheetContents(HashSet<RefPtr<CSSStyleSheet> >& sheetWrapperSet, StyleSheetContents* styleSheet)
{
    if (!styleSheet)
        return;
    RefPtr<CSSStyleSheet> styleSheetWrapper = CSSStyleSheet::create(styleSheet);
    sheetWrapperSet.add(styleSheetWrapper);
    collect(styleSheetWrapper.get());
}

void InspectorCSSOMWrappers::collectFromStyleSheets(const Vector<RefPtr<CSSStyleSheet> >& sheets)
{
    for (unsigned i = 0; i < sheets.size(); ++i)
        collect(sheets[i].get());
}

void InspectorCSSOMWrappers::collectFromDocumentStyleSheetCollection(DocumentStyleSheetCollection* styleSheetCollection)
{
    collectFromStyleSheets(styleSheetCollection->activeAuthorStyleSheets());
    collect(styleSheetCollection->pageUserSheet());
    collectFromStyleSheets(styleSheetCollection->injectedUserStyleSheets());
    collectFromStyleSheets(styleSheetCollection->documentUserStyleSheets());
}

CSSStyleRule* InspectorCSSOMWrappers::getWrapperForRuleInSheets(StyleRule* rule, DocumentStyleSheetCollection* styleSheetCollection)
{
    if (m_styleRuleToCSSOMWrapperMap.isEmpty()) {
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::simpleDefaultStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::defaultStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::quirksStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::svgStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::mathMLStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::mediaControlsStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::fullscreenStyleSheet);
        collectFromStyleSheetContents(m_styleSheetCSSOMWrapperSet, CSSDefaultStyleSheets::plugInsStyleSheet);

        collectFromDocumentStyleSheetCollection(styleSheetCollection);
    }
    return m_styleRuleToCSSOMWrapperMap.get(rule);
}

} // namespace WebCore
