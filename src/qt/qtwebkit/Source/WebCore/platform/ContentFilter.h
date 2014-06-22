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

#ifndef ContentFilter_h
#define ContentFilter_h

#if USE(CONTENT_FILTERING)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC)
OBJC_CLASS WebFilterEvaluator;
#endif

namespace WebCore {

class ResourceResponse;

class ContentFilter : public RefCounted<ContentFilter> {
public:
    static PassRefPtr<ContentFilter> create(const ResourceResponse&);
    static bool isEnabled();
    
    void addData(const char* data, int length);
    void finishedAddingData();
    bool needsMoreData() const;
    bool didBlockData() const;
    const char* getReplacementData(int& length) const;
    
private:
    explicit ContentFilter(const ResourceResponse&);
    
#if PLATFORM(MAC)
    RetainPtr<WebFilterEvaluator> m_platformContentFilter;
    RetainPtr<NSData> m_replacementData;
#endif
};

} // namespace WebCore

#endif // USE(CONTENT_FILTERING)

#endif // ContentFilter_h
