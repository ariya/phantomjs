/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "APIClientTraits.h"

#include "WKBundle.h"
#include "WKBundlePage.h"

namespace WebKit {

const size_t APIClientTraits<WKBundleClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundleClient, didReceiveMessageToPage),
    sizeof(WKBundleClient)
};

const size_t APIClientTraits<WKBundlePageLoaderClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageLoaderClient, didLayoutForFrame),
    offsetof(WKBundlePageLoaderClient, didFinishProgress),
    offsetof(WKBundlePageLoaderClient, didReceiveIntentForFrame_unavailable),
    offsetof(WKBundlePageLoaderClient, registerIntentServiceForFrame_unavailable),
    offsetof(WKBundlePageLoaderClient, didLayout),
    offsetof(WKBundlePageLoaderClient, featuresUsedInPage),
    sizeof(WKBundlePageLoaderClient)
};

const size_t APIClientTraits<WKBundlePageResourceLoadClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageResourceLoadClient, shouldCacheResponse),
    sizeof(WKBundlePageResourceLoadClient)
};

const size_t APIClientTraits<WKBundlePageFullScreenClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageFullScreenClient, beganEnterFullScreen),
    sizeof(WKBundlePageFullScreenClient)
};
    
const size_t APIClientTraits<WKPageContextMenuClient>::interfaceSizesByVersion[] = {
    offsetof(WKPageContextMenuClient, contextMenuDismissed),
    offsetof(WKPageContextMenuClient, getContextMenuFromProposedMenu),
    offsetof(WKPageContextMenuClient, showContextMenu),
    sizeof(WKPageContextMenuClient)
};

const size_t APIClientTraits<WKPageLoaderClient>::interfaceSizesByVersion[] = {
    offsetof(WKPageLoaderClient, didDetectXSSForFrame),
    offsetof(WKPageLoaderClient, didReceiveIntentForFrame_unavailable),
    offsetof(WKPageLoaderClient, pluginLoadPolicy),
    sizeof(WKPageLoaderClient)
};

const size_t APIClientTraits<WKPageUIClient>::interfaceSizesByVersion[] = {
    offsetof(WKPageUIClient, createNewPage),
    offsetof(WKPageUIClient, showColorPicker),
    sizeof(WKPageUIClient)
};
    
const size_t APIClientTraits<WKBundlePageFormClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageFormClient, willSendSubmitEvent),
    offsetof(WKBundlePageFormClient, didFocusTextField),
    sizeof(WKBundlePageFormClient)
};

const size_t APIClientTraits<WKBundlePageEditorClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageEditorClient, willWriteToPasteboard),
    sizeof(WKBundlePageEditorClient)
};

const size_t APIClientTraits<WKBundlePageUIClient>::interfaceSizesByVersion[] = {
    offsetof(WKBundlePageUIClient, didReachApplicationCacheOriginQuota),
    sizeof(WKBundlePageUIClient)
};

const size_t APIClientTraits<WKContextInjectedBundleClient>::interfaceSizesByVersion[] = {
    offsetof(WKContextInjectedBundleClient, getInjectedBundleInitializationUserData),
    sizeof(WKContextInjectedBundleClient)
};

const size_t APIClientTraits<WKIconDatabaseClient>::interfaceSizesByVersion[] = {
    offsetof(WKIconDatabaseClient, iconDataReadyForPageURL),
    sizeof(WKIconDatabaseClient)
};

} // namespace WebKit
