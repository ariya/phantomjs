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

#ifndef CSSInitialValue_h
#define CSSInitialValue_h

#include "CSSValue.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

class CSSInitialValue : public CSSValue {
public:
    static PassRefPtr<CSSInitialValue> createExplicit()
    {
        static CSSInitialValue* explicitValue = create(false).releaseRef();
        return explicitValue;
    }
    static PassRefPtr<CSSInitialValue> createImplicit()
    {
        static CSSInitialValue* explicitValue = create(true).releaseRef();
        return explicitValue;
    }

    virtual String cssText() const;
        
private:
    CSSInitialValue(bool implicit)
        : m_implicit(implicit)
    {
    }

    static PassRefPtr<CSSInitialValue> create(bool implicit)
    {
        return adoptRef(new CSSInitialValue(implicit));
    }

    virtual unsigned short cssValueType() const;
    virtual bool isImplicitInitialValue() const { return m_implicit; }

    bool m_implicit;
};

} // namespace WebCore

#endif // CSSInitialValue_h
