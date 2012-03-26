/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLObjectElement_h
#define HTMLObjectElement_h

#include "FormAssociatedElement.h"
#include "HTMLPlugInImageElement.h"

namespace WebCore {

class HTMLFormElement;

class HTMLObjectElement : public HTMLPlugInImageElement, public FormAssociatedElement {
public:
    static PassRefPtr<HTMLObjectElement> create(const QualifiedName&, Document*, HTMLFormElement*, bool createdByParser);
    virtual ~HTMLObjectElement();

    bool isDocNamedItem() const { return m_docNamedItem; }

    const String& classId() const { return m_classId; }

    bool containsJavaApplet() const;

    virtual bool useFallbackContent() const { return m_useFallbackContent; }
    void renderFallbackContent();

    // Implementations of FormAssociatedElement
    HTMLFormElement* form() const { return FormAssociatedElement::form(); }

    virtual bool isFormControlElement() const { return false; }

    virtual bool isEnumeratable() const { return true; }
    virtual bool appendFormData(FormDataList&, bool);

    // Implementations of constraint validation API.
    // Note that the object elements are always barred from constraint validation.
    String validationMessage() { return String(); }
    bool checkValidity() { return true; }
    void setCustomValidity(const String&) { }

    virtual void attributeChanged(Attribute*, bool preserveDecls = false);

    using TreeShared<ContainerNode>::ref;
    using TreeShared<ContainerNode>::deref;

private:
    HTMLObjectElement(const QualifiedName&, Document*, HTMLFormElement*, bool createdByParser);

    virtual void parseMappedAttribute(Attribute*);
    virtual void insertedIntoTree(bool deep);
    virtual void removedFromTree(bool deep);

    virtual bool rendererIsNeeded(RenderStyle*);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void willMoveToNewOwnerDocument();
    
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual bool isURLAttribute(Attribute*) const;
    virtual const QualifiedName& imageSourceAttributeName() const;

    virtual RenderWidget* renderWidgetForJSBindings() const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    virtual void updateWidget(PluginCreationOption);
    void updateDocNamedItem();

    bool hasFallbackContent() const;
    
    // FIXME: This function should not deal with url or serviceType
    // so that we can better share code between <object> and <embed>.
    void parametersForPlugin(Vector<String>& paramNames, Vector<String>& paramValues, String& url, String& serviceType);
    
    bool shouldAllowQuickTimeClassIdQuirk();
    bool hasValidClassId();

    virtual void refFormAssociatedElement() { ref(); }
    virtual void derefFormAssociatedElement() { deref(); }
    virtual HTMLFormElement* virtualForm() const;

    virtual const AtomicString& formControlName() const;

    AtomicString m_id;
    String m_classId;
    bool m_docNamedItem : 1;
    bool m_useFallbackContent : 1;
};

}

#endif
