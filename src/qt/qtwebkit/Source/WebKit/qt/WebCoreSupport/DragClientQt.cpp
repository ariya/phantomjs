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
#include "DragClientQt.h"

#include "ChromeClient.h"
#include "Clipboard.h"
#include "DragController.h"
#include "EventHandler.h"
#include "Frame.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PlatformMouseEvent.h"

#include <QDrag>
#include <QMimeData>
#include <QWebPageClient.h>

namespace WebCore {

static inline Qt::DropActions dragOperationsToDropActions(unsigned op)
{
    Qt::DropActions result = Qt::IgnoreAction;
    if (op & DragOperationCopy)
        result = Qt::CopyAction;
    if (op & DragOperationMove)
        result |= Qt::MoveAction;
    if (op & DragOperationGeneric)
        result |= Qt::MoveAction;
    if (op & DragOperationLink)
        result |= Qt::LinkAction;
    return result;
}

static inline DragOperation dropActionToDragOperation(Qt::DropActions action)
{
    DragOperation result = DragOperationNone;
    if (action & Qt::CopyAction)
        result = DragOperationCopy;
    if (action & Qt::LinkAction)
        result = DragOperationLink;
    if (action & Qt::MoveAction)
        result = DragOperationMove;
    return result;
}

DragDestinationAction DragClientQt::actionMaskForDrag(DragData*)
{
    return DragDestinationActionAny;
}

void DragClientQt::willPerformDragDestinationAction(DragDestinationAction, DragData*)
{
}

void DragClientQt::dragControllerDestroyed()
{
    delete this;
}

DragSourceAction DragClientQt::dragSourceActionMaskForPoint(const IntPoint&)
{
    return DragSourceActionAny;
}

void DragClientQt::willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*)
{
}

void DragClientQt::startDrag(DragImageRef dragImage, const IntPoint& dragImageOrigin, const IntPoint& eventPos, Clipboard* clipboard, Frame* frame, bool)
{
#ifndef QT_NO_DRAGANDDROP
    QMimeData* clipboardData = clipboard->pasteboard().clipboardData();
    clipboard->pasteboard().invalidateWritableData();
    QObject* view = m_chromeClient->platformPageClient()->ownerWidget();
    if (view) {
        QDrag* drag = new QDrag(view);
        if (dragImage) {
            drag->setPixmap(*dragImage);
            drag->setHotSpot(IntPoint(eventPos - dragImageOrigin));
        } else if (clipboardData && clipboardData->hasImage())
            drag->setPixmap(qvariant_cast<QPixmap>(clipboardData->imageData()));
        DragOperation dragOperationMask = clipboard->sourceOperation();
        drag->setMimeData(clipboardData);
        Qt::DropAction actualDropAction = drag->exec(dragOperationsToDropActions(dragOperationMask));

        // Send dragEnd event
        PlatformMouseEvent me(m_chromeClient->screenToRootView(QCursor::pos()), QCursor::pos(), LeftButton, PlatformEvent::MouseMoved, 0, false, false, false, false, 0);
        frame->eventHandler()->dragSourceEndedAt(me, dropActionToDragOperation(actualDropAction));
    }
    frame->page()->dragController()->dragEnded();
#endif
}


} // namespace WebCore
