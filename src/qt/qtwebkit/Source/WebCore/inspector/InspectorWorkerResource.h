/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorWorkerResource_h
#define InspectorWorkerResource_h

#if ENABLE(WORKERS) && ENABLE(INSPECTOR)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorWorkerResource : public RefCounted<InspectorWorkerResource> {
public:
    static PassRefPtr<InspectorWorkerResource> create(intptr_t id, const String& url, bool isSharedWorker)
    {
        return adoptRef(new InspectorWorkerResource(id, url, isSharedWorker));
    }

    intptr_t id() const { return m_id; }
    const String& url() const { return m_url; }
    bool isSharedWorker() const { return m_isSharedWorker; }
private:
    InspectorWorkerResource(intptr_t id, const String& url, bool isSharedWorker)
        : m_id(id)
        , m_url(url)
        , m_isSharedWorker(isSharedWorker)
    {
    }

    intptr_t m_id;
    String m_url;
    bool m_isSharedWorker;
};

} // namespace WebCore

#endif // ENABLE(WORKERS) && ENABLE(INSPECTOR)

#endif // InspectorWorkerResource_h
