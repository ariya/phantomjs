/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
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
#include "DOMHTMLClasses.h"
#include "WebFrame.h"

#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <WebCore/Document.h>
#include <WebCore/Element.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLCollection.h>
#include <WebCore/HTMLDocument.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLIFrameElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLOptionElement.h>
#include <WebCore/HTMLOptionsCollection.h>
#include <WebCore/HTMLSelectElement.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/IntRect.h>
#include <WebCore/RenderObject.h>
#include <WebCore/RenderTextControl.h>

using namespace WebCore;
using namespace HTMLNames;

// DOMHTMLCollection

DOMHTMLCollection::DOMHTMLCollection(WebCore::HTMLCollection* c)
: m_collection(c)
{
}

IDOMHTMLCollection* DOMHTMLCollection::createInstance(WebCore::HTMLCollection* c)
{
    if (!c)
        return 0;

    IDOMHTMLCollection* htmlCollection = 0;
    DOMHTMLCollection* newCollection = new DOMHTMLCollection(c);
    if (FAILED(newCollection->QueryInterface(IID_IDOMHTMLCollection, (void**)&htmlCollection))) {
        delete newCollection;
        return 0;
    }

    return htmlCollection;
}

// DOMHTMLCollection - IUnknown -----------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLCollection::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLCollection))
        *ppvObject = static_cast<IDOMHTMLCollection*>(this);
    else
        return DOMObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLCollection ----------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLCollection::length( 
    /* [retval][out] */ UINT* result)
{
    *result = 0;
    if (!m_collection)
        return E_POINTER;

    *result = m_collection->length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLCollection::item( 
    /* [in] */ UINT index,
    /* [retval][out] */ IDOMNode** node)
{
    *node = 0;
    if (!m_collection)
        return E_POINTER;

    *node = DOMNode::createInstance(m_collection->item(index));
    return *node ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMHTMLCollection::namedItem( 
    /* [in] */ BSTR /*name*/,
    /* [retval][out] */ IDOMNode** /*node*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMHTMLOptionsCollection - IUnknown ----------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLOptionsCollection::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLOptionsCollection))
        *ppvObject = static_cast<IDOMHTMLOptionsCollection*>(this);
    else
        return DOMObject::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLOptionsCollection ---------------------------------------------------

DOMHTMLOptionsCollection::DOMHTMLOptionsCollection(WebCore::HTMLOptionsCollection* collection)
    : m_collection(collection)
{
}

IDOMHTMLOptionsCollection* DOMHTMLOptionsCollection::createInstance(WebCore::HTMLOptionsCollection* collection)
{
    if (!collection)
        return 0;

    IDOMHTMLOptionsCollection* optionsCollection = 0;
    DOMHTMLOptionsCollection* newCollection = new DOMHTMLOptionsCollection(collection);
    if (FAILED(newCollection->QueryInterface(IID_IDOMHTMLOptionsCollection, (void**)&optionsCollection))) {
        delete newCollection;
        return 0;
    }

    return optionsCollection;
}

HRESULT STDMETHODCALLTYPE DOMHTMLOptionsCollection::length( 
    /* [retval][out] */ unsigned int* result)
{
    if (!result)
        return E_POINTER;

    *result = m_collection->length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLOptionsCollection::setLength( 
    /* [in] */ unsigned int /*length*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMHTMLOptionsCollection::item( 
    /* [in] */ unsigned int index,
    /* [retval][out] */ IDOMNode** result)
{
    if (!result)
        return E_POINTER;

    *result = DOMNode::createInstance(m_collection->item(index));

    return *result ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE DOMHTMLOptionsCollection::namedItem( 
    /* [in] */ BSTR /*name*/,
    /* [retval][out] */ IDOMNode** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMHTMLDocument - IUnknown -------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLDocument::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLDocument))
        *ppvObject = static_cast<IDOMHTMLDocument*>(this);
    else
        return DOMDocument::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLDocument ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLDocument::title( 
        /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    if (!m_document || !m_document->isHTMLDocument())
        return E_FAIL;

    *result = BString(m_document->title()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::setTitle( 
        /* [in] */ BSTR /*title*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::referrer( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::domain( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::URL( 
        /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(toHTMLDocument(m_document)->url()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::body( 
        /* [retval][out] */ IDOMHTMLElement** bodyElement)
{
    *bodyElement = 0;
    if (!m_document || !m_document->isHTMLDocument())
        return E_FAIL;

    HTMLDocument* htmlDoc = toHTMLDocument(m_document);
    COMPtr<IDOMElement> domElement;
    domElement.adoptRef(DOMHTMLElement::createInstance(htmlDoc->body()));
    if (domElement)
        return domElement->QueryInterface(IID_IDOMHTMLElement, (void**) bodyElement);
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::setBody( 
        /* [in] */ IDOMHTMLElement* /*body*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::images( 
        /* [retval][out] */ IDOMHTMLCollection** /*collection*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::applets( 
        /* [retval][out] */ IDOMHTMLCollection** /*collection*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::links( 
        /* [retval][out] */ IDOMHTMLCollection** /*collection*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::forms( 
        /* [retval][out] */ IDOMHTMLCollection** collection)
{
    *collection = 0;
    if (!m_document || !m_document->isHTMLDocument())
        return E_FAIL;

    HTMLDocument* htmlDoc = toHTMLDocument(m_document);
    RefPtr<HTMLCollection> forms = htmlDoc->forms();
    *collection = DOMHTMLCollection::createInstance(forms.get());
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::anchors( 
        /* [retval][out] */ IDOMHTMLCollection** /*collection*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::cookie( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::setCookie( 
        /* [in] */ BSTR /*cookie*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::open( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::close( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::write( 
        /* [in] */ BSTR /*text*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::writeln( 
        /* [in] */ BSTR /*text*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::getElementById_( 
        /* [in] */ BSTR /*elementId*/,
        /* [retval][out] */ IDOMElement** /*element*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLDocument::getElementsByName( 
        /* [in] */ BSTR /*elementName*/,
        /* [retval][out] */ IDOMNodeList** /*nodeList*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMHTMLElement - IUnknown --------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLElement))
        *ppvObject = static_cast<IDOMHTMLElement*>(this);
    else
        return DOMElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLElement -------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLElement::idName( 
        /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    ASSERT(m_element && m_element->isHTMLElement());
    String idString = toHTMLElement(m_element)->getAttribute(idAttr);
    *result = BString(idString).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setIdName( 
        /* [in] */ BSTR /*idName*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::title( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setTitle( 
        /* [in] */ BSTR /*title*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::lang( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setLang( 
        /* [in] */ BSTR /*lang*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::dir( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setDir( 
        /* [in] */ BSTR /*dir*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::className( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setClassName( 
        /* [in] */ BSTR /*className*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMHTMLElement::innerHTML( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setInnerHTML( 
        /* [in] */ BSTR /*html*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE DOMHTMLElement::innerText( 
        /* [retval][out] */ BSTR* result)
{
    ASSERT(m_element && m_element->isHTMLElement());
    WTF::String innerTextString = toHTMLElement(m_element)->innerText();
    *result = BString(innerTextString.characters(), innerTextString.length()).release();
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE DOMHTMLElement::setInnerText( 
        /* [in] */ BSTR text)
{
    ASSERT(m_element && m_element->isHTMLElement());
    HTMLElement* htmlEle = toHTMLElement(m_element);
    WTF::String textString(text, SysStringLen(text));
    WebCore::ExceptionCode ec = 0;
    htmlEle->setInnerText(textString, ec);
    return S_OK;
}

// DOMHTMLFormElement - IUnknown ----------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLFormElement))
        *ppvObject = static_cast<IDOMHTMLFormElement*>(this);
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLFormElement ---------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::elements( 
        /* [retval][out] */ IDOMHTMLCollection** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::length( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::name( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setName( 
        /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::acceptCharset( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setAcceptCharset( 
        /* [in] */ BSTR /*acceptCharset*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::action( 
        /* [retval][out] */ BSTR* result)
{
    ASSERT(m_element && isHTMLFormElement(m_element));
    WTF::String actionString = toHTMLFormElement(m_element)->action();
    *result = BString(actionString.characters(), actionString.length()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setAction( 
        /* [in] */ BSTR /*action*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::encType( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setEnctype( 
        /* [retval][out] */ BSTR* /*encType*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::method( 
        /* [retval][out] */ BSTR* result)
{
    ASSERT(m_element && isHTMLFormElement(m_element));
    WTF::String methodString = toHTMLFormElement(m_element)->method();
    *result = BString(methodString.characters(), methodString.length()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setMethod( 
        /* [in] */ BSTR /*method*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::target( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::setTarget( 
        /* [in] */ BSTR /*target*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::submit( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLFormElement::reset( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMHTMLSelectElement - IUnknown ----------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLSelectElement))
        *ppvObject = static_cast<IDOMHTMLSelectElement*>(this);
    else if (IsEqualGUID(riid, IID_IFormsAutoFillTransitionSelect))
        *ppvObject = static_cast<IFormsAutoFillTransitionSelect*>(this);
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLSelectElement -------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::type( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::selectedIndex( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setSelectedIndx( 
        /* [in] */ int /*selectedIndex*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::value( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setValue( 
        /* [in] */ BSTR /*value*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::length( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::form( 
        /* [retval][out] */ IDOMHTMLFormElement** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::options( 
        /* [retval][out] */ IDOMHTMLOptionsCollection** result)
{
    if (!result)
        return E_POINTER;

    ASSERT(m_element);
    HTMLSelectElement* selectElement = toHTMLSelectElement(m_element);

    if (!selectElement->options())
        return E_FAIL;

    *result = 0;
    RefPtr<HTMLOptionsCollection> options = selectElement->options();
    *result = DOMHTMLOptionsCollection::createInstance(options.get());
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::disabled( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setDisabled( 
        /* [in] */ BOOL /*disabled*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::multiple( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setMultiple( 
        /* [in] */ BOOL /*multiple*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::name( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setName( 
        /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::size( 
        /* [retval][out] */ int* /*size*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setSize( 
        /* [in] */ int /*size*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::tabIndex( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::setTabIndex( 
        /* [in] */ int /*tabIndex*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::add( 
        /* [in] */ IDOMHTMLElement* /*element*/,
        /* [in] */ IDOMHTMLElement* /*before*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::remove( 
        /* [in] */ int /*index*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
// DOMHTMLSelectElement - IFormsAutoFillTransitionSelect ----------------------

HRESULT STDMETHODCALLTYPE DOMHTMLSelectElement::activateItemAtIndex( 
    /* [in] */ int index)
{
    ASSERT(m_element);
    HTMLSelectElement* selectElement = toHTMLSelectElement(m_element);

    if (index >= selectElement->length())
        return E_FAIL;

    selectElement->setSelectedIndex(index);
    return S_OK;
}

// DOMHTMLOptionElement - IUnknown --------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLOptionElement))
        *ppvObject = static_cast<IDOMHTMLOptionElement*>(this);
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLOptionElement -------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::form( 
        /* [retval][out] */ IDOMHTMLFormElement** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::defaultSelected( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::setDefaultSelected( 
        /* [in] */ BOOL /*defaultSelected*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::text( 
        /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    ASSERT(m_element);
    ASSERT(isHTMLOptionElement(m_element));
    HTMLOptionElement* optionElement = toHTMLOptionElement(m_element);

    *result = BString(optionElement->text()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::index( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::disabled( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::setDisabled( 
        /* [in] */ BOOL /*disabled*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::label( 
        /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    ASSERT(m_element);
    ASSERT(isHTMLOptionElement(m_element));
    HTMLOptionElement* optionElement = toHTMLOptionElement(m_element);

    *result = BString(optionElement->label()).release();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::setLabel( 
        /* [in] */ BSTR /*label*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::selected( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::setSelected( 
        /* [in] */ BOOL /*selected*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::value( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLOptionElement::setValue( 
        /* [in] */ BSTR /*value*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// DOMHTMLInputElement - IUnknown ----------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLInputElement))
        *ppvObject = static_cast<IDOMHTMLInputElement*>(this);
    else if (IsEqualGUID(riid, IID_IFormsAutoFillTransition))
        *ppvObject = static_cast<IFormsAutoFillTransition*>(this);
    else if (IsEqualGUID(riid, IID_IFormPromptAdditions))
        *ppvObject = static_cast<IFormPromptAdditions*>(this);    
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLInputElement --------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::defaultValue( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setDefaultValue( 
        /* [in] */ BSTR /*val*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::defaultChecked( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setDefaultChecked( 
        /* [in] */ BSTR /*checked*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::form( 
        /* [retval][out] */ IDOMHTMLElement** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    COMPtr<IDOMElement> domElement;
    domElement.adoptRef(DOMHTMLElement::createInstance(inputElement->form()));
    if (domElement)
        return domElement->QueryInterface(IID_IDOMHTMLElement, (void**) result);
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::accept( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setAccept( 
        /* [in] */ BSTR /*accept*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::accessKey( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setAccessKey( 
        /* [in] */ BSTR /*key*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::align( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setAlign( 
        /* [in] */ BSTR /*align*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::alt( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setAlt( 
        /* [in] */ BSTR /*alt*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::checked( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setChecked( 
        /* [in] */ BOOL /*checked*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::disabled( 
        /* [retval][out] */ BOOL* result)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *result = inputElement->isDisabledFormControl() ? TRUE : FALSE;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setDisabled( 
        /* [in] */ BOOL /*disabled*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::maxLength( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setMaxLength( 
        /* [in] */ int /*maxLength*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::name( 
        /* [retval][out] */ BSTR* /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setName( 
        /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::readOnly( 
        /* [retval][out] */ BOOL* result)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *result = inputElement->isReadOnly() ? TRUE : FALSE;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setReadOnly( 
        /* [in] */ BOOL /*readOnly*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::size( 
        /* [retval][out] */ unsigned int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setSize( 
        /* [in] */ unsigned int /*size*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::src( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setSrc( 
        /* [in] */ BSTR /*src*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::tabIndex( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setTabIndex( 
        /* [in] */ int /*tabIndex*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::type( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setType( 
        /* [in] */ BSTR type)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    WTF::String typeString(type, SysStringLen(type));
    inputElement->setType(typeString);
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::useMap( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setUseMap( 
        /* [in] */ BSTR /*useMap*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::value( 
        /* [retval][out] */ BSTR* result)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    WTF::String valueString = inputElement->value();
    *result = BString(valueString.characters(), valueString.length()).release();
    if (valueString.length() && !*result)
        return E_OUTOFMEMORY;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setValue( 
        /* [in] */ BSTR value)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->setValue(String((UChar*) value, SysStringLen(value)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setValueForUser(
        /* [in] */ BSTR value)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->setValueForUser(String(static_cast<UChar*>(value), SysStringLen(value)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::select( void)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->select();
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::click( void)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setSelectionStart( 
    /* [in] */ long start)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->setSelectionStart(start);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::selectionStart( 
    /* [retval][out] */ long *start)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *start = inputElement->selectionStart();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setSelectionEnd( 
    /* [in] */ long end)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->setSelectionEnd(end);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::selectionEnd( 
    /* [retval][out] */ long *end)
{
    ASSERT(m_element && isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *end = inputElement->selectionEnd();
    return S_OK;
}

// DOMHTMLInputElement -- IFormsAutoFillTransition ----------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::isTextField(
    /* [retval][out] */ BOOL* result)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *result = inputElement->isTextField() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::rectOnScreen( 
    /* [retval][out] */ LPRECT rect)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    rect->left = rect->top = rect->right = rect->bottom = 0;
    RenderObject* renderer = m_element->renderer();
    FrameView* view = m_element->document()->view();
    if (!renderer || !view)
        return E_FAIL;

    IntRect coreRect = view->contentsToScreen(renderer->absoluteBoundingBoxRect());
    rect->left = coreRect.x();
    rect->top = coreRect.y();
    rect->right = coreRect.maxX();
    rect->bottom = coreRect.maxY();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::replaceCharactersInRange( 
    /* [in] */ int startTarget,
    /* [in] */ int endTarget,
    /* [in] */ BSTR replacementString,
    /* [in] */ int index)
{
    if (!replacementString)
        return E_POINTER;

    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);

    String newValue = inputElement->value();
    String webCoreReplacementString(static_cast<UChar*>(replacementString), SysStringLen(replacementString));
    newValue.replace(startTarget, endTarget - startTarget, webCoreReplacementString);
    inputElement->setValue(newValue);
    inputElement->setSelectionRange(index, newValue.length());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::selectedRange( 
    /* [out] */ int* start,
    /* [out] */ int* end)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *start = inputElement->selectionStart();
    *end = inputElement->selectionEnd();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::setAutofilled( 
    /* [in] */ BOOL filled)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    inputElement->setAutofilled(!!filled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::isAutofilled(
    /* [retval][out] */ BOOL* result)
{
    ASSERT(m_element);
    ASSERT(isHTMLInputElement(m_element));
    HTMLInputElement* inputElement = toHTMLInputElement(m_element);
    *result = inputElement->isAutofilled() ? TRUE : FALSE;
    return S_OK;
}

// DOMHTMLInputElement -- IFormPromptAdditions ------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLInputElement::isUserEdited( 
    /* [retval][out] */ BOOL *result)
{
    if (!result)
        return E_POINTER;

    *result = FALSE;
    ASSERT(m_element && isHTMLInputElement(m_element));
    BOOL textField = FALSE;
    if (FAILED(isTextField(&textField)) || !textField)
        return S_OK;
    if (toHTMLInputElement(m_element)->lastChangeWasUserEdit())
        *result = TRUE;
    return S_OK;
}

// DOMHTMLTextAreaElement - IUnknown ----------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLTextAreaElement))
        *ppvObject = static_cast<IDOMHTMLTextAreaElement*>(this);
    else if (IsEqualGUID(riid, IID_IFormPromptAdditions))
        *ppvObject = static_cast<IFormPromptAdditions*>(this);    
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLTextAreaElement -----------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::defaultValue( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setDefaultValue( 
        /* [in] */ BSTR /*val*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::form( 
        /* [retval][out] */ IDOMHTMLElement** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;
    ASSERT(m_element && isHTMLTextAreaElement(m_element));
    HTMLTextAreaElement* textareaElement = toHTMLTextAreaElement(m_element);
    COMPtr<IDOMElement> domElement;
    domElement.adoptRef(DOMHTMLElement::createInstance(textareaElement->form()));
    if (domElement)
        return domElement->QueryInterface(IID_IDOMHTMLElement, (void**) result);
    return E_FAIL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::accessKey( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setAccessKey( 
        /* [in] */ BSTR /*key*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::cols( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setCols( 
        /* [in] */ int /*cols*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::disabled( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setDisabled( 
        /* [in] */ BOOL /*disabled*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::name( 
        /* [retval][out] */ BSTR* /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setName( 
        /* [in] */ BSTR /*name*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::readOnly( 
        /* [retval][out] */ BOOL* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setReadOnly( 
        /* [in] */ BOOL /*readOnly*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::rows( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setRows( 
        /* [in] */ int /*rows*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::tabIndex( 
        /* [retval][out] */ int* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setTabIndex( 
        /* [in] */ int /*tabIndex*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::type( 
        /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::value( 
        /* [retval][out] */ BSTR* result)
{
    ASSERT(m_element && isHTMLTextAreaElement(m_element));
    HTMLTextAreaElement* textareaElement = toHTMLTextAreaElement(m_element);
    WTF::String valueString = textareaElement->value();
    *result = BString(valueString.characters(), valueString.length()).release();
    if (valueString.length() && !*result)
        return E_OUTOFMEMORY;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::setValue( 
        /* [in] */ BSTR value)
{
    ASSERT(m_element && isHTMLTextAreaElement(m_element));
    HTMLTextAreaElement* textareaElement = toHTMLTextAreaElement(m_element);
    textareaElement->setValue(String((UChar*) value, SysStringLen(value)));
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::select( void)
{
    ASSERT(m_element && isHTMLTextAreaElement(m_element));
    HTMLTextAreaElement* textareaElement = toHTMLTextAreaElement(m_element);
    textareaElement->select();
    return S_OK;
}

// DOMHTMLTextAreaElement -- IFormPromptAdditions ------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLTextAreaElement::isUserEdited( 
    /* [retval][out] */ BOOL *result)
{
    if (!result)
        return E_POINTER;

    *result = FALSE;
    ASSERT(m_element && isHTMLTextAreaElement(m_element));
    if (toHTMLTextAreaElement(m_element)->lastChangeWasUserEdit())
        *result = TRUE;
    return S_OK;
}

// DOMHTMLIFrameElement - IUnknown --------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLIFrameElement::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IDOMHTMLIFrameElement))
        *ppvObject = static_cast<IDOMHTMLIFrameElement*>(this);
    else
        return DOMHTMLElement::QueryInterface(riid, ppvObject);

    AddRef();
    return S_OK;
}

// DOMHTMLIFrameElement -------------------------------------------------------------

HRESULT STDMETHODCALLTYPE DOMHTMLIFrameElement::contentFrame( 
    /* [retval][out] */ IWebFrame **result)
{
    if (!result)
        return E_POINTER;
    *result = 0;
    ASSERT(m_element);
    HTMLIFrameElement* iFrameElement = toHTMLIFrameElement(m_element);
    COMPtr<IWebFrame> webFrame = kit(iFrameElement->contentFrame());
    if (!webFrame)
        return E_FAIL;
    return webFrame.copyRefTo(result);
}
