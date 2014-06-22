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

#ifndef WebNotificationCenter_H
#define WebNotificationCenter_H

#include "WebKit.h"
#include <wtf/OwnPtr.h>

struct WebNotificationCenterPrivate;

class WebNotificationCenter : public IWebNotificationCenter {
public:
    static WebNotificationCenter* createInstance();

protected:
    WebNotificationCenter();
    ~WebNotificationCenter();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebNotificationCenter
    virtual HRESULT STDMETHODCALLTYPE defaultCenter( 
        /* [retval][out] */ IWebNotificationCenter **center);
    
    virtual HRESULT STDMETHODCALLTYPE addObserver( 
        /* [in] */ IWebNotificationObserver *observer,
        /* [in] */ BSTR notificationName,
        /* [in] */ IUnknown *anObject);
    
    virtual HRESULT STDMETHODCALLTYPE postNotification( 
        /* [in] */ IWebNotification *notification);
    
    virtual HRESULT STDMETHODCALLTYPE postNotificationName( 
        /* [in] */ BSTR notificationName,
        /* [in] */ IUnknown *anObject,
        /* [optional][in] */ IPropertyBag *userInfo);
    
    virtual HRESULT STDMETHODCALLTYPE removeObserver( 
        /* [in] */ IWebNotificationObserver *anObserver,
        /* [in] */ BSTR notificationName,
        /* [optional][in] */ IUnknown *anObject);

    // WebNotificationCenter
    static IWebNotificationCenter* defaultCenterInternal();
    void postNotificationInternal(IWebNotification* notification, BSTR notificationName, IUnknown* anObject);

protected:
    ULONG m_refCount;
    OwnPtr<WebNotificationCenterPrivate> d;
    static IWebNotificationCenter* m_defaultCenter;
};

#endif // WebNotificationCenter_H
