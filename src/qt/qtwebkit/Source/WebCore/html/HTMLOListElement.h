/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLOListElement_h
#define HTMLOListElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLOListElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLOListElement> create(Document*);
    static PassRefPtr<HTMLOListElement> create(const QualifiedName&, Document*);

    int start() const { return m_hasExplicitStart ? m_start : (m_isReversed ? itemCount() : 1); }
    void setStart(int);

    bool isReversed() const { return m_isReversed; }

    void itemCountChanged() { m_shouldRecalculateItemCount = true; }

private:
    HTMLOListElement(const QualifiedName&, Document*);
        
    void updateItemValues();

    unsigned itemCount() const
    {
        if (m_shouldRecalculateItemCount)
            const_cast<HTMLOListElement*>(this)->recalculateItemCount();
        return m_itemCount;
    }

    void recalculateItemCount();

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) OVERRIDE;

    int m_start;
    unsigned m_itemCount;

    bool m_hasExplicitStart : 1;
    bool m_isReversed : 1;
    bool m_shouldRecalculateItemCount : 1;
};


} //namespace

#endif
