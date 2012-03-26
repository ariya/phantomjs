/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLButtonElement_h
#define HTMLButtonElement_h

#include "HTMLFormControlElement.h"

namespace WebCore {

class HTMLButtonElement : public HTMLFormControlElement {
public:
    static PassRefPtr<HTMLButtonElement> create(const QualifiedName&, Document*, HTMLFormElement*);

    String value() const;

private:
    HTMLButtonElement(const QualifiedName& tagName, Document*, HTMLFormElement*);

    enum Type { SUBMIT, RESET, BUTTON };

    virtual const AtomicString& formControlType() const;
        
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void defaultEventHandler(Event*);
    virtual bool appendFormData(FormDataList&, bool);

    virtual bool isEnumeratable() const { return true; } 

    virtual bool isSuccessfulSubmitButton() const;
    virtual bool isActivatedSubmit() const;
    virtual void setActivatedSubmit(bool flag);

    virtual void accessKeyAction(bool sendToAnyElement);
    virtual bool isURLAttribute(Attribute*) const;

    virtual bool canStartSelection() const { return false; }

    virtual bool isOptionalFormControl() const { return true; }
    virtual bool recalcWillValidate() const;

    Type m_type;
    bool m_isActivatedSubmit;
};

} // namespace

#endif
