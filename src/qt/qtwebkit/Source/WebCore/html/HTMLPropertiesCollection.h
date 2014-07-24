/*
 * Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLPropertiesCollection_h
#define HTMLPropertiesCollection_h

#if ENABLE(MICRODATA)

#include "DOMStringList.h"
#include "HTMLCollection.h"

namespace WebCore {

class PropertyNodeList;

class HTMLPropertiesCollection : public HTMLCollection {
public:
    static PassRefPtr<HTMLPropertiesCollection> create(Node*, CollectionType);
    virtual ~HTMLPropertiesCollection();

    void updateRefElements() const;

    PassRefPtr<DOMStringList> names() const;
    virtual PassRefPtr<PropertyNodeList> propertyNodeList(const String&) const;
    virtual bool hasNamedItem(const AtomicString&) const OVERRIDE;

    void invalidateCache() const
    {
        m_itemRefElements.clear();
        m_propertyNames.clear();
    }

private:
    HTMLPropertiesCollection(Node*);

    virtual Element* virtualItemAfter(unsigned& offsetInArray, Element*) const OVERRIDE;
    HTMLElement* virtualItemAfter(HTMLElement* base, Element* previous) const;

    void updateNameCache() const;

    void updatePropertyCache(const AtomicString& propertyName) const
    {
        if (!m_propertyNames)
            m_propertyNames = DOMStringList::create();

        if (!m_propertyNames->contains(propertyName))
            m_propertyNames->append(propertyName);
    }

    mutable Vector<HTMLElement*> m_itemRefElements;
    mutable RefPtr<DOMStringList> m_propertyNames;
};

} // namespace WebCore

#endif // ENABLE(MICRODATA)

#endif // HTMLPropertiesCollection_h
