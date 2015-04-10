/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
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

#ifndef Counter_h
#define Counter_h

#include "CSSPrimitiveValue.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class Counter : public RefCounted<Counter> {
public:
    static PassRefPtr<Counter> create(PassRefPtr<CSSPrimitiveValue> identifier, PassRefPtr<CSSPrimitiveValue> listStyle, PassRefPtr<CSSPrimitiveValue> separator)
    {
        return adoptRef(new Counter(identifier, listStyle, separator));
    }

    String identifier() const { return m_identifier ? m_identifier->getStringValue() : String(); }
    String listStyle() const { return m_listStyle ? m_listStyle->getStringValue() : String(); }
    String separator() const { return m_separator ? m_separator->getStringValue() : String(); }

    CSSValueID listStyleIdent() const { return m_listStyle ? m_listStyle->getValueID() : CSSValueInvalid; }

    void setIdentifier(PassRefPtr<CSSPrimitiveValue> identifier) { m_identifier = identifier; }
    void setListStyle(PassRefPtr<CSSPrimitiveValue> listStyle) { m_listStyle = listStyle; }
    void setSeparator(PassRefPtr<CSSPrimitiveValue> separator) { m_separator = separator; }

    bool equals(const Counter& other) const
    {
        return identifier() == other.identifier()
            && listStyle() == other.listStyle()
            && separator() == other.separator();
    }
    
    PassRefPtr<Counter> cloneForCSSOM() const
    {
        return create(m_identifier ? m_identifier->cloneForCSSOM() : 0
            , m_listStyle ? m_listStyle->cloneForCSSOM() : 0
            , m_separator ? m_separator->cloneForCSSOM() : 0);
    }

private:
    Counter(PassRefPtr<CSSPrimitiveValue> identifier, PassRefPtr<CSSPrimitiveValue> listStyle, PassRefPtr<CSSPrimitiveValue> separator)
        : m_identifier(identifier)
        , m_listStyle(listStyle)
        , m_separator(separator)
    {
    }

    RefPtr<CSSPrimitiveValue> m_identifier; // string
    RefPtr<CSSPrimitiveValue> m_listStyle; // ident
    RefPtr<CSSPrimitiveValue> m_separator; // string
};

} // namespace WebCore

#endif // Counter_h
