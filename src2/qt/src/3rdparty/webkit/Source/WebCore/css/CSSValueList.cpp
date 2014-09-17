/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "CSSValueList.h"

#include "CSSParserValues.h"
#include "PlatformString.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

CSSValueList::CSSValueList(bool isSpaceSeparated)
    : m_isSpaceSeparated(isSpaceSeparated)
{
}

CSSValueList::CSSValueList(CSSParserValueList* list)
    : m_isSpaceSeparated(true)
{
    if (list) {
        size_t size = list->size();
        for (unsigned i = 0; i < size; ++i)
            append(list->valueAt(i)->createCSSValue());
    }
}

CSSValueList::~CSSValueList()
{
}

CSSValue* CSSValueList::item(unsigned index)
{
    if (index >= m_values.size())
        return 0;
    return m_values[index].get();
}

unsigned short CSSValueList::cssValueType() const
{
    return CSS_VALUE_LIST;
}

void CSSValueList::append(PassRefPtr<CSSValue> val)
{
    m_values.append(val);
}

void CSSValueList::prepend(PassRefPtr<CSSValue> val)
{
    m_values.prepend(val);
}

bool CSSValueList::removeAll(CSSValue* val)
{
    bool found = false;
    // FIXME: we should be implementing operator== to CSSValue and its derived classes
    // to make comparison more flexible and fast.
    for (size_t index = 0; index < m_values.size(); index++) {
        if (m_values.at(index)->cssText() == val->cssText()) {
            m_values.remove(index);
            found = true;
        }
    }
    
    return found;
}
    
bool CSSValueList::hasValue(CSSValue* val)
{
    // FIXME: we should be implementing operator== to CSSValue and its derived classes
    // to make comparison more flexible and fast.
    for (size_t index = 0; index < m_values.size(); index++) {
        if (m_values.at(index)->cssText() == val->cssText())
            return true;
    }
    return false;
}

PassRefPtr<CSSValueList> CSSValueList::copy()
{
    PassRefPtr<CSSValueList> newList = m_isSpaceSeparated ? createSpaceSeparated() : createCommaSeparated();
    for (size_t index = 0; index < m_values.size(); index++)
        newList->append(item(index));
    return newList;
}

String CSSValueList::cssText() const
{
    String result = "";

    unsigned size = m_values.size();
    for (unsigned i = 0; i < size; i++) {
        if (!result.isEmpty()) {
            if (m_isSpaceSeparated)
                result += " ";
            else
                result += ", ";
        }
        result += m_values[i]->cssText();
    }

    return result;
}

void CSSValueList::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const CSSStyleSheet* styleSheet)
{
    size_t size = m_values.size();
    for (size_t i = 0; i < size; ++i)
        m_values[i]->addSubresourceStyleURLs(urls, styleSheet);
}

} // namespace WebCore
