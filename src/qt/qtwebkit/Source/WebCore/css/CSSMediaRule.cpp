/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2012 Apple Computer, Inc.
 * Copyright (C) 2006 Samuel Weinig (sam@webkit.org)
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
#include "CSSMediaRule.h"

#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleSheet.h"
#include "ExceptionCode.h"
#include "StyleRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

CSSMediaRule::CSSMediaRule(StyleRuleMedia* mediaRule, CSSStyleSheet* parent)
    : CSSGroupingRule(mediaRule, parent)
{
}

CSSMediaRule::~CSSMediaRule()
{
    if (m_mediaCSSOMWrapper)
        m_mediaCSSOMWrapper->clearParentRule();
}

MediaQuerySet* CSSMediaRule::mediaQueries() const
{
    return toStyleRuleMedia(m_groupRule.get())->mediaQueries();
}

String CSSMediaRule::cssText() const
{
    StringBuilder result;
    result.append("@media ");
    if (mediaQueries()) {
        result.append(mediaQueries()->mediaText());
        result.append(' ');
    }
    result.appendLiteral("{ \n");
    appendCssTextForItems(result);
    result.append('}');
    return result.toString();
}

MediaList* CSSMediaRule::media() const
{
    if (!mediaQueries())
        return 0;
    if (!m_mediaCSSOMWrapper)
        m_mediaCSSOMWrapper = MediaList::create(mediaQueries(), const_cast<CSSMediaRule*>(this));
    return m_mediaCSSOMWrapper.get();
}

void CSSMediaRule::reattach(StyleRuleBase* rule)
{
    CSSGroupingRule::reattach(rule);
    if (m_mediaCSSOMWrapper && mediaQueries())
        m_mediaCSSOMWrapper->reattach(mediaQueries());
}

} // namespace WebCore
