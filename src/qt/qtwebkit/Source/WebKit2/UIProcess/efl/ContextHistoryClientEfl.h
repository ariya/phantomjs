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

#ifndef ContextHistoryClientEfl_h
#define ContextHistoryClientEfl_h

#include "ewk_context.h"
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

class ContextHistoryClientEfl {
public:
    static PassOwnPtr<ContextHistoryClientEfl> create(WKContextRef context)
    {
        return adoptPtr(new ContextHistoryClientEfl(context));
    }

    ~ContextHistoryClientEfl();

    void setCallbacks(Ewk_History_Navigation_Cb, Ewk_History_Client_Redirection_Cb, Ewk_History_Server_Redirection_Cb, Ewk_History_Title_Update_Cb, Ewk_History_Populate_Visited_Links_Cb, void*);

private:
    explicit ContextHistoryClientEfl(WKContextRef);

    static void didNavigateWithNavigationData(WKContextRef, WKPageRef, WKNavigationDataRef, WKFrameRef, const void*);
    static void didPerformClientRedirect(WKContextRef, WKPageRef, WKURLRef sourceURL, WKURLRef, WKFrameRef, const void*);
    static void didPerformServerRedirect(WKContextRef, WKPageRef, WKURLRef sourceURL, WKURLRef, WKFrameRef, const void*);
    static void didUpdateHistoryTitle(WKContextRef, WKPageRef, WKStringRef, WKURLRef, WKFrameRef, const void*);
    static void populateVisitedLinks(WKContextRef, const void*);

    WKRetainPtr<WKContextRef> m_context;
    void* m_userData;
    Ewk_History_Navigation_Cb m_navigate;
    Ewk_History_Client_Redirection_Cb m_clientRedirect;
    Ewk_History_Server_Redirection_Cb m_serverRedirect;
    Ewk_History_Title_Update_Cb m_titleUpdated;
    Ewk_History_Populate_Visited_Links_Cb m_populateVisitedLinks;
};

} // namespace WebKit

#endif // ContextHistoryClientEfl_h
