/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLKeygenElement_h
#define HTMLKeygenElement_h

#include "HTMLFormControlElementWithState.h"

namespace WebCore {

class HTMLSelectElement;

class HTMLKeygenElement FINAL : public HTMLFormControlElementWithState {
public:
    static PassRefPtr<HTMLKeygenElement> create(const QualifiedName&, Document*, HTMLFormElement*);

    virtual bool willValidate() const { return false; }

private:
    HTMLKeygenElement(const QualifiedName&, Document*, HTMLFormElement*);

    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    virtual bool canStartSelection() const { return false; }

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual bool appendFormData(FormDataList&, bool);
    virtual const AtomicString& formControlType() const;
    virtual bool isOptionalFormControl() const { return false; }

    virtual bool isEnumeratable() const { return true; }
    virtual bool supportLabels() const OVERRIDE { return true; }

    virtual void reset();
    virtual bool shouldSaveAndRestoreFormControlState() const OVERRIDE;

    HTMLSelectElement* shadowSelect() const;
};

} //namespace

#endif
