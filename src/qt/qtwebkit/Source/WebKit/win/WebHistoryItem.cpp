/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "WebHistoryItem.h"

#include "COMEnumVariant.h"
#include "MarshallingHelpers.h"
#include "WebKit.h"
#include <WebCore/BString.h>
#include <WebCore/COMPtr.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/KURL.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

// WebHistoryItem ----------------------------------------------------------------

static HashMap<HistoryItem*, WebHistoryItem*>& historyItemWrappers()
{
    static HashMap<HistoryItem*, WebHistoryItem*> staticHistoryItemWrappers;
    return staticHistoryItemWrappers;
}

WebHistoryItem::WebHistoryItem(PassRefPtr<HistoryItem> historyItem)
: m_refCount(0)
, m_historyItem(historyItem)
{
    ASSERT(!historyItemWrappers().contains(m_historyItem.get()));
    historyItemWrappers().set(m_historyItem.get(), this);

    gClassCount++;
    gClassNameCount.add("WebHistoryItem");
}

WebHistoryItem::~WebHistoryItem()
{
    ASSERT(historyItemWrappers().contains(m_historyItem.get()));
    historyItemWrappers().remove(m_historyItem.get());

    gClassCount--;
    gClassNameCount.remove("WebHistoryItem");
}

WebHistoryItem* WebHistoryItem::createInstance()
{
    WebHistoryItem* instance = new WebHistoryItem(HistoryItem::create());
    instance->AddRef();
    return instance;
}

WebHistoryItem* WebHistoryItem::createInstance(PassRefPtr<HistoryItem> historyItem)
{
    WebHistoryItem* instance;

    instance = historyItemWrappers().get(historyItem.get());

    if (!instance)
        instance = new WebHistoryItem(historyItem);

    instance->AddRef();
    return instance;
}

// IWebHistoryItemPrivate -----------------------------------------------------

static CFStringRef urlKey = CFSTR("");
static CFStringRef lastVisitedDateKey = CFSTR("lastVisitedDate");
static CFStringRef titleKey = CFSTR("title");
static CFStringRef visitCountKey = CFSTR("visitCount");
static CFStringRef lastVisitWasFailureKey = CFSTR("lastVisitWasFailure");
static CFStringRef lastVisitWasHTTPNonGetKey = CFSTR("lastVisitWasHTTPNonGet");
static CFStringRef redirectURLsKey = CFSTR("redirectURLs");
static CFStringRef dailyVisitCountKey = CFSTR("D"); // short key to save space
static CFStringRef weeklyVisitCountKey = CFSTR("W"); // short key to save space

HRESULT STDMETHODCALLTYPE WebHistoryItem::initFromDictionaryRepresentation(void* dictionary)
{
    CFDictionaryRef dictionaryRef = (CFDictionaryRef) dictionary;

    CFStringRef urlStringRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, urlKey);
    if (urlStringRef && CFGetTypeID(urlStringRef) != CFStringGetTypeID())
        return E_FAIL;

    CFStringRef lastVisitedRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, lastVisitedDateKey);
    if (!lastVisitedRef || CFGetTypeID(lastVisitedRef) != CFStringGetTypeID())
        return E_FAIL;
    CFAbsoluteTime lastVisitedTime = CFStringGetDoubleValue(lastVisitedRef);

    CFStringRef titleRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, titleKey);
    if (titleRef && CFGetTypeID(titleRef) != CFStringGetTypeID())
        return E_FAIL;

    CFNumberRef visitCountRef = (CFNumberRef) CFDictionaryGetValue(dictionaryRef, visitCountKey);
    if (!visitCountRef || CFGetTypeID(visitCountRef) != CFNumberGetTypeID())
        return E_FAIL;
    int visitedCount = 0;
    if (!CFNumberGetValue(visitCountRef, kCFNumberIntType, &visitedCount))
        return E_FAIL;

    // Can't trust data on disk, and we've had at least one report of this (<rdar://6572300>).
    if (visitedCount < 0) {
        LOG_ERROR("visit count for history item \"%s\" is negative (%d), will be reset to 1", String(urlStringRef).utf8().data(), visitedCount);
        visitedCount = 1;
    }

    CFBooleanRef lastVisitWasFailureRef = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionaryRef, lastVisitWasFailureKey));
    if (lastVisitWasFailureRef && CFGetTypeID(lastVisitWasFailureRef) != CFBooleanGetTypeID())
        return E_FAIL;
    bool lastVisitWasFailure = lastVisitWasFailureRef && CFBooleanGetValue(lastVisitWasFailureRef);

    CFBooleanRef lastVisitWasHTTPNonGetRef = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionaryRef, lastVisitWasHTTPNonGetKey));
    if (lastVisitWasHTTPNonGetRef && CFGetTypeID(lastVisitWasHTTPNonGetRef) != CFBooleanGetTypeID())
        return E_FAIL;
    bool lastVisitWasHTTPNonGet = lastVisitWasHTTPNonGetRef && CFBooleanGetValue(lastVisitWasHTTPNonGetRef);

    OwnPtr<Vector<String> > redirectURLsVector;
    if (CFArrayRef redirectURLsRef = static_cast<CFArrayRef>(CFDictionaryGetValue(dictionaryRef, redirectURLsKey))) {
        CFIndex size = CFArrayGetCount(redirectURLsRef);
        redirectURLsVector = adoptPtr(new Vector<String>(size));
        for (CFIndex i = 0; i < size; ++i)
            (*redirectURLsVector)[i] = String(static_cast<CFStringRef>(CFArrayGetValueAtIndex(redirectURLsRef, i)));
    }

    CFArrayRef dailyCounts = static_cast<CFArrayRef>(CFDictionaryGetValue(dictionaryRef, dailyVisitCountKey));
    if (dailyCounts && CFGetTypeID(dailyCounts) != CFArrayGetTypeID())
        dailyCounts = 0;
    CFArrayRef weeklyCounts = static_cast<CFArrayRef>(CFDictionaryGetValue(dictionaryRef, weeklyVisitCountKey));
    if (weeklyCounts && CFGetTypeID(weeklyCounts) != CFArrayGetTypeID())
        weeklyCounts = 0;

    std::auto_ptr<Vector<int> > dailyVector, weeklyVector;
    if (dailyCounts || weeklyCounts) {
        CFIndex dailySize = dailyCounts ? CFArrayGetCount(dailyCounts) : 0;
        CFIndex weeklySize = weeklyCounts ? CFArrayGetCount(weeklyCounts) : 0;
        dailyVector.reset(new Vector<int>(dailySize));
        weeklyVector.reset(new Vector<int>(weeklySize));

        // Daily and weekly counts < 0 are errors in the data read from disk, so reset to 0.
        for (CFIndex i = 0; i < dailySize; ++i) {
            CFNumberRef dailyCount = static_cast<CFNumberRef>(CFArrayGetValueAtIndex(dailyCounts, i));        
            if (CFGetTypeID(dailyCount) == CFNumberGetTypeID())
                CFNumberGetValue(dailyCount, kCFNumberIntType, &(*dailyVector)[i]);
            if ((*dailyVector)[i] < 0)
                (*dailyVector)[i] = 0;
        }
        for (CFIndex i = 0; i < weeklySize; ++i) {
            CFNumberRef weeklyCount = static_cast<CFNumberRef>(CFArrayGetValueAtIndex(weeklyCounts, i));        
            if (CFGetTypeID(weeklyCount) == CFNumberGetTypeID())
                CFNumberGetValue(weeklyCount, kCFNumberIntType, &(*weeklyVector)[i]);
            if ((*weeklyVector)[i] < 0)
                (*weeklyVector)[i] = 0;
        }
    }

    historyItemWrappers().remove(m_historyItem.get());
    m_historyItem = HistoryItem::create(urlStringRef, titleRef, lastVisitedTime);
    historyItemWrappers().set(m_historyItem.get(), this);

    m_historyItem->setVisitCount(visitedCount);
    if (lastVisitWasFailure)
        m_historyItem->setLastVisitWasFailure(true);

    if (lastVisitWasHTTPNonGet && (protocolIs(m_historyItem->urlString(), "http") || protocolIs(m_historyItem->urlString(), "https")))
        m_historyItem->setLastVisitWasHTTPNonGet(true);

    if (redirectURLsVector)
        m_historyItem->setRedirectURLs(redirectURLsVector.release());

    if (dailyVector.get())
        m_historyItem->adoptVisitCounts(*dailyVector, *weeklyVector);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::dictionaryRepresentation(void** dictionary)
{
    CFDictionaryRef* dictionaryRef = (CFDictionaryRef*) dictionary;
    static CFStringRef lastVisitedFormat = CFSTR("%.1lf");
    CFStringRef lastVisitedStringRef =
        CFStringCreateWithFormat(0, 0, lastVisitedFormat, m_historyItem->lastVisitedTime());
    if (!lastVisitedStringRef)
        return E_FAIL;

    int keyCount = 0;
    CFTypeRef keys[9];
    CFTypeRef values[9];

    if (!m_historyItem->urlString().isEmpty()) {
        keys[keyCount] = urlKey;
        values[keyCount++] = m_historyItem->urlString().createCFString().leakRef();
    }

    keys[keyCount] = lastVisitedDateKey;
    values[keyCount++] = lastVisitedStringRef;

    if (!m_historyItem->title().isEmpty()) {
        keys[keyCount] = titleKey;
        values[keyCount++] = m_historyItem->title().createCFString().leakRef();
    }

    keys[keyCount] = visitCountKey;
    int visitCount = m_historyItem->visitCount();
    values[keyCount++] = CFNumberCreate(0, kCFNumberIntType, &visitCount);

    if (m_historyItem->lastVisitWasFailure()) {
        keys[keyCount] = lastVisitWasFailureKey;
        values[keyCount++] = CFRetain(kCFBooleanTrue);
    }

    if (m_historyItem->lastVisitWasHTTPNonGet()) {
        ASSERT(m_historyItem->urlString().startsWith("http:", false) || m_historyItem->urlString().startsWith("https:", false));
        keys[keyCount] = lastVisitWasHTTPNonGetKey;
        values[keyCount++] = CFRetain(kCFBooleanTrue);
    }

    if (Vector<String>* redirectURLs = m_historyItem->redirectURLs()) {
        size_t size = redirectURLs->size();
        ASSERT(size);
        CFStringRef* items = new CFStringRef[size];
        for (size_t i = 0; i < size; ++i)
            items[i] = redirectURLs->at(i).createCFString().leakRef();
        CFArrayRef result = CFArrayCreate(0, (const void**)items, size, &kCFTypeArrayCallBacks);
        for (size_t i = 0; i < size; ++i)
            CFRelease(items[i]);
        delete[] items;

        keys[keyCount] = redirectURLsKey;
        values[keyCount++] = result;
    }

    const Vector<int>& dailyVisitCount(m_historyItem->dailyVisitCounts());
    if (size_t size = dailyVisitCount.size()) {
        Vector<CFNumberRef, 13> numbers(size);
        for (size_t i = 0; i < size; ++i)
            numbers[i] = CFNumberCreate(0, kCFNumberIntType, &dailyVisitCount[i]);

        CFArrayRef result = CFArrayCreate(0, (const void**)numbers.data(), size, &kCFTypeArrayCallBacks);

        for (size_t i = 0; i < size; ++i)
            CFRelease(numbers[i]);

        keys[keyCount] = dailyVisitCountKey;
        values[keyCount++] = result;
    }

    const Vector<int>& weeklyVisitCount(m_historyItem->weeklyVisitCounts());
    if (size_t size = weeklyVisitCount.size()) {
        Vector<CFNumberRef, 5> numbers(size);
        for (size_t i = 0; i < size; ++i)
            numbers[i] = CFNumberCreate(0, kCFNumberIntType, &weeklyVisitCount[i]);

        CFArrayRef result = CFArrayCreate(0, (const void**)numbers.data(), size, &kCFTypeArrayCallBacks);

        for (size_t i = 0; i < size; ++i)
            CFRelease(numbers[i]);

        keys[keyCount] = weeklyVisitCountKey;
        values[keyCount++] = result;
    }

    *dictionaryRef = CFDictionaryCreate(0, keys, values, keyCount, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    for (int i = 0; i < keyCount; ++i)
        CFRelease(values[i]);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::hasURLString(BOOL *hasURL)
{
    *hasURL = m_historyItem->urlString().isEmpty() ? FALSE : TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::visitCount(int *count)
{
    *count = m_historyItem->visitCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setVisitCount(int count)
{
    m_historyItem->setVisitCount(count);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::mergeAutoCompleteHints(IWebHistoryItem* otherItem)
{
    if (!otherItem)
        return E_FAIL;

    COMPtr<WebHistoryItem> otherWebHistoryItem(Query, otherItem);
    if (!otherWebHistoryItem)
        return E_FAIL;

    m_historyItem->mergeAutoCompleteHints(otherWebHistoryItem->historyItem());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitedTimeInterval(DATE time)
{
    m_historyItem->setLastVisitedTime(MarshallingHelpers::DATEToCFAbsoluteTime(time));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setTitle(BSTR title)
{
    m_historyItem->setTitle(String(title, SysStringLen(title)));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::RSSFeedReferrer(BSTR* url)
{
    BString str(m_historyItem->referrer());
    *url = str.release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setRSSFeedReferrer(BSTR url)
{
    m_historyItem->setReferrer(String(url, SysStringLen(url)));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::hasPageCache(BOOL* /*hasCache*/)
{
    // FIXME - TODO
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setHasPageCache(BOOL /*hasCache*/)
{
    // FIXME - TODO
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::target(BSTR* target)
{
    if (!target) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *target = BString(m_historyItem->target()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::isTargetItem(BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = m_historyItem->isTargetItem() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::children(unsigned* outChildCount, SAFEARRAY** outChildren)
{
    if (!outChildCount || !outChildren) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *outChildCount = 0;
    *outChildren = 0;

    const HistoryItemVector& coreChildren = m_historyItem->children();
    if (coreChildren.isEmpty())
        return S_OK;
    size_t childCount = coreChildren.size();

    SAFEARRAY* children = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(childCount));
    if (!children)
        return E_OUTOFMEMORY;

    for (unsigned i = 0; i < childCount; ++i) {
        COMPtr<WebHistoryItem> item(AdoptCOM, WebHistoryItem::createInstance(coreChildren[i]));
        if (!item) {
            SafeArrayDestroy(children);
            return E_OUTOFMEMORY;
        }

        LONG longI = i;
        HRESULT hr = SafeArrayPutElement(children, &longI, item.get());
        if (FAILED(hr)) {
            SafeArrayDestroy(children);
            return hr;
        }
    }

    *outChildCount = static_cast<unsigned>(childCount);
    *outChildren = children;
    return S_OK;

}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitWasFailure(BOOL* wasFailure)
{
    if (!wasFailure) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *wasFailure = m_historyItem->lastVisitWasFailure();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitWasFailure(BOOL wasFailure)
{
    m_historyItem->setLastVisitWasFailure(wasFailure);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitWasHTTPNonGet(BOOL* HTTPNonGet)
{
    if (!HTTPNonGet) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *HTTPNonGet = m_historyItem->lastVisitWasHTTPNonGet();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitWasHTTPNonGet(BOOL HTTPNonGet)
{
    m_historyItem->setLastVisitWasHTTPNonGet(HTTPNonGet);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::redirectURLs(IEnumVARIANT** urls)
{
    if (!urls) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    Vector<String>* urlVector = m_historyItem->redirectURLs();
    if (!urlVector) {
        *urls = 0;
        return S_OK;
    }

    COMPtr<COMEnumVariant<Vector<String> > > enumVariant(AdoptCOM, COMEnumVariant<Vector<String> >::createInstance(*urlVector));
    *urls = enumVariant.leakRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::visitedWithTitle(BSTR title, BOOL increaseVisitCount)
{
    m_historyItem->visited(title, CFAbsoluteTimeGetCurrent(), increaseVisitCount ? IncreaseVisitCount : DoNotIncreaseVisitCount);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::getDailyVisitCounts(int* number, int** counts)
{
    if (!number || !counts) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *counts = const_cast<int*>(m_historyItem->dailyVisitCounts().data());
    *number = m_historyItem->dailyVisitCounts().size();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::getWeeklyVisitCounts(int* number, int** counts)
{
    if (!number || !counts) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *counts = const_cast<int*>(m_historyItem->weeklyVisitCounts().data());
    *number = m_historyItem->weeklyVisitCounts().size();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::recordInitialVisit()
{
    m_historyItem->recordInitialVisit();
    return S_OK;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebHistoryItem::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, __uuidof(WebHistoryItem)))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebHistoryItem*>(this);
    else if (IsEqualGUID(riid, IID_IWebHistoryItem))
        *ppvObject = static_cast<IWebHistoryItem*>(this);
    else if (IsEqualGUID(riid, IID_IWebHistoryItemPrivate))
        *ppvObject = static_cast<IWebHistoryItemPrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebHistoryItem::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebHistoryItem::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebHistoryItem -------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebHistoryItem::initWithURLString(
    /* [in] */ BSTR urlString,
    /* [in] */ BSTR title,
    /* [in] */ DATE lastVisited)
{
    historyItemWrappers().remove(m_historyItem.get());
    m_historyItem = HistoryItem::create(String(urlString, SysStringLen(urlString)), String(title, SysStringLen(title)), MarshallingHelpers::DATEToCFAbsoluteTime(lastVisited));
    historyItemWrappers().set(m_historyItem.get(), this);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::originalURLString( 
    /* [retval][out] */ BSTR* url)
{
    if (!url)
        return E_POINTER;

    BString str = m_historyItem->originalURLString();
    *url = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::URLString( 
    /* [retval][out] */ BSTR* url)
{
    if (!url)
        return E_POINTER;

    BString str = m_historyItem->urlString();
    *url = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::title( 
    /* [retval][out] */ BSTR* pageTitle)
{
    if (!pageTitle)
        return E_POINTER;

    BString str(m_historyItem->title());
    *pageTitle = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitedTimeInterval( 
    /* [retval][out] */ DATE* lastVisited)
{
    if (!lastVisited)
        return E_POINTER;

    *lastVisited = MarshallingHelpers::CFAbsoluteTimeToDATE(m_historyItem->lastVisitedTime());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setAlternateTitle( 
    /* [in] */ BSTR title)
{
    m_alternateTitle = String(title, SysStringLen(title));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::alternateTitle( 
    /* [retval][out] */ BSTR* title)
{
    if (!title) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *title = BString(m_alternateTitle).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::icon( 
    /* [out, retval] */ OLE_HANDLE* /*hBitmap*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// WebHistoryItem -------------------------------------------------------------

HistoryItem* WebHistoryItem::historyItem() const
{
    return m_historyItem.get();
}
