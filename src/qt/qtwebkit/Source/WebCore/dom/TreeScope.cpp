/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "TreeScope.h"

#include "ContainerNode.h"
#include "DOMSelection.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Element.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLAnchorElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLLabelElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "IdTargetObserverRegistry.h"
#include "NodeTraversal.h"
#include "Page.h"
#include "RenderView.h"
#include "RuntimeEnabledFeatures.h"
#include "ShadowRoot.h"
#include "TreeScopeAdopter.h"
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

namespace WebCore {

struct SameSizeAsTreeScope {
    virtual ~SameSizeAsTreeScope();
    void* pointers[9];
    int ints[1];
};

COMPILE_ASSERT(sizeof(TreeScope) == sizeof(SameSizeAsTreeScope), treescope_should_stay_small);

using namespace HTMLNames;

TreeScope::TreeScope(ContainerNode* rootNode, Document* document)
    : m_rootNode(rootNode)
    , m_documentScope(document)
    , m_parentTreeScope(document)
    , m_guardRefCount(0)
    , m_idTargetObserverRegistry(IdTargetObserverRegistry::create())
{
    ASSERT(rootNode);
    ASSERT(document);
    ASSERT(rootNode != document);
    m_parentTreeScope->guardRef();
    m_rootNode->setTreeScope(this);
}

TreeScope::TreeScope(Document* document)
    : m_rootNode(document)
    , m_documentScope(document)
    , m_parentTreeScope(0)
    , m_guardRefCount(0)
    , m_idTargetObserverRegistry(IdTargetObserverRegistry::create())
{
    ASSERT(document);
    m_rootNode->setTreeScope(this);
}

TreeScope::TreeScope()
    : m_rootNode(0)
    , m_documentScope(0)
    , m_parentTreeScope(0)
    , m_guardRefCount(0)
{
}

TreeScope::~TreeScope()
{
    ASSERT(!m_guardRefCount);
    m_rootNode->setTreeScope(noDocumentInstance());

    if (m_selection) {
        m_selection->clearTreeScope();
        m_selection = 0;
    }

    if (m_parentTreeScope)
        m_parentTreeScope->guardDeref();
}

void TreeScope::destroyTreeScopeData()
{
    m_elementsById.clear();
    m_imageMapsByName.clear();
    m_labelsByForAttribute.clear();
}

void TreeScope::clearDocumentScope()
{
    ASSERT(rootNode()->isDocumentNode());
    m_documentScope = 0;
}

void TreeScope::setParentTreeScope(TreeScope* newParentScope)
{
    // A document node cannot be re-parented.
    ASSERT(!rootNode()->isDocumentNode());
    // Every scope other than document needs a parent scope.
    ASSERT(newParentScope);

    newParentScope->guardRef();
    if (m_parentTreeScope)
        m_parentTreeScope->guardDeref();
    m_parentTreeScope = newParentScope;
    setDocumentScope(newParentScope->documentScope());
}

Element* TreeScope::getElementById(const AtomicString& elementId) const
{
    if (elementId.isEmpty())
        return 0;
    if (!m_elementsById)
        return 0;
    return m_elementsById->getElementById(elementId.impl(), this);
}

const Vector<Element*>* TreeScope::getAllElementsById(const AtomicString& elementId) const
{
    if (elementId.isEmpty())
        return 0;
    if (!m_elementsById)
        return 0;
    return m_elementsById->getAllElementsById(elementId.impl(), this);
}

void TreeScope::addElementById(const AtomicString& elementId, Element* element)
{
    if (!m_elementsById)
        m_elementsById = adoptPtr(new DocumentOrderedMap);
    m_elementsById->add(elementId.impl(), element);
    m_idTargetObserverRegistry->notifyObservers(elementId);
}

void TreeScope::removeElementById(const AtomicString& elementId, Element* element)
{
    if (!m_elementsById)
        return;
    m_elementsById->remove(elementId.impl(), element);
    m_idTargetObserverRegistry->notifyObservers(elementId);
}

Element* TreeScope::getElementByName(const AtomicString& name) const
{
    if (name.isEmpty())
        return 0;
    if (!m_elementsByName)
        return 0;
    return m_elementsByName->getElementByName(name.impl(), this);
}

void TreeScope::addElementByName(const AtomicString& name, Element* element)
{
    if (!m_elementsByName)
        m_elementsByName = adoptPtr(new DocumentOrderedMap);
    m_elementsByName->add(name.impl(), element);
}

void TreeScope::removeElementByName(const AtomicString& name, Element* element)
{
    if (!m_elementsByName)
        return;
    m_elementsByName->remove(name.impl(), element);
}

Node* TreeScope::ancestorInThisScope(Node* node) const
{
    while (node) {
        if (node->treeScope() == this)
            return node;
        if (!node->isInShadowTree())
            return 0;

        node = node->shadowHost();
    }

    return 0;
}

void TreeScope::addImageMap(HTMLMapElement* imageMap)
{
    AtomicStringImpl* name = imageMap->getName().impl();
    if (!name)
        return;
    if (!m_imageMapsByName)
        m_imageMapsByName = adoptPtr(new DocumentOrderedMap);
    m_imageMapsByName->add(name, imageMap);
}

void TreeScope::removeImageMap(HTMLMapElement* imageMap)
{
    if (!m_imageMapsByName)
        return;
    AtomicStringImpl* name = imageMap->getName().impl();
    if (!name)
        return;
    m_imageMapsByName->remove(name, imageMap);
}

HTMLMapElement* TreeScope::getImageMap(const String& url) const
{
    if (url.isNull())
        return 0;
    if (!m_imageMapsByName)
        return 0;
    size_t hashPos = url.find('#');
    String name = (hashPos == notFound ? url : url.substring(hashPos + 1)).impl();
    if (rootNode()->document()->isHTMLDocument())
        return toHTMLMapElement(m_imageMapsByName->getElementByLowercasedMapName(AtomicString(name.lower()).impl(), this));
    return toHTMLMapElement(m_imageMapsByName->getElementByMapName(AtomicString(name).impl(), this));
}

Node* nodeFromPoint(Document* document, int x, int y, LayoutPoint* localPoint)
{
    Frame* frame = document->frame();

    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    float scaleFactor = frame->pageZoomFactor() * frame->frameScaleFactor();
    IntPoint point = roundedIntPoint(FloatPoint(x * scaleFactor  + frameView->scrollX(), y * scaleFactor + frameView->scrollY()));

    if (!frameView->visibleContentRect().contains(point))
        return 0;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowShadowContent);
    HitTestResult result(point);
    document->renderView()->hitTest(request, result);

    if (localPoint)
        *localPoint = result.localPoint();

    return result.innerNode();
}

Element* TreeScope::elementFromPoint(int x, int y) const
{
    Node* node = nodeFromPoint(rootNode()->document(), x, y);
    while (node && !node->isElementNode())
        node = node->parentNode();
    if (node)
        node = ancestorInThisScope(node);
    return toElement(node);
}

void TreeScope::addLabel(const AtomicString& forAttributeValue, HTMLLabelElement* element)
{
    ASSERT(m_labelsByForAttribute);
    m_labelsByForAttribute->add(forAttributeValue.impl(), element);
}

void TreeScope::removeLabel(const AtomicString& forAttributeValue, HTMLLabelElement* element)
{
    ASSERT(m_labelsByForAttribute);
    m_labelsByForAttribute->remove(forAttributeValue.impl(), element);
}

HTMLLabelElement* TreeScope::labelElementForId(const AtomicString& forAttributeValue)
{
    if (forAttributeValue.isEmpty())
        return 0;

    if (!m_labelsByForAttribute) {
        // Populate the map on first access.
        m_labelsByForAttribute = adoptPtr(new DocumentOrderedMap);
        for (Element* element = ElementTraversal::firstWithin(rootNode()); element; element = ElementTraversal::next(element)) {
            if (isHTMLLabelElement(element)) {
                HTMLLabelElement* label = toHTMLLabelElement(element);
                const AtomicString& forValue = label->fastGetAttribute(forAttr);
                if (!forValue.isEmpty())
                    addLabel(forValue, label);
            }
        }
    }

    return toHTMLLabelElement(m_labelsByForAttribute->getElementByLabelForAttribute(forAttributeValue.impl(), this));
}

DOMSelection* TreeScope::getSelection() const
{
    if (!rootNode()->document()->frame())
        return 0;

    if (m_selection)
        return m_selection.get();

    // FIXME: The correct selection in Shadow DOM requires that Position can have a ShadowRoot
    // as a container. It is now enabled only if runtime Shadow DOM feature is enabled.
    // See https://bugs.webkit.org/show_bug.cgi?id=82697
#if ENABLE(SHADOW_DOM)
    if (RuntimeEnabledFeatures::shadowDOMEnabled()) {
        m_selection = DOMSelection::create(this);
        return m_selection.get();
    }
#endif

    if (this != rootNode()->document())
        return rootNode()->document()->getSelection();

    m_selection = DOMSelection::create(rootNode()->document());
    return m_selection.get();
}

Element* TreeScope::findAnchor(const String& name)
{
    if (name.isEmpty())
        return 0;
    if (Element* element = getElementById(name))
        return element;
    for (Element* element = ElementTraversal::firstWithin(rootNode()); element; element = ElementTraversal::next(element)) {
        if (isHTMLAnchorElement(element)) {
            HTMLAnchorElement* anchor = toHTMLAnchorElement(element);
            if (rootNode()->document()->inQuirksMode()) {
                // Quirks mode, case insensitive comparison of names.
                if (equalIgnoringCase(anchor->name(), name))
                    return anchor;
            } else {
                // Strict mode, names need to match exactly.
                if (anchor->name() == name)
                    return anchor;
            }
        }
    }
    return 0;
}

bool TreeScope::applyAuthorStyles() const
{
    return true;
}

bool TreeScope::resetStyleInheritance() const
{
    return false;
}

void TreeScope::adoptIfNeeded(Node* node)
{
    ASSERT(this);
    ASSERT(node);
    ASSERT(!node->isDocumentNode());
    ASSERT(!node->m_deletionHasBegun);
    TreeScopeAdopter adopter(node, this);
    if (adopter.needsScopeChange())
        adopter.execute();
}

static Element* focusedFrameOwnerElement(Frame* focusedFrame, Frame* currentFrame)
{
    for (; focusedFrame; focusedFrame = focusedFrame->tree()->parent()) {
        if (focusedFrame->tree()->parent() == currentFrame)
            return focusedFrame->ownerElement();
    }
    return 0;
}

Element* TreeScope::focusedElement()
{
    Document* document = rootNode()->document();
    Element* element = document->focusedElement();

    if (!element && document->page())
        element = focusedFrameOwnerElement(document->page()->focusController()->focusedFrame(), document->frame());
    if (!element)
        return 0;
    TreeScope* treeScope = element->treeScope();
    while (treeScope != this && treeScope != document) {
        element = toShadowRoot(treeScope->rootNode())->host();
        treeScope = element->treeScope();
    }
    if (this != treeScope)
        return 0;
    return element;
}

static void listTreeScopes(Node* node, Vector<TreeScope*, 5>& treeScopes)
{
    while (true) {
        treeScopes.append(node->treeScope());
        Element* ancestor = node->shadowHost();
        if (!ancestor)
            break;
        node = ancestor;
    }
}

TreeScope* commonTreeScope(Node* nodeA, Node* nodeB)
{
    if (!nodeA || !nodeB)
        return 0;

    if (nodeA->treeScope() == nodeB->treeScope())
        return nodeA->treeScope();

    Vector<TreeScope*, 5> treeScopesA;
    listTreeScopes(nodeA, treeScopesA);

    Vector<TreeScope*, 5> treeScopesB;
    listTreeScopes(nodeB, treeScopesB);

    size_t indexA = treeScopesA.size();
    size_t indexB = treeScopesB.size();

    for (; indexA > 0 && indexB > 0 && treeScopesA[indexA - 1] == treeScopesB[indexB - 1]; --indexA, --indexB) { }

    return treeScopesA[indexA] == treeScopesB[indexB] ? treeScopesA[indexA] : 0;
}

#ifndef NDEBUG
bool TreeScope::deletionHasBegun()
{
    return rootNode() && rootNode()->m_deletionHasBegun;
}

void TreeScope::beginDeletion()
{
    ASSERT(this != noDocumentInstance());
    rootNode()->m_deletionHasBegun = true;
}
#endif

int TreeScope::refCount() const
{
    if (Node* root = rootNode())
        return root->refCount();
    return 0;
}

} // namespace WebCore
