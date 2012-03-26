/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef ThreadableLoader_h
#define ThreadableLoader_h

#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class ResourceError;
    class ResourceRequest;
    class ResourceResponse;
    class ScriptExecutionContext;
    class ThreadableLoaderClient;

    enum StoredCredentials {
        AllowStoredCredentials,
        DoNotAllowStoredCredentials
    };
    
    enum CrossOriginRequestPolicy {
        DenyCrossOriginRequests,
        UseAccessControl,
        AllowCrossOriginRequests
    };
    
    struct ThreadableLoaderOptions {
        ThreadableLoaderOptions() : sendLoadCallbacks(false), sniffContent(false), allowCredentials(false), forcePreflight(false), crossOriginRequestPolicy(DenyCrossOriginRequests) { }
        bool sendLoadCallbacks;
        bool sniffContent;
        bool allowCredentials;  // Whether HTTP credentials and cookies are sent with the request.
        bool forcePreflight;  // If AccessControl is used, whether to force a preflight.
        CrossOriginRequestPolicy crossOriginRequestPolicy;
    };

    // Useful for doing loader operations from any thread (not threadsafe, 
    // just able to run on threads other than the main thread).
    class ThreadableLoader {
        WTF_MAKE_NONCOPYABLE(ThreadableLoader);
    public:
        static void loadResourceSynchronously(ScriptExecutionContext*, const ResourceRequest&, ThreadableLoaderClient&, const ThreadableLoaderOptions&);
        static PassRefPtr<ThreadableLoader> create(ScriptExecutionContext*, ThreadableLoaderClient*, const ResourceRequest&, const ThreadableLoaderOptions&);

        virtual void cancel() = 0;
        void ref() { refThreadableLoader(); }
        void deref() { derefThreadableLoader(); }

    protected:
        ThreadableLoader() { }
        virtual ~ThreadableLoader() { }
        virtual void refThreadableLoader() = 0;
        virtual void derefThreadableLoader() = 0;
    };

} // namespace WebCore

#endif // ThreadableLoader_h
