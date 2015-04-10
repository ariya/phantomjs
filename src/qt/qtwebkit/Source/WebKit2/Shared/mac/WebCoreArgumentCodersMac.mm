/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebCoreArgumentCoders.h"

#import "ArgumentCodersCF.h"
#import "PlatformCertificateInfo.h"
#import "WebKitSystemInterface.h"
#import <WebCore/KeyboardEvent.h>
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceRequest.h>

using namespace WebCore;
using namespace WebKit;

namespace CoreIPC {

void ArgumentCoder<ResourceRequest>::encodePlatformData(ArgumentEncoder& encoder, const ResourceRequest& resourceRequest)
{
    RetainPtr<NSURLRequest> requestToSerialize = resourceRequest.nsURLRequest(DoNotUpdateHTTPBody);

    bool requestIsPresent = requestToSerialize;
    encoder << requestIsPresent;

    if (!requestIsPresent)
        return;

    // We don't send HTTP body over IPC for better performance.
    // Also, it's not always possible to do, as streams can only be created in process that does networking.
    if ([requestToSerialize.get() HTTPBody] || [requestToSerialize.get() HTTPBodyStream]) {
        requestToSerialize = adoptNS([requestToSerialize.get() mutableCopy]);
        [(NSMutableURLRequest *)requestToSerialize.get() setHTTPBody:nil];
        [(NSMutableURLRequest *)requestToSerialize.get() setHTTPBodyStream:nil];
    }

    RetainPtr<CFDictionaryRef> dictionary = adoptCF(WKNSURLRequestCreateSerializableRepresentation(requestToSerialize.get(), CoreIPC::tokenNullTypeRef()));
    CoreIPC::encode(encoder, dictionary.get());

    // The fallback array is part of NSURLRequest, but it is not encoded by WKNSURLRequestCreateSerializableRepresentation.
    encoder << resourceRequest.responseContentDispositionEncodingFallbackArray();
}

bool ArgumentCoder<ResourceRequest>::decodePlatformData(ArgumentDecoder& decoder, ResourceRequest& resourceRequest)
{
    bool requestIsPresent;
    if (!decoder.decode(requestIsPresent))
        return false;

    if (!requestIsPresent) {
        resourceRequest = ResourceRequest();
        return true;
    }

    RetainPtr<CFDictionaryRef> dictionary;
    if (!CoreIPC::decode(decoder, dictionary))
        return false;

    NSURLRequest *nsURLRequest = WKNSURLRequestFromSerializableRepresentation(dictionary.get(), CoreIPC::tokenNullTypeRef());
    if (!nsURLRequest)
        return false;

    resourceRequest = ResourceRequest(nsURLRequest);
    
    Vector<String> responseContentDispositionEncodingFallbackArray;
    if (!decoder.decode(responseContentDispositionEncodingFallbackArray))
        return false;

    resourceRequest.setResponseContentDispositionEncodingFallbackArray(
        responseContentDispositionEncodingFallbackArray.size() > 0 ? responseContentDispositionEncodingFallbackArray[0] : String(),
        responseContentDispositionEncodingFallbackArray.size() > 1 ? responseContentDispositionEncodingFallbackArray[1] : String(),
        responseContentDispositionEncodingFallbackArray.size() > 2 ? responseContentDispositionEncodingFallbackArray[2] : String()
    );

    return true;
}

void ArgumentCoder<ResourceResponse>::encodePlatformData(ArgumentEncoder& encoder, const ResourceResponse& resourceResponse)
{
    bool responseIsPresent = resourceResponse.platformResponseIsUpToDate() && resourceResponse.nsURLResponse();
    encoder << responseIsPresent;

    if (!responseIsPresent)
        return;

    RetainPtr<CFDictionaryRef> dictionary = adoptCF(WKNSURLResponseCreateSerializableRepresentation(resourceResponse.nsURLResponse(), CoreIPC::tokenNullTypeRef()));
    CoreIPC::encode(encoder, dictionary.get());
}

bool ArgumentCoder<ResourceResponse>::decodePlatformData(ArgumentDecoder& decoder, ResourceResponse& resourceResponse)
{
    bool responseIsPresent;
    if (!decoder.decode(responseIsPresent))
        return false;

    if (!responseIsPresent) {
        resourceResponse = ResourceResponse();
        return true;
    }

    RetainPtr<CFDictionaryRef> dictionary;
    if (!CoreIPC::decode(decoder, dictionary))
        return false;

    NSURLResponse* nsURLResponse = WKNSURLResponseFromSerializableRepresentation(dictionary.get(), CoreIPC::tokenNullTypeRef());
    if (!nsURLResponse)
        return false;

    resourceResponse = ResourceResponse(nsURLResponse);
    return true;
}

static NSString* nsString(const String& string)
{
    return string.impl() ? [NSString stringWithCharacters:reinterpret_cast<const UniChar*>(string.characters()) length:string.length()] : @"";
}

void ArgumentCoder<ResourceError>::encodePlatformData(ArgumentEncoder& encoder, const ResourceError& resourceError)
{
    bool errorIsNull = resourceError.isNull();
    encoder << errorIsNull;

    if (errorIsNull)
        return;

    NSError *nsError = resourceError.nsError();

    String domain = [nsError domain];
    encoder << domain;

    int64_t code = [nsError code];
    encoder << code;

    HashMap<String, String> stringUserInfoMap;

    NSDictionary* userInfo = [nsError userInfo];
    for (NSString *key in userInfo) {
        id value = [userInfo objectForKey:key];
        if (![value isKindOfClass:[NSString class]])
            continue;

        stringUserInfoMap.set(key, (NSString *)value);
        continue;
    }
    encoder << stringUserInfoMap;

    id peerCertificateChain = [userInfo objectForKey:@"NSErrorPeerCertificateChainKey"];
    ASSERT(!peerCertificateChain || [peerCertificateChain isKindOfClass:[NSArray class]]);
    encoder << PlatformCertificateInfo((CFArrayRef)peerCertificateChain);
}

bool ArgumentCoder<ResourceError>::decodePlatformData(ArgumentDecoder& decoder, ResourceError& resourceError)
{
    bool errorIsNull;
    if (!decoder.decode(errorIsNull))
        return false;
    
    if (errorIsNull) {
        resourceError = ResourceError();
        return true;
    }

    String domain;
    if (!decoder.decode(domain))
        return false;

    int64_t code;
    if (!decoder.decode(code))
        return false;

    HashMap<String, String> stringUserInfoMap;
    if (!decoder.decode(stringUserInfoMap))
        return false;

    PlatformCertificateInfo certificate;
    if (!decoder.decode(certificate))
        return false;

    NSUInteger userInfoSize = stringUserInfoMap.size();
    if (certificate.certificateChain())
        userInfoSize++;

    NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithCapacity:userInfoSize];
    
    HashMap<String, String>::const_iterator it = stringUserInfoMap.begin();
    HashMap<String, String>::const_iterator end = stringUserInfoMap.end();
    for (; it != end; ++it)
        [userInfo setObject:nsString(it->value) forKey:nsString(it->key)];

    if (certificate.certificateChain())
        [userInfo setObject:(NSArray *)certificate.certificateChain() forKey:@"NSErrorPeerCertificateChainKey"];

    RetainPtr<NSError> nsError = adoptNS([[NSError alloc] initWithDomain:nsString(domain) code:code userInfo:userInfo]);

    resourceError = ResourceError(nsError.get());
    return true;
}

void ArgumentCoder<KeypressCommand>::encode(ArgumentEncoder& encoder, const KeypressCommand& keypressCommand)
{
    encoder << keypressCommand.commandName << keypressCommand.text;
}
    
bool ArgumentCoder<KeypressCommand>::decode(ArgumentDecoder& decoder, KeypressCommand& keypressCommand)
{
    if (!decoder.decode(keypressCommand.commandName))
        return false;

    if (!decoder.decode(keypressCommand.text))
        return false;

    return true;
}

} // namespace CoreIPC
