/*
 * Copyright (C) 2012, Igalia S.L.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef  RedirectedXCompositeWindow_h
#define  RedirectedXCompositeWindow_h

#if USE(OPENGL) && PLATFORM(X11)

#include "GLContext.h"
#include "IntSize.h"
#include "RefPtrCairo.h"

typedef unsigned long Pixmap;
typedef unsigned long Window;
typedef unsigned long Damage;
typedef void (*DamageNotifyCallback)(void*);

namespace WebCore {

class RedirectedXCompositeWindow {
public:
    enum GLContextNeeded { CreateGLContext, DoNotCreateGLContext };
    static PassOwnPtr<RedirectedXCompositeWindow> create(const IntSize&, GLContextNeeded = CreateGLContext);
    virtual ~RedirectedXCompositeWindow();
    const IntSize& size() { return m_size; }

    void resize(const IntSize& newSize);
    GLContext* context();
    cairo_surface_t* cairoSurfaceForWidget(GtkWidget*);
    Window windowId() { return m_window; }
    void callDamageNotifyCallback();

    void setDamageNotifyCallback(DamageNotifyCallback callback, void* data)
    {
        m_damageNotifyCallback = callback;
        m_damageNotifyData = data;
    }

private:
    RedirectedXCompositeWindow(const IntSize&, GLContextNeeded);
    void cleanupPixmapAndPixmapSurface();

    IntSize m_size;
    Window m_window;
    Window m_parentWindow;
    Pixmap m_pixmap;
    GLContextNeeded m_needsContext;
    OwnPtr<GLContext> m_context;
    RefPtr<cairo_surface_t> m_surface;
    unsigned int m_pendingResizeSourceId;
    bool m_needsNewPixmapAfterResize;

    Damage m_damage;
    DamageNotifyCallback m_damageNotifyCallback;
    void* m_damageNotifyData;
};

} // namespace WebCore

#endif // USE(OPENGL) && PLATFORM(X11)

#endif // RedirectedXCompositeWindow_h
