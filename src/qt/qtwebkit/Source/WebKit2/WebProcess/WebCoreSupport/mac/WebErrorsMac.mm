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
#import "WebErrors.h"

#import "WebError.h"
#import "WKError.h"
#import <WebCore/LocalizedStrings.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/ResourceResponse.h>

using namespace WebCore;
using namespace WebKit;

namespace WebKit {

static RetainPtr<NSError> createNSError(NSString* domain, int code, NSURL *URL)
{
    NSDictionary *userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
        URL, @"NSErrorFailingURLKey",
        [URL absoluteString], @"NSErrorFailingURLStringKey",
        nil];

    return adoptNS([[NSError alloc] initWithDomain:domain code:code userInfo:userInfo]);
}

// Use NSError's if available.

ResourceError cancelledError(const ResourceRequest& request)
{
    return ResourceError(createNSError(NSURLErrorDomain, NSURLErrorCancelled, request.url()).get());
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(createNSError(NSURLErrorDomain, NSURLErrorFileDoesNotExist, response.url()).get());
}


// Otherwise, fallback to our own errors.

ResourceError blockedError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotUseRestrictedPort, request.url(), WEB_UI_STRING("Not allowed to use restricted network port", "WebKitErrorCannotUseRestrictedPort description"));
}

ResourceError cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowURL, request.url(), WEB_UI_STRING("The URL can’t be shown", "WebKitErrorCannotShowURL description"));
}

ResourceError interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeFrameLoadInterruptedByPolicyChange, request.url(), WEB_UI_STRING("Frame load interrupted", "WebKitErrorFrameLoadInterruptedByPolicyChange description"));
}

ResourceError cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowMIMEType, response.url(), WEB_UI_STRING("Content with specified MIME type can’t be shown", "WebKitErrorCannotShowMIMEType description"));
}

ResourceError pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodePlugInWillHandleLoad, response.url(), WEB_UI_STRING("Plug-in handled load", "WebKitErrorPlugInWillHandleLoad description"));
}

ResourceError internalError(const KURL& url)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorInternal, url, WEB_UI_STRING("WebKit encountered an internal error", "WebKitErrorInternal description"));
}

} // namespace WebKit
