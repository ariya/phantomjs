/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

/*
    Version numbers are based on the 'current library version' specified in the WebKit build rules.
    All of these methods return or take version numbers with each part shifted to the left 2 bytes.
    For example the version 1.2.3 is returned as 0x00010203 and version 200.3.5 is returned as 0x00C80305
    A version of -1 is returned if the main executable did not link against WebKit.

    Please use the current WebKit version number, available in WebKit/mac/Configurations/Version.xcconfig,
    when adding a new version constant.
*/

#define WEBKIT_FIRST_VERSION_WITH_3_0_CONTEXT_MENU_TAGS 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITH_LOCAL_RESOURCE_SECURITY_RESTRICTION 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITHOUT_APERTURE_QUIRK 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITHOUT_QUICKBOOKS_QUIRK 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITH_MAIN_THREAD_EXCEPTIONS 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITHOUT_ADOBE_INSTALLER_QUIRK 0x020A0000 // 522.0.0
#define WEBKIT_FIRST_VERSION_WITH_INSPECT_ELEMENT_MENU_TAG 0x020A0C00 // 522.12.0
#define WEBKIT_FIRST_VERSION_WITH_CACHE_MODEL_API 0x020B0500 // 523.5.0
#define WEBKIT_FIRST_VERSION_WITHOUT_JAVASCRIPT_RETURN_QUIRK 0x020D0100 // 525.1.0
#define WEBKIT_FIRST_VERSION_WITH_IE_COMPATIBLE_KEYBOARD_EVENT_DISPATCH 0x020D0100 // 525.1.0
#define WEBKIT_FIRST_VERSION_WITH_LOADING_DURING_COMMON_RUNLOOP_MODES 0x020E0000 // 526.0.0
#define WEBKIT_FIRST_VERSION_WITH_MORE_STRICT_LOCAL_RESOURCE_SECURITY_RESTRICTION 0x02100200 // 528.2.0
#define WEBKIT_FIRST_VERSION_WITH_RELOAD_FROM_ORIGIN 0x02100700 // 528.7.0
#define WEBKIT_FIRST_VERSION_WITHOUT_WEBVIEW_INIT_THREAD_WORKAROUND 0x02100700 // 528.7.0
#define WEBKIT_FIRST_VERSION_WITH_ROUND_TWO_MAIN_THREAD_EXCEPTIONS 0x02120400 // 530.4.0
#define WEBKIT_FIRST_VERSION_WITHOUT_BUMPERCAR_BACK_FORWARD_QUIRK 0x02120700 // 530.7.0
#define WEBKIT_FIRST_VERSION_WITHOUT_CONTENT_SNIFFING_FOR_FILE_URLS 0x02120A00 // 530.10.0
#define WEBKIT_FIRST_VERSION_WITHOUT_LINK_ELEMENT_TEXT_CSS_QUIRK 0x02130200 // 531.2.0
#define WEBKIT_FIRST_VERSION_WITH_HTML5_PARSER 0x02160900 // 534.9.0
#define WEBKIT_FIRST_VERSION_WITH_GET_MATCHED_CSS_RULES_RESTRICTIONS 0x02160B00 // 534.11.0
#define WEBKIT_FIRST_VERSION_WITH_CORRECT_DID_FINISH_LOAD_ORDER 0x02170304 // 535.3.4
#define WEBKIT_FIRST_VERSION_WITH_CSS_ATTRIBUTE_SETTERS_IGNORING_PRIORITY 0x02170D00 // 535.13.0
#define WEBKIT_FIRST_VERSION_WITHOUT_LEGACY_BACKGROUNDSIZE_SHORTHAND_BEHAVIOR 0x02190100 // 537.1.0

#ifdef __cplusplus
extern "C" {
#endif

BOOL WebKitLinkedOnOrAfter(int version);
void setWebKitLinkTimeVersion(int);

#ifdef __cplusplus
}
#endif
