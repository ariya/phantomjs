/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "WebIconDatabase.h"

#include "CFDictionaryPropertyBag.h"
#include "WebNotificationCenter.h"
#include "WebPreferences.h"
#include "shlobj.h"
#include <WebCore/BString.h>
#include <WebCore/BitmapInfo.h>
#include <WebCore/COMPtr.h>
#include <WebCore/FileSystem.h>
#include <WebCore/HWndDC.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/Image.h>
#include <WebCore/SharedBuffer.h>
#include <wtf/MainThread.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;
using namespace WTF;

// WebIconDatabase ----------------------------------------------------------------

WebIconDatabase* WebIconDatabase::m_sharedWebIconDatabase = 0;

WebIconDatabase::WebIconDatabase()
: m_refCount(0)
, m_deliveryRequested(false)
{
    gClassCount++;
    gClassNameCount.add("WebIconDatabase");
}

WebIconDatabase::~WebIconDatabase()
{
    gClassCount--;
    gClassNameCount.remove("WebIconDatabase");
}

void WebIconDatabase::init()
{
    WebPreferences* standardPrefs = WebPreferences::sharedStandardPreferences();
    BOOL enabled = FALSE;
    if (FAILED(standardPrefs->iconDatabaseEnabled(&enabled))) {
        enabled = FALSE;
        LOG_ERROR("Unable to get icon database enabled preference");
    }
    iconDatabase().setEnabled(!!enabled);
    if (!(!!enabled))
        return;

    startUpIconDatabase();
}

void WebIconDatabase::startUpIconDatabase()
{
    WebPreferences* standardPrefs = WebPreferences::sharedStandardPreferences();

    iconDatabase().setClient(this);

    BString prefDatabasePath;
    if (FAILED(standardPrefs->iconDatabaseLocation(&prefDatabasePath)))
        LOG_ERROR("Unable to get icon database location preference");

    String databasePath(prefDatabasePath, SysStringLen(prefDatabasePath));

    if (databasePath.isEmpty()) {
        databasePath = localUserSpecificStorageDirectory();
        if (databasePath.isEmpty())
            LOG_ERROR("Failed to construct default icon database path");
    }

    if (!iconDatabase().open(databasePath, WebCore::IconDatabase::defaultDatabaseFilename()))
            LOG_ERROR("Failed to open icon database path");
}

void WebIconDatabase::shutDownIconDatabase()
{
}

WebIconDatabase* WebIconDatabase::createInstance()
{
    WebIconDatabase* instance = new WebIconDatabase();
    instance->AddRef();
    return instance;
}

WebIconDatabase* WebIconDatabase::sharedWebIconDatabase()
{
    if (m_sharedWebIconDatabase) {
        m_sharedWebIconDatabase->AddRef();
        return m_sharedWebIconDatabase;
    }
    m_sharedWebIconDatabase = createInstance();
    m_sharedWebIconDatabase->init();
    return m_sharedWebIconDatabase;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebIconDatabase::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebIconDatabase*>(this);
    else if (IsEqualGUID(riid, IID_IWebIconDatabase))
        *ppvObject = static_cast<IWebIconDatabase*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebIconDatabase::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebIconDatabase::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebIconDatabase --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebIconDatabase::sharedIconDatabase(
        /* [retval][out] */ IWebIconDatabase** result)
{
    *result = sharedWebIconDatabase();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::iconForURL(
        /* [in] */ BSTR url,
        /* [optional][in] */ LPSIZE size,
        /* [optional][in] */ BOOL /*cache*/,
        /* [retval][out] */ OLE_HANDLE* bitmap)
{
    IntSize intSize(*size);

    Image* icon = 0;
    if (url)
        icon = iconDatabase().synchronousIconForPageURL(String(url, SysStringLen(url)), intSize);

    // Make sure we check for the case of an "empty image"
    if (icon && icon->width()) {
        *bitmap = (OLE_HANDLE)(ULONG64)getOrCreateSharedBitmap(size);
        if (!icon->getHBITMAPOfSize((HBITMAP)(ULONG64)*bitmap, size)) {
            LOG_ERROR("Failed to draw Image to HBITMAP");
            *bitmap = 0;
            return E_FAIL;
        }
        return S_OK;
    }

    return defaultIconWithSize(size, bitmap);
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::defaultIconWithSize(
        /* [in] */ LPSIZE size,
        /* [retval][out] */ OLE_HANDLE* result)
{
    *result = (OLE_HANDLE)(ULONG64)getOrCreateDefaultIconBitmap(size);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::retainIconForURL(
        /* [in] */ BSTR url)
{
    iconDatabase().retainIconForPageURL(String(url, SysStringLen(url)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::releaseIconForURL(
        /* [in] */ BSTR url)
{
    iconDatabase().releaseIconForPageURL(String(url, SysStringLen(url)));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::removeAllIcons(void)
{
    iconDatabase().removeAllIcons();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::delayDatabaseCleanup(void)
{
    IconDatabase::delayDatabaseCleanup();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::allowDatabaseCleanup(void)
{
    IconDatabase::allowDatabaseCleanup();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::iconURLForURL( 
        /* [in] */ BSTR url,
        /* [retval][out] */ BSTR* iconURL)
{
    if (!url || !iconURL)
        return E_POINTER;
    BString iconURLBSTR(iconDatabase().synchronousIconURLForPageURL(String(url, SysStringLen(url))));
    *iconURL = iconURLBSTR.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::isEnabled( 
        /* [retval][out] */ BOOL *result)
{
    *result = iconDatabase().isEnabled();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::setEnabled( 
        /* [in] */ BOOL flag)
{
    BOOL currentlyEnabled;
    isEnabled(&currentlyEnabled);
    if (currentlyEnabled && !flag) {
        iconDatabase().setEnabled(false);
        shutDownIconDatabase();
    } else if (!currentlyEnabled && flag) {
        iconDatabase().setEnabled(true);
        startUpIconDatabase();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebIconDatabase::hasIconForURL(
        /* [in] */ BSTR url,
        /* [out][retval] */ BOOL* result)
{
    if (!url || !result)
        return E_POINTER;

    String urlString(url, SysStringLen(url));

    // Passing a size parameter of 0, 0 means we don't care about the result of the image, we just
    // want to make sure the read from disk to load the icon is kicked off.
    iconDatabase().synchronousIconForPageURL(urlString, IntSize(0, 0));

    // Check to see if we have a non-empty icon URL for the page, and if we do, we have an icon for
    // the page.
    *result = !(iconDatabase().synchronousIconURLForPageURL(urlString).isEmpty());

    return S_OK;
}

HBITMAP createDIB(LPSIZE size)
{
    BitmapInfo bmInfo = BitmapInfo::create(IntSize(*size));

    HWndDC dc(0);
    return CreateDIBSection(dc, &bmInfo, DIB_RGB_COLORS, 0, 0, 0);
}

HBITMAP WebIconDatabase::getOrCreateSharedBitmap(LPSIZE size)
{
    HBITMAP result = m_sharedIconMap.get(*size);
    if (result)
        return result;
    result = createDIB(size);
    m_sharedIconMap.set(*size, result);
    return result;
}

HBITMAP WebIconDatabase::getOrCreateDefaultIconBitmap(LPSIZE size)
{
    HBITMAP result = m_defaultIconMap.get(*size);
    if (result)
        return result;

    result = createDIB(size);

    m_defaultIconMap.set(*size, result);
    if (!iconDatabase().defaultIcon(*size) || !iconDatabase().defaultIcon(*size)->getHBITMAPOfSize(result, size)) {
        LOG_ERROR("Failed to draw Image to HBITMAP");
        return 0;
    }
    return result;
}

// IconDatabaseClient

void WebIconDatabase::didRemoveAllIcons()
{
    // Queueing the empty string is a special way of saying "this queued notification is the didRemoveAllIcons notification"
    MutexLocker locker(m_notificationMutex);
    m_notificationQueue.append(String());
    scheduleNotificationDelivery();
}

void WebIconDatabase::didImportIconURLForPageURL(const WTF::String& pageURL)
{
    MutexLocker locker(m_notificationMutex);
    m_notificationQueue.append(pageURL.isolatedCopy());
    scheduleNotificationDelivery();
}

void WebIconDatabase::didImportIconDataForPageURL(const WTF::String& pageURL)
{
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}

void WebIconDatabase::didChangeIconForPageURL(const WTF::String& pageURL)
{
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}

void WebIconDatabase::didFinishURLImport()
{
}

void WebIconDatabase::scheduleNotificationDelivery()
{
    // Caller of this method must hold the m_notificationQueue lock
    ASSERT(!m_notificationMutex.tryLock());

    if (!m_deliveryRequested) {
        m_deliveryRequested = true;
        callOnMainThread(deliverNotifications, 0);
    }
}

BSTR WebIconDatabase::iconDatabaseDidAddIconNotification()
{
    static BSTR didAddIconName = SysAllocString(WebIconDatabaseDidAddIconNotification);
    return didAddIconName;
}

CFStringRef WebIconDatabase::iconDatabaseNotificationUserInfoURLKey()
{
    static CFStringRef iconUserInfoURLKey = String(WebIconNotificationUserInfoURLKey).createCFString().leakRef();
    return iconUserInfoURLKey;
}

BSTR WebIconDatabase::iconDatabaseDidRemoveAllIconsNotification()
{
    static BSTR didRemoveAllIconsName = SysAllocString(WebIconDatabaseDidRemoveAllIconsNotification);
    return didRemoveAllIconsName;
}

static void postDidRemoveAllIconsNotification(WebIconDatabase* iconDB)
{
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->postNotificationName(WebIconDatabase::iconDatabaseDidRemoveAllIconsNotification(), static_cast<IWebIconDatabase*>(iconDB), 0);
}

static void postDidAddIconNotification(const String& pageURL, WebIconDatabase* iconDB)
{
    RetainPtr<CFMutableDictionaryRef> dictionary = adoptCF(
    CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    CFDictionaryAddValue(dictionary.get(), WebIconDatabase::iconDatabaseNotificationUserInfoURLKey(), pageURL.createCFString().get());

    COMPtr<CFDictionaryPropertyBag> userInfo = CFDictionaryPropertyBag::createInstance();
    userInfo->setDictionary(dictionary.get());

    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();
    notifyCenter->postNotificationName(WebIconDatabase::iconDatabaseDidAddIconNotification(), static_cast<IWebIconDatabase*>(iconDB), userInfo.get());
}

void WebIconDatabase::deliverNotifications(void*)
{
    ASSERT(m_sharedWebIconDatabase);
    if (!m_sharedWebIconDatabase)
        return;

    ASSERT(m_sharedWebIconDatabase->m_deliveryRequested);

    Vector<String> queue;
    {
        MutexLocker locker(m_sharedWebIconDatabase->m_notificationMutex);
        queue.swap(m_sharedWebIconDatabase->m_notificationQueue);
        m_sharedWebIconDatabase->m_deliveryRequested = false;
    }

    for (unsigned i = 0; i < queue.size(); ++i) {
        if (queue[i].isNull())
            postDidRemoveAllIconsNotification(m_sharedWebIconDatabase);
        else
            postDidAddIconNotification(queue[i], m_sharedWebIconDatabase);
    }
}
