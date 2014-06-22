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

#ifndef WebKit2_C_h
#define WebKit2_C_h

#include <WebKit2/WKBase.h>
#include <WebKit2/WKType.h>

#include <WebKit2/WKArray.h>
#include <WebKit2/WKBackForwardList.h>
#include <WebKit2/WKBackForwardListItem.h>
#include <WebKit2/WKConnectionRef.h>
#include <WebKit2/WKContext.h>
#include <WebKit2/WKData.h>
#include <WebKit2/WKDictionary.h>
#include <WebKit2/WKError.h>
#include <WebKit2/WKFormSubmissionListener.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKFramePolicyListener.h>
#include <WebKit2/WKGeolocationManager.h>
#include <WebKit2/WKGeolocationPermissionRequest.h>
#include <WebKit2/WKGeolocationPosition.h>
#include <WebKit2/WKGraphicsContext.h>
#include <WebKit2/WKHitTestResult.h>
#include <WebKit2/WKMutableArray.h>
#include <WebKit2/WKMutableDictionary.h>
#include <WebKit2/WKNavigationData.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKOpenPanelParameters.h>
#include <WebKit2/WKOpenPanelResultListener.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKPreferences.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKURLRequest.h>
#include <WebKit2/WKURLResponse.h>

#if defined(__OBJC__) && __OBJC__
#import <WebKit2/WKView.h>
#elif !((defined(__APPLE__) && __APPLE__) || defined(BUILDING_QT__))
#include <WebKit2/WKView.h>
#endif

#endif /* WebKit2_C_h */
