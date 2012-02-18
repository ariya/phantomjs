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

#ifndef CSSQuirkPrimitiveValue_h
#define CSSQuirkPrimitiveValue_h

#include "CSSPrimitiveValue.h"

namespace WebCore {

// This value is used to handle quirky margins in reflow roots (body, td, and th) like WinIE.
// The basic idea is that a stylesheet can use the value __qem (for quirky em) instead of em.
// When the quirky value is used, if you're in quirks mode, the margin will collapse away
// inside a table cell.
class CSSQuirkPrimitiveValue : public CSSPrimitiveValue {
public:
    static PassRefPtr<CSSQuirkPrimitiveValue> create(double value, UnitTypes type)
    {
        return adoptRef(new CSSQuirkPrimitiveValue(value, type));
    }

private:
    CSSQuirkPrimitiveValue(double num, UnitTypes type)
        : CSSPrimitiveValue(num, type)
    {
    }

    virtual bool isQuirkValue() { return true; }
};

} // namespace WebCore

#endif // CSSQuirkPrimitiveValue_h
