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

#ifndef PluginViewPrivateBlackBerry_h
#define PluginViewPrivateBlackBerry_h

#include "PluginView.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformIntRectRegion.h>
#include <pthread.h>

#define PLUGIN_BUFFERS 2

namespace WebCore {

class PluginViewPrivate {

public:
    PluginViewPrivate(PluginView*);
    ~PluginViewPrivate();

    BlackBerry::Platform::Graphics::BufferType toBufferType(NPSurfaceFormat);
    void setZoomFactor(float);
    void setVisibleRects(const NPRect rects[], int32_t count);
    void clearVisibleRects();
    void showKeyboard(bool value);
    void requestFullScreen();
    void exitFullScreen();
    void requestCenterFitZoom();
    void lockOrientation(bool landscape);
    void unlockOrientation();
    void preventIdle(bool preventIdle);
    void setHolePunch(int x, int y, int width, int height);
    NPSurface lockBackBuffer();
    void unlockBackBuffer();
    BlackBerry::Platform::Graphics::Buffer* lockReadFrontBufferInternal();
    NPSurface lockReadFrontBuffer();
    void unlockReadFrontBuffer();
    void swapBuffers();
    bool createBuffers(NPSurfaceFormat, int width, int height);
    bool resizeBuffers(NPSurfaceFormat, int width, int height);
    bool destroyBuffers();

private:
    PluginView* m_view;
    pthread_mutex_t m_backBufferMutex;
    pthread_rwlock_t m_frontBufferRwLock;

    BlackBerry::Platform::Graphics::Buffer* m_pluginBuffers[PLUGIN_BUFFERS];
    IntSize m_pluginBufferSize;
    BlackBerry::Platform::Graphics::BufferType m_pluginBufferType;
    int m_pluginFrontBuffer;

    IntRect m_holePunchRect;
    IntRect m_keepVisibleRect;

#if USE(ACCELERATED_COMPOSITING)
    RefPtr<PlatformLayer> m_platformLayer;
#endif

    bool m_idlePrevented;
    bool m_hasPendingGeometryChange;
    bool m_sentOnLoad;
    bool m_isFullScreen;
    bool m_isFocused;
    bool m_orientationLocked;
    bool m_isBackgroundPlaying;

    std::string m_pluginUniquePrefix;

    BlackBerry::Platform::IntRectRegion m_invalidateRegion;
    friend class PluginView;
};

} // namespace WebCore

#endif
