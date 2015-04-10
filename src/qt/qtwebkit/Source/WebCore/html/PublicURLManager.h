/*
 * Copyright (C) 2012 Motorola Mobility Inc.
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

#ifndef PublicURLManager_h
#define PublicURLManager_h

#if ENABLE(BLOB)
#include "ScriptExecutionContext.h"
#include "ThreadableBlobRegistry.h"
#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>

#if ENABLE(MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamRegistry.h"
#endif

#if ENABLE(MEDIA_SOURCE)
#include "MediaSource.h"
#include "MediaSourceRegistry.h"
#endif

namespace WebCore {

class ScriptExecutionContext;

class PublicURLManager {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<PublicURLManager> create() { return adoptPtr(new PublicURLManager); }
    void contextDestroyed()
    {
        HashSet<String>::iterator blobURLsEnd = m_blobURLs.end();
        for (HashSet<String>::iterator iter = m_blobURLs.begin(); iter != blobURLsEnd; ++iter)
            ThreadableBlobRegistry::unregisterBlobURL(KURL(ParsedURLString, *iter));

#if ENABLE(MEDIA_STREAM)
        HashSet<String>::iterator streamURLsEnd = m_streamURLs.end();
        for (HashSet<String>::iterator iter = m_streamURLs.begin(); iter != streamURLsEnd; ++iter)
            MediaStreamRegistry::registry().unregisterMediaStreamURL(KURL(ParsedURLString, *iter));
#endif
#if ENABLE(MEDIA_SOURCE)
        HashSet<String>::iterator sourceURLsEnd = m_sourceURLs.end();
        for (HashSet<String>::iterator iter = m_sourceURLs.begin(); iter != sourceURLsEnd; ++iter)
            MediaSourceRegistry::registry().unregisterMediaSourceURL(KURL(ParsedURLString, *iter));
#endif
    }

    HashSet<String>& blobURLs() { return m_blobURLs; }
#if ENABLE(MEDIA_STREAM)
    HashSet<String>& streamURLs() { return m_streamURLs; }
#endif
#if ENABLE(MEDIA_SOURCE)
    HashSet<String>& sourceURLs() { return m_sourceURLs; }
#endif

private:
    HashSet<String> m_blobURLs;
#if ENABLE(MEDIA_STREAM)
    HashSet<String> m_streamURLs;
#endif
#if ENABLE(MEDIA_SOURCE)
    HashSet<String> m_sourceURLs;
#endif
};

} // namespace WebCore

#endif // BLOB
#endif // PUBLICURLMANAGER_h
