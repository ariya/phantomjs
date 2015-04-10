/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef WKPluginInformation_h
#define WKPluginInformation_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Plug-in module information keys */

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKPluginInformationBundleIdentifierKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKPluginInformationBundleVersionKey();

/* Value type: WKStringRef */
WKStringRef WKPluginInformationBundleShortVersionKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKPluginInformationPathKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKPluginInformationDisplayNameKey();

/* Value type: WKUInt64Ref */
WK_EXPORT WKStringRef WKPluginInformationDefaultLoadPolicyKey();

/* Value type: WKBooleanRef */
WK_EXPORT WKStringRef WKPluginInformationUpdatePastLastBlockedVersionIsKnownAvailableKey();

/* Value type: WKBooleanRef */
WK_EXPORT WKStringRef WKPluginInformationHasSandboxProfileKey();


/* Plug-in load specific information keys */

/* Value type: WKURLRef */
WK_EXPORT WKStringRef WKPluginInformationFrameURLKey();

/* Value type: WKStringRef */
WK_EXPORT WKStringRef WKPluginInformationMIMETypeKey();

/* Value type: WKURLRef */
WK_EXPORT WKStringRef WKPluginInformationPageURLKey();

/* Value type: WKURLRef */
WK_EXPORT WKStringRef WKPluginInformationPluginspageAttributeURLKey();

/* Value type: WKURLRef */
WK_EXPORT WKStringRef WKPluginInformationPluginURLKey();

/* Value type: WKBooleanRef */
WK_EXPORT WKStringRef WKPlugInInformationReplacementObscuredKey();

#ifdef __cplusplus
}
#endif

#endif /* WKPluginInformation_h */
