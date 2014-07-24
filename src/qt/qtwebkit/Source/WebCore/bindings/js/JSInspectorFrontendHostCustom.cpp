/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "JSInspectorFrontendHost.h"

#include "ContextMenuItem.h"
#include "InspectorController.h"
#include "InspectorFrontendHost.h"
#include "JSEvent.h"
#include "MouseEvent.h"
#include <runtime/JSArray.h>
#include <runtime/JSLock.h>
#include <runtime/JSObject.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

using namespace JSC;

namespace WebCore {

JSValue JSInspectorFrontendHost::platform(ExecState* execState)
{
#if PLATFORM(MAC)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("mac")));
#elif OS(WINDOWS)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("windows")));
#elif OS(LINUX)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("linux")));
#elif OS(FREEBSD)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("freebsd")));
#elif OS(OPENBSD)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("openbsd")));
#elif OS(SOLARIS)
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("solaris")));
#else
    DEFINE_STATIC_LOCAL(const String, platform, (ASCIILiteral("unknown")));
#endif
    return jsStringWithCache(execState, platform);
}

JSValue JSInspectorFrontendHost::port(ExecState* execState)
{
#if PLATFORM(QT)
    DEFINE_STATIC_LOCAL(const String, port, (ASCIILiteral("qt")));
#elif PLATFORM(GTK)
    DEFINE_STATIC_LOCAL(const String, port, (ASCIILiteral("gtk")));
#elif PLATFORM(EFL)
    DEFINE_STATIC_LOCAL(const String, port, (ASCIILiteral("efl")));
#else
    DEFINE_STATIC_LOCAL(const String, port, (ASCIILiteral("unknown")));
#endif
    return jsStringWithCache(execState, port);
}

#if ENABLE(CONTEXT_MENUS)
static void populateContextMenuItems(ExecState* exec, JSArray* array, ContextMenu& menu)
{
    for (size_t i = 0; i < array->length(); ++i) {
        JSObject* item = asObject(array->getIndex(exec, i));
        JSValue label = item->get(exec, Identifier(exec, "label"));
        JSValue type = item->get(exec, Identifier(exec, "type"));
        JSValue id = item->get(exec, Identifier(exec, "id"));
        JSValue enabled = item->get(exec, Identifier(exec, "enabled"));
        JSValue checked = item->get(exec, Identifier(exec, "checked"));
        JSValue subItems = item->get(exec, Identifier(exec, "subItems"));
        if (!type.isString())
            continue;

        String typeString = type.toString(exec)->value(exec);
        if (typeString == "separator") {
            ContextMenuItem item(SeparatorType,
                                 ContextMenuItemCustomTagNoAction,
                                 String());
            menu.appendItem(item);
        } else if (typeString == "subMenu" && subItems.inherits(&JSArray::s_info)) {
            ContextMenu subMenu;
            JSArray* subItemsArray = asArray(subItems);
            populateContextMenuItems(exec, subItemsArray, subMenu);
            ContextMenuItem item(SubmenuType,
                                 ContextMenuItemCustomTagNoAction,
                                 label.toString(exec)->value(exec),
                                 &subMenu);
            menu.appendItem(item);
        } else {
            ContextMenuAction typedId = static_cast<ContextMenuAction>(ContextMenuItemBaseCustomTag + id.toInt32(exec));
            ContextMenuItem menuItem((typeString == "checkbox" ? CheckableActionType : ActionType), typedId, label.toString(exec)->value(exec));
            if (!enabled.isUndefined())
                menuItem.setEnabled(enabled.toBoolean(exec));
            if (!checked.isUndefined())
                menuItem.setChecked(checked.toBoolean(exec));
            menu.appendItem(menuItem);
        }
    }
}
#endif

JSValue JSInspectorFrontendHost::showContextMenu(ExecState* exec)
{
#if ENABLE(CONTEXT_MENUS)
    if (exec->argumentCount() < 2)
        return jsUndefined();
    Event* event = toEvent(exec->argument(0));

    JSArray* array = asArray(exec->argument(1));
    ContextMenu menu;
    populateContextMenuItems(exec, array, menu);

#if !USE(CROSS_PLATFORM_CONTEXT_MENUS)
    Vector<ContextMenuItem> items = contextMenuItemVector(menu.platformDescription());
#else
    Vector<ContextMenuItem> items = menu.items();
#endif
    impl()->showContextMenu(event, items);
#else
    UNUSED_PARAM(exec);
#endif
    return jsUndefined();
}

JSValue JSInspectorFrontendHost::recordActionTaken(ExecState*)
{
    return jsUndefined();
}

JSValue JSInspectorFrontendHost::recordPanelShown(ExecState*)
{
    return jsUndefined();
}

JSValue JSInspectorFrontendHost::recordSettingChanged(ExecState*)
{
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
