/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef CSSParserValues_h
#define CSSParserValues_h

#include "CSSSelector.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSValue;
class QualifiedName;

struct CSSParserString {
    void init(LChar* characters, unsigned length)
    {
        m_data.characters8 = characters;
        m_length = length;
        m_is8Bit = true;
    }

    void init(UChar* characters, unsigned length)
    {
        m_data.characters16 = characters;
        m_length = length;
        m_is8Bit = false;
    }

    void init(const String& string)
    {
        m_length = string.length();
        if (!m_length || string.is8Bit()) {
            m_data.characters8 = const_cast<LChar*>(string.characters8());
            m_is8Bit = true;
        } else {
            m_data.characters16 = const_cast<UChar*>(string.characters16());
            m_is8Bit = false;
        }
    }

    void clear()
    {
        m_data.characters8 = 0;
        m_length = 0;
        m_is8Bit = true;
    }

    bool is8Bit() const { return m_is8Bit; }
    LChar* characters8() const { ASSERT(is8Bit()); return m_data.characters8; }
    UChar* characters16() const { ASSERT(!is8Bit()); return m_data.characters16; }
    template <typename CharacterType>
    CharacterType* characters() const;

    unsigned length() const { return m_length; }
    void setLength(unsigned length) { m_length = length; }

    void lower();

    UChar operator[](unsigned i)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_length);
        if (is8Bit())
            return m_data.characters8[i];
        return m_data.characters16[i];
    }

    bool equalIgnoringCase(const char* str)
    {
        if (is8Bit())
            return WTF::equalIgnoringCase(str, characters8(), length());
        return WTF::equalIgnoringCase(str, characters16(), length());
    }

    operator String() const { return is8Bit() ? String(m_data.characters8, m_length) : String(m_data.characters16, m_length); }
    operator AtomicString() const { return is8Bit() ? AtomicString(m_data.characters8, m_length) : AtomicString(m_data.characters16, m_length); }

#if ENABLE(CSS_VARIABLES)
    AtomicString substring(unsigned position, unsigned length) const;
#endif

    union {
        LChar* characters8;
        UChar* characters16;
    } m_data;
    unsigned m_length;
    bool m_is8Bit;
};

struct CSSParserFunction;

struct CSSParserValue {
    CSSValueID id;
    bool isInt;
    union {
        double fValue;
        int iValue;
        CSSParserString string;
        CSSParserFunction* function;
    };
    enum {
        Operator = 0x100000,
        Function = 0x100001,
        Q_EMS    = 0x100002
    };
    int unit;


    PassRefPtr<CSSValue> createCSSValue();
};

class CSSParserValueList {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CSSParserValueList()
        : m_current(0)
    {
    }
    ~CSSParserValueList();

    void addValue(const CSSParserValue&);
    void insertValueAt(unsigned, const CSSParserValue&);
    void deleteValueAt(unsigned);
    void extend(CSSParserValueList&);

    unsigned size() const { return m_values.size(); }
    unsigned currentIndex() { return m_current; }
    CSSParserValue* current() { return m_current < m_values.size() ? &m_values[m_current] : 0; }
    CSSParserValue* next() { ++m_current; return current(); }
    CSSParserValue* previous()
    {
        if (!m_current)
            return 0;
        --m_current;
        return current();
    }

    CSSParserValue* valueAt(unsigned i) { return i < m_values.size() ? &m_values[i] : 0; }

    void clear() { m_values.clear(); }

private:
    unsigned m_current;
    Vector<CSSParserValue, 4> m_values;
};

struct CSSParserFunction {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CSSParserString name;
    OwnPtr<CSSParserValueList> args;
};

class CSSParserSelector {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CSSParserSelector();
    explicit CSSParserSelector(const QualifiedName&);
    ~CSSParserSelector();

    PassOwnPtr<CSSSelector> releaseSelector() { return m_selector.release(); }

    void setValue(const AtomicString& value) { m_selector->setValue(value); }
    void setAttribute(const QualifiedName& value, bool isCaseInsensitive) { m_selector->setAttribute(value, isCaseInsensitive); }
    void setArgument(const AtomicString& value) { m_selector->setArgument(value); }
    void setMatch(CSSSelector::Match value) { m_selector->m_match = value; }
    void setRelation(CSSSelector::Relation value) { m_selector->m_relation = value; }
    void setForPage() { m_selector->setForPage(); }

    void adoptSelectorVector(Vector<OwnPtr<CSSParserSelector> >& selectorVector);

    CSSSelector::PseudoType pseudoType() const { return m_selector->pseudoType(); }
    bool isCustomPseudoElement() const { return m_selector->isCustomPseudoElement(); }

    bool isSimple() const;
    bool hasShadowDescendant() const;

    CSSParserSelector* tagHistory() const { return m_tagHistory.get(); }
    void setTagHistory(PassOwnPtr<CSSParserSelector> selector) { m_tagHistory = selector; }
    void clearTagHistory() { m_tagHistory.clear(); }
    void insertTagHistory(CSSSelector::Relation before, PassOwnPtr<CSSParserSelector>, CSSSelector::Relation after);
    void appendTagHistory(CSSSelector::Relation, PassOwnPtr<CSSParserSelector>);
    void prependTagSelector(const QualifiedName&, bool tagIsForNamespaceRule = false);

private:
    OwnPtr<CSSSelector> m_selector;
    OwnPtr<CSSParserSelector> m_tagHistory;
};

inline bool CSSParserSelector::hasShadowDescendant() const
{
    return m_selector->relation() == CSSSelector::ShadowDescendant;
}

}

#endif
