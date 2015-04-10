/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "Clipboard.h"
#include "DragData.h"
#include "GraphicsContext.h"
#include "Pasteboard.h"
#include "ShareableBitmap.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"

using namespace WebCore;

namespace WebKit {

static PassRefPtr<ShareableBitmap> convertQPixmapToShareableBitmap(QPixmap* pixmap)
{
    if (!pixmap)
        return 0;

    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(IntSize(pixmap->size()), ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> graphicsContext = bitmap->createGraphicsContext();

    graphicsContext->platformContext()->drawPixmap(0, 0, *pixmap);
    return bitmap.release();
}

void WebDragClient::startDrag(DragImageRef dragImage, const IntPoint& clientPosition, const IntPoint& globalPosition, Clipboard* clipboard, Frame*, bool)
{
    QMimeData* clipboardData = clipboard->pasteboard().clipboardData();
    DragOperation dragOperationMask = clipboard->sourceOperation();
    clipboard->pasteboard().invalidateWritableData();
    DragData dragData(clipboardData, clientPosition, globalPosition, dragOperationMask);

    RefPtr<ShareableBitmap> bitmap = convertQPixmapToShareableBitmap(dragImage);
    ShareableBitmap::Handle handle;
    if (bitmap && !bitmap->createHandle(handle))
        return;

    m_page->send(Messages::WebPageProxy::StartDrag(dragData, handle));
}

}
