/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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
#include "PluginViewPrivateBlackBerry.h"

#include "FrameView.h"
#include "HostWindow.h"
#if USE(ACCELERATED_COMPOSITING)
#include "PluginLayerWebKitThread.h"
#endif
#include "NPCallbacksBlackBerry.h"
#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformMessageClient.h>
#include <wtf/MainThread.h>

static unsigned s_counter = 0;

namespace WebCore {

PluginViewPrivate::PluginViewPrivate(PluginView* view)
    : m_view(view)
    , m_pluginBufferType(BlackBerry::Platform::Graphics::PluginBufferWithAlpha)
    , m_pluginFrontBuffer(0)
    , m_idlePrevented(false)
    , m_hasPendingGeometryChange(true)
    , m_sentOnLoad(false)
    , m_isFullScreen(false)
    , m_isFocused(false)
    , m_orientationLocked(false)
    , m_isBackgroundPlaying(false)
{
    m_pluginBuffers[0] = 0;
    m_pluginBuffers[1] = 0;

    pthread_mutexattr_t mutexAttributes;
    pthread_mutexattr_init(&mutexAttributes);
    pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&m_backBufferMutex, &mutexAttributes);
    pthread_rwlock_init(&m_frontBufferRwLock, 0);

    pthread_mutexattr_destroy(&mutexAttributes);

    char uniqueId[50];
    snprintf(uniqueId, sizeof(uniqueId), "PluginViewBB-%08x%08x-", s_counter++, (int)this);
    m_pluginUniquePrefix = uniqueId;
}

PluginViewPrivate::~PluginViewPrivate()
{
#if USE(ACCELERATED_COMPOSITING)
        if (m_platformLayer)
            static_cast<PluginLayerWebKitThread*>(m_platformLayer.get())->setPluginView(0);
#endif

    destroyBuffers();

    pthread_mutex_destroy(&m_backBufferMutex);
    pthread_rwlock_destroy(&m_frontBufferRwLock);
}

BlackBerry::Platform::Graphics::BufferType PluginViewPrivate::toBufferType(NPSurfaceFormat format)
{
    switch (format) {
    case FORMAT_RGB_565:
        return BlackBerry::Platform::Graphics::PluginBuffer;
    case FORMAT_RGBA_8888:
    default:
        return BlackBerry::Platform::Graphics::PluginBufferWithAlpha;
    }
}

void PluginViewPrivate::setZoomFactor(float zoomFactor)
{
    if (((NPSetWindowCallbackStruct*)m_view->m_npWindow.ws_info)->zoomFactor != zoomFactor)
        m_hasPendingGeometryChange = true;

    ((NPSetWindowCallbackStruct*)m_view->m_npWindow.ws_info)->zoomFactor = zoomFactor;
}

void PluginViewPrivate::setVisibleRects(const NPRect rects[], int32_t count)
{
    m_keepVisibleRect = IntRect();

    if (!m_view->parent() || !count)
        return;

    for (int i = 0; i < count; i++) {
        IntRect addRect = IntRect(rects[i].left, rects[i].top, rects[i].right - rects[i].left, rects[i].bottom - rects[i].top);
        m_keepVisibleRect.unite(addRect);
    }

    // Don't cause a possible scroll if the result is an empty rectangle.
    if (m_keepVisibleRect.isEmpty())
        return;

    // Adjust the rect to the parent window and then adjust for scrolling.
    m_keepVisibleRect = m_view->convertToContainingWindow(m_keepVisibleRect);
    FrameView* frameView = toFrameView(m_view->parent());
    m_keepVisibleRect.move(frameView->scrollPosition().x(), frameView->scrollPosition().y());

    frameView->hostWindow()->platformPageClient()->ensureContentVisible();
}

void PluginViewPrivate::clearVisibleRects()
{
    if (!m_keepVisibleRect.isEmpty())
        setVisibleRects(0, 0);
}

void PluginViewPrivate::showKeyboard(bool value)
{
    FrameView* frameView = toFrameView(m_view->parent());
    frameView->hostWindow()->platformPageClient()->showVirtualKeyboard(value);
}

void PluginViewPrivate::requestFullScreen()
{
    if (FrameView* frameView = toFrameView(m_view->parent()))
        if (frameView->hostWindow()->platformPageClient()->shouldPluginEnterFullScreen(m_view, m_pluginUniquePrefix.c_str()))
            m_view->handleFullScreenAllowedEvent();
}

void PluginViewPrivate::exitFullScreen()
{
    m_view->handleFullScreenExitEvent();
}

void PluginViewPrivate::requestCenterFitZoom()
{
    FrameView* frameView = toFrameView(m_view->parent());

    if (!frameView)
        return;

    frameView->hostWindow()->platformPageClient()->zoomToContentRect(m_view->m_windowRect);
}

void PluginViewPrivate::lockOrientation(bool landscape)
{
    FrameView* frameView = toFrameView(m_view->parent());

    if (!frameView)
        return;

    frameView->hostWindow()->platformPageClient()->lockOrientation(landscape);
    m_orientationLocked = true;
}

void PluginViewPrivate::unlockOrientation()
{
    if (!m_orientationLocked)
        return;

    FrameView* frameView = toFrameView(m_view->parent());

    if (!frameView)
        return;

    frameView->hostWindow()->platformPageClient()->unlockOrientation();
    m_orientationLocked = false;
}

void PluginViewPrivate::preventIdle(bool preventIdle)
{
    if (preventIdle == m_idlePrevented)
        return;

    FrameView* frameView = toFrameView(m_view->parent());
    if (!frameView)
        return;

    frameView->hostWindow()->platformPageClient()->setPreventsScreenDimming(preventIdle);
    m_idlePrevented = preventIdle;
}

void PluginViewPrivate::setHolePunch(int x, int y, int width, int height)
{
    if (width > 0 && height > 0)
        m_holePunchRect = IntRect(x, y, width, height);
    else
        m_holePunchRect = IntRect();

    // Clip the hole punch rectangle in case a plugin is 'overzealous', or 
    // does not clean up the hole punch rectangle correctly when exiting 
    // fullscreen (will mask bugs in the plugin).
    m_holePunchRect.intersect(IntRect(IntPoint(0, 0), m_view->frameRect().size()));

#if USE(ACCELERATED_COMPOSITING)
    if (m_platformLayer) {
        IntRect rect = m_holePunchRect;

        // Translate from plugin coordinates to contents coordinates.
        if (!m_holePunchRect.isEmpty())
            rect.move(m_view->frameRect().x(), m_view->frameRect().y());

        OwnPtr<HolePunchData> hp = adoptPtr(new HolePunchData);
        hp->x = m_holePunchRect.x();
        hp->y = m_holePunchRect.y();
        hp->w = m_holePunchRect.width();
        hp->h = m_holePunchRect.height();
        hp->layer = m_platformLayer;

        // Notify compositing layer and page client in order to be able to
        // punch through composited and non-composited parts of the page.
        callOnMainThread(npSetHolePunchHandler, hp.leakPtr()); // npSetHolePunchHandler() takes ownership of hp.
    }
#endif
}

NPSurface PluginViewPrivate::lockBackBuffer()
{
    pthread_mutex_lock(&m_backBufferMutex);
    BlackBerry::Platform::Graphics::Buffer* buffer = m_pluginBuffers[(m_pluginFrontBuffer + 1) % PLUGIN_BUFFERS];

    if (!buffer || !BlackBerry::Platform::Graphics::platformBufferHandle(buffer)) {
        unlockBackBuffer();
        return 0;
    }

    return BlackBerry::Platform::Graphics::platformBufferHandle(buffer);
}

void PluginViewPrivate::unlockBackBuffer()
{
    pthread_mutex_unlock(&m_backBufferMutex);
}

BlackBerry::Platform::Graphics::Buffer* PluginViewPrivate::lockReadFrontBufferInternal()
{
    pthread_rwlock_rdlock(&m_frontBufferRwLock);
    return m_pluginBuffers[m_pluginFrontBuffer];
}

NPSurface PluginViewPrivate::lockReadFrontBuffer()
{
    BlackBerry::Platform::Graphics::Buffer* buffer = lockReadFrontBufferInternal();

    if (!buffer || !BlackBerry::Platform::Graphics::platformBufferHandle(buffer)) {
        unlockReadFrontBuffer();
        return 0;
    }

    return BlackBerry::Platform::Graphics::platformBufferHandle(buffer);
}

void PluginViewPrivate::unlockReadFrontBuffer()
{
    pthread_rwlock_unlock(&m_frontBufferRwLock);
}

void PluginViewPrivate::swapBuffers()
{
    PthreadMutexLocker backLock(&m_backBufferMutex);
    PthreadWriteLocker frontLock(&m_frontBufferRwLock);
    m_pluginFrontBuffer = (m_pluginFrontBuffer + 1) % 2;
}

bool PluginViewPrivate::createBuffers(NPSurfaceFormat format, int width, int height)
{
    bool success = true;

    PthreadMutexLocker backLock(&m_backBufferMutex);
    PthreadWriteLocker frontLock(&m_frontBufferRwLock);

    bool didDestroyBuffers = false;
    for (int i = 0; i < PLUGIN_BUFFERS; i++) {
        if (m_pluginBuffers[i]) {
            didDestroyBuffers = true;
            BlackBerry::Platform::Graphics::destroyBuffer(m_pluginBuffers[i]);
            m_pluginBuffers[i] = 0;
        }

        if (width <= 0 || height <= 0)
            success = true;
        else {
            m_pluginBuffers[i] = BlackBerry::Platform::Graphics::createBuffer(
                BlackBerry::Platform::IntSize(width, height),
                toBufferType(format));

            if (!m_pluginBuffers[i])
                success = false;
        }
    }

    if (success) {
        m_pluginBufferSize = IntSize(width, height);
        m_pluginBufferType = toBufferType(format);

        if (didDestroyBuffers) {
            BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
                BlackBerry::Platform::createFunctionCallMessage(&BlackBerry::Platform::Graphics::collectThreadSpecificGarbage));
        }
    } else {
        m_pluginBufferSize = IntSize();
        m_pluginBufferType = BlackBerry::Platform::Graphics::PluginBufferWithAlpha;
        destroyBuffers();
    }

    return success;
}

bool PluginViewPrivate::resizeBuffers(NPSurfaceFormat format, int width, int height)
{
    bool success = true;

    // If there is no buffer created, then try to create it.
    if (!m_pluginBufferSize.width() || !m_pluginBufferSize.height())
        return createBuffers(format, width, height);

    if (!width || !height)
        return destroyBuffers();

    PthreadMutexLocker backLock(&m_backBufferMutex);
    PthreadWriteLocker frontLock(&m_frontBufferRwLock);

    for (int i = 0; i < PLUGIN_BUFFERS && success; i++) {
        success &= BlackBerry::Platform::Graphics::reallocBuffer(m_pluginBuffers[i],
            BlackBerry::Platform::IntSize(width, height),
            toBufferType(format));
    }

    if (success) {
        m_pluginBufferSize = IntSize(width, height);
        m_pluginBufferType = toBufferType(format);
        return true;
    }

    // Attempt to undo if we failed to get the new size/format. We can't guarantee
    // we will get it back though.
    if (!m_pluginBufferSize.width() || !m_pluginBufferSize.height()) {
        destroyBuffers();
        return false;
    }

    bool undone = true;
    for (int i = 0; i < PLUGIN_BUFFERS; i++) {
        undone &= BlackBerry::Platform::Graphics::reallocBuffer(m_pluginBuffers[i],
            m_pluginBufferSize, m_pluginBufferType);
    }

    // If we fail to undo, delete the buffers altogether.
    if (!undone)
        destroyBuffers();

    return false;
}

bool PluginViewPrivate::destroyBuffers()
{
    bool didDestroyBuffers = false;

    {
        PthreadMutexLocker backLock(&m_backBufferMutex);
        PthreadWriteLocker frontLock(&m_frontBufferRwLock);

        for (int i = 0; i < PLUGIN_BUFFERS; i++) {
            if (m_pluginBuffers[i]) {
                didDestroyBuffers = true;
                BlackBerry::Platform::Graphics::destroyBuffer(m_pluginBuffers[i]);
                m_pluginBuffers[i] = 0;
            }
        }
        m_pluginBufferSize = IntSize();
    }

    if (didDestroyBuffers) {
        BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchSyncMessage(
            BlackBerry::Platform::createFunctionCallMessage(&BlackBerry::Platform::Graphics::collectThreadSpecificGarbage));
    }

    return true;
}

} // namespace WebCore


