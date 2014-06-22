/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDesktopNotificationsDelegate.h"
#include "WebSecurityOrigin.h"
#include "WebView.h"
#include <WebCore/BString.h>
#include <WebCore/Document.h>
#include <WebCore/KURL.h>

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)

using namespace WebCore;

class NotificationCOMWrapper : public IWebDesktopNotification {
public:
    static NotificationCOMWrapper* create(Notification* inner) { return new NotificationCOMWrapper(inner); }
   
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebDesktopNotification
    HRESULT STDMETHODCALLTYPE isHTML(BOOL* result);
    HRESULT STDMETHODCALLTYPE contentsURL(BSTR* result);
    HRESULT STDMETHODCALLTYPE iconURL(BSTR* result);
    HRESULT STDMETHODCALLTYPE title(BSTR* result);
    HRESULT STDMETHODCALLTYPE text(BSTR* result);
    HRESULT STDMETHODCALLTYPE notifyDisplay();
    HRESULT STDMETHODCALLTYPE notifyError();
    HRESULT STDMETHODCALLTYPE notifyClose(BOOL xplicit);

private:
    NotificationCOMWrapper(Notification* inner) : m_refCount(1), m_inner(inner) {}

    int m_refCount;
    Notification* m_inner;
};

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<NotificationCOMWrapper*>(this);
    else if (IsEqualGUID(riid, IID_IWebDesktopNotification))
        *ppvObject = static_cast<NotificationCOMWrapper*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE NotificationCOMWrapper::AddRef()
{ 
    return ++m_refCount; 
}

ULONG STDMETHODCALLTYPE NotificationCOMWrapper::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);
    return newRef;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::isHTML(BOOL* result)
{
    *result = m_inner->isHTML();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::contentsURL(BSTR* result)
{
    *result = BString(m_inner->url()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::iconURL(BSTR* result)
{
    *result = BString(m_inner->contents().icon()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::title(BSTR* result)
{
    *result = BString(m_inner->contents().title()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::text(BSTR* result)
{
    *result = BString(m_inner->contents().body()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::notifyDisplay()
{
    m_inner->dispatchDisplayEvent();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::notifyError()
{
    m_inner->dispatchErrorEvent();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE NotificationCOMWrapper::notifyClose(BOOL xplicit)
{
    m_inner->dispatchCloseEvent();
    return S_OK;
}

WebDesktopNotificationsDelegate::WebDesktopNotificationsDelegate(WebView* webView)
    : m_webView(webView)
{
}

bool WebDesktopNotificationsDelegate::show(Notification* object)
{
    if (hasNotificationDelegate())
        notificationDelegate()->showDesktopNotification(NotificationCOMWrapper::create(object));
    return true;
}

void WebDesktopNotificationsDelegate::cancel(Notification* object)
{
    if (hasNotificationDelegate())
        notificationDelegate()->cancelDesktopNotification(NotificationCOMWrapper::create(object));
}

void WebDesktopNotificationsDelegate::notificationObjectDestroyed(Notification* object)
{
    if (hasNotificationDelegate())
        notificationDelegate()->notificationDestroyed(NotificationCOMWrapper::create(object));
}

void WebDesktopNotificationsDelegate::notificationControllerDestroyed()
{
}

void WebDesktopNotificationsDelegate::requestPermission(SecurityOrigin* origin, PassRefPtr<VoidCallback> callback)
{
    BString org(origin->toString());
    if (hasNotificationDelegate())
        notificationDelegate()->requestNotificationPermission(org);
}

void WebDesktopNotificationsDelegate::requestPermission(SecurityOrigin*, PassRefPtr<NotificationPermissionCallback>)
{
}

void WebDesktopNotificationsDelegate::cancelRequestsForPermission(ScriptExecutionContext* context)
{
}

NotificationClient::Permission WebDesktopNotificationsDelegate::checkPermission(const KURL& url)
{
    int out = 0;
    BString org(SecurityOrigin::create(url)->toString());
    if (hasNotificationDelegate())
        notificationDelegate()->checkNotificationPermission(org, &out);
    return (NotificationClient::Permission) out;
}

bool WebDesktopNotificationsDelegate::hasNotificationDelegate()
{
    COMPtr<IWebUIDelegate> ui;
    m_webView->uiDelegate(&ui);

    COMPtr<IWebUIDelegate2> ui2;
    return SUCCEEDED(ui->QueryInterface(IID_IWebUIDelegate2, (void**) &ui2));
}

COMPtr<IWebDesktopNotificationsDelegate> WebDesktopNotificationsDelegate::notificationDelegate()
{
    COMPtr<IWebUIDelegate> ui;
    m_webView->uiDelegate(&ui);

    COMPtr<IWebUIDelegate2> ui2;
    COMPtr<IWebDesktopNotificationsDelegate> delegate;
    if (SUCCEEDED(ui->QueryInterface(IID_IWebUIDelegate2, (void**) &ui2)))
        ui2->desktopNotificationsDelegate(&delegate);

    return delegate;
}

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
