/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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

#ifndef BackingStore_h
#define BackingStore_h

#include "BlackBerryGlobal.h"
#include <BlackBerryPlatformMisc.h>

namespace WebCore {
class ChromeClientBlackBerry;
class FloatPoint;
class FrameLoaderClientBlackBerry;
class GLES2Context;
class IntRect;
}

namespace BlackBerry {
namespace Platform {
class IntRect;
class FloatPoint;

namespace Graphics {
class Buffer;
}
}
}

namespace BlackBerry {
namespace WebKit {

class WebPage;
class WebPagePrivate;
class WebPageCompositorPrivate;
class BackingStorePrivate;
class BackingStoreClient;

class BLACKBERRY_EXPORT BackingStore {
public:
    enum ResumeUpdateOperation { None, Blit, RenderAndBlit };
    BackingStore(WebPage*, BackingStoreClient*);
    virtual ~BackingStore();

    void createSurface();

    void suspendBackingStoreUpdates();
    void resumeBackingStoreUpdates();

    void suspendGeometryUpdates();
    void resumeGeometryUpdates();

    void suspendScreenUpdates();
    void resumeScreenUpdates(BackingStore::ResumeUpdateOperation);

    bool isScrollingOrZooming() const;
    void setScrollingOrZooming(bool);

    void blitVisibleContents();
    void repaint(int x, int y, int width, int height, bool contentChanged, bool immediate);

    bool hasBlitJobs() const;
    void blitOnIdle();

    void acquireBackingStoreMemory();
    void releaseOwnedBackingStoreMemory();

    bool drawContents(BlackBerry::Platform::Graphics::Buffer*, const BlackBerry::Platform::IntRect& dstRect, double scale, const BlackBerry::Platform::FloatPoint& documentScrollPosition);

private:
    friend class BlackBerry::WebKit::BackingStoreClient;
    friend class BlackBerry::WebKit::BackingStorePrivate;
    friend class BlackBerry::WebKit::WebPage;
    friend class BlackBerry::WebKit::WebPagePrivate; // FIXME: For now, we expose our internals to WebPagePrivate. See PR #120301.
    friend class BlackBerry::WebKit::WebPageCompositorPrivate;
    friend class WebCore::ChromeClientBlackBerry;
    friend class WebCore::FrameLoaderClientBlackBerry;
    friend class WebCore::GLES2Context;
    BackingStorePrivate *d;
    DISABLE_COPY(BackingStore)
};
}
}

#endif // BackingStore_h
