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

#ifndef FontFamilyValue_h
#define FontFamilyValue_h

#include "CSSPrimitiveValue.h"
#include "PlatformString.h"

namespace WebCore {

class FontFamilyValue : public CSSPrimitiveValue {
public:
    static PassRefPtr<FontFamilyValue> create(const String& familyName)
    {
        return adoptRef(new FontFamilyValue(familyName));
    }

    void appendSpaceSeparated(const UChar* characters, unsigned length);

    const String& familyName() const { return m_familyName; }

    virtual String cssText() const;

private:
    FontFamilyValue(const String& familyName);
    virtual bool isFontFamilyValue() const { return true; }

    String m_familyName;
};

} // namespace

#endif
