/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006 Apple Computer, Inc.
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
#include "CSSRuleList.h"

#include "CSSMutableStyleDeclaration.h"
#include "CSSRule.h"
#include "StyleList.h"
#include "WebKitCSSKeyframeRule.h"

namespace WebCore {

CSSRuleList::CSSRuleList()
{
}

CSSRuleList::CSSRuleList(StyleList* list, bool omitCharsetRules)
{
    m_list = list;
    if (list && omitCharsetRules) {
        m_list = 0;
        unsigned len = list->length();
        for (unsigned i = 0; i < len; ++i) {
            StyleBase* style = list->item(i);
            if (style->isRule() && !style->isCharsetRule())
                append(static_cast<CSSRule*>(style));
        }
    }
}

CSSRuleList::~CSSRuleList()
{
}

unsigned CSSRuleList::length() const
{
    return m_list ? m_list->length() : m_lstCSSRules.size();
}

CSSRule* CSSRuleList::item(unsigned index)
{
    if (m_list) {
        StyleBase* rule = m_list->item(index);
        ASSERT(!rule || rule->isRule());
        return static_cast<CSSRule*>(rule);
    }

    if (index < m_lstCSSRules.size())
        return m_lstCSSRules[index].get();
    return 0;
}

void CSSRuleList::deleteRule(unsigned index)
{
    ASSERT(!m_list);

    if (index >= m_lstCSSRules.size()) {
        // FIXME: Should we throw an INDEX_SIZE_ERR exception here?
        return;
    }

    if (m_lstCSSRules[index]->isKeyframeRule()) {
        if (CSSMutableStyleDeclaration* style = static_cast<WebKitCSSKeyframeRule*>(m_lstCSSRules[index].get())->style())
            style->setParent(0);
    }

    m_lstCSSRules[index]->setParent(0);
    m_lstCSSRules.remove(index);
}

void CSSRuleList::append(CSSRule* rule)
{
    ASSERT(!m_list);
    if (!rule) {
        // FIXME: Should we throw an exception?
        return;
    }

    m_lstCSSRules.append(rule);
}

unsigned CSSRuleList::insertRule(CSSRule* rule, unsigned index)
{
    ASSERT(!m_list);
    if (!rule) {
        // FIXME: Should we throw an exception?
        return 0;
    }

    if (index > m_lstCSSRules.size()) {
        // FIXME: Should we throw an INDEX_SIZE_ERR exception here?
        return 0;
    }

    m_lstCSSRules.insert(index, rule);
    return index;
}

} // namespace WebCore
