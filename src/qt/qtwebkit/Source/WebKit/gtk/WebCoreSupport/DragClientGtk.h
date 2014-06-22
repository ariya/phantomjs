/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Holger Hans Peter Freyther
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DragClientGtk_h
#define DragClientGtk_h

#include "DragClient.h"
#include "DragIcon.h"
#include <wtf/gobject/GRefPtr.h>

typedef struct _WebKitWebView WebKitWebView;

namespace WebKit {

    class DragClient : public WebCore::DragClient {
    public:
        DragClient(WebKitWebView*);
        ~DragClient();

        virtual void willPerformDragDestinationAction(WebCore::DragDestinationAction, WebCore::DragData*);
        virtual void willPerformDragSourceAction(WebCore::DragSourceAction, const WebCore::IntPoint&, WebCore::Clipboard*);
        virtual WebCore::DragDestinationAction actionMaskForDrag(WebCore::DragData*);

        virtual WebCore::DragSourceAction dragSourceActionMaskForPoint(const WebCore::IntPoint& windowPoint);

        virtual void startDrag(WebCore::DragImageRef dragImage, const WebCore::IntPoint& dragImageOrigin, const WebCore::IntPoint& eventPos, WebCore::Clipboard*, WebCore::Frame*, bool linkDrag = false);

        virtual void dragControllerDestroyed();

    private:
        WebKitWebView* m_webView;
        WebCore::IntPoint m_startPos;
        WebCore::DragIcon m_dragIcon;
    };
}

#endif
