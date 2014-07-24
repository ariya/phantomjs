/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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

#ifndef HTMLResourcePreloader_h
#define HTMLResourcePreloader_h

#include "CachedResource.h"
#include "CachedResourceRequest.h"

namespace WebCore {

class PreloadRequest {
public:
    static PassOwnPtr<PreloadRequest> create(const String& initiator, const String& resourceURL, const KURL& baseURL, CachedResource::Type resourceType)
    {
        return adoptPtr(new PreloadRequest(initiator, resourceURL, baseURL, resourceType));
    }

    bool isSafeToSendToAnotherThread() const;

    CachedResourceRequest resourceRequest(Document*);

    const String& charset() const { return m_charset; }
    void setCharset(const String& charset) { m_charset = charset.isolatedCopy(); }
    void setCrossOriginModeAllowsCookies(bool allowsCookies) { m_crossOriginModeAllowsCookies = allowsCookies; }
    CachedResource::Type resourceType() const { return m_resourceType; }

private:
    PreloadRequest(const String& initiator, const String& resourceURL, const KURL& baseURL, CachedResource::Type resourceType)
        : m_initiator(initiator)
        , m_resourceURL(resourceURL.isolatedCopy())
        , m_baseURL(baseURL.copy())
        , m_resourceType(resourceType)
        , m_crossOriginModeAllowsCookies(false)
    {
    }

    KURL completeURL(Document*);

    String m_initiator;
    String m_resourceURL;
    KURL m_baseURL;
    String m_charset;
    CachedResource::Type m_resourceType;
    bool m_crossOriginModeAllowsCookies;
};

typedef Vector<OwnPtr<PreloadRequest> > PreloadRequestStream;

class HTMLResourcePreloader {
    WTF_MAKE_NONCOPYABLE(HTMLResourcePreloader); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit HTMLResourcePreloader(Document* document)
        : m_document(document)
        , m_weakFactory(this)
    {
    }

    void takeAndPreload(PreloadRequestStream&);
    void preload(PassOwnPtr<PreloadRequest>);

    WeakPtr<HTMLResourcePreloader> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

private:
    Document* m_document;
    WeakPtrFactory<HTMLResourcePreloader> m_weakFactory;
};

}

#endif
