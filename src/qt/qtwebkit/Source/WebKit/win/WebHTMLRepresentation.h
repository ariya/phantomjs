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

#ifndef WebHTMLRepresentation_H
#define WebHTMLRepresentation_H

#include "WebKit.h"

class WebFrame;

class WebHTMLRepresentation : public IWebHTMLRepresentation, IWebDocumentRepresentation
{
public:
    static WebHTMLRepresentation* createInstance(WebFrame* frame);
protected:
    WebHTMLRepresentation();
    ~WebHTMLRepresentation();
public:

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebHTMLRepresentation
    virtual HRESULT STDMETHODCALLTYPE supportedMIMETypes(
        /* [out][in] */ BSTR* types,
        /* [out][in] */ int* cTypes);
    
    virtual HRESULT STDMETHODCALLTYPE supportedNonImageMIMETypes(
        /* [out][in] */ BSTR* types,
        /* [out][in] */ int* cTypes);
    
    virtual HRESULT STDMETHODCALLTYPE supportedImageMIMETypes(
        /* [out][in] */ BSTR* types,
        /* [out][in] */ int* cTypes);
    
    virtual HRESULT STDMETHODCALLTYPE attributedStringFromDOMNodes(
        /* [in] */ IDOMNode* startNode,
        /* [in] */ int startOffset,
        /* [in] */ IDOMNode* endNode,
        /* [in] */ int endOffset,
        /* [retval][out] */ IDataObject** attributedString);
    
    virtual HRESULT STDMETHODCALLTYPE elementWithName(
        /* [in] */ BSTR name,
        /* [in] */ IDOMElement* form,
        /* [retval][out] */ IDOMElement** element);
    
    virtual HRESULT STDMETHODCALLTYPE elementDoesAutoComplete(
        /* [in] */ IDOMElement* element,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE elementIsPassword(
        /* [in] */ IDOMElement* element,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE formForElement(
        /* [in] */ IDOMElement* element,
        /* [retval][out] */ IDOMElement** form);
    
    virtual HRESULT STDMETHODCALLTYPE currentForm(
        /* [retval][out] */ IDOMElement** form);
    
    virtual HRESULT STDMETHODCALLTYPE controlsInForm(
        /* [in] */ IDOMElement* form,
        /* [out][in] */ IDOMElement** controls,
        /* [out][in] */ int* cControls);
    
    /* Deprecated. Use the variant that includes resultDistance and resultIsInCellAbove instead. */
    virtual HRESULT STDMETHODCALLTYPE deprecatedSearchForLabels(
        /* [size_is][in] */ BSTR *labels,
        /* [in] */ int cLabels,
        /* [in] */ IDOMElement *beforeElement,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE matchLabels(
        /* [size_is][in] */ BSTR *labels,
        /* [in] */ int cLabels,
        /* [in] */ IDOMElement *againstElement,
        /* [retval][out] */ BSTR *result);

    virtual HRESULT STDMETHODCALLTYPE searchForLabels(BSTR* labels, unsigned cLabels, IDOMElement* beforeElement, unsigned* resultDistance, BOOL* resultIsInCellAbove, BSTR* result);
    
    // IWebDocumentRepresentation
    virtual HRESULT STDMETHODCALLTYPE setDataSource(
        /* [in] */ IWebDataSource* dataSource);
    
    virtual HRESULT STDMETHODCALLTYPE receivedData(
        /* [in] */ IStream* data,
        /* [in] */ IWebDataSource* dataSource);
    
    virtual HRESULT STDMETHODCALLTYPE receivedError(
        /* [in] */ IWebError* error,
        /* [in] */ IWebDataSource* dataSource);
    
    virtual HRESULT STDMETHODCALLTYPE finishedLoadingWithDataSource(
        /* [in] */ IWebDataSource* dataSource);
    
    virtual HRESULT STDMETHODCALLTYPE canProvideDocumentSource(
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE documentSource(
        /* [retval][out] */ BSTR* source);
    
    virtual HRESULT STDMETHODCALLTYPE title(
        /* [retval][out] */ BSTR* docTitle);

protected:
    ULONG m_refCount;
    WebFrame* m_frame;
};

#endif
