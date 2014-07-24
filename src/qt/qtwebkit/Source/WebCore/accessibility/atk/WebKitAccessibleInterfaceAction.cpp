/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2012 Igalia S.L.
 * Copyright (C) 2013 Samsung Electronics
 *
 * Portions from Mozilla a11y, copyright as follows:
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitAccessibleInterfaceAction.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "NotImplemented.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkAction* action)
{
    if (!WEBKIT_IS_ACCESSIBLE(action))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(action));
}

static gboolean webkitAccessibleActionDoAction(AtkAction* action, gint index)
{
    g_return_val_if_fail(!index, FALSE);
    return core(action)->performDefaultAction();
}

static gint webkitAccessibleActionGetNActions(AtkAction*)
{
    return 1;
}

static const gchar* webkitAccessibleActionGetDescription(AtkAction*, gint)
{
    // TODO: Need a way to provide/localize action descriptions.
    notImplemented();
    return "";
}

static const gchar* webkitAccessibleActionGetKeybinding(AtkAction* action, gint index)
{
    g_return_val_if_fail(!index, 0);
    // FIXME: Construct a proper keybinding string.
    return cacheAndReturnAtkProperty(ATK_OBJECT(action), AtkCachedActionKeyBinding, core(action)->accessKey().string());
}

static const gchar* webkitAccessibleActionGetName(AtkAction* action, gint index)
{
    g_return_val_if_fail(!index, 0);
    return cacheAndReturnAtkProperty(ATK_OBJECT(action), AtkCachedActionName, core(action)->actionVerb());
}

void webkitAccessibleActionInterfaceInit(AtkActionIface* iface)
{
    iface->do_action = webkitAccessibleActionDoAction;
    iface->get_n_actions = webkitAccessibleActionGetNActions;
    iface->get_description = webkitAccessibleActionGetDescription;
    iface->get_keybinding = webkitAccessibleActionGetKeybinding;
    iface->get_name = webkitAccessibleActionGetName;
}

#endif
