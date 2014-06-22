/*
 * Copyright (C) 2012 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#ifndef CachedResourceRequest_h
#define CachedResourceRequest_h

#include "Element.h"
#include "ResourceLoadPriority.h"
#include "ResourceLoaderOptions.h"
#include "ResourceRequest.h"
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {
class Document;

class CachedResourceRequest {
public:
    enum DeferOption { NoDefer, DeferredByClient };

    explicit CachedResourceRequest(const ResourceRequest&, const String& charset = String(), ResourceLoadPriority = ResourceLoadPriorityUnresolved);
    CachedResourceRequest(const ResourceRequest&, const ResourceLoaderOptions&);
    CachedResourceRequest(const ResourceRequest&, ResourceLoadPriority);
    ~CachedResourceRequest();

    ResourceRequest& mutableResourceRequest() { return m_resourceRequest; }
    const ResourceRequest& resourceRequest() const { return m_resourceRequest; }
    const String& charset() const { return m_charset; }
    void setCharset(const String& charset) { m_charset = charset; }
    const ResourceLoaderOptions& options() const { return m_options; }
    void setOptions(const ResourceLoaderOptions& options) { m_options = options; }
    void setPriority(ResourceLoadPriority priority) { m_priority = priority; }
    ResourceLoadPriority priority() const { return m_priority; }
    bool forPreload() const { return m_forPreload; }
    void setForPreload(bool forPreload) { m_forPreload = forPreload; }
    DeferOption defer() const { return m_defer; }
    void setDefer(DeferOption defer) { m_defer = defer; }
    void setInitiator(PassRefPtr<Element>);
    void setInitiator(const AtomicString& name);
    const AtomicString& initiatorName() const;

private:
    ResourceRequest m_resourceRequest;
    String m_charset;
    ResourceLoaderOptions m_options;
    ResourceLoadPriority m_priority;
    bool m_forPreload;
    DeferOption m_defer;
    RefPtr<Element> m_initiatorElement;
    AtomicString m_initiatorName;
};

} // namespace WebCore

#endif
