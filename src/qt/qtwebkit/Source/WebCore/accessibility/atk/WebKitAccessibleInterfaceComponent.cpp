/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2012 Igalia S.L.
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
#include "WebKitAccessibleInterfaceComponent.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "FrameView.h"
#include "IntRect.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkComponent* component)
{
    if (!WEBKIT_IS_ACCESSIBLE(component))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(component));
}

static IntPoint atkToContents(AccessibilityObject* coreObject, AtkCoordType coordType, gint x, gint y)
{
    IntPoint pos(x, y);

    FrameView* frameView = coreObject->documentFrameView();
    if (frameView) {
        switch (coordType) {
        case ATK_XY_SCREEN:
            return frameView->screenToContents(pos);
        case ATK_XY_WINDOW:
            return frameView->windowToContents(pos);
        }
    }

    return pos;
}

static AtkObject* webkitAccessibleComponentRefAccessibleAtPoint(AtkComponent* component, gint x, gint y, AtkCoordType coordType)
{
    IntPoint pos = atkToContents(core(component), coordType, x, y);

    AccessibilityObject* target = core(component)->accessibilityHitTest(pos);
    if (!target)
        return 0;
    g_object_ref(target->wrapper());
    return target->wrapper();
}

static void webkitAccessibleComponentGetExtents(AtkComponent* component, gint* x, gint* y, gint* width, gint* height, AtkCoordType coordType)
{
    IntRect rect = pixelSnappedIntRect(core(component)->elementRect());
    contentsRelativeToAtkCoordinateType(core(component), coordType, rect, x, y, width, height);
}

static gboolean webkitAccessibleComponentGrabFocus(AtkComponent* component)
{
    core(component)->setFocused(true);
    return core(component)->isFocused();
}

void webkitAccessibleComponentInterfaceInit(AtkComponentIface* iface)
{
    iface->ref_accessible_at_point = webkitAccessibleComponentRefAccessibleAtPoint;
    iface->get_extents = webkitAccessibleComponentGetExtents;
    iface->grab_focus = webkitAccessibleComponentGrabFocus;
}

#endif
