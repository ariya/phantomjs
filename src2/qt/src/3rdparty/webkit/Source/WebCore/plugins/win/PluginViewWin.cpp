/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc. All rights reserved.
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

#include "BitmapImage.h"
#include "BridgeJSC.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "EventNames.h"
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
#include "Image.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "KeyboardEvent.h"
#include "LocalWindowsContext.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PluginDatabase.h"
#include "PluginDebug.h"
#include "PluginMainThreadScheduler.h"
#include "PluginMessageThrottlerWin.h"
#include "PluginPackage.h"
#include "RenderWidget.h"
#include "Settings.h"
#include "WebCoreInstanceHandle.h"
#include "c_instance.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <runtime/JSLock.h>
#include <runtime/JSValue.h>
#include <wtf/ASCIICType.h>

#if !PLATFORM(WX)
#include "BitmapInfo.h"
#endif

#if OS(WINCE)
#undef LOG_NPERROR
#define LOG_NPERROR(x)
#undef LOG_PLUGIN_NET_ERROR
#define LOG_PLUGIN_NET_ERROR()
#endif

#if USE(CAIRO)
#include "PlatformContextCairo.h"
#include <cairo-win32.h>
#endif

#if PLATFORM(QT)
#include "QWebPageClient.h"
#include <QWidget>
#endif

#if PLATFORM(WX)
#include <wx/defs.h>
#include <wx/window.h>
#endif

static inline HWND windowHandleForPageClient(PlatformPageClient client)
{
#if PLATFORM(QT)
    if (!client)
        return 0;
    if (QWidget* pluginParent = qobject_cast<QWidget*>(client->pluginParent()))
        return pluginParent->winId();
    return 0;
#elif PLATFORM(WX)
    if (!client)
        return 0;
    return (HWND)client->GetHandle();
#else
    return client;
#endif
}

using JSC::ExecState;
using JSC::JSLock;
using JSC::JSObject;
using JSC::UString;

using std::min;

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

const LPCWSTR kWebPluginViewdowClassName = L"WebPluginView";
const LPCWSTR kWebPluginViewProperty = L"WebPluginViewProperty";

#if !OS(WINCE)
// The code used to hook BeginPaint/EndPaint originally came from
// <http://www.fengyuan.com/article/wmprint.html>.
// Copyright (C) 2000 by Feng Yuan (www.fengyuan.com).

static unsigned beginPaintSysCall;
static BYTE* beginPaint;

static unsigned endPaintSysCall;
static BYTE* endPaint;

typedef HDC (WINAPI *PtrBeginPaint)(HWND, PAINTSTRUCT*);
typedef BOOL (WINAPI *PtrEndPaint)(HWND, const PAINTSTRUCT*);

#if OS(WINDOWS) && CPU(X86_64) && COMPILER(MSVC)
extern "C" HDC __stdcall _HBeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
extern "C" BOOL __stdcall _HEndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
#endif

HDC WINAPI PluginView::hookedBeginPaint(HWND hWnd, PAINTSTRUCT* lpPaint)
{
    PluginView* pluginView = reinterpret_cast<PluginView*>(GetProp(hWnd, kWebPluginViewProperty));
    if (pluginView && pluginView->m_wmPrintHDC) {
        // We're secretly handling WM_PRINTCLIENT, so set up the PAINTSTRUCT so
        // that the plugin will paint into the HDC we provide.
        memset(lpPaint, 0, sizeof(PAINTSTRUCT));
        lpPaint->hdc = pluginView->m_wmPrintHDC;
        GetClientRect(hWnd, &lpPaint->rcPaint);
        return pluginView->m_wmPrintHDC;
    }

#if COMPILER(GCC)
    HDC result;
    asm ("push    %2\n"
         "push    %3\n"
         "call    *%4\n"
         : "=a" (result)
         : "a" (beginPaintSysCall), "g" (lpPaint), "g" (hWnd), "m" (beginPaint)
         : "memory"
        );
    return result;
#elif defined(_M_IX86)
    // Call through to the original BeginPaint.
    __asm   mov     eax, beginPaintSysCall
    __asm   push    lpPaint
    __asm   push    hWnd
    __asm   call    beginPaint
#else
    return _HBeginPaint(hWnd, lpPaint);
#endif
}

BOOL WINAPI PluginView::hookedEndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint)
{
    PluginView* pluginView = reinterpret_cast<PluginView*>(GetProp(hWnd, kWebPluginViewProperty));
    if (pluginView && pluginView->m_wmPrintHDC) {
        // We're secretly handling WM_PRINTCLIENT, so we don't have to do any
        // cleanup.
        return TRUE;
    }

#if COMPILER(GCC)
    BOOL result;
    asm ("push   %2\n"
         "push   %3\n"
         "call   *%4\n"
         : "=a" (result)
         : "a" (endPaintSysCall), "g" (lpPaint), "g" (hWnd), "m" (endPaint)
        );
    return result;
#elif defined (_M_IX86)
    // Call through to the original EndPaint.
    __asm   mov     eax, endPaintSysCall
    __asm   push    lpPaint
    __asm   push    hWnd
    __asm   call    endPaint
#else
    return _HEndPaint(hWnd, lpPaint);
#endif
}

static void hook(const char* module, const char* proc, unsigned& sysCallID, BYTE*& pProc, const void* pNewProc)
{
    // See <http://www.fengyuan.com/article/wmprint.html> for an explanation of
    // how this function works.

    HINSTANCE hMod = GetModuleHandleA(module);

    pProc = reinterpret_cast<BYTE*>(reinterpret_cast<ptrdiff_t>(GetProcAddress(hMod, proc)));

#if COMPILER(GCC) || defined(_M_IX86)
    if (pProc[0] != 0xB8)
        return;

    // FIXME: Should we be reading the bytes one-by-one instead of doing an
    // unaligned read?
    sysCallID = *reinterpret_cast<unsigned*>(pProc + 1);

    DWORD flOldProtect;
    if (!VirtualProtect(pProc, 5, PAGE_EXECUTE_READWRITE, &flOldProtect))
        return;

    pProc[0] = 0xE9;
    *reinterpret_cast<unsigned*>(pProc + 1) = reinterpret_cast<intptr_t>(pNewProc) - reinterpret_cast<intptr_t>(pProc + 5);

    pProc += 5;
#else
    /* Disassembly of BeginPaint()
    00000000779FC5B0 4C 8B D1         mov         r10,rcx
    00000000779FC5B3 B8 17 10 00 00   mov         eax,1017h
    00000000779FC5B8 0F 05            syscall
    00000000779FC5BA C3               ret
    00000000779FC5BB 90               nop
    00000000779FC5BC 90               nop
    00000000779FC5BD 90               nop
    00000000779FC5BE 90               nop
    00000000779FC5BF 90               nop
    00000000779FC5C0 90               nop
    00000000779FC5C1 90               nop
    00000000779FC5C2 90               nop
    00000000779FC5C3 90               nop
    */
    // Check for the signature as in the above disassembly
    DWORD guard = 0xB8D18B4C;
    if (*reinterpret_cast<DWORD*>(pProc) != guard)
        return;

    DWORD flOldProtect;
    VirtualProtect(pProc, 12, PAGE_EXECUTE_READWRITE, & flOldProtect);
    pProc[0] = 0x48;    // mov rax, this
    pProc[1] = 0xb8;
    *(__int64*)(pProc+2) = (__int64)pNewProc;
    pProc[10] = 0xff;   // jmp rax
    pProc[11] = 0xe0;
#endif
}

static void setUpOffscreenPaintingHooks(HDC (WINAPI*hookedBeginPaint)(HWND, PAINTSTRUCT*), BOOL (WINAPI*hookedEndPaint)(HWND, const PAINTSTRUCT*))
{
    static bool haveHooked = false;
    if (haveHooked)
        return;
    haveHooked = true;

    // Most (all?) windowed plugins don't seem to respond to WM_PRINTCLIENT, so
    // we hook into BeginPaint/EndPaint to allow their normal WM_PAINT handling
    // to draw into a given HDC. Note that this hooking affects the entire
    // process.
    hook("user32.dll", "BeginPaint", beginPaintSysCall, beginPaint, reinterpret_cast<const void *>(reinterpret_cast<ptrdiff_t>(hookedBeginPaint)));
    hook("user32.dll", "EndPaint", endPaintSysCall, endPaint, reinterpret_cast<const void *>(reinterpret_cast<ptrdiff_t>(hookedEndPaint)));

}
#endif

static bool registerPluginView()
{
    static bool haveRegisteredWindowClass = false;
    if (haveRegisteredWindowClass)
        return true;

    haveRegisteredWindowClass = true;

#if PLATFORM(QT)
    WebCore::setInstanceHandle((HINSTANCE)(qWinAppInst()));
#endif

    ASSERT(WebCore::instanceHandle());

#if OS(WINCE)
    WNDCLASS wcex = { 0 };
#else
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hIconSm        = 0;
#endif

    wcex.style          = CS_DBLCLKS;
#if OS(WINCE)
    wcex.style          |= CS_PARENTDC;
#endif
    wcex.lpfnWndProc    = DefWindowProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = WebCore::instanceHandle();
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)COLOR_WINDOW;
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kWebPluginViewdowClassName;

#if OS(WINCE)
    return !!RegisterClass(&wcex);
#else
    return !!RegisterClassEx(&wcex);
#endif
}

LRESULT CALLBACK PluginView::PluginViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PluginView* pluginView = reinterpret_cast<PluginView*>(GetProp(hWnd, kWebPluginViewProperty));

    return pluginView->wndProc(hWnd, message, wParam, lParam);
}

static bool isWindowsMessageUserGesture(UINT message)
{
    switch (message) {
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_KEYUP:
            return true;
        default:
            return false;
    }
}

LRESULT
PluginView::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // <rdar://5711136> Sometimes Flash will call SetCapture before creating
    // a full-screen window and will not release it, which causes the
    // full-screen window to never receive mouse events. We set/release capture
    // on mouse down/up before sending the event to the plug-in to prevent that.
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            ::SetCapture(hWnd);
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            ::ReleaseCapture();
            break;
    }

    if (message == m_lastMessage &&
        m_plugin->quirks().contains(PluginQuirkDontCallWndProcForSameMessageRecursively) && 
        m_isCallingPluginWndProc)
        return 1;

    if (message == WM_USER + 1 &&
        m_plugin->quirks().contains(PluginQuirkThrottleWMUserPlusOneMessages)) {
        if (!m_messageThrottler)
            m_messageThrottler = adoptPtr(new PluginMessageThrottlerWin(this));

        m_messageThrottler->appendMessage(hWnd, message, wParam, lParam);
        return 0;
    }

    m_lastMessage = message;
    m_isCallingPluginWndProc = true;

    // If the plug-in doesn't explicitly support changing the pop-up state, we enable
    // popups for all user gestures.
    // Note that we need to pop the state in a timer, because the Flash plug-in 
    // pops up windows in response to a posted message.
    if (m_plugin->pluginFuncs()->version < NPVERS_HAS_POPUPS_ENABLED_STATE &&
        isWindowsMessageUserGesture(message) && !m_popPopupsStateTimer.isActive()) {

        pushPopupsEnabledState(true);

        m_popPopupsStateTimer.startOneShot(0);
    }

#if !OS(WINCE)
    if (message == WM_PRINTCLIENT) {
        // Most (all?) windowed plugins don't respond to WM_PRINTCLIENT, so we
        // change the message to WM_PAINT and rely on our hooked versions of
        // BeginPaint/EndPaint to make the plugin draw into the given HDC.
        message = WM_PAINT;
        m_wmPrintHDC = reinterpret_cast<HDC>(wParam);
    }
#endif

    // Call the plug-in's window proc.
    LRESULT result = ::CallWindowProc(m_pluginWndProc, hWnd, message, wParam, lParam);

    m_wmPrintHDC = 0;

    m_isCallingPluginWndProc = false;

    return result;
}

void PluginView::updatePluginWidget()
{
    if (!parent())
        return;

    ASSERT(parent()->isFrameView());
    FrameView* frameView = static_cast<FrameView*>(parent());

    IntRect oldWindowRect = m_windowRect;
    IntRect oldClipRect = m_clipRect;

#if OS(WINCE)
    m_windowRect = frameView->contentsToWindow(frameRect());
#else
    m_windowRect = IntRect(frameView->contentsToWindow(frameRect().location()), frameRect().size());
#endif
    m_clipRect = windowClipRect();
    m_clipRect.move(-m_windowRect.x(), -m_windowRect.y());

    if (platformPluginWidget() && (!m_haveUpdatedPluginWidget || m_windowRect != oldWindowRect || m_clipRect != oldClipRect)) {
        HRGN rgn;

        setCallingPlugin(true);

        // To prevent flashes while scrolling, we disable drawing during the window
        // update process by clipping the window to the zero rect.

        bool clipToZeroRect = !m_plugin->quirks().contains(PluginQuirkDontClipToZeroRectWhenScrolling);

        if (clipToZeroRect) {
            rgn = ::CreateRectRgn(0, 0, 0, 0);
            ::SetWindowRgn(platformPluginWidget(), rgn, FALSE);
        } else {
            rgn = ::CreateRectRgn(m_clipRect.x(), m_clipRect.y(), m_clipRect.maxX(), m_clipRect.maxY());
            ::SetWindowRgn(platformPluginWidget(), rgn, TRUE);
        }

        if (!m_haveUpdatedPluginWidget || m_windowRect != oldWindowRect)
            ::MoveWindow(platformPluginWidget(), m_windowRect.x(), m_windowRect.y(), m_windowRect.width(), m_windowRect.height(), TRUE);

        if (clipToZeroRect) {
            rgn = ::CreateRectRgn(m_clipRect.x(), m_clipRect.y(), m_clipRect.maxX(), m_clipRect.maxY());
            ::SetWindowRgn(platformPluginWidget(), rgn, TRUE);
        }

        setCallingPlugin(false);

        m_haveUpdatedPluginWidget = true;
    }
}

void PluginView::setFocus(bool focused)
{
    if (focused && platformPluginWidget())
        SetFocus(platformPluginWidget());

    Widget::setFocus(focused);
}

void PluginView::show()
{
    setSelfVisible(true);

    if (isParentVisible() && platformPluginWidget())
        ShowWindow(platformPluginWidget(), SW_SHOWNA);

    Widget::show();
}

void PluginView::hide()
{
    setSelfVisible(false);

    if (isParentVisible() && platformPluginWidget())
        ShowWindow(platformPluginWidget(), SW_HIDE);

    Widget::hide();
}

bool PluginView::dispatchNPEvent(NPEvent& npEvent)
{
    if (!m_plugin->pluginFuncs()->event)
        return true;

    bool shouldPop = false;

    if (m_plugin->pluginFuncs()->version < NPVERS_HAS_POPUPS_ENABLED_STATE && isWindowsMessageUserGesture(npEvent.event)) {
        pushPopupsEnabledState(true);
        shouldPop = true;
    }

    JSC::JSLock::DropAllLocks dropAllLocks(JSC::SilenceAssertionsOnly);
    setCallingPlugin(true);
    bool result = m_plugin->pluginFuncs()->event(m_instance, &npEvent);
    setCallingPlugin(false);

    if (shouldPop) 
        popPopupsEnabledState();

    return result;
}

void PluginView::paintIntoTransformedContext(HDC hdc)
{
    if (m_isWindowed) {
#if !OS(WINCE)
        SendMessage(platformPluginWidget(), WM_PRINTCLIENT, reinterpret_cast<WPARAM>(hdc), PRF_CLIENT | PRF_CHILDREN | PRF_OWNED);
#endif
        return;
    }

    m_npWindow.type = NPWindowTypeDrawable;
    m_npWindow.window = hdc;

    WINDOWPOS windowpos = { 0, 0, 0, 0, 0, 0, 0 };

    IntRect r = static_cast<FrameView*>(parent())->contentsToWindow(frameRect());

    windowpos.x = r.x();
    windowpos.y = r.y();
    windowpos.cx = r.width();
    windowpos.cy = r.height();

    NPEvent npEvent;
    npEvent.event = WM_WINDOWPOSCHANGED;
    npEvent.lParam = reinterpret_cast<uintptr_t>(&windowpos);
    npEvent.wParam = 0;

    dispatchNPEvent(npEvent);

    setNPWindowRect(frameRect());

    npEvent.event = WM_PAINT;
    npEvent.wParam = reinterpret_cast<uintptr_t>(hdc);

    // This is supposed to be a pointer to the dirty rect, but it seems that the Flash plugin
    // ignores it so we just pass null.
    npEvent.lParam = 0;

    dispatchNPEvent(npEvent);
}

void PluginView::paintWindowedPluginIntoContext(GraphicsContext* context, const IntRect& rect)
{
#if !OS(WINCE)
    ASSERT(m_isWindowed);
    ASSERT(context->shouldIncludeChildWindows());

    ASSERT(parent()->isFrameView());
    IntPoint locationInWindow = static_cast<FrameView*>(parent())->contentsToWindow(frameRect().location());

    LocalWindowsContext windowsContext(context, frameRect(), false);

#if USE(CAIRO)
    // Must flush drawings up to this point to the backing metafile, otherwise the
    // plugin region will be overwritten with any clear regions specified in the
    // cairo-controlled portions of the rendering.
    cairo_show_page(context->platformContext()->cr());
#endif

    HDC hdc = windowsContext.hdc();
    XFORM originalTransform;
    GetWorldTransform(hdc, &originalTransform);

    // The plugin expects the DC to be in client coordinates, so we translate
    // the DC to make that so.
    AffineTransform ctm = context->getCTM();
    ctm.translate(locationInWindow.x(), locationInWindow.y());
    XFORM transform = static_cast<XFORM>(ctm.toTransformationMatrix());

    SetWorldTransform(hdc, &transform);

    paintIntoTransformedContext(hdc);

    SetWorldTransform(hdc, &originalTransform);
#endif
}

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_isStarted) {
        // Draw the "missing plugin" image
        paintMissingPluginIcon(context, rect);
        return;
    }

    if (context->paintingDisabled())
        return;

    // Ensure that we have called SetWindow before we try to paint.
    if (!m_haveCalledSetWindow)
        setNPWindowRect(frameRect());

    if (m_isWindowed) {
#if !OS(WINCE)
        if (context->shouldIncludeChildWindows())
            paintWindowedPluginIntoContext(context, rect);
#endif
        return;
    }

    ASSERT(parent()->isFrameView());
    IntRect rectInWindow = static_cast<FrameView*>(parent())->contentsToWindow(frameRect());
    LocalWindowsContext windowsContext(context, rectInWindow, m_isTransparent);

    // On Safari/Windows without transparency layers the GraphicsContext returns the HDC
    // of the window and the plugin expects that the passed in DC has window coordinates.
    // In the Qt port we always draw in an offscreen buffer and therefore need to preserve
    // the translation set in getWindowsContext.
#if !PLATFORM(QT) && !OS(WINCE)
    if (!context->inTransparencyLayer()) {
        XFORM transform;
        GetWorldTransform(windowsContext.hdc(), &transform);
        transform.eDx = 0;
        transform.eDy = 0;
        SetWorldTransform(windowsContext.hdc(), &transform);
    }
#endif

    paintIntoTransformedContext(windowsContext.hdc());
}

void PluginView::handleKeyboardEvent(KeyboardEvent* event)
{
    NPEvent npEvent;

    npEvent.wParam = event->keyCode();    

    if (event->type() == eventNames().keydownEvent) {
        npEvent.event = WM_KEYDOWN;
        npEvent.lParam = 0;
    } else if (event->type() == eventNames().keyupEvent) {
        npEvent.event = WM_KEYUP;
        npEvent.lParam = 0x8000;
    }

    JSC::JSLock::DropAllLocks dropAllLocks(JSC::SilenceAssertionsOnly);
    if (!dispatchNPEvent(npEvent))
        event->setDefaultHandled();
}

#if !OS(WINCE)
extern bool ignoreNextSetCursor;
#endif

void PluginView::handleMouseEvent(MouseEvent* event)
{
    NPEvent npEvent;

    IntPoint p = static_cast<FrameView*>(parent())->contentsToWindow(IntPoint(event->pageX(), event->pageY()));

    npEvent.lParam = MAKELPARAM(p.x(), p.y());
    npEvent.wParam = 0;

    if (event->ctrlKey())
        npEvent.wParam |= MK_CONTROL;
    if (event->shiftKey())
        npEvent.wParam |= MK_SHIFT;

    if (event->type() == eventNames().mousemoveEvent ||
        event->type() == eventNames().mouseoutEvent || 
        event->type() == eventNames().mouseoverEvent) {
        npEvent.event = WM_MOUSEMOVE;
        if (event->buttonDown())
            switch (event->button()) {
                case LeftButton:
                    npEvent.wParam |= MK_LBUTTON;
                    break;
                case MiddleButton:
                    npEvent.wParam |= MK_MBUTTON;
                    break;
                case RightButton:
                    npEvent.wParam |= MK_RBUTTON;
                break;
            }
    }
    else if (event->type() == eventNames().mousedownEvent) {
        focusPluginElement();
        switch (event->button()) {
            case 0:
                npEvent.event = WM_LBUTTONDOWN;
                break;
            case 1:
                npEvent.event = WM_MBUTTONDOWN;
                break;
            case 2:
                npEvent.event = WM_RBUTTONDOWN;
                break;
        }
    } else if (event->type() == eventNames().mouseupEvent) {
        switch (event->button()) {
            case 0:
                npEvent.event = WM_LBUTTONUP;
                break;
            case 1:
                npEvent.event = WM_MBUTTONUP;
                break;
            case 2:
                npEvent.event = WM_RBUTTONUP;
                break;
        }
    } else
        return;

    JSC::JSLock::DropAllLocks dropAllLocks(JSC::SilenceAssertionsOnly);
    if (!dispatchNPEvent(npEvent))
        event->setDefaultHandled();

#if !PLATFORM(QT) && !PLATFORM(WX) && !OS(WINCE)
    // Currently, Widget::setCursor is always called after this function in EventHandler.cpp
    // and since we don't want that we set ignoreNextSetCursor to true here to prevent that.
    ignoreNextSetCursor = true;
    if (Page* page = m_parentFrame->page())
        page->chrome()->client()->setLastSetCursorToCurrentCursor();    
#endif
}

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

#if OS(WINCE)
    if (parent) {
        init();
        if (parent->isVisible())
            show();
        else
            hide();
    }
#else
    if (parent)
        init();
    else {
        if (!platformPluginWidget())
            return;

        // If the plug-in window or one of its children have the focus, we need to 
        // clear it to prevent the web view window from being focused because that can
        // trigger a layout while the plugin element is being detached.
        HWND focusedWindow = ::GetFocus();
        if (platformPluginWidget() == focusedWindow || ::IsChild(platformPluginWidget(), focusedWindow))
            ::SetFocus(0);
    }
#endif
}

void PluginView::setParentVisible(bool visible)
{
    if (isParentVisible() == visible)
        return;

    Widget::setParentVisible(visible);

    if (isSelfVisible() && platformPluginWidget()) {
        if (visible)
            ShowWindow(platformPluginWidget(), SW_SHOWNA);
        else
            ShowWindow(platformPluginWidget(), SW_HIDE);
    }
}

void PluginView::setNPWindowRect(const IntRect& rect)
{
    if (!m_isStarted)
        return;

#if OS(WINCE)
    IntRect r = static_cast<FrameView*>(parent())->contentsToWindow(rect);
    m_npWindow.x = r.x();
    m_npWindow.y = r.y();

    m_npWindow.width = r.width();
    m_npWindow.height = r.height();

    m_npWindow.clipRect.right = r.width();
    m_npWindow.clipRect.bottom = r.height();
#else
    IntPoint p = static_cast<FrameView*>(parent())->contentsToWindow(rect.location());
    m_npWindow.x = p.x();
    m_npWindow.y = p.y();

    m_npWindow.width = rect.width();
    m_npWindow.height = rect.height();

    m_npWindow.clipRect.right = rect.width();
    m_npWindow.clipRect.bottom = rect.height();
#endif
    m_npWindow.clipRect.left = 0;
    m_npWindow.clipRect.top = 0;

    if (m_plugin->pluginFuncs()->setwindow) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSC::SilenceAssertionsOnly);
        setCallingPlugin(true);
        m_plugin->pluginFuncs()->setwindow(m_instance, &m_npWindow);
        setCallingPlugin(false);

        m_haveCalledSetWindow = true;

        if (!m_isWindowed)
            return;

        ASSERT(platformPluginWidget());

#if OS(WINCE)
        if (!m_pluginWndProc) {
            WNDPROC currentWndProc = (WNDPROC)GetWindowLong(platformPluginWidget(), GWL_WNDPROC);
            if (currentWndProc != PluginViewWndProc)
                m_pluginWndProc = (WNDPROC)SetWindowLong(platformPluginWidget(), GWL_WNDPROC, (LONG)PluginViewWndProc);
        }
#else
        WNDPROC currentWndProc = (WNDPROC)GetWindowLongPtr(platformPluginWidget(), GWLP_WNDPROC);
        if (currentWndProc != PluginViewWndProc)
            m_pluginWndProc = (WNDPROC)SetWindowLongPtr(platformPluginWidget(), GWLP_WNDPROC, (LONG_PTR)PluginViewWndProc);
#endif
    }
}

NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    String filename(buf, len);

    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    // Get file info
    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (GetFileAttributesExW(filename.charactersWithNullTermination(), GetFileExInfoStandard, &attrs) == 0)
        return NPERR_FILE_NOT_FOUND;

    if (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return NPERR_FILE_NOT_FOUND;

    HANDLE fileHandle = CreateFileW(filename.charactersWithNullTermination(), FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    if (fileHandle == INVALID_HANDLE_VALUE)
        return NPERR_FILE_NOT_FOUND;

    buffer.resize(attrs.nFileSizeLow);

    DWORD bytesRead;
    int retval = ReadFile(fileHandle, buffer.data(), attrs.nFileSizeLow, &bytesRead, 0);

    CloseHandle(fileHandle);

    if (retval == 0 || bytesRead != attrs.nFileSizeLow)
        return NPERR_FILE_NOT_FOUND;

    return NPERR_NO_ERROR;
}

bool PluginView::platformGetValueStatic(NPNVariable, void*, NPError*)
{
    return false;
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
        case NPNVnetscapeWindow: {
            HWND* w = reinterpret_cast<HWND*>(value);
            *w = windowHandleForPageClient(parent() ? parent()->hostWindow()->platformPageClient() : 0);
            *result = NPERR_NO_ERROR;
            return true;
        }

        case NPNVSupportsWindowless: {
            NPBool* flag = reinterpret_cast<NPBool*>(value);
            *flag = TRUE;
            *result = NPERR_NO_ERROR;
            return true;
        }

    default:
        return false;
    }
}

void PluginView::invalidateRect(const IntRect& rect)
{
    if (m_isWindowed) {
        RECT invalidRect = { rect.x(), rect.y(), rect.maxX(), rect.maxY() };
        ::InvalidateRect(platformPluginWidget(), &invalidRect, false);
        return;
    }

    invalidateWindowlessPluginRect(rect);
}

void PluginView::invalidateRect(NPRect* rect)
{
    if (!rect) {
        invalidate();
        return;
    }

    IntRect r(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);

    if (m_isWindowed) {
        RECT invalidRect = { r.x(), r.y(), r.maxX(), r.maxY() };
        InvalidateRect(platformPluginWidget(), &invalidRect, FALSE);
    } else {
        if (m_plugin->quirks().contains(PluginQuirkThrottleInvalidate)) {
            m_invalidRects.append(r);
            if (!m_invalidateTimer.isActive())
                m_invalidateTimer.startOneShot(0.001);
        } else
            invalidateRect(r);
    }
}

void PluginView::invalidateRegion(NPRegion region)
{
    if (m_isWindowed)
        return;

    RECT r;

    if (GetRgnBox(region, &r) == 0) {
        invalidate();
        return;
    }

    IntRect rect(IntPoint(r.left, r.top), IntSize(r.right-r.left, r.bottom-r.top));
    invalidateRect(rect);
}

void PluginView::forceRedraw()
{
    if (m_isWindowed)
        ::UpdateWindow(platformPluginWidget());
    else
        ::UpdateWindow(windowHandleForPageClient(parent() ? parent()->hostWindow()->platformPageClient() : 0));
}

bool PluginView::platformStart()
{
    ASSERT(m_isStarted);
    ASSERT(m_status == PluginStatusLoadedSuccessfully);

    if (m_isWindowed) {
        registerPluginView();
#if !OS(WINCE)
        setUpOffscreenPaintingHooks(hookedBeginPaint, hookedEndPaint);
#endif

        DWORD flags = WS_CHILD;
        if (isSelfVisible())
            flags |= WS_VISIBLE;

        HWND parentWindowHandle = windowHandleForPageClient(m_parentFrame->view()->hostWindow()->platformPageClient());
        HWND window = ::CreateWindowEx(0, kWebPluginViewdowClassName, 0, flags,
                                       0, 0, 0, 0, parentWindowHandle, 0, WebCore::instanceHandle(), 0);

#if OS(WINDOWS) && (PLATFORM(QT) || PLATFORM(WX))
        m_window = window;
#else
        setPlatformWidget(window);
#endif

        // Calling SetWindowLongPtrA here makes the window proc ASCII, which is required by at least
        // the Shockwave Director plug-in.
#if OS(WINDOWS) && CPU(X86_64)
        ::SetWindowLongPtrA(platformPluginWidget(), GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
#elif OS(WINCE)
        ::SetWindowLong(platformPluginWidget(), GWL_WNDPROC, (LONG)DefWindowProc);
#else
        ::SetWindowLongPtrA(platformPluginWidget(), GWL_WNDPROC, (LONG)DefWindowProcA);
#endif
        SetProp(platformPluginWidget(), kWebPluginViewProperty, this);

        m_npWindow.type = NPWindowTypeWindow;
        m_npWindow.window = platformPluginWidget();
    } else {
        m_npWindow.type = NPWindowTypeDrawable;
        m_npWindow.window = 0;
    }

    updatePluginWidget();

    if (!m_plugin->quirks().contains(PluginQuirkDeferFirstSetWindowCall))
        setNPWindowRect(frameRect());

    return true;
}

void PluginView::platformDestroy()
{
    if (!platformPluginWidget())
        return;

    DestroyWindow(platformPluginWidget());
    setPlatformPluginWidget(0);
}

PassRefPtr<Image> PluginView::snapshot()
{
#if !PLATFORM(WX) && !OS(WINCE)
    OwnPtr<HDC> hdc = adoptPtr(CreateCompatibleDC(0));

    if (!m_isWindowed) {
        // Enable world transforms.
        SetGraphicsMode(hdc.get(), GM_ADVANCED);

        XFORM transform;
        GetWorldTransform(hdc.get(), &transform);

        // Windowless plug-ins assume that they're drawing onto the view's DC.
        // Translate the context so that the plug-in draws at (0, 0).
        ASSERT(parent()->isFrameView());
        IntPoint position = static_cast<FrameView*>(parent())->contentsToWindow(frameRect()).location();
        transform.eDx = -position.x();
        transform.eDy = -position.y();
        SetWorldTransform(hdc.get(), &transform);
    }

    void* bits;
    BitmapInfo bmp = BitmapInfo::createBottomUp(frameRect().size());
    OwnPtr<HBITMAP> hbmp = adoptPtr(CreateDIBSection(0, &bmp, DIB_RGB_COLORS, &bits, 0, 0));

    HBITMAP hbmpOld = static_cast<HBITMAP>(SelectObject(hdc.get(), hbmp.get()));

    paintIntoTransformedContext(hdc.get());

    SelectObject(hdc.get(), hbmpOld);

    return BitmapImage::create(hbmp.get());
#else
    return 0;
#endif
}

void PluginView::halt()
{
    ASSERT(!m_isHalted);
    ASSERT(m_isStarted);

#if !PLATFORM(QT)
    // Show a screenshot of the plug-in.
    toRenderWidget(m_element->renderer())->showSubstituteImage(snapshot());
#endif

    m_isHalted = true;
    m_hasBeenHalted = true;

    stop();
    platformDestroy();
}

void PluginView::restart()
{
    ASSERT(!m_isStarted);
    ASSERT(m_isHalted);

    // Clear any substitute image.
    toRenderWidget(m_element->renderer())->showSubstituteImage(0);

    m_isHalted = false;
    m_haveUpdatedPluginWidget = false;
    start();
}

} // namespace WebCore
