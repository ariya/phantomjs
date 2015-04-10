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
#include "NPCallbacksBlackBerry.h"

#include "PluginViewPrivateBlackBerry.h"

#if USE(ACCELERATED_COMPOSITING)
#include "PluginLayerWebKitThread.h"
#endif

namespace WebCore {

PthreadMutexLocker::PthreadMutexLocker(pthread_mutex_t* mutex)
    : m_mutex(mutex)
{
    pthread_mutex_lock(m_mutex);
}

PthreadMutexLocker::~PthreadMutexLocker()
{
    pthread_mutex_unlock(m_mutex);
}

PthreadReadLocker::PthreadReadLocker(pthread_rwlock_t* rwlock)
    : m_rwlock(rwlock)
{
    pthread_rwlock_rdlock(m_rwlock);
}

PthreadReadLocker::~PthreadReadLocker()
{
    pthread_rwlock_unlock(m_rwlock);
}

PthreadWriteLocker::PthreadWriteLocker(pthread_rwlock_t* rwlock)
    : m_rwlock(rwlock)
{
    pthread_rwlock_wrlock(m_rwlock);
}

PthreadWriteLocker::~PthreadWriteLocker()
{
    pthread_rwlock_unlock(m_rwlock);
}

#if USE(ACCELERATED_COMPOSITING)
void npSetHolePunchHandler(void* holePunchData)
{
    OwnPtr<HolePunchData> data = adoptPtr(static_cast<HolePunchData*>(holePunchData));
    if (data->layer)
        static_cast<PluginLayerWebKitThread*>(data->layer.get())->setHolePunchRect(IntRect(data->x, data->y, data->w, data->h));
}
#endif

// NPAPI callback functions

void setVisibleRects(NPP instance, const NPRect rects[], int32_t count)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->setVisibleRects(rects, count);
}

void clearVisibleRects(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->clearVisibleRects();
}

void showKeyboard(NPP instance, bool value)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->showKeyboard(value);
}

void requestFullScreen(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->requestFullScreen();
}

void exitFullScreen(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->exitFullScreen();
}

void requestCenterFitZoom(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->requestCenterFitZoom();
}

void lockOrientation(NPP instance, bool landscape)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->lockOrientation(landscape);
}

void unlockOrientation(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->unlockOrientation();
}

void preventIdle(NPP instance, bool preventIdle)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->preventIdle(preventIdle);
}

NPSurface lockBackBuffer(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    return viewPrivate->lockBackBuffer();
}

void unlockBackBuffer(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->unlockBackBuffer();
}

NPSurface lockReadFrontBuffer(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    return viewPrivate->lockReadFrontBuffer();
}

void unlockReadFrontBuffer(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->unlockReadFrontBuffer();
}

void swapBuffers(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->swapBuffers();
}

bool createBuffers(NPP instance, NPSurfaceFormat format, int width, int height)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    return viewPrivate->createBuffers(format, width, height);
}

bool destroyBuffers(NPP instance)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    return viewPrivate->destroyBuffers();
}

bool resizeBuffers(NPP instance, NPSurfaceFormat format, int width, int height)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    return viewPrivate->resizeBuffers(format, width, height);
}

void setHolePunch(NPP instance, int x, int y, int width, int height)
{
    PluginView* view = static_cast<PluginView*>(instance->ndata);
    PluginViewPrivate* viewPrivate = view->getPrivate();
    viewPrivate->setHolePunch(x, y, width, height);
}

NPCallbacks s_NpCallbacks = {
    setVisibleRects,
    clearVisibleRects,
    showKeyboard,
    requestFullScreen,
    exitFullScreen,
    requestCenterFitZoom,
    lockOrientation,
    unlockOrientation,
    preventIdle,
    lockBackBuffer,
    unlockBackBuffer,
    lockReadFrontBuffer,
    unlockReadFrontBuffer,
    swapBuffers,
    createBuffers,
    destroyBuffers,
    resizeBuffers,
    setHolePunch
};

} // Namespace WebCore

