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

#ifndef WebLoaderClient_h
#define WebLoaderClient_h

#include "APIClient.h"
#include "PluginModuleInfo.h"
#include "SameDocumentNavigationType.h"
#include "WKPage.h"
#include <WebCore/LayoutMilestones.h>
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
class ResourceError;
}

namespace WebKit {

class APIObject;
class AuthenticationChallengeProxy;
class AuthenticationDecisionListener;
class ImmutableDictionary;
class WebBackForwardListItem;
class WebFrameProxy;
class WebPageProxy;
class WebProtectionSpace;

class WebLoaderClient : public APIClient<WKPageLoaderClient, kWKPageLoaderClientCurrentVersion> {
public:
    void didStartProvisionalLoadForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didReceiveServerRedirectForProvisionalLoadForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didFailProvisionalLoadWithErrorForFrame(WebPageProxy*, WebFrameProxy*, const WebCore::ResourceError&, APIObject*);
    void didCommitLoadForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didFinishDocumentLoadForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didFinishLoadForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didFailLoadWithErrorForFrame(WebPageProxy*, WebFrameProxy*, const WebCore::ResourceError&, APIObject*);
    void didSameDocumentNavigationForFrame(WebPageProxy*, WebFrameProxy*, SameDocumentNavigationType, APIObject*);
    void didReceiveTitleForFrame(WebPageProxy*, const String&, WebFrameProxy*, APIObject*);
    void didFirstLayoutForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didFirstVisuallyNonEmptyLayoutForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didRemoveFrameFromHierarchy(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didDisplayInsecureContentForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didRunInsecureContentForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);
    void didDetectXSSForFrame(WebPageProxy*, WebFrameProxy*, APIObject*);

    // FIXME: didNewFirstVisuallyNonEmptyLayout should be removed. We should consider removing didFirstVisuallyNonEmptyLayoutForFrame
    // as well. They are both being replaced by didLayout.
    void didNewFirstVisuallyNonEmptyLayout(WebPageProxy*, APIObject*);
    void didLayout(WebPageProxy*, WebCore::LayoutMilestones, APIObject*);
    
    bool canAuthenticateAgainstProtectionSpaceInFrame(WebPageProxy*, WebFrameProxy*, WebProtectionSpace*);
    void didReceiveAuthenticationChallengeInFrame(WebPageProxy*, WebFrameProxy*, AuthenticationChallengeProxy*);

    void didStartProgress(WebPageProxy*);
    void didChangeProgress(WebPageProxy*);
    void didFinishProgress(WebPageProxy*);

    // FIXME: These three functions should not be part of this client.
    void processDidBecomeUnresponsive(WebPageProxy*);
    void interactionOccurredWhileProcessUnresponsive(WebPageProxy*);
    void processDidBecomeResponsive(WebPageProxy*);
    void processDidCrash(WebPageProxy*);

    void didChangeBackForwardList(WebPageProxy*, WebBackForwardListItem* addedItem, Vector<RefPtr<APIObject> >* removedItems);
    bool shouldGoToBackForwardListItem(WebPageProxy*, WebBackForwardListItem*);
    void willGoToBackForwardListItem(WebPageProxy*, WebBackForwardListItem*, APIObject*);

    PluginModuleLoadPolicy pluginLoadPolicy(WebPageProxy*, PluginModuleLoadPolicy currentPluginLoadPolicy, ImmutableDictionary*, String& unavailabilityDescriptionOutParameter);
    void didFailToInitializePlugin(WebPageProxy*, ImmutableDictionary*);
    void didBlockInsecurePluginVersion(WebPageProxy*, ImmutableDictionary*);
};

} // namespace WebKit

#endif // WebLoaderClient_h
