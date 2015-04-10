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

#include "config.h"
#include "WebDragClient.h"
#include "WebDropSource.h"
#include "WebKitGraphics.h"
#include "WebView.h"

#include <WebCore/Clipboard.h>
#include <WebCore/DragController.h>
#include <WebCore/DragData.h>
#include <WebCore/EventHandler.h>
#include <WebCore/Page.h>
#include <WebCore/Pasteboard.h>
#include <WebCore/PlatformMouseEvent.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <shlobj.h>

using namespace WebCore;

static DWORD draggingSourceOperationMaskToDragCursors(DragOperation op)
{
    DWORD result = DROPEFFECT_NONE;
    if (op == DragOperationEvery)
        return DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE; 
    if (op & DragOperationCopy)
        result |= DROPEFFECT_COPY; 
    if (op & DragOperationLink)
        result |= DROPEFFECT_LINK; 
    if (op & DragOperationMove)
        result |= DROPEFFECT_MOVE;
    if (op & DragOperationGeneric)
        result |= DROPEFFECT_MOVE;
    return result;
}

WebDragClient::WebDragClient(WebView* webView)
    : m_webView(webView) 
{
    ASSERT(webView);
}

DragDestinationAction WebDragClient::actionMaskForDrag(DragData* dragData)
{
    COMPtr<IWebUIDelegate> delegateRef = 0;
    //Default behaviour (eg. no delegate, or callback not implemented) is to allow
    //any action
    WebDragDestinationAction mask = WebDragDestinationActionAny;
    if (SUCCEEDED(m_webView->uiDelegate(&delegateRef)))
        delegateRef->dragDestinationActionMaskForDraggingInfo(m_webView, dragData->platformData(), &mask);

    return (DragDestinationAction)mask;
}

void WebDragClient::willPerformDragDestinationAction(DragDestinationAction action, DragData* dragData)
{
    //Default delegate for willPerformDragDestinationAction has no side effects
    //so we just call the delegate, and don't worry about whether it's implemented
    COMPtr<IWebUIDelegate> delegateRef = 0;
    if (SUCCEEDED(m_webView->uiDelegate(&delegateRef)))
        delegateRef->willPerformDragDestinationAction(m_webView, (WebDragDestinationAction)action, dragData->platformData());
}

DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint& windowPoint)
{
   COMPtr<IWebUIDelegate> delegateRef = 0;
   WebDragSourceAction action = WebDragSourceActionAny;
   POINT localpt = core(m_webView)->mainFrame()->view()->windowToContents(windowPoint);
   if (SUCCEEDED(m_webView->uiDelegate(&delegateRef)))
       delegateRef->dragSourceActionMaskForPoint(m_webView, &localpt, &action);
   return (DragSourceAction)action;
}

void WebDragClient::willPerformDragSourceAction(DragSourceAction action, const IntPoint& intPoint, Clipboard* clipboard)
{
    COMPtr<IWebUIDelegate> uiDelegate;
    if (!SUCCEEDED(m_webView->uiDelegate(&uiDelegate)))
        return;

    POINT point = intPoint;
    COMPtr<IDataObject> dataObject = clipboard->pasteboard().dataObject();

    COMPtr<IDataObject> newDataObject;
    HRESULT result = uiDelegate->willPerformDragSourceAction(m_webView, static_cast<WebDragSourceAction>(action), &point, dataObject.get(), &newDataObject);
    if (result == S_OK && newDataObject != dataObject)
        const_cast<Pasteboard&>(clipboard->pasteboard()).setExternalDataObject(newDataObject.get());
}

void WebDragClient::startDrag(DragImageRef image, const IntPoint& imageOrigin, const IntPoint& dragPoint, Clipboard* clipboard, Frame* frame, bool isLink)
{
    //FIXME: Allow UIDelegate to override behaviour <rdar://problem/5015953>

    //We liberally protect everything, to protect against a load occurring mid-drag
    RefPtr<Frame> frameProtector = frame;
    COMPtr<IDragSourceHelper> helper;
    COMPtr<IDataObject> dataObject;
    COMPtr<WebView> viewProtector = m_webView;
    COMPtr<IDropSource> source;
    if (FAILED(WebDropSource::createInstance(m_webView, &source)))
        return;

    dataObject = clipboard->pasteboard().dataObject();
    if (source && (image || dataObject)) {
        if (image) {
            if(SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER,
                IID_IDragSourceHelper,(LPVOID*)&helper))) {
                BITMAP b;
                GetObject(image, sizeof(BITMAP), &b);
                SHDRAGIMAGE sdi;
                sdi.sizeDragImage.cx = b.bmWidth;
                sdi.sizeDragImage.cy = b.bmHeight;
                sdi.crColorKey = 0xffffffff;
                sdi.hbmpDragImage = image;
                sdi.ptOffset.x = dragPoint.x() - imageOrigin.x();
                sdi.ptOffset.y = dragPoint.y() - imageOrigin.y();
                if (isLink)
                    sdi.ptOffset.y = b.bmHeight - sdi.ptOffset.y;

                helper->InitializeFromBitmap(&sdi, dataObject.get());
            }
        }

        DWORD okEffect = draggingSourceOperationMaskToDragCursors(m_webView->page()->dragController()->sourceDragOperation());
        DWORD effect = DROPEFFECT_NONE;
        COMPtr<IWebUIDelegate> ui;
        HRESULT hr = E_NOTIMPL;
        if (SUCCEEDED(m_webView->uiDelegate(&ui))) {
            COMPtr<IWebUIDelegatePrivate> uiPrivate;
            if (SUCCEEDED(ui->QueryInterface(IID_IWebUIDelegatePrivate, (void**)&uiPrivate)))
                hr = uiPrivate->doDragDrop(m_webView, dataObject.get(), source.get(), okEffect, &effect);
        }
        if (hr == E_NOTIMPL)
            hr = DoDragDrop(dataObject.get(), source.get(), okEffect, &effect);

        DragOperation operation = DragOperationNone;
        if (hr == DRAGDROP_S_DROP) {
            if (effect & DROPEFFECT_COPY)
                operation = DragOperationCopy;
            else if (effect & DROPEFFECT_LINK)
                operation = DragOperationLink;
            else if (effect & DROPEFFECT_MOVE)
                operation = DragOperationMove;
        }
        frame->eventHandler()->dragSourceEndedAt(generateMouseEvent(m_webView, false), operation);
    }
}

void WebDragClient::dragControllerDestroyed()
{
    delete this;
}
