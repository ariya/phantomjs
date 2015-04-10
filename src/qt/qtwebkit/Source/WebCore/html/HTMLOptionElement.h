/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef HTMLOptionElement_h
#define HTMLOptionElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLDataListElement;
class HTMLSelectElement;

class HTMLOptionElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLOptionElement> create(Document*);
    static PassRefPtr<HTMLOptionElement> create(const QualifiedName&, Document*);
    static PassRefPtr<HTMLOptionElement> createForJSConstructor(Document*, const String& data, const String& value,
       bool defaultSelected, bool selected, ExceptionCode&);

    virtual String text() const;
    void setText(const String&, ExceptionCode&);

    int index() const;

    String value() const;
    void setValue(const String&);

    bool selected();
    void setSelected(bool);

#if ENABLE(DATALIST_ELEMENT)
    HTMLDataListElement* ownerDataListElement() const;
#endif
    HTMLSelectElement* ownerSelectElement() const;

    String label() const;
    void setLabel(const String&);

    bool ownElementDisabled() const { return m_disabled; }

    virtual bool isDisabledFormControl() const OVERRIDE;

    String textIndentedToRespectGroupLabel() const;

    void setSelectedState(bool);

private:
    HTMLOptionElement(const QualifiedName&, Document*);

    virtual bool isFocusable() const OVERRIDE;
    virtual bool rendererIsNeeded(const NodeRenderingContext&) { return false; }
    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void accessKeyAction(bool);

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    // <option> never has a renderer so we manually manage a cached style.
    void updateNonRenderStyle();
    virtual RenderStyle* nonRendererStyle() const OVERRIDE;
    virtual PassRefPtr<RenderStyle> customStyleForRenderer() OVERRIDE;

    void didRecalcStyle(StyleChange) OVERRIDE;

    String collectOptionInnerText() const;

    bool m_disabled;
    bool m_isSelected;
    RefPtr<RenderStyle> m_style;
};

inline bool isHTMLOptionElement(Node* node)
{
    return node->hasTagName(HTMLNames::optionTag);
}

inline bool isHTMLOptionElement(Element* element)
{
    return element->hasTagName(HTMLNames::optionTag);
}

inline HTMLOptionElement* toHTMLOptionElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLOptionElement(node));
    return static_cast<HTMLOptionElement*>(node);
}

} // namespace

#endif
