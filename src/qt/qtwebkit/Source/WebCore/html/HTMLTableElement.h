/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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

#ifndef HTMLTableElement_h
#define HTMLTableElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLCollection;
class HTMLTableCaptionElement;
class HTMLTableRowsCollection;
class HTMLTableSectionElement;

class HTMLTableElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLTableElement> create(Document*);
    static PassRefPtr<HTMLTableElement> create(const QualifiedName&, Document*);

    HTMLTableCaptionElement* caption() const;
    void setCaption(PassRefPtr<HTMLTableCaptionElement>, ExceptionCode&);

    HTMLTableSectionElement* tHead() const;
    void setTHead(PassRefPtr<HTMLTableSectionElement>, ExceptionCode&);

    HTMLTableSectionElement* tFoot() const;
    void setTFoot(PassRefPtr<HTMLTableSectionElement>, ExceptionCode&);

    PassRefPtr<HTMLElement> createTHead();
    void deleteTHead();
    PassRefPtr<HTMLElement> createTFoot();
    void deleteTFoot();
    PassRefPtr<HTMLElement> createTBody();
    PassRefPtr<HTMLElement> createCaption();
    void deleteCaption();
    PassRefPtr<HTMLElement> insertRow(int index, ExceptionCode&);
    void deleteRow(int index, ExceptionCode&);

    PassRefPtr<HTMLCollection> rows();
    PassRefPtr<HTMLCollection> tBodies();

    String rules() const;
    String summary() const;

    const StylePropertySet* additionalCellStyle();
    const StylePropertySet* additionalGroupStyle(bool rows);

private:
    HTMLTableElement(const QualifiedName&, Document*);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) OVERRIDE;
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;

    // Used to obtain either a solid or outset border decl and to deal with the frame and rules attributes.
    virtual const StylePropertySet* additionalPresentationAttributeStyle() OVERRIDE;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    enum TableRules { UnsetRules, NoneRules, GroupsRules, RowsRules, ColsRules, AllRules };
    enum CellBorders { NoBorders, SolidBorders, InsetBorders, SolidBordersColsOnly, SolidBordersRowsOnly };

    CellBorders cellBorders() const;

    PassRefPtr<StylePropertySet> createSharedCellStyle();

    HTMLTableSectionElement* lastBody() const;

    bool m_borderAttr;          // Sets a precise border width and creates an outset border for the table and for its cells.
    bool m_borderColorAttr;     // Overrides the outset border and makes it solid for the table and cells instead.
    bool m_frameAttr;           // Implies a thin border width if no border is set and then a certain set of solid/hidden borders based off the value.
    TableRules m_rulesAttr;     // Implies a thin border width, a collapsing border model, and all borders on the table becoming set to hidden (if frame/border
                                // are present, to none otherwise).

    unsigned short m_padding;
    RefPtr<StylePropertySet> m_sharedCellStyle;
};

inline bool isHTMLTableElement(const Node* node)
{
    return node->hasTagName(HTMLNames::tableTag);
}

inline bool isHTMLTableElement(const Element* element)
{
    return element->hasTagName(HTMLNames::tableTag);
}

inline HTMLTableElement* toHTMLTableElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLTableElement(node));
    return static_cast<HTMLTableElement*>(node);
}

} //namespace

#endif
