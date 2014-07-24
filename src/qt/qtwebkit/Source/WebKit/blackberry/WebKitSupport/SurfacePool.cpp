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

#include "config.h"
#include "SurfacePool.h"

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformThreading.h>
#include <errno.h>

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
#include <EGL/eglext.h>
#endif

namespace BlackBerry {
namespace WebKit {

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

SurfacePool* SurfacePool::globalSurfacePool()
{
    static SurfacePool* s_instance = 0;
    if (!s_instance)
        s_instance = new SurfacePool;
    return s_instance;
}

SurfacePool::SurfacePool()
    : m_numberOfFrontBuffers(0)
    , m_initialized(false)
    , m_buffersSuspended(false)
    , m_hasFenceExtension(false)
{
}

int SurfacePool::numberOfBackingStoreFrontBuffers() const
{
    return m_numberOfFrontBuffers;
}

void SurfacePool::initialize(const Platform::IntSize& tileSize)
{
    if (m_initialized)
        return;
    m_initialized = true;

    m_numberOfFrontBuffers = Platform::Settings::instance()->numberOfBackingStoreFrontBuffers();

    if (!m_numberOfFrontBuffers)
        return; // We completely disable tile rendering when 0 tiles are specified.

    const unsigned numberOfBackBuffers = Platform::Settings::instance()->numberOfBackingStoreBackBuffers();
    const unsigned numberOfPoolTiles = m_numberOfFrontBuffers + numberOfBackBuffers; // back buffer

    for (size_t i = 0; i < numberOfPoolTiles; ++i)
        m_tileBufferPool.append(new TileBuffer(tileSize));

    // All buffers not used as front buffers are used as back buffers.
    // Initially, that's all of them.
    m_availableBackBufferPool = m_tileBufferPool;

#if BLACKBERRY_PLATFORM_GRAPHICS_EGL
    const char* extensions = eglQueryString(Platform::Graphics::eglDisplay(), EGL_EXTENSIONS);
    if (strstr(extensions, "EGL_KHR_fence_sync")) {
        // We assume GL_OES_EGL_sync is present, but we don't check for it because
        // no GL context is current at this point.
        // TODO: check for it
        eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) eglGetProcAddress("eglCreateSyncKHR");
        eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) eglGetProcAddress("eglDestroySyncKHR");
        eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) eglGetProcAddress("eglClientWaitSyncKHR");
        m_hasFenceExtension = eglCreateSyncKHR && eglDestroySyncKHR && eglClientWaitSyncKHR;
    }
#endif

    // m_mutex must be recursive because destroyPlatformSync may be called indirectly
    // from notifyBuffersComposited
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

PlatformGraphicsContext* SurfacePool::createPlatformGraphicsContext(Platform::Graphics::Drawable* drawable) const
{
    return drawable;
}

void SurfacePool::destroyPlatformGraphicsContext(PlatformGraphicsContext*) const
{
}

unsigned SurfacePool::numberOfAvailableBackBuffers() const
{
    return m_availableBackBufferPool.size();
}

TileBuffer* SurfacePool::takeBackBuffer()
{
    ASSERT(!m_availableBackBufferPool.isEmpty());
    if (m_availableBackBufferPool.isEmpty())
        return 0;

    TileBuffer* buffer = m_availableBackBufferPool.first();

    // Reorder so we get FIFO semantics. Should minimize fence waiting times.
    for (unsigned i = 0; i < m_availableBackBufferPool.size() - 1; ++i)
        m_availableBackBufferPool[i] = m_availableBackBufferPool[i + 1];
    m_availableBackBufferPool.removeLast();

    ASSERT(buffer);
    return buffer;
}

void SurfacePool::addBackBuffer(TileBuffer* tileBuffer)
{
    ASSERT(tileBuffer);
    tileBuffer->clearRenderedRegion();
    m_availableBackBufferPool.append(tileBuffer);
}

void SurfacePool::createBuffers()
{
    if (!m_initialized || m_tileBufferPool.isEmpty() || !m_buffersSuspended)
        return;

    // Create the tile pool.
    for (size_t i = 0; i < m_tileBufferPool.size(); ++i) {
        if (m_tileBufferPool[i]->wasNativeBufferCreated())
            Platform::Graphics::createPixmapBuffer(m_tileBufferPool[i]->nativeBuffer());
    }

    m_buffersSuspended = false;
}

void SurfacePool::releaseBuffers()
{
    if (!m_initialized || m_tileBufferPool.isEmpty() || m_buffersSuspended)
        return;

    m_buffersSuspended = true;

    // Release the tile pool.
    for (size_t i = 0; i < m_tileBufferPool.size(); ++i) {
        if (!m_tileBufferPool[i]->wasNativeBufferCreated())
            continue;
        m_tileBufferPool[i]->clearRenderedRegion();
        // Clear the buffer to prevent accidental leakage of (possibly sensitive) pixel data.
        Platform::Graphics::clearBuffer(m_tileBufferPool[i]->nativeBuffer(), 0, 0, 0, 0);
        Platform::Graphics::destroyPixmapBuffer(m_tileBufferPool[i]->nativeBuffer());
    }

    Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
        Platform::createFunctionCallMessage(&Platform::Graphics::collectThreadSpecificGarbage));
}

void SurfacePool::waitForBuffer(TileBuffer*)
{
    if (!m_hasFenceExtension)
        return;
}

void SurfacePool::notifyBuffersComposited(const TileBufferList&)
{
    if (!m_hasFenceExtension)
        return;
}

void SurfacePool::destroyPlatformSync(void*)
{
}

}
}
