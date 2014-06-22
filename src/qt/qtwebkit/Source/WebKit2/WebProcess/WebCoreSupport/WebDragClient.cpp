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

#include "config.h"
#include "WebDragClient.h"

#if ENABLE(DRAG_SUPPORT)

#include "WebPage.h"

using namespace WebCore;

namespace WebKit {

void WebDragClient::willPerformDragDestinationAction(DragDestinationAction action, DragData*)
{
    if (action == DragDestinationActionLoad)
        m_page->willPerformLoadDragDestinationAction();
    else
        m_page->mayPerformUploadDragDestinationAction(); // Upload can happen from a drop event handler, so we should prepare early.
}

void WebDragClient::willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*)
{
}

DragDestinationAction WebDragClient::actionMaskForDrag(DragData*)
{
    return DragDestinationActionAny;
}

DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint& /*windowPoint*/)
{
    return DragSourceActionAny;
}

#if !PLATFORM(MAC) && !PLATFORM(QT) && !PLATFORM(GTK)
void WebDragClient::startDrag(DragImageRef, const IntPoint&, const IntPoint&, Clipboard*, Frame*, bool)
{
}
#endif

void WebDragClient::dragControllerDestroyed()
{
    delete this;
}

} // namespace WebKit

#endif // ENABLE(DRAG_SUPPORT)
