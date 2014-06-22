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

#ifndef NPCallbacksBlackBerry_h
#define NPCallbacksBlackBerry_h

#include "PluginView.h"
#include <pthread.h>

namespace WebCore {

struct PthreadMutexLocker {
    PthreadMutexLocker(pthread_mutex_t*);
    ~PthreadMutexLocker();

    pthread_mutex_t* m_mutex;
};

struct PthreadReadLocker {
    PthreadReadLocker(pthread_rwlock_t*);
    ~PthreadReadLocker();

    pthread_rwlock_t* m_rwlock;
};

struct PthreadWriteLocker {
    PthreadWriteLocker(pthread_rwlock_t*);
    ~PthreadWriteLocker();

    pthread_rwlock_t* m_rwlock;
};

#if USE(ACCELERATED_COMPOSITING)
struct HolePunchData {
    int x, y;
    int w, h;
    RefPtr<PlatformLayer> layer;
};

void npSetHolePunchHandler(void* holePunchData);
#endif

// Static NPAPI Callbacks

void setVisibleRects(NPP, const NPRect rects[], int32_t count);
void clearVisibleRects(NPP);
void showKeyboard(NPP, bool value);
void requestFullScreen(NPP);
void exitFullScreen(NPP);
void requestCenterFitZoom(NPP);
void lockOrientation(NPP, bool landscape);
void unlockOrientation(NPP);
void preventIdle(NPP, bool preventIdle);
NPSurface lockBackBuffer(NPP);
void unlockBackBuffer(NPP);
NPSurface lockReadFrontBuffer(NPP);
void unlockReadFrontBuffer(NPP);
void swapBuffers(NPP);
bool createBuffers(NPP, NPSurfaceFormat, int width, int height);
bool destroyBuffers(NPP);
bool resizeBuffers(NPP, NPSurfaceFormat, int width, int height);
void setHolePunch(NPP, int x, int y, int width, int height);

extern NPCallbacks s_NpCallbacks;

} // Namespace WebCore

#endif
