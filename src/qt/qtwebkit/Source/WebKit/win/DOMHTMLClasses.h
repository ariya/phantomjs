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

#ifndef DOMHTMLClasses_H
#define DOMHTMLClasses_H

#include "WebKit.h"
#include "DOMCoreClasses.h"
#include <wtf/RefPtr.h>

namespace WebCore {
    class HTMLCollection;
    class HTMLOptionsCollection;
}

class DOMHTMLCollection : public DOMObject, public IDOMHTMLCollection
{
protected:
    DOMHTMLCollection(WebCore::HTMLCollection* c);

public:
    static IDOMHTMLCollection* createInstance(WebCore::HTMLCollection*);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMHTMLCollection
    virtual HRESULT STDMETHODCALLTYPE length( 
        /* [retval][out] */ UINT *result);
    
    virtual HRESULT STDMETHODCALLTYPE item( 
        /* [in] */ UINT index,
        /* [retval][out] */ IDOMNode **node);
    
    virtual HRESULT STDMETHODCALLTYPE namedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNode **node);

protected:
    RefPtr<WebCore::HTMLCollection> m_collection;
};

class DOMHTMLOptionsCollection : public DOMObject, public IDOMHTMLOptionsCollection
{
public:
    static IDOMHTMLOptionsCollection* createInstance(WebCore::HTMLOptionsCollection*);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMHTMLOptionsCollection
    virtual HRESULT STDMETHODCALLTYPE length( 
        /* [retval][out] */ unsigned int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setLength( 
        /* [in] */ unsigned int length);
    
    virtual HRESULT STDMETHODCALLTYPE item( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ IDOMNode **result);
    
    virtual HRESULT STDMETHODCALLTYPE namedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNode **result);

private:
    DOMHTMLOptionsCollection(WebCore::HTMLOptionsCollection*);

    RefPtr<WebCore::HTMLOptionsCollection> m_collection;
};

class DOMHTMLDocument : public DOMDocument, public IDOMHTMLDocument
{
protected:
    DOMHTMLDocument();
public:
    DOMHTMLDocument(WebCore::Document* d) : DOMDocument(d) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMDocument::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMDocument::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMDocument::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMDocument::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMDocument::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMDocument::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMDocument::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMDocument::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMDocument::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMDocument::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMDocument::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMDocument::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMDocument::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMDocument::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMDocument::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMDocument::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMDocument::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMDocument::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMDocument::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMDocument::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMDocument::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMDocument::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMDocument::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMDocument::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMDocument::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMDocument::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMDocument::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMDocument::setTextContent(text); }
    
    // IDOMDocument
    virtual HRESULT STDMETHODCALLTYPE doctype( 
        /* [retval][out] */ IDOMDocumentType **result) { return DOMDocument::doctype(result); }
    
    virtual HRESULT STDMETHODCALLTYPE implementation( 
        /* [retval][out] */ IDOMImplementation **result) { return DOMDocument::implementation(result); }
    
    virtual HRESULT STDMETHODCALLTYPE documentElement( 
        /* [retval][out] */ IDOMElement **result) { return DOMDocument::documentElement(result); }
    
    virtual HRESULT STDMETHODCALLTYPE createElement( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IDOMElement **result) { return DOMDocument::createElement(tagName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createDocumentFragment( 
        /* [retval][out] */ IDOMDocumentFragment **result) { return DOMDocument::createDocumentFragment(result); }
    
    virtual HRESULT STDMETHODCALLTYPE createTextNode( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMText **result) { return DOMDocument::createTextNode(data, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createComment( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMComment **result) { return DOMDocument::createComment(data, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createCDATASection( 
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMCDATASection **result) { return DOMDocument::createCDATASection(data, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createProcessingInstruction( 
        /* [in] */ BSTR target,
        /* [in] */ BSTR data,
        /* [retval][out] */ IDOMProcessingInstruction **result) { return DOMDocument::createProcessingInstruction(target, data, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMDocument::createAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createEntityReference( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMEntityReference **result) { return DOMDocument::createEntityReference(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMDocument::getElementsByTagName(tagName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE importNode( 
        /* [in] */ IDOMNode *importedNode,
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMDocument::importNode(importedNode, deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createElementNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [retval][out] */ IDOMElement **result) { return DOMDocument::createElementNS(namespaceURI, qualifiedName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE createAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [retval][out] */ IDOMAttr **result) { return DOMDocument::createAttributeNS(namespaceURI, qualifiedName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMDocument::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementById( 
        /* [in] */ BSTR elementId,
        /* [retval][out] */ IDOMElement **result) { return DOMDocument::getElementById(elementId, result); }

    // IDOMHTMLDocument
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title);
    
    virtual HRESULT STDMETHODCALLTYPE referrer( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE domain( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE URL( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE body( 
        /* [retval][out] */ IDOMHTMLElement **bodyElement);
    
    virtual HRESULT STDMETHODCALLTYPE setBody( 
        /* [in] */ IDOMHTMLElement *body);
    
    virtual HRESULT STDMETHODCALLTYPE images( 
        /* [retval][out] */ IDOMHTMLCollection **collection);
    
    virtual HRESULT STDMETHODCALLTYPE applets( 
        /* [retval][out] */ IDOMHTMLCollection **collection);
    
    virtual HRESULT STDMETHODCALLTYPE links( 
        /* [retval][out] */ IDOMHTMLCollection **collection);
    
    virtual HRESULT STDMETHODCALLTYPE forms( 
        /* [retval][out] */ IDOMHTMLCollection **collection);
    
    virtual HRESULT STDMETHODCALLTYPE anchors( 
        /* [retval][out] */ IDOMHTMLCollection **collection);
    
    virtual HRESULT STDMETHODCALLTYPE cookie( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setCookie( 
        /* [in] */ BSTR cookie);
    
    virtual HRESULT STDMETHODCALLTYPE open( void);
    
    virtual HRESULT STDMETHODCALLTYPE close( void);
    
    virtual HRESULT STDMETHODCALLTYPE write( 
        /* [in] */ BSTR text);
    
    virtual HRESULT STDMETHODCALLTYPE writeln( 
        /* [in] */ BSTR text);
    
    virtual HRESULT STDMETHODCALLTYPE getElementById_( 
        /* [in] */ BSTR elementId,
        /* [retval][out] */ IDOMElement **element);
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByName( 
        /* [in] */ BSTR elementName,
        /* [retval][out] */ IDOMNodeList **nodeList);
};

class DOMHTMLElement : public DOMElement, public IDOMHTMLElement
{
protected:
    DOMHTMLElement();
public:
    DOMHTMLElement(WebCore::Element* e) : DOMElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName);
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title);
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang);
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir);
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className);

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result);
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html);
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result);
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text);        

};

class DOMHTMLFormElement : public DOMHTMLElement, public IDOMHTMLFormElement
{
protected:
    DOMHTMLFormElement();
public:
    DOMHTMLFormElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLFormElement
    virtual HRESULT STDMETHODCALLTYPE elements( 
        /* [retval][out] */ IDOMHTMLCollection **result);
    
    virtual HRESULT STDMETHODCALLTYPE length( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE name( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setName( 
        /* [in] */ BSTR name);
    
    virtual HRESULT STDMETHODCALLTYPE acceptCharset( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAcceptCharset( 
        /* [in] */ BSTR acceptCharset);
    
    virtual HRESULT STDMETHODCALLTYPE action( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAction( 
        /* [in] */ BSTR action);
    
    virtual HRESULT STDMETHODCALLTYPE encType( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setEnctype( 
        /* [retval][out] */ BSTR *encType);
    
    virtual HRESULT STDMETHODCALLTYPE method( 
        /* [retval][out] */ BSTR *method);
    
    virtual HRESULT STDMETHODCALLTYPE setMethod( 
        /* [in] */ BSTR method);
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTarget( 
        /* [in] */ BSTR target);
    
    virtual HRESULT STDMETHODCALLTYPE submit( void);
    
    virtual HRESULT STDMETHODCALLTYPE reset( void);
};

class DOMHTMLSelectElement : public DOMHTMLElement, public IDOMHTMLSelectElement, public IFormsAutoFillTransitionSelect
{
protected:
    DOMHTMLSelectElement();
public:
    DOMHTMLSelectElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLSelectElement
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE selectedIndex( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setSelectedIndx( 
        /* [in] */ int selectedIndex);
    
    virtual HRESULT STDMETHODCALLTYPE value( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setValue( 
        /* [in] */ BSTR value);
    
    virtual HRESULT STDMETHODCALLTYPE length( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE form( 
        /* [retval][out] */ IDOMHTMLFormElement **result);
    
    virtual HRESULT STDMETHODCALLTYPE options( 
        /* [retval][out] */ IDOMHTMLOptionsCollection **result);
    
    virtual HRESULT STDMETHODCALLTYPE disabled( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDisabled( 
        /* [in] */ BOOL disabled);
    
    virtual HRESULT STDMETHODCALLTYPE multiple( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setMultiple( 
        /* [in] */ BOOL multiple);
    
    virtual HRESULT STDMETHODCALLTYPE name( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setName( 
        /* [in] */ BSTR name);
    
    virtual HRESULT STDMETHODCALLTYPE size( 
        /* [retval][out] */ int *size);
    
    virtual HRESULT STDMETHODCALLTYPE setSize( 
        /* [in] */ int size);
    
    virtual HRESULT STDMETHODCALLTYPE tabIndex( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTabIndex( 
        /* [in] */ int tabIndex);
    
    virtual HRESULT STDMETHODCALLTYPE add( 
        /* [in] */ IDOMHTMLElement *element,
        /* [in] */ IDOMHTMLElement *before);
    
    virtual HRESULT STDMETHODCALLTYPE remove( 
        /* [in] */ int index);
    
    // IFormsAutoFillTransitionSelect
    virtual HRESULT STDMETHODCALLTYPE activateItemAtIndex( 
        /* [in] */ int index);
};

class DOMHTMLOptionElement : public DOMHTMLElement, public IDOMHTMLOptionElement
{
protected:
    DOMHTMLOptionElement();
public:
    DOMHTMLOptionElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLOptionElement
    virtual HRESULT STDMETHODCALLTYPE form( 
        /* [retval][out] */ IDOMHTMLFormElement **result);
    
    virtual HRESULT STDMETHODCALLTYPE defaultSelected( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDefaultSelected( 
        /* [in] */ BOOL defaultSelected);
    
    virtual HRESULT STDMETHODCALLTYPE text( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE index( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE disabled( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDisabled( 
        /* [in] */ BOOL disabled);
    
    virtual HRESULT STDMETHODCALLTYPE label( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setLabel( 
        /* [in] */ BSTR label);
    
    virtual HRESULT STDMETHODCALLTYPE selected( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setSelected( 
        /* [in] */ BOOL selected);
    
    virtual HRESULT STDMETHODCALLTYPE value( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setValue( 
        /* [in] */ BSTR value);
};

class DOMHTMLInputElement : public DOMHTMLElement, public IDOMHTMLInputElement, public IFormsAutoFillTransition, public IFormPromptAdditions
{
protected:
    DOMHTMLInputElement();
public:
    DOMHTMLInputElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLInputElement
    virtual HRESULT STDMETHODCALLTYPE defaultValue( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDefaultValue( 
        /* [in] */ BSTR val);
    
    virtual HRESULT STDMETHODCALLTYPE defaultChecked( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDefaultChecked( 
        /* [in] */ BSTR checked);
    
    virtual HRESULT STDMETHODCALLTYPE form( 
        /* [retval][out] */ IDOMHTMLElement **result);
    
    virtual HRESULT STDMETHODCALLTYPE accept( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAccept( 
        /* [in] */ BSTR accept);
    
    virtual HRESULT STDMETHODCALLTYPE accessKey( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAccessKey( 
        /* [in] */ BSTR key);
    
    virtual HRESULT STDMETHODCALLTYPE align( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAlign( 
        /* [in] */ BSTR align);
    
    virtual HRESULT STDMETHODCALLTYPE alt( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAlt( 
        /* [in] */ BSTR alt);
    
    virtual HRESULT STDMETHODCALLTYPE checked( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setChecked( 
        /* [in] */ BOOL checked);
    
    virtual HRESULT STDMETHODCALLTYPE disabled( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDisabled( 
        /* [in] */ BOOL disabled);
    
    virtual HRESULT STDMETHODCALLTYPE maxLength( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setMaxLength( 
        /* [in] */ int maxLength);
    
    virtual HRESULT STDMETHODCALLTYPE name( 
        /* [retval][out] */ BSTR *name);
    
    virtual HRESULT STDMETHODCALLTYPE setName( 
        /* [in] */ BSTR name);
    
    virtual HRESULT STDMETHODCALLTYPE readOnly( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setReadOnly( 
        /* [in] */ BOOL readOnly);
    
    virtual HRESULT STDMETHODCALLTYPE size( 
        /* [retval][out] */ unsigned int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setSize( 
        /* [in] */ unsigned int size);
    
    virtual HRESULT STDMETHODCALLTYPE src( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setSrc( 
        /* [in] */ BSTR src);
    
    virtual HRESULT STDMETHODCALLTYPE tabIndex( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTabIndex( 
        /* [in] */ int tabIndex);
    
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setType( 
        /* [in] */ BSTR type);
    
    virtual HRESULT STDMETHODCALLTYPE useMap( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setUseMap( 
        /* [in] */ BSTR useMap);
    
    virtual HRESULT STDMETHODCALLTYPE value( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setValue( 
        /* [in] */ BSTR value);
        
    virtual HRESULT STDMETHODCALLTYPE select( void);
    
    virtual HRESULT STDMETHODCALLTYPE click( void);

    virtual HRESULT STDMETHODCALLTYPE setSelectionStart( 
        /* [in] */ long start);
    
    virtual HRESULT STDMETHODCALLTYPE selectionStart( 
        /* [retval][out] */ long *start);
    
    virtual HRESULT STDMETHODCALLTYPE setSelectionEnd( 
        /* [in] */ long end);
    
    virtual HRESULT STDMETHODCALLTYPE selectionEnd( 
        /* [retval][out] */ long *end);

    // IFormsAutoFillTransition
    virtual HRESULT STDMETHODCALLTYPE isTextField(
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE rectOnScreen( 
        /* [retval][out] */ LPRECT rect);
    
    virtual HRESULT STDMETHODCALLTYPE replaceCharactersInRange( 
        /* [in] */ int startTarget,
        /* [in] */ int endTarget,
        /* [in] */ BSTR replacementString,
        /* [in] */ int index);
    
    virtual HRESULT STDMETHODCALLTYPE selectedRange( 
        /* [out] */ int *start,
        /* [out] */ int *end);
    
    virtual HRESULT STDMETHODCALLTYPE setAutofilled( 
        /* [in] */ BOOL filled);

    virtual HRESULT STDMETHODCALLTYPE isAutofilled(
        /* [retval][out] */ BOOL *result);
    
    // IFormPromptAdditions
    virtual HRESULT STDMETHODCALLTYPE isUserEdited( 
        /* [retval][out] */ BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE setValueForUser(
        /* [in] */ BSTR value);
};

class DOMHTMLTextAreaElement : public DOMHTMLElement, public IDOMHTMLTextAreaElement, public IFormPromptAdditions
{
protected:
    DOMHTMLTextAreaElement();
public:
    DOMHTMLTextAreaElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLTextArea
    virtual HRESULT STDMETHODCALLTYPE defaultValue( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDefaultValue( 
        /* [in] */ BSTR val);
    
    virtual HRESULT STDMETHODCALLTYPE form( 
        /* [retval][out] */ IDOMHTMLElement **result);
    
    virtual HRESULT STDMETHODCALLTYPE accessKey( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setAccessKey( 
        /* [in] */ BSTR key);
    
    virtual HRESULT STDMETHODCALLTYPE cols( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setCols( 
        /* [in] */ int cols);
    
    virtual HRESULT STDMETHODCALLTYPE disabled( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setDisabled( 
        /* [in] */ BOOL disabled);
    
    virtual HRESULT STDMETHODCALLTYPE name( 
        /* [retval][out] */ BSTR *name);
    
    virtual HRESULT STDMETHODCALLTYPE setName( 
        /* [in] */ BSTR name);
    
    virtual HRESULT STDMETHODCALLTYPE readOnly( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE setReadOnly( 
        /* [in] */ BOOL readOnly);
    
    virtual HRESULT STDMETHODCALLTYPE rows( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setRows( 
        /* [in] */ int rows);
    
    virtual HRESULT STDMETHODCALLTYPE tabIndex( 
        /* [retval][out] */ int *result);
    
    virtual HRESULT STDMETHODCALLTYPE setTabIndex( 
        /* [in] */ int tabIndex);
    
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE value( 
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE setValue( 
        /* [in] */ BSTR value);
        
    virtual HRESULT STDMETHODCALLTYPE select( void);

    // IFormPromptAdditions
    virtual HRESULT STDMETHODCALLTYPE isUserEdited( 
        /* [retval][out] */ BOOL *result);
};

class DOMHTMLIFrameElement : public DOMHTMLElement, public IDOMHTMLIFrameElement
{
protected:
    DOMHTMLIFrameElement();
public:
    DOMHTMLIFrameElement(WebCore::Element* e) : DOMHTMLElement(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMHTMLElement::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMHTMLElement::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMHTMLElement::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT *result) { return DOMHTMLElement::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMHTMLElement::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMHTMLElement::setException(description); }

    // IDOMNode
    virtual HRESULT STDMETHODCALLTYPE nodeName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeValue( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::nodeValue(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setNodeValue( 
        /* [in] */ BSTR value) { return DOMHTMLElement::setNodeValue(value); }
    
    virtual HRESULT STDMETHODCALLTYPE nodeType( 
        /* [retval][out] */ unsigned short *result) { return DOMHTMLElement::nodeType(result); }
    
    virtual HRESULT STDMETHODCALLTYPE parentNode( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::parentNode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE childNodes( 
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::childNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE firstChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::firstChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE lastChild( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::lastChild(result); }
    
    virtual HRESULT STDMETHODCALLTYPE previousSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::previousSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE nextSibling( 
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::nextSibling(result); }
    
    virtual HRESULT STDMETHODCALLTYPE attributes( 
        /* [retval][out] */ IDOMNamedNodeMap **result) { return DOMHTMLElement::attributes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE ownerDocument( 
        /* [retval][out] */ IDOMDocument **result) { return DOMHTMLElement::ownerDocument(result); }
    
    virtual HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *refChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::insertBefore(newChild, refChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IDOMNode *newChild,
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::replaceChild(newChild, oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::removeChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IDOMNode *oldChild,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::appendChild(oldChild, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasChildNodes(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ BOOL deep,
        /* [retval][out] */ IDOMNode **result) { return DOMHTMLElement::cloneNode(deep, result); }
    
    virtual HRESULT STDMETHODCALLTYPE normalize( void) { return DOMHTMLElement::normalize(); }
    
    virtual HRESULT STDMETHODCALLTYPE isSupported( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::isSupported(feature, version, result); }
    
    virtual HRESULT STDMETHODCALLTYPE namespaceURI( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::namespaceURI(result); }
    
    virtual HRESULT STDMETHODCALLTYPE prefix( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::prefix(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setPrefix( 
        /* [in] */ BSTR prefix) { return DOMHTMLElement::setPrefix(prefix); }
    
    virtual HRESULT STDMETHODCALLTYPE localName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::localName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributes( 
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributes(result); }

    virtual HRESULT STDMETHODCALLTYPE isSameNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isSameNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE isEqualNode( 
        /* [in] */ IDOMNode* other,
        /* [retval][out] */ BOOL* result) { return DOMHTMLElement::isEqualNode(other, result); }
    
    virtual HRESULT STDMETHODCALLTYPE textContent( 
        /* [retval][out] */ BSTR* result) { return DOMHTMLElement::textContent(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTextContent( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setTextContent(text); }
    
    // IDOMElement
    virtual HRESULT STDMETHODCALLTYPE tagName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::tagName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttribute(name, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR name) { return DOMHTMLElement::removeAttribute(name); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNode( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNode(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNode( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNode(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNode( 
        /* [in] */ IDOMAttr *oldAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::removeAttributeNode(oldAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagName(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::getAttributeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR qualifiedName,
        /* [in] */ BSTR value) { return DOMHTMLElement::setAttributeNS(namespaceURI, qualifiedName, value); }
    
    virtual HRESULT STDMETHODCALLTYPE removeAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName) { return DOMHTMLElement::removeAttributeNS(namespaceURI, localName); }
    
    virtual HRESULT STDMETHODCALLTYPE getAttributeNodeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::getAttributeNodeNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setAttributeNodeNS( 
        /* [in] */ IDOMAttr *newAttr,
        /* [retval][out] */ IDOMAttr **result) { return DOMHTMLElement::setAttributeNodeNS(newAttr, result); }
    
    virtual HRESULT STDMETHODCALLTYPE getElementsByTagNameNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ IDOMNodeList **result) { return DOMHTMLElement::getElementsByTagNameNS(namespaceURI, localName, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttribute(name, result); }
    
    virtual HRESULT STDMETHODCALLTYPE hasAttributeNS( 
        /* [in] */ BSTR namespaceURI,
        /* [in] */ BSTR localName,
        /* [retval][out] */ BOOL *result) { return DOMHTMLElement::hasAttributeNS(namespaceURI, localName, result); }

    virtual HRESULT STDMETHODCALLTYPE focus( void) { return DOMHTMLElement::focus(); }
    
    virtual HRESULT STDMETHODCALLTYPE blur( void) { return DOMHTMLElement::blur(); }

    // IDOMHTMLElement
    virtual HRESULT STDMETHODCALLTYPE idName( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::idName(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setIdName( 
        /* [in] */ BSTR idName) { return DOMHTMLElement::setIdName(idName); }
    
    virtual HRESULT STDMETHODCALLTYPE title( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::title(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setTitle( 
        /* [in] */ BSTR title) { return DOMHTMLElement::setTitle(title); }
    
    virtual HRESULT STDMETHODCALLTYPE lang( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::lang(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setLang( 
        /* [in] */ BSTR lang) { return DOMHTMLElement::setLang(lang); }
    
    virtual HRESULT STDMETHODCALLTYPE dir( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::dir(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setDir( 
        /* [in] */ BSTR dir) { return DOMHTMLElement::setDir(dir); }
    
    virtual HRESULT STDMETHODCALLTYPE className( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::className(result); }
    
    virtual HRESULT STDMETHODCALLTYPE setClassName( 
        /* [in] */ BSTR className) { return DOMHTMLElement::setClassName(className); }

    virtual HRESULT STDMETHODCALLTYPE innerHTML( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerHTML(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerHTML( 
        /* [in] */ BSTR html) { return DOMHTMLElement::setInnerHTML(html); }
        
    virtual HRESULT STDMETHODCALLTYPE innerText( 
        /* [retval][out] */ BSTR *result) { return DOMHTMLElement::innerText(result); }
        
    virtual HRESULT STDMETHODCALLTYPE setInnerText( 
        /* [in] */ BSTR text) { return DOMHTMLElement::setInnerText(text); }

    // IDOMHTMLIFrameElement
    virtual HRESULT STDMETHODCALLTYPE contentFrame( 
        /* [retval][out] */ IWebFrame **result);
};

#endif
