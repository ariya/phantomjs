/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "X11Helper.h"

namespace WebCore {

// Used for handling XError.
static bool validOperation = true;
static int handleXPixmapCreationError(Display*, XErrorEvent* event)
{
    if (event->error_code == BadMatch || event->error_code == BadWindow || event->error_code == BadAlloc) {
        validOperation = false;

        switch (event->error_code) {
        case BadMatch:
            LOG_ERROR("BadMatch.");
            break;
        case BadWindow:
            LOG_ERROR("BadWindow.");
            break;
        case BadAlloc:
            LOG_ERROR("BadAlloc.");
            break;
        default:
            break;
        }
    }

    return 0;
}

struct DisplayConnection {
    DisplayConnection()
    {
        m_display = XOpenDisplay(0);

        if (!m_display)
            LOG_ERROR("Failed to make connection with X");
    }

    ~DisplayConnection()
    {
        XCloseDisplay(m_display);
    }

    Display* display() { return m_display; }
private:
    Display* m_display;
};

struct OffScreenRootWindow {

    OffScreenRootWindow()
    {
        m_window = 0;
        Display* dpy = X11Helper::nativeDisplay();
        if (!dpy)
            return;

        XSetWindowAttributes attributes;
        attributes.override_redirect = true;
        m_window = XCreateSimpleWindow(dpy, XDefaultRootWindow(dpy), -1, -1, 1, 1, 0, BlackPixel(dpy, 0), WhitePixel(dpy, 0));
        // From http://tronche.com/gui/x/xlib/window/attributes/
        XChangeWindowAttributes(dpy, m_window, CWOverrideRedirect, &attributes);
        XMapWindow(dpy, m_window);

        if (!m_window)
            LOG_ERROR("Failed to create offscreen root window.");
    }

    ~OffScreenRootWindow()
    {
        if (!X11Helper::nativeDisplay())
            return;

        if (m_window) {
            XUnmapWindow(X11Helper::nativeDisplay(), m_window);
            XDestroyWindow(X11Helper::nativeDisplay(), m_window);
            m_window = 0;
        }
    }

    Window rootWindow()
    {
        return m_window;
    }

private:
    Window m_window;
};

ScopedXPixmapCreationErrorHandler::ScopedXPixmapCreationErrorHandler()
{
    // XSync must be called to ensure that current errors are handled by the original handler.
    XSync(X11Helper::nativeDisplay(), false);
    m_previousErrorHandler = XSetErrorHandler(handleXPixmapCreationError);
}

ScopedXPixmapCreationErrorHandler::~ScopedXPixmapCreationErrorHandler()
{
    // Restore the original handler.
    XSetErrorHandler(m_previousErrorHandler);
}

bool ScopedXPixmapCreationErrorHandler::isValidOperation() const
{
    validOperation = true;
    // XSync is needed to catch possible errors as they are generated asynchronously.
    XSync(X11Helper::nativeDisplay(), false);
    return validOperation;
}

void X11Helper::resizeWindow(const IntRect& newRect, const uint32_t windowId)
{
    XResizeWindow(nativeDisplay(), windowId, newRect.width(), newRect.height());
    XFlush(nativeDisplay());
}

void X11Helper::createPixmap(Pixmap* handleId, const XVisualInfo& visualInfo, const IntSize& size)
{
    Display* display = nativeDisplay();
    if (!display)
        return;

    if (!visualInfo.visual) {
        LOG_ERROR("Failed to find valid XVisual.");
        return;
    }

    Window xWindow = offscreenRootWindow();
    if (!xWindow) {
        LOG_ERROR("Failed to create offscreen root window.");
        return;
    }

    Pixmap tempHandleId = XCreatePixmap(display, xWindow, size.width(), size.height(), visualInfo.depth);

    if (!tempHandleId) {
        LOG_ERROR("Failed to create offscreen pixmap.");
        return;
    }

    *handleId = tempHandleId;
    XSync(X11Helper::nativeDisplay(), false);
}

void X11Helper::destroyPixmap(const uint32_t pixmapId)
{
    if (!pixmapId)
        return;

    Display* display = nativeDisplay();
    if (!display)
        return;

    XFreePixmap(display, pixmapId);
    XSync(X11Helper::nativeDisplay(), false);
}

void X11Helper::createOffScreenWindow(uint32_t* handleId, const XVisualInfo& visInfo, const IntSize& size)
{
#if USE(GRAPHICS_SURFACE)
    Display* display = nativeDisplay();
    if (!display)
        return;

    if (!visInfo.visual) {
        LOG_ERROR("Failed to find valid XVisual.");
        return;
    }

    Window xWindow = offscreenRootWindow();
    if (!xWindow)
        return;

    Colormap cmap = XCreateColormap(display, xWindow, visInfo.visual, AllocNone);
    XSetWindowAttributes attribute;
    attribute.background_pixel = WhitePixel(display, 0);
    attribute.border_pixel = BlackPixel(display, 0);
    attribute.colormap = cmap;
#if USE(GLX)
    attribute.event_mask = ResizeRedirectMask;
#endif
    uint32_t tempHandleId = XCreateWindow(display, xWindow, 0, 0, size.width(), size.height(), 0, visInfo.depth, InputOutput, visInfo.visual, CWBackPixel | CWBorderPixel | CWColormap, &attribute);

    if (!tempHandleId) {
        LOG_ERROR("Failed to create offscreen window.");
        return;
    }

    XSetWindowBackgroundPixmap(display, tempHandleId, 0);
#if USE(GLX)
    XCompositeRedirectWindow(display, tempHandleId, CompositeRedirectManual);
#endif
    XMapWindow(display, tempHandleId);
    *handleId = tempHandleId;
#else
    UNUSED_PARAM(handleId);
    UNUSED_PARAM(visInfo);
    UNUSED_PARAM(size);
#endif
}

#if USE(EGL)
void X11Helper::createOffScreenWindow(uint32_t* handleId, const EGLint id, bool supportsAlpha, const IntSize& size)
{
#if USE(GRAPHICS_SURFACE)
    VisualID visualId = static_cast<VisualID>(id);

    if (!visualId)
        return;

    // EGL has suggested a visual id, so get the rest of the visual info for that id.
    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = visualId;
    int matchingCount = 0;
    OwnPtrX11<XVisualInfo> matchingVisuals(XGetVisualInfo(nativeDisplay(), VisualIDMask, &visualInfoTemplate, &matchingCount));
    XVisualInfo* foundVisual = 0;

    if (matchingVisuals) {
        for (int i = 0; i< matchingCount; i++) {
            XVisualInfo* temp = &matchingVisuals[i];
            int matchingdepth = supportsAlpha ? 32 : 24;

            if (temp->visualid == visualId && temp->depth == matchingdepth) {
                foundVisual = temp;
                break;
            }
        }

        if (foundVisual)
            createOffScreenWindow(handleId, *foundVisual, size);
    }
#else
    UNUSED_PARAM(handleId);
    UNUSED_PARAM(id);
    UNUSED_PARAM(size);
#endif
}

void X11Helper::createPixmap(Pixmap* handleId, const EGLint id, bool hasAlpha, const IntSize& size)
{
    VisualID visualId = static_cast<VisualID>(id);

    if (!visualId)
        return;

    // EGL has suggested a visual id, so get the rest of the visual info for that id.
    XVisualInfo visualInfoTemplate;
    memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
    visualInfoTemplate.visualid = visualId;
    int matchingCount = 0;
    OwnPtrX11<XVisualInfo> matchingVisuals(XGetVisualInfo(nativeDisplay(), VisualIDMask, &visualInfoTemplate, &matchingCount));
    XVisualInfo* foundVisual = 0;
    int requiredDepth = hasAlpha ? 32 : 24;

    if (matchingVisuals) {
        for (int i = 0; i< matchingCount; i++) {
            XVisualInfo* temp = &matchingVisuals[i];

            if (temp->visualid == visualId && temp->depth == requiredDepth) {
                foundVisual = temp;
                break;
            }
        }

        if (foundVisual)
            createPixmap(handleId, *foundVisual, size);
    }
}
#endif

void X11Helper::destroyWindow(const uint32_t windowId)
{
    if (!windowId)
        return;

    Display* display = nativeDisplay();
    if (!display)
        return;

    XDestroyWindow(display, windowId);
}

bool X11Helper::isXRenderExtensionSupported()
{
    static bool queryDone = false;
    static bool supportsXRenderExtension = false;

    if (!queryDone) {
        queryDone = true;
#if USE(GRAPHICS_SURFACE) && USE(GLX)
        Display* display = nativeDisplay();

        if (display) {
            int eventBasep, errorBasep;
            supportsXRenderExtension = XRenderQueryExtension(display, &eventBasep, &errorBasep);
        }
#endif
    }

    return supportsXRenderExtension;
}

Display* X11Helper::nativeDisplay()
{
    // Display connection will only be broken at program shutdown.
    static DisplayConnection displayConnection;
    return displayConnection.display();
}

Window X11Helper::offscreenRootWindow()
{
    static OffScreenRootWindow offscreenWindow;
    return offscreenWindow.rootWindow();
}

}
