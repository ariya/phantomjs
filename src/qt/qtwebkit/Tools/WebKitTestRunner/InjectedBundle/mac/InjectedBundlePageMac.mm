/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#import "InjectedBundlePage.h"

#import "StringFunctions.h"
#import <WebKit2/WKBundleFrame.h>
#import <WebKit2/WKURLCF.h>
#import <WebKitSystemInterface.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/StringBuilder.h>
#import <wtf/text/WTFString.h>

namespace WTR {

using namespace WTF;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
static String testPathFromURL(WKURLRef url)
{
    RetainPtr<CFURLRef> cfURL = adoptCF(WKURLCopyCFURL(kCFAllocatorDefault, url));
    if (!cfURL)
        return String();

    RetainPtr<CFStringRef> schemeCFString = adoptCF(CFURLCopyScheme(cfURL.get()));
    RetainPtr<CFStringRef> pathCFString = adoptCF(CFURLCopyPath(cfURL.get()));

    String schemeString(schemeCFString.get());
    String pathString(pathCFString.get());
    
    if (equalIgnoringCase(schemeString, "file")) {
        String layoutTests("/LayoutTests/");
        size_t layoutTestsOffset = pathString.find(layoutTests);
        if (layoutTestsOffset == notFound)
            return String();

        return pathString.substring(layoutTestsOffset + layoutTests.length());
    }

    if (!equalIgnoringCase(schemeString, "http") && !equalIgnoringCase(schemeString, "https"))
        return String();

    RetainPtr<CFStringRef> hostCFString = adoptCF(CFURLCopyHostName(cfURL.get()));
    String hostString(hostCFString.get());
    if (hostString == "127.0.0.1"  && (CFURLGetPortNumber(cfURL.get()) == 8000 || CFURLGetPortNumber(cfURL.get()) == 8443))
        return pathString;

    return String();
}
#endif

void InjectedBundlePage::platformDidStartProvisionalLoadForFrame(WKBundleFrameRef frame)
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (!WKBundleFrameIsMainFrame(frame))
        return;

    WKRetainPtr<WKURLRef> mainFrameURL = adoptWK(WKBundleFrameCopyProvisionalURL(frame));
    
    String testPath = testPathFromURL(mainFrameURL.get());
    if (!testPath.isNull()) {
        StringBuilder builder;
        builder.appendLiteral("CRASHING TEST: ");
        builder.append(testPath);
        WKSetCrashReportApplicationSpecificInformation(builder.toString().createCFString().get());
    }
#endif
}

} // namespace WTR
