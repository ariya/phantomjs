/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef BackingStoreTile_h
#define BackingStoreTile_h

#include "BlackBerryPlatformIntRectRegion.h"
#include "BlackBerryPlatformMisc.h"
#include "BlackBerryPlatformPrimitives.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace BlackBerry {
namespace Platform {
namespace Graphics {
struct Buffer;
}
}

namespace WebKit {

// Represents a fence that has been inserted into the command stream. You can wait on the corresponding platform sync
// object to make sure that previous commands have been completed.
// There is no reason to wait twice on the same platform sync object, so the only mechanism provided to access the sync
// object will also clear it to prevent anyone from waiting again.
// Fence is only refcounted on the compositing thread, so RefCounted will suffice, we don't need ThreadSafeRefCounted.
class Fence : public RefCounted<Fence> {
public:
    static PassRefPtr<Fence> create(void* platformSync = 0)
    {
        return adoptRef(new Fence(platformSync));
    }

    // Returns 0 if this fence is stale (someone already waited on it)
    // Otherwise returns the platform sync object and clears the sync
    // object so no-one waits again.
    void* takePlatformSync()
    {
        void* tmp = m_platformSync;
        m_platformSync = 0;
        return tmp;
    }

    ~Fence();

private:
    Fence(void* platformSync)
        : m_platformSync(platformSync)
    {
    }

    void* m_platformSync;
};

class TileBuffer {
    public:
        TileBuffer(const Platform::IntSize&);
        ~TileBuffer();
        Platform::IntSize size() const;
        Platform::IntRect surfaceRect() const;
        Platform::IntRect pixelContentsRect() const;

        Platform::IntPoint lastRenderOrigin() const { return m_lastRenderOrigin; }
        void setLastRenderOrigin(const Platform::IntPoint& origin) { m_lastRenderOrigin = origin; }
        double lastRenderScale() const { return m_lastRenderScale; }
        void setLastRenderScale(double scale) { m_lastRenderScale = scale; }

        bool isRendered(double scale) const;
        bool isRendered(const Platform::IntRectRegion& pixelContentsRegion, double scale) const;
        void clearRenderedRegion();
        void clearRenderedRegion(const Platform::IntRectRegion&);
        void addRenderedRegion(const Platform::IntRectRegion&);
        Platform::IntRectRegion renderedRegion() const;
        Platform::IntRectRegion notRenderedRegion() const;

        Platform::Graphics::Buffer* nativeBuffer() const;
        bool wasNativeBufferCreated() const;

        bool backgroundPainted() const { return m_backgroundPainted; }
        void paintBackground();

        Fence* fence() const { return m_fence.get(); }
        void setFence(PassRefPtr<Fence> fence) { m_fence = fence; }

    private:
        Platform::IntPoint m_lastRenderOrigin;
        Platform::IntSize m_size;
        Platform::IntRectRegion m_renderedRegion;
        RefPtr<Fence> m_fence;
        mutable Platform::Graphics::Buffer* m_nativeBuffer;
        double m_lastRenderScale;
        bool m_backgroundPainted;
        DISABLE_COPY(TileBuffer)
};

} // namespace WebKit
} // namespace BlackBerry

#endif // BackingStoreTile_h
