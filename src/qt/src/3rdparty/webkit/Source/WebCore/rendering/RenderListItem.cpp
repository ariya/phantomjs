/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
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
 *
 */

#include "config.h"
#include "RenderListItem.h"

#include "CachedImage.h"
#include "HTMLNames.h"
#include "HTMLOListElement.h"
#include "RenderListMarker.h"
#include "RenderView.h"
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderListItem::RenderListItem(Node* node)
    : RenderBlock(node)
    , m_marker(0)
    , m_hasExplicitValue(false)
    , m_isValueUpToDate(false)
    , m_notInList(false)
{
    setInline(false);
}

void RenderListItem::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);

    if (style()->listStyleType() != NoneListStyle
        || (style()->listStyleImage() && !style()->listStyleImage()->errorOccurred())) {
        RefPtr<RenderStyle> newStyle = RenderStyle::create();
        // The marker always inherits from the list item, regardless of where it might end
        // up (e.g., in some deeply nested line box). See CSS3 spec.
        newStyle->inheritFrom(style()); 
        if (!m_marker)
            m_marker = new (renderArena()) RenderListMarker(this);
        m_marker->setStyle(newStyle.release());
    } else if (m_marker) {
        m_marker->destroy();
        m_marker = 0;
    }
}

void RenderListItem::destroy()
{    
    if (m_marker) {
        m_marker->destroy();
        m_marker = 0;
    }
    RenderBlock::destroy();
}

static bool isList(Node* node)
{
    return (node->hasTagName(ulTag) || node->hasTagName(olTag));
}

static Node* enclosingList(const RenderListItem* listItem)
{
    Node* firstNode = 0;

    for (const RenderObject* renderer = listItem->parent(); renderer; renderer = renderer->parent()) {
        Node* node = renderer->node();
        if (node) {
            if (isList(node))
                return node;
            if (!firstNode)
                firstNode = node;
        }
    }

    // If there's no actual <ul> or <ol> list element, then the first found
    // node acts as our list for purposes of determining what other list items
    // should be numbered as part of the same list.
    return firstNode;
}

static RenderListItem* previousListItem(Node* list, const RenderListItem* item)
{
    for (RenderObject* renderer = item->previousInPreOrder(); renderer && renderer != list->renderer(); renderer = renderer->previousInPreOrder()) {
        if (!renderer->isListItem())
            continue;
        Node* otherList = enclosingList(toRenderListItem(renderer));
        // This item is part of our current list, so it's what we're looking for.
        if (list == otherList)
            return toRenderListItem(renderer);
        // We found ourself inside another list; lets skip the rest of it.
        // Use nextInPreOrder() here because the other list itself may actually
        // be a list item itself. We need to examine it, so we do this to counteract
        // the previousInPreOrder() that will be done by the loop.
        if (otherList)
            renderer = otherList->renderer()->nextInPreOrder();
    }
    return 0;
}

inline int RenderListItem::calcValue() const
{
    if (m_hasExplicitValue)
        return m_explicitValue;
    Node* list = enclosingList(this);
    // FIXME: This recurses to a possible depth of the length of the list.
    // That's not good -- we need to change this to an iterative algorithm.
    if (RenderListItem* previousItem = previousListItem(list, this))
        return previousItem->value() + 1;
    if (list && list->hasTagName(olTag))
        return static_cast<HTMLOListElement*>(list)->start();
    return 1;
}

void RenderListItem::updateValueNow() const
{
    m_value = calcValue();
    m_isValueUpToDate = true;
}

bool RenderListItem::isEmpty() const
{
    return lastChild() == m_marker;
}

static RenderObject* getParentOfFirstLineBox(RenderBlock* curr, RenderObject* marker)
{
    RenderObject* firstChild = curr->firstChild();
    if (!firstChild)
        return 0;

    bool inQuirksMode = curr->document()->inQuirksMode();
    for (RenderObject* currChild = firstChild; currChild; currChild = currChild->nextSibling()) {
        if (currChild == marker)
            continue;

        if (currChild->isInline() && (!currChild->isRenderInline() || curr->generatesLineBoxesForInlineChild(currChild)))
            return curr;

        if (currChild->isFloating() || currChild->isPositioned())
            continue;

        if (currChild->isTable() || !currChild->isRenderBlock() || (currChild->isBox() && toRenderBox(currChild)->isWritingModeRoot()))
            break;

        if (curr->isListItem() && inQuirksMode && currChild->node() &&
            (currChild->node()->hasTagName(ulTag)|| currChild->node()->hasTagName(olTag)))
            break;

        RenderObject* lineBox = getParentOfFirstLineBox(toRenderBlock(currChild), marker);
        if (lineBox)
            return lineBox;
    }

    return 0;
}

void RenderListItem::updateValue()
{
    if (!m_hasExplicitValue) {
        m_isValueUpToDate = false;
        if (m_marker)
            m_marker->setNeedsLayoutAndPrefWidthsRecalc();
    }
}

static RenderObject* firstNonMarkerChild(RenderObject* parent)
{
    RenderObject* result = parent->firstChild();
    while (result && result->isListMarker())
        result = result->nextSibling();
    return result;
}

void RenderListItem::updateMarkerLocation()
{
    // Sanity check the location of our marker.
    if (m_marker) {
        RenderObject* markerPar = m_marker->parent();
        RenderObject* lineBoxParent = getParentOfFirstLineBox(this, m_marker);
        if (!lineBoxParent) {
            // If the marker is currently contained inside an anonymous box,
            // then we are the only item in that anonymous box (since no line box
            // parent was found).  It's ok to just leave the marker where it is
            // in this case.
            if (markerPar && markerPar->isAnonymousBlock())
                lineBoxParent = markerPar;
            else
                lineBoxParent = this;
        }

        if (markerPar != lineBoxParent || m_marker->preferredLogicalWidthsDirty()) {
            // Removing and adding the marker can trigger repainting in
            // containers other than ourselves, so we need to disable LayoutState.
            view()->disableLayoutState();
            updateFirstLetter();
            m_marker->remove();
            if (!lineBoxParent)
                lineBoxParent = this;
            lineBoxParent->addChild(m_marker, firstNonMarkerChild(lineBoxParent));
            if (m_marker->preferredLogicalWidthsDirty())
                m_marker->computePreferredLogicalWidths();
            view()->enableLayoutState();
        }
    }
}

void RenderListItem::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());
    
    updateMarkerLocation();

    RenderBlock::computePreferredLogicalWidths();
}

void RenderListItem::layout()
{
    ASSERT(needsLayout()); 

    updateMarkerLocation();    
    RenderBlock::layout();
}

void RenderListItem::addOverflowFromChildren()
{
    RenderBlock::addOverflowFromChildren();
    positionListMarker();
}

void RenderListItem::positionListMarker()
{
    if (m_marker && m_marker->parent()->isBox() && !m_marker->isInside() && m_marker->inlineBoxWrapper()) {
        int markerOldLogicalLeft = m_marker->logicalLeft();
        int blockOffset = 0;
        int lineOffset = 0;
        for (RenderBox* o = m_marker->parentBox(); o != this; o = o->parentBox()) {
            blockOffset += o->logicalTop();
            lineOffset += o->logicalLeft();
        }

        bool adjustOverflow = false;
        int markerLogicalLeft;
        RootInlineBox* root = m_marker->inlineBoxWrapper()->root();
        bool hitSelfPaintingLayer = false;
        
        RootInlineBox* rootBox = m_marker->inlineBoxWrapper()->root();
        int lineTop = rootBox->lineTop();
        int lineBottom = rootBox->lineBottom();

        // FIXME: Need to account for relative positioning in the layout overflow.
        if (style()->isLeftToRightDirection()) {
            int leftLineOffset = logicalLeftOffsetForLine(blockOffset, logicalLeftOffsetForLine(blockOffset, false), false);
            markerLogicalLeft = leftLineOffset - lineOffset - paddingStart() - borderStart() + m_marker->marginStart();
            m_marker->inlineBoxWrapper()->adjustLineDirectionPosition(markerLogicalLeft - markerOldLogicalLeft);
            for (InlineFlowBox* box = m_marker->inlineBoxWrapper()->parent(); box; box = box->parent()) {
                IntRect newLogicalVisualOverflowRect = box->logicalVisualOverflowRect(lineTop, lineBottom);
                IntRect newLogicalLayoutOverflowRect = box->logicalLayoutOverflowRect(lineTop, lineBottom);
                if (markerLogicalLeft < newLogicalVisualOverflowRect.x() && !hitSelfPaintingLayer) {
                    newLogicalVisualOverflowRect.setWidth(newLogicalVisualOverflowRect.maxX() - markerLogicalLeft);
                    newLogicalVisualOverflowRect.setX(markerLogicalLeft);
                    if (box == root)
                        adjustOverflow = true;
                }
                if (markerLogicalLeft < newLogicalLayoutOverflowRect.x()) {
                    newLogicalLayoutOverflowRect.setWidth(newLogicalLayoutOverflowRect.maxX() - markerLogicalLeft);
                    newLogicalLayoutOverflowRect.setX(markerLogicalLeft);
                    if (box == root)
                        adjustOverflow = true;
                }
                box->setOverflowFromLogicalRects(newLogicalLayoutOverflowRect, newLogicalVisualOverflowRect, lineTop, lineBottom);
                if (box->boxModelObject()->hasSelfPaintingLayer())
                    hitSelfPaintingLayer = true;
            }
        } else {
            markerLogicalLeft = m_marker->logicalLeft() + paddingStart() + borderStart() + m_marker->marginEnd();
            int rightLineOffset = logicalRightOffsetForLine(blockOffset, logicalRightOffsetForLine(blockOffset, false), false);
            markerLogicalLeft = rightLineOffset - lineOffset + paddingStart() + borderStart() + m_marker->marginEnd();
            m_marker->inlineBoxWrapper()->adjustLineDirectionPosition(markerLogicalLeft - markerOldLogicalLeft);
            for (InlineFlowBox* box = m_marker->inlineBoxWrapper()->parent(); box; box = box->parent()) {
                IntRect newLogicalVisualOverflowRect = box->logicalVisualOverflowRect(lineTop, lineBottom);
                IntRect newLogicalLayoutOverflowRect = box->logicalLayoutOverflowRect(lineTop, lineBottom);
                if (markerLogicalLeft + m_marker->logicalWidth() > newLogicalVisualOverflowRect.maxX() && !hitSelfPaintingLayer) {
                    newLogicalVisualOverflowRect.setWidth(markerLogicalLeft + m_marker->logicalWidth() - newLogicalVisualOverflowRect.x());
                    if (box == root)
                        adjustOverflow = true;
                }
                if (markerLogicalLeft + m_marker->logicalWidth() > newLogicalLayoutOverflowRect.maxX()) {
                    newLogicalLayoutOverflowRect.setWidth(markerLogicalLeft + m_marker->logicalWidth() - newLogicalLayoutOverflowRect.x());
                    if (box == root)
                        adjustOverflow = true;
                }
                box->setOverflowFromLogicalRects(newLogicalLayoutOverflowRect, newLogicalVisualOverflowRect, lineTop, lineBottom);
                
                if (box->boxModelObject()->hasSelfPaintingLayer())
                    hitSelfPaintingLayer = true;
            }
        }

        if (adjustOverflow) {
            IntRect markerRect(markerLogicalLeft + lineOffset, blockOffset, m_marker->width(), m_marker->height());
            if (!style()->isHorizontalWritingMode())
                markerRect = markerRect.transposedRect();
            RenderBox* o = m_marker;
            bool propagateVisualOverflow = true;
            bool propagateLayoutOverflow = true;
            do {
                o = o->parentBox();
                if (o->hasOverflowClip())
                    propagateVisualOverflow = false;
                if (o->isRenderBlock()) {
                    if (propagateVisualOverflow)
                        toRenderBlock(o)->addVisualOverflow(markerRect);
                    if (propagateLayoutOverflow)
                        toRenderBlock(o)->addLayoutOverflow(markerRect);
                }
                if (o->hasOverflowClip())
                    propagateLayoutOverflow = false;
                if (o->hasSelfPaintingLayer())
                    propagateVisualOverflow = false;
                markerRect.move(-o->x(), -o->y());
            } while (o != this && propagateVisualOverflow && propagateLayoutOverflow);
        }
    }
}

void RenderListItem::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!logicalHeight())
        return;

    RenderBlock::paint(paintInfo, tx, ty);
}

const String& RenderListItem::markerText() const
{
    if (m_marker)
        return m_marker->text();
    DEFINE_STATIC_LOCAL(String, staticNullString, ());
    return staticNullString;
}

String RenderListItem::markerTextWithSuffix() const
{
    if (!m_marker)
        return String();

    // Append the suffix for the marker in the right place depending
    // on the direction of the text (right-to-left or left-to-right).

    const String& markerText = m_marker->text();
    const String markerSuffix = m_marker->suffix();
    Vector<UChar> resultVector;

    if (!m_marker->style()->isLeftToRightDirection())
        resultVector.append(markerSuffix.characters(), markerSuffix.length());

    resultVector.append(markerText.characters(), markerText.length());

    if (m_marker->style()->isLeftToRightDirection())
        resultVector.append(markerSuffix.characters(), markerSuffix.length());

    return String::adopt(resultVector);
}

void RenderListItem::explicitValueChanged()
{
    if (m_marker)
        m_marker->setNeedsLayoutAndPrefWidthsRecalc();
    Node* listNode = enclosingList(this);
    RenderObject* listRenderer = 0;
    if (listNode)
        listRenderer = listNode->renderer();
    for (RenderObject* renderer = this; renderer; renderer = renderer->nextInPreOrder(listRenderer))
        if (renderer->isListItem()) {
            RenderListItem* item = toRenderListItem(renderer);
            if (!item->m_hasExplicitValue) {
                item->m_isValueUpToDate = false;
                if (RenderListMarker* marker = item->m_marker)
                    marker->setNeedsLayoutAndPrefWidthsRecalc();
            }
        }
}

void RenderListItem::setExplicitValue(int value)
{
    ASSERT(node());

    if (m_hasExplicitValue && m_explicitValue == value)
        return;
    m_explicitValue = value;
    m_value = value;
    m_hasExplicitValue = true;
    explicitValueChanged();
}

void RenderListItem::clearExplicitValue()
{
    ASSERT(node());

    if (!m_hasExplicitValue)
        return;
    m_hasExplicitValue = false;
    m_isValueUpToDate = false;
    explicitValueChanged();
}

void RenderListItem::updateListMarkerNumbers()
{
    Node* listNode = enclosingList(this);
    ASSERT(listNode && listNode->renderer());
    if (!listNode || !listNode->renderer())
        return;

    RenderObject* list = listNode->renderer();
    RenderObject* child = nextInPreOrder(list);
    while (child) {
        if (child->node() && isList(child->node())) {
            // We've found a nested, independent list: nothing to do here.
            child = child->nextInPreOrderAfterChildren(list);
            continue;
        }

        if (child->isListItem()) {
            RenderListItem* item = toRenderListItem(child);

            if (!item->m_isValueUpToDate) {
                // If an item has been marked for update before, we can safely
                // assume that all the following ones have too.
                // This gives us the opportunity to stop here and avoid
                // marking the same nodes again.
                break;
            }

            item->updateValue();
        }

        child = child->nextInPreOrder(list);
    }
}

} // namespace WebCore
