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

#ifndef WebHistory_H
#define WebHistory_H

#include "WebKit.h"
#include <CoreFoundation/CoreFoundation.h>
#include <WebCore/COMPtr.h>
#include <wtf/Forward.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RetainPtr.h>

namespace WebCore {
    class KURL;
    class PageGroup;
}

//-----------------------------------------------------------------------------

class WebPreferences;

class WebHistory : public IWebHistory, public IWebHistoryPrivate {
public:
    static WebHistory* createInstance();
private:
    WebHistory();
    ~WebHistory();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebHistory
    virtual HRESULT STDMETHODCALLTYPE optionalSharedHistory( 
        /* [retval][out] */ IWebHistory** history);
    
    virtual HRESULT STDMETHODCALLTYPE setOptionalSharedHistory( 
        /* [in] */ IWebHistory* history);
    
    virtual HRESULT STDMETHODCALLTYPE unused1() OVERRIDE;
    virtual HRESULT STDMETHODCALLTYPE unused2() OVERRIDE;
    
    virtual HRESULT STDMETHODCALLTYPE addItems( 
        /* [in] */ int itemCount,
        /* [in] */ IWebHistoryItem** items);
    
    virtual HRESULT STDMETHODCALLTYPE removeItems( 
        /* [in] */ int itemCount,
        /* [in] */ IWebHistoryItem** items);
    
    virtual HRESULT STDMETHODCALLTYPE removeAllItems( void);
    
    virtual HRESULT STDMETHODCALLTYPE orderedLastVisitedDays( 
        /* [out][in] */ int* count,
        /* [in] */ DATE* calendarDates);
    
    virtual HRESULT STDMETHODCALLTYPE orderedItemsLastVisitedOnDay( 
        /* [out][in] */ int* count,
        /* [in] */ IWebHistoryItem** items,
        /* [in] */ DATE calendarDate);

    virtual HRESULT STDMETHODCALLTYPE itemForURL( 
        /* [in] */ BSTR url,
        /* [retval][out] */ IWebHistoryItem** item);
    
    virtual HRESULT STDMETHODCALLTYPE setHistoryItemLimit( 
        /* [in] */ int limit);
    
    virtual HRESULT STDMETHODCALLTYPE historyItemLimit( 
        /* [retval][out] */ int* limit);
    
    virtual HRESULT STDMETHODCALLTYPE setHistoryAgeInDaysLimit( 
        /* [in] */ int limit);
    
    virtual HRESULT STDMETHODCALLTYPE historyAgeInDaysLimit( 
        /* [retval][out] */ int* limit);

    // IWebHistoryPrivate

    virtual HRESULT STDMETHODCALLTYPE allItems( 
        /* [out][in] */ int* count,
        /* [retval][out] */ IWebHistoryItem** items);

    virtual HRESULT STDMETHODCALLTYPE setVisitedLinkTrackingEnabled(BOOL visitedLinkTrackingEnable);
    virtual HRESULT STDMETHODCALLTYPE removeAllVisitedLinks();

    // WebHistory
    static WebHistory* sharedHistory();
    void visitedURL(const WebCore::KURL&, const WTF::String& title, const WTF::String& httpMethod, bool wasFailure, bool increaseVisitCount);
    void addVisitedLinksToPageGroup(WebCore::PageGroup&);

    COMPtr<IWebHistoryItem> itemForURLString(const WTF::String&) const;

    typedef int64_t DateKey;
    typedef HashMap<DateKey, RetainPtr<CFMutableArrayRef> > DateToEntriesMap;

private:

    enum NotificationType
    {
        kWebHistoryItemsAddedNotification = 0,
        kWebHistoryItemsRemovedNotification = 1,
        kWebHistoryAllItemsRemovedNotification = 2,
        kWebHistoryLoadedNotification = 3,
        kWebHistoryItemsDiscardedWhileLoadingNotification = 4,
        kWebHistorySavedNotification = 5
    };

    HRESULT postNotification(NotificationType notifyType, IPropertyBag* userInfo = 0);
    HRESULT removeItem(IWebHistoryItem* entry);
    HRESULT addItem(IWebHistoryItem* entry, bool discardDuplicate, bool* added);
    HRESULT removeItemForURLString(CFStringRef urlString);
    HRESULT addItemToDateCaches(IWebHistoryItem* entry);
    HRESULT removeItemFromDateCaches(IWebHistoryItem* entry);
    HRESULT insertItem(IWebHistoryItem* entry, DateKey);
    HRESULT ageLimitDate(CFAbsoluteTime* time);
    bool findKey(DateKey*, CFAbsoluteTime forDay);
    static CFAbsoluteTime timeToDate(CFAbsoluteTime time);
    BSTR getNotificationString(NotificationType notifyType);
    HRESULT itemForURLString(CFStringRef urlString, IWebHistoryItem** item) const;

    ULONG m_refCount;
    RetainPtr<CFMutableDictionaryRef> m_entriesByURL;
    DateToEntriesMap m_entriesByDate;
    OwnArrayPtr<DATE> m_orderedLastVisitedDays;
    COMPtr<WebPreferences> m_preferences;
};

#endif
