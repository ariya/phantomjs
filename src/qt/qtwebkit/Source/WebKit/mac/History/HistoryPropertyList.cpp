/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "HistoryPropertyList.h"

#include <WebCore/HistoryItem.h>
#include <wtf/StringExtras.h>

using namespace WebCore;

static const int currentFileVersion = 1;

HistoryPropertyListWriter::HistoryPropertyListWriter()
    : m_dailyVisitCountsKey("D")
    , m_displayTitleKey("displayTitle")
    , m_lastVisitWasFailureKey("lastVisitWasFailure")
    , m_lastVisitWasHTTPNonGetKey("lastVisitWasHTTPNonGet")
    , m_lastVisitedDateKey("lastVisitedDate")
    , m_redirectURLsKey("redirectURLs")
    , m_titleKey("title")
    , m_urlKey("")
    , m_visitCountKey("visitCount")
    , m_weeklyVisitCountsKey("W")
    , m_buffer(0)
{
}

UInt8* HistoryPropertyListWriter::buffer(size_t size)
{
    ASSERT(!m_buffer);
    m_buffer = static_cast<UInt8*>(CFAllocatorAllocate(0, size, 0));
    m_bufferSize = size;
    return m_buffer;
}

RetainPtr<CFDataRef> HistoryPropertyListWriter::releaseData()
{
    UInt8* buffer = m_buffer;
    if (!buffer)
        return 0;
    m_buffer = 0;
    RetainPtr<CFDataRef> data = adoptCF(CFDataCreateWithBytesNoCopy(0, buffer, m_bufferSize, 0));
    if (!data) {
        CFAllocatorDeallocate(0, buffer);
        return 0;
    }
    return data;
}

void HistoryPropertyListWriter::writeObjects(BinaryPropertyListObjectStream& stream)
{
    size_t outerDictionaryStart = stream.writeDictionaryStart();

    stream.writeString("WebHistoryFileVersion");
    stream.writeString("WebHistoryDates");

    stream.writeInteger(currentFileVersion);
    size_t outerDateArrayStart = stream.writeArrayStart();
    writeHistoryItems(stream);
    stream.writeArrayEnd(outerDateArrayStart);

    stream.writeDictionaryEnd(outerDictionaryStart);
}

void HistoryPropertyListWriter::writeHistoryItem(BinaryPropertyListObjectStream& stream, HistoryItem* item)
{
    size_t itemDictionaryStart = stream.writeDictionaryStart();

    const String& title = item->title();
    const String& displayTitle = item->alternateTitle();
    double lastVisitedDate = item->lastVisitedTime();
    int visitCount = item->visitCount();
    Vector<String>* redirectURLs = item->redirectURLs();
    const Vector<int>& dailyVisitCounts = item->dailyVisitCounts();
    const Vector<int>& weeklyVisitCounts = item->weeklyVisitCounts();

    // keys
    stream.writeString(m_urlKey);
    if (!title.isEmpty())
        stream.writeString(m_titleKey);
    if (!displayTitle.isEmpty())
        stream.writeString(m_displayTitleKey);
    if (lastVisitedDate)
        stream.writeString(m_lastVisitedDateKey);
    if (visitCount)
        stream.writeString(m_visitCountKey);
    if (item->lastVisitWasFailure())
        stream.writeString(m_lastVisitWasFailureKey);
    if (item->lastVisitWasHTTPNonGet())
        stream.writeString(m_lastVisitWasHTTPNonGetKey);
    if (redirectURLs)
        stream.writeString(m_redirectURLsKey);
    if (!dailyVisitCounts.isEmpty())
        stream.writeString(m_dailyVisitCountsKey);
    if (!weeklyVisitCounts.isEmpty())
        stream.writeString(m_weeklyVisitCountsKey);

    // values
    stream.writeUniqueString(item->urlString());
    if (!title.isEmpty())
        stream.writeString(title);
    if (!displayTitle.isEmpty())
        stream.writeString(displayTitle);
    if (lastVisitedDate) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1lf", lastVisitedDate);
        stream.writeUniqueString(buffer);
    }
    if (visitCount)
        stream.writeInteger(visitCount);
    if (item->lastVisitWasFailure())
        stream.writeBooleanTrue();
    if (item->lastVisitWasHTTPNonGet()) {
        ASSERT(item->urlString().startsWith("http:", false) || item->urlString().startsWith("https:", false));
        stream.writeBooleanTrue();
    }
    if (redirectURLs) {
        size_t redirectArrayStart = stream.writeArrayStart();
        size_t size = redirectURLs->size();
        ASSERT(size);
        for (size_t i = 0; i < size; ++i)
            stream.writeUniqueString(redirectURLs->at(i));
        stream.writeArrayEnd(redirectArrayStart);
    }
    if (size_t size = dailyVisitCounts.size())
        stream.writeIntegerArray(dailyVisitCounts.data(), size);
    if (size_t size = weeklyVisitCounts.size())
        stream.writeIntegerArray(weeklyVisitCounts.data(), size);

    stream.writeDictionaryEnd(itemDictionaryStart);
}

