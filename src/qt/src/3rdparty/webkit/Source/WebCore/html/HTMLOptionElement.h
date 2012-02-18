/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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

#include "HTMLFormControlElement.h"
#include "OptionElement.h"

namespace WebCore {

class HTMLSelectElement;

class HTMLOptionElement : public HTMLFormControlElement, public OptionElement {
    friend class HTMLSelectElement;
    friend class RenderMenuList;

public:
    static PassRefPtr<HTMLOptionElement> create(Document*, HTMLFormElement*);
    static PassRefPtr<HTMLOptionElement> create(const QualifiedName&, Document*, HTMLFormElement*);
    static PassRefPtr<HTMLOptionElement> createForJSConstructor(Document*, const String& data, const String& value,
       bool defaultSelected, bool selected, ExceptionCode&);

    virtual String text() const;
    void setText(const String&, ExceptionCode&);

    int index() const;

    virtual String value() const;
    void setValue(const String&);

    virtual bool selected() const;
    void setSelected(bool);

    HTMLSelectElement* ownerSelectElement() const;

    bool defaultSelected() const;
    void setDefaultSelected(bool);

    String label() const;

    bool ownElementDisabled() const { return HTMLFormControlElement::disabled(); }

    virtual bool disabled() const;

private:
    HTMLOptionElement(const QualifiedName&, Document*, HTMLFormElement* = 0);

    virtual bool supportsFocus() const;
    virtual bool isFocusable() const;
    virtual bool rendererIsNeeded(RenderStyle*) { return false; }
    virtual void attach();
    virtual void detach();
    virtual void setRenderStyle(PassRefPtr<RenderStyle>);

    virtual const AtomicString& formControlType() const;

    virtual void parseMappedAttribute(Attribute*);

    virtual void setSelectedState(bool);

    virtual String textIndentedToRespectGroupLabel() const;

    virtual void insertedIntoTree(bool);
    virtual void accessKeyAction(bool);

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual RenderStyle* nonRendererRenderStyle() const;

    OptionElementData m_data;
    RefPtr<RenderStyle> m_style;
};

} //namespace

#endif
