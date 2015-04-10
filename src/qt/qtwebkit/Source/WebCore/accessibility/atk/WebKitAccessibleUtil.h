/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2010, 2012 Igalia S.L.
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

#ifndef WebKitAccessibleUtil_h
#define WebKitAccessibleUtil_h

#if HAVE(ACCESSIBILITY)

#include <atk/atk.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class AccessibilityObject;
class IntRect;
class VisibleSelection;
}

AtkAttributeSet* addToAtkAttributeSet(AtkAttributeSet*, const char* name, const char* value);

void contentsRelativeToAtkCoordinateType(WebCore::AccessibilityObject*, AtkCoordType, WebCore::IntRect, gint* x, gint* y, gint* width = 0, gint* height = 0);

String accessibilityTitle(WebCore::AccessibilityObject*);

String accessibilityDescription(WebCore::AccessibilityObject*);

bool selectionBelongsToObject(WebCore::AccessibilityObject*, WebCore::VisibleSelection&);

#endif
#endif // WebKitAccessibleUtil_h
