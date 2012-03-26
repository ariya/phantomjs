/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef PageURLRecord_h
#define PageURLRecord_h

#include "PlatformString.h"

#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class IconRecord;

class PageURLSnapshot {
public:
    PageURLSnapshot() { }
    
    PageURLSnapshot(const String& pageURL, const String& iconURL)
        : m_pageURL(pageURL)
        , m_iconURL(iconURL)
    { }

    const String& pageURL() const { return m_pageURL; }
    const String& iconURL() const { return m_iconURL; }

private:
    String m_pageURL;
    String m_iconURL;
};

class PageURLRecord {
    WTF_MAKE_NONCOPYABLE(PageURLRecord); WTF_MAKE_FAST_ALLOCATED;
public:
    PageURLRecord(const String& pageURL);
    ~PageURLRecord();

    inline String url() const { return m_pageURL; }
    
    void setIconRecord(PassRefPtr<IconRecord>);
    IconRecord* iconRecord() { return m_iconRecord.get(); }

    PageURLSnapshot snapshot(bool forDeletion = false) const;

    // Returns false if the page wasn't retained beforehand, true if the retain count was already 1 or higher
    inline bool retain() { return m_retainCount++; }

    // Returns true if the page is still retained after the call.  False if the retain count just dropped to 0
    inline bool release()
    {
        ASSERT(m_retainCount > 0);
        return --m_retainCount;
    }

    inline int retainCount() const { return m_retainCount; }
private:
    String m_pageURL;
    RefPtr<IconRecord> m_iconRecord;
    int m_retainCount;
};

}

#endif // PageURLRecord_h
