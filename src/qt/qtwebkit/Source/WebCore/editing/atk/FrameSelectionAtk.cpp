/*
 * Copyright (C) 2009 Igalia S.L.
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
#include "FrameSelection.h"

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"
#include "Document.h"
#include "Frame.h"
#include "WebKitAccessibleWrapperAtk.h"

#if PLATFORM(EFL)
#include <glib.h>
#else
#include <gtk/gtk.h>
#endif

#include <wtf/RefPtr.h>

namespace WebCore {

static void emitTextSelectionChange(AccessibilityObject* object, VisibleSelection selection, int offset)
{
    AtkObject* axObject = object->wrapper();
    if (!axObject || !ATK_IS_TEXT(axObject))
        return;

    g_signal_emit_by_name(axObject, "text-caret-moved", offset);
    if (selection.isRange())
        g_signal_emit_by_name(axObject, "text-selection-changed");
}

static void maybeEmitTextFocusChange(PassRefPtr<AccessibilityObject> prpObject)
{
    // This static variable is needed to keep track of the old object
    // as per previous calls to this function, in order to properly
    // decide whether to emit some signals or not.
    DEFINE_STATIC_LOCAL(RefPtr<AccessibilityObject>, oldObject, ());

    RefPtr<AccessibilityObject> object = prpObject;

    // Ensure the oldObject belongs to the same document that the
    // current object so further comparisons make sense. Otherwise,
    // just reset oldObject to 0 so it won't be taken into account in
    // the immediately following call to this function.
    if (object && oldObject && oldObject->document() != object->document())
        oldObject = 0;

    AtkObject* axObject = object ? object->wrapper() : 0;
    AtkObject* oldAxObject = oldObject ? oldObject->wrapper() : 0;

    if (axObject != oldAxObject) {
        if (oldAxObject && ATK_IS_TEXT(oldAxObject)) {
            g_signal_emit_by_name(oldAxObject, "focus-event", false);
            g_signal_emit_by_name(oldAxObject, "state-change", "focused", false);
        }
        if (axObject && ATK_IS_TEXT(axObject)) {
            g_signal_emit_by_name(axObject, "focus-event", true);
            g_signal_emit_by_name(axObject, "state-change", "focused", true);
        }
    }

    // Update pointer to last focused object.
    oldObject = object;
}


void FrameSelection::notifyAccessibilityForSelectionChange()
{
    if (!AXObjectCache::accessibilityEnabled())
        return;

    if (!m_selection.start().isNotNull() || !m_selection.end().isNotNull())
        return;

    RenderObject* focusedNode = m_selection.end().containerNode()->renderer();
    AXObjectCache* cache = m_frame->document()->existingAXObjectCache();
    if (!cache)
        return;

    AccessibilityObject* accessibilityObject = cache->getOrCreate(focusedNode);
    if (!accessibilityObject)
        return;

    int offset;
    RefPtr<AccessibilityObject> object = objectFocusedAndCaretOffsetUnignored(accessibilityObject, offset);
    if (!object)
        return;

    emitTextSelectionChange(object.get(), m_selection, offset);
    maybeEmitTextFocusChange(object.release());
}

} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
