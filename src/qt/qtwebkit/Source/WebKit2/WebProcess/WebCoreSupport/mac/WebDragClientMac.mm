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

#import "config.h"
#import "WebDragClient.h"

#if ENABLE(DRAG_SUPPORT)

#import "PasteboardTypes.h"
#import "ShareableBitmap.h"
#import "WebCoreArgumentCoders.h"
#import "WebPage.h"
#import "WebPageProxyMessages.h"
#import <WebCore/CachedImage.h>
#import <WebCore/DOMElementInternal.h>
#import <WebCore/DOMPrivate.h>
#import <WebCore/DragController.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/Page.h>
#import <WebCore/RenderImage.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/StringTruncator.h>
#import <WebKit/WebArchive.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebNSFileManagerExtras.h>
#import <WebKit/WebNSPasteboardExtras.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKitSystemInterface.h>
#import <wtf/StdLibExtras.h>

using namespace WebCore;
using namespace WebKit;

namespace WebKit {

static PassRefPtr<ShareableBitmap> convertImageToBitmap(NSImage *image, const IntSize& size)
{
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(size, ShareableBitmap::SupportsAlpha);
    OwnPtr<GraphicsContext> graphicsContext = bitmap->createGraphicsContext();

    RetainPtr<NSGraphicsContext> savedContext = [NSGraphicsContext currentContext];

    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithGraphicsPort:graphicsContext->platformContext() flipped:YES]];
    [image drawInRect:NSMakeRect(0, 0, bitmap->size().width(), bitmap->size().height()) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1 respectFlipped:YES hints:nil];

    [NSGraphicsContext setCurrentContext:savedContext.get()];

    return bitmap.release();
}

void WebDragClient::startDrag(RetainPtr<NSImage> image, const IntPoint& point, const IntPoint&, Clipboard*, Frame* frame, bool linkDrag)
{
    IntSize bitmapSize([image.get() size]);
    bitmapSize.scale(frame->page()->deviceScaleFactor());
    RefPtr<ShareableBitmap> bitmap = convertImageToBitmap(image.get(), bitmapSize);
    ShareableBitmap::Handle handle;
    if (!bitmap->createHandle(handle))
        return;

    // FIXME: Seems this message should be named StartDrag, not SetDragImage.
    m_page->send(Messages::WebPageProxy::SetDragImage(frame->view()->contentsToWindow(point), handle, linkDrag));
}

static WebCore::CachedImage* cachedImage(Element* element)
{
    RenderObject* renderer = element->renderer();
    if (!renderer)
        return 0;
    if (!renderer->isRenderImage())
        return 0;
    WebCore::CachedImage* image = toRenderImage(renderer)->cachedImage();
    if (!image || image->errorOccurred()) 
        return 0;
    return image;
}

void WebDragClient::declareAndWriteDragImage(const String& pasteboardName, DOMElement *element, NSURL *URL, NSString *title, WebCore::Frame*)
{
    ASSERT(element);
    ASSERT(pasteboardName == String(NSDragPboard));

    Element* coreElement = core(element);

    WebCore::CachedImage* image = cachedImage(coreElement);

    NSString *extension = @"";
    if (image) {
        extension = image->image()->filenameExtension();
        if (![extension length])
            return;
    }

    if (![title length]) {
        title = [[URL path] lastPathComponent];
        if (![title length])
            title = [URL _web_userVisibleString];
    }

    RefPtr<LegacyWebArchive> archive = LegacyWebArchive::create(coreElement);

    NSURLResponse *response = image->response().nsURLResponse();
    
    RefPtr<SharedBuffer> imageBuffer = image->image()->data();
    size_t imageSize = imageBuffer->size();
    SharedMemory::Handle imageHandle;
    
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(imageBuffer->size());
    memcpy(sharedMemoryBuffer->data(), imageBuffer->data(), imageSize);
    sharedMemoryBuffer->createHandle(imageHandle, SharedMemory::ReadOnly);
    
    RetainPtr<CFDataRef> data = archive ? archive->rawDataRepresentation() : 0;
    SharedMemory::Handle archiveHandle;
    size_t archiveSize = 0;
    if (data) {
        RefPtr<SharedBuffer> buffer = SharedBuffer::wrapNSData((NSData *)data.get());
        RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(buffer->size());
        archiveSize = buffer->size();
        memcpy(sharedMemoryBuffer->data(), buffer->data(), archiveSize);
        sharedMemoryBuffer->createHandle(archiveHandle, SharedMemory::ReadOnly);            
    }
    m_page->send(Messages::WebPageProxy::SetPromisedData(pasteboardName, imageHandle, imageSize, String([response suggestedFilename]), String(extension), String(title), String([[response URL] absoluteString]), String([URL _web_userVisibleString]), archiveHandle, archiveSize));
}

} // namespace WebKit

#endif // ENABLE(DRAG_SUPPORT)
