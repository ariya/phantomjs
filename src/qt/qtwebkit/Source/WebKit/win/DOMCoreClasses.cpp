/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc.  All rights reserved.
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
#include "WebKitDLL.h"
#include "DOMCoreClasses.h"

#include "DOMCSSClasses.h"
#include "DOMEventsClasses.h"
#include "DOMHTMLClasses.h"
#include "WebKitGraphics.h"

#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <WebCore/DOMWindow.h>
#include <WebCore/Document.h>
#include <WebCore/Element.h>
#include <WebCore/Font.h>
#include <WebCore/Frame.h>
#include <WebCore/SimpleFontData.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLOptionElement.h>
#include <WebCore/HTMLSelectElement.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/NodeList.h>
#include <WebCore/RenderObject.h>
#include <WebCore/RenderTreeAsText.h>

#include <initguid.h>
// {3B0C0EFF-478B-4b0b-8290-D2321E08E23E}
DEFINE_GUID(IID_DOMElement, 0x3b0c0eff, 0x478b, 0x4b0b, 0x82, 0x90, 0xd2, 0x32, 0x1e, 0x8, 0xe2, 0x3e);

// Our normal style is just to say "using namespace WebCore" rather than having
// individual using directives for each type from that namespace. But
// "DOMObject" exists both in the WebCore namespace and unnamespaced in this
// file, which leads to ambiguities if we say "using namespace WebCore".
using namespace WebCore::HTMLNames;
using WTF::AtomicString;
using WebCore::BString;
using WebCore::Element;
using WebCore::ExceptionCode;
using WebCore::FontDescription;
using WebCore::Frame;
using WebCore::IntRect;
using WTF::String;

// DOMObject - IUnknown -------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMObject::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMObject))
        *ppvObject = static_cast<IDOMObject*>(this);
    else
        return WebScriptObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMNode - IUnknown ---------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMNode::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMNode))
        *ppvObject = static_cast<IDOMNode*>(this);
    else if (IsEqualGUID(riid, __uuidof(DOMNode)))
        *ppvObject = static_cast<DOMNode*>(this);
    else if (IsEqualGUID(riid, __uuidof(IDOMEventTarget)))
        *ppvObject = static_cast<IDOMEventTarget*>(this);
    else
        return DOMObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMNode --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMNode::nodeName( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    if (!m_node)
        return E_FAIL;

    *result = BString(m_node->nodeName()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::nodeValue( 
    /* [retval][out] */ BSTR* result)
{
    if (!m_node)
        return E_FAIL;
    WTF::String nodeValueStr = m_node->nodeValue();
    *result = SysAllocStringLen(nodeValueStr.characters(), nodeValueStr.length());
    if (nodeValueStr.length() && !*result)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::setNodeValue( 
    /* [in] */ BSTR /*value*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::nodeType( 
    /* [retval][out] */ unsigned short* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::parentNode( 
    /* [retval][out] */ IDOMNode** result)
{
    *result = 0;
    if (!m_node || !m_node->parentNode())
        return E_FAIL;
    *result = DOMNode::createInstance(m_node->parentNode());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMNode::childNodes( 
    /* [retval][out] */ IDOMNodeList** result)
{
    if (!m_node)
        return E_FAIL;

    if (!result)
        return E_POINTER;

    *result = DOMNodeList::createInstance(m_node->childNodes().get());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMNode::firstChild( 
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::lastChild( 
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::previousSibling( 
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::nextSibling( 
    /* [retval][out] */ IDOMNode** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;
    if (!m_node)
        return E_FAIL;
    *result = DOMNode::createInstance(m_node->nextSibling());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMNode::attributes( 
    /* [retval][out] */ IDOMNamedNodeMap** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::ownerDocument( 
    /* [retval][out] */ IDOMDocument** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;
    if (!m_node)
        return E_FAIL;
    *result = DOMDocument::createInstance(m_node->ownerDocument());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMNode::insertBefore( 
    /* [in] */ IDOMNode* newChild,
    /* [in] */ IDOMNode* refChild,
    /* [retval][out] */ IDOMNode** result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    if (!m_node)
        return E_FAIL;

    COMPtr<DOMNode> newChildNode(Query, newChild);
    if (!newChildNode)
        return E_FAIL;

    COMPtr<DOMNode> refChildNode(Query, refChild);

    ExceptionCode ec;
    if (!m_node->insertBefore(newChildNode->node(), refChildNode ? refChildNode->node() : 0, ec))
        return E_FAIL;

    *result = newChild;
    (*result)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::replaceChild( 
    /* [in] */ IDOMNode* /*newChild*/,
    /* [in] */ IDOMNode* /*oldChild*/,
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::removeChild( 
    /* [in] */ IDOMNode* oldChild,
    /* [retval][out] */ IDOMNode** result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    if (!m_node)
        return E_FAIL;

    COMPtr<DOMNode> oldChildNode(Query, oldChild);
    if (!oldChildNode)
        return E_FAIL;

    ExceptionCode ec;
    if (!m_node->removeChild(oldChildNode->node(), ec))
        return E_FAIL;

    *result = oldChild;
    (*result)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::appendChild( 
    /* [in] */ IDOMNode* /*oldChild*/,
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::hasChildNodes( 
    /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::cloneNode( 
    /* [in] */ BOOL /*deep*/,
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::normalize( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::isSupported( 
    /* [in] */ BSTR /*feature*/,
    /* [in] */ BSTR /*version*/,
    /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::namespaceURI( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::prefix( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::setPrefix( 
    /* [in] */ BSTR /*prefix*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::localName( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::hasAttributes( 
    /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::isSameNode( 
    /* [in] */ IDOMNode* other,
    /* [retval][out] */ BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = FALSE;

    if (!other)
        return E_POINTER;

    COMPtr<DOMNode> domOther;
    HRESULT hr = other->QueryInterface(__uuidof(DOMNode), (void**)&domOther);
    if (FAILED(hr))
        return hr;

    *result = m_node->isSameNode(domOther->node()) ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::isEqualNode( 
    /* [in] */ IDOMNode* /*other*/,
    /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMNode::textContent( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_node->textContent()).release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMNode::setTextContent( 
    /* [in] */ BSTR /*text*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMNode - IDOMEventTarget --------------------------------------------------

HRESULT DOMNode::addEventListener(
    /* [in] */ BSTR type,
    /* [in] */ IDOMEventListener* listener,
    /* [in] */ BOOL useCapture)
{
    RefPtr<WebEventListener> webListener = WebEventListener::create(listener);
    m_node->addEventListener(type, webListener, useCapture);

    return S_OK;
}

HRESULT DOMNode::removeEventListener(
    /* [in] */ BSTR type,
    /* [in] */ IDOMEventListener* listener,
    /* [in] */ BOOL useCapture)
{
    if (!listener || !type)
        return E_POINTER;
    if (!m_node)
        return E_FAIL;
    RefPtr<WebEventListener> webListener = WebEventListener::create(listener);
    m_node->removeEventListener(type, webListener.get(), useCapture);
    return S_OK;
}

HRESULT DOMNode::dispatchEvent(
    /* [in] */ IDOMEvent* evt,
    /* [retval][out] */ BOOL* result)
{
    if (!evt || !result)
        return E_POINTER;
    if (!m_node)
        return E_FAIL;

    COMPtr<DOMEvent> domEvent;
    HRESULT hr = evt->QueryInterface(IID_DOMEvent, (void**) &domEvent);
    if (FAILED(hr))
        return hr;

    WebCore::ExceptionCode ec = 0;
    *result = m_node->dispatchEvent(domEvent->coreEvent(), ec) ? TRUE : FALSE;
    return ec ? E_FAIL : S_OK;
}

// DOMNode - DOMNode ----------------------------------------------------------

DOMNode::DOMNode(WebCore::Node* n)
: m_node(0)
{
    if (n)
        n->ref();

    m_node = n;
}

DOMNode::~DOMNode()
{
    if (m_node)
        m_node->deref();
}

IDOMNode* DOMNode::createInstance(WebCore::Node* n)
{
    if (!n)
        return 0;

    HRESULT hr = S_OK;
    IDOMNode* domNode = 0;
    WebCore::Node::NodeType nodeType = n->nodeType();

    switch (nodeType) {
        case WebCore::Node::ELEMENT_NODE: 
        {
            IDOMElement* newElement = DOMElement::createInstance(static_cast<WebCore::Element*>(n));
            if (newElement) {
                hr = newElement->QueryInterface(IID_IDOMNode, (void**)&domNode);
                newElement->Release();
            }
        }
        break;
        case WebCore::Node::DOCUMENT_NODE:
        {
            IDOMDocument* newDocument = DOMDocument::createInstance(n->document());
            if (newDocument) {
                hr = newDocument->QueryInterface(IID_IDOMNode, (void**)&domNode);
                newDocument->Release();
            }
        }
        break;
        default:
        {
            DOMNode* newNode = new DOMNode(n);
            hr = newNode->QueryInterface(IID_IDOMNode, (void**)&domNode);
        }
        break;
    }

    if (FAILED(hr))
        return 0;

    return domNode;
}

// DOMNodeList - IUnknown -----------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMNodeList::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMNodeList))
        *ppvObject = static_cast<IDOMNodeList*>(this);
    else
        return DOMObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// IDOMNodeList ---------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMNodeList::item( 
    /* [in] */ UINT index,
    /* [retval][out] */ IDOMNode **result)
{
    *result = 0;
    if (!m_nodeList)
        return E_FAIL;

    WebCore::Node* itemNode = m_nodeList->item(index);
    if (!itemNode)
        return E_FAIL;

    *result = DOMNode::createInstance(itemNode);
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMNodeList::length( 
        /* [retval][out] */ UINT *result)
{
    *result = 0;
    if (!m_nodeList)
        return E_FAIL;
    *result = m_nodeList->length();
    return S_OK;
}

// DOMNodeList - DOMNodeList --------------------------------------------------

DOMNodeList::DOMNodeList(WebCore::NodeList* l)
: m_nodeList(0)
{
    if (l)
        l->ref();

    m_nodeList = l;
}

DOMNodeList::~DOMNodeList()
{
    if (m_nodeList)
        m_nodeList->deref();
}

IDOMNodeList* DOMNodeList::createInstance(WebCore::NodeList* l)
{
    if (!l)
        return 0;

    IDOMNodeList* domNodeList = 0;
    DOMNodeList* newNodeList = new DOMNodeList(l);
    if (FAILED(newNodeList->QueryInterface(IID_IDOMNodeList, (void**)&domNodeList)))
        return 0;

    return domNodeList;
}

// DOMDocument - IUnknown -----------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMDocument::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMDocument))
        *ppvObject = static_cast<IDOMDocument*>(this);
    else if (IsEqualGUID(riid, IID_IDOMViewCSS))
        *ppvObject = static_cast<IDOMViewCSS*>(this);
    else if (IsEqualGUID(riid, IID_IDOMDocumentEvent))
        *ppvObject = static_cast<IDOMDocumentEvent*>(this);
    else
        return DOMNode::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMDocument ----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMDocument::doctype( 
    /* [retval][out] */ IDOMDocumentType** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::implementation( 
    /* [retval][out] */ IDOMImplementation** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::documentElement( 
    /* [retval][out] */ IDOMElement** result)
{
    *result = DOMElement::createInstance(m_document->documentElement());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createElement( 
    /* [in] */ BSTR tagName,
    /* [retval][out] */ IDOMElement** result)
{
    if (!m_document)
        return E_FAIL;

    String tagNameString(tagName);
    ExceptionCode ec;
    *result = DOMElement::createInstance(m_document->createElement(tagNameString, ec).get());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createDocumentFragment( 
    /* [retval][out] */ IDOMDocumentFragment** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createTextNode( 
    /* [in] */ BSTR /*data*/,
    /* [retval][out] */ IDOMText** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createComment( 
    /* [in] */ BSTR /*data*/,
    /* [retval][out] */ IDOMComment** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createCDATASection( 
    /* [in] */ BSTR /*data*/,
    /* [retval][out] */ IDOMCDATASection** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createProcessingInstruction( 
    /* [in] */ BSTR /*target*/,
    /* [in] */ BSTR /*data*/,
    /* [retval][out] */ IDOMProcessingInstruction** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createAttribute( 
    /* [in] */ BSTR /*name*/,
    /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createEntityReference( 
    /* [in] */ BSTR /*name*/,
    /* [retval][out] */ IDOMEntityReference** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::getElementsByTagName( 
    /* [in] */ BSTR tagName,
    /* [retval][out] */ IDOMNodeList** result)
{
    if (!m_document)
        return E_FAIL;

    String tagNameString(tagName);
    *result = DOMNodeList::createInstance(m_document->getElementsByTagName(tagNameString).get());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::importNode( 
    /* [in] */ IDOMNode* /*importedNode*/,
    /* [in] */ BOOL /*deep*/,
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createElementNS( 
    /* [in] */ BSTR /*namespaceURI*/,
    /* [in] */ BSTR /*qualifiedName*/,
    /* [retval][out] */ IDOMElement** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::createAttributeNS( 
    /* [in] */ BSTR /*namespaceURI*/,
    /* [in] */ BSTR /*qualifiedName*/,
    /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::getElementsByTagNameNS( 
    /* [in] */ BSTR namespaceURI,
    /* [in] */ BSTR localName,
    /* [retval][out] */ IDOMNodeList** result)
{
    if (!m_document)
        return E_FAIL;

    String namespaceURIString(namespaceURI);
    String localNameString(localName);
    *result = DOMNodeList::createInstance(m_document->getElementsByTagNameNS(namespaceURIString, localNameString).get());
    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMDocument::getElementById( 
    /* [in] */ BSTR elementId,
    /* [retval][out] */ IDOMElement** result)
{
    if (!m_document)
        return E_FAIL;

    String idString(elementId);
    *result = DOMElement::createInstance(m_document->getElementById(idString));
    return *result ? S_OK : E_FAIL;
}

// DOMDocument - IDOMViewCSS --------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMDocument::getComputedStyle( 
    /* [in] */ IDOMElement* elt,
    /* [in] */ BSTR pseudoElt,
    /* [retval][out] */ IDOMCSSStyleDeclaration** result)
{
    if (!elt || !result)
        return E_POINTER;

    COMPtr<DOMElement> domEle;
    HRESULT hr = elt->QueryInterface(IID_DOMElement, (void**)&domEle);
    if (FAILED(hr))
        return hr;
    Element* element = domEle->element();

    WebCore::DOMWindow* dv = m_document->defaultView();
    String pseudoEltString(pseudoElt);
    
    if (!dv)
        return E_FAIL;
    
    *result = DOMCSSStyleDeclaration::createInstance(dv->getComputedStyle(element, pseudoEltString.impl()).get());
    return *result ? S_OK : E_FAIL;
}

// DOMDocument - IDOMDocumentEvent --------------------------------------------

HRESULT STDMETHODCALLTYPE DOMDocument::createEvent( 
    /* [in] */ BSTR eventType,
    /* [retval][out] */ IDOMEvent **result)
{
    String eventTypeString(eventType, SysStringLen(eventType));
    WebCore::ExceptionCode ec = 0;
    *result = DOMEvent::createInstance(m_document->createEvent(eventTypeString, ec));
    return *result ? S_OK : E_FAIL;
}

// DOMDocument - DOMDocument --------------------------------------------------

DOMDocument::DOMDocument(WebCore::Document* d)
: DOMNode(d)
, m_document(d)
{
}

DOMDocument::~DOMDocument()
{
}

IDOMDocument* DOMDocument::createInstance(WebCore::Document* d)
{
    if (!d)
        return 0;

    HRESULT hr;
    IDOMDocument* domDocument = 0;

    if (d->isHTMLDocument()) {
        DOMHTMLDocument* newDocument = new DOMHTMLDocument(d);
        hr = newDocument->QueryInterface(IID_IDOMDocument, (void**)&domDocument);
    } else {
        DOMDocument* newDocument = new DOMDocument(d);
        hr = newDocument->QueryInterface(IID_IDOMDocument, (void**)&domDocument);
    }

    if (FAILED(hr))
        return 0;

    return domDocument;
}

// DOMWindow - IUnknown ------------------------------------------------------

HRESULT DOMWindow::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMWindow))
        *ppvObject = static_cast<IDOMWindow*>(this);
    else if (IsEqualGUID(riid, IID_IDOMEventTarget))
        *ppvObject = static_cast<IDOMEventTarget*>(this);
    else
        return DOMObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMWindow - IDOMWindow ------------------------------------------------------

HRESULT DOMWindow::document(
    /* [out, retval] */ IDOMDocument** result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = DOMDocument::createInstance(m_window->document());
    return *result ? S_OK : E_FAIL;
}

HRESULT DOMWindow::getComputedStyle(
    /* [in] */ IDOMElement* element, 
    /* [in] */ BSTR pseudoElement)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT DOMWindow::getMatchedCSSRules(
    /* [in] */ IDOMElement* element, 
    /* [in] */ BSTR pseudoElement, 
    /* [in] */ BOOL authorOnly, 
    /* [out, retval] */ IDOMCSSRuleList** result)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT DOMWindow::devicePixelRatio(
    /* [out, retval] */ double* result)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMWindow - IDOMEventTarget ------------------------------------------------------

HRESULT DOMWindow::addEventListener(
    /* [in] */ BSTR type,
    /* [in] */ IDOMEventListener* listener,
    /* [in] */ BOOL useCapture)
{
    if (!type || !listener)
        return E_POINTER;
    if (!m_window)
        return E_FAIL;
    RefPtr<WebEventListener> webListener = WebEventListener::create(listener);
    m_window->addEventListener(type, webListener, useCapture);
    return S_OK;
}

HRESULT DOMWindow::removeEventListener(
    /* [in] */ BSTR type,
    /* [in] */ IDOMEventListener* listener,
    /* [in] */ BOOL useCapture)
{
    if (!type || !listener)
        return E_POINTER;
    if (!m_window)
        return E_FAIL;
    RefPtr<WebEventListener> webListener = WebEventListener::create(listener);
    m_window->removeEventListener(type, webListener.get(), useCapture);
    return S_OK;
}

HRESULT DOMWindow::dispatchEvent(
    /* [in] */ IDOMEvent* evt,
    /* [retval][out] */ BOOL* result)
{
    if (!result || !evt)
        return E_POINTER;
    if (!m_window)
        return E_FAIL;

    COMPtr<DOMEvent> domEvent;
    HRESULT hr = evt->QueryInterface(IID_DOMEvent, (void**) &domEvent);
    if (FAILED(hr))
        return hr;

    WebCore::ExceptionCode ec = 0;
    *result = m_window->dispatchEvent(domEvent->coreEvent(), ec) ? TRUE : FALSE;
    return ec ? E_FAIL : S_OK;
}


// DOMWindow - DOMWindow --------------------------------------------------

DOMWindow::DOMWindow(WebCore::DOMWindow* w)
: m_window(w)
{
}

DOMWindow::~DOMWindow()
{
}

IDOMWindow* DOMWindow::createInstance(WebCore::DOMWindow* w)
{
    if (!w)
        return 0;

    DOMWindow* newWindow = new DOMWindow(w);

    IDOMWindow* domWindow = 0;
    HRESULT hr = newWindow->QueryInterface(IID_IDOMWindow, reinterpret_cast<void**>(&domWindow));

    if (FAILED(hr))
        return 0;

    return domWindow;
}

// DOMElement - IUnknown ------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMElement))
        *ppvObject = static_cast<IDOMElement*>(this);
    else if (IsEqualGUID(riid, IID_DOMElement))
        *ppvObject = static_cast<DOMElement*>(this);
    else if (IsEqualGUID(riid, IID_IDOMElementPrivate))
        *ppvObject = static_cast<IDOMElementPrivate*>(this);
    else if (IsEqualGUID(riid, IID_IDOMNodeExtensions))
        *ppvObject = static_cast<IDOMNodeExtensions*>(this);
    else if (IsEqualGUID(riid, IID_IDOMElementCSSInlineStyle))
        *ppvObject = static_cast<IDOMElementCSSInlineStyle*>(this);
    else if (IsEqualGUID(riid, IID_IDOMElementExtensions))
        *ppvObject = static_cast<IDOMElementExtensions*>(this);
    else
        return DOMNode::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMElement - IDOMNodeExtensions---------------------------------------------

HRESULT STDMETHODCALLTYPE DOMElement::boundingBox( 
    /* [retval][out] */ LPRECT rect)
{
    ::SetRectEmpty(rect);

    if (!m_element)
        return E_FAIL;

    WebCore::RenderObject *renderer = m_element->renderer();
    if (renderer) {
        IntRect boundsIntRect = renderer->absoluteBoundingBoxRect();
        rect->left = boundsIntRect.x();
        rect->top = boundsIntRect.y();
        rect->right = boundsIntRect.x() + boundsIntRect.width();
        rect->bottom = boundsIntRect.y() + boundsIntRect.height();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::lineBoxRects( 
    /* [size_is][in] */ RECT* /*rects*/,
    /* [in] */ int /*cRects*/)
{
    return E_NOTIMPL;
}

// IDOMElement ----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMElement::tagName( 
        /* [retval][out] */ BSTR* result)
{
    if (!m_element)
        return E_FAIL;

    if (!result)
        return E_POINTER;

    *result = BString(m_element->tagName()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getAttribute( 
        /* [in] */ BSTR name,
        /* [retval][out] */ BSTR* result)
{
    if (!m_element)
        return E_FAIL;
    WTF::String nameString(name, SysStringLen(name));
    WTF::String& attrValueString = (WTF::String&) m_element->getAttribute(nameString);
    *result = SysAllocStringLen(attrValueString.characters(), attrValueString.length());
    if (attrValueString.length() && !*result)
        return E_OUTOFMEMORY;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::setAttribute( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR value)
{
    if (!m_element)
        return E_FAIL;

    WTF::String nameString(name, SysStringLen(name));
    WTF::String valueString(value, SysStringLen(value));
    WebCore::ExceptionCode ec = 0;
    m_element->setAttribute(nameString, valueString, ec);
    return ec ? E_FAIL : S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::removeAttribute( 
        /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getAttributeNode( 
        /* [in] */ BSTR /*name*/,
        /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::setAttributeNode( 
        /* [in] */ IDOMAttr* /*newAttr*/,
        /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::removeAttributeNode( 
        /* [in] */ IDOMAttr* /*oldAttr*/,
        /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getElementsByTagName( 
        /* [in] */ BSTR /*name*/,
        /* [retval][out] */ IDOMNodeList** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getAttributeNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*localName*/,
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::setAttributeNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*qualifiedName*/,
        /* [in] */ BSTR /*value*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::removeAttributeNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*localName*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getAttributeNodeNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*localName*/,
        /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::setAttributeNodeNS( 
        /* [in] */ IDOMAttr* /*newAttr*/,
        /* [retval][out] */ IDOMAttr** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::getElementsByTagNameNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*localName*/,
        /* [retval][out] */ IDOMNodeList** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::hasAttribute( 
        /* [in] */ BSTR /*name*/,
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMElement::hasAttributeNS( 
        /* [in] */ BSTR /*namespaceURI*/,
        /* [in] */ BSTR /*localName*/,
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMElement::focus( void)
{
    if (!m_element)
        return E_FAIL;
    m_element->focus();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::blur( void)
{
    if (!m_element)
        return E_FAIL;
    m_element->blur();
    return S_OK;
}

// IDOMElementPrivate ---------------------------------------------------------

HRESULT DOMElement::coreElement(void **element)
{
    if (!m_element)
        return E_FAIL;
    *element = (void*) m_element;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::isEqual( 
    /* [in] */ IDOMElement *other,
    /* [retval][out] */ BOOL *result)
{
    *result = FALSE;

    if (!other || !result)
        return E_POINTER;

    IDOMElementPrivate* otherPriv;
    HRESULT hr = other->QueryInterface(IID_IDOMElementPrivate, (void**) &otherPriv);
    if (FAILED(hr))
        return hr;
    
    void* otherCoreEle;
    hr = otherPriv->coreElement(&otherCoreEle);
    otherPriv->Release();
    if (FAILED(hr))
        return hr;

    *result = (otherCoreEle == (void*)m_element) ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::isFocused( 
    /* [retval][out] */ BOOL *result)
{
    if (!m_element)
        return E_FAIL;

    if (m_element->document()->focusedElement() == m_element)
        *result = TRUE;
    else
        *result = FALSE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::innerText(
    /* [retval][out] */ BSTR* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    if (!m_element) {
        ASSERT_NOT_REACHED();
        return E_FAIL;
    }

    *result = BString(m_element->innerText()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::font(WebFontDescription* webFontDescription)
{
    if (!webFontDescription) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    ASSERT(m_element);

    WebCore::RenderObject* renderer = m_element->renderer();
    if (!renderer)
        return E_FAIL;

    FontDescription fontDescription = renderer->style()->font().fontDescription();
    AtomicString family = fontDescription.firstFamily();
    webFontDescription->family = family.characters();
    webFontDescription->familyLength = family.length();
    webFontDescription->size = fontDescription.computedSize();
    webFontDescription->bold = fontDescription.weight() >= WebCore::FontWeight600;
    webFontDescription->italic = fontDescription.italic();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::renderedImage(HBITMAP* image)
{
    if (!image) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }
    *image = 0;

    ASSERT(m_element);

    Frame* frame = m_element->document()->frame();
    if (!frame)
        return E_FAIL;

    *image = frame->nodeImage(m_element);
    if (!*image)
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::markerTextForListItem(
    /* [retval][out] */ BSTR* markerText)
{
    if (!markerText)
        return E_POINTER;

    ASSERT(m_element);

    *markerText = BString(WebCore::markerTextForListItem(m_element)).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::shadowPseudoId(
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    ASSERT(m_element);

    *result = BString(m_element->shadowPseudoId().string()).release();
    return S_OK;
}

// IDOMElementCSSInlineStyle --------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMElement::style( 
    /* [retval][out] */ IDOMCSSStyleDeclaration** result)
{
    if (!result)
        return E_POINTER;
    if (!m_element)
        return E_FAIL;

    WebCore::CSSStyleDeclaration* style = m_element->style();
    if (!style)
        return E_FAIL;

    *result = DOMCSSStyleDeclaration::createInstance(style);
    return *result ? S_OK : E_FAIL;
}

// IDOMElementExtensions ------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMElement::offsetLeft( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->offsetLeft();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::offsetTop( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->offsetTop();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::offsetWidth( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->offsetWidth();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::offsetHeight( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->offsetHeight();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::offsetParent( 
    /* [retval][out] */ IDOMElement** /*result*/)
{
    // FIXME
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMElement::clientWidth( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->clientWidth();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::clientHeight( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->clientHeight();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollLeft( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->scrollLeft();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::setScrollLeft( 
    /* [in] */ int /*newScrollLeft*/)
{
    // FIXME
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollTop( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->scrollTop();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::setScrollTop( 
    /* [in] */ int /*newScrollTop*/)
{
    // FIXME
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollWidth( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->scrollWidth();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollHeight( 
    /* [retval][out] */ int* result)
{
    if (!m_element)
        return E_FAIL;

    *result = m_element->scrollHeight();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollIntoView( 
    /* [in] */ BOOL alignWithTop)
{
    if (!m_element)
        return E_FAIL;

    m_element->scrollIntoView(!!alignWithTop);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMElement::scrollIntoViewIfNeeded( 
    /* [in] */ BOOL centerIfNeeded)
{
    if (!m_element)
        return E_FAIL;

    m_element->scrollIntoViewIfNeeded(!!centerIfNeeded);
    return S_OK;
}

// DOMElement -----------------------------------------------------------------

DOMElement::DOMElement(WebCore::Element* e)
: DOMNode(e)
, m_element(e)
{
}

DOMElement::~DOMElement()
{
}

IDOMElement* DOMElement::createInstance(WebCore::Element* e)
{
    if (!e)
        return 0;

    HRESULT hr;
    IDOMElement* domElement = 0;

    if (isHTMLFormElement(e)) {
        DOMHTMLFormElement* newElement = new DOMHTMLFormElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (e->hasTagName(iframeTag)) {
        DOMHTMLIFrameElement* newElement = new DOMHTMLIFrameElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (isHTMLInputElement(e)) {
        DOMHTMLInputElement* newElement = new DOMHTMLInputElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (isHTMLOptionElement(e)) {
        DOMHTMLOptionElement* newElement = new DOMHTMLOptionElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (e->hasTagName(selectTag)) {
        DOMHTMLSelectElement* newElement = new DOMHTMLSelectElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (isHTMLTextAreaElement(e)) {
        DOMHTMLTextAreaElement* newElement = new DOMHTMLTextAreaElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else if (e->isHTMLElement()) {
        DOMHTMLElement* newElement = new DOMHTMLElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    } else {
        DOMElement* newElement = new DOMElement(e);
        hr = newElement->QueryInterface(IID_IDOMElement, (void**)&domElement);
    }

    if (FAILED(hr))
        return 0;

    return domElement;
}
