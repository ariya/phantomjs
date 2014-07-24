/*
 * Copyright (C) 2011 Igalia S.L.
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

#include "ArgumentCodersGtk.h"
#include "ShareableBitmap.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/Clipboard.h>
#include <WebCore/DataObjectGtk.h>
#include <WebCore/DragData.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/Pasteboard.h>
#include <WebCore/PlatformContextCairo.h>

using namespace WebCore;

namespace WebKit {

static PassRefPtr<ShareableBitmap> convertCairoSurfaceToShareableBitmap(cairo_surface_t* surface)
{
    if (!surface)
        return 0;

    IntSize imageSize(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface));
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(imageSize, ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> graphicsContext = bitmap->createGraphicsContext();

    graphicsContext->platformContext()->drawSurfaceToContext(surface, IntRect(IntPoint(), imageSize), IntRect(IntPoint(), imageSize), graphicsContext.get());
    return bitmap.release();
}

void WebDragClient::startDrag(DragImageRef dragImage, const IntPoint& clientPosition, const IntPoint& globalPosition, Clipboard* clipboard, Frame*, bool)
{
    RefPtr<ShareableBitmap> bitmap = convertCairoSurfaceToShareableBitmap(dragImage);
    ShareableBitmap::Handle handle;

    // If we have a bitmap, but cannot create a handle to it, we fail early.
    if (bitmap && !bitmap->createHandle(handle))
        return;

    RefPtr<DataObjectGtk> dataObject = clipboard->pasteboard().dataObject();
    DragData dragData(dataObject.get(), clientPosition, globalPosition, clipboard->sourceOperation());
    m_page->send(Messages::WebPageProxy::StartDrag(dragData, handle));
}

}; // namespace WebKit.
