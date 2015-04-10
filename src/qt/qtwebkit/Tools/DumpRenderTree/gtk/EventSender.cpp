/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2012 ChangSeok Oh <shivamidow@gmail.com>
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
#include "EventSender.h"

#include "DumpRenderTree.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"
#include <GOwnPtrGtk.h>
#include <GRefPtrGtk.h>
#include <GtkVersioning.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <cstring>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit/webkitwebframe.h>
#include <webkit/webkitwebview.h>
#include <wtf/ASCIICType.h>
#include <wtf/Platform.h>
#include <wtf/text/CString.h>

extern "C" {
    extern GtkMenu* webkit_web_view_get_context_menu(WebKitWebView*);
}

static bool dragMode;
static int timeOffset = 0;

static int lastMousePositionX;
static int lastMousePositionY;
static int lastClickPositionX;
static int lastClickPositionY;
static int lastClickTimeOffset;
static int lastClickButton;
static unsigned buttonCurrentlyDown;
static int clickCount;
GdkDragContext* currentDragSourceContext;

struct DelayedMessage {
    GdkEvent* event;
    gulong delay;
};

static DelayedMessage msgQueue[1024];

static unsigned endOfQueue;
static unsigned startOfQueue;

static const float zoomMultiplierRatio = 1.2f;

// WebCore and layout tests assume this value.
static const float pixelsPerScrollTick = 40;

// Key event location code defined in DOM Level 3.
enum KeyLocationCode {
    DOM_KEY_LOCATION_STANDARD      = 0x00,
    DOM_KEY_LOCATION_LEFT          = 0x01,
    DOM_KEY_LOCATION_RIGHT         = 0x02,
    DOM_KEY_LOCATION_NUMPAD        = 0x03
};

static void sendOrQueueEvent(GdkEvent*, bool = true);
static void dispatchEvent(GdkEvent* event);
static guint getStateFlags();

static JSValueRef getDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, dragMode);
}

static bool setDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    dragMode = JSValueToBoolean(context, value);
    return true;
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount > 0) {
        msgQueue[endOfQueue].delay = JSValueToNumber(context, arguments[0], exception);
        timeOffset += msgQueue[endOfQueue].delay;
        ASSERT(!exception || !*exception);
    }

    return JSValueMakeUndefined(context);
}

bool prepareMouseButtonEvent(GdkEvent* event, int eventSenderButtonNumber, guint modifiers)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return false;

    // The logic for mapping EventSender button numbers to GDK button
    // numbers originates from the Windows EventSender.
    int gdkButtonNumber = 3;
    if (eventSenderButtonNumber >= 0 && eventSenderButtonNumber <= 2)
        gdkButtonNumber = eventSenderButtonNumber + 1;

    // fast/events/mouse-click-events expects the 4th button
    // to be event->button = 1, so send a middle-button event.
    else if (eventSenderButtonNumber == 3)
        gdkButtonNumber = 2;

    event->button.button = gdkButtonNumber;
    event->button.x = lastMousePositionX;
    event->button.y = lastMousePositionY;
    event->button.window = gtk_widget_get_window(GTK_WIDGET(view));
    g_object_ref(event->button.window);
    event->button.device = getDefaultGDKPointerDevice(event->button.window);
    event->button.state = modifiers | getStateFlags();
    event->button.time = GDK_CURRENT_TIME;
    event->button.axes = 0;

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(GTK_WIDGET(view)), lastMousePositionX, lastMousePositionY, &xRoot, &yRoot);
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;

    return true;
}

static JSValueRef getMenuItemTitleCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    GtkWidget* widget = GTK_WIDGET(JSObjectGetPrivate(object));
    CString label;
    if (GTK_IS_SEPARATOR_MENU_ITEM(widget))
        label = "<separator>";
    else
        label = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));

    JSRetainPtr<JSStringRef> itemText(Adopt, JSStringCreateWithUTF8CString(label.data()));
    return JSValueMakeString(context, itemText.get());
}

static bool setMenuItemTitleCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    return true;
}

static JSValueRef menuItemClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    GtkMenuItem* item = GTK_MENU_ITEM(JSObjectGetPrivate(thisObject));
    gtk_menu_item_activate(item);
    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticMenuItemFunctions[] = {
    { "click", menuItemClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticMenuItemValues[] = {
    { "title", getMenuItemTitleCallback, setMenuItemTitleCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getMenuItemClass()
{
    static JSClassRef menuItemClass = 0;

    if (!menuItemClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticMenuItemFunctions;
        classDefinition.staticValues = staticMenuItemValues;

        menuItemClass = JSClassCreate(&classDefinition);
    }

    return menuItemClass;
}


static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    GdkEvent* pressEvent = gdk_event_new(GDK_BUTTON_PRESS);

    if (!prepareMouseButtonEvent(pressEvent, 2, 0)) {
        gdk_event_free(pressEvent);
        return JSObjectMakeArray(context, 0, 0, 0);
    }

    GdkEvent* releaseEvent = gdk_event_copy(pressEvent);
    sendOrQueueEvent(pressEvent);

    JSValueRef valueRef = JSObjectMakeArray(context, 0, 0, 0);
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    GtkMenu* gtkMenu = webkit_web_view_get_context_menu(view);
    if (gtkMenu) {
        GOwnPtr<GList> items(gtk_container_get_children(GTK_CONTAINER(gtkMenu)));
        JSValueRef arrayValues[g_list_length(items.get())];
        int index = 0;
        for (GList* item = g_list_first(items.get()); item; item = g_list_next(item)) {
            arrayValues[index] = JSObjectMake(context, getMenuItemClass(), item->data);
            index++;
        }
        if (index)
            valueRef = JSObjectMakeArray(context, index - 1, arrayValues, 0);
    }

    releaseEvent->type = GDK_BUTTON_RELEASE;
    sendOrQueueEvent(releaseEvent);
    return valueRef;
}

static gboolean sendClick(gpointer)
{
    GdkEvent* pressEvent = gdk_event_new(GDK_BUTTON_PRESS);

    if (!prepareMouseButtonEvent(pressEvent, 1, 0)) {
        gdk_event_free(pressEvent);
        return FALSE;
    }

    GdkEvent* releaseEvent = gdk_event_copy(pressEvent);
    dispatchEvent(pressEvent);
    releaseEvent->type = GDK_BUTTON_RELEASE;
    dispatchEvent(releaseEvent);

    return FALSE;
}

static JSValueRef scheduleAsynchronousClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    g_timeout_add(0, sendClick, 0);
    return JSValueMakeUndefined(context);
}

static void updateClickCount(int button)
{
    if (lastClickPositionX != lastMousePositionX
        || lastClickPositionY != lastMousePositionY
        || lastClickButton != button
        || timeOffset - lastClickTimeOffset >= 1)
        clickCount = 1;
    else
        clickCount++;
}

static guint gdkModifierFromJSValue(JSContextRef context, const JSValueRef value)
{
    JSStringRef string = JSValueToStringCopy(context, value, 0);
    guint gdkModifier = 0;
    if (JSStringIsEqualToUTF8CString(string, "ctrlKey")
        || JSStringIsEqualToUTF8CString(string, "addSelectionKey"))
        gdkModifier = GDK_CONTROL_MASK;
    else if (JSStringIsEqualToUTF8CString(string, "shiftKey")
             || JSStringIsEqualToUTF8CString(string, "rangeSelectionKey"))
        gdkModifier = GDK_SHIFT_MASK;
    else if (JSStringIsEqualToUTF8CString(string, "altKey"))
        gdkModifier = GDK_MOD1_MASK;
    
    // Currently the metaKey as defined in WebCore/platform/gtk/PlatformMouseEventGtk.cpp
    // is GDK_META_MASK. This code must be kept in sync with that file.
    else if (JSStringIsEqualToUTF8CString(string, "metaKey"))
        gdkModifier = GDK_META_MASK;
    
    JSStringRelease(string);
    return gdkModifier;
}

static guint gdkModifersFromJSValue(JSContextRef context, const JSValueRef modifiers)
{
    // The value may either be a string with a single modifier or an array of modifiers.
    if (JSValueIsString(context, modifiers))
        return gdkModifierFromJSValue(context, modifiers);

    JSObjectRef modifiersArray = JSValueToObject(context, modifiers, 0);
    if (!modifiersArray)
        return 0;

    guint gdkModifiers = 0;
    JSRetainPtr<JSStringRef> lengthProperty(Adopt, JSStringCreateWithUTF8CString("length"));
    int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty.get(), 0), 0);
    for (int i = 0; i < modifiersCount; ++i)
        gdkModifiers |= gdkModifierFromJSValue(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0));
    return gdkModifiers;
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
        g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));
    }
    guint modifiers = argumentCount >= 2 ? gdkModifersFromJSValue(context, arguments[1]) : 0;

    GdkEvent* event = gdk_event_new(GDK_BUTTON_PRESS);
    if (!prepareMouseButtonEvent(event, button, modifiers)) {
        gdk_event_free(event);
        return JSValueMakeUndefined(context);
    }

    // If the same mouse button is already in the down position don't send another event as it may confuse Xvfb.
    if (buttonCurrentlyDown == event->button.button) {
        gdk_event_free(event);
        return JSValueMakeUndefined(context);
    }

    buttonCurrentlyDown = event->button.button;

    // Normally GDK will send both GDK_BUTTON_PRESS and GDK_2BUTTON_PRESS for
    // the second button press during double-clicks. WebKit GTK+ selectively
    // ignores the first GDK_BUTTON_PRESS of that pair using gdk_event_peek.
    // Since our events aren't ever going onto the GDK event queue, WebKit won't
    // be able to filter out the first GDK_BUTTON_PRESS, so we just don't send
    // it here. Eventually this code should probably figure out a way to get all
    // appropriate events onto the event queue and this work-around should be
    // removed.
    updateClickCount(event->button.button);
    if (clickCount == 2)
        event->type = GDK_2BUTTON_PRESS;
    else if (clickCount == 3)
        event->type = GDK_3BUTTON_PRESS;

    sendOrQueueEvent(event);
    return JSValueMakeUndefined(context);
}

static guint getStateFlags()
{
    if (buttonCurrentlyDown == 1)
        return GDK_BUTTON1_MASK;
    if (buttonCurrentlyDown == 2)
        return GDK_BUTTON2_MASK;
    if (buttonCurrentlyDown == 3)
        return GDK_BUTTON3_MASK;
    return 0;
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
        g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));
    }
    guint modifiers = argumentCount >= 2 ? gdkModifersFromJSValue(context, arguments[1]) : 0;

    GdkEvent* event = gdk_event_new(GDK_BUTTON_RELEASE);
    if (!prepareMouseButtonEvent(event, button, modifiers)) {
        gdk_event_free(event);
        return JSValueMakeUndefined(context);
    }

    lastClickPositionX = lastMousePositionX;
    lastClickPositionY = lastMousePositionY;
    lastClickButton = buttonCurrentlyDown;
    lastClickTimeOffset = timeOffset;
    buttonCurrentlyDown = 0;

    sendOrQueueEvent(event);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return JSValueMakeUndefined(context);

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    lastMousePositionX = (int)JSValueToNumber(context, arguments[0], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));
    lastMousePositionY = (int)JSValueToNumber(context, arguments[1], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));

    GdkEvent* event = gdk_event_new(GDK_MOTION_NOTIFY);
    event->motion.x = lastMousePositionX;
    event->motion.y = lastMousePositionY;

    event->motion.time = GDK_CURRENT_TIME;
    event->motion.window = gtk_widget_get_window(GTK_WIDGET(view));
    g_object_ref(event->motion.window);
    event->button.device = getDefaultGDKPointerDevice(event->motion.window);

    guint modifiers = argumentCount >= 3 ? gdkModifersFromJSValue(context, arguments[2]) : 0;
    event->motion.state = modifiers | getStateFlags();
    event->motion.axes = 0;

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(GTK_WIDGET(view)), lastMousePositionX, lastMousePositionY, &xRoot, &yRoot);
    event->motion.x_root = xRoot;
    event->motion.y_root = yRoot;

    sendOrQueueEvent(event, false);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return JSValueMakeUndefined(context);

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int horizontal = (int)JSValueToNumber(context, arguments[0], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));
    int vertical = (int)JSValueToNumber(context, arguments[1], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));

    // Copy behaviour of Qt and EFL - just return in case of (0,0) mouse scroll
    if (!horizontal && !vertical)
        return JSValueMakeUndefined(context);

    GdkEvent* event = gdk_event_new(GDK_SCROLL);
    event->scroll.x = lastMousePositionX;
    event->scroll.y = lastMousePositionY;
    event->scroll.time = GDK_CURRENT_TIME;
    event->scroll.window = gtk_widget_get_window(GTK_WIDGET(view));
    g_object_ref(event->scroll.window);

    // GTK+ only supports one tick in each scroll event that is not smooth. For the cases of more than one direction,
    // and more than one step in a direction, we can only use smooth events, supported from Gtk 3.3.18.
#if GTK_CHECK_VERSION(3, 3, 18)
    if ((horizontal && vertical) || horizontal > 1 || horizontal < -1 || vertical > 1 || vertical < -1) {
        event->scroll.direction = GDK_SCROLL_SMOOTH;
        event->scroll.delta_x = -horizontal;
        event->scroll.delta_y = -vertical;

        sendOrQueueEvent(event);
        return JSValueMakeUndefined(context);
    }
#else
    g_return_val_if_fail((!vertical || !horizontal), JSValueMakeUndefined(context));
#endif

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
    return JSValueMakeUndefined(context);
}

static JSValueRef continuousMouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
#if GTK_CHECK_VERSION(3, 3, 18)
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return JSValueMakeUndefined(context);

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int horizontal = JSValueToNumber(context, arguments[0], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));
    int vertical = JSValueToNumber(context, arguments[1], exception);
    g_return_val_if_fail((!exception || !*exception), JSValueMakeUndefined(context));

    // We do not yet support continuous scrolling by page.
    if (argumentCount >= 3 && JSValueToBoolean(context, arguments[2]))
        return JSValueMakeUndefined(context);

    GdkEvent* event = gdk_event_new(GDK_SCROLL);
    event->scroll.x = lastMousePositionX;
    event->scroll.y = lastMousePositionY;
    event->scroll.time = GDK_CURRENT_TIME;
    event->scroll.window = gtk_widget_get_window(GTK_WIDGET(view));
    g_object_ref(event->scroll.window);

    event->scroll.direction = GDK_SCROLL_SMOOTH;
    event->scroll.delta_x = -horizontal / pixelsPerScrollTick;
    event->scroll.delta_y = -vertical / pixelsPerScrollTick;

    sendOrQueueEvent(event);
#endif
    return JSValueMakeUndefined(context);
}

static void dragWithFilesDragDataGetCallback(GtkWidget*, GdkDragContext*, GtkSelectionData *data, guint, guint, gpointer userData)
{
    gtk_selection_data_set_uris(data, static_cast<gchar**>(userData));
}

static void dragWithFilesDragEndCallback(GtkWidget* widget, GdkDragContext*, gpointer userData)
{
    g_signal_handlers_disconnect_by_func(widget, reinterpret_cast<void*>(dragWithFilesDragEndCallback), userData);
    g_signal_handlers_disconnect_by_func(widget, reinterpret_cast<void*>(dragWithFilesDragDataGetCallback), userData);
    g_strfreev(static_cast<gchar**>(userData));
}

static JSValueRef beginDragWithFilesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSObjectRef filesArray = JSValueToObject(context, arguments[0], exception);
    ASSERT(!exception || !*exception);

    const gchar* mainFrameURI = webkit_web_frame_get_uri(mainFrame);
    GRefPtr<GFile> testFile(adoptGRef(g_file_new_for_uri(mainFrameURI)));
    GRefPtr<GFile> parentDirectory(g_file_get_parent(testFile.get()));
    if (!parentDirectory)
        return JSValueMakeUndefined(context);

    // If this is an HTTP test, we still need to pass a local file path
    // to WebCore. Even though the file doesn't exist, this should be fine
    // for most tests.
    GOwnPtr<gchar> scheme(g_file_get_uri_scheme(parentDirectory.get()));
    if (g_str_equal(scheme.get(), "http") || g_str_equal(scheme.get(), "https")) {
        GOwnPtr<gchar> currentDirectory(g_get_current_dir());
        parentDirectory = adoptGRef(g_file_new_for_path(currentDirectory.get()));
    }

    JSStringRef lengthProperty = JSStringCreateWithUTF8CString("length");
    int filesArrayLength = JSValueToNumber(context, JSObjectGetProperty(context, filesArray, lengthProperty, 0), 0);
    JSStringRelease(lengthProperty);

    gchar** draggedFilesURIList = g_new0(gchar*, filesArrayLength + 1);
    for (int i = 0; i < filesArrayLength; ++i) {
        JSStringRef filenameString = JSValueToStringCopy(context,
                                                         JSObjectGetPropertyAtIndex(context, filesArray, i, 0), 0);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(filenameString);
        GOwnPtr<gchar> filenameBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
        JSStringGetUTF8CString(filenameString, filenameBuffer.get(), bufferSize);
        JSStringRelease(filenameString);

        GRefPtr<GFile> dragFile(g_file_get_child(parentDirectory.get(), filenameBuffer.get()));
        draggedFilesURIList[i] = g_file_get_uri(dragFile.get());
    }

    GtkWidget* view = GTK_WIDGET(webkit_web_frame_get_web_view(mainFrame));
    g_object_connect(G_OBJECT(view),
        "signal::drag-end", dragWithFilesDragEndCallback, draggedFilesURIList,
        "signal::drag-data-get", dragWithFilesDragDataGetCallback, draggedFilesURIList,
        NULL);

    GdkEvent event;
    GdkWindow* viewGDKWindow = gtk_widget_get_window(view);
    memset(&event, 0, sizeof(event));
    event.type = GDK_MOTION_NOTIFY;
    event.motion.x = lastMousePositionX;
    event.motion.y = lastMousePositionY;
    event.motion.time = GDK_CURRENT_TIME;
    event.motion.window = viewGDKWindow;
    event.motion.device = getDefaultGDKPointerDevice(viewGDKWindow);
    event.motion.state = GDK_BUTTON1_MASK;

    int xRoot, yRoot;
    gdk_window_get_root_coords(viewGDKWindow, lastMousePositionX, lastMousePositionY, &xRoot, &yRoot);
    event.motion.x_root = xRoot;
    event.motion.y_root = yRoot;

    GtkTargetList* targetList = gtk_target_list_new(0, 0);
    gtk_target_list_add_uri_targets(targetList, 0);
    gtk_drag_begin(view, targetList, GDK_ACTION_COPY, 1, &event);
    gtk_target_list_unref(targetList);

    return JSValueMakeUndefined(context);
}

static void sendOrQueueEvent(GdkEvent* event, bool shouldReplaySavedEvents)
{
    // Mouse move events are queued if the previous event was queued or if a
    // delay was set up by leapForward().
    if ((dragMode && buttonCurrentlyDown) || endOfQueue != startOfQueue || msgQueue[endOfQueue].delay) {
        msgQueue[endOfQueue++].event = event;

        if (shouldReplaySavedEvents)
            replaySavedEvents();

        return;
    }

    dispatchEvent(event);
}

static void dispatchEvent(GdkEvent* event)
{
    DumpRenderTreeSupportGtk::layoutFrame(mainFrame);
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view) {
        gdk_event_free(event);
        return;
    }

    // The widget focus may have been lost in the course of the test,
    // so force another explicit focus grab here.
    gtk_widget_grab_focus(GTK_WIDGET(view));
    gtk_main_do_event(event);

    if (!currentDragSourceContext) {
        gdk_event_free(event);
        return;
    }

    if (event->type == GDK_MOTION_NOTIFY) {
        // WebKit has called gtk_drag_start(), but because the main loop isn't
        // running GDK internals don't know that the drag has started yet. Pump
        // the main loop a little bit so that GDK is in the correct state.
        while (gtk_events_pending())
            gtk_main_iteration();

        // Simulate a drag motion on the top-level GDK window.
        GtkWidget* parentWidget = gtk_widget_get_parent(GTK_WIDGET(view));
        GdkWindow* parentWidgetWindow = gtk_widget_get_window(parentWidget);
        gdk_drag_motion(currentDragSourceContext, parentWidgetWindow, GDK_DRAG_PROTO_XDND,
            event->motion.x_root, event->motion.y_root,
            gdk_drag_context_get_selected_action(currentDragSourceContext),
            gdk_drag_context_get_actions(currentDragSourceContext),
            GDK_CURRENT_TIME);

    } else if (currentDragSourceContext && event->type == GDK_BUTTON_RELEASE) {
        // We've released the mouse button, we should just be able to spin the
        // event loop here and have GTK+ send the appropriate notifications for
        // the end of the drag.
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    gdk_event_free(event);
}

void replaySavedEvents()
{
    // First send all the events that are ready to be sent
    while (startOfQueue < endOfQueue) {
        if (msgQueue[startOfQueue].delay) {
            g_usleep(msgQueue[startOfQueue].delay * 1000);
            msgQueue[startOfQueue].delay = 0;
        }

        dispatchEvent(msgQueue[startOfQueue++].event);
    }

    startOfQueue = 0;
    endOfQueue = 0;
}

static GdkEvent* createKeyPressEvent(JSContextRef context, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    g_return_val_if_fail(argumentCount >= 1, 0);
    guint modifiers = argumentCount >= 2 ? gdkModifersFromJSValue(context, arguments[1]) : 0;

    // handle location argument.
    int location = DOM_KEY_LOCATION_STANDARD;
    if (argumentCount > 2)
        location = (int)JSValueToNumber(context, arguments[2], exception);

    JSStringRef character = JSValueToStringCopy(context, arguments[0], exception);
    g_return_val_if_fail((!exception || !*exception), 0);

    int gdkKeySym = GDK_VoidSymbol;
    if (location == DOM_KEY_LOCATION_NUMPAD) {
        if (JSStringIsEqualToUTF8CString(character, "leftArrow"))
            gdkKeySym = GDK_KP_Left;
        else if (JSStringIsEqualToUTF8CString(character, "rightArrow"))
            gdkKeySym = GDK_KP_Right;
        else if (JSStringIsEqualToUTF8CString(character, "upArrow"))
            gdkKeySym = GDK_KP_Up;
        else if (JSStringIsEqualToUTF8CString(character, "downArrow"))
            gdkKeySym = GDK_KP_Down;
        else if (JSStringIsEqualToUTF8CString(character, "pageUp"))
            gdkKeySym = GDK_KP_Page_Up;
        else if (JSStringIsEqualToUTF8CString(character, "pageDown"))
            gdkKeySym = GDK_KP_Page_Down;
        else if (JSStringIsEqualToUTF8CString(character, "home"))
            gdkKeySym = GDK_KP_Home;
        else if (JSStringIsEqualToUTF8CString(character, "end"))
            gdkKeySym = GDK_KP_End;
        else if (JSStringIsEqualToUTF8CString(character, "insert"))
            gdkKeySym = GDK_KP_Insert;
        else if (JSStringIsEqualToUTF8CString(character, "delete"))
            gdkKeySym = GDK_KP_Delete;
        else
            // If we get some other key specified with the numpad location,
            // crash here, so we add it sooner rather than later.
            g_assert_not_reached();
    } else {
        if (JSStringIsEqualToUTF8CString(character, "leftArrow"))
            gdkKeySym = GDK_Left;
        else if (JSStringIsEqualToUTF8CString(character, "rightArrow"))
            gdkKeySym = GDK_Right;
        else if (JSStringIsEqualToUTF8CString(character, "upArrow"))
            gdkKeySym = GDK_Up;
        else if (JSStringIsEqualToUTF8CString(character, "downArrow"))
            gdkKeySym = GDK_Down;
        else if (JSStringIsEqualToUTF8CString(character, "pageUp"))
            gdkKeySym = GDK_Page_Up;
        else if (JSStringIsEqualToUTF8CString(character, "pageDown"))
            gdkKeySym = GDK_Page_Down;
        else if (JSStringIsEqualToUTF8CString(character, "home"))
            gdkKeySym = GDK_Home;
        else if (JSStringIsEqualToUTF8CString(character, "end"))
            gdkKeySym = GDK_End;
        else if (JSStringIsEqualToUTF8CString(character, "insert"))
            gdkKeySym = GDK_Insert;
        else if (JSStringIsEqualToUTF8CString(character, "delete"))
            gdkKeySym = GDK_Delete;
        else if (JSStringIsEqualToUTF8CString(character, "printScreen"))
            gdkKeySym = GDK_Print;
        else if (JSStringIsEqualToUTF8CString(character, "menu"))
            gdkKeySym = GDK_Menu;
        else if (JSStringIsEqualToUTF8CString(character, "F1"))
            gdkKeySym = GDK_F1;
        else if (JSStringIsEqualToUTF8CString(character, "F2"))
            gdkKeySym = GDK_F2;
        else if (JSStringIsEqualToUTF8CString(character, "F3"))
            gdkKeySym = GDK_F3;
        else if (JSStringIsEqualToUTF8CString(character, "F4"))
            gdkKeySym = GDK_F4;
        else if (JSStringIsEqualToUTF8CString(character, "F5"))
            gdkKeySym = GDK_F5;
        else if (JSStringIsEqualToUTF8CString(character, "F6"))
            gdkKeySym = GDK_F6;
        else if (JSStringIsEqualToUTF8CString(character, "F7"))
            gdkKeySym = GDK_F7;
        else if (JSStringIsEqualToUTF8CString(character, "F8"))
            gdkKeySym = GDK_F8;
        else if (JSStringIsEqualToUTF8CString(character, "F9"))
            gdkKeySym = GDK_F9;
        else if (JSStringIsEqualToUTF8CString(character, "F10"))
            gdkKeySym = GDK_F10;
        else if (JSStringIsEqualToUTF8CString(character, "F11"))
            gdkKeySym = GDK_F11;
        else if (JSStringIsEqualToUTF8CString(character, "F12"))
            gdkKeySym = GDK_F12;
        else if (JSStringIsEqualToUTF8CString(character, "leftAlt"))
            gdkKeySym = GDK_Alt_L;
        else if (JSStringIsEqualToUTF8CString(character, "leftControl"))
            gdkKeySym = GDK_Control_L;
        else if (JSStringIsEqualToUTF8CString(character, "leftShift"))
            gdkKeySym = GDK_Shift_L;
        else if (JSStringIsEqualToUTF8CString(character, "rightAlt"))
            gdkKeySym = GDK_Alt_R;
        else if (JSStringIsEqualToUTF8CString(character, "rightControl"))
            gdkKeySym = GDK_Control_R;
        else if (JSStringIsEqualToUTF8CString(character, "rightShift"))
            gdkKeySym = GDK_Shift_R;
        else {
            int charCode = JSStringGetCharactersPtr(character)[0];
            if (charCode == '\n' || charCode == '\r')
                gdkKeySym = GDK_Return;
            else if (charCode == '\t')
                gdkKeySym = GDK_Tab;
            else if (charCode == '\x8')
                gdkKeySym = GDK_BackSpace;
            else {
                gdkKeySym = gdk_unicode_to_keyval(charCode);
                if (WTF::isASCIIUpper(charCode))
                    modifiers |= GDK_SHIFT_MASK;
            }
        }
    }
    JSStringRelease(character);

    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    g_return_val_if_fail(view, 0);

    GdkEvent* pressEvent = gdk_event_new(GDK_KEY_PRESS);
    pressEvent->key.keyval = gdkKeySym;
    pressEvent->key.state = modifiers;
    pressEvent->key.window = gtk_widget_get_window(GTK_WIDGET(view));
    g_object_ref(pressEvent->key.window);
#ifndef GTK_API_VERSION_2
    gdk_event_set_device(pressEvent, getDefaultGDKPointerDevice(pressEvent->key.window));
#endif

    // When synthesizing an event, an invalid hardware_keycode value
    // can cause it to be badly processed by Gtk+.
    GOwnPtr<GdkKeymapKey> keys;
    gint nKeys;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), gdkKeySym, &keys.outPtr(), &nKeys))
        pressEvent->key.hardware_keycode = keys.get()[0].keycode;

    return pressEvent;
}

static void sendKeyDown(GdkEvent* pressEvent)
{
    g_return_if_fail(pressEvent);
    GdkEvent* releaseEvent = gdk_event_copy(pressEvent);
    releaseEvent->type = GDK_KEY_RELEASE;

    dispatchEvent(pressEvent);
    dispatchEvent(releaseEvent);

    DumpRenderTreeSupportGtk::deliverAllMutationsIfNecessary();
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    GdkEvent* pressEvent = createKeyPressEvent(context, argumentCount, arguments, exception);
    sendKeyDown(pressEvent);

    return JSValueMakeUndefined(context);
}

static void zoomIn(gboolean fullContentsZoom)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return;

    webkit_web_view_set_full_content_zoom(view, fullContentsZoom);
    gfloat currentZoom = webkit_web_view_get_zoom_level(view);
    webkit_web_view_set_zoom_level(view, currentZoom * zoomMultiplierRatio);
}

static void zoomOut(gboolean fullContentsZoom)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return;

    webkit_web_view_set_full_content_zoom(view, fullContentsZoom);
    gfloat currentZoom = webkit_web_view_get_zoom_level(view);
    webkit_web_view_set_zoom_level(view, currentZoom / zoomMultiplierRatio);
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    zoomIn(FALSE);
    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    zoomOut(FALSE);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    zoomIn(TRUE);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    zoomOut(TRUE);
    return JSValueMakeUndefined(context);
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    float scaleFactor = JSValueToNumber(context, arguments[0], exception);
    float x = JSValueToNumber(context, arguments[1], exception);
    float y = JSValueToNumber(context, arguments[2], exception);

    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        return JSValueMakeUndefined(context);

    DumpRenderTreeSupportGtk::scalePageBy(view, scaleFactor, x, y);

    return JSValueMakeUndefined(context);
}

static gboolean sendAsynchronousKeyDown(gpointer userData)
{
    sendKeyDown(static_cast<GdkEvent*>(userData));
    return FALSE;
}

static JSValueRef scheduleAsynchronousKeyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    GdkEvent* pressEvent = createKeyPressEvent(context, argumentCount, arguments, exception);
    if (pressEvent)
        g_timeout_add(0, sendAsynchronousKeyDown, static_cast<gpointer>(pressEvent));

    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseScrollBy", mouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "continuousMouseScrollBy", continuousMouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousClick", scheduleAsynchronousClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousKeyDown", scheduleAsynchronousKeyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },

    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
    { "dragMode", getDragModeCallback, setDragModeCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef context)
{
    static JSClassRef eventSenderClass = 0;

    if (!eventSenderClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticFunctions;
        classDefinition.staticValues = staticValues;

        eventSenderClass = JSClassCreate(&classDefinition);
    }

    return eventSenderClass;
}

JSObjectRef makeEventSender(JSContextRef context, bool isTopFrame)
{
    if (isTopFrame) {
        dragMode = true;

        // Fly forward in time one second when the main frame loads. This will
        // ensure that when a test begins clicking in the same location as
        // a previous test, those clicks won't be interpreted as continuations
        // of the previous test's click sequences.
        timeOffset += 1000;

        lastMousePositionX = lastMousePositionY = 0;
        lastClickPositionX = lastClickPositionY = 0;
        lastClickTimeOffset = 0;
        lastClickButton = 0;
        buttonCurrentlyDown = 0;
        clickCount = 0;

        endOfQueue = 0;
        startOfQueue = 0;

        currentDragSourceContext = 0;
    }

    return JSObjectMake(context, getClass(context), 0);
}

void dragBeginCallback(GtkWidget*, GdkDragContext* context, gpointer)
{
    currentDragSourceContext = context;
}

void dragEndCallback(GtkWidget*, GdkDragContext* context, gpointer)
{
    currentDragSourceContext = 0;
}

gboolean dragFailedCallback(GtkWidget*, GdkDragContext* context, gpointer)
{
    // Return TRUE here to disable the stupid GTK+ drag failed animation,
    // which introduces asynchronous behavior into our drags.
    return TRUE;
}
