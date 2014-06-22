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
#include "AccessibilityTable.h"
#include "Editor.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "NodeList.h"
#include "NodeTraversal.h"
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
#include "RenderedPosition.h"
#include "Settings.h"
#include "TextCheckerClient.h"
#include "TextCheckingHelper.h"
#include "TextIterator.h"
#include "UserGestureIndicator.h"
#include "VisibleUnits.h"
#include "htmlediting.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

AccessibilityObject::AccessibilityObject()
    : m_id(0)
    , m_haveChildren(false)
    , m_role(UnknownRole)
    , m_lastKnownIsIgnoredValue(DefaultBehavior)
#if PLATFORM(GTK) || (PLATFORM(EFL) && HAVE(ACCESSIBILITY))
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
    // Clear any children and call detachFromParent on them so that
    // no children are left with dangling pointers to their parent.
    clearChildren();

#if HAVE(ACCESSIBILITY)
    setWrapper(0);
#endif
}

bool AccessibilityObject::isDetached() const
{
#if HAVE(ACCESSIBILITY)
    return !wrapper();
#else
    return true;
#endif
}

bool AccessibilityObject::isAccessibilityObjectSearchMatchAtIndex(AccessibilityObject* axObject, AccessibilitySearchCriteria* criteria, size_t index)
{
    switch (criteria->searchKeys[index]) {
    // The AnyTypeSearchKey matches any non-null AccessibilityObject.
    case AnyTypeSearchKey:
        return true;
        
    case BlockquoteSameLevelSearchKey:
        return criteria->startObject
            && axObject->isBlockquote()
            && axObject->blockquoteLevel() == criteria->startObject->blockquoteLevel();
        
    case BlockquoteSearchKey:
        return axObject->isBlockquote();
        
    case BoldFontSearchKey:
        return axObject->hasBoldFont();
        
    case ButtonSearchKey:
        return axObject->isButton();
        
    case CheckBoxSearchKey:
        return axObject->isCheckbox();
        
    case ControlSearchKey:
        return axObject->isControl();
        
    case DifferentTypeSearchKey:
        return criteria->startObject
            && axObject->roleValue() != criteria->startObject->roleValue();
        
    case FontChangeSearchKey:
        return criteria->startObject
            && !axObject->hasSameFont(criteria->startObject->renderer());
        
    case FontColorChangeSearchKey:
        return criteria->startObject
            && !axObject->hasSameFontColor(criteria->startObject->renderer());
        
    case FrameSearchKey:
        return axObject->isWebArea();
        
    case GraphicSearchKey:
        return axObject->isImage();
        
    case HeadingLevel1SearchKey:
        return axObject->headingLevel() == 1;
        
    case HeadingLevel2SearchKey:
        return axObject->headingLevel() == 2;
        
    case HeadingLevel3SearchKey:
        return axObject->headingLevel() == 3;
        
    case HeadingLevel4SearchKey:
        return axObject->headingLevel() == 4;
        
    case HeadingLevel5SearchKey:
        return axObject->headingLevel() == 5;
        
    case HeadingLevel6SearchKey:
        return axObject->headingLevel() == 6;
        
    case HeadingSameLevelSearchKey:
        return criteria->startObject
            && axObject->isHeading()
            && axObject->headingLevel() == criteria->startObject->headingLevel();
        
    case HeadingSearchKey:
        return axObject->isHeading();
    
    case HighlightedSearchKey:
        return axObject->hasHighlighting();
            
    case ItalicFontSearchKey:
        return axObject->hasItalicFont();
        
    case LandmarkSearchKey:
        return axObject->isLandmark();
        
    case LinkSearchKey:
        return axObject->isLink();
        
    case ListSearchKey:
        return axObject->isList();
        
    case LiveRegionSearchKey:
        return axObject->supportsARIALiveRegion();
        
    case MisspelledWordSearchKey:
        return axObject->hasMisspelling();
        
    case PlainTextSearchKey:
        return axObject->hasPlainText();
        
    case RadioGroupSearchKey:
        return axObject->isRadioGroup();
        
    case SameTypeSearchKey:
        return criteria->startObject
            && axObject->roleValue() == criteria->startObject->roleValue();
        
    case StaticTextSearchKey:
        return axObject->isStaticText();
        
    case StyleChangeSearchKey:
        return criteria->startObject
            && !axObject->hasSameStyle(criteria->startObject->renderer());
        
    case TableSameLevelSearchKey:
        return criteria->startObject
            && axObject->isAccessibilityTable()
            && axObject->tableLevel() == criteria->startObject->tableLevel();
        
    case TableSearchKey:
        return axObject->isAccessibilityTable();
        
    case TextFieldSearchKey:
        return axObject->isTextControl();
        
    case UnderlineSearchKey:
        return axObject->hasUnderline();
        
    case UnvisitedLinkSearchKey:
        return axObject->isUnvisited();
        
    case VisitedLinkSearchKey:
        return axObject->isVisited();
        
    default:
        return false;
    }
}

bool AccessibilityObject::isAccessibilityObjectSearchMatch(AccessibilityObject* axObject, AccessibilitySearchCriteria* criteria)
{
    if (!axObject || !criteria)
        return false;
    
    size_t length = criteria->searchKeys.size();
    for (size_t i = 0; i < length; ++i) {
        if (isAccessibilityObjectSearchMatchAtIndex(axObject, criteria, i)) {
            if (criteria->visibleOnly && !axObject->isOnscreen())
                return false;
            return true;
        }
    }
    return false;
}

bool AccessibilityObject::isAccessibilityTextSearchMatch(AccessibilityObject* axObject, AccessibilitySearchCriteria* criteria)
{
    if (!axObject || !criteria)
        return false;
    
    return axObject->accessibilityObjectContainsText(criteria->searchText);
}

bool AccessibilityObject::accessibilityObjectContainsText(String* text) const
{
    // If text is null or empty we return true.
    return !text
        || text->isEmpty()
        || title().contains(*text, false)
        || accessibilityDescription().contains(*text, false)
        || stringValue().contains(*text, false);
}

bool AccessibilityObject::isBlockquote() const
{
    return node() && node()->hasTagName(blockquoteTag);
}

bool AccessibilityObject::isTextControl() const
{
    switch (roleValue()) {
    case TextAreaRole:
    case TextFieldRole:
    case ComboBoxRole:
        return true;
    default:
        return false;
    }
}
    
bool AccessibilityObject::isARIATextControl() const
{
    return ariaRoleAttribute() == TextAreaRole || ariaRoleAttribute() == TextFieldRole;
}

bool AccessibilityObject::isLandmark() const
{
    AccessibilityRole role = roleValue();
    
    return role == LandmarkApplicationRole
        || role == LandmarkBannerRole
        || role == LandmarkComplementaryRole
        || role == LandmarkContentInfoRole
        || role == LandmarkMainRole
        || role == LandmarkNavigationRole
        || role == LandmarkSearchRole;
}

bool AccessibilityObject::hasMisspelling() const
{
    if (!node())
        return false;
    
    Document* document = node()->document();
    if (!document)
        return false;
    
    Frame* frame = document->frame();
    if (!frame)
        return false;
    
    Editor& editor = frame->editor();
    
    TextCheckerClient* textChecker = editor.textChecker();
    if (!textChecker)
        return false;
    
    const UChar* chars = stringValue().characters();
    int charsLength = stringValue().length();
    bool isMisspelled = false;

    if (unifiedTextCheckerEnabled(frame)) {
        Vector<TextCheckingResult> results;
        checkTextOfParagraph(textChecker, chars, charsLength, TextCheckingTypeSpelling, results);
        if (!results.isEmpty())
            isMisspelled = true;
        return isMisspelled;
    }

    int misspellingLength = 0;
    int misspellingLocation = -1;
    textChecker->checkSpellingOfString(chars, charsLength, &misspellingLocation, &misspellingLength);
    if (misspellingLength || misspellingLocation != -1)
        isMisspelled = true;
    
    return isMisspelled;
}

int AccessibilityObject::blockquoteLevel() const
{
    int level = 0;
    for (Node* elementNode = node(); elementNode; elementNode = elementNode->parentNode()) {
        if (elementNode->hasTagName(blockquoteTag))
            ++level;
    }
    
    return level;
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
    if (!node)
        return 0;

    Document* document = node->document();
    if (!document)
        return 0;

    AXObjectCache* cache = document->axObjectCache();

    AccessibilityObject* accessibleObject = cache->getOrCreate(node->renderer());
    while (accessibleObject && accessibleObject->accessibilityIsIgnored()) {
        node = NodeTraversal::next(node);

        while (node && !node->renderer())
            node = NodeTraversal::nextSkippingChildren(node);

        if (!node)
            return 0;

        accessibleObject = cache->getOrCreate(node->renderer());
    }

    return accessibleObject;
}

static void appendAccessibilityObject(AccessibilityObject* object, AccessibilityObject::AccessibilityChildrenVector& results)
{
    // Find the next descendant of this attachment object so search can continue through frames.
    if (object->isAttachment()) {
        Widget* widget = object->widgetForAttachmentView();
        if (!widget || !widget->isFrameView())
            return;
        
        Document* doc = toFrameView(widget)->frame()->document();
        if (!doc || !doc->renderer())
            return;
        
        object = object->axObjectCache()->getOrCreate(doc);
    }

    if (object)
        results.append(object);
}
    
static void appendChildrenToArray(AccessibilityObject* object, bool isForward, AccessibilityObject* startObject, AccessibilityObject::AccessibilityChildrenVector& results)
{
    AccessibilityObject::AccessibilityChildrenVector searchChildren;
    // A table's children includes elements whose own children are also the table's children (due to the way the Mac exposes tables).
    // The rows from the table should be queried, since those are direct descendants of the table, and they contain content.
    if (object->isAccessibilityTable())
        searchChildren = toAccessibilityTable(object)->rows();
    else
        searchChildren = object->children();

    size_t childrenSize = searchChildren.size();

    size_t startIndex = isForward ? childrenSize : 0;
    size_t endIndex = isForward ? 0 : childrenSize;

    size_t searchPosition = startObject ? searchChildren.find(startObject) : WTF::notFound;
    if (searchPosition != WTF::notFound) {
        if (isForward)
            endIndex = searchPosition + 1;
        else
            endIndex = searchPosition;
    }

    // This is broken into two statements so that it's easier read.
    if (isForward) {
        for (size_t i = startIndex; i > endIndex; i--)
            appendAccessibilityObject(searchChildren.at(i - 1).get(), results);
    } else {
        for (size_t i = startIndex; i < endIndex; i++)
            appendAccessibilityObject(searchChildren.at(i).get(), results);
    }
}

// Returns true if the number of results is now >= the number of results desired.
bool AccessibilityObject::objectMatchesSearchCriteriaWithResultLimit(AccessibilityObject* object, AccessibilitySearchCriteria* criteria, AccessibilityChildrenVector& results)
{
    if (isAccessibilityObjectSearchMatch(object, criteria) && isAccessibilityTextSearchMatch(object, criteria)) {
        results.append(object);
        
        // Enough results were found to stop searching.
        if (results.size() >= criteria->resultsLimit)
            return true;
    }
    
    return false;
}

void AccessibilityObject::findMatchingObjects(AccessibilitySearchCriteria* criteria, AccessibilityChildrenVector& results)
{
    ASSERT(criteria);
    
    if (!criteria)
        return;

    axObjectCache()->startCachingComputedObjectAttributesUntilTreeMutates();

    // This search mechanism only searches the elements before/after the starting object.
    // It does this by stepping up the parent chain and at each level doing a DFS.
    
    // If there's no start object, it means we want to search everything.
    AccessibilityObject* startObject = criteria->startObject;
    if (!startObject)
        startObject = this;
    
    bool isForward = criteria->searchDirection == SearchDirectionNext;
    
    // In the first iteration of the loop, it will examine the children of the start object for matches.
    // However, when going backwards, those children should not be considered, so the loop is skipped ahead.
    AccessibilityObject* previousObject = 0;
    if (!isForward) {
        previousObject = startObject;
        startObject = startObject->parentObjectUnignored();
    }
    
    // The outer loop steps up the parent chain each time (unignored is important here because otherwise elements would be searched twice)
    for (AccessibilityObject* stopSearchElement = parentObjectUnignored(); startObject != stopSearchElement; startObject = startObject->parentObjectUnignored()) {

        // Only append the children after/before the previous element, so that the search does not check elements that are 
        // already behind/ahead of start element.
        AccessibilityChildrenVector searchStack;
        appendChildrenToArray(startObject, isForward, previousObject, searchStack);

        // This now does a DFS at the current level of the parent.
        while (!searchStack.isEmpty()) {
            AccessibilityObject* searchObject = searchStack.last().get();
            searchStack.removeLast();
            
            if (objectMatchesSearchCriteriaWithResultLimit(searchObject, criteria, results))
                break;
            
            appendChildrenToArray(searchObject, isForward, 0, searchStack);
        }
        
        if (results.size() >= criteria->resultsLimit)
            break;

        // When moving backwards, the parent object needs to be checked, because technically it's "before" the starting element.
        if (!isForward && objectMatchesSearchCriteriaWithResultLimit(startObject, criteria, results))
            break;

        previousObject = startObject;
    }
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
    
bool AccessibilityObject::isRangeControl() const
{
    switch (roleValue()) {
    case ProgressIndicatorRole:
    case SliderRole:
    case ScrollBarRole:
    case SpinButtonRole:
        return true;
    default:
        return false;
    }
}

IntPoint AccessibilityObject::clickPoint()
{
    LayoutRect rect = elementRect();
    return roundedIntPoint(LayoutPoint(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2));
}

IntRect AccessibilityObject::boundingBoxForQuads(RenderObject* obj, const Vector<FloatQuad>& quads)
{
    ASSERT(obj);
    if (!obj)
        return IntRect();
    
    size_t count = quads.size();
    if (!count)
        return IntRect();
    
    IntRect result;
    for (size_t i = 0; i < count; ++i) {
        IntRect r = quads[i].enclosingBoundingBox();
        if (!r.isEmpty()) {
            if (obj->style()->hasAppearance())
                obj->theme()->adjustRepaintRect(obj, r);
            result.unite(r);
        }
    }
    return result;
}
    
bool AccessibilityObject::press() const
{
    Element* actionElem = actionElement();
    if (!actionElem)
        return false;
    if (Frame* f = actionElem->document()->frame())
        f->loader()->resetMultipleFormSubmissionProtection();
    
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
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
    while (true) {
        tempPosition = startPosition.previous();
        if (tempPosition.isNull())
            break;
        Position p = tempPosition.deepEquivalent();
        RenderObject* renderer = p.deprecatedNode()->renderer();
        if (!renderer || (renderer->isRenderBlock() && !p.deprecatedEditingOffset()))
            break;
        if (!RenderedPosition(tempPosition).isNull())
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

static VisiblePosition startOfStyleRange(const VisiblePosition& visiblePos)
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
    AccessibilityObject* object = replacedNode->renderer()->document()->axObjectCache()->getOrCreate(replacedNode);
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
    return listItem->markerTextWithSuffix();
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

            it.appendTextToStringBuilder(builder);
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

#if HAVE(ACCESSIBILITY)
int AccessibilityObject::lineForPosition(const VisiblePosition& visiblePos) const
{
    if (visiblePos.isNull() || !node())
        return -1;

    // If the position is not in the same editable region as this AX object, return -1.
    Node* containerNode = visiblePos.deepEquivalent().containerNode();
    if (!containerNode->containsIncludingShadowDOM(node()) && !node()->containsIncludingShadowDOM(containerNode))
        return -1;

    int lineCount = -1;
    VisiblePosition currentVisiblePos = visiblePos;
    VisiblePosition savedVisiblePos;

    // move up until we get to the top
    // FIXME: This only takes us to the top of the rootEditableElement, not the top of the
    // top document.
    do {
        savedVisiblePos = currentVisiblePos;
        VisiblePosition prevVisiblePos = previousLinePosition(currentVisiblePos, 0, HasEditableAXRole);
        currentVisiblePos = prevVisiblePos;
        ++lineCount;
    }  while (currentVisiblePos.isNotNull() && !(inSameLine(currentVisiblePos, savedVisiblePos)));

    return lineCount;
}
#endif

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

#if HAVE(ACCESSIBILITY)
void AccessibilityObject::updateBackingStore()
{
    // Updating the layout may delete this object.
    if (Document* document = this->document())
        document->updateLayoutIgnorePendingStylesheets();
}
#endif

Document* AccessibilityObject::document() const
{
    FrameView* frameView = documentFrameView();
    if (!frameView)
        return 0;
    
    return frameView->frame()->document();
}
    
Page* AccessibilityObject::page() const
{
    Document* document = this->document();
    if (!document)
        return 0;
    return document->page();
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

#if HAVE(ACCESSIBILITY)
const AccessibilityObject::AccessibilityChildrenVector& AccessibilityObject::children()
{
    updateChildrenIfNecessary();

    return m_children;
}
#endif

void AccessibilityObject::updateChildrenIfNecessary()
{
    if (!hasChildren())
        addChildren();    
}
    
void AccessibilityObject::clearChildren()
{
    // Some objects have weak pointers to their parents and those associations need to be detached.
    size_t length = m_children.size();
    for (size_t i = 0; i < length; i++)
        m_children[i]->detachFromParent();
    
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

#if HAVE(ACCESSIBILITY)
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
    DEFINE_STATIC_LOCAL(const String, listItemAction, (AXListItemActionVerb()));
    DEFINE_STATIC_LOCAL(const String, noAction, ());

    switch (roleValue()) {
    case ButtonRole:
    case ToggleButtonRole:
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
    case ListItemRole:
        return listItemAction;
    default:
        return noAction;
    }
}
#endif

bool AccessibilityObject::ariaIsMultiline() const
{
    return equalIgnoringCase(getAttribute(aria_multilineAttr), "true");
}

const AtomicString& AccessibilityObject::invalidStatus() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, invalidStatusFalse, ("false", AtomicString::ConstructFromLiteral));
    
    // aria-invalid can return false (default), grammer, spelling, or true.
    const AtomicString& ariaInvalid = getAttribute(aria_invalidAttr);
    
    // If empty or not present, it should return false.
    if (ariaInvalid.isEmpty())
        return invalidStatusFalse;
    
    return ariaInvalid;
}
 
bool AccessibilityObject::hasAttribute(const QualifiedName& attribute) const
{
    Node* elementNode = node();
    if (!elementNode)
        return false;
    
    if (!elementNode->isElementNode())
        return false;
    
    Element* element = toElement(elementNode);
    return element->fastHasAttribute(attribute);
}
    
const AtomicString& AccessibilityObject::getAttribute(const QualifiedName& attribute) const
{
    Node* elementNode = node();
    if (!elementNode)
        return nullAtom;
    
    if (!elementNode->isElementNode())
        return nullAtom;
    
    Element* element = toElement(elementNode);
    return element->fastGetAttribute(attribute);
}
    
// Lacking concrete evidence of orientation, horizontal means width > height. vertical is height > width;
AccessibilityOrientation AccessibilityObject::orientation() const
{
    LayoutRect bounds = elementRect();
    if (bounds.size().width() > bounds.size().height())
        return AccessibilityOrientationHorizontal;
    if (bounds.size().height() > bounds.size().width())
        return AccessibilityOrientationVertical;

    // A tie goes to horizontal.
    return AccessibilityOrientationHorizontal;
}    

bool AccessibilityObject::isDescendantOfObject(const AccessibilityObject* axObject) const
{
    if (!axObject || !axObject->hasChildren())
        return false;

    for (const AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        if (parent == axObject)
            return true;
    }
    return false;
}

bool AccessibilityObject::isAncestorOfObject(const AccessibilityObject* axObject) const
{
    if (!axObject)
        return false;

    return this == axObject || axObject->isDescendantOfObject(this);
}

AccessibilityObject* AccessibilityObject::firstAnonymousBlockChild() const
{
    for (AccessibilityObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->renderer() && child->renderer()->isAnonymousBlock())
            return child;
    }
    return 0;
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
        { "definition", DefinitionRole },
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
        { "menuitem", MenuItemRole },
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
        { "scrollbar", ScrollBarRole },
        { "search", LandmarkSearchRole },
        { "separator", SplitterRole },
        { "slider", SliderRole },
        { "spinbutton", SpinButtonRole },
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

bool AccessibilityObject::hasHighlighting() const
{
    for (Node* node = this->node(); node; node = node->parentNode()) {
        if (node->hasTagName(markTag))
            return true;
    }
    
    return false;
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
    return supportsARIALiveRegion()
        || supportsARIADragging()
        || supportsARIADropping()
        || supportsARIAFlowTo()
        || supportsARIAOwns()
        || hasAttribute(aria_labelAttr);
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
    
    // Check if there are any mock elements that need to be handled.
    size_t count = m_children.size();
    for (size_t k = 0; k < count; k++) {
        if (m_children[k]->isMockObject() && m_children[k]->elementRect().contains(point))
            return m_children[k]->elementAccessibilityHitTest(point);
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

bool AccessibilityObject::supportsRangeValue() const
{
    return isProgressIndicator()
        || isSlider()
        || isScrollbar()
        || isSpinButton();
}
    
bool AccessibilityObject::supportsARIASetSize() const
{
    return hasAttribute(aria_setsizeAttr);
}

bool AccessibilityObject::supportsARIAPosInSet() const
{
    return hasAttribute(aria_posinsetAttr);
}
    
int AccessibilityObject::ariaSetSize() const
{
    return getAttribute(aria_setsizeAttr).toInt();
}

int AccessibilityObject::ariaPosInSet() const
{
    return getAttribute(aria_posinsetAttr).toInt();
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

// This is a 1-dimensional scroll offset helper function that's applied
// separately in the horizontal and vertical directions, because the
// logic is the same. The goal is to compute the best scroll offset
// in order to make an object visible within a viewport.
//
// In case the whole object cannot fit, you can specify a
// subfocus - a smaller region within the object that should
// be prioritized. If the whole object can fit, the subfocus is
// ignored.
//
// Example: the viewport is scrolled to the right just enough
// that the object is in view.
//   Before:
//   +----------Viewport---------+
//                         +---Object---+
//                         +--SubFocus--+
//
//   After:
//          +----------Viewport---------+
//                         +---Object---+
//                         +--SubFocus--+
//
// When constraints cannot be fully satisfied, the min
// (left/top) position takes precedence over the max (right/bottom).
//
// Note that the return value represents the ideal new scroll offset.
// This may be out of range - the calling function should clip this
// to the available range.
static int computeBestScrollOffset(int currentScrollOffset,
                                   int subfocusMin, int subfocusMax,
                                   int objectMin, int objectMax,
                                   int viewportMin, int viewportMax) {
    int viewportSize = viewportMax - viewportMin;

    // If the focus size is larger than the viewport size, shrink it in the
    // direction of subfocus.
    if (objectMax - objectMin > viewportSize) {
        // Subfocus must be within focus:
        subfocusMin = std::max(subfocusMin, objectMin);
        subfocusMax = std::min(subfocusMax, objectMax);

        // Subfocus must be no larger than the viewport size; favor top/left.
        if (subfocusMax - subfocusMin > viewportSize)
            subfocusMax = subfocusMin + viewportSize;

        if (subfocusMin + viewportSize > objectMax)
            objectMin = objectMax - viewportSize;
        else {
            objectMin = subfocusMin;
            objectMax = subfocusMin + viewportSize;
        }
    }

    // Exit now if the focus is already within the viewport.
    if (objectMin - currentScrollOffset >= viewportMin
        && objectMax - currentScrollOffset <= viewportMax)
        return currentScrollOffset;

    // Scroll left if we're too far to the right.
    if (objectMax - currentScrollOffset > viewportMax)
        return objectMax - viewportMax;

    // Scroll right if we're too far to the left.
    if (objectMin - currentScrollOffset < viewportMin)
        return objectMin - viewportMin;

    ASSERT_NOT_REACHED();

    // This shouldn't happen.
    return currentScrollOffset;
}

bool AccessibilityObject::isOnscreen() const
{   
    bool isOnscreen = true;

    // To figure out if the element is onscreen, we start by building of a stack starting with the
    // element, and then include every scrollable parent in the hierarchy.
    Vector<const AccessibilityObject*> objects;
    
    objects.append(this);
    for (AccessibilityObject* parentObject = this->parentObject(); parentObject; parentObject = parentObject->parentObject()) {
        if (parentObject->getScrollableAreaIfScrollable())
            objects.append(parentObject);
    }

    // Now, go back through that chain and make sure each inner object is within the
    // visible bounds of the outer object.
    size_t levels = objects.size() - 1;
    
    for (size_t i = levels; i >= 1; i--) {
        const AccessibilityObject* outer = objects[i];
        const AccessibilityObject* inner = objects[i - 1];
        const IntRect outerRect = i < levels ? pixelSnappedIntRect(outer->boundingBoxRect()) : outer->getScrollableAreaIfScrollable()->visibleContentRect();
        const IntRect innerRect = pixelSnappedIntRect(inner->isAccessibilityScrollView() ? inner->parentObject()->boundingBoxRect() : inner->boundingBoxRect());
        
        if (!outerRect.intersects(innerRect)) {
            isOnscreen = false;
            break;
        }
    }
    
    return isOnscreen;
}

void AccessibilityObject::scrollToMakeVisible() const
{
    IntRect objectRect = pixelSnappedIntRect(boundingBoxRect());
    objectRect.setLocation(IntPoint());
    scrollToMakeVisibleWithSubFocus(objectRect);
}

void AccessibilityObject::scrollToMakeVisibleWithSubFocus(const IntRect& subfocus) const
{
    // Search up the parent chain until we find the first one that's scrollable.
    AccessibilityObject* scrollParent = parentObject();
    ScrollableArea* scrollableArea;
    for (scrollableArea = 0;
         scrollParent && !(scrollableArea = scrollParent->getScrollableAreaIfScrollable());
         scrollParent = scrollParent->parentObject()) { }
    if (!scrollableArea)
        return;

    LayoutRect objectRect = boundingBoxRect();
    IntPoint scrollPosition = scrollableArea->scrollPosition();
    IntRect scrollVisibleRect = scrollableArea->visibleContentRect();

    int desiredX = computeBestScrollOffset(
        scrollPosition.x(),
        objectRect.x() + subfocus.x(), objectRect.x() + subfocus.maxX(),
        objectRect.x(), objectRect.maxX(),
        0, scrollVisibleRect.width());
    int desiredY = computeBestScrollOffset(
        scrollPosition.y(),
        objectRect.y() + subfocus.y(), objectRect.y() + subfocus.maxY(),
        objectRect.y(), objectRect.maxY(),
        0, scrollVisibleRect.height());

    scrollParent->scrollTo(IntPoint(desiredX, desiredY));

    // Recursively make sure the scroll parent itself is visible.
    if (scrollParent->parentObject())
        scrollParent->scrollToMakeVisible();
}

void AccessibilityObject::scrollToGlobalPoint(const IntPoint& globalPoint) const
{
    // Search up the parent chain and create a vector of all scrollable parent objects
    // and ending with this object itself.
    Vector<const AccessibilityObject*> objects;

    objects.append(this);
    for (AccessibilityObject* parentObject = this->parentObject(); parentObject; parentObject = parentObject->parentObject()) {
        if (parentObject->getScrollableAreaIfScrollable())
            objects.append(parentObject);
    }

    objects.reverse();

    // Start with the outermost scrollable (the main window) and try to scroll the
    // next innermost object to the given point.
    int offsetX = 0, offsetY = 0;
    IntPoint point = globalPoint;
    size_t levels = objects.size() - 1;
    for (size_t i = 0; i < levels; i++) {
        const AccessibilityObject* outer = objects[i];
        const AccessibilityObject* inner = objects[i + 1];

        ScrollableArea* scrollableArea = outer->getScrollableAreaIfScrollable();

        LayoutRect innerRect = inner->isAccessibilityScrollView() ? inner->parentObject()->boundingBoxRect() : inner->boundingBoxRect();
        LayoutRect objectRect = innerRect;
        IntPoint scrollPosition = scrollableArea->scrollPosition();

        // Convert the object rect into local coordinates.
        objectRect.move(offsetX, offsetY);
        if (!outer->isAccessibilityScrollView())
            objectRect.move(scrollPosition.x(), scrollPosition.y());

        int desiredX = computeBestScrollOffset(
            0,
            objectRect.x(), objectRect.maxX(),
            objectRect.x(), objectRect.maxX(),
            point.x(), point.x());
        int desiredY = computeBestScrollOffset(
            0,
            objectRect.y(), objectRect.maxY(),
            objectRect.y(), objectRect.maxY(),
            point.y(), point.y());
        outer->scrollTo(IntPoint(desiredX, desiredY));

        if (outer->isAccessibilityScrollView() && !inner->isAccessibilityScrollView()) {
            // If outer object we just scrolled is a scroll view (main window or iframe) but the
            // inner object is not, keep track of the coordinate transformation to apply to
            // future nested calculations.
            scrollPosition = scrollableArea->scrollPosition();
            offsetX -= (scrollPosition.x() + point.x());
            offsetY -= (scrollPosition.y() + point.y());
            point.move(scrollPosition.x() - innerRect.x(),
                       scrollPosition.y() - innerRect.y());
        } else if (inner->isAccessibilityScrollView()) {
            // Otherwise, if the inner object is a scroll view, reset the coordinate transformation.
            offsetX = 0;
            offsetY = 0;
        }
    }
}

bool AccessibilityObject::lastKnownIsIgnoredValue()
{
    if (m_lastKnownIsIgnoredValue == DefaultBehavior)
        m_lastKnownIsIgnoredValue = accessibilityIsIgnored() ? IgnoreObject : IncludeObject;

    return m_lastKnownIsIgnoredValue == IgnoreObject;
}

void AccessibilityObject::setLastKnownIsIgnoredValue(bool isIgnored)
{
    m_lastKnownIsIgnoredValue = isIgnored ? IgnoreObject : IncludeObject;
}

void AccessibilityObject::notifyIfIgnoredValueChanged()
{
    bool isIgnored = accessibilityIsIgnored();
    if (lastKnownIsIgnoredValue() != isIgnored) {
        axObjectCache()->childrenChanged(parentObject());
        setLastKnownIsIgnoredValue(isIgnored);
    }
}

bool AccessibilityObject::ariaPressedIsPresent() const
{
    return !getAttribute(aria_pressedAttr).isEmpty();
}

TextIteratorBehavior AccessibilityObject::textIteratorBehaviorForTextRange() const
{
    TextIteratorBehavior behavior = TextIteratorIgnoresStyleVisibility;
    
#if PLATFORM(GTK)
    // We need to emit replaced elements for GTK, and present
    // them with the 'object replacement character' (0xFFFC).
    behavior = static_cast<TextIteratorBehavior>(behavior | TextIteratorEmitsObjectReplacementCharacters);
#endif
    
    return behavior;
}
    
AccessibilityRole AccessibilityObject::buttonRoleType() const
{
    // If aria-pressed is present, then it should be exposed as a toggle button.
    // http://www.w3.org/TR/wai-aria/states_and_properties#aria-pressed
    if (ariaPressedIsPresent())
        return ToggleButtonRole;
    if (ariaHasPopup())
        return PopUpButtonRole;
    // We don't contemplate RadioButtonRole, as it depends on the input
    // type.

    return ButtonRole;
}

bool AccessibilityObject::isButton() const
{
    AccessibilityRole role = roleValue();

    return role == ButtonRole || role == PopUpButtonRole || role == ToggleButtonRole;
}

bool AccessibilityObject::accessibilityIsIgnoredByDefault() const
{
    return defaultObjectInclusion() == IgnoreObject;
}

bool AccessibilityObject::ariaIsHidden() const
{
    if (equalIgnoringCase(getAttribute(aria_hiddenAttr), "true"))
        return true;
    
    for (AccessibilityObject* object = parentObject(); object; object = object->parentObject()) {
        if (equalIgnoringCase(object->getAttribute(aria_hiddenAttr), "true"))
            return true;
    }
    
    return false;
}

AccessibilityObjectInclusion AccessibilityObject::defaultObjectInclusion() const
{
    if (ariaIsHidden())
        return IgnoreObject;
    
    if (isPresentationalChildOfAriaRole())
        return IgnoreObject;
    
    return accessibilityPlatformIncludesObject();
}
    
bool AccessibilityObject::accessibilityIsIgnored() const
{
    AXComputedObjectAttributeCache* attributeCache = axObjectCache()->computedObjectAttributeCache();
    if (attributeCache) {
        AccessibilityObjectInclusion ignored = attributeCache->getIgnored(axObjectID());
        switch (ignored) {
        case IgnoreObject:
            return true;
        case IncludeObject:
            return false;
        case DefaultBehavior:
            break;
        }
    }

    bool result = computeAccessibilityIsIgnored();

    if (attributeCache)
        attributeCache->setIgnored(axObjectID(), result ? IgnoreObject : IncludeObject);

    return result;
}

} // namespace WebCore
