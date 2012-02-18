/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef SubresourceLoaderClient_h
#define SubresourceLoaderClient_h

namespace WebCore {

class AuthenticationChallenge;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SubresourceLoader;
    
class SubresourceLoaderClient {
public:
    virtual ~SubresourceLoaderClient() { } 

    // request may be modified
    virtual void willSendRequest(SubresourceLoader*, ResourceRequest&, const ResourceResponse& /*redirectResponse*/) { }
    virtual void didSendData(SubresourceLoader*, unsigned long long /*bytesSent*/, unsigned long long /*totalBytesToBeSent*/) { }

    virtual void didReceiveResponse(SubresourceLoader*, const ResourceResponse&) { }
    virtual void didReceiveData(SubresourceLoader*, const char*, int /*dataLength*/) { }
    virtual void didReceiveCachedMetadata(SubresourceLoader*, const char*, int /*dataLength*/) { }
    virtual void didFinishLoading(SubresourceLoader*, double /*finishTime*/) { }
    virtual void didFail(SubresourceLoader*, const ResourceError&) { }
    
    virtual bool getShouldUseCredentialStorage(SubresourceLoader*, bool& /*shouldUseCredentialStorage*/) { return false; }
    virtual void didReceiveAuthenticationChallenge(SubresourceLoader*, const AuthenticationChallenge&) { }
    virtual void receivedCancellation(SubresourceLoader*, const AuthenticationChallenge&) { }

};

} // namespace WebCore

#endif // SubresourceLoaderClient_h
