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

#ifndef CSSPropertyLonghand_h
#define CSSPropertyLonghand_h

namespace WebCore {

class CSSPropertyLonghand {
public:
    CSSPropertyLonghand()
        : m_properties(0)
        , m_length(0)
    {
    }

    CSSPropertyLonghand(const int* firstProperty, unsigned numProperties)
        : m_properties(firstProperty)
        , m_length(numProperties)
    {
    }

    const int* properties() const { return m_properties; }
    unsigned length() const { return m_length; }

private:
    const int* m_properties;
    unsigned m_length;
};

// Returns an empty list if the property is not a shorthand
CSSPropertyLonghand longhandForProperty(int);

} // namespace WebCore

#endif // CSSPropertyLonghand_h
