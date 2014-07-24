/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
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

#include "config.h"
#include "PluginView.h"

#include "BridgeJSC.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "FloatPoint.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HostWindow.h"
#include "IFrameShimSupport.h"
#include "Image.h"
#include "JSDOMBinding.h"
#include "JSDOMWindowBase.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PluginDebug.h"
#include "PluginMainThreadScheduler.h"
#include "PluginPackage.h"
#include "QWebPageClient.h"
#include "RenderObject.h"
#include "Settings.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <QKeyEvent>
#include <QPainter>
#include <X11/X.h>
#ifndef QT_NO_XRENDER
#define Bool int
#define Status int
#include <X11/extensions/Xrender.h>
#endif
#include <runtime/JSCJSValue.h>
#include <runtime/JSLock.h>

#include "QtX11ImageConversion.h"
#include <QGuiApplication>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>

using JSC::ExecState;
using JSC::Interpreter;
using JSC::JSLock;
using JSC::JSObject;

using std::min;

using namespace WTF;

namespace WebCore {

bool PluginView::s_isRunningUnderDRT = false;

using namespace HTMLNames;

struct X11Environment {
    Display* display;
    int screenID;
    unsigned long rootWindowID;
    int displayDepth;
};

static X11Environment x11Environment = { 0, 0, 0, 0 };

static inline Display* x11Display() { return x11Environment.display; }
static inline int x11Screen() { return x11Environment.screenID; }
static inline unsigned long rootWindowID() { return x11Environment.rootWindowID; }
static inline int displayDepth() { return x11Environment.displayDepth; }

static inline void syncX()
{
    XSync(x11Display(), false);
}

QWebPageClient* PluginView::platformPageClient() const
{
    FrameView* view = m_parentFrame->view();
    if (!view)
        return 0;
    HostWindow* hostWindow = view->hostWindow();
    if (!hostWindow)
        return 0;
    return hostWindow->platformPageClient();
}

void PluginView::updatePluginWidget()
{
    if (!parent())
        return;

    ASSERT(parent()->isFrameView());
    FrameView* frameView = toFrameView(parent());

    IntRect oldWindowRect = m_windowRect;
    IntRect oldClipRect = m_clipRect;

    m_windowRect = IntRect(frameView->contentsToWindow(frameRect().location()), frameRect().size());
    m_clipRect = windowClipRect();
    m_clipRect.move(-m_windowRect.x(), -m_windowRect.y());

    if (m_windowRect == oldWindowRect && m_clipRect == oldClipRect)
        return;

    // The plugin had a zero width or height before but was resized, we need to show it again.
    if (oldWindowRect.isEmpty())
        show();
    if (m_windowRect.isEmpty())
        hide();

    if (!m_isWindowed && m_windowRect.size() != oldWindowRect.size()) {
        if (m_drawable) {
            XFreePixmap(x11Display(), m_drawable);
            m_drawable = 0;
        }

        if (!m_windowRect.isEmpty()) {
            m_drawable = XCreatePixmap(x11Display(), rootWindowID(), m_windowRect.width(), m_windowRect.height(),
                                       ((NPSetWindowCallbackStruct*)m_npWindow.ws_info)->depth);
            syncX(); // make sure that the server knows about the Drawable
        }
    }

    // do not call setNPWindowIfNeeded immediately, will be called on paint()
    m_hasPendingGeometryChange = true;

    // (i) in order to move/resize the plugin window at the same time as the
    // rest of frame during e.g. scrolling, we set the window geometry
    // in the paint() function, but as paint() isn't called when the
    // plugin window is outside the frame which can be caused by a
    // scroll, we need to move/resize immediately.
    // (ii) if we are running layout tests from DRT, paint() won't ever get called
    // so we need to call setNPWindowIfNeeded() if window geometry has changed
    if (!m_windowRect.intersects(frameView->frameRect())
        || (s_isRunningUnderDRT && platformPluginWidget() && (m_windowRect != oldWindowRect || m_clipRect != oldClipRect)))
        setNPWindowIfNeeded();

    if (!m_platformLayer) {
        // Make sure we get repainted afterwards. This is necessary for downward
        // scrolling to move the plugin widget properly.
        // Note that we don't invalidate the frameRect() here. This is because QWebFrame::renderRelativeCoords()
        // imitates ScrollView and adds the scroll offset back on to the rect we damage here (making the co-ordinates absolute
        // to the frame again) before passing it to FrameView.
        invalidate();
    }
}

void PluginView::setFocus(bool focused)
{
    Widget::setFocus(focused);
}

void PluginView::show()
{
    Q_ASSERT(platformPluginWidget() == platformWidget());
    Widget::show();
}

void PluginView::hide()
{
    Q_ASSERT(platformPluginWidget() == platformWidget());
    Widget::hide();
}

static void setupGraphicsExposeEvent(Pixmap drawable, const QRect& exposedRect, XEvent& xevent)
{
    memset(&xevent, 0, sizeof(XEvent));
    XGraphicsExposeEvent& exposeEvent = xevent.xgraphicsexpose;
    exposeEvent.type = GraphicsExpose;
    exposeEvent.display = x11Display();
    exposeEvent.drawable = drawable;
    exposeEvent.x = exposedRect.x();
    exposeEvent.y = exposedRect.y();
    exposeEvent.width = exposedRect.x() + exposedRect.width(); // flash bug? it thinks width is the right in transparent mode
    exposeEvent.height = exposedRect.y() + exposedRect.height(); // flash bug? it thinks height is the bottom in transparent mode
}

void PluginView::paintUsingXPixmap(QPainter* painter, const QRect &exposedRect)
{
    bool shouldSyncX = m_pluginDisplay && m_pluginDisplay != x11Display();
    XEvent xevent;

    setupGraphicsExposeEvent(m_drawable, exposedRect, xevent);
    dispatchNPEvent(xevent);

    if (shouldSyncX)
        XSync(m_pluginDisplay, false); // sync changes by plugin

    XImage* xImage = XGetImage(x11Display(), m_drawable, exposedRect.x(), exposedRect.y(),
                               exposedRect.width(), exposedRect.height(), ULONG_MAX, ZPixmap);
    painter->drawImage(QPoint(exposedRect.x(), exposedRect.y()), qimageFromXImage(xImage));
    XDestroyImage(xImage);
}

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_isStarted) {
        paintMissingPluginIcon(context, rect);
        return;
    }

    if (context->paintingDisabled())
        return;

    setNPWindowIfNeeded();

    if (m_isWindowed)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_platformLayer)
        return;
#endif

    if (!m_drawable)
        return;

    QPainter* painter = context->platformContext();
    IntRect exposedRect(rect);
    exposedRect.intersect(frameRect());
    exposedRect.move(-frameRect().x(), -frameRect().y());

    painter->translate(frameRect().x(), frameRect().y());
    paintUsingXPixmap(painter, exposedRect);
    painter->translate(-frameRect().x(), -frameRect().y());
}

// TODO: Unify across ports.
bool PluginView::dispatchNPEvent(NPEvent& event)
{
    if (!m_plugin->pluginFuncs()->event)
        return false;

    bool shouldPop = false;

    if (m_plugin->pluginFuncs()->version < NPVERS_HAS_POPUPS_ENABLED_STATE
        && (event.type == ButtonRelease || event.type == 3 /*KeyRelease*/)) {
        pushPopupsEnabledState(true);
        shouldPop = true;
    }

    PluginView::setCurrentPluginView(this);
    JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
    setCallingPlugin(true);
    bool accepted = !m_plugin->pluginFuncs()->event(m_instance, &event);
    setCallingPlugin(false);
    PluginView::setCurrentPluginView(0);

    if (shouldPop)
        popPopupsEnabledState();

    return accepted;
}

void setSharedXEventFields(XEvent* xEvent, QWebPageClient* pageClient)
{
    xEvent->xany.serial = 0; // we are unaware of the last request processed by X Server
    xEvent->xany.send_event = false;
    xEvent->xany.display = x11Display();
    // NOTE: event->xany.window doesn't always respond to the .window property of other XEvent's
    // but does in the case of KeyPress, KeyRelease, ButtonPress, ButtonRelease, and MotionNotify
    // events; thus, this is right:
    QWindow* window = pageClient ? pageClient->ownerWindow() : 0;
    xEvent->xany.window = window ? window->winId() : 0;
}

void PluginView::initXEvent(XEvent* xEvent)
{
    memset(xEvent, 0, sizeof(XEvent));

    QWebPageClient* client = platformPageClient();
    setSharedXEventFields(xEvent, client);
}

void PluginView::setXKeyEventSpecificFields(XEvent* xEvent, KeyboardEvent* event)
{
    const PlatformKeyboardEvent* keyEvent = event->keyEvent();

    xEvent->type = (event->type() == eventNames().keydownEvent) ? 2 : 3; // ints as Qt unsets KeyPress and KeyRelease
    xEvent->xkey.root = rootWindowID();
    xEvent->xkey.subwindow = 0; // we have no child window
    xEvent->xkey.time = event->timeStamp();
    xEvent->xkey.state = keyEvent->nativeModifiers();
    xEvent->xkey.keycode = keyEvent->nativeScanCode();

    // We may not have a nativeScanCode() if the key event is from DRT's eventsender. In that
    // case fetch the XEvent's keycode from the event's text. The only
    // place this keycode will be used is in webkit_test_plugin_handle_event().
    // FIXME: Create Qt API so that we can set the appropriate keycode in DRT EventSender instead.
    if (s_isRunningUnderDRT && !xEvent->xkey.keycode) {
        QKeyEvent* qKeyEvent = keyEvent->qtEvent();
        ASSERT(qKeyEvent);
        QString keyText = qKeyEvent->text().left(1);
        xEvent->xkey.keycode = XKeysymToKeycode(x11Display(), XStringToKeysym(keyText.toUtf8().constData()));
    }

    xEvent->xkey.same_screen = true;

    // NOTE: As the XEvents sent to the plug-in are synthesized and there is not a native window
    // corresponding to the plug-in rectangle, some of the members of the XEvent structures are not
    // set to their normal Xserver values. e.g. Key events don't have a position.
    // source: https://developer.mozilla.org/en/NPEvent
    xEvent->xkey.x = 0;
    xEvent->xkey.y = 0;
    xEvent->xkey.x_root = 0;
    xEvent->xkey.y_root = 0;
}

void PluginView::handleKeyboardEvent(KeyboardEvent* event)
{
    if (m_isWindowed)
        return;

    if (event->type() != eventNames().keydownEvent && event->type() != eventNames().keyupEvent)
        return;

    XEvent npEvent;
    initXEvent(&npEvent);
    setXKeyEventSpecificFields(&npEvent, event);

    if (dispatchNPEvent(npEvent))
        event->setDefaultHandled();
}

static unsigned int inputEventState(MouseEvent* event)
{
    unsigned int state = 0;
    if (event->ctrlKey())
        state |= ControlMask;
    if (event->shiftKey())
        state |= ShiftMask;
    if (event->altKey())
        state |= Mod1Mask;
    if (event->metaKey())
        state |= Mod4Mask;
    return state;
}

static void setXButtonEventSpecificFields(XEvent* xEvent, MouseEvent* event, const IntPoint& postZoomPos)
{
    XButtonEvent& xbutton = xEvent->xbutton;
    xbutton.type = event->type() == eventNames().mousedownEvent ? ButtonPress : ButtonRelease;
    xbutton.root = rootWindowID();
    xbutton.subwindow = 0;
    xbutton.time = event->timeStamp();
    xbutton.x = postZoomPos.x();
    xbutton.y = postZoomPos.y();
    xbutton.x_root = event->screenX();
    xbutton.y_root = event->screenY();
    xbutton.state = inputEventState(event);
    switch (event->button()) {
    case MiddleButton:
        xbutton.button = Button2;
        break;
    case RightButton:
        xbutton.button = Button3;
        break;
    case LeftButton:
    default:
        xbutton.button = Button1;
        break;
    }
    xbutton.same_screen = true;
}

static void setXMotionEventSpecificFields(XEvent* xEvent, MouseEvent* event, const IntPoint& postZoomPos)
{
    XMotionEvent& xmotion = xEvent->xmotion;
    xmotion.type = MotionNotify;
    xmotion.root = rootWindowID();
    xmotion.subwindow = 0;
    xmotion.time = event->timeStamp();
    xmotion.x = postZoomPos.x();
    xmotion.y = postZoomPos.y();
    xmotion.x_root = event->screenX();
    xmotion.y_root = event->screenY();
    xmotion.state = inputEventState(event);
    xmotion.is_hint = NotifyNormal;
    xmotion.same_screen = true;
}

static void setXCrossingEventSpecificFields(XEvent* xEvent, MouseEvent* event, const IntPoint& postZoomPos)
{
    XCrossingEvent& xcrossing = xEvent->xcrossing;
    xcrossing.type = event->type() == eventNames().mouseoverEvent ? EnterNotify : LeaveNotify;
    xcrossing.root = rootWindowID();
    xcrossing.subwindow = 0;
    xcrossing.time = event->timeStamp();
    xcrossing.x = postZoomPos.y();
    xcrossing.y = postZoomPos.x();
    xcrossing.x_root = event->screenX();
    xcrossing.y_root = event->screenY();
    xcrossing.state = inputEventState(event);
    xcrossing.mode = NotifyNormal;
    xcrossing.detail = NotifyDetailNone;
    xcrossing.same_screen = true;
    xcrossing.focus = false;
}

void PluginView::handleMouseEvent(MouseEvent* event)
{
    if (m_isWindowed)
        return;

    if (event->button() == RightButton && m_plugin->quirks().contains(PluginQuirkIgnoreRightClickInWindowlessMode))
        return;

    if (event->type() == eventNames().mousedownEvent) {
        // Give focus to the plugin on click
        if (Page* page = m_parentFrame->page())
            page->focusController()->setActive(true);

        focusPluginElement();
    }

    XEvent npEvent;
    initXEvent(&npEvent);

    IntPoint postZoomPos = roundedIntPoint(m_element->renderer()->absoluteToLocal(event->absoluteLocation()));

    if (event->type() == eventNames().mousedownEvent || event->type() == eventNames().mouseupEvent)
        setXButtonEventSpecificFields(&npEvent, event, postZoomPos);
    else if (event->type() == eventNames().mousemoveEvent)
        setXMotionEventSpecificFields(&npEvent, event, postZoomPos);
    else if (event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mouseoverEvent)
        setXCrossingEventSpecificFields(&npEvent, event, postZoomPos);
    else
        return;

    if (dispatchNPEvent(npEvent))
        event->setDefaultHandled();
}

void PluginView::handleFocusInEvent()
{
    XEvent npEvent;
    initXEvent(&npEvent);

    XFocusChangeEvent& event = npEvent.xfocus;
    event.type = 9; /* int as Qt unsets FocusIn */
    event.mode = NotifyNormal;
    event.detail = NotifyDetailNone;

    dispatchNPEvent(npEvent);
}

void PluginView::handleFocusOutEvent()
{
    XEvent npEvent;
    initXEvent(&npEvent);

    XFocusChangeEvent& event = npEvent.xfocus;
    event.type = 10; /* int as Qt unsets FocusOut */
    event.mode = NotifyNormal;
    event.detail = NotifyDetailNone;

    dispatchNPEvent(npEvent);
}

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent)
        init();
}

void PluginView::setNPWindowRect(const IntRect&)
{
    if (!m_isWindowed)
        setNPWindowIfNeeded();
}

void PluginView::setNPWindowIfNeeded()
{
    if (!m_isStarted || !parent() || !m_plugin->pluginFuncs()->setwindow)
        return;

    // If the plugin didn't load sucessfully, no point in calling setwindow
    if (m_status != PluginStatusLoadedSuccessfully)
        return;

    // On Unix, only call plugin if it's full-page or windowed
    if (m_mode != NP_FULL && m_mode != NP_EMBED)
        return;

    // Check if the platformPluginWidget still exists
    if (m_isWindowed && !platformPluginWidget())
        return;

    if (!m_hasPendingGeometryChange)
        return;
    m_hasPendingGeometryChange = false;

    m_npWindow.x = 0;
    m_npWindow.y = 0;

    // If the width or height are null, set the clipRect to null, indicating that
    // the plugin is not visible/scrolled out.
    if (!m_clipRect.width() || !m_clipRect.height()) {
        m_npWindow.clipRect.left = 0;
        m_npWindow.clipRect.right = 0;
        m_npWindow.clipRect.top = 0;
        m_npWindow.clipRect.bottom = 0;
    } else {
        // Clipping rectangle of the plug-in; the origin is the top left corner of the drawable or window. 
        m_npWindow.clipRect.left = m_npWindow.x + m_clipRect.x();
        m_npWindow.clipRect.top = m_npWindow.y + m_clipRect.y();
        m_npWindow.clipRect.right = m_npWindow.x + m_clipRect.x() + m_clipRect.width();
        m_npWindow.clipRect.bottom = m_npWindow.y + m_clipRect.y() + m_clipRect.height();
    }

    if (m_plugin->quirks().contains(PluginQuirkDontCallSetWindowMoreThanOnce)) {
        // FLASH WORKAROUND: Only set initially. Multiple calls to
        // setNPWindow() cause the plugin to crash in windowed mode.
        if (!m_isWindowed || m_npWindow.width == -1 || m_npWindow.height == -1) {
            m_npWindow.width = m_windowRect.width();
            m_npWindow.height = m_windowRect.height();
        }
    } else {
        m_npWindow.width = m_windowRect.width();
        m_npWindow.height = m_windowRect.height();
    }

    PluginView::setCurrentPluginView(this);
    JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
    setCallingPlugin(true);
    m_plugin->pluginFuncs()->setwindow(m_instance, &m_npWindow);
    setCallingPlugin(false);
    PluginView::setCurrentPluginView(0);
}

void PluginView::setParentVisible(bool visible)
{
    if (isParentVisible() == visible)
        return;

    Widget::setParentVisible(visible);
}

NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    String filename(buf, len);

    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    long long size;
    if (!getFileSize(filename, size))
        return NPERR_FILE_NOT_FOUND;

    FILE* fileHandle = fopen((filename.utf8()).data(), "r");
    if (!fileHandle)
        return NPERR_FILE_NOT_FOUND;

    buffer.resize(size);
    int bytesRead = fread(buffer.data(), 1, size, fileHandle);

    fclose(fileHandle);

    if (bytesRead <= 0)
        return NPERR_FILE_NOT_FOUND;

    return NPERR_NO_ERROR;
}

bool PluginView::platformGetValueStatic(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVToolkit:
        *static_cast<uint32_t*>(value) = 0;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsXEmbedBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsWindowless:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    default:
        return false;
    }
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVxDisplay:
        *reinterpret_cast<void**>(value) = x11Display();
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVxtAppContext:
        *result = NPERR_GENERIC_ERROR;
        return true;

    case NPNVnetscapeWindow: {
        QWebPageClient* client = platformPageClient();
        QWindow* window = client ? client->ownerWindow() : 0;
        *reinterpret_cast<XID*>(value) = window ? window->winId() : 0;
        *result = NPERR_NO_ERROR;
        return true;
    }

    case NPNVToolkit:
        if (m_plugin->quirks().contains(PluginQuirkRequiresGtkToolKit)) {
            *((uint32_t *)value) = 2;
            *result = NPERR_NO_ERROR;
            return true;
        }
        return false;

    default:
        return false;
    }
}

void PluginView::invalidateRect(const IntRect& rect)
{
    invalidateWindowlessPluginRect(rect);
}

void PluginView::invalidateRect(NPRect* rect)
{
    if (!rect) {
        invalidate();
        return;
    }
    IntRect r(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
    invalidateRect(r);
}

void PluginView::invalidateRegion(NPRegion region)
{
    Q_UNUSED(region);
    invalidate();
}

void PluginView::forceRedraw()
{
    invalidate();
}

static Display *getPluginDisplay()
{
    // The plugin toolkit might run using a different X connection. At the moment, we only
    // support gdk based plugins (like flash) that use a different X connection.
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
}

static bool getVisualAndColormap(int depth, Visual*& visual, Colormap& colormap, bool forceARGB32)
{
    ASSERT(depth == 32 || !forceARGB32);

    visual = 0;
    colormap = 0;

    if (forceARGB32)
        return false;

    int nvi;
    XVisualInfo templ;
    templ.screen  = x11Screen();
    templ.depth   = depth;
    templ.c_class = TrueColor;
    XVisualInfo* xvi = XGetVisualInfo(x11Display(), VisualScreenMask | VisualDepthMask | VisualClassMask, &templ, &nvi);
    ASSERT(xvi || forceARGB32);
    if (!xvi)
        return false;

    visual = xvi[0].visual;
    ASSERT(visual);

    XFree(xvi);

    colormap = XCreateColormap(x11Display(), rootWindowID(), visual, AllocNone);
    return true;
}

bool PluginView::platformStart()
{
    ASSERT(m_isStarted);
    ASSERT(m_status == PluginStatusLoadedSuccessfully);

    if (!x11Environment.display) {
        Display* display;
        display = static_cast<Display*>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("display", 0));
        x11Environment.display = display;
        x11Environment.screenID = XDefaultScreen(display);
        x11Environment.displayDepth = XDefaultDepth(display, x11Environment.screenID);
        x11Environment.rootWindowID = XDefaultRootWindow(display);
    }

    // Windowed mode is not supported with Qt5 yet.
    if (m_isWindowed)
        return false;
    setPlatformWidget(0);
    m_pluginDisplay = getPluginDisplay();

    // If the width and the height are not zero we show the PluginView.
    if (!frameRect().isEmpty())
        show();

    NPSetWindowCallbackStruct* wsi = new NPSetWindowCallbackStruct();
    wsi->type = 0;

    int depth = displayDepth();
    bool found = getVisualAndColormap(depth, m_visual, m_colormap, /* forceARGB32 = */ false);
    ASSERT_UNUSED(found, found);
    wsi->depth = depth;

    wsi->display = x11Display();
    wsi->visual = m_visual;
    wsi->colormap = m_colormap;

    m_npWindow.type = NPWindowTypeDrawable;
    m_npWindow.window = 0; // Not used?
    m_npWindow.x = 0;
    m_npWindow.y = 0;
    m_npWindow.width = -1;
    m_npWindow.height = -1;

    m_npWindow.ws_info = wsi;

    if (!(m_plugin->quirks().contains(PluginQuirkDeferFirstSetWindowCall))) {
        updatePluginWidget();
        setNPWindowIfNeeded();
    }

    return true;
}

void PluginView::platformDestroy()
{
    if (platformPluginWidget())
        delete platformPluginWidget();

    if (m_drawable)
        XFreePixmap(x11Display(), m_drawable);

    if (m_colormap)
        XFreeColormap(x11Display(), m_colormap);
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* PluginView::platformLayer() const
{
    return m_platformLayer.get();
}
#endif

} // namespace WebCore
