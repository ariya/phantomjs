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

#ifndef WebFrameProxy_h
#define WebFrameProxy_h

#include "APIObject.h"
#include "ImmutableArray.h"
#include "GenericCallback.h"
#include "WebFrameListenerProxy.h"
#include <WebCore/FrameLoaderTypes.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
}

namespace WebKit {

class ImmutableArray;
class PlatformCertificateInfo;
class WebCertificateInfo;
class WebFormSubmissionListenerProxy;
class WebFramePolicyListenerProxy;
class WebPageProxy;

typedef GenericCallback<WKDataRef> DataCallback;

class WebFrameProxy : public TypedAPIObject<APIObject::TypeFrame> {
public:
    static PassRefPtr<WebFrameProxy> create(WebPageProxy* page, uint64_t frameID)
    {
        return adoptRef(new WebFrameProxy(page, frameID));
    }

    virtual ~WebFrameProxy();

    enum LoadState {
        LoadStateProvisional,
        LoadStateCommitted,
        LoadStateFinished
    };

    uint64_t frameID() const { return m_frameID; }
    WebPageProxy* page() const { return m_page; }

    void disconnect();

    bool isMainFrame() const;

    void setIsFrameSet(bool value) { m_isFrameSet = value; }
    bool isFrameSet() const { return m_isFrameSet; }

    LoadState loadState() const { return m_loadState; }
    
    void stopLoading() const;

    const String& url() const { return m_url; }
    const String& provisionalURL() const { return m_provisionalURL; }

    void setUnreachableURL(const String&);
    const String& unreachableURL() const { return m_unreachableURL; }

    const String& mimeType() const { return m_MIMEType; }

    const String& title() const { return m_title; }

    WebCertificateInfo* certificateInfo() const { return m_certificateInfo.get(); }

    bool canProvideSource() const;
    bool canShowMIMEType(const String& mimeType) const;

    bool isDisplayingStandaloneImageDocument() const;
    bool isDisplayingMarkupDocument() const;
    bool isDisplayingPDFDocument() const;

    void getWebArchive(PassRefPtr<DataCallback>);
    void getMainResourceData(PassRefPtr<DataCallback>);
    void getResourceData(WebURL*, PassRefPtr<DataCallback>);

    void didStartProvisionalLoad(const String& url);
    void didReceiveServerRedirectForProvisionalLoad(const String& url);
    void didFailProvisionalLoad();
    void didCommitLoad(const String& contentType, const PlatformCertificateInfo&);
    void didFinishLoad();
    void didFailLoad();
    void didSameDocumentNavigation(const String&); // eg. anchor navigation, session state change.
    void didChangeTitle(const String&);

    // Policy operations.
    void receivedPolicyDecision(WebCore::PolicyAction, uint64_t listenerID);
    WebFramePolicyListenerProxy* setUpPolicyListenerProxy(uint64_t listenerID);
    WebFormSubmissionListenerProxy* setUpFormSubmissionListenerProxy(uint64_t listenerID);

private:
    WebFrameProxy(WebPageProxy* page, uint64_t frameID);

    WebPageProxy* m_page;
    LoadState m_loadState;
    String m_url;
    String m_provisionalURL;
    String m_unreachableURL;
    String m_lastUnreachableURL;
    String m_MIMEType;
    String m_title;
    bool m_isFrameSet;
    RefPtr<WebCertificateInfo> m_certificateInfo;
    RefPtr<WebFrameListenerProxy> m_activeListener;
    uint64_t m_frameID;
};

} // namespace WebKit

#endif // WebFrameProxy_h
