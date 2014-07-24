/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef PageViewportControllerClientEfl_h
#define PageViewportControllerClientEfl_h

#include "PageViewportControllerClient.h"
#include <WebCore/FloatPoint.h>
#include <wtf/PassOwnPtr.h>

class EwkView;

namespace WebKit {

class PageViewportControllerClientEfl : public PageViewportControllerClient {
public:
    static PassOwnPtr<PageViewportControllerClientEfl> create(EwkView* viewImpl)
    {
        return adoptPtr(new PageViewportControllerClientEfl(viewImpl));
    }
    virtual ~PageViewportControllerClientEfl() { }

    virtual void setViewportPosition(const WebCore::FloatPoint&) OVERRIDE;
    virtual void setPageScaleFactor(float) OVERRIDE;

    virtual void didChangeContentsSize(const WebCore::IntSize&) OVERRIDE;
    virtual void didChangeVisibleContents() OVERRIDE;
    virtual void didChangeViewportAttributes() OVERRIDE;

    virtual void setController(PageViewportController*) OVERRIDE;

private:
    explicit PageViewportControllerClientEfl(EwkView*);

    EwkView* m_view;
    WebCore::FloatPoint m_contentPosition;
    PageViewportController* m_controller;
};

} // namespace WebKit

#endif // PageViewportControllerClientEfl_h
