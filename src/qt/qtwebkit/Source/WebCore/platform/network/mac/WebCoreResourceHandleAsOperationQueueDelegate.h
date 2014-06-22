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

#ifndef WebCoreResourceHandleAsOperationQueueDelegate_h
#define WebCoreResourceHandleAsOperationQueueDelegate_h

#if !USE(CFNETWORK)

#include <dispatch/dispatch.h>
#include <wtf/RetainPtr.h>

namespace WebCore {
class ResourceHandle;
}

@interface WebCoreResourceHandleAsOperationQueueDelegate : NSObject <NSURLConnectionDelegate> {
    WebCore::ResourceHandle* m_handle;

    // Synchronous delegates on operation queue wait until main thread sends an asynchronous response.
    dispatch_semaphore_t m_semaphore;
    RetainPtr<NSURLRequest> m_requestResult;
    RetainPtr<NSCachedURLResponse> m_cachedResponseResult;
    BOOL m_boolResult;
}

- (id)initWithHandle:(WebCore::ResourceHandle*)handle;
- (void)detachHandle;
- (void)continueWillSendRequest:(NSURLRequest *)newRequest;
- (void)continueDidReceiveResponse;
- (void)continueShouldUseCredentialStorage:(BOOL)useCredentialStorage;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
- (void)continueCanAuthenticateAgainstProtectionSpace:(BOOL)canAuthenticate;
#endif
- (void)continueWillCacheResponse:(NSCachedURLResponse *)response;
@end

#endif // !USE(CFNETWORK)
#endif // WebCoreResourceHandleAsOperationQueueDelegate_h
