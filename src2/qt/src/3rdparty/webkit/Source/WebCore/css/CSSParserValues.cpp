/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "CSSParserValues.h"

#include "CSSPrimitiveValue.h"
#include "CSSFunctionValue.h"
#include "CSSQuirkPrimitiveValue.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"

namespace WebCore {
        
using namespace WTF;

CSSParserValueList::~CSSParserValueList()
{
    size_t numValues = m_values.size();
    for (size_t i = 0; i < numValues; i++) {
        if (m_values[i].unit == CSSParserValue::Function)
            delete m_values[i].function;
    }
}

void CSSParserValueList::addValue(const CSSParserValue& v)
{
    m_values.append(v);
}

void CSSParserValueList::insertValueAt(unsigned i, const CSSParserValue& v)
{
    m_values.insert(i, v);
}

void CSSParserValueList::deleteValueAt(unsigned i)
{ 
    m_values.remove(i);
}

void CSSParserValueList::extend(CSSParserValueList& valueList)
{
    for (unsigned int i = 0; i < valueList.size(); ++i)
        m_values.append(*(valueList.valueAt(i)));
}

PassRefPtr<CSSValue> CSSParserValue::createCSSValue()
{
    RefPtr<CSSValue> parsedValue;
    if (id)
        parsedValue = CSSPrimitiveValue::createIdentifier(id);
    else if (unit == CSSPrimitiveValue::CSS_IDENT)
        parsedValue = CSSPrimitiveValue::create(string, CSSPrimitiveValue::CSS_PARSER_IDENTIFIER);
    else if (unit == CSSPrimitiveValue::CSS_NUMBER && isInt)
        parsedValue = CSSPrimitiveValue::create(fValue, CSSPrimitiveValue::CSS_PARSER_INTEGER);
    else if (unit == CSSParserValue::Operator) {
        RefPtr<CSSPrimitiveValue> primitiveValue = CSSPrimitiveValue::createIdentifier(iValue);
        primitiveValue->setPrimitiveType(CSSPrimitiveValue::CSS_PARSER_OPERATOR);
        parsedValue = primitiveValue;
    } else if (unit == CSSParserValue::Function)
        parsedValue = CSSFunctionValue::create(function);
    else if (unit == CSSPrimitiveValue::CSS_STRING || unit == CSSPrimitiveValue::CSS_URI || unit == CSSPrimitiveValue::CSS_PARSER_HEXCOLOR)
        parsedValue = CSSPrimitiveValue::create(string, (CSSPrimitiveValue::UnitTypes)unit);
    else if (unit >= CSSPrimitiveValue::CSS_NUMBER && unit <= CSSPrimitiveValue::CSS_KHZ)
        parsedValue = CSSPrimitiveValue::create(fValue, (CSSPrimitiveValue::UnitTypes)unit);
    else if (unit >= CSSPrimitiveValue::CSS_TURN && unit <= CSSPrimitiveValue::CSS_REMS) // CSS3 Values and Units
        parsedValue = CSSPrimitiveValue::create(fValue, (CSSPrimitiveValue::UnitTypes)unit);
    else if (unit >= CSSParserValue::Q_EMS)
        parsedValue = CSSQuirkPrimitiveValue::create(fValue, CSSPrimitiveValue::CSS_EMS);
    return parsedValue;
}
    
CSSParserSelector::CSSParserSelector()
    : m_selector(adoptPtr(fastNew<CSSSelector>()))
{
}

CSSParserSelector::~CSSParserSelector()
{
    if (!m_tagHistory)
        return;
    Vector<CSSParserSelector*, 16> toDelete;
    CSSParserSelector* selector = m_tagHistory.leakPtr();
    while (true) {
        toDelete.append(selector);
        CSSParserSelector* next = selector->m_tagHistory.leakPtr();
        if (!next)
            break;
        selector = next;
    }
    deleteAllValues(toDelete);
}

void CSSParserSelector::adoptSelectorVector(Vector<OwnPtr<CSSParserSelector> >& selectorVector)
{
    CSSSelectorList* selectorList = fastNew<CSSSelectorList>();
    selectorList->adoptSelectorVector(selectorVector);
    m_selector->setSelectorList(adoptPtr(selectorList));
}

void CSSParserSelector::insertTagHistory(CSSSelector::Relation before, PassOwnPtr<CSSParserSelector> selector, CSSSelector::Relation after)
{
    if (m_tagHistory)
        selector->setTagHistory(m_tagHistory.release());
    setRelation(before);
    selector->setRelation(after);
    m_tagHistory = selector;
}

void CSSParserSelector::appendTagHistory(CSSSelector::Relation relation, PassOwnPtr<CSSParserSelector> selector)
{
    CSSParserSelector* end = this;
    while (end->tagHistory())
        end = end->tagHistory();
    end->setRelation(relation);
    end->setTagHistory(selector);
}

}

