/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HTMLTableElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLTableCaptionElement.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableRowsCollection.h"
#include "HTMLTableSectionElement.h"
#include "RenderTable.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

HTMLTableElement::HTMLTableElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_borderAttr(false)
    , m_borderColorAttr(false)
    , m_frameAttr(false)
    , m_rulesAttr(UnsetRules)
    , m_padding(1)
{
    ASSERT(hasTagName(tableTag));
}

PassRefPtr<HTMLTableElement> HTMLTableElement::create(Document* document)
{
    return adoptRef(new HTMLTableElement(tableTag, document));
}

PassRefPtr<HTMLTableElement> HTMLTableElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTableElement(tagName, document));
}

HTMLTableCaptionElement* HTMLTableElement::caption() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(captionTag))
            return static_cast<HTMLTableCaptionElement*>(child);
    }
    return 0;
}

void HTMLTableElement::setCaption(PassRefPtr<HTMLTableCaptionElement> newCaption, ExceptionCode& ec)
{
    deleteCaption();
    insertBefore(newCaption, firstChild(), ec);
}

HTMLTableSectionElement* HTMLTableElement::tHead() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(theadTag))
            return static_cast<HTMLTableSectionElement*>(child);
    }
    return 0;
}

void HTMLTableElement::setTHead(PassRefPtr<HTMLTableSectionElement> newHead, ExceptionCode& ec)
{
    deleteTHead();

    Node* child;
    for (child = firstChild(); child; child = child->nextSibling())
        if (child->isElementNode() && !child->hasTagName(captionTag) && !child->hasTagName(colgroupTag))
            break;

    insertBefore(newHead, child, ec);
}

HTMLTableSectionElement* HTMLTableElement::tFoot() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(tfootTag))
            return static_cast<HTMLTableSectionElement*>(child);
    }
    return 0;
}

void HTMLTableElement::setTFoot(PassRefPtr<HTMLTableSectionElement> newFoot, ExceptionCode& ec)
{
    deleteTFoot();

    Node* child;
    for (child = firstChild(); child; child = child->nextSibling())
        if (child->isElementNode() && !child->hasTagName(captionTag) && !child->hasTagName(colgroupTag) && !child->hasTagName(theadTag))
            break;

    insertBefore(newFoot, child, ec);
}

PassRefPtr<HTMLElement> HTMLTableElement::createTHead()
{
    if (HTMLTableSectionElement* existingHead = tHead())
        return existingHead;
    RefPtr<HTMLTableSectionElement> head = HTMLTableSectionElement::create(theadTag, document());
    ExceptionCode ec;
    setTHead(head, ec);
    return head.release();
}

void HTMLTableElement::deleteTHead()
{
    ExceptionCode ec;
    removeChild(tHead(), ec);
}

PassRefPtr<HTMLElement> HTMLTableElement::createTFoot()
{
    if (HTMLTableSectionElement* existingFoot = tFoot())
        return existingFoot;
    RefPtr<HTMLTableSectionElement> foot = HTMLTableSectionElement::create(tfootTag, document());
    ExceptionCode ec;
    setTFoot(foot, ec);
    return foot.release();
}

void HTMLTableElement::deleteTFoot()
{
    ExceptionCode ec;
    removeChild(tFoot(), ec);
}

PassRefPtr<HTMLElement> HTMLTableElement::createCaption()
{
    if (HTMLTableCaptionElement* existingCaption = caption())
        return existingCaption;
    RefPtr<HTMLTableCaptionElement> caption = HTMLTableCaptionElement::create(captionTag, document());
    ExceptionCode ec;
    setCaption(caption, ec);
    return caption.release();
}

void HTMLTableElement::deleteCaption()
{
    ExceptionCode ec;
    removeChild(caption(), ec);
}

HTMLTableSectionElement* HTMLTableElement::lastBody() const
{
    for (Node* child = lastChild(); child; child = child->previousSibling()) {
        if (child->hasTagName(tbodyTag))
            return static_cast<HTMLTableSectionElement*>(child);
    }
    return 0;
}

PassRefPtr<HTMLElement> HTMLTableElement::insertRow(int index, ExceptionCode& ec)
{
    if (index < -1) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }

    HTMLTableRowElement* lastRow = 0;
    HTMLTableRowElement* row = 0;
    if (index == -1)
        lastRow = HTMLTableRowsCollection::lastRow(this);
    else {
        for (int i = 0; i <= index; ++i) {
            row = HTMLTableRowsCollection::rowAfter(this, lastRow);
            if (!row) {
                if (i != index) {
                    ec = INDEX_SIZE_ERR;
                    return 0;
                }
                break;
            }
            lastRow = row;
        }
    }

    ContainerNode* parent;
    if (lastRow)
        parent = row ? row->parentNode() : lastRow->parentNode();
    else {
        parent = lastBody();
        if (!parent) {
            RefPtr<HTMLTableSectionElement> newBody = HTMLTableSectionElement::create(tbodyTag, document());
            RefPtr<HTMLTableRowElement> newRow = HTMLTableRowElement::create(document());
            newBody->appendChild(newRow, ec);
            appendChild(newBody.release(), ec);
            return newRow.release();
        }
    }

    RefPtr<HTMLTableRowElement> newRow = HTMLTableRowElement::create(document());
    parent->insertBefore(newRow, row, ec);
    return newRow.release();
}

void HTMLTableElement::deleteRow(int index, ExceptionCode& ec)
{
    HTMLTableRowElement* row = 0;
    if (index == -1)
        row = HTMLTableRowsCollection::lastRow(this);
    else {
        for (int i = 0; i <= index; ++i) {
            row = HTMLTableRowsCollection::rowAfter(this, row);
            if (!row)
                break;
        }
    }
    if (!row) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    row->remove(ec);
}

bool HTMLTableElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == backgroundAttr) {
        result = (MappedAttributeEntry)(eLastEntry + document()->docID());
        return false;
    }
    
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == bgcolorAttr ||
        attrName == cellspacingAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr ||
        attrName == valignAttr) {
        result = eUniversal;
        return false;
    }
    
    if (attrName == bordercolorAttr || attrName == frameAttr || attrName == rulesAttr) {
        result = eUniversal;
        return true;
    }
    
    if (attrName == borderAttr) {
        result = eTable;
        return true;
    }
    
    if (attrName == alignAttr) {
        result = eTable;
        return false;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

static inline bool isTableCellAncestor(Node* n)
{
    return n->hasTagName(theadTag) || n->hasTagName(tbodyTag) ||
           n->hasTagName(tfootTag) || n->hasTagName(trTag) ||
           n->hasTagName(thTag);
}

static bool setTableCellsChanged(Node* n)
{
    ASSERT(n);
    bool cellChanged = false;

    if (n->hasTagName(tdTag))
        cellChanged = true;
    else if (isTableCellAncestor(n)) {
        for (Node* child = n->firstChild(); child; child = child->nextSibling())
            cellChanged |= setTableCellsChanged(child);
    }

    if (cellChanged)
       n->setNeedsStyleRecalc();

    return cellChanged;
}

void HTMLTableElement::parseMappedAttribute(Attribute* attr)
{
    CellBorders bordersBefore = cellBorders();
    unsigned short oldPadding = m_padding;

    if (attr->name() == widthAttr)
        addCSSLength(attr, CSSPropertyWidth, attr->value());
    else if (attr->name() == heightAttr)
        addCSSLength(attr, CSSPropertyHeight, attr->value());
    else if (attr->name() == borderAttr)  {
        m_borderAttr = true;
        if (attr->decl()) {
            RefPtr<CSSValue> val = attr->decl()->getPropertyCSSValue(CSSPropertyBorderLeftWidth);
            if (val && val->isPrimitiveValue()) {
                CSSPrimitiveValue* primVal = static_cast<CSSPrimitiveValue*>(val.get());
                m_borderAttr = primVal->getDoubleValue(CSSPrimitiveValue::CSS_NUMBER);
            }
        } else if (!attr->isNull()) {
            int border = 0;
            if (attr->isEmpty())
                border = 1;
            else
                border = attr->value().toInt();
            m_borderAttr = border;
            addCSSLength(attr, CSSPropertyBorderWidth, String::number(border));
        }
    } else if (attr->name() == bgcolorAttr)
        addCSSColor(attr, CSSPropertyBackgroundColor, attr->value());
    else if (attr->name() == bordercolorAttr) {
        m_borderColorAttr = attr->decl();
        if (!attr->decl() && !attr->isEmpty()) {
            addCSSColor(attr, CSSPropertyBorderColor, attr->value());
            m_borderColorAttr = true;
        }
    } else if (attr->name() == backgroundAttr) {
        String url = stripLeadingAndTrailingHTMLSpaces(attr->value());
        if (!url.isEmpty())
            addCSSImageProperty(attr, CSSPropertyBackgroundImage, document()->completeURL(url).string());
    } else if (attr->name() == frameAttr) {
        // Cache the value of "frame" so that the table can examine it later.
        m_frameAttr = false;
        
        // Whether or not to hide the top/right/bottom/left borders.
        const int cTop = 0;
        const int cRight = 1;
        const int cBottom = 2;
        const int cLeft = 3;
        bool borders[4] = { false, false, false, false };
        
        // void, above, below, hsides, vsides, lhs, rhs, box, border
        if (equalIgnoringCase(attr->value(), "void"))
            m_frameAttr = true;
        else if (equalIgnoringCase(attr->value(), "above")) {
            m_frameAttr = true;
            borders[cTop] = true;
        } else if (equalIgnoringCase(attr->value(), "below")) {
            m_frameAttr = true;
            borders[cBottom] = true;
        } else if (equalIgnoringCase(attr->value(), "hsides")) {
            m_frameAttr = true;
            borders[cTop] = borders[cBottom] = true;
        } else if (equalIgnoringCase(attr->value(), "vsides")) {
            m_frameAttr = true;
            borders[cLeft] = borders[cRight] = true;
        } else if (equalIgnoringCase(attr->value(), "lhs")) {
            m_frameAttr = true;
            borders[cLeft] = true;
        } else if (equalIgnoringCase(attr->value(), "rhs")) {
            m_frameAttr = true;
            borders[cRight] = true;
        } else if (equalIgnoringCase(attr->value(), "box") ||
                   equalIgnoringCase(attr->value(), "border")) {
            m_frameAttr = true;
            borders[cTop] = borders[cBottom] = borders[cLeft] = borders[cRight] = true;
        }
        
        // Now map in the border styles of solid and hidden respectively.
        if (m_frameAttr) {
            addCSSProperty(attr, CSSPropertyBorderTopWidth, CSSValueThin);
            addCSSProperty(attr, CSSPropertyBorderBottomWidth, CSSValueThin);
            addCSSProperty(attr, CSSPropertyBorderLeftWidth, CSSValueThin);
            addCSSProperty(attr, CSSPropertyBorderRightWidth, CSSValueThin);
            addCSSProperty(attr, CSSPropertyBorderTopStyle, borders[cTop] ? CSSValueSolid : CSSValueHidden);
            addCSSProperty(attr, CSSPropertyBorderBottomStyle, borders[cBottom] ? CSSValueSolid : CSSValueHidden);
            addCSSProperty(attr, CSSPropertyBorderLeftStyle, borders[cLeft] ? CSSValueSolid : CSSValueHidden);
            addCSSProperty(attr, CSSPropertyBorderRightStyle, borders[cRight] ? CSSValueSolid : CSSValueHidden);
        }
    } else if (attr->name() == rulesAttr) {
        m_rulesAttr = UnsetRules;
        if (equalIgnoringCase(attr->value(), "none"))
            m_rulesAttr = NoneRules;
        else if (equalIgnoringCase(attr->value(), "groups"))
            m_rulesAttr = GroupsRules;
        else if (equalIgnoringCase(attr->value(), "rows"))
            m_rulesAttr = RowsRules;
        if (equalIgnoringCase(attr->value(), "cols"))
            m_rulesAttr = ColsRules;
        if (equalIgnoringCase(attr->value(), "all"))
            m_rulesAttr = AllRules;
        
        // The presence of a valid rules attribute causes border collapsing to be enabled.
        if (m_rulesAttr != UnsetRules)
            addCSSProperty(attr, CSSPropertyBorderCollapse, CSSValueCollapse);
    } else if (attr->name() == cellspacingAttr) {
        if (!attr->value().isEmpty())
            addCSSLength(attr, CSSPropertyBorderSpacing, attr->value());
    } else if (attr->name() == cellpaddingAttr) {
        if (!attr->value().isEmpty())
            m_padding = max(0, attr->value().toInt());
        else
            m_padding = 1;
    } else if (attr->name() == colsAttr) {
        // ###
    } else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginTop, attr->value());
        addCSSLength(attr, CSSPropertyMarginBottom, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginLeft, attr->value());
        addCSSLength(attr, CSSPropertyMarginRight, attr->value());
    } else if (attr->name() == alignAttr) {
        if (!attr->value().isEmpty()) {
            if (equalIgnoringCase(attr->value(), "center")) {
                addCSSProperty(attr, CSSPropertyWebkitMarginStart, CSSValueAuto);
                addCSSProperty(attr, CSSPropertyWebkitMarginEnd, CSSValueAuto);
            } else
                addCSSProperty(attr, CSSPropertyFloat, attr->value());
        }
    } else if (attr->name() == valignAttr) {
        if (!attr->value().isEmpty())
            addCSSProperty(attr, CSSPropertyVerticalAlign, attr->value());
    } else
        HTMLElement::parseMappedAttribute(attr);

    if (bordersBefore != cellBorders() || oldPadding != m_padding) {
        if (oldPadding != m_padding)
            m_paddingDecl = 0;
        bool cellChanged = false;
        for (Node* child = firstChild(); child; child = child->nextSibling())
            cellChanged |= setTableCellsChanged(child);
        if (cellChanged)
            setNeedsStyleRecalc();
    }
}

void HTMLTableElement::additionalAttributeStyleDecls(Vector<CSSMutableStyleDeclaration*>& results)
{
    if ((!m_borderAttr && !m_borderColorAttr) || m_frameAttr)
        return;

    AtomicString borderValue = m_borderColorAttr ? "solid" : "outset";
    CSSMappedAttributeDeclaration* decl = getMappedAttributeDecl(ePersistent, tableborderAttr, borderValue);
    if (!decl) {
        decl = CSSMappedAttributeDeclaration::create().releaseRef(); // This single ref pins us in the table until the document dies.
        decl->setParent(document()->elementSheet());
        decl->setNode(this);
        decl->setStrictParsing(false); // Mapped attributes are just always quirky.
        
        int v = m_borderColorAttr ? CSSValueSolid : CSSValueOutset;
        decl->setProperty(CSSPropertyBorderTopStyle, v, false);
        decl->setProperty(CSSPropertyBorderBottomStyle, v, false);
        decl->setProperty(CSSPropertyBorderLeftStyle, v, false);
        decl->setProperty(CSSPropertyBorderRightStyle, v, false);

        setMappedAttributeDecl(ePersistent, tableborderAttr, borderValue, decl);
        decl->setParent(0);
        decl->setNode(0);
        decl->setMappedState(ePersistent, tableborderAttr, borderValue);
    }
    
    
    results.append(decl);
}

HTMLTableElement::CellBorders HTMLTableElement::cellBorders() const
{
    switch (m_rulesAttr) {
        case NoneRules:
        case GroupsRules:
            return NoBorders;
        case AllRules:
            return SolidBorders;
        case ColsRules:
            return SolidBordersColsOnly;
        case RowsRules:
            return SolidBordersRowsOnly;
        case UnsetRules:
            if (!m_borderAttr)
                return NoBorders;
            if (m_borderColorAttr)
                return SolidBorders;
            return InsetBorders;
    }
    ASSERT_NOT_REACHED();
    return NoBorders;
}

void HTMLTableElement::addSharedCellDecls(Vector<CSSMutableStyleDeclaration*>& results)
{
    addSharedCellBordersDecl(results);
    addSharedCellPaddingDecl(results);
}

void HTMLTableElement::addSharedCellBordersDecl(Vector<CSSMutableStyleDeclaration*>& results)
{
    CellBorders borders = cellBorders();

    static const AtomicString* cellBorderNames[] = { new AtomicString("none"), new AtomicString("solid"), new AtomicString("inset"), new AtomicString("solid-cols"), new AtomicString("solid-rows") };
    const AtomicString& cellborderValue = *cellBorderNames[borders];
    CSSMappedAttributeDeclaration* decl = getMappedAttributeDecl(ePersistent, cellborderAttr, cellborderValue);
    if (!decl) {
        decl = CSSMappedAttributeDeclaration::create().releaseRef(); // This single ref pins us in the table until the document dies.
        decl->setParent(document()->elementSheet());
        decl->setNode(this);
        decl->setStrictParsing(false); // Mapped attributes are just always quirky.
        
        switch (borders) {
            case SolidBordersColsOnly:
                decl->setProperty(CSSPropertyBorderLeftWidth, CSSValueThin, false);
                decl->setProperty(CSSPropertyBorderRightWidth, CSSValueThin, false);
                decl->setProperty(CSSPropertyBorderLeftStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderRightStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderColor, "inherit", false);
                break;
            case SolidBordersRowsOnly:
                decl->setProperty(CSSPropertyBorderTopWidth, CSSValueThin, false);
                decl->setProperty(CSSPropertyBorderBottomWidth, CSSValueThin, false);
                decl->setProperty(CSSPropertyBorderTopStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderBottomStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderColor, "inherit", false);
                break;
            case SolidBorders:
                decl->setProperty(CSSPropertyBorderWidth, "1px", false);
                decl->setProperty(CSSPropertyBorderTopStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderBottomStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderLeftStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderRightStyle, CSSValueSolid, false);
                decl->setProperty(CSSPropertyBorderColor, "inherit", false);
                break;
            case InsetBorders:
                decl->setProperty(CSSPropertyBorderWidth, "1px", false);
                decl->setProperty(CSSPropertyBorderTopStyle, CSSValueInset, false);
                decl->setProperty(CSSPropertyBorderBottomStyle, CSSValueInset, false);
                decl->setProperty(CSSPropertyBorderLeftStyle, CSSValueInset, false);
                decl->setProperty(CSSPropertyBorderRightStyle, CSSValueInset, false);
                decl->setProperty(CSSPropertyBorderColor, "inherit", false);
                break;
            case NoBorders:
                decl->setProperty(CSSPropertyBorderWidth, "0", false);
                break;
        }

        setMappedAttributeDecl(ePersistent, cellborderAttr, *cellBorderNames[borders], decl);
        decl->setParent(0);
        decl->setNode(0);
        decl->setMappedState(ePersistent, cellborderAttr, cellborderValue);
    }
    
    results.append(decl);
}

void HTMLTableElement::addSharedCellPaddingDecl(Vector<CSSMutableStyleDeclaration*>& results)
{
    if (m_padding == 0)
        return;

    if (!m_paddingDecl) {
        String paddingValue = String::number(m_padding);
        m_paddingDecl = getMappedAttributeDecl(eUniversal, cellpaddingAttr, paddingValue);
        if (!m_paddingDecl) {
            m_paddingDecl = CSSMappedAttributeDeclaration::create();
            m_paddingDecl->setParent(document()->elementSheet());
            m_paddingDecl->setNode(this);
            m_paddingDecl->setStrictParsing(false); // Mapped attributes are just always quirky.
            
            m_paddingDecl->setProperty(CSSPropertyPaddingTop, paddingValue, false);
            m_paddingDecl->setProperty(CSSPropertyPaddingRight, paddingValue, false);
            m_paddingDecl->setProperty(CSSPropertyPaddingBottom, paddingValue, false);
            m_paddingDecl->setProperty(CSSPropertyPaddingLeft, paddingValue, false);
        }
        setMappedAttributeDecl(eUniversal, cellpaddingAttr, paddingValue, m_paddingDecl.get());
        m_paddingDecl->setParent(0);
        m_paddingDecl->setNode(0);
        m_paddingDecl->setMappedState(eUniversal, cellpaddingAttr, paddingValue);
    }
    
    results.append(m_paddingDecl.get());
}

void HTMLTableElement::addSharedGroupDecls(bool rows, Vector<CSSMutableStyleDeclaration*>& results)
{
    if (m_rulesAttr != GroupsRules)
        return;

    AtomicString rulesValue = rows ? "rowgroups" : "colgroups";
    CSSMappedAttributeDeclaration* decl = getMappedAttributeDecl(ePersistent, rulesAttr, rulesValue);
    if (!decl) {
        decl = CSSMappedAttributeDeclaration::create().releaseRef(); // This single ref pins us in the table until the document dies.
        decl->setParent(document()->elementSheet());
        decl->setNode(this);
        decl->setStrictParsing(false); // Mapped attributes are just always quirky.
        
        if (rows) {
            decl->setProperty(CSSPropertyBorderTopWidth, CSSValueThin, false);
            decl->setProperty(CSSPropertyBorderBottomWidth, CSSValueThin, false);
            decl->setProperty(CSSPropertyBorderTopStyle, CSSValueSolid, false);
            decl->setProperty(CSSPropertyBorderBottomStyle, CSSValueSolid, false);
        } else {
            decl->setProperty(CSSPropertyBorderLeftWidth, CSSValueThin, false);
            decl->setProperty(CSSPropertyBorderRightWidth, CSSValueThin, false);
            decl->setProperty(CSSPropertyBorderLeftStyle, CSSValueSolid, false);
            decl->setProperty(CSSPropertyBorderRightStyle, CSSValueSolid, false);
        }

        setMappedAttributeDecl(ePersistent, rulesAttr, rulesValue, decl);
        decl->setParent(0);
        decl->setNode(0);
        decl->setMappedState(ePersistent, rulesAttr, rulesValue);
    }

    results.append(decl);
}

void HTMLTableElement::attach()
{
    ASSERT(!attached());
    HTMLElement::attach();
}

bool HTMLTableElement::isURLAttribute(Attribute *attr) const
{
    return attr->name() == backgroundAttr;
}

PassRefPtr<HTMLCollection> HTMLTableElement::rows()
{
    return HTMLTableRowsCollection::create(this);
}

PassRefPtr<HTMLCollection> HTMLTableElement::tBodies()
{
    return HTMLCollection::create(this, TableTBodies);
}

String HTMLTableElement::rules() const
{
    return getAttribute(rulesAttr);
}

String HTMLTableElement::summary() const
{
    return getAttribute(summaryAttr);
}

void HTMLTableElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(getAttribute(backgroundAttr)));
}

}
