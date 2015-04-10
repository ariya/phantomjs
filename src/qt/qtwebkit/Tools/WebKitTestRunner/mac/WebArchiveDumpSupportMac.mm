/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebArchiveDumpSupport.h"

#import <CFNetwork/CFHTTPMessage.h>
#import <Foundation/Foundation.h>
#import <WebKit/WebHTMLRepresentation.h>
#import <wtf/RetainPtr.h>

extern "C" {

enum CFURLCacheStoragePolicy {
  kCFURLCacheStorageAllowed = 0,
  kCFURLCacheStorageAllowedInMemoryOnly = 1,
  kCFURLCacheStorageNotAllowed = 2
};
typedef enum CFURLCacheStoragePolicy CFURLCacheStoragePolicy;

extern const CFStringRef kCFHTTPVersion1_1;

CFURLResponseRef CFURLResponseCreate(CFAllocatorRef alloc, CFURLRef URL, CFStringRef mimeType, SInt64 expectedContentLength, CFStringRef textEncodingName, CFURLCacheStoragePolicy recommendedPolicy);
CFURLResponseRef CFURLResponseCreateWithHTTPResponse(CFAllocatorRef alloc, CFURLRef URL, CFHTTPMessageRef httpResponse, CFURLCacheStoragePolicy recommendedPolicy);
void CFURLResponseSetExpectedContentLength(CFURLResponseRef response, SInt64 length);
void CFURLResponseSetMIMEType(CFURLResponseRef response, CFStringRef mimeType);

}

CFURLResponseRef createCFURLResponseFromResponseData(CFDataRef responseData)
{
    // Decode NSURLResponse
    RetainPtr<NSKeyedUnarchiver> unarchiver = adoptNS([[NSKeyedUnarchiver alloc] initForReadingWithData:(NSData *)responseData]);
    NSURLResponse *response = [unarchiver.get() decodeObjectForKey:@"WebResourceResponse"]; // WebResourceResponseKey in WebResource.m
    [unarchiver.get() finishDecoding];

    if (![response isKindOfClass:[NSHTTPURLResponse class]])
        return CFURLResponseCreate(kCFAllocatorDefault, (CFURLRef)[response URL], (CFStringRef)[response MIMEType], [response expectedContentLength], (CFStringRef)[response textEncodingName], kCFURLCacheStorageAllowed);

    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;

    // NSURLResponse is not toll-free bridged to CFURLResponse.
    RetainPtr<CFHTTPMessageRef> httpMessage = adoptCF(CFHTTPMessageCreateResponse(kCFAllocatorDefault, [httpResponse statusCode], 0, kCFHTTPVersion1_1));

    NSDictionary *headerFields = [httpResponse allHeaderFields];
    for (NSString *headerField in [headerFields keyEnumerator])
        CFHTTPMessageSetHeaderFieldValue(httpMessage.get(), (CFStringRef)headerField, (CFStringRef)[headerFields objectForKey:headerField]);

    return CFURLResponseCreateWithHTTPResponse(kCFAllocatorDefault, (CFURLRef)[response URL], httpMessage.get(), kCFURLCacheStorageAllowed);
}

CFArrayRef supportedNonImageMIMETypes()
{
    return (CFArrayRef)[WebHTMLRepresentation supportedNonImageMIMETypes];
}
