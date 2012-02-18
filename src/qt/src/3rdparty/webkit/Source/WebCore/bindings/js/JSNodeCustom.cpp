/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSNode.h"

#include "Attr.h"
#include "CachedImage.h"
#include "CachedScript.h"
#include "CDATASection.h"
#include "Comment.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Entity.h"
#include "EntityReference.h"
#include "ExceptionCode.h"
#include "HTMLAudioElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLImageElement.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"
#include "JSAttr.h"
#include "JSCDATASection.h"
#include "JSComment.h"
#include "JSDOMBinding.h"
#include "JSDocument.h"
#include "JSDocumentFragment.h"
#include "JSDocumentType.h"
#include "JSEntity.h"
#include "JSEntityReference.h"
#include "JSEventListener.h"
#include "JSHTMLElement.h"
#include "JSHTMLElementWrapperFactory.h"
#include "JSNotation.h"
#include "JSProcessingInstruction.h"
#include "JSText.h"
#include "Node.h"
#include "Notation.h"
#include "ProcessingInstruction.h"
#include "RegisteredEventListener.h"
#include "ShadowRoot.h"
#include "StyleSheet.h"
#include "StyledElement.h"
#include "Text.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

#if ENABLE(SVG)
#include "JSSVGElementWrapperFactory.h"
#include "SVGElement.h"
#endif

using namespace JSC;

namespace WebCore {

using namespace HTMLNames;

static inline bool isObservable(JSNode* jsNode, Node* node)
{
    // The DOM doesn't know how to keep a tree of nodes alive without the root
    // being explicitly referenced. So, we artificially treat the root of
    // every tree as observable.
    // FIXME: Resolve this lifetime issue in the DOM, and remove this.
    if (!node->parentNode())
        return true;

    if (jsNode->hasCustomProperties())
        return true;

    // A node's JS wrapper is responsible for marking its JS event listeners.
    if (node->hasEventListeners())
        return true;

    return false;
}

static inline bool isReachableFromDOM(JSNode* jsNode, Node* node, SlotVisitor& visitor)
{
    if (!node->inDocument()) {
        // If a wrapper is the last reference to an image element
        // that is loading but not in the document, the wrapper is observable
        // because it is the only thing keeping the image element alive, and if
        // the element is destroyed, its load event will not fire.
        // FIXME: The DOM should manage this issue without the help of JavaScript wrappers.
        if (node->hasTagName(imgTag)) {
            if (static_cast<HTMLImageElement*>(node)->hasPendingActivity())
                return true;
        }
    #if ENABLE(VIDEO)
        else if (node->hasTagName(audioTag)) {
            if (!static_cast<HTMLAudioElement*>(node)->paused())
                return true;
        }
    #endif

        // If a node is firing event listeners, its wrapper is observable because
        // its wrapper is responsible for marking those event listeners.
        if (node->isFiringEventListeners())
            return true;
    }

    return isObservable(jsNode, node) && visitor.containsOpaqueRoot(root(node));
}

bool JSNodeOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    JSNode* jsNode = static_cast<JSNode*>(handle.get().asCell());
    return isReachableFromDOM(jsNode, jsNode->impl(), visitor);
}

void JSNodeOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSNode* jsNode = static_cast<JSNode*>(handle.get().asCell());
    DOMWrapperWorld* world = static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, jsNode->impl(), jsNode);
}

JSValue JSNode::insertBefore(ExecState* exec)
{
    Node* imp = static_cast<Node*>(impl());
    ExceptionCode ec = 0;
    bool ok = imp->insertBefore(toNode(exec->argument(0)), toNode(exec->argument(1)), ec, true);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

JSValue JSNode::replaceChild(ExecState* exec)
{
    Node* imp = static_cast<Node*>(impl());
    ExceptionCode ec = 0;
    bool ok = imp->replaceChild(toNode(exec->argument(0)), toNode(exec->argument(1)), ec, true);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(1);
    return jsNull();
}

JSValue JSNode::removeChild(ExecState* exec)
{
    Node* imp = static_cast<Node*>(impl());
    ExceptionCode ec = 0;
    bool ok = imp->removeChild(toNode(exec->argument(0)), ec);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

JSValue JSNode::appendChild(ExecState* exec)
{
    Node* imp = static_cast<Node*>(impl());
    ExceptionCode ec = 0;
    bool ok = imp->appendChild(toNode(exec->argument(0)), ec, true);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

ScopeChainNode* JSNode::pushEventHandlerScope(ExecState*, ScopeChainNode* node) const
{
    return node;
}

void JSNode::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    Node* node = m_impl.get();
    node->visitJSEventListeners(visitor);

    visitor.addOpaqueRoot(root(node));
}

static ALWAYS_INLINE JSValue createWrapperInline(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    ASSERT(node);
    ASSERT(!getCachedWrapper(currentWorld(exec), node));
    
    JSDOMWrapper* wrapper;    
    switch (node->nodeType()) {
        case Node::ELEMENT_NODE:
            if (node->isHTMLElement())
                wrapper = createJSHTMLWrapper(exec, globalObject, toHTMLElement(node));
#if ENABLE(SVG)
            else if (node->isSVGElement())
                wrapper = createJSSVGWrapper(exec, globalObject, static_cast<SVGElement*>(node));
#endif
            else
                wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Element, node);
            break;
        case Node::ATTRIBUTE_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Attr, node);
            break;
        case Node::TEXT_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Text, node);
            break;
        case Node::CDATA_SECTION_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CDATASection, node);
            break;
        case Node::ENTITY_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Entity, node);
            break;
        case Node::PROCESSING_INSTRUCTION_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, ProcessingInstruction, node);
            break;
        case Node::COMMENT_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Comment, node);
            break;
        case Node::DOCUMENT_NODE:
            // we don't want to cache the document itself in the per-document dictionary
            return toJS(exec, globalObject, static_cast<Document*>(node));
        case Node::DOCUMENT_TYPE_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, DocumentType, node);
            break;
        case Node::NOTATION_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Notation, node);
            break;
        case Node::DOCUMENT_FRAGMENT_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, DocumentFragment, node);
            break;
        case Node::SHADOW_ROOT_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Node, node);
            break;
        case Node::ENTITY_REFERENCE_NODE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, EntityReference, node);
            break;
        default:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Node, node);
    }

    return wrapper;    
}

JSValue createWrapper(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    return createWrapperInline(exec, globalObject, node);
}
    
JSValue toJSNewlyCreated(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    if (!node)
        return jsNull();
    
    return createWrapperInline(exec, globalObject, node);
}

} // namespace WebCore
