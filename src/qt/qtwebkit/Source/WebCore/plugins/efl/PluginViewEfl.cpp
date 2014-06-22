/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2011 Samsung Electronics
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

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HostWindow.h"
#include "JSDOMWindowBase.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "PluginPackage.h"
#include "ScriptController.h"
#include "npruntime_impl.h"
#include "runtime/JSLock.h"
#include "runtime/Operations.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Evas.h>

namespace WebCore {

using namespace HTMLNames;

bool PluginView::dispatchNPEvent(NPEvent& event)
{
    if (!m_plugin->pluginFuncs()->event)
        return false;

    PluginView::setCurrentPluginView(this);
    JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
    setCallingPlugin(true);

    bool accepted = m_plugin->pluginFuncs()->event(m_instance, &event);

    setCallingPlugin(false);
    PluginView::setCurrentPluginView(0);
    return accepted;
}

#if defined(XP_UNIX)
void PluginView::handleFocusInEvent()
{
    notImplemented();
}

void PluginView::handleFocusOutEvent()
{
    notImplemented();
}
#endif

void PluginView::handleKeyboardEvent(KeyboardEvent*)
{
    notImplemented();
}

void PluginView::handleMouseEvent(MouseEvent* event)
{
    NPEvent xEvent;

#if defined(XP_UNIX)
    const IntRect rect = parent()->contentsToScreen(IntRect(0, 0, event->offsetX(), event->offsetY()));
    const int eventX = rect.width();
    const int eventY = rect.height();
    if (event->type() == eventNames().mousemoveEvent
            || event->type() == eventNames().mouseoutEvent
            || event->type() == eventNames().mouseoverEvent) {
        xEvent.type = MotionNotify;
        xEvent.xmotion.x = eventX;
        xEvent.xmotion.y = eventY;
    } else if (event->type() == eventNames().mousedownEvent) {
        xEvent.type = ButtonPress;
        xEvent.xbutton.x = eventX;
        xEvent.xbutton.y = eventY;
        xEvent.xbutton.button = event->button() + 1; // DOM MouseEvent counts from 0, and XButtonEvent from 1.
    } else if (event->type() == eventNames().mouseupEvent) {
        xEvent.type = ButtonRelease;
        xEvent.xbutton.x = eventX;
        xEvent.xbutton.y = eventY;
        xEvent.xbutton.button = event->button() + 1;
    } else
        return;
#endif

    if (dispatchNPEvent(xEvent))
        event->setDefaultHandled();
}

void PluginView::updatePluginWidget()
{
    notImplemented();
}

void PluginView::setFocus(bool focused)
{
    if (focused)
        m_element->document()->setFocusedElement(m_element);

    Widget::setFocus(focused);
}

void PluginView::show()
{
    setSelfVisible(true);

    Widget::show();
}

void PluginView::hide()
{
    setSelfVisible(false);

    Widget::hide();
}

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_isStarted)
        paintMissingPluginIcon(context, rect);
}

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent)
        init();
}

void PluginView::setNPWindowRect(const IntRect&)
{
    notImplemented();
}

void PluginView::setNPWindowIfNeeded()
{
}

void PluginView::setParentVisible(bool visible)
{
    Widget::setParentVisible(visible);
}

NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    String filename(buf, len);

    if (filename.startsWith("file:///"))
#if defined(XP_UNIX)
        filename = filename.substring(7);
#else
        filename = filename.substring(8);
#endif

    long long size;
    if (!getFileSize(filename, size))
        return NPERR_FILE_NOT_FOUND;

    FILE* fileHandle = fopen(filename.utf8().data(), "r");
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
        *static_cast<NPBool*>(value) = false;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsWindowless:
        *static_cast<NPBool*>(value) = false;
        *result = NPERR_NO_ERROR;
        return true;

    default:
        return false;
    }
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    if (!value || !result)
        return false;

    switch (variable) {
    case NPNVxDisplay:
        *static_cast<void**>(value) = static_cast<Display*>(ecore_x_display_get());
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVxtAppContext:
        *result = NPERR_GENERIC_ERROR;
        return true;

    case NPNVWindowNPObject: {
        if (m_isJavaScriptPaused) {
            *result = NPERR_GENERIC_ERROR;
            return true;
        }

        NPObject* windowScriptObject = m_parentFrame->script()->windowScriptNPObject();

        // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugin/npruntime.html>
        if (windowScriptObject)
            _NPN_RetainObject(windowScriptObject);

        *static_cast<void**>(value) = windowScriptObject;

        *result = NPERR_NO_ERROR;
        return true;
    }

    case NPNVPluginElementNPObject: {
        if (m_isJavaScriptPaused) {
            *result = NPERR_GENERIC_ERROR;
            return true;
        }

        NPObject* pluginScriptObject = 0;

        if (m_element->hasTagName(appletTag) || m_element->hasTagName(embedTag) || m_element->hasTagName(objectTag))
            pluginScriptObject = static_cast<HTMLPlugInElement*>(m_element)->getNPObject();

        // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugin/npruntime.html>
        if (pluginScriptObject)
            _NPN_RetainObject(pluginScriptObject);

        *static_cast<void**>(value) = pluginScriptObject;

        *result = NPERR_NO_ERROR;
        return true;
    }

    case NPNVnetscapeWindow: {
        Evas* evas = evas_object_evas_get(m_parentFrame->view()->evasObject());
        if (!evas)
            return false;

        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(evas);
        *static_cast<XID*>(value) = static_cast<Window>(ecore_evas_window_get(ecoreEvas));
        *result = NPERR_NO_ERROR;
        return true;
    }

    case NPNVToolkit:
        if (m_plugin->quirks().contains(PluginQuirkRequiresGtkToolKit)) {
            *static_cast<uint32_t*>(value) = 2;
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

    invalidateRect(IntRect(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top));
}

void PluginView::invalidateRegion(NPRegion)
{
    notImplemented();
}

void PluginView::forceRedraw()
{
    notImplemented();
}

bool PluginView::platformStart()
{
    ASSERT(m_isStarted);
    ASSERT(m_status == PluginStatusLoadedSuccessfully);

    notImplemented();
    return true;
}

void PluginView::platformDestroy()
{
}

} // namespace WebCore
