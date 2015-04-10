/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
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
#include "WebKitAccessibleInterfaceText.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"
#include "Document.h"
#include "Font.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "InlineTextBox.h"
#include "NotImplemented.h"
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderText.h"
#include "TextEncoding.h"
#include "TextIterator.h"
#include "VisibleUnits.h"
#include "WebKitAccessibleUtil.h"
#include "WebKitAccessibleWrapperAtk.h"
#include "htmlediting.h"
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#if PLATFORM(GTK)
#include <libgail-util/gail-util.h>
#include <pango/pango.h>
#endif

using namespace WebCore;

static AccessibilityObject* core(AtkText* text)
{
    if (!WEBKIT_IS_ACCESSIBLE(text))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(text));
}

static gchar* textForRenderer(RenderObject* renderer)
{
    GString* resultText = g_string_new(0);

    if (!renderer)
        return g_string_free(resultText, FALSE);

    // For RenderBlocks, piece together the text from the RenderText objects they contain.
    for (RenderObject* object = renderer->firstChild(); object; object = object->nextSibling()) {
        if (object->isBR()) {
            g_string_append(resultText, "\n");
            continue;
        }

        RenderText* renderText;
        if (object->isText())
            renderText = toRenderText(object);
        else {
            // List item's markers will be treated in an special way
            // later on this function, so ignore them here.
            if (object->isReplaced() && !object->isListMarker())
                g_string_append_unichar(resultText, objectReplacementCharacter);

            // We need to check children, if any, to consider when
            // current object is not a text object but some of its
            // children are, in order not to miss those portions of
            // text by not properly handling those situations
            if (object->firstChild()) {
                GOwnPtr<char> objectText(textForRenderer(object));
                g_string_append(resultText, objectText.get());
            }
            continue;
        }

        InlineTextBox* box = renderText ? renderText->firstTextBox() : 0;
        while (box) {
            // WebCore introduces line breaks in the text that do not reflect
            // the layout you see on the screen, replace them with spaces.
            String text = String(renderText->characters(), renderText->textLength()).replace("\n", " ");
            g_string_append(resultText, text.substring(box->start(), box->end() - box->start() + 1).utf8().data());

            // Newline chars in the source result in separate text boxes, so check
            // before adding a newline in the layout. See bug 25415 comment #78.
            // If the next sibling is a BR, we'll add the newline when we examine that child.
            if (!box->nextOnLineExists() && !(object->nextSibling() && object->nextSibling()->isBR())) {
                // If there was a '\n' in the last position of the
                // current text box, it would have been converted to a
                // space in String::replace(), so remove it first.
                if (renderText->characters()[box->end()] == '\n')
                    g_string_erase(resultText, resultText->len - 1, -1);

                g_string_append(resultText, "\n");
            }
            box = box->nextTextBox();
        }
    }

    // Insert the text of the marker for list item in the right place, if present
    if (renderer->isListItem()) {
        String markerText = toRenderListItem(renderer)->markerTextWithSuffix();
        if (renderer->style()->direction() == LTR)
            g_string_prepend(resultText, markerText.utf8().data());
        else
            g_string_append(resultText, markerText.utf8().data());
    }

    return g_string_free(resultText, FALSE);
}

static gchar* textForObject(AccessibilityObject* coreObject)
{
    GString* str = g_string_new(0);

    // For text controls, we can get the text line by line.
    if (coreObject->isTextControl()) {
        unsigned textLength = coreObject->textLength();
        int lineNumber = 0;
        PlainTextRange range = coreObject->doAXRangeForLine(lineNumber);
        while (range.length) {
            // When a line of text wraps in a text area, the final space is removed.
            if (range.start + range.length < textLength)
                range.length -= 1;
            String lineText = coreObject->doAXStringForRange(range);
            g_string_append(str, lineText.utf8().data());
            g_string_append(str, "\n");
            range = coreObject->doAXRangeForLine(++lineNumber);
        }
    } else if (coreObject->isAccessibilityRenderObject()) {
        GOwnPtr<gchar> rendererText(textForRenderer(coreObject->renderer()));
        g_string_append(str, rendererText.get());
    }

    return g_string_free(str, FALSE);
}

static gchar* webkitAccessibleTextGetText(AtkText*, gint startOffset, gint endOffset);

#if PLATFORM(GTK)
static GailTextUtil* getGailTextUtilForAtk(AtkText* textObject)
{
    GailTextUtil* gailTextUtil = gail_text_util_new();
    GOwnPtr<char> text(webkitAccessibleTextGetText(textObject, 0, -1));
    gail_text_util_text_setup(gailTextUtil, text.get());
    return gailTextUtil;
}

static PangoLayout* getPangoLayoutForAtk(AtkText* textObject)
{
    AccessibilityObject* coreObject = core(textObject);

    Document* document = coreObject->document();
    if (!document)
        return 0;

    HostWindow* hostWindow = document->view()->hostWindow();
    if (!hostWindow)
        return 0;
    PlatformPageClient webView = hostWindow->platformPageClient();
    if (!webView)
        return 0;

    // Create a string with the layout as it appears on the screen
    GOwnPtr<char> objectText(textForObject(coreObject));
    PangoLayout* layout = gtk_widget_create_pango_layout(static_cast<GtkWidget*>(webView), objectText.get());
    return layout;
}
#endif

static int baselinePositionForRenderObject(RenderObject* renderObject)
{
    // FIXME: This implementation of baselinePosition originates from RenderObject.cpp and was
    // removed in r70072. The implementation looks incorrect though, because this is not the
    // baseline of the underlying RenderObject, but of the AccessibilityRenderObject.
    const FontMetrics& fontMetrics = renderObject->firstLineStyle()->fontMetrics();
    return fontMetrics.ascent() + (renderObject->firstLineStyle()->computedLineHeight() - fontMetrics.height()) / 2;
}

static AtkAttributeSet* getAttributeSetForAccessibilityObject(const AccessibilityObject* object)
{
    if (!object->isAccessibilityRenderObject())
        return 0;

    RenderObject* renderer = object->renderer();
    RenderStyle* style = renderer->style();

    AtkAttributeSet* result = 0;
    GOwnPtr<gchar> buffer(g_strdup_printf("%i", style->fontSize()));
    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_SIZE), buffer.get());

    Color bgColor = style->visitedDependentColor(CSSPropertyBackgroundColor);
    if (bgColor.isValid()) {
        buffer.set(g_strdup_printf("%i,%i,%i", bgColor.red(), bgColor.green(), bgColor.blue()));
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_BG_COLOR), buffer.get());
    }

    Color fgColor = style->visitedDependentColor(CSSPropertyColor);
    if (fgColor.isValid()) {
        buffer.set(g_strdup_printf("%i,%i,%i", fgColor.red(), fgColor.green(), fgColor.blue()));
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_FG_COLOR), buffer.get());
    }

    int baselinePosition;
    bool includeRise = true;
    switch (style->verticalAlign()) {
    case SUB:
        baselinePosition = -1 * baselinePositionForRenderObject(renderer);
        break;
    case SUPER:
        baselinePosition = baselinePositionForRenderObject(renderer);
        break;
    case BASELINE:
        baselinePosition = 0;
        break;
    default:
        includeRise = false;
        break;
    }

    if (includeRise) {
        buffer.set(g_strdup_printf("%i", baselinePosition));
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_RISE), buffer.get());
    }

    if (!style->textIndent().isUndefined()) {
        int indentation = valueForLength(style->textIndent(), object->size().width(), renderer->view());
        buffer.set(g_strdup_printf("%i", indentation));
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_INDENT), buffer.get());
    }

    String fontFamilyName = style->font().firstFamily();
    if (fontFamilyName.left(8) == "-webkit-")
        fontFamilyName = fontFamilyName.substring(8);

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_FAMILY_NAME), fontFamilyName.utf8().data());

    int fontWeight = -1;
    switch (style->font().weight()) {
    case FontWeight100:
        fontWeight = 100;
        break;
    case FontWeight200:
        fontWeight = 200;
        break;
    case FontWeight300:
        fontWeight = 300;
        break;
    case FontWeight400:
        fontWeight = 400;
        break;
    case FontWeight500:
        fontWeight = 500;
        break;
    case FontWeight600:
        fontWeight = 600;
        break;
    case FontWeight700:
        fontWeight = 700;
        break;
    case FontWeight800:
        fontWeight = 800;
        break;
    case FontWeight900:
        fontWeight = 900;
    }
    if (fontWeight > 0) {
        buffer.set(g_strdup_printf("%i", fontWeight));
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_WEIGHT), buffer.get());
    }

    switch (style->textAlign()) {
    case TASTART:
    case TAEND:
        break;
    case LEFT:
    case WEBKIT_LEFT:
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_JUSTIFICATION), "left");
        break;
    case RIGHT:
    case WEBKIT_RIGHT:
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_JUSTIFICATION), "right");
        break;
    case CENTER:
    case WEBKIT_CENTER:
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_JUSTIFICATION), "center");
        break;
    case JUSTIFY:
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_JUSTIFICATION), "fill");
    }

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_UNDERLINE), (style->textDecoration() & TextDecorationUnderline) ? "single" : "none");

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_STYLE), style->font().italic() ? "italic" : "normal");

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_STRIKETHROUGH), (style->textDecoration() & TextDecorationLineThrough) ? "true" : "false");

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_INVISIBLE), (style->visibility() == HIDDEN) ? "true" : "false");

    result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_EDITABLE), object->isReadOnly() ? "false" : "true");

    String language = object->language();
    if (!language.isEmpty())
        result = addToAtkAttributeSet(result, atk_text_attribute_get_name(ATK_TEXT_ATTR_LANGUAGE), language.utf8().data());

    return result;
}

static gint compareAttribute(const AtkAttribute* a, const AtkAttribute* b)
{
    return g_strcmp0(a->name, b->name) || g_strcmp0(a->value, b->value);
}

// Returns an AtkAttributeSet with the elements of attributeSet1 which
// are either not present or different in attributeSet2. Neither
// attributeSet1 nor attributeSet2 should be used after calling this.
static AtkAttributeSet* attributeSetDifference(AtkAttributeSet* attributeSet1, AtkAttributeSet* attributeSet2)
{
    if (!attributeSet2)
        return attributeSet1;

    AtkAttributeSet* currentSet = attributeSet1;
    AtkAttributeSet* found;
    AtkAttributeSet* toDelete = 0;

    while (currentSet) {
        found = g_slist_find_custom(attributeSet2, currentSet->data, (GCompareFunc)compareAttribute);
        if (found) {
            AtkAttributeSet* nextSet = currentSet->next;
            toDelete = g_slist_prepend(toDelete, currentSet->data);
            attributeSet1 = g_slist_delete_link(attributeSet1, currentSet);
            currentSet = nextSet;
        } else
            currentSet = currentSet->next;
    }

    atk_attribute_set_free(attributeSet2);
    atk_attribute_set_free(toDelete);
    return attributeSet1;
}

static guint accessibilityObjectLength(const AccessibilityObject* object)
{
    // Non render objects are not taken into account
    if (!object->isAccessibilityRenderObject())
        return 0;

    // For those objects implementing the AtkText interface we use the
    // well known API to always get the text in a consistent way
    AtkObject* atkObj = ATK_OBJECT(object->wrapper());
    if (ATK_IS_TEXT(atkObj)) {
        GOwnPtr<gchar> text(webkitAccessibleTextGetText(ATK_TEXT(atkObj), 0, -1));
        return g_utf8_strlen(text.get(), -1);
    }

    // Even if we don't expose list markers to Assistive
    // Technologies, we need to have a way to measure their length
    // for those cases when it's needed to take it into account
    // separately (as in getAccessibilityObjectForOffset)
    RenderObject* renderer = object->renderer();
    if (renderer && renderer->isListMarker()) {
        RenderListMarker* marker = toRenderListMarker(renderer);
        return marker->text().length() + marker->suffix().length();
    }

    return 0;
}

static const AccessibilityObject* getAccessibilityObjectForOffset(const AccessibilityObject* object, guint offset, gint* startOffset, gint* endOffset)
{
    const AccessibilityObject* result;
    guint length = accessibilityObjectLength(object);
    if (length > offset) {
        *startOffset = 0;
        *endOffset = length;
        result = object;
    } else {
        *startOffset = -1;
        *endOffset = -1;
        result = 0;
    }

    if (!object->firstChild())
        return result;

    AccessibilityObject* child = object->firstChild();
    guint currentOffset = 0;
    guint childPosition = 0;
    while (child && currentOffset <= offset) {
        guint childLength = accessibilityObjectLength(child);
        currentOffset = childLength + childPosition;
        if (currentOffset > offset) {
            gint childStartOffset;
            gint childEndOffset;
            const AccessibilityObject* grandChild = getAccessibilityObjectForOffset(child, offset-childPosition,  &childStartOffset, &childEndOffset);
            if (childStartOffset >= 0) {
                *startOffset = childStartOffset + childPosition;
                *endOffset = childEndOffset + childPosition;
                result = grandChild;
            }
        } else {
            childPosition += childLength;
            child = child->nextSibling();
        }
    }
    return result;
}

static AtkAttributeSet* getRunAttributesFromAccesibilityObject(const AccessibilityObject* element, gint offset, gint* startOffset, gint* endOffset)
{
    const AccessibilityObject* child = getAccessibilityObjectForOffset(element, offset, startOffset, endOffset);
    if (!child) {
        *startOffset = -1;
        *endOffset = -1;
        return 0;
    }

    AtkAttributeSet* defaultAttributes = getAttributeSetForAccessibilityObject(element);
    AtkAttributeSet* childAttributes = getAttributeSetForAccessibilityObject(child);

    return attributeSetDifference(childAttributes, defaultAttributes);
}

static IntRect textExtents(AtkText* text, gint startOffset, gint length, AtkCoordType coords)
{
    GOwnPtr<char> textContent(webkitAccessibleTextGetText(text, startOffset, -1));
    gint textLength = g_utf8_strlen(textContent.get(), -1);

    // The first case (endOffset of -1) should work, but seems broken for all Gtk+ apps.
    gint rangeLength = length;
    if (rangeLength < 0 || rangeLength > textLength)
        rangeLength = textLength;
    AccessibilityObject* coreObject = core(text);

    IntRect extents = coreObject->doAXBoundsForRange(PlainTextRange(startOffset, rangeLength));
    switch (coords) {
    case ATK_XY_SCREEN:
        if (Document* document = coreObject->document())
            extents = document->view()->contentsToScreen(extents);
        break;
    case ATK_XY_WINDOW:
        // No-op
        break;
    }

    return extents;
}

static int offsetAdjustmentForListItem(const AccessibilityObject* object)
{
    // We need to adjust the offsets for the list item marker in
    // Left-To-Right text, since we expose it together with the text.
    RenderObject* renderer = object->renderer();
    if (renderer && renderer->isListItem() && renderer->style()->direction() == LTR)
        return toRenderListItem(renderer)->markerTextWithSuffix().length();

    return 0;
}

static int webCoreOffsetToAtkOffset(const AccessibilityObject* object, int offset)
{
    if (!object->isListItem())
        return offset;

    return offset + offsetAdjustmentForListItem(object);
}

static int atkOffsetToWebCoreOffset(AtkText* text, int offset)
{
    AccessibilityObject* coreObject = core(text);
    if (!coreObject || !coreObject->isListItem())
        return offset;

    return offset - offsetAdjustmentForListItem(coreObject);
}

static void getSelectionOffsetsForObject(AccessibilityObject* coreObject, VisibleSelection& selection, gint& startOffset, gint& endOffset)
{
    if (!coreObject->isAccessibilityRenderObject())
        return;

    // Early return if the selection doesn't affect the selected node.
    if (!selectionBelongsToObject(coreObject, selection))
        return;

    // We need to find the exact start and end positions in the
    // selected node that intersects the selection, to later on get
    // the right values for the effective start and end offsets.
    Position nodeRangeStart;
    Position nodeRangeEnd;
    Node* node = coreObject->node();
    RefPtr<Range> selRange = selection.toNormalizedRange();

    // If the selection affects the selected node and its first
    // possible position is also in the selection, we must set
    // nodeRangeStart to that position, otherwise to the selection's
    // start position (it would belong to the node anyway).
    Node* firstLeafNode = node->firstDescendant();
    if (selRange->isPointInRange(firstLeafNode, 0, IGNORE_EXCEPTION))
        nodeRangeStart = firstPositionInOrBeforeNode(firstLeafNode);
    else
        nodeRangeStart = selRange->startPosition();

    // If the selection affects the selected node and its last
    // possible position is also in the selection, we must set
    // nodeRangeEnd to that position, otherwise to the selection's
    // end position (it would belong to the node anyway).
    Node* lastLeafNode = node->lastDescendant();
    if (selRange->isPointInRange(lastLeafNode, lastOffsetInNode(lastLeafNode), IGNORE_EXCEPTION))
        nodeRangeEnd = lastPositionInOrAfterNode(lastLeafNode);
    else
        nodeRangeEnd = selRange->endPosition();

    // Calculate position of the selected range inside the object.
    Position parentFirstPosition = firstPositionInOrBeforeNode(node);
    RefPtr<Range> rangeInParent = Range::create(node->document(), parentFirstPosition, nodeRangeStart);


    // Set values for start and end offsets.
    startOffset = webCoreOffsetToAtkOffset(coreObject, TextIterator::rangeLength(rangeInParent.get(), true));

    RefPtr<Range> nodeRange = Range::create(node->document(), nodeRangeStart, nodeRangeEnd);
    endOffset = startOffset + TextIterator::rangeLength(nodeRange.get(), true);
}

static gchar* webkitAccessibleTextGetText(AtkText* text, gint startOffset, gint endOffset)
{
    AccessibilityObject* coreObject = core(text);

    int end = endOffset;
    if (endOffset == -1) {
        end = coreObject->stringValue().length();
        if (!end)
            end = coreObject->textUnderElement(TextUnderElementModeIncludeAllChildren).length();
    }

    String ret;
    if (coreObject->isTextControl())
        ret = coreObject->doAXStringForRange(PlainTextRange(0, endOffset));
    else {
        ret = coreObject->stringValue();
        if (!ret)
            ret = coreObject->textUnderElement(TextUnderElementModeIncludeAllChildren);
    }

    if (!ret.length()) {
        // This can happen at least with anonymous RenderBlocks (e.g. body text amongst paragraphs)
        // In such instances, there may also be embedded objects. The object replacement character
        // is something ATs want included and we have to account for the fact that it is multibyte.
        GOwnPtr<char> objectText(textForObject(coreObject));
        ret = String::fromUTF8(objectText.get());
        if (!end)
            end = ret.length();
    }

    // Prefix a item number/bullet if needed
    if (coreObject->roleValue() == ListItemRole) {
        RenderObject* objRenderer = coreObject->renderer();
        if (objRenderer && objRenderer->isListItem()) {
            String markerText = toRenderListItem(objRenderer)->markerTextWithSuffix();
            ret = objRenderer->style()->direction() == LTR ? markerText + ret : ret + markerText;
            if (endOffset == -1)
                end += markerText.length();
        }
    }

    ret = ret.substring(startOffset, end - startOffset);
    return g_strdup(ret.utf8().data());
}

enum GetTextRelativePosition {
    GetTextPositionAt,
    GetTextPositionBefore,
    GetTextPositionAfter
};

static gchar* webkitAccessibleTextGetChar(AtkText* text, gint offset, GetTextRelativePosition textPosition, gint* startOffset, gint* endOffset)
{
    AccessibilityObject* coreObject = core(text);
    if (!coreObject || !coreObject->isAccessibilityRenderObject())
        return g_strdup("");

    int actualOffset = offset;
    if (textPosition == GetTextPositionBefore)
        actualOffset--;
    else if (textPosition == GetTextPositionAfter)
        actualOffset++;

    GOwnPtr<char> textData(webkitAccessibleTextGetText(text, 0, -1));
    int textLength = g_utf8_strlen(textData.get(), -1);

    *startOffset = std::max(0, actualOffset);
    *startOffset = std::min(*startOffset, textLength);

    *endOffset = std::max(0, actualOffset + 1);
    *endOffset = std::min(*endOffset, textLength);

    if (*startOffset == *endOffset)
        return g_strdup("");

    return g_utf8_substring(textData.get(), *startOffset, *endOffset);
}

static gchar* webkitAccessibleTextGetTextForOffset(AtkText* text, gint offset, AtkTextBoundary boundaryType, GetTextRelativePosition textPosition, gint* startOffset, gint* endOffset)
{
    // Make sure we always return valid valid values for offsets.
    *startOffset = 0;
    *endOffset = 0;

    if (boundaryType == ATK_TEXT_BOUNDARY_CHAR)
        return webkitAccessibleTextGetChar(text, offset, textPosition, startOffset, endOffset);

#if PLATFORM(GTK)
    // FIXME: Get rid of the code below once every single get_text_*_offset
    // function has been properly implemented without using Pango/Cairo.
    GailOffsetType offsetType = GAIL_AT_OFFSET;
    switch (textPosition) {
    case GetTextPositionBefore:
        offsetType = GAIL_BEFORE_OFFSET;
        break;

    case GetTextPositionAt:
        break;

    case GetTextPositionAfter:
        offsetType = GAIL_AFTER_OFFSET;
        break;

    default:
        ASSERT_NOT_REACHED();
    }

    return gail_text_util_get_text(getGailTextUtilForAtk(text), getPangoLayoutForAtk(text), offsetType, boundaryType, offset, startOffset, endOffset);
#endif

    notImplemented();
    return 0;
}

static gchar* webkitAccessibleTextGetTextAfterOffset(AtkText* text, gint offset, AtkTextBoundary boundaryType, gint* startOffset, gint* endOffset)
{
    return webkitAccessibleTextGetTextForOffset(text, offset, boundaryType, GetTextPositionAfter, startOffset, endOffset);
}

static gchar* webkitAccessibleTextGetTextAtOffset(AtkText* text, gint offset, AtkTextBoundary boundaryType, gint* startOffset, gint* endOffset)
{
    return webkitAccessibleTextGetTextForOffset(text, offset, boundaryType, GetTextPositionAt, startOffset, endOffset);
}

static gchar* webkitAccessibleTextGetTextBeforeOffset(AtkText* text, gint offset, AtkTextBoundary boundaryType, gint* startOffset, gint* endOffset)
{
    return webkitAccessibleTextGetTextForOffset(text, offset, boundaryType, GetTextPositionBefore, startOffset, endOffset);
}

static gunichar webkitAccessibleTextGetCharacterAtOffset(AtkText*, gint)
{
    notImplemented();
    return 0;
}

static gint webkitAccessibleTextGetCaretOffset(AtkText* text)
{
    // coreObject is the unignored object whose offset the caller is requesting.
    // focusedObject is the object with the caret. It is likely ignored -- unless it's a link.
    AccessibilityObject* coreObject = core(text);
    if (!coreObject->isAccessibilityRenderObject())
        return 0;

    // We need to make sure we pass a valid object as reference.
    if (coreObject->accessibilityIsIgnored())
        coreObject = coreObject->parentObjectUnignored();
    if (!coreObject)
        return 0;

    int offset;
    if (!objectFocusedAndCaretOffsetUnignored(coreObject, offset))
        return 0;

    return webCoreOffsetToAtkOffset(coreObject, offset);
}

static AtkAttributeSet* webkitAccessibleTextGetRunAttributes(AtkText* text, gint offset, gint* startOffset, gint* endOffset)
{
    AccessibilityObject* coreObject = core(text);
    AtkAttributeSet* result;

    if (!coreObject) {
        *startOffset = 0;
        *endOffset = atk_text_get_character_count(text);
        return 0;
    }

    if (offset == -1)
        offset = atk_text_get_caret_offset(text);

    result = getRunAttributesFromAccesibilityObject(coreObject, offset, startOffset, endOffset);

    if (*startOffset < 0) {
        *startOffset = offset;
        *endOffset = offset;
    }

    return result;
}

static AtkAttributeSet* webkitAccessibleTextGetDefaultAttributes(AtkText* text)
{
    AccessibilityObject* coreObject = core(text);
    if (!coreObject || !coreObject->isAccessibilityRenderObject())
        return 0;

    return getAttributeSetForAccessibilityObject(coreObject);
}

static void webkitAccessibleTextGetCharacterExtents(AtkText* text, gint offset, gint* x, gint* y, gint* width, gint* height, AtkCoordType coords)
{
    IntRect extents = textExtents(text, offset, 1, coords);
    *x = extents.x();
    *y = extents.y();
    *width = extents.width();
    *height = extents.height();
}

static void webkitAccessibleTextGetRangeExtents(AtkText* text, gint startOffset, gint endOffset, AtkCoordType coords, AtkTextRectangle* rect)
{
    IntRect extents = textExtents(text, startOffset, endOffset - startOffset, coords);
    rect->x = extents.x();
    rect->y = extents.y();
    rect->width = extents.width();
    rect->height = extents.height();
}

static gint webkitAccessibleTextGetCharacterCount(AtkText* text)
{
    return accessibilityObjectLength(core(text));
}

static gint webkitAccessibleTextGetOffsetAtPoint(AtkText* text, gint x, gint y, AtkCoordType)
{
    // FIXME: Use the AtkCoordType
    // TODO: Is it correct to ignore range.length?
    IntPoint pos(x, y);
    PlainTextRange range = core(text)->doAXRangeForPosition(pos);
    return range.start;
}

static gint webkitAccessibleTextGetNSelections(AtkText* text)
{
    AccessibilityObject* coreObject = core(text);
    VisibleSelection selection = coreObject->selection();

    // Only range selections are needed for the purpose of this method
    if (!selection.isRange())
        return 0;

    // We don't support multiple selections for now, so there's only
    // two possibilities
    // Also, we don't want to do anything if the selection does not
    // belong to the currently selected object. We have to check since
    // there's no way to get the selection for a given object, only
    // the global one (the API is a bit confusing)
    return selectionBelongsToObject(coreObject, selection) ? 1 : 0;
}

static gchar* webkitAccessibleTextGetSelection(AtkText* text, gint selectionNum, gint* startOffset, gint* endOffset)
{
    // Default values, unless the contrary is proved
    *startOffset = *endOffset = 0;

    // WebCore does not support multiple selection, so anything but 0 does not make sense for now.
    if (selectionNum)
        return 0;

    // Get the offsets of the selection for the selected object
    AccessibilityObject* coreObject = core(text);
    VisibleSelection selection = coreObject->selection();
    getSelectionOffsetsForObject(coreObject, selection, *startOffset, *endOffset);

    // Return 0 instead of "", as that's the expected result for
    // this AtkText method when there's no selection
    if (*startOffset == *endOffset)
        return 0;

    return webkitAccessibleTextGetText(text, *startOffset, *endOffset);
}

static gboolean webkitAccessibleTextAddSelection(AtkText*, gint, gint)
{
    notImplemented();
    return FALSE;
}

static gboolean webkitAccessibleTextSetSelection(AtkText* text, gint selectionNum, gint startOffset, gint endOffset)
{
    // WebCore does not support multiple selection, so anything but 0 does not make sense for now.
    if (selectionNum)
        return FALSE;

    AccessibilityObject* coreObject = core(text);
    if (!coreObject->isAccessibilityRenderObject())
        return FALSE;

    // Consider -1 and out-of-bound values and correct them to length
    gint textCount = webkitAccessibleTextGetCharacterCount(text);
    if (startOffset < 0 || startOffset > textCount)
        startOffset = textCount;
    if (endOffset < 0 || endOffset > textCount)
        endOffset = textCount;

    // We need to adjust the offsets for the list item marker.
    int offsetAdjustment = offsetAdjustmentForListItem(coreObject);
    if (offsetAdjustment) {
        if (startOffset < offsetAdjustment || endOffset < offsetAdjustment)
            return FALSE;

        startOffset = atkOffsetToWebCoreOffset(text, startOffset);
        endOffset = atkOffsetToWebCoreOffset(text, endOffset);
    }

    PlainTextRange textRange(startOffset, endOffset - startOffset);
    VisiblePositionRange range = coreObject->visiblePositionRangeForRange(textRange);
    if (range.isNull())
        return FALSE;

    coreObject->setSelectedVisiblePositionRange(range);
    return TRUE;
}

static gboolean webkitAccessibleTextRemoveSelection(AtkText* text, gint selectionNum)
{
    // WebCore does not support multiple selection, so anything but 0 does not make sense for now.
    if (selectionNum)
        return FALSE;

    // Do nothing if current selection doesn't belong to the object
    if (!webkitAccessibleTextGetNSelections(text))
        return FALSE;

    // Set a new 0-sized selection to the caret position, in order
    // to simulate selection removal (GAIL style)
    gint caretOffset = webkitAccessibleTextGetCaretOffset(text);
    return webkitAccessibleTextSetSelection(text, selectionNum, caretOffset, caretOffset);
}

static gboolean webkitAccessibleTextSetCaretOffset(AtkText* text, gint offset)
{
    AccessibilityObject* coreObject = core(text);

    if (!coreObject->isAccessibilityRenderObject())
        return FALSE;

    // We need to adjust the offsets for the list item marker.
    int offsetAdjustment = offsetAdjustmentForListItem(coreObject);
    if (offsetAdjustment) {
        if (offset < offsetAdjustment)
            return FALSE;

        offset = atkOffsetToWebCoreOffset(text, offset);
    }

    PlainTextRange textRange(offset, 0);
    VisiblePositionRange range = coreObject->visiblePositionRangeForRange(textRange);
    if (range.isNull())
        return FALSE;

    coreObject->setSelectedVisiblePositionRange(range);
    return TRUE;
}

void webkitAccessibleTextInterfaceInit(AtkTextIface* iface)
{
    iface->get_text = webkitAccessibleTextGetText;
    iface->get_text_after_offset = webkitAccessibleTextGetTextAfterOffset;
    iface->get_text_at_offset = webkitAccessibleTextGetTextAtOffset;
    iface->get_text_before_offset = webkitAccessibleTextGetTextBeforeOffset;
    iface->get_character_at_offset = webkitAccessibleTextGetCharacterAtOffset;
    iface->get_caret_offset = webkitAccessibleTextGetCaretOffset;
    iface->get_run_attributes = webkitAccessibleTextGetRunAttributes;
    iface->get_default_attributes = webkitAccessibleTextGetDefaultAttributes;
    iface->get_character_extents = webkitAccessibleTextGetCharacterExtents;
    iface->get_range_extents = webkitAccessibleTextGetRangeExtents;
    iface->get_character_count = webkitAccessibleTextGetCharacterCount;
    iface->get_offset_at_point = webkitAccessibleTextGetOffsetAtPoint;
    iface->get_n_selections = webkitAccessibleTextGetNSelections;
    iface->get_selection = webkitAccessibleTextGetSelection;
    iface->add_selection = webkitAccessibleTextAddSelection;
    iface->remove_selection = webkitAccessibleTextRemoveSelection;
    iface->set_selection = webkitAccessibleTextSetSelection;
    iface->set_caret_offset = webkitAccessibleTextSetCaretOffset;
}

#endif
