/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "SecItemShim.h"

#if USE(SECURITY_FRAMEWORK)

#include "BlockingResponseMap.h"
#include "ChildProcess.h"
#include "SecItemRequestData.h"
#include "SecItemResponseData.h"
#include "SecItemShimLibrary.h"
#include "SecItemShimMessages.h"
#include "SecItemShimProxyMessages.h"
#include <Security/Security.h>
#include <dlfcn.h>

namespace WebKit {

static BlockingResponseMap<SecItemResponseData>& responseMap()
{
    AtomicallyInitializedStatic(BlockingResponseMap<SecItemResponseData>*, responseMap = new BlockingResponseMap<SecItemResponseData>);
    return *responseMap;
}

static ChildProcess* sharedProcess;

SecItemShim& SecItemShim::shared()
{
    static SecItemShim* shim;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        shim = adoptRef(new SecItemShim).leakRef();
    });

    return *shim;
}

SecItemShim::SecItemShim()
    : m_queue(WorkQueue::create("com.apple.WebKit.SecItemShim"))
{
}

static uint64_t generateSecItemRequestID()
{
    static int64_t uniqueSecItemRequestID;
    return atomicIncrement(&uniqueSecItemRequestID);
}

static PassOwnPtr<SecItemResponseData> sendSecItemRequest(SecItemRequestData::Type requestType, CFDictionaryRef query, CFDictionaryRef attributesToMatch = 0)
{
    uint64_t requestID = generateSecItemRequestID();
    if (!sharedProcess->parentProcessConnection()->send(Messages::SecItemShimProxy::SecItemRequest(requestID, SecItemRequestData(requestType, query, attributesToMatch)), 0))
        return nullptr;

    return responseMap().waitForResponse(requestID);
}

static OSStatus webSecItemCopyMatching(CFDictionaryRef query, CFTypeRef* result)
{
    OwnPtr<SecItemResponseData> response = sendSecItemRequest(SecItemRequestData::CopyMatching, query);
    if (!response)
        return errSecInteractionNotAllowed;

    *result = response->resultObject().leakRef();
    return response->resultCode();
}

static OSStatus webSecItemAdd(CFDictionaryRef query, CFTypeRef* result)
{
    OwnPtr<SecItemResponseData> response = sendSecItemRequest(SecItemRequestData::Add, query);
    if (!response)
        return errSecInteractionNotAllowed;

    if (result)
        *result = response->resultObject().leakRef();
    return response->resultCode();
}

static OSStatus webSecItemUpdate(CFDictionaryRef query, CFDictionaryRef attributesToUpdate)
{
    OwnPtr<SecItemResponseData> response = sendSecItemRequest(SecItemRequestData::Update, query, attributesToUpdate);
    if (!response)
        return errSecInteractionNotAllowed;
    
    return response->resultCode();
}

static OSStatus webSecItemDelete(CFDictionaryRef query)
{
    OwnPtr<SecItemResponseData> response = sendSecItemRequest(SecItemRequestData::Delete, query);
    if (!response)
        return errSecInteractionNotAllowed;
    
    return response->resultCode();
}

void SecItemShim::secItemResponse(uint64_t requestID, const SecItemResponseData& response)
{
    responseMap().didReceiveResponse(requestID, adoptPtr(new SecItemResponseData(response)));
}

void SecItemShim::initialize(ChildProcess* process)
{
    sharedProcess = process;

    const SecItemShimCallbacks callbacks = {
        webSecItemCopyMatching,
        webSecItemAdd,
        webSecItemUpdate,
        webSecItemDelete
    };
    
    SecItemShimInitializeFunc func = reinterpret_cast<SecItemShimInitializeFunc>(dlsym(RTLD_DEFAULT, "WebKitSecItemShimInitialize"));
    func(callbacks);
}

void SecItemShim::initializeConnection(CoreIPC::Connection* connection)
{
    connection->addWorkQueueMessageReceiver(Messages::SecItemShim::messageReceiverName(), m_queue.get(), this);
}

} // namespace WebKit

#endif // USE(SECURITY_FRAMEWORK)
