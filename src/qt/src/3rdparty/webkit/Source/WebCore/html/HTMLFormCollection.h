/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef HTMLFormCollection_h
#define HTMLFormCollection_h

#include "HTMLCollection.h"

namespace WebCore {

class HTMLFormElement;
class QualifiedName;

// This class is just a big hack to find form elements even in malformed HTML elements.
// The famous <table><tr><form><td> problem.

class HTMLFormCollection : public HTMLCollection {
public:
    static PassRefPtr<HTMLFormCollection> create(PassRefPtr<HTMLFormElement>);

    virtual ~HTMLFormCollection();

    virtual Node* item(unsigned index) const;
    virtual Node* nextItem() const;

    virtual Node* namedItem(const AtomicString& name) const;
    virtual Node* nextNamedItem(const AtomicString& name) const;

private:
    HTMLFormCollection(PassRefPtr<HTMLFormElement>);

    virtual void updateNameCache() const;
    virtual unsigned calcLength() const;

    static CollectionCache* formCollectionInfo(HTMLFormElement*);

    Element* getNamedItem(const QualifiedName& attrName, const AtomicString& name) const;
    Element* nextNamedItemInternal(const String& name) const;

    Element* getNamedFormItem(const QualifiedName& attrName, const String& name, int duplicateNumber) const;

    mutable int currentPos;
};

} //namespace

#endif
