/*
 * Copyright (C) 2005, 2011 Apple Inc.  All rights reserved.
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

#import <WebKit/WebKeyGenerator.h>

#import <WebKitSystemInterface.h>
#import <wtf/Assertions.h>

@implementation WebKeyGenerator

+ (WebKeyGenerator *)sharedGenerator
{
    static WebKeyGenerator *sharedGenerator = [[WebKeyGenerator alloc] init];
    return sharedGenerator;
}

static inline WebCertificateParseResult toWebCertificateParseResult(WKCertificateParseResult result)
{
    // FIXME: WebKeyGenerator is not used in WebKit, and this code should be moved to Safari.

    switch (result) {
    case WKCertificateParseResultSucceeded:
        return WebCertificateParseResultSucceeded;
    case WKCertificateParseResultFailed:
        return WebCertificateParseResultFailed;
    case WKCertificateParseResultPKCS7:
        return WebCertificateParseResultPKCS7;
    }

    ASSERT_NOT_REACHED();
    return WebCertificateParseResultFailed;
}

- (WebCertificateParseResult)addCertificatesToKeychainFromData:(NSData *)data
{
    return toWebCertificateParseResult(WKAddCertificatesToKeychainFromData([data bytes], [data length]));
}

@end
