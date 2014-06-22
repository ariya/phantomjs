/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#import "WebDragClient.h"

#if ENABLE(DRAG_SUPPORT)

#import "WebArchive.h"
#import "WebDOMOperations.h"
#import "WebFrame.h"
#import "WebFrameInternal.h"
#import "WebHTMLViewInternal.h"
#import "WebHTMLViewPrivate.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebNSPasteboardExtras.h"
#import "WebNSURLExtras.h"
#import "WebStringTruncator.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <WebCore/Clipboard.h>
#import <WebCore/DragData.h>
#import <WebCore/Editor.h>
#import <WebCore/EditorClient.h>
#import <WebCore/EventHandler.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/Image.h>
#import <WebCore/Page.h>
#import <WebCore/Pasteboard.h>

using namespace WebCore;

WebDragClient::WebDragClient(WebView* webView)
    : m_webView(webView) 
{
}

static WebHTMLView *getTopHTMLView(Frame* frame)
{
    ASSERT(frame);
    ASSERT(frame->page());
    return (WebHTMLView*)[[kit(frame->page()->mainFrame()) frameView] documentView];
}

WebCore::DragDestinationAction WebDragClient::actionMaskForDrag(WebCore::DragData* dragData)
{
    return (WebCore::DragDestinationAction)[[m_webView _UIDelegateForwarder] webView:m_webView dragDestinationActionMaskForDraggingInfo:dragData->platformData()];
}

void WebDragClient::willPerformDragDestinationAction(WebCore::DragDestinationAction action, WebCore::DragData* dragData)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView willPerformDragDestinationAction:(WebDragDestinationAction)action forDraggingInfo:dragData->platformData()];
}


WebCore::DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint& rootViewPoint)
{
    NSPoint viewPoint = [m_webView _convertPointFromRootView:rootViewPoint];
    return (DragSourceAction)[[m_webView _UIDelegateForwarder] webView:m_webView dragSourceActionMaskForPoint:viewPoint];
}

void WebDragClient::willPerformDragSourceAction(WebCore::DragSourceAction action, const WebCore::IntPoint& mouseDownPoint, WebCore::Clipboard* clipboard)
{
    ASSERT(clipboard);
    [[m_webView _UIDelegateForwarder] webView:m_webView willPerformDragSourceAction:(WebDragSourceAction)action fromPoint:mouseDownPoint withPasteboard:[NSPasteboard pasteboardWithName:clipboard->pasteboard().name()]];
}

void WebDragClient::startDrag(DragImageRef dragImage, const IntPoint& at, const IntPoint& eventPos, Clipboard* clipboard, Frame* frame, bool linkDrag)
{
    if (!frame)
        return;
    ASSERT(clipboard);
    RetainPtr<WebHTMLView> htmlView = (WebHTMLView*)[[kit(frame) frameView] documentView];
    if (![htmlView.get() isKindOfClass:[WebHTMLView class]])
        return;
    
    NSEvent *event = linkDrag ? frame->eventHandler()->currentNSEvent() : [htmlView.get() _mouseDownEvent];
    WebHTMLView* topHTMLView = getTopHTMLView(frame);
    RetainPtr<WebHTMLView> topViewProtector = topHTMLView;
    
    [topHTMLView _stopAutoscrollTimer];
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:clipboard->pasteboard().name()];

    NSImage *dragNSImage = dragImage.get();
    WebHTMLView *sourceHTMLView = htmlView.get();

    id delegate = [m_webView UIDelegate];
    SEL selector = @selector(webView:dragImage:at:offset:event:pasteboard:source:slideBack:forView:);
    if ([delegate respondsToSelector:selector]) {
        @try {
            [delegate webView:m_webView dragImage:dragNSImage at:at offset:NSZeroSize event:event pasteboard:pasteboard source:sourceHTMLView slideBack:YES forView:topHTMLView];
        } @catch (id exception) {
            ReportDiscardedDelegateException(selector, exception);
        }
    } else
        [topHTMLView dragImage:dragNSImage at:at offset:NSZeroSize event:event pasteboard:pasteboard source:sourceHTMLView slideBack:YES];
}

void WebDragClient::declareAndWriteDragImage(const String& pasteboardName, DOMElement* element, NSURL* URL, NSString* title, WebCore::Frame* frame) 
{
    ASSERT(pasteboardName);
    ASSERT(element);
    WebHTMLView *source = getTopHTMLView(frame);      
    WebArchive *archive = [element webArchive];
    
    [[NSPasteboard pasteboardWithName:pasteboardName] _web_declareAndWriteDragImageForElement:element URL:URL title:title archive:archive source:source];
}

void WebDragClient::dragControllerDestroyed() 
{
    delete this;
}

#endif // ENABLE(DRAG_SUPPORT)
