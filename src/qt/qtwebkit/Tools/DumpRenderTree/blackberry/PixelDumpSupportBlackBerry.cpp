/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PixelDumpSupportBlackBerry.h"

#include "BackingStore.h"
#include "BlackBerryPlatformExecutableMessage.h"
#include "BlackBerryPlatformGraphics.h"
#include "BlackBerryPlatformGraphicsContext.h"
#include "BlackBerryPlatformGraphicsImpl.h"
#include "BlackBerryPlatformMessageClient.h"
#include "DumpRenderTreeBlackBerry.h"
#include "PNGImageEncoder.h"
#include "PixelDumpSupport.h"
#include "WebPage.h"
#include "WebPageClient.h"

#include <BlackBerryPlatformWindow.h>
#include <wtf/MD5.h>
#include <wtf/Vector.h>

using namespace BlackBerry::WebKit;
using namespace BlackBerry;
using namespace WTF;

void readPixelsUserInterfaceThread(BlackBerry::Platform::Graphics::PlatformGraphicsContext* context, const BlackBerry::Platform::IntRect& srcRect, unsigned char* pixels)
{
    context->readPixels(srcRect, pixels, false, false);
}

PassRefPtr<BitmapContext> createBitmapContextFromWebView(bool /*onscreen*/, bool /*incrementalRepaint*/, bool /*sweepHorizontally*/, bool /*drawSelectionRect*/)
{
    Platform::Graphics::Window* window = DumpRenderTree::currentInstance()->page()->client()->window();
    ASSERT(window);

    // The BackingStore has a queue of pending jobs, which are run on idle
    // and which may not have been run yet.
    BackingStore* backingStore = DumpRenderTree::currentInstance()->page()->backingStore();
    while (backingStore->hasBlitJobs())
        backingStore->blitOnIdle();

    const Platform::IntRect& windowRect = window->viewportRect();
    const Platform::IntSize& windowSize = window->viewportSize();
    unsigned char* data = new unsigned char[windowSize.width() * windowSize.height() * 4];

    BlackBerry::Platform::Graphics::Buffer* buffer = BlackBerry::Platform::Graphics::createBuffer(windowSize, BlackBerry::Platform::Graphics::AlwaysBacked);
    BlackBerry::Platform::Graphics::Drawable* drawable = BlackBerry::Platform::Graphics::lockBufferDrawable(buffer);
    if (drawable) {
        backingStore->drawContents(drawable, windowRect, windowSize);
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
            BlackBerry::Platform::createFunctionCallMessage(&readPixelsUserInterfaceThread, drawable, windowRect, data));
        BlackBerry::Platform::Graphics::releaseBufferDrawable(buffer);
    }
    BlackBerry::Platform::Graphics::destroyBuffer(buffer);
    return BitmapContext::createByAdoptingData(data, windowSize.width(), windowSize.height());
}

void computeMD5HashStringForBitmapContext(BitmapContext* context, char hashString[33])
{
    int pixelsWide = context->m_width;
    int pixelsHigh = context->m_height;
    int bytesPerRow = context->m_width * 4;
    unsigned char* pixelData = context->m_data;

    MD5 md5;
    for (int i = 0; i < pixelsHigh; ++i) {
        md5.addBytes(pixelData, 4 * pixelsWide);
        pixelData += bytesPerRow;
    }

    Vector<uint8_t, 16> hash;
    md5.checksum(hash);

    hashString[0] = '\0';
    for (int i = 0; i < 16; ++i)
        snprintf(hashString, 33, "%s%02x", hashString, hash[i]);
}

static void printPNG(BitmapContext* context, const char* checksum)
{
    Vector<unsigned char> pngData;
    encodeBitmapToPNG(context->m_data, context->m_width, context->m_height, &pngData);
    printPNG(pngData.data(), pngData.size(), checksum);
}

void dumpBitmap(BitmapContext* context, const char* checksum)
{
    printPNG(context, checksum);
}
