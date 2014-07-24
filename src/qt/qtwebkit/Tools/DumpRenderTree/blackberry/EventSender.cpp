/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2009, 2010, 2012 Research In Motion Limited. All rights reserved.
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
#include "EventSender.h"

#include "DumpRenderTreeBlackBerry.h"
#include "DumpRenderTreeSupport.h"
#include "IntPoint.h"
#include "NotImplemented.h"
#include "WebKitThreadViewportAccessor.h"
#include "WebPage.h"

#include <BlackBerryPlatformKeyboardEvent.h>
#include <BlackBerryPlatformMouseEvent.h>
#include <BlackBerryPlatformTouchEvent.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>
#include <wtf/Vector.h>

using namespace WebCore;

static IntPoint lastMousePosition;
static Vector<BlackBerry::Platform::TouchPoint> touches;
static bool touchActive = false;

void sendTouchEvent(BlackBerry::Platform::TouchEvent::Type);

// Callbacks

static JSValueRef getDragModeCallback(JSContextRef context, JSObjectRef, JSStringRef, JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static bool setDragModeCallback(JSContextRef context, JSObjectRef, JSStringRef, JSValueRef, JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseWheelToCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

void setMouseEventDocumentPos(BlackBerry::Platform::MouseEvent &event, const BlackBerry::WebKit::WebPage* page)
{
    // We have added document viewport position and document content position as members of the mouse event, when we create the event, we should initialize them as well.
    BlackBerry::Platform::ViewportAccessor* viewportAccessor = page->webkitThreadViewportAccessor();
    IntPoint documentContentPos = viewportAccessor->roundToDocumentFromPixelContents(BlackBerry::Platform::FloatPoint(viewportAccessor->pixelContentsFromViewport(lastMousePosition)));
    IntPoint documentViewportMousePos = viewportAccessor->roundToDocumentFromPixelContents(BlackBerry::Platform::FloatPoint(lastMousePosition));
    event.populateDocumentPosition(documentViewportMousePos, documentContentPos);
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    BlackBerry::Platform::MouseEvent event(BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, 0, lastMousePosition, IntPoint::zero(), 0, 0, 0);

    setMouseEventDocumentPos(event, page);

    page->mouseEvent(event);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    BlackBerry::Platform::MouseEvent event(0, BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, lastMousePosition, IntPoint::zero(), 0, 0, 0);

    setMouseEventDocumentPos(event, page);

    page->mouseEvent(event);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int x = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    lastMousePosition = IntPoint(x, y);
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    BlackBerry::Platform::MouseEvent event(BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, lastMousePosition, IntPoint::zero(), 0, 0, 0);

    setMouseEventDocumentPos(event, page);

    page->mouseEvent(event);

    return JSValueMakeUndefined(context);
}

static JSValueRef beginDragWithFilesCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSStringRef character = JSValueToStringCopy(context, arguments[0], exception);
    ASSERT(!*exception);
    unsigned charCode = 0;
    bool needsShiftKeyModifier = false;
    if (JSStringIsEqualToUTF8CString(character, "leftArrow"))
        charCode = KEYCODE_LEFT;
    else if (JSStringIsEqualToUTF8CString(character, "rightArrow"))
        charCode = KEYCODE_RIGHT;
    else if (JSStringIsEqualToUTF8CString(character, "upArrow"))
        charCode = KEYCODE_UP;
    else if (JSStringIsEqualToUTF8CString(character, "downArrow"))
        charCode = KEYCODE_DOWN;
    else if (JSStringIsEqualToUTF8CString(character, "pageUp"))
        charCode = KEYCODE_PG_UP;
    else if (JSStringIsEqualToUTF8CString(character, "pageDown"))
        charCode = KEYCODE_PG_DOWN;
    else if (JSStringIsEqualToUTF8CString(character, "home"))
        charCode = KEYCODE_HOME;
    else if (JSStringIsEqualToUTF8CString(character, "end"))
        charCode = KEYCODE_END;
    else if (JSStringIsEqualToUTF8CString(character, "delete"))
        charCode = KEYCODE_BACKSPACE;
    else {
        charCode = JSStringGetCharactersPtr(character)[0];
        if (0x8 == charCode)
            charCode = KEYCODE_BACKSPACE;
        else if (0x7F == charCode)
            charCode = KEYCODE_DELETE;
        else if (WTF::isASCIIUpper(charCode))
            needsShiftKeyModifier = true;
    }
    JSStringRelease(character);

    static const JSStringRef lengthProperty = JSStringCreateWithUTF8CString("length");
    bool needsAltKeyModifier = false;
    bool needsCtrlKeyModifier = false;
    if (argumentCount > 1) {
        if (JSObjectRef modifiersArray = JSValueToObject(context, arguments[1], 0)) {
            int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty, 0), 0);
            for (int i = 0; i < modifiersCount; ++i) {
                JSStringRef string = JSValueToStringCopy(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0), 0);
                if (JSStringIsEqualToUTF8CString(string, "shiftKey"))
                    needsShiftKeyModifier = true;
                else if (JSStringIsEqualToUTF8CString(string, "altKey"))
                    needsAltKeyModifier = true;
                else if (JSStringIsEqualToUTF8CString(string, "ctrlKey"))
                    needsCtrlKeyModifier = true;
                JSStringRelease(string);
            }
        }
    }

    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();

    unsigned modifiers = 0;
    if (needsShiftKeyModifier)
        modifiers |= KEYMOD_SHIFT;
    if (needsAltKeyModifier)
        modifiers |= KEYMOD_ALT;
    if (needsCtrlKeyModifier)
        modifiers |= KEYMOD_CTRL;

    page->keyEvent(BlackBerry::Platform::KeyboardEvent(charCode, BlackBerry::Platform::KeyboardEvent::KeyDown, modifiers));
    page->keyEvent(BlackBerry::Platform::KeyboardEvent(charCode, BlackBerry::Platform::KeyboardEvent::KeyUp, modifiers));

    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef addTouchPointCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int x = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    int id = touches.isEmpty() ? 0 : touches.last().id() + 1;

    // pixelViewportPosition is unused in the WebKit layer, so use this for screen position
    IntPoint pos(x, y);

    BlackBerry::Platform::TouchPoint touch(id, BlackBerry::Platform::TouchPoint::TouchPressed, pos, pos, 0);

    // Unfortunately we don't know the scroll position at this point, so use pos for the content position too.
    // This assumes scroll position is 0,0
    touch.populateDocumentPosition(pos, pos);

    touches.append(touch);

    return JSValueMakeUndefined(context);
}

static JSValueRef updateTouchPointCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int x = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[2], exception));
    ASSERT(!exception || !*exception);

    if (index < 0 || index >= (int)touches.size())
        return JSValueMakeUndefined(context);

    BlackBerry::Platform::TouchPoint& touch = touches[index];

    // pixelViewportPosition is unused in the WebKit layer
    IntPoint pos(x, y);

    // Unfortunately we don't know the scroll position at this point, so use pos for the content position too.
    // This assumes scroll position is 0,0
    touch.populateDocumentPosition(pos, pos);
    touch.setScreenPosition(pos);
    touch.updateState(BlackBerry::Platform::TouchPoint::TouchMoved);

    return JSValueMakeUndefined(context);
}

static JSValueRef setTouchModifierCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef touchStartCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    if (!touchActive) {
        sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchStart);
        touchActive = true;
    } else
        sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
    return JSValueMakeUndefined(context);
}

static JSValueRef touchCancelCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef touchMoveCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
    return JSValueMakeUndefined(context);
}

static JSValueRef touchEndCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    for (unsigned i = 0; i < touches.size(); ++i)
        if (touches[i].state() != BlackBerry::Platform::TouchPoint::TouchReleased) {
            sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
            return JSValueMakeUndefined(context);
        }
    sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchEnd);
    touchActive = false;
    return JSValueMakeUndefined(context);
}

static JSValueRef clearTouchPointsCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    touches.clear();
    touchActive = false;
    return JSValueMakeUndefined(context);
}

static JSValueRef cancelTouchPointCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef releaseTouchPointCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    if (index < 0 || index >= (int)touches.size())
        return JSValueMakeUndefined(context);

    touches[index].updateState(BlackBerry::Platform::TouchPoint::TouchReleased);
    return JSValueMakeUndefined(context);
}

void sendTouchEvent(BlackBerry::Platform::TouchEvent::Type type)
{
    BlackBerry::Platform::TouchEvent event;
    event.m_type = type;
    event.m_points.assign(touches.begin(), touches.end());
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->touchEvent(event);

    Vector<BlackBerry::Platform::TouchPoint> t;

    for (Vector<BlackBerry::Platform::TouchPoint>::iterator it = touches.begin(); it != touches.end(); ++it) {
        if (it->state() != BlackBerry::Platform::TouchPoint::TouchReleased) {
            it->updateState(BlackBerry::Platform::TouchPoint::TouchStationary);
            t.append(*it);
        }
    }
    touches = t;
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    float scaleFactor = JSValueToNumber(context, arguments[0], exception);
    float x = JSValueToNumber(context, arguments[1], exception);
    float y = JSValueToNumber(context, arguments[2], exception);

    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    if (!page)
        return JSValueMakeUndefined(context);

    DumpRenderTreeSupport::scalePageBy(page, scaleFactor, x, y);

    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseWheelTo", mouseWheelToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "addTouchPoint", addTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "cancelTouchPoint", cancelTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "clearTouchPoints", clearTouchPointsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "releaseTouchPoint", releaseTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "setTouchModifier", setTouchModifierCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchCancel", touchCancelCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchEnd", touchEndCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchMove", touchMoveCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchStart", touchStartCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "updateTouchPoint", updateTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
    { "dragMode", getDragModeCallback, setDragModeCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef)
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

void replaySavedEvents()
{
    notImplemented();
}

JSObjectRef makeEventSender(JSContextRef context)
{
    return JSObjectMake(context, getClass(context), 0);
}

