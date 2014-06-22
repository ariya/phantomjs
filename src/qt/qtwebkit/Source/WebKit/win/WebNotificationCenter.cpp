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

#include "config.h"
#include "WebKitDLL.h"
#include "WebNotificationCenter.h"

#include "WebNotification.h"
#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <utility>
#include <wchar.h>
#include <wtf/HashMap.h>
#include <wtf/HashTraits.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

typedef std::pair<COMPtr<IUnknown>, COMPtr<IWebNotificationObserver> > ObjectObserverPair;
typedef Vector<ObjectObserverPair> ObjectObserverList;
typedef ObjectObserverList::iterator ObserverListIterator;
typedef HashMap<String, ObjectObserverList> MappedObservers;

struct WebNotificationCenterPrivate {
    MappedObservers m_mappedObservers;
};

// WebNotificationCenter ----------------------------------------------------------------

IWebNotificationCenter* WebNotificationCenter::m_defaultCenter = 0;

WebNotificationCenter::WebNotificationCenter()
    : m_refCount(0)
    , d(adoptPtr(new WebNotificationCenterPrivate))
{
    gClassCount++;
    gClassNameCount.add("WebNotificationCenter");
}

WebNotificationCenter::~WebNotificationCenter()
{
    gClassCount--;
    gClassNameCount.remove("WebNotificationCenter");
}

WebNotificationCenter* WebNotificationCenter::createInstance()
{
    WebNotificationCenter* instance = new WebNotificationCenter();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebNotificationCenter::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebNotificationCenter*>(this);
    else if (IsEqualGUID(riid, IID_IWebNotificationCenter))
        *ppvObject = static_cast<IWebNotificationCenter*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebNotificationCenter::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebNotificationCenter::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

IWebNotificationCenter* WebNotificationCenter::defaultCenterInternal()
{
    if (!m_defaultCenter)
        m_defaultCenter = WebNotificationCenter::createInstance();
    return m_defaultCenter;
}

void WebNotificationCenter::postNotificationInternal(IWebNotification* notification, BSTR notificationName, IUnknown* anObject)
{
    String name(notificationName, SysStringLen(notificationName));
    MappedObservers::iterator it = d->m_mappedObservers.find(name);
    if (it == d->m_mappedObservers.end())
        return;

    // Intentionally make a copy of the list to avoid the possibility of errors
    // from a mutation of the list in the onNotify callback.
    ObjectObserverList list = it->value;

    ObserverListIterator end = list.end();
    for (ObserverListIterator it2 = list.begin(); it2 != end; ++it2) {
        IUnknown* observedObject = it2->first.get();
        IWebNotificationObserver* observer = it2->second.get();
        if (!observedObject || !anObject || observedObject == anObject)
            observer->onNotify(notification);
    }
}

// IWebNotificationCenter -----------------------------------------------------

HRESULT STDMETHODCALLTYPE WebNotificationCenter::defaultCenter( 
    /* [retval][out] */ IWebNotificationCenter** center)
{
    *center = defaultCenterInternal();
    (*center)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebNotificationCenter::addObserver( 
    /* [in] */ IWebNotificationObserver* observer,
    /* [in] */ BSTR notificationName,
    /* [in] */ IUnknown* anObject)
{
    String name(notificationName, SysStringLen(notificationName));
    MappedObservers::iterator it = d->m_mappedObservers.find(name);
    if (it != d->m_mappedObservers.end())
        it->value.append(ObjectObserverPair(anObject, observer));
    else {
        ObjectObserverList list;
        list.append(ObjectObserverPair(anObject, observer));
        d->m_mappedObservers.add(name, list);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebNotificationCenter::postNotification( 
    /* [in] */ IWebNotification* notification)
{
    BString name;
    HRESULT hr = notification->name(&name);
    if (FAILED(hr))
        return hr;

    COMPtr<IUnknown> obj;
    hr = notification->getObject(&obj);
    if (FAILED(hr))
        return hr;

    postNotificationInternal(notification, name, obj.get());

    return hr;
}

HRESULT STDMETHODCALLTYPE WebNotificationCenter::postNotificationName( 
    /* [in] */ BSTR notificationName,
    /* [in] */ IUnknown* anObject,
    /* [optional][in] */ IPropertyBag* userInfo)
{
    COMPtr<WebNotification> notification(AdoptCOM, WebNotification::createInstance(notificationName, anObject, userInfo));
    postNotificationInternal(notification.get(), notificationName, anObject);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebNotificationCenter::removeObserver( 
    /* [in] */ IWebNotificationObserver* anObserver,
    /* [in] */ BSTR notificationName,
    /* [optional][in] */ IUnknown* anObject)
{
    String name(notificationName, SysStringLen(notificationName));
    MappedObservers::iterator it = d->m_mappedObservers.find(name);
    if (it == d->m_mappedObservers.end())
        return E_FAIL;

    ObjectObserverList& observerList = it->value;
    ObserverListIterator end = observerList.end();

    int i = 0;
    for (ObserverListIterator it2 = observerList.begin(); it2 != end; ++it2, ++i) {
        IUnknown* observedObject = it2->first.get();
        IWebNotificationObserver* observer = it2->second.get();
        if (observer == anObserver && (!anObject || anObject == observedObject)) {
            observerList.remove(i);
            break;
        }
    }

    if (observerList.isEmpty())
        d->m_mappedObservers.remove(name);

    return S_OK;
}
