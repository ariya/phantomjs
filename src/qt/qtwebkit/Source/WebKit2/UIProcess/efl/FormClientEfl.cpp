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
#include "FormClientEfl.h"

#include "EwkView.h"
#include "WKPage.h"
#include "ewk_form_submission_request_private.h"

using namespace EwkViewCallbacks;

namespace WebKit {

static inline FormClientEfl* toFormClientEfl(const void* clientInfo)
{
    return static_cast<FormClientEfl*>(const_cast<void*>(clientInfo));
}

void FormClientEfl::willSubmitForm(WKPageRef, WKFrameRef /*frame*/, WKFrameRef /*sourceFrame*/, WKDictionaryRef values, WKTypeRef /*userData*/, WKFormSubmissionListenerRef listener, const void* clientInfo)
{
    FormClientEfl* formClient = toFormClientEfl(clientInfo);

    RefPtr<EwkFormSubmissionRequest> request = EwkFormSubmissionRequest::create(values, listener);
    formClient->m_view->smartCallback<NewFormSubmissionRequest>().call(request.get());
}

FormClientEfl::FormClientEfl(EwkView* viewImpl)
    : m_view(viewImpl)
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKPageFormClient formClient;
    memset(&formClient, 0, sizeof(WKPageFormClient));
    formClient.version = kWKPageFormClientCurrentVersion;
    formClient.clientInfo = this;
    formClient.willSubmitForm = willSubmitForm;
    WKPageSetPageFormClient(pageRef, &formClient);
}

} // namespace WebKit
