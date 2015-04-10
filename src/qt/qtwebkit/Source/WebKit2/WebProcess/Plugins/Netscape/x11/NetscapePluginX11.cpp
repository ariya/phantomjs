/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged
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
#if PLUGIN_ARCHITECTURE(X11) && ENABLE(NETSCAPE_PLUGIN_API)

#include "NetscapePlugin.h"

#include "PluginController.h"
#include "WebEvent.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/NotImplemented.h>

#if PLATFORM(QT)
#include <WebCore/QtX11ImageConversion.h>
#elif PLATFORM(GTK)
#include <gtk/gtk.h>
#ifndef GTK_API_VERSION_2
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkx.h>
#include <WebCore/GtkVersioning.h>
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
#include <Ecore_X.h>
#endif

#if USE(CAIRO)
#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo/cairo-xlib.h>
#endif

using namespace WebCore;

namespace WebKit {

static Display* getPluginDisplay()
{
#if PLATFORM(QT)
    // At the moment, we only support gdk based plugins (like Flash) that use a different X connection.
    // The code below has the same effect as this one:
    // Display *gdkDisplay = gdk_x11_display_get_xdisplay(gdk_display_get_default());

    QLibrary library(QLatin1String("libgdk-x11-2.0"), 0);
    if (!library.load())
        return 0;

    typedef void *(*gdk_init_check_ptr)(void*, void*);
    gdk_init_check_ptr gdk_init_check = (gdk_init_check_ptr)library.resolve("gdk_init_check");
    if (!gdk_init_check)
        return 0;

    typedef void *(*gdk_display_get_default_ptr)();
    gdk_display_get_default_ptr gdk_display_get_default = (gdk_display_get_default_ptr)library.resolve("gdk_display_get_default");
    if (!gdk_display_get_default)
        return 0;

    typedef void *(*gdk_x11_display_get_xdisplay_ptr)(void *);
    gdk_x11_display_get_xdisplay_ptr gdk_x11_display_get_xdisplay = (gdk_x11_display_get_xdisplay_ptr)library.resolve("gdk_x11_display_get_xdisplay");
    if (!gdk_x11_display_get_xdisplay)
        return 0;

    gdk_init_check(0, 0);
    return (Display*)gdk_x11_display_get_xdisplay(gdk_display_get_default());
#elif PLATFORM(GTK)
    // Since we're a gdk/gtk app, we'll (probably?) have the same X connection as any gdk-based
    // plugins, so we can return that. We might want to add other implementations here later.
    return GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
    return static_cast<Display*>(ecore_x_display_get());
#else
    return 0;
#endif
}

static inline int x11Screen()
{
#if PLATFORM(QT)
    return XDefaultScreen(NetscapePlugin::x11HostDisplay());
#elif PLATFORM(GTK)
    return gdk_screen_get_number(gdk_screen_get_default());
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
    return ecore_x_screen_index_get(ecore_x_default_screen_get());
#else
    return 0;
#endif
}

static inline int displayDepth()
{
#if PLATFORM(QT)
    return XDefaultDepth(NetscapePlugin::x11HostDisplay(), x11Screen());
#elif PLATFORM(GTK)
    return gdk_visual_get_depth(gdk_screen_get_system_visual(gdk_screen_get_default()));
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
    return ecore_x_default_depth_get(NetscapePlugin::x11HostDisplay(), ecore_x_default_screen_get());
#else
    return 0;
#endif
}

static inline unsigned long rootWindowID()
{
#if PLATFORM(QT)
    return XDefaultRootWindow(NetscapePlugin::x11HostDisplay());
#elif PLATFORM(GTK)
    return GDK_ROOT_WINDOW();
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
    return ecore_x_window_root_first_get();
#else
    return 0;
#endif
}

#if PLATFORM(GTK)
static bool moduleMixesGtkSymbols(Module* module)
{
#ifdef GTK_API_VERSION_2
    return module->functionPointer<gpointer>("gtk_application_get_type");
#else
    return module->functionPointer<gpointer>("gtk_object_get_type");
#endif
}
#endif

Display* NetscapePlugin::x11HostDisplay()
{
#if PLATFORM(QT)
    static Display* dedicatedDisplay = 0;
    if (!dedicatedDisplay)
        dedicatedDisplay = XOpenDisplay(0);

    ASSERT(dedicatedDisplay);
    return dedicatedDisplay;
#elif PLATFORM(GTK)
    return GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
#elif PLATFORM(EFL) && defined(HAVE_ECORE_X)
    return static_cast<Display*>(ecore_x_display_get());
#else
    return 0;
#endif
}

#if PLATFORM(GTK)
static gboolean socketPlugRemovedCallback(GtkSocket*)
{
    // Default action is to destroy the GtkSocket, so we just return TRUE here
    // to be able to reuse the socket. For some obscure reason, newer versions
    // of flash plugin remove the plug from the socket, probably because the plug
    // created by the plugin is re-parented.
    return TRUE;
}
#endif

bool NetscapePlugin::platformPostInitializeWindowed(bool needsXEmbed, uint64_t windowID)
{
    m_npWindow.type = NPWindowTypeWindow;
    if (!needsXEmbed) {
        notImplemented();
        return false;
    }

    Display* display = x11HostDisplay();

#if PLATFORM(GTK)
    // It seems flash needs the socket to be in the same process,
    // I guess it uses gdk_window_lookup(), so we create a new socket here
    // containing a plug with the UI process socket embedded.
    m_platformPluginWidget = gtk_plug_new(static_cast<Window>(windowID));
    GtkWidget* socket = gtk_socket_new();
    g_signal_connect(socket, "plug-removed", G_CALLBACK(socketPlugRemovedCallback), 0);
    gtk_container_add(GTK_CONTAINER(m_platformPluginWidget), socket);
    gtk_widget_show(socket);
    gtk_widget_show(m_platformPluginWidget);

    m_npWindow.window = GINT_TO_POINTER(gtk_socket_get_id(GTK_SOCKET(socket)));
    GdkWindow* window = gtk_widget_get_window(socket);
    NPSetWindowCallbackStruct* callbackStruct = static_cast<NPSetWindowCallbackStruct*>(m_npWindow.ws_info);
    callbackStruct->display = GDK_WINDOW_XDISPLAY(window);
    callbackStruct->visual = GDK_VISUAL_XVISUAL(gdk_window_get_visual(window));
    callbackStruct->depth = gdk_visual_get_depth(gdk_window_get_visual(window));
    callbackStruct->colormap = XCreateColormap(display, GDK_ROOT_WINDOW(), callbackStruct->visual, AllocNone);
#else
    UNUSED_PARAM(windowID);
#endif

    XFlush(display);

    callSetWindow();

    return true;
}

bool NetscapePlugin::platformPostInitializeWindowless()
{
    Display* display = x11HostDisplay();
    m_npWindow.type = NPWindowTypeDrawable;
    m_npWindow.window = 0;

    int depth = displayDepth();
#if PLATFORM(QT)
    ASSERT(depth == 16 || depth == 24 || depth == 32);
#endif
    NPSetWindowCallbackStruct* callbackStruct = static_cast<NPSetWindowCallbackStruct*>(m_npWindow.ws_info);
    callbackStruct->display = display;
    callbackStruct->depth = depth;

    XVisualInfo visualTemplate;
    visualTemplate.screen = x11Screen();
    visualTemplate.depth = depth;
    visualTemplate.c_class = TrueColor;
    int numMatching;
    XVisualInfo* visualInfo = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask | VisualClassMask,
                                             &visualTemplate, &numMatching);
    ASSERT(visualInfo);
    Visual* visual = visualInfo[0].visual;
    ASSERT(visual);
    XFree(visualInfo);

    callbackStruct->visual = visual;
    callbackStruct->colormap = XCreateColormap(display, rootWindowID(), visual, AllocNone);

    callSetWindow();

    return true;
}

void NetscapePlugin::platformPreInitialize()
{
}

bool NetscapePlugin::platformPostInitialize()
{
#if PLATFORM(GTK)
    if (moduleMixesGtkSymbols(m_pluginModule->module()))
        return false;
#endif

    uint64_t windowID = 0;
    bool needsXEmbed = false;
    if (m_isWindowed) {
        NPP_GetValue(NPPVpluginNeedsXEmbed, &needsXEmbed);
        if (needsXEmbed) {
            windowID = controller()->createPluginContainer();
            if (!windowID)
                return false;
        } else {
            notImplemented();
            return false;
        }
    }

    if (!(m_pluginDisplay = getPluginDisplay()))
        return false;

    NPSetWindowCallbackStruct* callbackStruct = new NPSetWindowCallbackStruct;
    callbackStruct->type = 0;
    m_npWindow.ws_info = callbackStruct;

    if (m_isWindowed)
        return platformPostInitializeWindowed(needsXEmbed, windowID);

    return platformPostInitializeWindowless();
}

void NetscapePlugin::platformDestroy()
{
    NPSetWindowCallbackStruct* callbackStruct = static_cast<NPSetWindowCallbackStruct*>(m_npWindow.ws_info);
    Display* hostDisplay = x11HostDisplay();
    XFreeColormap(hostDisplay, callbackStruct->colormap);
    delete callbackStruct;

    if (m_drawable) {
        XFreePixmap(hostDisplay, m_drawable);
        m_drawable = 0;
    }
}

bool NetscapePlugin::platformInvalidate(const IntRect&)
{
    notImplemented();
    return false;
}

void NetscapePlugin::platformGeometryDidChange()
{
    if (m_isWindowed) {
        uint64_t windowID = 0;
#if PLATFORM(GTK)
        windowID = static_cast<uint64_t>(GDK_WINDOW_XID(gtk_plug_get_socket_window(GTK_PLUG(m_platformPluginWidget))));
#endif
        controller()->windowedPluginGeometryDidChange(m_frameRectInWindowCoordinates, m_clipRect, windowID);
        return;
    }

    Display* display = x11HostDisplay();
    if (m_drawable)
        XFreePixmap(display, m_drawable);

    if (m_pluginSize.isEmpty()) {
        m_drawable = 0;
        return;
    }

    m_drawable = XCreatePixmap(display, rootWindowID(), m_pluginSize.width(), m_pluginSize.height(), displayDepth());

    XSync(display, false); // Make sure that the server knows about the Drawable.
}

void NetscapePlugin::platformVisibilityDidChange()
{
    notImplemented();
}

void NetscapePlugin::platformPaint(GraphicsContext* context, const IntRect& dirtyRect, bool /*isSnapshot*/)
{
    if (m_isWindowed)
        return;

    if (!m_isStarted) {
        // FIXME: we should paint a missing plugin icon.
        return;
    }

    if (context->paintingDisabled() || !m_drawable)
        return;

    XEvent xevent;
    memset(&xevent, 0, sizeof(XEvent));
    XGraphicsExposeEvent& exposeEvent = xevent.xgraphicsexpose;
    exposeEvent.type = GraphicsExpose;
    exposeEvent.display = x11HostDisplay();
    exposeEvent.drawable = m_drawable;

    IntRect exposedRect(dirtyRect);
    exposeEvent.x = exposedRect.x();
    exposeEvent.y = exposedRect.y();

    // Note: in transparent mode Flash thinks width is the right and height is the bottom.
    // We should take it into account if we want to support transparency.
    exposeEvent.width = exposedRect.width();
    exposeEvent.height = exposedRect.height();

    NPP_HandleEvent(&xevent);

    if (m_pluginDisplay != x11HostDisplay())
        XSync(m_pluginDisplay, false);

#if PLATFORM(QT)
    XImage* xImage = XGetImage(NetscapePlugin::x11HostDisplay(), m_drawable, exposedRect.x(), exposedRect.y(),
                               exposedRect.width(), exposedRect.height(), ULONG_MAX, ZPixmap);
    QPainter* painter = context->platformContext();
    painter->drawImage(QPoint(exposedRect.x(), exposedRect.y()), qimageFromXImage(xImage), exposedRect);

    XDestroyImage(xImage);
#elif PLATFORM(GTK) || (PLATFORM(EFL) && USE(CAIRO))
    RefPtr<cairo_surface_t> drawableSurface = adoptRef(cairo_xlib_surface_create(m_pluginDisplay,
                                                                                 m_drawable,
                                                                                 static_cast<NPSetWindowCallbackStruct*>(m_npWindow.ws_info)->visual,
                                                                                 m_pluginSize.width(),
                                                                                 m_pluginSize.height()));
    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);

    cairo_set_source_surface(cr, drawableSurface.get(), 0, 0);

    cairo_rectangle(cr, exposedRect.x(), exposedRect.y(), exposedRect.width(), exposedRect.height());
    cairo_clip(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);

    cairo_restore(cr);
#else
    notImplemented();
#endif
}

static inline void initializeXEvent(XEvent& event)
{
    memset(&event, 0, sizeof(XEvent));
    event.xany.serial = 0;
    event.xany.send_event = false;
    event.xany.display = NetscapePlugin::x11HostDisplay();
    event.xany.window = 0;
}

static inline uint64_t xTimeStamp(double timestampInSeconds)
{
    return timestampInSeconds * 1000;
}

static inline unsigned xKeyModifiers(const WebEvent& event)
{
    unsigned xModifiers = 0;
    if (event.controlKey())
        xModifiers |= ControlMask;
    if (event.shiftKey())
        xModifiers |= ShiftMask;
    if (event.altKey())
        xModifiers |= Mod1Mask;
    if (event.metaKey())
        xModifiers |= Mod4Mask;

    return xModifiers;
}

template <typename XEventType, typename WebEventType>
static inline void setCommonMouseEventFields(XEventType& xEvent, const WebEventType& webEvent, const WebCore::IntPoint& pluginLocation)
{
    xEvent.root = rootWindowID();
    xEvent.subwindow = 0;
    xEvent.time = xTimeStamp(webEvent.timestamp());
    xEvent.x = webEvent.position().x() - pluginLocation.x();
    xEvent.y = webEvent.position().y() - pluginLocation.y();
    xEvent.x_root = webEvent.globalPosition().x();
    xEvent.y_root = webEvent.globalPosition().y();
    xEvent.state = xKeyModifiers(webEvent);
    xEvent.same_screen = true;
}

static inline void setXMotionEventFields(XEvent& xEvent, const WebMouseEvent& webEvent, const WebCore::IntPoint& pluginLocation)
{
    XMotionEvent& xMotion = xEvent.xmotion;
    setCommonMouseEventFields(xMotion, webEvent, pluginLocation);
    xMotion.type = MotionNotify;
}

static inline void setXButtonEventFields(XEvent& xEvent, const WebMouseEvent& webEvent, const WebCore::IntPoint& pluginLocation)
{
    XButtonEvent& xButton = xEvent.xbutton;
    setCommonMouseEventFields(xButton, webEvent, pluginLocation);

    xButton.type = (webEvent.type() == WebEvent::MouseDown) ? ButtonPress : ButtonRelease;
    switch (webEvent.button()) {
    case WebMouseEvent::LeftButton:
        xButton.button = Button1;
        break;
    case WebMouseEvent::MiddleButton:
        xButton.button = Button2;
        break;
    case WebMouseEvent::RightButton:
        xButton.button = Button3;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

static inline void setXButtonEventFieldsByWebWheelEvent(XEvent& xEvent, const WebWheelEvent& webEvent, const WebCore::IntPoint& pluginLocation)
{
    XButtonEvent& xButton = xEvent.xbutton;
    setCommonMouseEventFields(xButton, webEvent, pluginLocation);

    xButton.type = ButtonPress;
    FloatSize ticks = webEvent.wheelTicks();
    if (ticks.height()) {
        if (ticks.height() > 0)
            xButton.button = 4; // up
        else
            xButton.button = 5; // down
    } else {
        if (ticks.width() > 0)
            xButton.button = 6; // left
        else
            xButton.button = 7; // right
    }
}

static inline void setXCrossingEventFields(XEvent& xEvent, const WebMouseEvent& webEvent, const WebCore::IntPoint& pluginLocation, int type)
{
    XCrossingEvent& xCrossing = xEvent.xcrossing;
    setCommonMouseEventFields(xCrossing, webEvent, pluginLocation);

    xCrossing.type = type;
    xCrossing.mode = NotifyNormal;
    xCrossing.detail = NotifyDetailNone;
    xCrossing.focus = false;
}

bool NetscapePlugin::platformHandleMouseEvent(const WebMouseEvent& event)
{
    if (m_isWindowed)
        return false;

    if ((event.type() == WebEvent::MouseDown || event.type() == WebEvent::MouseUp)
         && event.button() == WebMouseEvent::RightButton
         && quirks().contains(PluginQuirks::IgnoreRightClickInWindowlessMode))
        return false;

    XEvent xEvent;
    initializeXEvent(xEvent);

    switch (event.type()) {
    case WebEvent::MouseDown:
    case WebEvent::MouseUp:
        setXButtonEventFields(xEvent, event, convertToRootView(IntPoint()));
        break;
    case WebEvent::MouseMove:
        setXMotionEventFields(xEvent, event, convertToRootView(IntPoint()));
        break;
    case WebEvent::NoType:
    case WebEvent::Wheel:
    case WebEvent::KeyDown:
    case WebEvent::KeyUp:
    case WebEvent::RawKeyDown:
    case WebEvent::Char:
#if ENABLE(GESTURE_EVENTS)
    case WebEvent::GestureScrollBegin:
    case WebEvent::GestureScrollEnd:
    case WebEvent::GestureSingleTap:
#endif
#if ENABLE(TOUCH_EVENTS)
    case WebEvent::TouchStart:
    case WebEvent::TouchMove:
    case WebEvent::TouchEnd:
    case WebEvent::TouchCancel:
#endif
        return false;
    }

    return !NPP_HandleEvent(&xEvent);
}

// We undefine these constants in npruntime_internal.h to avoid collision
// with WebKit and platform headers. Values are defined in X.h.
const int kKeyPressType = 2;
const int kKeyReleaseType = 3;
const int kFocusInType = 9;
const int kFocusOutType = 10;

bool NetscapePlugin::platformHandleWheelEvent(const WebWheelEvent& event)
{
    if (m_isWindowed)
        return false;

    XEvent xEvent;
    initializeXEvent(xEvent);
    setXButtonEventFieldsByWebWheelEvent(xEvent, event, convertToRootView(IntPoint()));

    return !NPP_HandleEvent(&xEvent);
}

void NetscapePlugin::platformSetFocus(bool focusIn)
{
    if (m_isWindowed)
        return;

    XEvent xEvent;
    initializeXEvent(xEvent);
    XFocusChangeEvent& focusEvent = xEvent.xfocus;
    focusEvent.type = focusIn ? kFocusInType : kFocusOutType;
    focusEvent.mode = NotifyNormal;
    focusEvent.detail = NotifyDetailNone;

    NPP_HandleEvent(&xEvent);
}

bool NetscapePlugin::wantsPluginRelativeNPWindowCoordinates()
{
    return true;
}

bool NetscapePlugin::platformHandleMouseEnterEvent(const WebMouseEvent& event)
{
    if (m_isWindowed)
        return false;

    XEvent xEvent;
    initializeXEvent(xEvent);
    setXCrossingEventFields(xEvent, event, convertToRootView(IntPoint()), EnterNotify);

    return !NPP_HandleEvent(&xEvent);
}

bool NetscapePlugin::platformHandleMouseLeaveEvent(const WebMouseEvent& event)
{
    if (m_isWindowed)
        return false;

    XEvent xEvent;
    initializeXEvent(xEvent);
    setXCrossingEventFields(xEvent, event, convertToRootView(IntPoint()), LeaveNotify);

    return !NPP_HandleEvent(&xEvent);
}

static inline void setXKeyEventFields(XEvent& xEvent, const WebKeyboardEvent& webEvent)
{
    xEvent.xany.type = (webEvent.type() == WebEvent::KeyDown) ? kKeyPressType : kKeyReleaseType;
    XKeyEvent& xKey = xEvent.xkey;
    xKey.root = rootWindowID();
    xKey.subwindow = 0;
    xKey.time = xTimeStamp(webEvent.timestamp());
    xKey.state = xKeyModifiers(webEvent);
    xKey.keycode = webEvent.nativeVirtualKeyCode();

    xKey.same_screen = true;

    // Key events propagated to the plugin does not need to have position.
    // source: https://developer.mozilla.org/en/NPEvent
    xKey.x = 0;
    xKey.y = 0;
    xKey.x_root = 0;
    xKey.y_root = 0;
}

bool NetscapePlugin::platformHandleKeyboardEvent(const WebKeyboardEvent& event)
{
    // We don't generate other types of keyboard events via WebEventFactory.
    ASSERT(event.type() == WebEvent::KeyDown || event.type() == WebEvent::KeyUp);

    XEvent xEvent;
    initializeXEvent(xEvent);
    setXKeyEventFields(xEvent, event);

    return !NPP_HandleEvent(&xEvent);
}

} // namespace WebKit

#endif // PLUGIN_ARCHITECTURE(X11) && ENABLE(NETSCAPE_PLUGIN_API)
