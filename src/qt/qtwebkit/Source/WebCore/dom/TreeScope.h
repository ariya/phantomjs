/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 * Copyright (C) 2012, 2013 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TreeScope_h
#define TreeScope_h

#include "DocumentOrderedMap.h"
#include <wtf/Forward.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class ContainerNode;
class DOMSelection;
class Document;
class Element;
class HTMLLabelElement;
class HTMLMapElement;
class LayoutPoint;
class IdTargetObserverRegistry;
class Node;

// A class which inherits both Node and TreeScope must call clearRareData() in its destructor
// so that the Node destructor no longer does problematic NodeList cache manipulation in
// the destructor.
class TreeScope {
    friend class Document;
    friend class TreeScopeAdopter;

public:
    TreeScope* parentTreeScope() const { return m_parentTreeScope; }
    void setParentTreeScope(TreeScope*);

    Element* focusedElement();
    Element* getElementById(const AtomicString&) const;
    const Vector<Element*>* getAllElementsById(const AtomicString&) const;
    bool hasElementWithId(AtomicStringImpl* id) const;
    bool containsMultipleElementsWithId(const AtomicString& id) const;
    void addElementById(const AtomicString& elementId, Element*);
    void removeElementById(const AtomicString& elementId, Element*);

    Element* getElementByName(const AtomicString&) const;
    bool hasElementWithName(AtomicStringImpl*) const;
    bool containsMultipleElementsWithName(const AtomicString&) const;
    void addElementByName(const AtomicString&, Element*);
    void removeElementByName(const AtomicString&, Element*);

    Document* documentScope() const { return m_documentScope; }

    Node* ancestorInThisScope(Node*) const;

    void addImageMap(HTMLMapElement*);
    void removeImageMap(HTMLMapElement*);
    HTMLMapElement* getImageMap(const String& url) const;

    Element* elementFromPoint(int x, int y) const;

    // For accessibility.
    bool shouldCacheLabelsByForAttribute() const { return m_labelsByForAttribute; }
    void addLabel(const AtomicString& forAttributeValue, HTMLLabelElement*);
    void removeLabel(const AtomicString& forAttributeValue, HTMLLabelElement*);
    HTMLLabelElement* labelElementForId(const AtomicString& forAttributeValue);

    DOMSelection* getSelection() const;

    // Find first anchor with the given name.
    // First searches for an element with the given ID, but if that fails, then looks
    // for an anchor with the given name. ID matching is always case sensitive, but
    // Anchor name matching is case sensitive in strict mode and not case sensitive in
    // quirks mode for historical compatibility reasons.
    Element* findAnchor(const String& name);

    virtual bool applyAuthorStyles() const;
    virtual bool resetStyleInheritance() const;

    // Used by the basic DOM mutation methods (e.g., appendChild()).
    void adoptIfNeeded(Node*);

    ContainerNode* rootNode() const { return m_rootNode; }

    IdTargetObserverRegistry& idTargetObserverRegistry() const { return *m_idTargetObserverRegistry.get(); }

    static TreeScope* noDocumentInstance()
    {
        DEFINE_STATIC_LOCAL(TreeScope, instance, ());
        return &instance;
    }

    // Nodes belonging to this scope hold guard references -
    // these are enough to keep the scope from being destroyed, but
    // not enough to keep it from removing its children. This allows a
    // node that outlives its scope to still have a valid document
    // pointer without introducing reference cycles.
    void guardRef()
    {
        ASSERT(!deletionHasBegun());
        ++m_guardRefCount;
    }

    void guardDeref()
    {
        ASSERT(!deletionHasBegun());
        --m_guardRefCount;
        if (!m_guardRefCount && !refCount() && this != noDocumentInstance()) {
            beginDeletion();
            delete this;
        }
    }

    void removedLastRefToScope();

protected:
    TreeScope(ContainerNode*, Document*);
    TreeScope(Document*);
    virtual ~TreeScope();

    void destroyTreeScopeData();
    void clearDocumentScope();
    void setDocumentScope(Document* document)
    {
        ASSERT(document);
        ASSERT(this != noDocumentInstance());
        m_documentScope = document;
    }

    bool hasGuardRefCount() const { return m_guardRefCount; }

private:
    TreeScope();

    virtual void dispose() { }

    int refCount() const;
#ifndef NDEBUG
    bool deletionHasBegun();
    void beginDeletion();
#else
    bool deletionHasBegun() { return false; }
    void beginDeletion() { }
#endif

    ContainerNode* m_rootNode;
    Document* m_documentScope;
    TreeScope* m_parentTreeScope;
    int m_guardRefCount;

    OwnPtr<DocumentOrderedMap> m_elementsById;
    OwnPtr<DocumentOrderedMap> m_elementsByName;
    OwnPtr<DocumentOrderedMap> m_imageMapsByName;
    OwnPtr<DocumentOrderedMap> m_labelsByForAttribute;

    OwnPtr<IdTargetObserverRegistry> m_idTargetObserverRegistry;

    mutable RefPtr<DOMSelection> m_selection;
};

inline bool TreeScope::hasElementWithId(AtomicStringImpl* id) const
{
    ASSERT(id);
    return m_elementsById && m_elementsById->contains(id);
}

inline bool TreeScope::containsMultipleElementsWithId(const AtomicString& id) const
{
    return m_elementsById && m_elementsById->containsMultiple(id.impl());
}

inline bool TreeScope::hasElementWithName(AtomicStringImpl* id) const
{
    ASSERT(id);
    return m_elementsByName && m_elementsByName->contains(id);
}

inline bool TreeScope::containsMultipleElementsWithName(const AtomicString& name) const
{
    return m_elementsByName && m_elementsByName->containsMultiple(name.impl());
}

Node* nodeFromPoint(Document*, int x, int y, LayoutPoint* localPoint = 0);
TreeScope* commonTreeScope(Node*, Node*);

} // namespace WebCore

#endif // TreeScope_h
