/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebBackForwardList.h"

#include "Logging.h"
#include <wtf/RetainPtr.h>
#include <CoreFoundation/CoreFoundation.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateWebBackForwardItemID()
{
    // These IDs exist in the UIProcess for items created by the UIProcess.
    // The IDs generated here need to never collide with the IDs created in WebBackForwardListProxy in the WebProcess.
    // We accomplish this by starting from 2, and only ever using even ids.
    static uint64_t uniqueHistoryItemID = 0;
    uniqueHistoryItemID += 2;
    return uniqueHistoryItemID;
}

static CFIndex currentVersion = 1;
DEFINE_STATIC_GETTER(CFNumberRef, SessionHistoryCurrentVersion, (CFNumberCreate(0, kCFNumberCFIndexType, &currentVersion)));

DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryVersionKey, (CFSTR("SessionHistoryVersion")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryCurrentIndexKey, (CFSTR("SessionHistoryCurrentIndex")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryEntriesKey, (CFSTR("SessionHistoryEntries")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryEntryTitleKey, (CFSTR("SessionHistoryEntryTitle")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryEntryURLKey, (CFSTR("SessionHistoryEntryURL")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryEntryOriginalURLKey, (CFSTR("SessionHistoryEntryOriginalURL")));
DEFINE_STATIC_GETTER(CFStringRef, SessionHistoryEntryDataKey, (CFSTR("SessionHistoryEntryData")));

static bool extractBackForwardListEntriesFromArray(CFArrayRef, BackForwardListItemVector&);

static CFDictionaryRef createEmptySessionHistoryDictionary()
{
    static const void* keys[1] = { SessionHistoryVersionKey() };
    static const void* values[1] = { SessionHistoryCurrentVersion() };

    return CFDictionaryCreate(0, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

CFDictionaryRef WebBackForwardList::createCFDictionaryRepresentation(WebPageProxy::WebPageProxySessionStateFilterCallback filter, void* context) const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_hasCurrentIndex) {
        // We represent having no current index by writing out an empty dictionary (besides the version).
        return createEmptySessionHistoryDictionary();
    }

    RetainPtr<CFMutableArrayRef> entries = adoptCF(CFArrayCreateMutable(0, m_entries.size(), &kCFTypeArrayCallBacks));

    // We may need to update the current index to account for entries that are filtered by the callback.
    CFIndex currentIndex = m_currentIndex;
    bool hasCurrentIndex = true;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        // If we somehow ended up with a null entry then we should consider the data invalid and not save session history at all.
        ASSERT(m_entries[i]);
        if (!m_entries[i]) {
            LOG(SessionState, "WebBackForwardList contained a null entry at index %lu", i);
            return 0;
        }

        if (filter) {
            if (!filter(toAPI(m_page), WKPageGetSessionBackForwardListItemValueType(), toAPI(m_entries[i].get()), context)
                || !filter(toAPI(m_page), WKPageGetSessionHistoryURLValueType(), toURLRef(m_entries[i]->originalURL().impl()), context)) {
                if (i <= m_currentIndex)
                    currentIndex--;
                continue;
            }
        }
        
        RetainPtr<CFStringRef> url = m_entries[i]->url().createCFString();
        RetainPtr<CFStringRef> title = m_entries[i]->title().createCFString();
        RetainPtr<CFStringRef> originalURL = m_entries[i]->originalURL().createCFString();

        // FIXME: This uses the CoreIPC data encoding format, which means that whenever we change the CoreIPC encoding we need to bump the CurrentSessionStateDataVersion
        // constant in WebPageProxyCF.cpp. The CoreIPC data format is meant to be an implementation detail, and not something that should be written to disk.
        RetainPtr<CFDataRef> entryData = adoptCF(CFDataCreate(kCFAllocatorDefault, m_entries[i]->backForwardData().data(), m_entries[i]->backForwardData().size()));
        
        const void* keys[4] = { SessionHistoryEntryURLKey(), SessionHistoryEntryTitleKey(), SessionHistoryEntryOriginalURLKey(), SessionHistoryEntryDataKey() };
        const void* values[4] = { url.get(), title.get(), originalURL.get(), entryData.get() };

        RetainPtr<CFDictionaryRef> entryDictionary = adoptCF(CFDictionaryCreate(0, keys, values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        CFArrayAppendValue(entries.get(), entryDictionary.get());
    }
        
    ASSERT(currentIndex == -1 || (currentIndex > -1 && currentIndex < CFArrayGetCount(entries.get())));
    if (currentIndex < -1 || currentIndex >= CFArrayGetCount(entries.get())) {
        LOG(SessionState, "Filtering entries to be saved resulted in an inconsistent state that we cannot represent");
        return 0;
    }

    // If we have an index and all items before and including the current item were filtered then currentIndex will be -1.
    // In this case the new current index should point at the first item.
    // It's also possible that all items were filtered so we should represent not having a current index.
    if (currentIndex == -1) {
        if (CFArrayGetCount(entries.get()))
            currentIndex = 0;
        else
            hasCurrentIndex = false;
    }

    if (hasCurrentIndex) {
        RetainPtr<CFNumberRef> currentIndexNumber = adoptCF(CFNumberCreate(0, kCFNumberCFIndexType, &currentIndex));
        const void* keys[3] = { SessionHistoryVersionKey(), SessionHistoryCurrentIndexKey(), SessionHistoryEntriesKey() };
        const void* values[3] = { SessionHistoryCurrentVersion(), currentIndexNumber.get(), entries.get() };
 
        return CFDictionaryCreate(0, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }

    // We represent having no current index by writing out an empty dictionary (besides the version).
    return createEmptySessionHistoryDictionary();
}

bool WebBackForwardList::restoreFromCFDictionaryRepresentation(CFDictionaryRef dictionary)
{
    CFNumberRef cfVersion = (CFNumberRef)CFDictionaryGetValue(dictionary, SessionHistoryVersionKey());
    if (!cfVersion) {
        // v0 session history dictionaries did not contain versioning
        return restoreFromV0CFDictionaryRepresentation(dictionary);
    }
    
    if (CFGetTypeID(cfVersion) != CFNumberGetTypeID()) {
        LOG(SessionState, "WebBackForwardList dictionary representation contains a version that is not a number");
        return false;
    }

    CFIndex version;
    if (!CFNumberGetValue(cfVersion, kCFNumberCFIndexType, &version)) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a correctly typed current version");
        return false;
    }
    
    if (version == 1)
        return restoreFromV1CFDictionaryRepresentation(dictionary);
    
    LOG(SessionState, "WebBackForwardList dictionary representation has an invalid current version (%ld)", version);
    return false;
}

bool WebBackForwardList::restoreFromV0CFDictionaryRepresentation(CFDictionaryRef dictionary)
{
    CFNumberRef cfIndex = (CFNumberRef)CFDictionaryGetValue(dictionary, SessionHistoryCurrentIndexKey());
    if (!cfIndex || CFGetTypeID(cfIndex) != CFNumberGetTypeID()) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a valid current index");
        return false;
    }

    CFIndex currentCFIndex;
    if (!CFNumberGetValue(cfIndex, kCFNumberCFIndexType, &currentCFIndex)) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a correctly typed current index");
        return false;
    }
    
    if (currentCFIndex < -1) {
        LOG(SessionState, "WebBackForwardList dictionary representation contains an unexpected negative current index (%ld)", currentCFIndex);
        return false;
    }

    CFArrayRef cfEntries = (CFArrayRef)CFDictionaryGetValue(dictionary, SessionHistoryEntriesKey());
    if (!cfEntries || CFGetTypeID(cfEntries) != CFArrayGetTypeID()) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a valid list of entries");
        return false;
    }

    CFIndex size = CFArrayGetCount(cfEntries);
    if (size < 0 || currentCFIndex >= size) {
        LOG(SessionState, "WebBackForwardList dictionary representation contains an invalid current index (%ld) for the number of entries (%ld)", currentCFIndex, size);
        return false;
    }

    // Version 0 session history relied on currentIndex == -1 to represent the same thing as not having a current index.
    bool hasCurrentIndex = currentCFIndex != -1;

    if (!hasCurrentIndex && size) {
        LOG(SessionState, "WebBackForwardList dictionary representation says there is no current index, but there is a list of %ld entries", size);
        return false;
    }
    
    BackForwardListItemVector entries;
    if (!extractBackForwardListEntriesFromArray(cfEntries, entries)) {
        // extractBackForwardListEntriesFromArray has already logged the appropriate error message.
        return false;
    }

    ASSERT(entries.size() == static_cast<unsigned>(size));
    
    m_hasCurrentIndex = hasCurrentIndex;
    m_currentIndex = m_hasCurrentIndex ? static_cast<uint32_t>(currentCFIndex) : 0;
    m_entries = entries;

    return true;
}

bool WebBackForwardList::restoreFromV1CFDictionaryRepresentation(CFDictionaryRef dictionary)
{
    CFNumberRef cfIndex = (CFNumberRef)CFDictionaryGetValue(dictionary, SessionHistoryCurrentIndexKey());
    if (!cfIndex) {
        // No current index means the dictionary represents an empty session.
        m_hasCurrentIndex = false;
        m_currentIndex = 0;
        m_entries.clear();

        return true;
    }

    if (CFGetTypeID(cfIndex) != CFNumberGetTypeID()) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a valid current index");
        return false;
    }

    CFIndex currentCFIndex;
    if (!CFNumberGetValue(cfIndex, kCFNumberCFIndexType, &currentCFIndex)) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a correctly typed current index");
        return false;
    }
    
    if (currentCFIndex < 0) {
        LOG(SessionState, "WebBackForwardList dictionary representation contains an unexpected negative current index (%ld)", currentCFIndex);
        return false;
    }

    CFArrayRef cfEntries = (CFArrayRef)CFDictionaryGetValue(dictionary, SessionHistoryEntriesKey());
    if (!cfEntries || CFGetTypeID(cfEntries) != CFArrayGetTypeID()) {
        LOG(SessionState, "WebBackForwardList dictionary representation does not have a valid list of entries");
        return false;
    }

    CFIndex size = CFArrayGetCount(cfEntries);
    if (currentCFIndex >= size) {
        LOG(SessionState, "WebBackForwardList dictionary representation contains an invalid current index (%ld) for the number of entries (%ld)", currentCFIndex, size);
        return false;
    }
    
    BackForwardListItemVector entries;
    if (!extractBackForwardListEntriesFromArray(cfEntries, entries)) {
        // extractBackForwardListEntriesFromArray has already logged the appropriate error message.
        return false;
    }
    
    ASSERT(entries.size() == static_cast<unsigned>(size));
    
    m_hasCurrentIndex = true;
    m_currentIndex = static_cast<uint32_t>(currentCFIndex);
    m_entries = entries;

    return true;
}

static bool extractBackForwardListEntriesFromArray(CFArrayRef cfEntries, BackForwardListItemVector& entries)
{
    CFIndex size = CFArrayGetCount(cfEntries);

    entries.reserveCapacity(size);
    for (CFIndex i = 0; i < size; ++i) {
        CFDictionaryRef entryDictionary = (CFDictionaryRef)CFArrayGetValueAtIndex(cfEntries, i);
        if (!entryDictionary || CFGetTypeID(entryDictionary) != CFDictionaryGetTypeID()) {
            LOG(SessionState, "WebBackForwardList entry array does not have a valid entry at index %i", (int)i);
            return false;
        }
        
        CFStringRef entryURL = (CFStringRef)CFDictionaryGetValue(entryDictionary, SessionHistoryEntryURLKey());
        if (!entryURL || CFGetTypeID(entryURL) != CFStringGetTypeID()) {
            LOG(SessionState, "WebBackForwardList entry at index %i does not have a valid URL", (int)i);
            return false;
        }

        CFStringRef entryTitle = (CFStringRef)CFDictionaryGetValue(entryDictionary, SessionHistoryEntryTitleKey());
        if (!entryTitle || CFGetTypeID(entryTitle) != CFStringGetTypeID()) {
            LOG(SessionState, "WebBackForwardList entry at index %i does not have a valid title", (int)i);
            return false;
        }

        CFStringRef originalURL = (CFStringRef)CFDictionaryGetValue(entryDictionary, SessionHistoryEntryOriginalURLKey());
        if (!originalURL || CFGetTypeID(originalURL) != CFStringGetTypeID()) {
            LOG(SessionState, "WebBackForwardList entry at index %i does not have a valid original URL", (int)i);
            return false;
        }

        CFDataRef backForwardData = (CFDataRef)CFDictionaryGetValue(entryDictionary, SessionHistoryEntryDataKey());
        if (!backForwardData || CFGetTypeID(backForwardData) != CFDataGetTypeID()) {
            LOG(SessionState, "WebBackForwardList entry at index %i does not have back/forward data", (int)i);
            return false;
        }
        
        entries.append(WebBackForwardListItem::create(originalURL, entryURL, entryTitle, CFDataGetBytePtr(backForwardData), CFDataGetLength(backForwardData), generateWebBackForwardItemID()));
    }

    return true;
}

} // namespace WebKit
