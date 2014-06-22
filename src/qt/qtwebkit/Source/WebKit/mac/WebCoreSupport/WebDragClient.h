/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if ENABLE(DRAG_SUPPORT)

#import <WebCore/DragClient.h>

@class WebView;

class WebDragClient : public WebCore::DragClient {
public:
    WebDragClient(WebView*);
    virtual void willPerformDragDestinationAction(WebCore::DragDestinationAction, WebCore::DragData*) OVERRIDE;
    virtual void willPerformDragSourceAction(WebCore::DragSourceAction, const WebCore::IntPoint&, WebCore::Clipboard*) OVERRIDE;
    virtual WebCore::DragDestinationAction actionMaskForDrag(WebCore::DragData*) OVERRIDE;
    virtual void dragControllerDestroyed() OVERRIDE;
    virtual WebCore::DragSourceAction dragSourceActionMaskForPoint(const WebCore::IntPoint& windowPoint) OVERRIDE;
    virtual void startDrag(WebCore::DragImageRef, const WebCore::IntPoint& dragPos, const WebCore::IntPoint& eventPos, WebCore::Clipboard*, WebCore::Frame*, bool linkDrag) OVERRIDE;
    virtual void declareAndWriteDragImage(const String& pasteboardName, DOMElement*, NSURL*, NSString*, WebCore::Frame*) OVERRIDE;
private:
    WebView* m_webView;
};

#endif // ENABLE(DRAG_SUPPORT)
