/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2012 Igalia S.L.
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
#include "WebKitAccessibleInterfaceImage.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "IntRect.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkImage* image)
{
    if (!WEBKIT_IS_ACCESSIBLE(image))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(image));
}

static void webkitAccessibleImageGetImagePosition(AtkImage* image, gint* x, gint* y, AtkCoordType coordType)
{
    IntRect rect = pixelSnappedIntRect(core(image)->elementRect());
    contentsRelativeToAtkCoordinateType(core(image), coordType, rect, x, y);
}

static const gchar* webkitAccessibleImageGetImageDescription(AtkImage* image)
{
    return cacheAndReturnAtkProperty(ATK_OBJECT(image), AtkCachedImageDescription, accessibilityDescription(core(image)));
}

static void webkitAccessibleImageGetImageSize(AtkImage* image, gint* width, gint* height)
{
    IntSize size = core(image)->pixelSnappedSize();

    if (width)
        *width = size.width();
    if (height)
        *height = size.height();
}

void webkitAccessibleImageInterfaceInit(AtkImageIface* iface)
{
    iface->get_image_position = webkitAccessibleImageGetImagePosition;
    iface->get_image_description = webkitAccessibleImageGetImageDescription;
    iface->get_image_size = webkitAccessibleImageGetImageSize;
}

#endif
