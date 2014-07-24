/*
 * Copyright (C) 2011, 2012 Igalia S.L.
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
#include "WebKitAccessibleInterfaceValue.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "HTMLNames.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkValue* value)
{
    if (!WEBKIT_IS_ACCESSIBLE(value))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(value));
}

static void webkitAccessibleValueGetCurrentValue(AtkValue* value, GValue* gValue)
{
    memset(gValue,  0, sizeof(GValue));
    g_value_init(gValue, G_TYPE_FLOAT);
    g_value_set_float(gValue, core(value)->valueForRange());
}

static void webkitAccessibleValueGetMaximumValue(AtkValue* value, GValue* gValue)
{
    memset(gValue,  0, sizeof(GValue));
    g_value_init(gValue, G_TYPE_FLOAT);
    g_value_set_float(gValue, core(value)->maxValueForRange());
}

static void webkitAccessibleValueGetMinimumValue(AtkValue* value, GValue* gValue)
{
    memset(gValue,  0, sizeof(GValue));
    g_value_init(gValue, G_TYPE_FLOAT);
    g_value_set_float(gValue, core(value)->minValueForRange());
}

static gboolean webkitAccessibleValueSetCurrentValue(AtkValue* value, const GValue* gValue)
{
    double newValue;
    if (G_VALUE_HOLDS_DOUBLE(gValue))
        newValue = g_value_get_double(gValue);
    else if (G_VALUE_HOLDS_FLOAT(gValue))
        newValue = g_value_get_float(gValue);
    else if (G_VALUE_HOLDS_INT64(gValue))
        newValue = g_value_get_int64(gValue);
    else if (G_VALUE_HOLDS_INT(gValue))
        newValue = g_value_get_int(gValue);
    else if (G_VALUE_HOLDS_LONG(gValue))
        newValue = g_value_get_long(gValue);
    else if (G_VALUE_HOLDS_ULONG(gValue))
        newValue = g_value_get_ulong(gValue);
    else if (G_VALUE_HOLDS_UINT64(gValue))
        newValue = g_value_get_uint64(gValue);
    else if (G_VALUE_HOLDS_UINT(gValue))
        newValue = g_value_get_uint(gValue);
    else
        return false;

    AccessibilityObject* coreObject = core(value);
    if (!coreObject->canSetValueAttribute())
        return FALSE;

    // Check value against range limits
    newValue = std::max(static_cast<double>(coreObject->minValueForRange()), newValue);
    newValue = std::min(static_cast<double>(coreObject->maxValueForRange()), newValue);

    coreObject->setValue(String::number(newValue));
    return TRUE;
}

static void webkitAccessibleValueGetMinimumIncrement(AtkValue* value, GValue* gValue)
{
    memset(gValue,  0, sizeof(GValue));
    g_value_init(gValue, G_TYPE_FLOAT);

    AccessibilityObject* coreObject = core(value);
    if (!coreObject->getAttribute(HTMLNames::stepAttr).isEmpty()) {
        g_value_set_float(gValue, coreObject->stepValueForRange());
        return;
    }

    // If 'step' attribute is not defined, WebCore assumes a 5% of the
    // range between minimum and maximum values, so return that.
    float range = coreObject->maxValueForRange() - coreObject->minValueForRange();
    g_value_set_float(gValue, range * 0.05);
}

void webkitAccessibleValueInterfaceInit(AtkValueIface* iface)
{
    iface->get_current_value = webkitAccessibleValueGetCurrentValue;
    iface->get_maximum_value = webkitAccessibleValueGetMaximumValue;
    iface->get_minimum_value = webkitAccessibleValueGetMinimumValue;
    iface->set_current_value = webkitAccessibleValueSetCurrentValue;
    iface->get_minimum_increment = webkitAccessibleValueGetMinimumIncrement;
}

#endif
