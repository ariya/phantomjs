/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
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
 *
 */

#ifndef NamedNodeMap_h
#define NamedNodeMap_h

#include "Attribute.h"
#include "SpaceSplitString.h"

namespace WebCore {

class Node;

typedef int ExceptionCode;

class NamedNodeMap : public RefCounted<NamedNodeMap> {
    friend class Element;
public:
    static PassRefPtr<NamedNodeMap> create(Element* element = 0)
    {
        return adoptRef(new NamedNodeMap(element));
    }

    ~NamedNodeMap();

    // Public DOM interface.

    PassRefPtr<Node> getNamedItem(const String& name) const;
    PassRefPtr<Node> removeNamedItem(const String& name, ExceptionCode&);

    PassRefPtr<Node> getNamedItemNS(const String& namespaceURI, const String& localName) const;
    PassRefPtr<Node> removeNamedItemNS(const String& namespaceURI, const String& localName, ExceptionCode&);

    PassRefPtr<Node> getNamedItem(const QualifiedName& name) const;
    PassRefPtr<Node> removeNamedItem(const QualifiedName& name, ExceptionCode&);
    PassRefPtr<Node> setNamedItem(Node*, ExceptionCode&);
    PassRefPtr<Node> setNamedItemNS(Node*, ExceptionCode&);

    PassRefPtr<Node> item(unsigned index) const;
    size_t length() const { return m_attributes.size(); }
    bool isEmpty() const { return !length(); }

    // Internal interface.

    void setAttributes(const NamedNodeMap&);

    Attribute* attributeItem(unsigned index) const { return m_attributes[index].get(); }
    Attribute* getAttributeItem(const QualifiedName&) const;

    void copyAttributesToVector(Vector<RefPtr<Attribute> >&);

    void shrinkToLength() { m_attributes.shrinkCapacity(length()); }
    void reserveInitialCapacity(unsigned capacity) { m_attributes.reserveInitialCapacity(capacity); }

    // Used during parsing: only inserts if not already there. No error checking!
    void insertAttribute(PassRefPtr<Attribute> newAttribute, bool allowDuplicates)
    {
        ASSERT(!m_element);
        if (allowDuplicates || !getAttributeItem(newAttribute->name()))
            addAttribute(newAttribute);
    }

    const AtomicString& idForStyleResolution() const { return m_idForStyleResolution; }
    void setIdForStyleResolution(const AtomicString& newId) { m_idForStyleResolution = newId; }

    // FIXME: These two functions should be merged if possible.
    bool mapsEquivalent(const NamedNodeMap* otherMap) const;
    bool mappedMapsEquivalent(const NamedNodeMap* otherMap) const;

    // These functions do no error checking.
    void addAttribute(PassRefPtr<Attribute>);
    void removeAttribute(const QualifiedName&);

    Element* element() const { return m_element; }

    void clearClass() { m_classNames.clear(); }
    void setClass(const String&);
    const SpaceSplitString& classNames() const { return m_classNames; }

    bool hasMappedAttributes() const { return m_mappedAttributeCount > 0; }
    void declRemoved() { m_mappedAttributeCount--; }
    void declAdded() { m_mappedAttributeCount++; }

private:
    NamedNodeMap(Element* element) 
        : m_mappedAttributeCount(0)
        , m_element(element)
    {
    }

    void detachAttributesFromElement();
    void detachFromElement();
    Attribute* getAttributeItem(const String& name, bool shouldIgnoreAttributeCase) const;
    Attribute* getAttributeItemSlowCase(const String& name, bool shouldIgnoreAttributeCase) const;
    void clearAttributes();
    int declCount() const;

    int m_mappedAttributeCount;
    SpaceSplitString m_classNames;
    Element* m_element;
    Vector<RefPtr<Attribute> > m_attributes;
    AtomicString m_idForStyleResolution;
};

inline Attribute* NamedNodeMap::getAttributeItem(const QualifiedName& name) const
{
    unsigned len = length();
    for (unsigned i = 0; i < len; ++i) {
        if (m_attributes[i]->name().matches(name))
            return m_attributes[i].get();
    }
    return 0;
}

// We use a boolean parameter instead of calling shouldIgnoreAttributeCase so that the caller
// can tune the behavior (hasAttribute is case sensitive whereas getAttribute is not).
inline Attribute* NamedNodeMap::getAttributeItem(const String& name, bool shouldIgnoreAttributeCase) const
{
    unsigned len = length();
    bool doSlowCheck = shouldIgnoreAttributeCase;
    
    // Optimize for the case where the attribute exists and its name exactly matches.
    for (unsigned i = 0; i < len; ++i) {
        const QualifiedName& attrName = m_attributes[i]->name();
        if (!attrName.hasPrefix()) {
            if (name == attrName.localName())
                return m_attributes[i].get();
        } else
            doSlowCheck = true;
    }

    if (doSlowCheck)
        return getAttributeItemSlowCase(name, shouldIgnoreAttributeCase);
    return 0;
}

} // namespace WebCore

#endif // NamedNodeMap_h
