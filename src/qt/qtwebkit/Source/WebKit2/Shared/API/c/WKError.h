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

#ifndef WKError_h
#define WKError_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kWKErrorCodeCannotShowMIMEType =                             100,
    kWKErrorCodeCannotShowURL =                                  101,
    kWKErrorCodeFrameLoadInterruptedByPolicyChange =             102,
    kWKErrorCodeCannotUseRestrictedPort =                        103,
    kWKErrorCodeCannotFindPlugIn =                               200,
    kWKErrorCodeCannotLoadPlugIn =                               201,
    kWKErrorCodeJavaUnavailable =                                202,
    kWKErrorCodePlugInCancelledConnection =                      203,
    kWKErrorCodePlugInWillHandleLoad =                           204,
    kWKErrorCodeInsecurePlugInVersion =                          205,
    kWKErrorInternal =                                           300,
};
typedef uint32_t WKErrorCode;

WK_EXPORT WKTypeID WKErrorGetTypeID();

WK_EXPORT WKStringRef WKErrorCopyWKErrorDomain();

WK_EXPORT WKStringRef WKErrorCopyDomain(WKErrorRef error);
WK_EXPORT int WKErrorGetErrorCode(WKErrorRef error);
WK_EXPORT WKURLRef WKErrorCopyFailingURL(WKErrorRef error);
WK_EXPORT WKStringRef WKErrorCopyLocalizedDescription(WKErrorRef error);

#ifdef __cplusplus
}
#endif

#endif // WKError_h
