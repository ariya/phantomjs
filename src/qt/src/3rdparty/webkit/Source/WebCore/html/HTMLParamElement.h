/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLParamElement_h
#define HTMLParamElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLParamElement : public HTMLElement {
public:
    static PassRefPtr<HTMLParamElement> create(const QualifiedName&, Document*);

    String name() const { return m_name; }
    String value() const { return m_value; }

    static bool isURLParameter(const String&);

private:
    HTMLParamElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);

    virtual bool isURLAttribute(Attribute*) const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    // FIXME: These don't need to be stored as members and instead
    // name() value() could use getAttribute(nameAttr/valueAttr).
    AtomicString m_name;
    AtomicString m_value;
};

} // namespace WebCore

#endif
