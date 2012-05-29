/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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
 *
 */

#ifndef WorkerScriptLoader_h
#define WorkerScriptLoader_h

#if ENABLE(WORKERS)

#include "KURL.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "TextResourceDecoder.h"
#include "ThreadableLoader.h"
#include "ThreadableLoaderClient.h"

namespace WebCore {

    class ScriptExecutionContext;
    class WorkerScriptLoaderClient;

    class WorkerScriptLoader : public ThreadableLoaderClient {
    public:
        explicit WorkerScriptLoader(ResourceRequestBase::TargetType);

        void loadSynchronously(ScriptExecutionContext*, const KURL&, CrossOriginRequestPolicy);
        void loadAsynchronously(ScriptExecutionContext*, const KURL&, CrossOriginRequestPolicy, WorkerScriptLoaderClient*);

        void notifyError();

        const String& script() const { return m_script; }
        const KURL& url() const { return m_url; }
        const KURL& responseURL() const;
        bool failed() const { return m_failed; }
        unsigned long identifier() const { return m_identifier; }

        virtual void didReceiveResponse(const ResourceResponse&);
        virtual void didReceiveData(const char* data, int dataLength);
        virtual void didFinishLoading(unsigned long identifier, double);
        virtual void didFail(const ResourceError&);
        virtual void didFailRedirectCheck();
        virtual void didReceiveAuthenticationCancellation(const ResourceResponse&);

    private:
        PassOwnPtr<ResourceRequest> createResourceRequest();
        void notifyFinished();

        WorkerScriptLoaderClient* m_client;
        RefPtr<ThreadableLoader> m_threadableLoader;
        String m_responseEncoding;        
        RefPtr<TextResourceDecoder> m_decoder;
        String m_script;
        KURL m_url;
        KURL m_responseURL;
        bool m_failed;
        unsigned long m_identifier;
        ResourceRequestBase::TargetType m_targetType;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerScriptLoader_h
