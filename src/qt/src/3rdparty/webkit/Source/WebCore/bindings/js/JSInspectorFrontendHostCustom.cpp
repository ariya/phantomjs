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
#include "JSInspectorFrontendHost.h"

#if ENABLE(INSPECTOR)

#include "ContextMenuItem.h"
#include "InspectorController.h"
#include "InspectorFrontendHost.h"
#include "JSEvent.h"
#include "MouseEvent.h"
#include "PlatformString.h"
#include <runtime/JSArray.h>
#include <runtime/JSLock.h>
#include <runtime/JSObject.h>
#include <wtf/Vector.h>

using namespace JSC;

namespace WebCore {

JSValue JSInspectorFrontendHost::platform(ExecState* execState)
{
#if PLATFORM(MAC)
    DEFINE_STATIC_LOCAL(const String, platform, ("mac"));
#elif OS(WINDOWS)
    DEFINE_STATIC_LOCAL(const String, platform, ("windows"));
#elif OS(LINUX)
    DEFINE_STATIC_LOCAL(const String, platform, ("linux"));
#elif OS(FREEBSD)
    DEFINE_STATIC_LOCAL(const String, platform, ("freebsd"));
#elif OS(OPENBSD)
    DEFINE_STATIC_LOCAL(const String, platform, ("openbsd"));
#elif OS(SOLARIS)
    DEFINE_STATIC_LOCAL(const String, platform, ("solaris"));
#else
    DEFINE_STATIC_LOCAL(const String, platform, ("unknown"));
#endif
    return jsString(execState, platform);
}

JSValue JSInspectorFrontendHost::port(ExecState* execState)
{
#if PLATFORM(QT)
    DEFINE_STATIC_LOCAL(const String, port, ("qt"));
#elif PLATFORM(GTK)
    DEFINE_STATIC_LOCAL(const String, port, ("gtk"));
#elif PLATFORM(WX)
    DEFINE_STATIC_LOCAL(const String, port, ("wx"));
#else
    DEFINE_STATIC_LOCAL(const String, port, ("unknown"));
#endif
    return jsString(execState, port);
}

JSValue JSInspectorFrontendHost::showContextMenu(ExecState* exec)
{
    if (exec->argumentCount() < 2)
        return jsUndefined();
#if ENABLE(CONTEXT_MENUS)
    Event* event = toEvent(exec->argument(0));

    JSArray* array = asArray(exec->argument(1));
    Vector<ContextMenuItem*> items;

    for (size_t i = 0; i < array->length(); ++i) {
        JSObject* item = asObject(array->getIndex(i));
        JSValue label = item->get(exec, Identifier(exec, "label"));
        JSValue type = item->get(exec, Identifier(exec, "type"));
        JSValue id = item->get(exec, Identifier(exec, "id"));
        JSValue enabled = item->get(exec, Identifier(exec, "enabled"));
        JSValue checked = item->get(exec, Identifier(exec, "checked"));
        if (!type.isString())
            continue;

        String typeString = ustringToString(type.toString(exec));
        if (typeString == "separator") {
            items.append(new ContextMenuItem(SeparatorType,
                                             ContextMenuItemCustomTagNoAction,
                                             String()));
        } else {
            ContextMenuAction typedId = static_cast<ContextMenuAction>(ContextMenuItemBaseCustomTag + id.toInt32(exec));
            ContextMenuItem* menuItem = new ContextMenuItem((typeString == "checkbox" ? CheckableActionType : ActionType), typedId, ustringToString(label.toString(exec)));
            if (!enabled.isUndefined())
                menuItem->setEnabled(enabled.toBoolean(exec));
            if (!checked.isUndefined())
                menuItem->setChecked(checked.toBoolean(exec));
            items.append(menuItem);
        }
    }

    impl()->showContextMenu(event, items);
#endif
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
