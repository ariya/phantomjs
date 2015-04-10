/*
 * Copyright (C) 2008 Apple Ltd.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
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
#include "AccessibilityObject.h"

#include "RenderObject.h"
#include "RenderText.h"
#include <glib-object.h>

#if HAVE(ACCESSIBILITY)

namespace WebCore {

bool AccessibilityObject::accessibilityIgnoreAttachment() const
{
    return false;
}

AccessibilityObjectInclusion AccessibilityObject::accessibilityPlatformIncludesObject() const
{
    AccessibilityObject* parent = parentObject();
    if (!parent)
        return DefaultBehavior;

    AccessibilityRole role = roleValue();
    if (role == HorizontalRuleRole)
        return IncludeObject;

    // We expose the slider as a whole but not its value indicator.
    if (role == SliderThumbRole)
        return IgnoreObject;

    // When a list item is made up entirely of children (e.g. paragraphs)
    // the list item gets ignored. We need it.
    if (isGroup() && parent->isList())
        return IncludeObject;

    // Entries and password fields have extraneous children which we want to ignore.
    if (parent->isPasswordField() || parent->isTextControl())
        return IgnoreObject;

    // Include all tables, even layout tables. The AT can decide what to do with each.
    if (role == CellRole || role == TableRole)
        return IncludeObject;

    // The object containing the text should implement AtkText itself.
    if (role == StaticTextRole)
        return IgnoreObject;

    // Include all list items, regardless they have or not inline children
    if (role == ListItemRole)
        return IncludeObject;

    // Bullets/numbers for list items shouldn't be exposed as AtkObjects.
    if (role == ListMarkerRole)
        return IgnoreObject;

    // Never expose an unknown object, since AT's won't know what to
    // do with them. This is what is done on the Mac as well.
    if (role == UnknownRole)
        return IgnoreObject;

    // Given a paragraph or div containing a non-nested anonymous block, WebCore
    // ignores the paragraph or div and includes the block. We want the opposite:
    // ATs are expecting accessible objects associated with textual elements. They
    // usually have no need for the anonymous block. And when the wrong objects
    // get included or ignored, needed accessibility signals do not get emitted.
    if (role == ParagraphRole || role == DivRole) {
        // Don't call textUnderElement() here, because it's slow and it can
        // crash when called while we're in the middle of a subtree being deleted.
        if (!renderer()->firstChild())
            return DefaultBehavior;

        if (!parent->renderer() || parent->renderer()->isAnonymousBlock())
            return DefaultBehavior;

        for (RenderObject* r = renderer()->firstChild(); r; r = r->nextSibling()) {
            if (r->isAnonymousBlock())
                return IncludeObject;
        }
    }

    // Block spans result in objects of ATK_ROLE_PANEL which are almost always unwanted.
    // However, if we ignore block spans whose parent is the body, the child controls
    // will become immediate children of the ATK_ROLE_DOCUMENT_FRAME and any text will
    // become text within the document frame itself. This ultimately may be what we want
    // and would largely be consistent with what we see from Gecko. However, ignoring
    // spans whose parent is the body changes the current behavior we see from WebCore.
    // Until we have sufficient time to properly analyze these cases, we will defer to
    // WebCore. We only check that the parent is not aria because we do not expect
    // anonymous blocks which are aria-related to themselves have an aria role, nor
    // have we encountered instances where the parent of an anonymous block also lacked
    // an aria role but the grandparent had one.
    if (renderer() && renderer()->isAnonymousBlock() && !parent->renderer()->isBody()
        && parent->ariaRoleAttribute() == UnknownRole)
        return IgnoreObject;

    return DefaultBehavior;
}

AccessibilityObjectWrapper* AccessibilityObject::wrapper() const
{
    return m_wrapper;
}

void AccessibilityObject::setWrapper(AccessibilityObjectWrapper* wrapper)
{
    if (wrapper == m_wrapper)
        return;

    if (m_wrapper)
        g_object_unref(m_wrapper);

    m_wrapper = wrapper;

    if (m_wrapper)
        g_object_ref(m_wrapper);
}

bool AccessibilityObject::allowsTextRanges() const
{
    // Check type for the AccessibilityObject.
    if (isTextControl() || isWebArea() || isGroup() || isLink() || isHeading() || isListItem() || isTableCell())
        return true;

    // Check roles as the last fallback mechanism.
    AccessibilityRole role = roleValue();
    return role == ParagraphRole || role == LabelRole || role == DivRole || role == FormRole;
}

unsigned AccessibilityObject::getLengthForTextRange() const
{
    unsigned textLength = text().length();

    if (textLength)
        return textLength;

    // Gtk ATs need this for all text objects; not just text controls.
    Node* node = this->node();
    RenderObject* renderer = node ? node->renderer() : 0;
    if (renderer && renderer->isText()) {
        RenderText* renderText = toRenderText(renderer);
        textLength = renderText ? renderText->textLength() : 0;
    }

    // Get the text length from the elements under the
    // accessibility object if the value is still zero.
    if (!textLength && allowsTextRanges())
        textLength = textUnderElement(TextUnderElementModeIncludeAllChildren).length();

    return textLength;
}

} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
