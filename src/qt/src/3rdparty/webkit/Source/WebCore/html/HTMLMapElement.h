/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLMapElement_h
#define HTMLMapElement_h

#include "HTMLElement.h"

namespace WebCore {

class IntSize;
class HitTestResult;
class HTMLImageElement;
    
class HTMLMapElement : public HTMLElement {
public:
    static PassRefPtr<HTMLMapElement> create(Document*);
    static PassRefPtr<HTMLMapElement> create(const QualifiedName&, Document*);
    virtual ~HTMLMapElement();

    const AtomicString& getName() const { return m_name; }

    bool mapMouseEvent(int x, int y, const IntSize&, HitTestResult&);
    
    HTMLImageElement* imageElement() const;
    PassRefPtr<HTMLCollection> areas();

private:
    HTMLMapElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    AtomicString m_name;
};

} //namespace

#endif
