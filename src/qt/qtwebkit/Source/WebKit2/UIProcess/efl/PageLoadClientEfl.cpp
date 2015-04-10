/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PageLoadClientEfl.h"

#include "EwkView.h"
#include "PageViewportController.h"
#include "WKAPICast.h"
#include "WKFrame.h"
#include "WKPage.h"
#include "ewk_auth_request_private.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_error_private.h"
#include "ewk_view.h"

using namespace EwkViewCallbacks;

namespace WebKit {

static inline PageLoadClientEfl* toPageLoadClientEfl(const void* clientInfo)
{
    return static_cast<PageLoadClientEfl*>(const_cast<void*>(clientInfo));
}

void PageLoadClientEfl::didReceiveTitleForFrame(WKPageRef, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    view->smartCallback<TitleChange>().call(toImpl(title)->string());
}

void PageLoadClientEfl::didChangeProgress(WKPageRef page, const void* clientInfo)
{
    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    double progress = WKPageGetEstimatedProgress(page);
    view->smartCallback<LoadProgress>().call(&progress);
}

void PageLoadClientEfl::didFinishLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    view->smartCallback<LoadFinished>().call();
}

void PageLoadClientEfl::didFailLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    OwnPtr<EwkError> ewkError = EwkError::create(error);
    view->smartCallback<LoadError>().call(ewkError.get());
    view->smartCallback<LoadFinished>().call();
}

void PageLoadClientEfl::didStartProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    view->informURLChange();
    view->smartCallback<ProvisionalLoadStarted>().call();
}

void PageLoadClientEfl::didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    view->informURLChange();
    view->smartCallback<ProvisionalLoadRedirect>().call();
}

void PageLoadClientEfl::didFailProvisionalLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    OwnPtr<EwkError> ewkError = EwkError::create(error);
    view->smartCallback<ProvisionalLoadFailed>().call(ewkError.get());
}

void PageLoadClientEfl::didCommitLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    if (WKPageUseFixedLayout(view->wkPage())) {
#if USE(ACCELERATED_COMPOSITING)
        view->pageViewportController()->didCommitLoad();
#endif
        return;
    }

    view->scheduleUpdateDisplay();
}

void PageLoadClientEfl::didChangeBackForwardList(WKPageRef, WKBackForwardListItemRef addedItem, WKArrayRef removedItems, const void* clientInfo)
{
    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    ASSERT(view);

    Ewk_Back_Forward_List* list = ewk_view_back_forward_list_get(view->evasObject());
    ASSERT(list);
    list->update(addedItem, removedItems);

    view->smartCallback<BackForwardListChange>().call();
}

void PageLoadClientEfl::didSameDocumentNavigationForFrame(WKPageRef, WKFrameRef frame, WKSameDocumentNavigationType, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkView* view = toPageLoadClientEfl(clientInfo)->view();
    view->informURLChange();
}

void PageLoadClientEfl::didReceiveAuthenticationChallengeInFrame(WKPageRef, WKFrameRef, WKAuthenticationChallengeRef authenticationChallenge, const void* clientInfo)
{
    EwkView* view = toPageLoadClientEfl(clientInfo)->view();

    RefPtr<EwkAuthRequest> authenticationRequest = EwkAuthRequest::create(authenticationChallenge);
    view->smartCallback<AuthenticationRequest>().call(authenticationRequest.get());
}

PageLoadClientEfl::PageLoadClientEfl(EwkView* view)
    : m_view(view)
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKPageLoaderClient loadClient;
    memset(&loadClient, 0, sizeof(WKPageLoaderClient));
    loadClient.version = kWKPageLoaderClientCurrentVersion;
    loadClient.clientInfo = this;
    loadClient.didReceiveTitleForFrame = didReceiveTitleForFrame;
    loadClient.didStartProgress = didChangeProgress;
    loadClient.didChangeProgress = didChangeProgress;
    loadClient.didFinishProgress = didChangeProgress;
    loadClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loadClient.didFailLoadWithErrorForFrame = didFailLoadWithErrorForFrame;
    loadClient.didStartProvisionalLoadForFrame = didStartProvisionalLoadForFrame;
    loadClient.didReceiveServerRedirectForProvisionalLoadForFrame = didReceiveServerRedirectForProvisionalLoadForFrame;
    loadClient.didFailProvisionalLoadWithErrorForFrame = didFailProvisionalLoadWithErrorForFrame;
    loadClient.didCommitLoadForFrame = didCommitLoadForFrame;
    loadClient.didChangeBackForwardList = didChangeBackForwardList;
    loadClient.didSameDocumentNavigationForFrame = didSameDocumentNavigationForFrame;
    loadClient.didReceiveAuthenticationChallengeInFrame = didReceiveAuthenticationChallengeInFrame;
    WKPageSetPageLoaderClient(pageRef, &loadClient);
}

} // namespace WebKit
