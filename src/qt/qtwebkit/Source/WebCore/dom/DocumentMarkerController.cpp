/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "DocumentMarkerController.h"

#include "Node.h"
#include "NodeTraversal.h"
#include "Range.h"
#include "RenderObject.h"
#include "RenderedDocumentMarker.h"
#include "TextIterator.h"
#include <stdio.h>

namespace WebCore {

inline bool DocumentMarkerController::possiblyHasMarkers(DocumentMarker::MarkerTypes types)
{
    return m_possiblyExistingMarkerTypes.intersects(types);
}

DocumentMarkerController::DocumentMarkerController()
    : m_possiblyExistingMarkerTypes(0)
{
}

DocumentMarkerController::~DocumentMarkerController()
{
}

void DocumentMarkerController::detach()
{
    m_markers.clear();
    m_possiblyExistingMarkerTypes = 0;
}

void DocumentMarkerController::addMarker(Range* range, DocumentMarker::MarkerType type, const String& description)
{
    // Use a TextIterator to visit the potentially multiple nodes the range covers.
    for (TextIterator markedText(range); !markedText.atEnd(); markedText.advance()) {
        RefPtr<Range> textPiece = markedText.range();
        addMarker(textPiece->startContainer(), DocumentMarker(type, textPiece->startOffset(), textPiece->endOffset(), description));
    }
}

void DocumentMarkerController::addMarker(Range* range, DocumentMarker::MarkerType type)
{
    // Use a TextIterator to visit the potentially multiple nodes the range covers.
    for (TextIterator markedText(range); !markedText.atEnd(); markedText.advance()) {
        RefPtr<Range> textPiece = markedText.range();
        addMarker(textPiece->startContainer(), DocumentMarker(type, textPiece->startOffset(), textPiece->endOffset()));
    }

}

void DocumentMarkerController::addMarkerToNode(Node* node, unsigned startOffset, unsigned length, DocumentMarker::MarkerType type)
{
    addMarker(node, DocumentMarker(type, startOffset, startOffset + length));
}

void DocumentMarkerController::addMarkerToNode(Node* node, unsigned startOffset, unsigned length, DocumentMarker::MarkerType type, PassRefPtr<DocumentMarkerDetails> details)
{
    addMarker(node, DocumentMarker(type, startOffset, startOffset + length, details));
}


void DocumentMarkerController::addTextMatchMarker(const Range* range, bool activeMatch)
{
    // Use a TextIterator to visit the potentially multiple nodes the range covers.
    for (TextIterator markedText(range); !markedText.atEnd(); markedText.advance()) {
        RefPtr<Range> textPiece = markedText.range();
        unsigned startOffset = textPiece->startOffset();
        unsigned endOffset = textPiece->endOffset();
        addMarker(textPiece->startContainer(), DocumentMarker(startOffset, endOffset, activeMatch));
        if (endOffset > startOffset) {
            // Rendered rects for markers in WebKit are not populated until each time
            // the markers are painted. However, we need it to happen sooner, because
            // the whole purpose of tickmarks on the scrollbar is to show where
            // matches off-screen are (that haven't been painted yet).
            Node* node = textPiece->startContainer();
            Vector<DocumentMarker*> markers = markersFor(node);
            static_cast<RenderedDocumentMarker*>(markers[markers.size() - 1])->setRenderedRect(range->boundingBox());
        }
    }
}

void DocumentMarkerController::removeMarkers(Range* range, DocumentMarker::MarkerTypes markerTypes, RemovePartiallyOverlappingMarkerOrNot shouldRemovePartiallyOverlappingMarker)
{
    for (TextIterator markedText(range); !markedText.atEnd(); markedText.advance()) {
        if (!possiblyHasMarkers(markerTypes))
            return;
        ASSERT(!m_markers.isEmpty());

        RefPtr<Range> textPiece = markedText.range();
        int startOffset = textPiece->startOffset();
        int endOffset = textPiece->endOffset();
        removeMarkers(textPiece->startContainer(), startOffset, endOffset - startOffset, markerTypes, shouldRemovePartiallyOverlappingMarker);
    }
}

// Markers are stored in order sorted by their start offset.
// Markers of the same type do not overlap each other.

void DocumentMarkerController::addMarker(Node* node, const DocumentMarker& newMarker) 
{
    ASSERT(newMarker.endOffset() >= newMarker.startOffset());
    if (newMarker.endOffset() == newMarker.startOffset())
        return;

    m_possiblyExistingMarkerTypes.add(newMarker.type());

    OwnPtr<MarkerList>& list = m_markers.add(node, nullptr).iterator->value;

    if (!list) {
        list = adoptPtr(new MarkerList);
        list->append(RenderedDocumentMarker(newMarker));
    } else {
        RenderedDocumentMarker toInsert(newMarker);
        size_t numMarkers = list->size();
        size_t i;
        // Iterate over all markers whose start offset is less than or equal to the new marker's.
        // If one of them is of the same type as the new marker and touches it or intersects with it
        // (there is at most one), remove it and adjust the new marker's start offset to encompass it.
        for (i = 0; i < numMarkers; ++i) {
            DocumentMarker marker = list->at(i);
            if (marker.startOffset() > toInsert.startOffset())
                break;
            if (marker.type() == toInsert.type() && marker.endOffset() >= toInsert.startOffset()) {
                toInsert.setStartOffset(marker.startOffset());
                list->remove(i);
                numMarkers--;
                break;
            }
        }
        size_t j = i;
        // Iterate over all markers whose end offset is less than or equal to the new marker's,
        // removing markers of the same type as the new marker which touch it or intersect with it,
        // adjusting the new marker's end offset to cover them if necessary.
        while (j < numMarkers) {
            DocumentMarker marker = list->at(j);
            if (marker.startOffset() > toInsert.endOffset())
                break;
            if (marker.type() == toInsert.type()) {
                list->remove(j);
                if (toInsert.endOffset() <= marker.endOffset()) {
                    toInsert.setEndOffset(marker.endOffset());
                    break;
                }
                numMarkers--;
            } else
                j++;
        }
        // At this point i points to the node before which we want to insert.
        list->insert(i, RenderedDocumentMarker(toInsert));
    }

    // repaint the affected node
    if (node->renderer())
        node->renderer()->repaint();
}

// copies markers from srcNode to dstNode, applying the specified shift delta to the copies.  The shift is
// useful if, e.g., the caller has created the dstNode from a non-prefix substring of the srcNode.
void DocumentMarkerController::copyMarkers(Node* srcNode, unsigned startOffset, int length, Node* dstNode, int delta)
{
    if (length <= 0)
        return;

    if (!possiblyHasMarkers(DocumentMarker::AllMarkers()))
        return;
    ASSERT(!m_markers.isEmpty());

    MarkerList* list = m_markers.get(srcNode);
    if (!list)
        return;

    bool docDirty = false;
    unsigned endOffset = startOffset + length - 1;
    for (size_t i = 0; i != list->size(); ++i) {
        DocumentMarker marker = list->at(i);

        // stop if we are now past the specified range
        if (marker.startOffset() > endOffset)
            break;

        // skip marker that is before the specified range or is the wrong type
        if (marker.endOffset() < startOffset)
            continue;

        // pin the marker to the specified range and apply the shift delta
        docDirty = true;
        if (marker.startOffset() < startOffset)
            marker.setStartOffset(startOffset);
        if (marker.endOffset() > endOffset)
            marker.setEndOffset(endOffset);
        marker.shiftOffsets(delta);

        addMarker(dstNode, marker);
    }

    // repaint the affected node
    if (docDirty && dstNode->renderer())
        dstNode->renderer()->repaint();
}

void DocumentMarkerController::removeMarkers(Node* node, unsigned startOffset, int length, DocumentMarker::MarkerTypes markerTypes, RemovePartiallyOverlappingMarkerOrNot shouldRemovePartiallyOverlappingMarker)
{
    if (length <= 0)
        return;

    if (!possiblyHasMarkers(markerTypes))
        return;
    ASSERT(!(m_markers.isEmpty()));

    MarkerList* list = m_markers.get(node);
    if (!list)
        return;

    bool docDirty = false;
    unsigned endOffset = startOffset + length;
    for (size_t i = 0; i < list->size();) {
        DocumentMarker marker = list->at(i);

        // markers are returned in order, so stop if we are now past the specified range
        if (marker.startOffset() >= endOffset)
            break;

        // skip marker that is wrong type or before target
        if (marker.endOffset() <= startOffset || !markerTypes.contains(marker.type())) {
            i++;
            continue;
        }

        // at this point we know that marker and target intersect in some way
        docDirty = true;

        // pitch the old marker
        list->remove(i);

        if (shouldRemovePartiallyOverlappingMarker)
            // Stop here. Don't add resulting slices back.
            continue;

        // add either of the resulting slices that are left after removing target
        if (startOffset > marker.startOffset()) {
            DocumentMarker newLeft = marker;
            newLeft.setEndOffset(startOffset);
            list->insert(i, RenderedDocumentMarker(newLeft));
            // i now points to the newly-inserted node, but we want to skip that one
            i++;
        }
        if (marker.endOffset() > endOffset) {
            DocumentMarker newRight = marker;
            newRight.setStartOffset(endOffset);
            list->insert(i, RenderedDocumentMarker(newRight));
            // i now points to the newly-inserted node, but we want to skip that one
            i++;
        }
    }

    if (list->isEmpty()) {
        m_markers.remove(node);
        if (m_markers.isEmpty())
            m_possiblyExistingMarkerTypes = 0;
    }

    // repaint the affected node
    if (docDirty && node->renderer())
        node->renderer()->repaint();
}

DocumentMarker* DocumentMarkerController::markerContainingPoint(const LayoutPoint& point, DocumentMarker::MarkerType markerType)
{
    if (!possiblyHasMarkers(markerType))
        return 0;
    ASSERT(!(m_markers.isEmpty()));

    // outer loop: process each node that contains any markers
    MarkerMap::iterator end = m_markers.end();
    for (MarkerMap::iterator nodeIterator = m_markers.begin(); nodeIterator != end; ++nodeIterator) {
        // inner loop; process each marker in this node
        MarkerList* list = nodeIterator->value.get();
        unsigned markerCount = list->size();
        for (unsigned markerIndex = 0; markerIndex < markerCount; ++markerIndex) {
            RenderedDocumentMarker& marker = list->at(markerIndex);

            // skip marker that is wrong type
            if (marker.type() != markerType)
                continue;

            if (marker.contains(point))
                return &marker;
        }
    }

    return 0;
}

Vector<DocumentMarker*> DocumentMarkerController::markersFor(Node* node, DocumentMarker::MarkerTypes markerTypes)
{
    Vector<DocumentMarker*> result;
    MarkerList* list = m_markers.get(node);
    if (!list)
        return result;

    for (size_t i = 0; i < list->size(); ++i) {
        if (markerTypes.contains(list->at(i).type()))
            result.append(&(list->at(i)));
    }

    return result;
}

// FIXME: Should be removed after all relevant patches are landed
Vector<DocumentMarker> DocumentMarkerController::markersForNode(Node* node)
{
    Vector<DocumentMarker> result;
    MarkerList* list = m_markers.get(node);
    if (!list)
        return result;

    for (size_t i = 0; i < list->size(); ++i)
        result.append(list->at(i));

    return result;
}

Vector<DocumentMarker*> DocumentMarkerController::markersInRange(Range* range, DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return Vector<DocumentMarker*>();

    Vector<DocumentMarker*> foundMarkers;

    Node* startContainer = range->startContainer();
    ASSERT(startContainer);
    Node* endContainer = range->endContainer();
    ASSERT(endContainer);

    Node* pastLastNode = range->pastLastNode();
    for (Node* node = range->firstNode(); node != pastLastNode; node = NodeTraversal::next(node)) {
        Vector<DocumentMarker*> markers = markersFor(node);
        Vector<DocumentMarker*>::const_iterator end = markers.end();
        for (Vector<DocumentMarker*>::const_iterator it = markers.begin(); it != end; ++it) {
            DocumentMarker* marker = *it;
            if (!markerTypes.contains(marker->type()))
                continue;
            if (node == startContainer && marker->endOffset() <= static_cast<unsigned>(range->startOffset()))
                continue;
            if (node == endContainer && marker->startOffset() >= static_cast<unsigned>(range->endOffset()))
                continue;
            foundMarkers.append(marker);
        }
    }
    return foundMarkers;
}

Vector<IntRect> DocumentMarkerController::renderedRectsForMarkers(DocumentMarker::MarkerType markerType)
{
    Vector<IntRect> result;

    if (!possiblyHasMarkers(markerType))
        return result;
    ASSERT(!(m_markers.isEmpty()));

    // outer loop: process each node
    MarkerMap::iterator end = m_markers.end();
    for (MarkerMap::iterator nodeIterator = m_markers.begin(); nodeIterator != end; ++nodeIterator) {
        // inner loop; process each marker in this node
        MarkerList* list = nodeIterator->value.get();
        unsigned markerCount = list->size();
        for (unsigned markerIndex = 0; markerIndex < markerCount; ++markerIndex) {
            const RenderedDocumentMarker& marker = list->at(markerIndex);

            // skip marker that is wrong type
            if (marker.type() != markerType)
                continue;

            if (!marker.isRendered())
                continue;

            result.append(marker.renderedRect());
        }
    }

    return result;
}

void DocumentMarkerController::removeMarkers(Node* node, DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return;
    ASSERT(!m_markers.isEmpty());
    
    MarkerMap::iterator iterator = m_markers.find(node);
    if (iterator != m_markers.end())
        removeMarkersFromList(iterator, markerTypes);
}

void DocumentMarkerController::removeMarkers(DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return;
    ASSERT(!m_markers.isEmpty());

    Vector<RefPtr<Node> > nodesWithMarkers;
    copyKeysToVector(m_markers, nodesWithMarkers);
    unsigned size = nodesWithMarkers.size();
    for (unsigned i = 0; i < size; ++i) {
        MarkerMap::iterator iterator = m_markers.find(nodesWithMarkers[i]);
        if (iterator != m_markers.end())
            removeMarkersFromList(iterator, markerTypes);
    }

    m_possiblyExistingMarkerTypes.remove(markerTypes);
}

void DocumentMarkerController::removeMarkersFromList(MarkerMap::iterator iterator, DocumentMarker::MarkerTypes markerTypes)
{
    bool needsRepainting = false;
    bool listCanBeRemoved;

    if (markerTypes == DocumentMarker::AllMarkers()) {
        needsRepainting = true;
        listCanBeRemoved = true;
    } else {
        MarkerList* list = iterator->value.get();

        for (size_t i = 0; i != list->size(); ) {
            DocumentMarker marker = list->at(i);

            // skip nodes that are not of the specified type
            if (!markerTypes.contains(marker.type())) {
                ++i;
                continue;
            }

            // pitch the old marker
            list->remove(i);
            needsRepainting = true;
            // i now is the index of the next marker
        }

        listCanBeRemoved = list->isEmpty();
    }

    if (needsRepainting) {
        if (RenderObject* renderer = iterator->key->renderer())
            renderer->repaint();
    }

    if (listCanBeRemoved) {
        m_markers.remove(iterator);
        if (m_markers.isEmpty())
            m_possiblyExistingMarkerTypes = 0;
    }
}

void DocumentMarkerController::repaintMarkers(DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return;
    ASSERT(!m_markers.isEmpty());

    // outer loop: process each markered node in the document
    MarkerMap::iterator end = m_markers.end();
    for (MarkerMap::iterator i = m_markers.begin(); i != end; ++i) {
        Node* node = i->key.get();

        // inner loop: process each marker in the current node
        MarkerList* list = i->value.get();
        bool nodeNeedsRepaint = false;
        for (size_t i = 0; i != list->size(); ++i) {
            DocumentMarker marker = list->at(i);

            // skip nodes that are not of the specified type
            if (markerTypes.contains(marker.type())) {
                nodeNeedsRepaint = true;
                break;
            }
        }

        if (!nodeNeedsRepaint)
            continue;

        // cause the node to be redrawn
        if (RenderObject* renderer = node->renderer())
            renderer->repaint();
    }
}

void DocumentMarkerController::invalidateRenderedRectsForMarkersInRect(const LayoutRect& r)
{
    // outer loop: process each markered node in the document
    MarkerMap::iterator end = m_markers.end();
    for (MarkerMap::iterator i = m_markers.begin(); i != end; ++i) {

        // inner loop: process each rect in the current node
        MarkerList* list = i->value.get();
        for (size_t listIndex = 0; listIndex < list->size(); ++listIndex)
            list->at(listIndex).invalidate(r);
    }
}

void DocumentMarkerController::shiftMarkers(Node* node, unsigned startOffset, int delta)
{
    if (!possiblyHasMarkers(DocumentMarker::AllMarkers()))
        return;
    ASSERT(!m_markers.isEmpty());

    MarkerList* list = m_markers.get(node);
    if (!list)
        return;

    bool docDirty = false;
    for (size_t i = 0; i != list->size(); ++i) {
        RenderedDocumentMarker& marker = list->at(i);
        if (marker.startOffset() >= startOffset) {
            ASSERT((int)marker.startOffset() + delta >= 0);
            marker.shiftOffsets(delta);
            docDirty = true;

            // Marker moved, so previously-computed rendered rectangle is now invalid
            marker.invalidate();
        }
    }

    // repaint the affected node
    if (docDirty && node->renderer())
        node->renderer()->repaint();
}

void DocumentMarkerController::setMarkersActive(Range* range, bool active)
{
    if (!possiblyHasMarkers(DocumentMarker::AllMarkers()))
        return;
    ASSERT(!m_markers.isEmpty());

    Node* startContainer = range->startContainer();
    Node* endContainer = range->endContainer();

    Node* pastLastNode = range->pastLastNode();

    for (Node* node = range->firstNode(); node != pastLastNode; node = NodeTraversal::next(node)) {
        int startOffset = node == startContainer ? range->startOffset() : 0;
        int endOffset = node == endContainer ? range->endOffset() : INT_MAX;
        setMarkersActive(node, startOffset, endOffset, active);
    }
}

void DocumentMarkerController::setMarkersActive(Node* node, unsigned startOffset, unsigned endOffset, bool active)
{
    MarkerList* list = m_markers.get(node);
    if (!list)
        return;

    bool docDirty = false;
    for (size_t i = 0; i != list->size(); ++i) {
        DocumentMarker& marker = list->at(i);

        // Markers are returned in order, so stop if we are now past the specified range.
        if (marker.startOffset() >= endOffset)
            break;

        // Skip marker that is wrong type or before target.
        if (marker.endOffset() < startOffset || marker.type() != DocumentMarker::TextMatch)
            continue;

        marker.setActiveMatch(active);
        docDirty = true;
    }

    // repaint the affected node
    if (docDirty && node->renderer())
        node->renderer()->repaint();
}

bool DocumentMarkerController::hasMarkers(Range* range, DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return false;
    ASSERT(!m_markers.isEmpty());

    Node* startContainer = range->startContainer();
    ASSERT(startContainer);
    Node* endContainer = range->endContainer();
    ASSERT(endContainer);

    Node* pastLastNode = range->pastLastNode();
    for (Node* node = range->firstNode(); node != pastLastNode; node = NodeTraversal::next(node)) {
        Vector<DocumentMarker*> markers = markersFor(node);
        Vector<DocumentMarker*>::const_iterator end = markers.end();
        for (Vector<DocumentMarker*>::const_iterator it = markers.begin(); it != end; ++it) {
            DocumentMarker* marker = *it;
            if (!markerTypes.contains(marker->type()))
                continue;
            if (node == startContainer && marker->endOffset() <= static_cast<unsigned>(range->startOffset()))
                continue;
            if (node == endContainer && marker->startOffset() >= static_cast<unsigned>(range->endOffset()))
                continue;
            return true;
        }
    }
    return false;
}

void DocumentMarkerController::clearDescriptionOnMarkersIntersectingRange(Range* range, DocumentMarker::MarkerTypes markerTypes)
{
    if (!possiblyHasMarkers(markerTypes))
        return;
    ASSERT(!m_markers.isEmpty());

    Node* startContainer = range->startContainer();
    Node* endContainer = range->endContainer();

    Node* pastLastNode = range->pastLastNode();
    for (Node* node = range->firstNode(); node != pastLastNode; node = NodeTraversal::next(node)) {
        unsigned startOffset = node == startContainer ? range->startOffset() : 0;
        unsigned endOffset = node == endContainer ? static_cast<unsigned>(range->endOffset()) : std::numeric_limits<unsigned>::max();
        MarkerList* list = m_markers.get(node);
        if (!list)
            continue;

        for (size_t i = 0; i < list->size(); ++i) {
            DocumentMarker& marker = list->at(i);

            // markers are returned in order, so stop if we are now past the specified range
            if (marker.startOffset() >= endOffset)
                break;

            // skip marker that is wrong type or before target
            if (marker.endOffset() <= startOffset || !markerTypes.contains(marker.type())) {
                i++;
                continue;
            }

            marker.clearDetails();
        }
    }
}

#ifndef NDEBUG
void DocumentMarkerController::showMarkers() const
{
    fprintf(stderr, "%d nodes have markers:\n", m_markers.size());
    MarkerMap::const_iterator end = m_markers.end();
    for (MarkerMap::const_iterator nodeIterator = m_markers.begin(); nodeIterator != end; ++nodeIterator) {
        Node* node = nodeIterator->key.get();
        fprintf(stderr, "%p", node);
        MarkerList* list = nodeIterator->value.get();
        for (unsigned markerIndex = 0; markerIndex < list->size(); ++markerIndex) {
            const DocumentMarker& marker = list->at(markerIndex);
            fprintf(stderr, " %d:[%d:%d](%d)", marker.type(), marker.startOffset(), marker.endOffset(), marker.activeMatch());
        }

        fprintf(stderr, "\n");
    }
}
#endif

} // namespace WebCore

#ifndef NDEBUG
void showDocumentMarkers(const WebCore::DocumentMarkerController* controller)
{
    if (controller)
        controller->showMarkers();
}
#endif
