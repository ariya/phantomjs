/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2011, 2012 Igalia S.L.
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
#include "WebKitAccessibleInterfaceEditableText.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "Document.h"
#include "Editor.h"
#include "Frame.h"
#include "NotImplemented.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkEditableText* text)
{
    if (!WEBKIT_IS_ACCESSIBLE(text))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(text));
}

static gboolean webkitAccessibleEditableTextSetRunAttributes(AtkEditableText*, AtkAttributeSet*, gint, gint)
{
    notImplemented();
    return FALSE;
}

static void webkitAccessibleEditableTextSetTextContents(AtkEditableText* text, const gchar* string)
{
    // FIXME: string nullcheck?
    core(text)->setValue(String::fromUTF8(string));
}

static void webkitAccessibleEditableTextInsertText(AtkEditableText* text, const gchar* string, gint length, gint* position)
{
    if (!string)
        return;

    AccessibilityObject* coreObject = core(text);
    // FIXME: Not implemented in WebCore
    // coreObject->setSelectedTextRange(PlainTextRange(*position, 0));
    // coreObject->setSelectedText(String::fromUTF8(string));

    Document* document = coreObject->document();
    if (!document || !document->frame())
        return;

    coreObject->setSelectedVisiblePositionRange(coreObject->visiblePositionRangeForRange(PlainTextRange(*position, 0)));
    coreObject->setFocused(true);
    // FIXME: We should set position to the actual inserted text length, which may be less than that requested.
    if (document->frame()->editor().insertTextWithoutSendingTextEvent(String::fromUTF8(string).substring(0, length), false, 0))
        *position += length;
}

static void webkitAccessibleEditableTextCopyText(AtkEditableText*, gint, gint)
{
    notImplemented();
}

static void webkitAccessibleEditableTextCutText(AtkEditableText*, gint, gint)
{
    notImplemented();
}

static void webkitAccessibleEditableTextDeleteText(AtkEditableText* text, gint startPos, gint endPos)
{
    AccessibilityObject* coreObject = core(text);
    // FIXME: Not implemented in WebCore
    // coreObject->setSelectedTextRange(PlainTextRange(startPos, endPos - startPos));
    // coreObject->setSelectedText(String());

    Document* document = coreObject->document();
    if (!document || !document->frame())
        return;

    coreObject->setSelectedVisiblePositionRange(coreObject->visiblePositionRangeForRange(PlainTextRange(startPos, endPos - startPos)));
    coreObject->setFocused(true);
    document->frame()->editor().performDelete();
}

static void webkitAccessibleEditableTextPasteText(AtkEditableText*, gint)
{
    notImplemented();
}

void webkitAccessibleEditableTextInterfaceInit(AtkEditableTextIface* iface)
{
    iface->set_run_attributes = webkitAccessibleEditableTextSetRunAttributes;
    iface->set_text_contents = webkitAccessibleEditableTextSetTextContents;
    iface->insert_text = webkitAccessibleEditableTextInsertText;
    iface->copy_text = webkitAccessibleEditableTextCopyText;
    iface->cut_text = webkitAccessibleEditableTextCutText;
    iface->delete_text = webkitAccessibleEditableTextDeleteText;
    iface->paste_text = webkitAccessibleEditableTextPasteText;
}

#endif
