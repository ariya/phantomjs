/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLFormControlElementWithState_h
#define HTMLFormControlElementWithState_h

#include "HTMLFormControlElement.h"

namespace WebCore {

class FormControlState;

class HTMLFormControlElementWithState : public HTMLFormControlElement {
public:
    virtual ~HTMLFormControlElementWithState();

    virtual bool canContainRangeEndPoint() const { return false; }

    virtual bool shouldSaveAndRestoreFormControlState() const;
    virtual FormControlState saveFormControlState() const;
    // The specified FormControlState must have at least one string value.
    virtual void restoreFormControlState(const FormControlState&) { }
    void notifyFormStateChanged();

protected:
    HTMLFormControlElementWithState(const QualifiedName& tagName, Document*, HTMLFormElement*);

    virtual bool shouldAutocomplete() const;
    virtual void finishParsingChildren();
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual bool isFormControlElementWithState() const OVERRIDE;
};

} // namespace

#endif
