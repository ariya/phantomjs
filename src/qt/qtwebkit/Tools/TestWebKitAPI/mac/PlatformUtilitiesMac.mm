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

#include "config.h"
#include "PlatformUtilities.h"

#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKStringCF.h>
#include <WebKit2/WKURLCF.h>
#include <WebKit2/WKURLResponseNS.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RetainPtr.h>

namespace TestWebKitAPI {
namespace Util {

void run(bool* done)
{
    while (!*done)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
}

void sleep(double seconds)
{
    usleep(seconds * 1000000);
}

WKStringRef createInjectedBundlePath()
{
    NSString *nsString = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"InjectedBundleTestWebKitAPI.bundle"];
    return WKStringCreateWithCFString((CFStringRef)nsString);
}

WKURLRef createURLForResource(const char* resource, const char* extension)
{
    NSURL *nsURL = [[NSBundle mainBundle] URLForResource:[NSString stringWithUTF8String:resource] withExtension:[NSString stringWithUTF8String:extension] subdirectory:@"TestWebKitAPI.resources"];
    return WKURLCreateWithCFURL((CFURLRef)nsURL);
}

WKURLRef URLForNonExistentResource()
{
    NSURL *nsURL = [NSURL URLWithString:@"file:///does-not-exist.html"];
    return WKURLCreateWithCFURL((CFURLRef)nsURL);
}

WKRetainPtr<WKStringRef> MIMETypeForWKURLResponse(WKURLResponseRef wkResponse)
{
    RetainPtr<NSURLResponse> response = adoptNS(WKURLResponseCopyNSURLResponse(wkResponse));
    return adoptWK(WKStringCreateWithCFString((CFStringRef)[response.get() MIMEType]));
}

bool isKeyDown(WKNativeEventPtr event)
{
    return [event type] == NSKeyDown;
}

std::string toSTD(NSString *string)
{
    if (!string)
        return std::string();

    size_t bufferSize = [string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[bufferSize]);
    NSUInteger stringLength;
    [string getBytes:buffer.get() maxLength:bufferSize usedLength:&stringLength encoding:NSUTF8StringEncoding options:0 range:NSMakeRange(0, [string length]) remainingRange:0];
    return std::string(buffer.get(), stringLength);
}

} // namespace Util
} // namespace TestWebKitAPI
