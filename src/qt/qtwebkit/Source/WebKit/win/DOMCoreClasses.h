/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#ifndef DOMCoreClasses_H
#define DOMCoreClasses_H

#include "WebKit.h"
#include "WebScriptObject.h"

namespace WebCore {
class Element;
class Document;
class DOMWindow;
class Node;
class NodeList;
}


class DOMObject : public WebScriptObject, public IDOMObject {
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return WebScriptObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return WebScriptObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return WebScriptObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return WebScriptObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return WebScriptObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return WebScriptObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return WebScriptObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return WebScriptObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return WebScriptObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return WebScriptObject::setException(description); }
};

class DECLSPEC_UUID("062AEEE3-9E42-44DC-A8A9-236B216FE011") DOMNode : public DOMObject, public IDOMNode, public IDOMEventTarget {
protected:
    DOMNode(WebCore::Node* n);
    ~DOMNode();

public:
    static IDOMNode* createInstance(WebCore::Node* n);

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue(
        /* [in] */ BSTR value);
    
    virtual HRESULT STDMETHODCALLTYPE nodeType(
        /* [retval][out] */ unsigned short* result);
    
    virtual HRESULT STDMETHODCALLTYPE parentNode(
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE childNodes(
        /* [retval][out] */ IDOMNodeList** result);
    
    virtual HRESULT STDMETHODCALLTYPE firstChild(
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE lastChild(
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling(
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling(
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE attributes(
        /* [retval][out] */ IDOMNamedNodeMap** result);
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument(
        /* [retval][out] */ IDOMDocument** result);
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* refChild,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE removeChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE appendChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes(
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode(
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void);
    
    virtual HRESULT STDMETHODCALLTYPE isSupported(
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE prefix(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix(
        /* [in] */ BSTR prefix);
    
    virtual HRESULT STDMETHODCALLTYPE localName(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes(
        /* [retval][out] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE isSameNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE textContent(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent(
        /* [in] */ BSTR text);

    // IDOMEventTarget
    virtual HRESULT STDMETHODCALLTYPE addEventListener(
        /* [in] */ BSTR type,
        /* [in] */ IDOMEventListener *listener,
        /* [in] */ BOOL useCapture);
    
    virtual HRESULT STDMETHODCALLTYPE removeEventListener(
        /* [in] */ BSTR type,
        /* [in] */ IDOMEventListener *listener,
        /* [in] */ BOOL useCapture);
    
    virtual HRESULT STDMETHODCALLTYPE dispatchEvent(
        /* [in] */ IDOMEvent *evt,
        /* [retval][out] */ BOOL* result);

    // DOMNode
    WebCore::Node* node() const { return m_node; }

protected:
    WebCore::Node* m_node;
};

class DOMNodeList : public DOMObject, public IDOMNodeList {
protected:
    DOMNodeList(WebCore::NodeList* l);
    ~DOMNodeList();

public:
    static IDOMNodeList* createInstance(WebCore::NodeList* l);

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMNodeList
    virtual HRESULT STDMETHODCALLTYPE item(
        /* [in] */ UINT index,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE length(
        /* [retval][out] */ UINT* result);

protected:
    WebCore::NodeList* m_nodeList;
};

class DOMDocument : public DOMNode, public IDOMDocument, public IDOMViewCSS, public IDOMDocumentEvent {
protected:
    DOMDocument(WebCore::Document* d);
    ~DOMDocument();

public:
    static IDOMDocument* createInstance(WebCore::Document* d);

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMNode::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMNode::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMNode::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMNode::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMNode::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return DOMNode::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMNode::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMNode::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMNode::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return DOMNode::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName(
        /* [retval][out] */ BSTR* result) { return DOMNode::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue(
        /* [retval][out] */ BSTR* result) { return DOMNode::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue(
        /* [in] */ BSTR value) { return DOMNode::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType(
        /* [retval][out] */ unsigned short* result) { return DOMNode::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes(
        /* [retval][out] */ IDOMNodeList** result) { return DOMNode::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes(
        /* [retval][out] */ IDOMNamedNodeMap** result) { return DOMNode::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument(
        /* [retval][out] */ IDOMDocument** result) { return DOMNode::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* refChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes(
        /* [retval][out] */ BOOL* result) { return DOMNode::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode(
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMNode::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported(
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL* result) { return DOMNode::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI(
        /* [retval][out] */ BSTR* result) { return DOMNode::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix(
        /* [retval][out] */ BSTR* result) { return DOMNode::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix(
        /* [in] */ BSTR prefix) { return DOMNode::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName(
        /* [retval][out] */ BSTR* result) { return DOMNode::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes(
        /* [retval][out] */ BOOL* result) { return DOMNode::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMNode::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMNode::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent(
        /* [retval][out] */ BSTR* result) { return DOMNode::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent(
        /* [in] */ BSTR text) { return DOMNode::setTextContent(text); }
    
    // IDOMDocument
    virtual HRESULT STDMETHODCALLTYPE doctype(
        /* [retval][out] */ IDOMDocumentType** result);
    
    virtual HRESULT STDMETHODCALLTYPE implementation(
        /* [retval][out] */ IDOMImplementation** result);
    
    virtual HRESULT STDMETHODCALLTYPE documentElement(
        /* [retval][out] */ IDOMElement** result);
    
    virtual HRESULT STDMETHODCALLTYPE createElement(
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IDOMElement** result);
    
    virtual HRESULT STDMETHODCALLTYPE createDocumentFragment(
        /* [retval][out] */ IDOMDocumentFragment** result);
    
    virtual HRESULT STDMETHODCALLTYPE createTextNode(
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMText** result);
    
    virtual HRESULT STDMETHODCALLTYPE createComment(
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMComment** result);
    
    virtual HRESULT STDMETHODCALLTYPE createCDATASection(
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMCDATASection** result);
    
    virtual HRESULT STDMETHODCALLTYPE createProcessingInstruction(
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMProcessingInstruction** result);
    
    virtual HRESULT STDMETHODCALLTYPE createAttribute(
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE createEntityReference(
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMEntityReference** result);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName(
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IDOMNodeList** result);
    
    virtual HRESULT STDMETHODCALLTYPE importNode(
        /* [in] */ IDOMNode* importedNode,
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE createElementNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [retval][out] */ IDOMElement** result);
    
    virtual HRESULT STDMETHODCALLTYPE createAttributeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList** result);
    
    virtual HRESULT STDMETHODCALLTYPE getElementById(
        /* [in] */ BSTR elementId,
        /* [retval][out] */ IDOMElement** result);

    // IDOMViewCSS
    virtual HRESULT STDMETHODCALLTYPE getComputedStyle(
        /* [in] */ IDOMElement* elt,
        /* [in] */ BSTR pseudoElt,
        /* [retval][out] */ IDOMCSSStyleDeclaration** result);

    // IDOMDocumentEvent
    virtual HRESULT STDMETHODCALLTYPE createEvent(
        /* [in] */ BSTR eventType,
        /* [retval][out] */ IDOMEvent** result);

    // DOMDocument
    WebCore::Document* document() { return m_document; }

protected:
    WebCore::Document* m_document;
};

class DOMWindow : public DOMObject, public IDOMWindow, public IDOMEventTarget {
protected:
    DOMWindow(WebCore::DOMWindow*);
    ~DOMWindow();

public:
    static IDOMWindow* createInstance(WebCore::DOMWindow*);

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    virtual HRESULT STDMETHODCALLTYPE document(
        /* [out, retval] */ IDOMDocument**);

    virtual HRESULT STDMETHODCALLTYPE getComputedStyle(
        /* [in] */ IDOMElement*, 
        /* [in] */ BSTR);

    virtual HRESULT STDMETHODCALLTYPE getMatchedCSSRules(
        /* [in] */ IDOMElement*, 
        /* [in] */ BSTR, 
        /* [in] */ BOOL, 
        /* [out, retval] */ IDOMCSSRuleList**);

    virtual HRESULT STDMETHODCALLTYPE devicePixelRatio(
        /* [out, retval] */ double*);

    virtual HRESULT STDMETHODCALLTYPE addEventListener(
        /* [in] */ BSTR,
        /* [in] */ IDOMEventListener *,
        /* [in] */ BOOL);
    
    virtual HRESULT STDMETHODCALLTYPE removeEventListener(
        /* [in] */ BSTR,
        /* [in] */ IDOMEventListener *,
        /* [in] */ BOOL);
    
    virtual HRESULT STDMETHODCALLTYPE dispatchEvent(
        /* [in] */ IDOMEvent *,
        /* [retval][out] */ BOOL *);

    // DOMWindow
    WebCore::DOMWindow* window() { return m_window; }

protected:
    WebCore::DOMWindow* m_window;
};



class DOMElement : public DOMNode, public IDOMElement, public IDOMElementPrivate, public IDOMNodeExtensions, public IDOMElementCSSInlineStyle, public IDOMElementExtensions {
protected:
    DOMElement(WebCore::Element* e);
    ~DOMElement();

public:
    static IDOMElement* createInstance(WebCore::Element* e);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMNode::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMNode::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException(
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMNode::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod(
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMNode::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript(
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMNode::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey(
        /* [in] */ BSTR name) { return DOMNode::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation(
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMNode::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMNode::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex(
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMNode::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException(
        /* [in] */ BSTR description) { return DOMNode::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName(
        /* [retval][out] */ BSTR* result) { return DOMNode::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue(
        /* [retval][out] */ BSTR* result) { return DOMNode::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue(
        /* [in] */ BSTR value) { return DOMNode::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType(
        /* [retval][out] */ unsigned short* result) { return DOMNode::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes(
        /* [retval][out] */ IDOMNodeList** result) { return DOMNode::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling(
        /* [retval][out] */ IDOMNode** result) { return DOMNode::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes(
        /* [retval][out] */ IDOMNamedNodeMap** result) { return DOMNode::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument(
        /* [retval][out] */ IDOMDocument** result) { return DOMNode::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* refChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild(
        /* [in] */ IDOMNode* newChild,
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild(
        /* [in] */ IDOMNode* oldChild,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes(
        /* [retval][out] */ BOOL* result) { return DOMNode::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode(
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode** result) { return DOMNode::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMNode::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported(
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL* result) { return DOMNode::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI(
        /* [retval][out] */ BSTR* result) { return DOMNode::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix(
        /* [retval][out] */ BSTR* result) { return DOMNode::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix(
        /* [in] */ BSTR prefix) { return DOMNode::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName(
        /* [retval][out] */ BSTR* result) { return DOMNode::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes(
        /* [retval][out] */ BOOL* result) { return DOMNode::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMNode::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode(
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMNode::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent(
        /* [retval][out] */ BSTR* result) { return DOMNode::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent(
        /* [in] */ BSTR text) { return DOMNode::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName(
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute(
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute(
        /* [in] */ BSTR name,
        /* [in] */ BSTR value);
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute(
        /* [in] */ BSTR name);
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode(
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode(
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode(
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName(
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList** result);
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value);
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName);
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS(
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr** result);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList** result);
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute(
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS(
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE focus( void);
    
    virtual HRESULT STDMETHODCALLTYPE blur( void);

    // IDOMNodeExtensions
    virtual HRESULT STDMETHODCALLTYPE boundingBox(
        /* [retval][out] */ LPRECT rect);
    
    virtual HRESULT STDMETHODCALLTYPE lineBoxRects(
        /* [size_is][in] */ RECT* rects,
        /* [in] */ int cRects);

    // IDOMElementPrivate
    virtual HRESULT STDMETHODCALLTYPE coreElement(
        void** element);

    virtual HRESULT STDMETHODCALLTYPE isEqual(
        /* [in] */ IDOMElement* other,
        /* [retval][out] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE isFocused(
        /* [retval][out] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE innerText(
        /* [retval][out] */ BSTR* result);

    virtual HRESULT STDMETHODCALLTYPE font(
        WebFontDescription* webFontDescription);

    virtual HRESULT STDMETHODCALLTYPE renderedImage(
        /* [retval][out] */ HBITMAP* image);

    virtual HRESULT STDMETHODCALLTYPE markerTextForListItem(
        /* [retval][out] */ BSTR* markerText);

    virtual HRESULT STDMETHODCALLTYPE shadowPseudoId(
        /* [retval][out] */ BSTR* result);

    // IDOMElementCSSInlineStyle
    virtual HRESULT STDMETHODCALLTYPE style(
        /* [retval][out] */ IDOMCSSStyleDeclaration** result);

    // IDOMElementExtensions
    virtual HRESULT STDMETHODCALLTYPE offsetLeft(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetTop(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetWidth(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetHeight(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetParent(
        /* [retval][out] */ IDOMElement** result);
    
    virtual HRESULT STDMETHODCALLTYPE clientWidth(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE clientHeight(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE scrollLeft(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE setScrollLeft(
        /* [in] */ int newScrollLeft);
    
    virtual HRESULT STDMETHODCALLTYPE scrollTop(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE setScrollTop(
        /* [in] */ int newScrollTop);
    
    virtual HRESULT STDMETHODCALLTYPE scrollWidth(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE scrollHeight(
        /* [retval][out] */ int* result);
    
    virtual HRESULT STDMETHODCALLTYPE scrollIntoView(
        /* [in] */ BOOL alignWithTop);
    
    virtual HRESULT STDMETHODCALLTYPE scrollIntoViewIfNeeded(
        /* [in] */ BOOL centerIfNeeded);

    // DOMElement
    WebCore::Element* element() { return m_element; }

protected:
    WebCore::Element* m_element;
};

#endif
