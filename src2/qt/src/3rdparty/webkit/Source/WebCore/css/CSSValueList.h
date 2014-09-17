/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef CSSValueList_h
#define CSSValueList_h

#include "CSSValue.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSParserValueList;

class CSSValueList : public CSSValue {
public:
    static PassRefPtr<CSSValueList> createCommaSeparated()
    {
        return adoptRef(new CSSValueList(false));
    }
    static PassRefPtr<CSSValueList> createSpaceSeparated()
    {
        return adoptRef(new CSSValueList(true));
    }
    static PassRefPtr<CSSValueList> createFromParserValueList(CSSParserValueList* list)
    {
        return adoptRef(new CSSValueList(list));
    }

    virtual ~CSSValueList();

    size_t length() const { return m_values.size(); }
    CSSValue* item(unsigned);
    CSSValue* itemWithoutBoundsCheck(unsigned index) { return m_values[index].get(); }

    void append(PassRefPtr<CSSValue>);
    void prepend(PassRefPtr<CSSValue>);
    bool removeAll(CSSValue*);
    bool hasValue(CSSValue*);
    PassRefPtr<CSSValueList> copy();

    virtual String cssText() const;

    virtual void addSubresourceStyleURLs(ListHashSet<KURL>&, const CSSStyleSheet*);

protected:
    CSSValueList(bool isSpaceSeparated);
    CSSValueList(CSSParserValueList*);

private:
    virtual bool isValueList() const { return true; }

    virtual unsigned short cssValueType() const;

    Vector<RefPtr<CSSValue> > m_values;
    bool m_isSpaceSeparated;
};

} // namespace WebCore

#endif // CSSValueList_h
