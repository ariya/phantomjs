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

#ifndef HTMLLabelElement_h
#define HTMLLabelElement_h

#include "HTMLElement.h"
#include "LabelableElement.h"

namespace WebCore {

class HTMLLabelElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLLabelElement> create(const QualifiedName&, Document*);

    LabelableElement* control();
    HTMLFormElement* form() const;

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

private:
    HTMLLabelElement(const QualifiedName&, Document*);

    virtual bool isFocusable() const OVERRIDE;

    virtual void accessKeyAction(bool sendMouseEvents);

    // Overridden to update the hover/active state of the corresponding control.
    virtual void setActive(bool = true, bool pause = false) OVERRIDE;
    virtual void setHovered(bool = true) OVERRIDE;

    // Overridden to either click() or focus() the corresponding control.
    virtual void defaultEventHandler(Event*);

    virtual void focus(bool restorePreviousSelection, FocusDirection) OVERRIDE;
};

inline bool isHTMLLabelElement(Node* node)
{
    return node->hasTagName(HTMLNames::labelTag);
}

inline bool isHTMLLabelElement(Element* element)
{
    return element->hasTagName(HTMLNames::labelTag);
}

inline HTMLLabelElement* toHTMLLabelElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLLabelElement(node));
    return static_cast<HTMLLabelElement*>(node);
}

} //namespace

#endif
