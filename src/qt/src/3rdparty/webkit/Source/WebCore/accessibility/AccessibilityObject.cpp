/*
 * Copyright (C) 2008, 2009, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityObject.h"

#include "AXObjectCache.h"
#include "AccessibilityRenderObject.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "LocalizedStrings.h"
#include "NodeList.h"
#include "NotImplemented.h"
#include "Page.h"
#include "RenderImage.h"
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderMenuList.h"
#include "RenderTextControl.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "SelectionController.h"
#include "TextIterator.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

AccessibilityObject::AccessibilityObject()
    : m_id(0)
    , m_haveChildren(false)
    , m_role(UnknownRole)
#if PLATFORM(GTK)
    , m_wrapper(0)
#endif
{
}

AccessibilityObject::~AccessibilityObject()
{
    ASSERT(isDetached());
}

void AccessibilityObject::detach()
{
#if HAVE(ACCESSIBILITY)
    setWrapper(0);
#endif    
}

AccessibilityObject* AccessibilityObject::parentObjectUnignored() const
{
    AccessibilityObject* parent;
    for (parent = parentObject(); parent && parent->accessibilityIsIgnored(); parent = parent->parentObject()) {
    }
    
    return parent;
}

AccessibilityObject* AccessibilityObject::firstAccessibleObjectFromNode(const Node* node)
{
    ASSERT(AXObjectCache::accessibilityEnabled());

    if (!node)
        return 0;

    Document* document = node->document();
    if (!document)
        return 0;

    AXObjectCache* cache = document->axObjectCache();

    AccessibilityObject* accessibleObject = cache->getOrCreate(node->renderer());
    while (accessibleObject && accessibleObject->accessibilityIsIgnored()) {
        node = node->traverseNextNode();

        while (node && !node->renderer())
            node = node->traverseNextSibling();

        if (!node)
            return 0;

        accessibleObject = cache->getOrCreate(node->renderer());
    }

    return accessibleObject;
}

bool AccessibilityObject::isARIAInput(AccessibilityRole ariaRole)
{
    return ariaRole == RadioButtonRole || ariaRole == CheckBoxRole || ariaRole == TextFieldRole;
}    
    
bool AccessibilityObject::isARIAControl(AccessibilityRole ariaRole)
{
    return isARIAInput(ariaRole) || ariaRole == TextAreaRole || ariaRole == ButtonRole 
    || ariaRole == ComboBoxRole || ariaRole == SliderRole; 
}

IntPoint AccessibilityObject::clickPoint() const
{
    IntRect rect = elementRect();
    return IntPoint(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
}

bool AccessibilityObject::press() const
{
    Element* actionElem = actionElement();
    if (!actionElem)
        return false;
    if (Frame* f = actionElem->document()->frame())
        f->loader()->resetMultipleFormSubmissionProtection();
    actionElem->accessKeyAction(true);
    return true;
}
    
String AccessibilityObject::language() const
{
    const AtomicString& lang = getAttribute(langAttr);
    if (!lang.isEmpty())
        return lang;

    AccessibilityObject* parent = parentObject();
    
    // as a last resort, fall back to the content language specified in the meta tag
    if (!parent) {
        Document* doc = document();
        if (doc)
            return doc->contentLanguage();
        return nullAtom;
    }
    
    return parent->language();
}
    
VisiblePositionRange AccessibilityObject::visiblePositionRangeForUnorderedPositions(const VisiblePosition& visiblePos1, const VisiblePosition& visiblePos2) const
{
    if (visiblePos1.isNull() || visiblePos2.isNull())
        return VisiblePositionRange();

    VisiblePosition startPos;
    VisiblePosition endPos;
    bool alreadyInOrder;

    // upstream is ordered before downstream for the same position
    if (visiblePos1 == visiblePos2 && visiblePos2.affinity() == UPSTREAM)
        alreadyInOrder = false;

    // use selection order to see if the positions are in order
    else
        alreadyInOrder = VisibleSelection(visiblePos1, visiblePos2).isBaseFirst();

    if (alreadyInOrder) {
        startPos = visiblePos1;
        endPos = visiblePos2;
    } else {
        startPos = visiblePos2;
        endPos = visiblePos1;
    }

    return VisiblePositionRange(startPos, endPos);
}

VisiblePositionRange AccessibilityObject::positionOfLeftWord(const VisiblePosition& visiblePos) const
{
    VisiblePosition startPosition = startOfWord(visiblePos, LeftWordIfOnBoundary);
    VisiblePosition endPosition = endOfWord(startPosition);
    return VisiblePositionRange(startPosition, endPosition);
}

VisiblePositionRange AccessibilityObject::positionOfRightWord(const VisiblePosition& visiblePos) const
{
    VisiblePosition startPosition = startOfWord(visiblePos, RightWordIfOnBoundary);
    VisiblePosition endPosition = endOfWord(startPosition);
    return VisiblePositionRange(startPosition, endPosition);
}

static VisiblePosition updateAXLineStartForVisiblePosition(const VisiblePosition& visiblePosition)
{
    // A line in the accessibility sense should include floating objects, such as aligned image, as part of a line.
    // So let's update the position to include that.
    VisiblePosition tempPosition;
    VisiblePosition startPosition = visiblePosition;
    Position p;
    RenderObject* renderer;
    while (true) {
        tempPosition = startPosition.previous();
        if (tempPosition.isNull())
            break;
        p = tempPosition.deepEquivalent();
        if (!p.deprecatedNode())
            break;
        renderer = p.deprecatedNode()->renderer();
        if (!renderer || (renderer->isRenderBlock() && !p.deprecatedEditingOffset()))
            break;
        InlineBox* box;
        int ignoredCaretOffset;
        p.getInlineBoxAndOffset(tempPosition.affinity(), box, ignoredCaretOffset);
        if (box)
            break;
        startPosition = tempPosition;
    }

    return startPosition;
}

VisiblePositionRange AccessibilityObject::leftLineVisiblePositionRange(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePositionRange();

    // make a caret selection for the position before marker position (to make sure
    // we move off of a line start)
    VisiblePosition prevVisiblePos = visiblePos.previous();
    if (prevVisiblePos.isNull())
        return VisiblePositionRange();

    VisiblePosition startPosition = startOfLine(prevVisiblePos);

    // keep searching for a valid line start position.  Unless the VisiblePosition is at the very beginning, there should
    // always be a valid line range.  However, startOfLine will return null for position next to a floating object,
    // since floating object doesn't really belong to any line.
    // This check will reposition the marker before the floating object, to ensure we get a line start.
    if (startPosition.isNull()) {
        while (startPosition.isNull() && prevVisiblePos.isNotNull()) {
            prevVisiblePos = prevVisiblePos.previous();
            startPosition = startOfLine(prevVisiblePos);
        }
    } else
        startPosition = updateAXLineStartForVisiblePosition(startPosition);

    VisiblePosition endPosition = endOfLine(prevVisiblePos);
    return VisiblePositionRange(startPosition, endPosition);
}

VisiblePositionRange AccessibilityObject::rightLineVisiblePositionRange(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePositionRange();

    // make sure we move off of a line end
    VisiblePosition nextVisiblePos = visiblePos.next();
    if (nextVisiblePos.isNull())
        return VisiblePositionRange();

    VisiblePosition startPosition = startOfLine(nextVisiblePos);

    // fetch for a valid line start position
    if (startPosition.isNull()) {
        startPosition = visiblePos;
        nextVisiblePos = nextVisiblePos.next();
    } else
        startPosition = updateAXLineStartForVisiblePosition(startPosition);

    VisiblePosition endPosition = endOfLine(nextVisiblePos);

    // as long as the position hasn't reached the end of the doc,  keep searching for a valid line end position
    // Unless the VisiblePosition is at the very end, there should always be a valid line range.  However, endOfLine will
    // return null for position by a floating object, since floating object doesn't really belong to any line.
    // This check will reposition the marker after the floating object, to ensure we get a line end.
    while (endPosition.isNull() && nextVisiblePos.isNotNull()) {
        nextVisiblePos = nextVisiblePos.next();
        endPosition = endOfLine(nextVisiblePos);
    }

    return VisiblePositionRange(startPosition, endPosition);
}

VisiblePositionRange AccessibilityObject::sentenceForPosition(const VisiblePosition& visiblePos) const
{
    // FIXME: FO 2 IMPLEMENT (currently returns incorrect answer)
    // Related? <rdar://problem/3927736> Text selection broken in 8A336
    VisiblePosition startPosition = startOfSentence(visiblePos);
    VisiblePosition endPosition = endOfSentence(startPosition);
    return VisiblePositionRange(startPosition, endPosition);
}

VisiblePositionRange AccessibilityObject::paragraphForPosition(const VisiblePosition& visiblePos) const
{
    VisiblePosition startPosition = startOfParagraph(visiblePos);
    VisiblePosition endPosition = endOfParagraph(startPosition);
    return VisiblePositionRange(startPosition, endPosition);
}

static VisiblePosition startOfStyleRange(const VisiblePosition visiblePos)
{
    RenderObject* renderer = visiblePos.deepEquivalent().deprecatedNode()->renderer();
    RenderObject* startRenderer = renderer;
    RenderStyle* style = renderer->style();

    // traverse backward by renderer to look for style change
    for (RenderObject* r = renderer->previousInPreOrder(); r; r = r->previousInPreOrder()) {
        // skip non-leaf nodes
        if (r->firstChild())
            continue;

        // stop at style change
        if (r->style() != style)
            break;

        // remember match
        startRenderer = r;
    }

    return firstPositionInOrBeforeNode(startRenderer->node());
}

static VisiblePosition endOfStyleRange(const VisiblePosition& visiblePos)
{
    RenderObject* renderer = visiblePos.deepEquivalent().deprecatedNode()->renderer();
    RenderObject* endRenderer = renderer;
    RenderStyle* style = renderer->style();

    // traverse forward by renderer to look for style change
    for (RenderObject* r = renderer->nextInPreOrder(); r; r = r->nextInPreOrder()) {
        // skip non-leaf nodes
        if (r->firstChild())
            continue;

        // stop at style change
        if (r->style() != style)
            break;

        // remember match
        endRenderer = r;
    }

    return lastPositionInOrAfterNode(endRenderer->node());
}

VisiblePositionRange AccessibilityObject::styleRangeForPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePositionRange();

    return VisiblePositionRange(startOfStyleRange(visiblePos), endOfStyleRange(visiblePos));
}

// NOTE: Consider providing this utility method as AX API
VisiblePositionRange AccessibilityObject::visiblePositionRangeForRange(const PlainTextRange& range) const
{
    unsigned textLength = getLengthForTextRange();
    if (range.start + range.length > textLength)
        return VisiblePositionRange();

    VisiblePosition startPosition = visiblePositionForIndex(range.start);
    startPosition.setAffinity(DOWNSTREAM);
    VisiblePosition endPosition = visiblePositionForIndex(range.start + range.length);
    return VisiblePositionRange(startPosition, endPosition);
}

static bool replacedNodeNeedsCharacter(Node* replacedNode)
{
    // we should always be given a rendered node and a replaced node, but be safe
    // replaced nodes are either attachments (widgets) or images
    if (!replacedNode || !replacedNode->renderer() || !replacedNode->renderer()->isReplaced() || replacedNode->isTextNode())
        return false;

    // create an AX object, but skip it if it is not supposed to be seen
    AccessibilityObject* object = replacedNode->renderer()->document()->axObjectCache()->getOrCreate(replacedNode->renderer());
    if (object->accessibilityIsIgnored())
        return false;

    return true;
}

// Finds a RenderListItem parent give a node.
static RenderListItem* renderListItemContainerForNode(Node* node)
{
    for (; node; node = node->parentNode()) {
        RenderBoxModelObject* renderer = node->renderBoxModelObject();
        if (renderer && renderer->isListItem())
            return toRenderListItem(renderer);
    }
    return 0;
}
    
// Returns the text associated with a list marker if this node is contained within a list item.
String AccessibilityObject::listMarkerTextForNodeAndPosition(Node* node, const VisiblePosition& visiblePositionStart) const
{
    // If the range does not contain the start of the line, the list marker text should not be included.
    if (!isStartOfLine(visiblePositionStart))
        return String();

    RenderListItem* listItem = renderListItemContainerForNode(node);
    if (!listItem)
        return String();
        
    // If this is in a list item, we need to manually add the text for the list marker 
    // because a RenderListMarker does not have a Node equivalent and thus does not appear
    // when iterating text.
    const String& markerText = listItem->markerText();
    if (markerText.isEmpty())
        return String();
                
    // Append text, plus the period that follows the text.
    // FIXME: Not all list marker styles are followed by a period, but this
    // sounds much better when there is a synthesized pause because of a period.
    return makeString(markerText, ". ");
}
    
String AccessibilityObject::stringForVisiblePositionRange(const VisiblePositionRange& visiblePositionRange) const
{
    if (visiblePositionRange.isNull())
        return String();

    StringBuilder builder;
    RefPtr<Range> range = makeRange(visiblePositionRange.start, visiblePositionRange.end);
    for (TextIterator it(range.get()); !it.atEnd(); it.advance()) {
        // non-zero length means textual node, zero length means replaced node (AKA "attachments" in AX)
        if (it.length()) {
            // Add a textual representation for list marker text
            String listMarkerText = listMarkerTextForNodeAndPosition(it.node(), visiblePositionRange.start);
            if (!listMarkerText.isEmpty())
                builder.append(listMarkerText);

            builder.append(it.characters(), it.length());
        } else {
            // locate the node and starting offset for this replaced range
            int exception = 0;
            Node* node = it.range()->startContainer(exception);
            ASSERT(node == it.range()->endContainer(exception));
            int offset = it.range()->startOffset(exception);

            if (replacedNodeNeedsCharacter(node->childNode(offset)))
                builder.append(objectReplacementCharacter);
        }
    }

    return builder.toString();
}

int AccessibilityObject::lengthForVisiblePositionRange(const VisiblePositionRange& visiblePositionRange) const
{
    // FIXME: Multi-byte support
    if (visiblePositionRange.isNull())
        return -1;
    
    int length = 0;
    RefPtr<Range> range = makeRange(visiblePositionRange.start, visiblePositionRange.end);
    for (TextIterator it(range.get()); !it.atEnd(); it.advance()) {
        // non-zero length means textual node, zero length means replaced node (AKA "attachments" in AX)
        if (it.length())
            length += it.length();
        else {
            // locate the node and starting offset for this replaced range
            int exception = 0;
            Node* node = it.range()->startContainer(exception);
            ASSERT(node == it.range()->endContainer(exception));
            int offset = it.range()->startOffset(exception);

            if (replacedNodeNeedsCharacter(node->childNode(offset)))
                length++;
        }
    }
    
    return length;
}

VisiblePosition AccessibilityObject::nextWordEnd(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a word end
    VisiblePosition nextVisiblePos = visiblePos.next();
    if (nextVisiblePos.isNull())
        return VisiblePosition();

    return endOfWord(nextVisiblePos, LeftWordIfOnBoundary);
}

VisiblePosition AccessibilityObject::previousWordStart(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a word start
    VisiblePosition prevVisiblePos = visiblePos.previous();
    if (prevVisiblePos.isNull())
        return VisiblePosition();

    return startOfWord(prevVisiblePos, RightWordIfOnBoundary);
}

VisiblePosition AccessibilityObject::nextLineEndPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // to make sure we move off of a line end
    VisiblePosition nextVisiblePos = visiblePos.next();
    if (nextVisiblePos.isNull())
        return VisiblePosition();

    VisiblePosition endPosition = endOfLine(nextVisiblePos);

    // as long as the position hasn't reached the end of the doc,  keep searching for a valid line end position
    // There are cases like when the position is next to a floating object that'll return null for end of line. This code will avoid returning null.
    while (endPosition.isNull() && nextVisiblePos.isNotNull()) {
        nextVisiblePos = nextVisiblePos.next();
        endPosition = endOfLine(nextVisiblePos);
    }

    return endPosition;
}

VisiblePosition AccessibilityObject::previousLineStartPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a line start
    VisiblePosition prevVisiblePos = visiblePos.previous();
    if (prevVisiblePos.isNull())
        return VisiblePosition();

    VisiblePosition startPosition = startOfLine(prevVisiblePos);

    // as long as the position hasn't reached the beginning of the doc,  keep searching for a valid line start position
    // There are cases like when the position is next to a floating object that'll return null for start of line. This code will avoid returning null.
    if (startPosition.isNull()) {
        while (startPosition.isNull() && prevVisiblePos.isNotNull()) {
            prevVisiblePos = prevVisiblePos.previous();
            startPosition = startOfLine(prevVisiblePos);
        }
    } else
        startPosition = updateAXLineStartForVisiblePosition(startPosition);

    return startPosition;
}

VisiblePosition AccessibilityObject::nextSentenceEndPosition(const VisiblePosition& visiblePos) const
{
    // FIXME: FO 2 IMPLEMENT (currently returns incorrect answer)
    // Related? <rdar://problem/3927736> Text selection broken in 8A336
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a sentence end
    VisiblePosition nextVisiblePos = visiblePos.next();
    if (nextVisiblePos.isNull())
        return VisiblePosition();

    // an empty line is considered a sentence. If it's skipped, then the sentence parser will not
    // see this empty line.  Instead, return the end position of the empty line.
    VisiblePosition endPosition;
    
    String lineString = plainText(makeRange(startOfLine(nextVisiblePos), endOfLine(nextVisiblePos)).get());
    if (lineString.isEmpty())
        endPosition = nextVisiblePos;
    else
        endPosition = endOfSentence(nextVisiblePos);

    return endPosition;
}

VisiblePosition AccessibilityObject::previousSentenceStartPosition(const VisiblePosition& visiblePos) const
{
    // FIXME: FO 2 IMPLEMENT (currently returns incorrect answer)
    // Related? <rdar://problem/3927736> Text selection broken in 8A336
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a sentence start
    VisiblePosition previousVisiblePos = visiblePos.previous();
    if (previousVisiblePos.isNull())
        return VisiblePosition();

    // treat empty line as a separate sentence.
    VisiblePosition startPosition;
    
    String lineString = plainText(makeRange(startOfLine(previousVisiblePos), endOfLine(previousVisiblePos)).get());
    if (lineString.isEmpty())
        startPosition = previousVisiblePos;
    else
        startPosition = startOfSentence(previousVisiblePos);

    return startPosition;
}

VisiblePosition AccessibilityObject::nextParagraphEndPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a paragraph end
    VisiblePosition nextPos = visiblePos.next();
    if (nextPos.isNull())
        return VisiblePosition();

    return endOfParagraph(nextPos);
}

VisiblePosition AccessibilityObject::previousParagraphStartPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return VisiblePosition();

    // make sure we move off of a paragraph start
    VisiblePosition previousPos = visiblePos.previous();
    if (previousPos.isNull())
        return VisiblePosition();

    return startOfParagraph(previousPos);
}

AccessibilityObject* AccessibilityObject::accessibilityObjectForPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return 0;

    RenderObject* obj = visiblePos.deepEquivalent().deprecatedNode()->renderer();
    if (!obj)
        return 0;

    return obj->document()->axObjectCache()->getOrCreate(obj);
}

int AccessibilityObject::lineForPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull())
        return 0;

    unsigned lineCount = 0;
    VisiblePosition currentVisiblePos = visiblePos;
    VisiblePosition savedVisiblePos;

    // move up until we get to the top
    // FIXME: This only takes us to the top of the rootEditableElement, not the top of the
    // top document.
    while (currentVisiblePos.isNotNull() && !(inSameLine(currentVisiblePos, savedVisiblePos))) {
        ++lineCount;
        savedVisiblePos = currentVisiblePos;
        VisiblePosition prevVisiblePos = previousLinePosition(currentVisiblePos, 0);
        currentVisiblePos = prevVisiblePos;
    }

    return lineCount - 1;
}

// NOTE: Consider providing this utility method as AX API
PlainTextRange AccessibilityObject::plainTextRangeForVisiblePositionRange(const VisiblePositionRange& positionRange) const
{
    int index1 = index(positionRange.start);
    int index2 = index(positionRange.end);
    if (index1 < 0 || index2 < 0 || index1 > index2)
        return PlainTextRange();

    return PlainTextRange(index1, index2 - index1);
}

// The composed character range in the text associated with this accessibility object that
// is specified by the given screen coordinates. This parameterized attribute returns the
// complete range of characters (including surrogate pairs of multi-byte glyphs) at the given
// screen coordinates.
// NOTE: This varies from AppKit when the point is below the last line. AppKit returns an
// an error in that case. We return textControl->text().length(), 1. Does this matter?
PlainTextRange AccessibilityObject::doAXRangeForPosition(const IntPoint& point) const
{
    int i = index(visiblePositionForPoint(point));
    if (i < 0)
        return PlainTextRange();

    return PlainTextRange(i, 1);
}

// Given a character index, the range of text associated with this accessibility object
// over which the style in effect at that character index applies.
PlainTextRange AccessibilityObject::doAXStyleRangeForIndex(unsigned index) const
{
    VisiblePositionRange range = styleRangeForPosition(visiblePositionForIndex(index, false));
    return plainTextRangeForVisiblePositionRange(range);
}

// Given an indexed character, the line number of the text associated with this accessibility
// object that contains the character.
unsigned AccessibilityObject::doAXLineForIndex(unsigned index)
{
    return lineForPosition(visiblePositionForIndex(index, false));
}
    
Document* AccessibilityObject::document() const
{
    FrameView* frameView = documentFrameView();
    if (!frameView)
        return 0;
    
    return frameView->frame()->document();
}

FrameView* AccessibilityObject::documentFrameView() const 
{ 
    const AccessibilityObject* object = this;
    while (object && !object->isAccessibilityRenderObject()) 
        object = object->parentObject();
        
    if (!object)
        return 0;

    return object->documentFrameView();
}
    
void AccessibilityObject::updateChildrenIfNecessary()
{
    if (!hasChildren())
        addChildren();    
}

void AccessibilityObject::clearChildren()
{
    m_children.clear();
    m_haveChildren = false;
}

AccessibilityObject* AccessibilityObject::anchorElementForNode(Node* node)
{
    RenderObject* obj = node->renderer();
    if (!obj)
        return 0;
    
    RefPtr<AccessibilityObject> axObj = obj->document()->axObjectCache()->getOrCreate(obj);
    Element* anchor = axObj->anchorElement();
    if (!anchor)
        return 0;
    
    RenderObject* anchorRenderer = anchor->renderer();
    if (!anchorRenderer)
        return 0;
    
    return anchorRenderer->document()->axObjectCache()->getOrCreate(anchorRenderer);
}
    
void AccessibilityObject::ariaTreeRows(AccessibilityChildrenVector& result)
{
    AccessibilityChildrenVector axChildren = children();
    unsigned count = axChildren.size();
    for (unsigned k = 0; k < count; ++k) {
        AccessibilityObject* obj = axChildren[k].get();
        
        // Add tree items as the rows.
        if (obj->roleValue() == TreeItemRole) 
            result.append(obj);

        // Now see if this item also has rows hiding inside of it.
        obj->ariaTreeRows(result);
    }
}
    
void AccessibilityObject::ariaTreeItemContent(AccessibilityChildrenVector& result)
{
    // The ARIA tree item content are the item that are not other tree items or their containing groups.
    AccessibilityChildrenVector axChildren = children();
    unsigned count = axChildren.size();
    for (unsigned k = 0; k < count; ++k) {
        AccessibilityObject* obj = axChildren[k].get();
        AccessibilityRole role = obj->roleValue();
        if (role == TreeItemRole || role == GroupRole)
            continue;
        
        result.append(obj);
    }
}

void AccessibilityObject::ariaTreeItemDisclosedRows(AccessibilityChildrenVector& result)
{
    AccessibilityChildrenVector axChildren = children();
    unsigned count = axChildren.size();
    for (unsigned k = 0; k < count; ++k) {
        AccessibilityObject* obj = axChildren[k].get();
        
        // Add tree items as the rows.
        if (obj->roleValue() == TreeItemRole)
            result.append(obj);
        // If it's not a tree item, then descend into the group to find more tree items.
        else 
            obj->ariaTreeRows(result);
    }    
}
    
const String& AccessibilityObject::actionVerb() const
{
    // FIXME: Need to add verbs for select elements.
    DEFINE_STATIC_LOCAL(const String, buttonAction, (AXButtonActionVerb()));
    DEFINE_STATIC_LOCAL(const String, textFieldAction, (AXTextFieldActionVerb()));
    DEFINE_STATIC_LOCAL(const String, radioButtonAction, (AXRadioButtonActionVerb()));
    DEFINE_STATIC_LOCAL(const String, checkedCheckBoxAction, (AXCheckedCheckBoxActionVerb()));
    DEFINE_STATIC_LOCAL(const String, uncheckedCheckBoxAction, (AXUncheckedCheckBoxActionVerb()));
    DEFINE_STATIC_LOCAL(const String, linkAction, (AXLinkActionVerb()));
    DEFINE_STATIC_LOCAL(const String, menuListAction, (AXMenuListActionVerb()));
    DEFINE_STATIC_LOCAL(const String, menuListPopupAction, (AXMenuListPopupActionVerb()));
    DEFINE_STATIC_LOCAL(const String, noAction, ());

    switch (roleValue()) {
    case ButtonRole:
        return buttonAction;
    case TextFieldRole:
    case TextAreaRole:
        return textFieldAction;
    case RadioButtonRole:
        return radioButtonAction;
    case CheckBoxRole:
        return isChecked() ? checkedCheckBoxAction : uncheckedCheckBoxAction;
    case LinkRole:
    case WebCoreLinkRole:
        return linkAction;
    case PopUpButtonRole:
        return menuListAction;
    case MenuListPopupRole:
        return menuListPopupAction;
    default:
        return noAction;
    }
}

bool AccessibilityObject::ariaIsMultiline() const
{
    return equalIgnoringCase(getAttribute(aria_multilineAttr), "true");
}

const AtomicString& AccessibilityObject::invalidStatus() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, invalidStatusFalse, ("false"));
    
    // aria-invalid can return false (default), grammer, spelling, or true.
    const AtomicString& ariaInvalid = getAttribute(aria_invalidAttr);
    
    // If empty or not present, it should return false.
    if (ariaInvalid.isEmpty())
        return invalidStatusFalse;
    
    return ariaInvalid;
}
 
const AtomicString& AccessibilityObject::getAttribute(const QualifiedName& attribute) const
{
    Node* elementNode = node();
    if (!elementNode)
        return nullAtom;
    
    if (!elementNode->isElementNode())
        return nullAtom;
    
    Element* element = static_cast<Element*>(elementNode);
    return element->fastGetAttribute(attribute);
}
    
// Lacking concrete evidence of orientation, horizontal means width > height. vertical is height > width;
AccessibilityOrientation AccessibilityObject::orientation() const
{
    IntRect bounds = elementRect();
    if (bounds.size().width() > bounds.size().height())
        return AccessibilityOrientationHorizontal;
    if (bounds.size().height() > bounds.size().width())
        return AccessibilityOrientationVertical;

    // A tie goes to horizontal.
    return AccessibilityOrientationHorizontal;
}    

typedef HashMap<String, AccessibilityRole, CaseFoldingHash> ARIARoleMap;

struct RoleEntry {
    String ariaRole;
    AccessibilityRole webcoreRole;
};

static ARIARoleMap* createARIARoleMap()
{
    const RoleEntry roles[] = {
        { "alert", ApplicationAlertRole },
        { "alertdialog", ApplicationAlertDialogRole },
        { "application", LandmarkApplicationRole },
        { "article", DocumentArticleRole },
        { "banner", LandmarkBannerRole },
        { "button", ButtonRole },
        { "checkbox", CheckBoxRole },
        { "complementary", LandmarkComplementaryRole },
        { "contentinfo", LandmarkContentInfoRole },
        { "dialog", ApplicationDialogRole },
        { "directory", DirectoryRole },
        { "grid", TableRole },
        { "gridcell", CellRole },
        { "columnheader", ColumnHeaderRole },
        { "combobox", ComboBoxRole },
        { "definition", DefinitionListDefinitionRole },
        { "document", DocumentRole },
        { "rowheader", RowHeaderRole },
        { "group", GroupRole },
        { "heading", HeadingRole },
        { "img", ImageRole },
        { "link", WebCoreLinkRole },
        { "list", ListRole },        
        { "listitem", ListItemRole },        
        { "listbox", ListBoxRole },
        { "log", ApplicationLogRole },
        // "option" isn't here because it may map to different roles depending on the parent element's role
        { "main", LandmarkMainRole },
        { "marquee", ApplicationMarqueeRole },
        { "math", DocumentMathRole },
        { "menu", MenuRole },
        { "menubar", MenuBarRole },
        // "menuitem" isn't here because it may map to different roles depending on the parent element's role
        { "menuitemcheckbox", MenuItemRole },
        { "menuitemradio", MenuItemRole },
        { "note", DocumentNoteRole },
        { "navigation", LandmarkNavigationRole },
        { "option", ListBoxOptionRole },
        { "presentation", PresentationalRole },
        { "progressbar", ProgressIndicatorRole },
        { "radio", RadioButtonRole },
        { "radiogroup", RadioGroupRole },
        { "region", DocumentRegionRole },
        { "row", RowRole },
        { "range", SliderRole },
        { "scrollbar", ScrollBarRole },
        { "search", LandmarkSearchRole },
        { "separator", SplitterRole },
        { "slider", SliderRole },
        { "spinbutton", ProgressIndicatorRole },
        { "status", ApplicationStatusRole },
        { "tab", TabRole },
        { "tablist", TabListRole },
        { "tabpanel", TabPanelRole },
        { "text", StaticTextRole },
        { "textbox", TextAreaRole },
        { "timer", ApplicationTimerRole },
        { "toolbar", ToolbarRole },
        { "tooltip", UserInterfaceTooltipRole },
        { "tree", TreeRole },
        { "treegrid", TreeGridRole },
        { "treeitem", TreeItemRole }
    };
    ARIARoleMap* roleMap = new ARIARoleMap;

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(roles); ++i)
        roleMap->set(roles[i].ariaRole, roles[i].webcoreRole);
    return roleMap;
}

AccessibilityRole AccessibilityObject::ariaRoleToWebCoreRole(const String& value)
{
    ASSERT(!value.isEmpty());
    
    static const ARIARoleMap* roleMap = createARIARoleMap();

    Vector<String> roleVector;
    value.split(' ', roleVector);
    AccessibilityRole role = UnknownRole;
    unsigned size = roleVector.size();
    for (unsigned i = 0; i < size; ++i) {
        String roleName = roleVector[i];
        role = roleMap->get(roleName);
        if (role)
            return role;
    }
    
    return role;
}

const AtomicString& AccessibilityObject::placeholderValue() const
{
    const AtomicString& placeholder = getAttribute(placeholderAttr);
    if (!placeholder.isEmpty())
        return placeholder;
    
    return nullAtom;
}
    
bool AccessibilityObject::isInsideARIALiveRegion() const
{
    if (supportsARIALiveRegion())
        return true;
    
    for (AccessibilityObject* axParent = parentObject(); axParent; axParent = axParent->parentObject()) {
        if (axParent->supportsARIALiveRegion())
            return true;
    }
    
    return false;
}

bool AccessibilityObject::supportsARIAAttributes() const
{
    return supportsARIALiveRegion() || supportsARIADragging() || supportsARIADropping() || supportsARIAFlowTo() || supportsARIAOwns();
}
    
bool AccessibilityObject::supportsARIALiveRegion() const
{
    const AtomicString& liveRegion = ariaLiveRegionStatus();
    return equalIgnoringCase(liveRegion, "polite") || equalIgnoringCase(liveRegion, "assertive");
}

AccessibilityObject* AccessibilityObject::elementAccessibilityHitTest(const IntPoint& point) const
{ 
    // Send the hit test back into the sub-frame if necessary.
    if (isAttachment()) {
        Widget* widget = widgetForAttachmentView();
        // Normalize the point for the widget's bounds.
        if (widget && widget->isFrameView())
            return axObjectCache()->getOrCreate(widget)->accessibilityHitTest(IntPoint(point - widget->frameRect().location()));
    }

    return const_cast<AccessibilityObject*>(this); 
}
    
AXObjectCache* AccessibilityObject::axObjectCache() const
{
    Document* doc = document();
    if (doc)
        return doc->axObjectCache();
    return 0;
}
    
AccessibilityObject* AccessibilityObject::focusedUIElement() const
{
    Document* doc = document();
    if (!doc)
        return 0;
    
    Page* page = doc->page();
    if (!page)
        return 0;
    
    return AXObjectCache::focusedUIElementForPage(page);
}
    
AccessibilitySortDirection AccessibilityObject::sortDirection() const
{
    const AtomicString& sortAttribute = getAttribute(aria_sortAttr);
    if (equalIgnoringCase(sortAttribute, "ascending"))
        return SortDirectionAscending;
    if (equalIgnoringCase(sortAttribute, "descending"))
        return SortDirectionDescending;
    
    return SortDirectionNone;
}
    
bool AccessibilityObject::supportsARIAExpanded() const
{
    return !getAttribute(aria_expandedAttr).isEmpty();
}
    
bool AccessibilityObject::isExpanded() const
{
    if (equalIgnoringCase(getAttribute(aria_expandedAttr), "true"))
        return true;
    
    return false;  
}
    
AccessibilityButtonState AccessibilityObject::checkboxOrRadioValue() const
{
    // If this is a real checkbox or radio button, AccessibilityRenderObject will handle.
    // If it's an ARIA checkbox or radio, the aria-checked attribute should be used.

    const AtomicString& result = getAttribute(aria_checkedAttr);
    if (equalIgnoringCase(result, "true"))
        return ButtonStateOn;
    if (equalIgnoringCase(result, "mixed"))
        return ButtonStateMixed;
    
    return ButtonStateOff;
}
    
} // namespace WebCore
