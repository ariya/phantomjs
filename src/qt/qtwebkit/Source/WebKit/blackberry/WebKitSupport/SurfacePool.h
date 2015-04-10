/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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

#ifndef SurfacePool_h
#define SurfacePool_h

#include "BackingStoreTile.h"
#include "GraphicsContext.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformPrimitives.h>
#include <pthread.h>
#include <set>
#include <wtf/Vector.h>

namespace BlackBerry {
namespace WebKit {

class SurfacePool {
public:
    static SurfacePool* globalSurfacePool();

    void initialize(const BlackBerry::Platform::IntSize&);

    int isActive() const { return !m_tileBufferPool.isEmpty() && !m_buffersSuspended; }
    int isEmpty() const { return m_tileBufferPool.isEmpty(); }
    int numberOfBackingStoreFrontBuffers() const;

    PlatformGraphicsContext* createPlatformGraphicsContext(BlackBerry::Platform::Graphics::Drawable*) const;
    void destroyPlatformGraphicsContext(PlatformGraphicsContext*) const;

    // The surface pool will allocate as many back buffers as specified by
    // Platform::Settings::instance()->numberOfBackingStoreBackBuffers() which
    // allows for at least one back buffer to be available for drawing before
    // swapping buffers/geometry to the front.
    unsigned numberOfAvailableBackBuffers() const;
    TileBuffer* takeBackBuffer();
    void addBackBuffer(TileBuffer*);

    void releaseBuffers();
    void createBuffers();

    // EGLImage synchronization between WebKit and compositing threads
    // TODO: Figure out how to improve the BlackBerry::Platform::Graphics with API that can encapsulate
    // this kind of synchronisation mechanism.

    // WebKit thread must waitForBuffer() before rendering to EGLImage
    void waitForBuffer(TileBuffer*);

    // Compositing thread must notify the SurfacePool when EGLImages are composited
    void notifyBuffersComposited(const WTF::Vector<TileBuffer*>& buffers);

    void destroyPlatformSync(void* platformSync);

private:
    SurfacePool();

    typedef WTF::Vector<TileBuffer*> TileBufferList;
    TileBufferList m_tileBufferPool;
    TileBufferList m_availableBackBufferPool;
    unsigned m_numberOfFrontBuffers;
    bool m_initialized; // SurfacePool has been set up, with or without buffers.
    bool m_buffersSuspended; // Buffer objects exist, but pixel memory has been freed.

    std::set<void*> m_garbage;
    bool m_hasFenceExtension;
    mutable pthread_mutex_t m_mutex;
};
}
}

#endif // SurfacePool_h
