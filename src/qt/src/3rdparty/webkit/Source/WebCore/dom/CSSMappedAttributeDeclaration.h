/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
 *
 */

#ifndef CSSMappedAttributeDeclaration_h
#define CSSMappedAttributeDeclaration_h

#include "CSSMutableStyleDeclaration.h"
#include "MappedAttributeEntry.h"
#include "QualifiedName.h"

namespace WebCore {

class CSSMappedAttributeDeclaration : public CSSMutableStyleDeclaration {
public:
    static PassRefPtr<CSSMappedAttributeDeclaration> create()
    {
        return adoptRef(new CSSMappedAttributeDeclaration(0));
    }

    virtual ~CSSMappedAttributeDeclaration();

    void setMappedState(MappedAttributeEntry type, const QualifiedName& name, const AtomicString& val)
    {
        m_entryType = type;
        m_attrName = name;
        m_attrValue = val;
    }

private:
    CSSMappedAttributeDeclaration(CSSRule* parentRule)
        : CSSMutableStyleDeclaration(parentRule)
        , m_entryType(eNone)
        , m_attrName(anyQName())
    {
    }
    
    MappedAttributeEntry m_entryType;
    QualifiedName m_attrName;
    AtomicString m_attrValue;
};

} //namespace

#endif
