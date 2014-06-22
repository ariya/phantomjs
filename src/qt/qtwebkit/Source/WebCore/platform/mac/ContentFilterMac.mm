/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import "config.h"
#import "ContentFilter.h"

#if USE(CONTENT_FILTERING)

#import "ResourceResponse.h"
#import "WebCoreSystemInterface.h"

namespace WebCore {

PassRefPtr<ContentFilter> ContentFilter::create(const ResourceResponse& response)
{
    return adoptRef(new ContentFilter(response));
}

ContentFilter::ContentFilter(const ResourceResponse& response)
    : m_platformContentFilter(adoptNS(wkFilterCreateInstance(response.nsURLResponse())))
{
    ASSERT(m_platformContentFilter);
}

bool ContentFilter::isEnabled()
{
    return wkFilterIsManagedSession();
}

void ContentFilter::addData(const char* data, int length)
{
    ASSERT(needsMoreData());
    ASSERT(![m_replacementData.get() length]);
    m_replacementData = wkFilterAddData(m_platformContentFilter.get(), [NSData dataWithBytesNoCopy:(void*)data length:length freeWhenDone:NO]);
    ASSERT(needsMoreData() || [m_replacementData.get() length]);
}
    
void ContentFilter::finishedAddingData()
{
    ASSERT(needsMoreData());
    ASSERT(![m_replacementData.get() length]);
    m_replacementData = wkFilterDataComplete(m_platformContentFilter.get());
    ASSERT(!needsMoreData());
}

bool ContentFilter::needsMoreData() const
{
    return wkFilterIsBuffering(m_platformContentFilter.get());
}

bool ContentFilter::didBlockData() const
{
    return wkFilterWasBlocked(m_platformContentFilter.get());
}

const char* ContentFilter::getReplacementData(int& length) const
{
    ASSERT(!needsMoreData());
    length = [m_replacementData.get() length];
    return static_cast<const char*>([m_replacementData.get() bytes]);
}

} // namespace WebCore

#endif // USE(CONTENT_FILTERING)
