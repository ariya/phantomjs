/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef CSSMutableStyleDeclaration_h
#define CSSMutableStyleDeclaration_h

#include "CSSStyleDeclaration.h"
#include "CSSPrimitiveValue.h"
#include "CSSProperty.h"
#include "KURLHash.h"
#include "PlatformString.h"
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>

namespace WebCore {

class Node;

class CSSMutableStyleDeclarationConstIterator {
public:
    CSSMutableStyleDeclarationConstIterator(const CSSMutableStyleDeclaration* decl, CSSProperty* current);
    CSSMutableStyleDeclarationConstIterator(const CSSMutableStyleDeclarationConstIterator& o);
    ~CSSMutableStyleDeclarationConstIterator();
    
    const CSSProperty& operator*() const { return *m_current; }
    const CSSProperty* operator->() const { return m_current; }
    
    bool operator!=(const CSSMutableStyleDeclarationConstIterator& o) { ASSERT(m_decl == o.m_decl); return m_current != o.m_current; }
    bool operator==(const CSSMutableStyleDeclarationConstIterator& o) { ASSERT(m_decl == o.m_decl); return m_current == o.m_current; }
    
    CSSMutableStyleDeclarationConstIterator& operator=(const CSSMutableStyleDeclarationConstIterator& o);
    
    CSSMutableStyleDeclarationConstIterator& operator++();
    CSSMutableStyleDeclarationConstIterator& operator--();

private:
    const CSSMutableStyleDeclaration* m_decl;
    CSSProperty* m_current;
};

class CSSMutableStyleDeclaration : public CSSStyleDeclaration {
public:
    virtual ~CSSMutableStyleDeclaration();

    static PassRefPtr<CSSMutableStyleDeclaration> create()
    {
        return adoptRef(new CSSMutableStyleDeclaration);
    }
    static PassRefPtr<CSSMutableStyleDeclaration> create(CSSRule* parentRule)
    {
        return adoptRef(new CSSMutableStyleDeclaration(parentRule));
    }
    static PassRefPtr<CSSMutableStyleDeclaration> create(CSSRule* parentRule, const CSSProperty* const* properties, int numProperties)
    {
        return adoptRef(new CSSMutableStyleDeclaration(parentRule, properties, numProperties));
    }
    static PassRefPtr<CSSMutableStyleDeclaration> create(const Vector<CSSProperty>& properties)
    {
        return adoptRef(new CSSMutableStyleDeclaration(0, properties));
    }

    CSSMutableStyleDeclaration& operator=(const CSSMutableStyleDeclaration&);
    
    typedef CSSMutableStyleDeclarationConstIterator const_iterator;

    const_iterator begin() { return const_iterator(this, m_properties.begin()); }
    const_iterator end() { return const_iterator(this, m_properties.end()); }

    void setNode(Node* node) { m_node = node; }

    Node* node() const { return m_node; }

    virtual bool isMutableStyleDeclaration() const { return true; }

    virtual String cssText() const;
    virtual void setCssText(const String&, ExceptionCode&);

    virtual unsigned virtualLength() const;
    unsigned length() const { return m_properties.size(); }

    virtual String item(unsigned index) const;

    virtual PassRefPtr<CSSValue> getPropertyCSSValue(int propertyID) const;
    virtual String getPropertyValue(int propertyID) const;
    virtual bool getPropertyPriority(int propertyID) const;
    virtual int getPropertyShorthand(int propertyID) const;
    virtual bool isPropertyImplicit(int propertyID) const;

    virtual void setProperty(int propertyId, const String& value, bool important, ExceptionCode&);
    virtual String removeProperty(int propertyID, ExceptionCode&);

    virtual PassRefPtr<CSSMutableStyleDeclaration> copy() const;

    bool setProperty(int propertyID, int value, bool important = false, bool notifyChanged = true);
    bool setProperty(int propertyId, double value, CSSPrimitiveValue::UnitTypes, bool important = false, bool notifyChanged = true);
    bool setProperty(int propertyID, const String& value, bool important = false, bool notifyChanged = true);

    String removeProperty(int propertyID, bool notifyChanged = true, bool returnText = false);
 
    // setLengthProperty treats integers as pixels! (Needed for conversion of HTML attributes.)
    void setLengthProperty(int propertyId, const String& value, bool important, bool multiLength = false);
    void setStringProperty(int propertyId, const String& value, CSSPrimitiveValue::UnitTypes, bool important = false); // parsed string value
    void setImageProperty(int propertyId, const String& url, bool important = false);

    // The following parses an entire new style declaration.
    void parseDeclaration(const String& styleDeclaration);

    // Besides adding the properties, this also removes any existing properties with these IDs.
    // It does no notification since it's called by the parser.
    void addParsedProperties(const CSSProperty* const *, int numProperties);
    // This does no change notifications since it's only called by createMarkup.
    void addParsedProperty(const CSSProperty&);
 
    PassRefPtr<CSSMutableStyleDeclaration> copyBlockProperties() const;
    void removeBlockProperties();
    void removePropertiesInSet(const int* set, unsigned length, bool notifyChanged = true);

    void merge(const CSSMutableStyleDeclaration*, bool argOverridesOnConflict = true);

    void setStrictParsing(bool b) { m_strictParsing = b; }
    bool useStrictParsing() const { return m_strictParsing; }

    void addSubresourceStyleURLs(ListHashSet<KURL>&);
    
    bool propertiesEqual(const CSSMutableStyleDeclaration* o) const { return m_properties == o->m_properties; }

    bool isInlineStyleDeclaration();

protected:
    CSSMutableStyleDeclaration(CSSRule* parentRule);

private:
    CSSMutableStyleDeclaration();
    CSSMutableStyleDeclaration(CSSRule* parentRule, const Vector<CSSProperty>&);
    CSSMutableStyleDeclaration(CSSRule* parentRule, const CSSProperty* const *, int numProperties);

    virtual PassRefPtr<CSSMutableStyleDeclaration> makeMutable();

    void setNeedsStyleRecalc();

    String getShorthandValue(const int* properties, size_t) const;
    String getCommonValue(const int* properties, size_t) const;
    String getLayeredShorthandValue(const int* properties, size_t) const;
    String get4Values(const int* properties) const;
    String borderSpacingValue(const int properties[2]) const;

    template<size_t size> String getShorthandValue(const int (&properties)[size]) const { return getShorthandValue(properties, size); }
    template<size_t size> String getCommonValue(const int (&properties)[size]) const { return getCommonValue(properties, size); }
    template<size_t size> String getLayeredShorthandValue(const int (&properties)[size]) const { return getLayeredShorthandValue(properties, size); }

    void setPropertyInternal(const CSSProperty&, CSSProperty* slot = 0);
    bool removeShorthandProperty(int propertyID, bool notifyChanged);

    Vector<CSSProperty>::const_iterator findPropertyWithId(int propertyId) const;
    Vector<CSSProperty>::iterator findPropertyWithId(int propertyId);

    Vector<CSSProperty, 4> m_properties;

    Node* m_node;
    bool m_strictParsing : 1;
#ifndef NDEBUG
    unsigned m_iteratorCount : 4;
#endif

    friend class CSSMutableStyleDeclarationConstIterator;
};
    
inline CSSMutableStyleDeclarationConstIterator::CSSMutableStyleDeclarationConstIterator(const CSSMutableStyleDeclaration* decl, CSSProperty* current) 
: m_decl(decl)
, m_current(current)
{ 
#ifndef NDEBUG
    const_cast<CSSMutableStyleDeclaration*>(m_decl)->m_iteratorCount++; 
#endif
}

inline CSSMutableStyleDeclarationConstIterator::CSSMutableStyleDeclarationConstIterator(const CSSMutableStyleDeclarationConstIterator& o)
: m_decl(o.m_decl)
, m_current(o.m_current)
{
#ifndef NDEBUG
    const_cast<CSSMutableStyleDeclaration*>(m_decl)->m_iteratorCount++; 
#endif
}

inline CSSMutableStyleDeclarationConstIterator::~CSSMutableStyleDeclarationConstIterator() 
{
#ifndef NDEBUG
    const_cast<CSSMutableStyleDeclaration*>(m_decl)->m_iteratorCount--;
#endif
}

inline CSSMutableStyleDeclarationConstIterator& CSSMutableStyleDeclarationConstIterator::operator=(const CSSMutableStyleDeclarationConstIterator& o)
{
    m_decl = o.m_decl;
    m_current = o.m_current;
#ifndef NDEBUG
    const_cast<CSSMutableStyleDeclaration*>(m_decl)->m_iteratorCount++; 
#endif
    return *this;
}
    
inline CSSMutableStyleDeclarationConstIterator& CSSMutableStyleDeclarationConstIterator::operator++() 
{ 
    ASSERT(m_current != const_cast<CSSMutableStyleDeclaration*>(m_decl)->m_properties.end());
    ++m_current;
    return *this; 
}

inline CSSMutableStyleDeclarationConstIterator& CSSMutableStyleDeclarationConstIterator::operator--() 
{ 
    --m_current; 
    return *this; 
}

} // namespace WebCore

#endif // CSSMutableStyleDeclaration_h
