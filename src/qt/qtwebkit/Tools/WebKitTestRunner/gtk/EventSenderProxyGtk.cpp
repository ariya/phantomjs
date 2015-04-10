/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventSenderProxy.h"

#include "PlatformWebView.h"
#include "TestController.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WTR {

// WebCore and layout tests assume this value
static const float pixelsPerScrollTick = 40;

// Key event location code defined in DOM Level 3.
enum KeyLocationCode {
    DOMKeyLocationStandard      = 0x00,
    DOMKeyLocationLeft          = 0x01,
    DOMKeyLocationRight         = 0x02,
    DOMKeyLocationNumpad        = 0x03
};


struct WTREventQueueItem {
    GdkEvent* event;
    gulong delay;

    WTREventQueueItem()
        : event(0)
        , delay(0)
    {
    }
    WTREventQueueItem(GdkEvent* event, gulong delay)
        : event(event)
        , delay(delay)
    {
    }
};

EventSenderProxy::EventSenderProxy(TestController* testController)
    : m_testController(testController)
    , m_time(0)
    , m_leftMouseButtonDown(false)
    , m_clickCount(0)
    , m_clickTime(0)
    , m_clickButton(kWKEventMouseButtonNoButton)
    , m_mouseButtonCurrentlyDown(0)
{
}

EventSenderProxy::~EventSenderProxy()
{
}

static guint getMouseButtonModifiers(int gdkButton)
{
    if (gdkButton == 1)
        return GDK_BUTTON1_MASK;
    if (gdkButton == 2)
        return GDK_BUTTON2_MASK;
    if (gdkButton == 3)
        return GDK_BUTTON3_MASK;
    return 0;
}

static unsigned eventSenderButtonToGDKButton(unsigned button)
{
    int mouseButton = 3;
    if (button <= 2)
        mouseButton = button + 1;
    // fast/events/mouse-click-events expects the 4th button to be treated as the middle button.
    else if (button == 3)
        mouseButton = 2;

    return mouseButton;
}

GdkEvent* EventSenderProxy::createMouseButtonEvent(GdkEventType eventType, unsigned button, WKEventModifiers modifiers)
{
    GdkEvent* mouseEvent = gdk_event_new(eventType);

    mouseEvent->button.button = eventSenderButtonToGDKButton(button);
    mouseEvent->button.x = m_position.x;
    mouseEvent->button.y = m_position.y;
    mouseEvent->button.window = gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformView()));
    g_object_ref(mouseEvent->button.window);
    gdk_event_set_device(mouseEvent, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_window_get_display(mouseEvent->button.window))));
    mouseEvent->button.state = modifiers | getMouseButtonModifiers(mouseEvent->button.button);
    mouseEvent->button.time = GDK_CURRENT_TIME;
    mouseEvent->button.axes = 0;

    int xRoot, yRoot;
    gdk_window_get_root_coords(mouseEvent->button.window, m_position.x, m_position.y, &xRoot, &yRoot);
    mouseEvent->button.x_root = xRoot;
    mouseEvent->button.y_root = yRoot;

    return mouseEvent;
}

void EventSenderProxy::updateClickCountForButton(int button)
{
    if (m_time - m_clickTime < 1 && m_position == m_clickPosition && button == m_clickButton) {
        ++m_clickCount;
        m_clickTime = m_time;
        return;
    }

    m_clickCount = 1;
    m_clickTime = m_time;
    m_clickPosition = m_position;
    m_clickButton = button;
}

static void dispatchEvent(GdkEvent* event)
{
    gtk_main_do_event(event);
    gdk_event_free(event);
}

void EventSenderProxy::replaySavedEvents()
{
    while (!m_eventQueue.isEmpty()) {
        WTREventQueueItem item = m_eventQueue.takeFirst();
        if (item.delay)
            g_usleep(item.delay * 1000);

        dispatchEvent(item.event);
    }
}

void EventSenderProxy::sendOrQueueEvent(GdkEvent* event)
{
    if (m_eventQueue.isEmpty() || !m_eventQueue.last().delay) {
        dispatchEvent(event);
        return;
    }

    m_eventQueue.last().event = event;
    replaySavedEvents();
}

static guint webkitModifiersToGDKModifiers(WKEventModifiers wkModifiers)
{
    guint modifiers = 0;

    if (wkModifiers & kWKEventModifiersControlKey)
        modifiers |= GDK_CONTROL_MASK;
    if (wkModifiers & kWKEventModifiersShiftKey)
        modifiers |= GDK_SHIFT_MASK;
    if (wkModifiers & kWKEventModifiersAltKey)
        modifiers |= GDK_MOD1_MASK;
    if (wkModifiers & kWKEventModifiersMetaKey)
        modifiers |= GDK_META_MASK;

    return modifiers;
}

int getGDKKeySymForKeyRef(WKStringRef keyRef, unsigned location, guint* modifiers)
{
    if (location == DOMKeyLocationNumpad) {
        if (WKStringIsEqualToUTF8CString(keyRef, "leftArrow"))
            return GDK_KEY_KP_Left;
        if (WKStringIsEqualToUTF8CString(keyRef, "rightArror"))
            return GDK_KEY_KP_Right;
        if (WKStringIsEqualToUTF8CString(keyRef, "upArrow"))
            return GDK_KEY_KP_Up;
        if (WKStringIsEqualToUTF8CString(keyRef, "downArrow"))
            return GDK_KEY_KP_Down;
        if (WKStringIsEqualToUTF8CString(keyRef, "pageUp"))
            return GDK_KEY_KP_Page_Up;
        if (WKStringIsEqualToUTF8CString(keyRef, "pageDown"))
            return GDK_KEY_KP_Page_Down;
        if (WKStringIsEqualToUTF8CString(keyRef, "home"))
            return GDK_KEY_KP_Home;
        if (WKStringIsEqualToUTF8CString(keyRef, "end"))
            return GDK_KEY_KP_End;
        if (WKStringIsEqualToUTF8CString(keyRef, "insert"))
            return GDK_KEY_KP_Insert;
        if (WKStringIsEqualToUTF8CString(keyRef, "delete"))
            return GDK_KEY_KP_Delete;

        return GDK_KEY_VoidSymbol;
    }

    if (WKStringIsEqualToUTF8CString(keyRef, "leftArrow"))
        return GDK_KEY_Left;
    if (WKStringIsEqualToUTF8CString(keyRef, "rightArrow"))
        return GDK_KEY_Right;
    if (WKStringIsEqualToUTF8CString(keyRef, "upArrow"))
        return GDK_KEY_Up;
    if (WKStringIsEqualToUTF8CString(keyRef, "downArrow"))
        return GDK_KEY_Down;
    if (WKStringIsEqualToUTF8CString(keyRef, "pageUp"))
        return GDK_KEY_Page_Up;
    if (WKStringIsEqualToUTF8CString(keyRef, "pageDown"))
        return GDK_KEY_Page_Down;
    if (WKStringIsEqualToUTF8CString(keyRef, "home"))
        return GDK_KEY_Home;
    if (WKStringIsEqualToUTF8CString(keyRef, "end"))
        return GDK_KEY_End;
    if (WKStringIsEqualToUTF8CString(keyRef, "insert"))
        return GDK_KEY_Insert;
    if (WKStringIsEqualToUTF8CString(keyRef, "delete"))
        return GDK_KEY_Delete;
    if (WKStringIsEqualToUTF8CString(keyRef, "printScreen"))
        return GDK_KEY_Print;
    if (WKStringIsEqualToUTF8CString(keyRef, "menu"))
        return GDK_KEY_Menu;
    if (WKStringIsEqualToUTF8CString(keyRef, "F1"))
        return GDK_KEY_F1;
    if (WKStringIsEqualToUTF8CString(keyRef, "F2"))
        return GDK_KEY_F2;
    if (WKStringIsEqualToUTF8CString(keyRef, "F3"))
        return GDK_KEY_F3;
    if (WKStringIsEqualToUTF8CString(keyRef, "F4"))
        return GDK_KEY_F4;
    if (WKStringIsEqualToUTF8CString(keyRef, "F5"))
        return GDK_KEY_F5;
    if (WKStringIsEqualToUTF8CString(keyRef, "F6"))
        return GDK_KEY_F6;
    if (WKStringIsEqualToUTF8CString(keyRef, "F7"))
        return GDK_KEY_F7;
    if (WKStringIsEqualToUTF8CString(keyRef, "F8"))
        return GDK_KEY_F8;
    if (WKStringIsEqualToUTF8CString(keyRef, "F9"))
        return GDK_KEY_F9;
    if (WKStringIsEqualToUTF8CString(keyRef, "F10"))
        return GDK_KEY_F10;
    if (WKStringIsEqualToUTF8CString(keyRef, "F11"))
        return GDK_KEY_F11;
    if (WKStringIsEqualToUTF8CString(keyRef, "F12"))
        return GDK_KEY_F12;

    size_t bufferSize = WKStringGetMaximumUTF8CStringSize(keyRef);
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[bufferSize]);
    WKStringGetUTF8CString(keyRef, buffer.get(), bufferSize);
    char charCode = buffer.get()[0];

    if (charCode == '\n' || charCode == '\r')
        return GDK_KEY_Return;
    if (charCode == '\t')
        return GDK_KEY_Tab;
    if (charCode == '\x8')
        return GDK_KEY_BackSpace;

    if (WTF::isASCIIUpper(charCode))
        *modifiers |= GDK_SHIFT_MASK;

    return gdk_unicode_to_keyval(static_cast<guint32>(buffer.get()[0]));
}

void EventSenderProxy::keyDown(WKStringRef keyRef, WKEventModifiers wkModifiers, unsigned location)
{
    guint modifiers = webkitModifiersToGDKModifiers(wkModifiers);
    int gdkKeySym = getGDKKeySymForKeyRef(keyRef, location, &modifiers);

    GdkEvent* pressEvent = gdk_event_new(GDK_KEY_PRESS);
    pressEvent->key.keyval = gdkKeySym;
    pressEvent->key.state = modifiers;
    pressEvent->key.window = gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformWindow()));
    g_object_ref(pressEvent->key.window);
    gdk_event_set_device(pressEvent, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_window_get_display(pressEvent->key.window))));

    GOwnPtr<GdkKeymapKey> keys;
    gint nKeys;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), gdkKeySym, &keys.outPtr(), &nKeys))
        pressEvent->key.hardware_keycode = keys.get()[0].keycode;

    GdkEvent* releaseEvent = gdk_event_copy(pressEvent);
    dispatchEvent(pressEvent);
    releaseEvent->key.type = GDK_KEY_RELEASE;
    dispatchEvent(releaseEvent);
}

void EventSenderProxy::mouseDown(unsigned button, WKEventModifiers wkModifiers)
{
    // If the same mouse button is already in the down position don't
    // send another event as it may confuse Xvfb.
    unsigned gdkButton = eventSenderButtonToGDKButton(button);
    if (m_mouseButtonCurrentlyDown == gdkButton)
        return;

    m_mouseButtonCurrentlyDown = gdkButton;

    // Normally GDK will send both GDK_BUTTON_PRESS and GDK_2BUTTON_PRESS for
    // the second button press during double-clicks. WebKit GTK+ selectively
    // ignores the first GDK_BUTTON_PRESS of that pair using gdk_event_peek.
    // Since our events aren't ever going onto the GDK event queue, WebKit won't
    // be able to filter out the first GDK_BUTTON_PRESS, so we just don't send
    // it here. Eventually this code should probably figure out a way to get all
    // appropriate events onto the event queue and this work-around should be
    // removed.
    updateClickCountForButton(button);

    GdkEventType eventType;
    if (m_clickCount == 2)
        eventType = GDK_2BUTTON_PRESS;
    else if (m_clickCount == 3)
        eventType = GDK_3BUTTON_PRESS;
    else
        eventType = GDK_BUTTON_PRESS;

    GdkEvent* event = createMouseButtonEvent(eventType, button, wkModifiers);
    sendOrQueueEvent(event);
}

void EventSenderProxy::mouseUp(unsigned button, WKEventModifiers wkModifiers)
{
    m_clickButton = kWKEventMouseButtonNoButton;
    GdkEvent* event = createMouseButtonEvent(GDK_BUTTON_RELEASE, button, wkModifiers);
    sendOrQueueEvent(event);

    if (m_mouseButtonCurrentlyDown == event->button.button)
        m_mouseButtonCurrentlyDown = 0;
    m_clickPosition = m_position;
    m_clickTime = GDK_CURRENT_TIME;
}

void EventSenderProxy::mouseMoveTo(double x, double y)
{
    m_position.x = x;
    m_position.y = y;

    GdkEvent* event = gdk_event_new(GDK_MOTION_NOTIFY);
    event->motion.x = m_position.x;
    event->motion.y = m_position.y;

    event->motion.time = GDK_CURRENT_TIME;
    event->motion.window = gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformView()));
    g_object_ref(event->motion.window);
    gdk_event_set_device(event, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_window_get_display(event->motion.window))));
    event->motion.state = 0 | getMouseButtonModifiers(m_mouseButtonCurrentlyDown);
    event->motion.axes = 0;

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformView())), m_position.x, m_position.y , &xRoot, &yRoot);
    event->motion.x_root = xRoot;
    event->motion.y_root = yRoot;

    sendOrQueueEvent(event);
}

void EventSenderProxy::mouseScrollBy(int horizontal, int vertical)
{
    // Copy behaviour of Qt and EFL - just return in case of (0,0) mouse scroll
    if (!horizontal && !vertical)
        return;

    GdkEvent* event = gdk_event_new(GDK_SCROLL);
    event->scroll.x = m_position.x;
    event->scroll.y = m_position.y;
    event->scroll.time = GDK_CURRENT_TIME;
    event->scroll.window = gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformView()));
    g_object_ref(event->scroll.window);
    gdk_event_set_device(event, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_window_get_display(event->scroll.window))));

    // For more than one tick in a scroll, we need smooth scroll event
    if ((horizontal && vertical) || horizontal > 1 || horizontal < -1 || vertical > 1 || vertical < -1) {
        event->scroll.direction = GDK_SCROLL_SMOOTH;
        event->scroll.delta_x = -horizontal;
        event->scroll.delta_y = -vertical;

        sendOrQueueEvent(event);
        return;
    }

    if (horizontal < 0)
        event->scroll.direction = GDK_SCROLL_RIGHT;
    else if (horizontal > 0)
        event->scroll.direction = GDK_SCROLL_LEFT;
    else if (vertical < 0)
        event->scroll.direction = GDK_SCROLL_DOWN;
    else if (vertical > 0)
        event->scroll.direction = GDK_SCROLL_UP;
    else
        g_assert_not_reached();

    sendOrQueueEvent(event);
}

void EventSenderProxy::continuousMouseScrollBy(int horizontal, int vertical, bool paged)
{
    // Gtk+ does not support paged scroll events.
    g_return_if_fail(!paged);

    GdkEvent* event = gdk_event_new(GDK_SCROLL);
    event->scroll.x = m_position.x;
    event->scroll.y = m_position.y;
    event->scroll.time = GDK_CURRENT_TIME;
    event->scroll.window = gtk_widget_get_window(GTK_WIDGET(m_testController->mainWebView()->platformView()));
    g_object_ref(event->scroll.window);
    gdk_event_set_device(event, gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_window_get_display(event->scroll.window))));

    event->scroll.direction = GDK_SCROLL_SMOOTH;
    event->scroll.delta_x = -horizontal / pixelsPerScrollTick;
    event->scroll.delta_y = -vertical / pixelsPerScrollTick;

    sendOrQueueEvent(event);
}

void EventSenderProxy::leapForward(int milliseconds)
{
    if (m_eventQueue.isEmpty())
        m_eventQueue.append(WTREventQueueItem());

    m_eventQueue.last().delay = milliseconds;
    m_time += milliseconds / 1000.0;
}

} // namespace WTR
