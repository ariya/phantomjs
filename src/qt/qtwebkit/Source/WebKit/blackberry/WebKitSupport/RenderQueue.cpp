/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "RenderQueue.h"

#include "BackingStore_p.h"
#include "SurfacePool.h"
#include "WebPageClient.h"
#include "WebPage_p.h"

#include <wtf/NonCopyingSort.h>

#define DEBUG_RENDER_QUEUE 0
#define DEBUG_RENDER_QUEUE_SORT 0

#if DEBUG_RENDER_QUEUE
#include <BlackBerryPlatformLog.h>
#include <wtf/CurrentTime.h>
#endif

namespace BlackBerry {
namespace WebKit {

template<SortDirection sortDirection>
static inline int compareRectOneDirection(const TileIndex& t1, const TileIndex& t2)
{
    switch (sortDirection) {
    case LeftToRight:
        return t1.i() - t2.i();
    case RightToLeft:
        return t2.i() - t1.i();
    case TopToBottom:
        return t1.j() - t2.j();
    case BottomToTop:
        return t2.j() - t1.j();
    default:
        break;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

template<SortDirection primarySortDirection, SortDirection secondarySortDirection>
static bool tileIndexIsLessThan(const TileIndex& t1, const TileIndex& t2)
{
    int primaryResult = compareRectOneDirection<primarySortDirection>(t1, t2);
    if (primaryResult || secondarySortDirection == primarySortDirection)
        return primaryResult < 0;
    return compareRectOneDirection<secondarySortDirection>(t1, t2) < 0;
}

typedef bool (*FuncTileIndexLessThan)(const TileIndex& t1, const TileIndex& t2);
static FuncTileIndexLessThan tileIndexLessThanFunction(SortDirection primary, SortDirection secondary)
{
    static FuncTileIndexLessThan s_tileIndexLessThanFunctions[NumSortDirections][NumSortDirections] = { { 0 } };
    static bool s_initialized = false;
    if (!s_initialized) {
#define ADD_COMPARE_FUNCTION(_primary, _secondary) \
        s_tileIndexLessThanFunctions[_primary][_secondary] = tileIndexIsLessThan<_primary, _secondary>

        ADD_COMPARE_FUNCTION(LeftToRight, LeftToRight);
        ADD_COMPARE_FUNCTION(LeftToRight, RightToLeft);
        ADD_COMPARE_FUNCTION(LeftToRight, TopToBottom);
        ADD_COMPARE_FUNCTION(LeftToRight, BottomToTop);

        ADD_COMPARE_FUNCTION(RightToLeft, LeftToRight);
        ADD_COMPARE_FUNCTION(RightToLeft, RightToLeft);
        ADD_COMPARE_FUNCTION(RightToLeft, TopToBottom);
        ADD_COMPARE_FUNCTION(RightToLeft, BottomToTop);

        ADD_COMPARE_FUNCTION(TopToBottom, LeftToRight);
        ADD_COMPARE_FUNCTION(TopToBottom, RightToLeft);
        ADD_COMPARE_FUNCTION(TopToBottom, TopToBottom);
        ADD_COMPARE_FUNCTION(TopToBottom, BottomToTop);

        ADD_COMPARE_FUNCTION(BottomToTop, LeftToRight);
        ADD_COMPARE_FUNCTION(BottomToTop, RightToLeft);
        ADD_COMPARE_FUNCTION(BottomToTop, TopToBottom);
        ADD_COMPARE_FUNCTION(BottomToTop, BottomToTop);
#undef ADD_COMPARE_FUNCTION

        s_initialized = true;
    }

    return s_tileIndexLessThanFunctions[primary][secondary];
}

class TileIndexLessThan {
public:
    TileIndexLessThan(SortDirection primarySortDirection, SortDirection secondarySortDirection)
        : m_tileIndexIsLessThan(tileIndexLessThanFunction(primarySortDirection, secondarySortDirection))
    {
    }

    bool operator()(const TileIndex& t1, const TileIndex& t2)
    {
        return m_tileIndexIsLessThan(t1, t2);
    }

private:
    FuncTileIndexLessThan m_tileIndexIsLessThan;
};

RenderQueue::RenderQueue(BackingStorePrivate* parent)
    : m_parent(parent)
    , m_rectsAddedToRegularRenderJobsInCurrentCycle(false)
    , m_currentRegularRenderJobsBatchUnderPressure(false)
    , m_primarySortDirection(TopToBottom)
    , m_secondarySortDirection(LeftToRight)
{
}

void RenderQueue::reset()
{
    m_rectsAddedToRegularRenderJobsInCurrentCycle = false;
    m_currentRegularRenderJobsBatchUnderPressure = false;
    m_primarySortDirection = TopToBottom;
    m_secondarySortDirection = LeftToRight;
    m_visibleZoomJobs.clear();
    m_visibleZoomJobsCompleted.clear();
    m_visibleScrollJobs.clear();
    m_visibleScrollJobsCompleted.clear();
    m_nonVisibleScrollJobs.clear();
    m_nonVisibleScrollJobsCompleted.clear();
    m_regularRenderJobsRegion = Platform::IntRectRegion();
    m_currentRegularRenderJobsBatch.clear();
    m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion();
    m_regularRenderJobsNotRenderedRegion = Platform::IntRectRegion();
    ASSERT(isEmpty());
}

bool RenderQueue::isEmpty(bool shouldPerformRegularRenderJobs) const
{
    return m_visibleZoomJobs.isEmpty() && m_visibleScrollJobs.isEmpty()
        && (!shouldPerformRegularRenderJobs || m_currentRegularRenderJobsBatch.isEmpty())
        && (!shouldPerformRegularRenderJobs || m_regularRenderJobsRegion.isEmpty())
        && m_nonVisibleScrollJobs.isEmpty();
}

bool RenderQueue::hasCurrentRegularRenderJob() const
{
    return !m_currentRegularRenderJobsBatch.isEmpty() || !m_regularRenderJobsRegion.isEmpty();
}

bool RenderQueue::hasCurrentVisibleZoomJob() const
{
    return !m_visibleZoomJobs.isEmpty();
}

bool RenderQueue::hasCurrentVisibleScrollJob() const
{
    return !m_visibleScrollJobs.isEmpty();
}

bool RenderQueue::isCurrentVisibleZoomJob(const TileIndex& index) const
{
    return m_visibleZoomJobs.contains(index);
}

bool RenderQueue::isCurrentVisibleZoomJobCompleted(const TileIndex& index) const
{
    return m_visibleZoomJobsCompleted.contains(index);
}

bool RenderQueue::isCurrentVisibleScrollJob(const TileIndex& index) const
{
    return m_visibleScrollJobs.contains(index);
}

bool RenderQueue::isCurrentVisibleScrollJobCompleted(const TileIndex& index) const
{
    return m_visibleScrollJobsCompleted.contains(index);
}

bool RenderQueue::isCurrentRegularRenderJob(const TileIndex& index, BackingStoreGeometry* geometry) const
{
    Platform::IntRect tileRect(geometry->originOfTile(index), m_parent->tileSize());
    Platform::IntRectRegion::IntersectionState regularJobsState = m_regularRenderJobsRegion.isRectInRegion(tileRect);
    Platform::IntRectRegion::IntersectionState currentRegularJobsState = m_currentRegularRenderJobsBatchRegion.isRectInRegion(tileRect);

    return regularJobsState == Platform::IntRectRegion::ContainedInRegion
        || regularJobsState == Platform::IntRectRegion::PartiallyContainedInRegion
        || currentRegularJobsState == Platform::IntRectRegion::ContainedInRegion
        || currentRegularJobsState == Platform::IntRectRegion::PartiallyContainedInRegion;
}

bool RenderQueue::currentRegularRenderJobBatchUnderPressure() const
{
    return m_currentRegularRenderJobsBatchUnderPressure;
}

void RenderQueue::setCurrentRegularRenderJobBatchUnderPressure(bool currentRegularRenderJobsBatchUnderPressure)
{
    m_currentRegularRenderJobsBatchUnderPressure = currentRegularRenderJobsBatchUnderPressure;
}

void RenderQueue::eventQueueCycled()
{
    // Called by the backing store when the event queue has cycled to allow the
    // render queue to determine if the regular render jobs are under pressure.
    if (m_rectsAddedToRegularRenderJobsInCurrentCycle && m_currentRegularRenderJobsBatchRegion.isEmpty())
        m_currentRegularRenderJobsBatchUnderPressure = true;
    m_rectsAddedToRegularRenderJobsInCurrentCycle = false;
}

TileIndexList RenderQueue::tileIndexesIntersectingRegion(const Platform::IntRectRegion& region, BackingStoreGeometry* geometry) const
{
    TileIndexList indexes;
    TileIndexList allIndexes = m_parent->indexesForBackingStoreRect(geometry->backingStoreRect());

    for (size_t i = 0; i < allIndexes.size(); ++i) {
        Platform::IntRect tileRect(geometry->originOfTile(allIndexes[i]), m_parent->tileSize());
        Platform::IntRectRegion::IntersectionState state = region.isRectInRegion(tileRect);
        if (state == Platform::IntRectRegion::ContainedInRegion || state == Platform::IntRectRegion::PartiallyContainedInRegion)
            indexes.append(allIndexes[i]);
    }
    return indexes;
}

TileIndexList RenderQueue::tileIndexesFullyContainedInRegion(const Platform::IntRectRegion& region, BackingStoreGeometry* geometry) const
{
    TileIndexList indexes;
    TileIndexList allIndexes = m_parent->indexesForBackingStoreRect(geometry->backingStoreRect());

    for (size_t i = 0; i < allIndexes.size(); ++i) {
        Platform::IntRect tileRect(geometry->originOfTile(allIndexes[i]), m_parent->tileSize());
        Platform::IntRectRegion::IntersectionState state = region.isRectInRegion(tileRect);
        if (state == Platform::IntRectRegion::ContainedInRegion)
            indexes.append(allIndexes[i]);
    }
    return indexes;
}

Platform::IntRectRegion RenderQueue::tileRegion(const TileIndexList& indexes, BackingStoreGeometry* geometry) const
{
    Platform::IntRectRegion region;

    // Chances are the tiles comprise a perfect rectangle. If we add rectangles
    // row by row or column by column, IntRectRegion should have an easier time
    // merging them as the individual rows or columns will merge into another
    // plain rectangle.
    Platform::IntRectRegion currentRowOrColumnRegion;
    size_t lastI = 0;
    size_t lastJ = 0;

    for (size_t i = 0; i < indexes.size(); ++i) {
        Platform::IntRect tileRect(geometry->originOfTile(indexes[i]), m_parent->tileSize());
        bool extendsCurrentRowOrColumn = i && (lastI == indexes[i].i() || lastJ == indexes[i].j());

        if (extendsCurrentRowOrColumn)
            currentRowOrColumnRegion = Platform::IntRectRegion::unionRegions(currentRowOrColumnRegion, tileRect);
        else {
            region = Platform::IntRectRegion::unionRegions(region, currentRowOrColumnRegion);
            currentRowOrColumnRegion = tileRect;
        }
        lastI = indexes[i].i();
        lastJ = indexes[i].j();
    }

    return Platform::IntRectRegion::unionRegions(region, currentRowOrColumnRegion);
}

void RenderQueue::addToQueue(JobType type, const Platform::IntRectRegion& region)
{
    if (type == RegularRender) {
        // Flag that we added rects in the current event queue cycle.
        m_rectsAddedToRegularRenderJobsInCurrentCycle = true;

        // We try and detect if this newly added region intersects or is contained in the currently running
        // batch of render jobs. If so, then we have to start the batch over since we decompose individual
        // rects into subrects and might have already rendered one of them. If the web page's content has
        // changed state then this can lead to artifacts. We mark this by noting the batch is now under pressure
        // and the backingstore will attempt to clear it at the next available opportunity.
        std::vector<Platform::IntRect> rectsInRegion = region.rects();
        for (size_t i = 0; i < rectsInRegion.size(); ++i) {
            Platform::IntRectRegion::IntersectionState state = m_currentRegularRenderJobsBatchRegion.isRectInRegion(rectsInRegion[i]);
            if (state == Platform::IntRectRegion::ContainedInRegion || state == Platform::IntRectRegion::PartiallyContainedInRegion) {
                m_regularRenderJobsRegion = Platform::IntRectRegion::unionRegions(m_regularRenderJobsRegion, m_currentRegularRenderJobsBatchRegion);
                m_currentRegularRenderJobsBatch.clear();
                m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion();
                m_currentRegularRenderJobsBatchUnderPressure = true;
            }
        }
        addToRegularQueue(region);
        return;
    }

    TileIndexList indexes = tileIndexesIntersectingRegion(region, m_parent->frontState());

    switch (type) {
    case VisibleZoom:
        addToScrollZoomQueue(indexes, &m_visibleZoomJobs);
        return;
    case VisibleScroll:
        addToScrollZoomQueue(indexes, &m_visibleScrollJobs);
        return;
    case NonVisibleScroll:
        addToScrollZoomQueue(indexes, &m_nonVisibleScrollJobs);
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}

void RenderQueue::addToRegularQueue(const Platform::IntRectRegion& region)
{
#if DEBUG_RENDER_QUEUE
    std::vector<Platform::IntRect> rectsInRegion = region.rects();
    for (size_t i = 0; i < rectsInRegion.size(); ++i) {
        if (m_regularRenderJobsRegion.isRectInRegion(rectsInRegion[i]) != Platform::IntRectRegion::ContainedInRegion) {
            Platform::logAlways(Platform::LogLevelCritical,
                "RenderQueue::addToRegularQueue region within %s",
                region.extents().toString().c_str());
            break;
        }
    }
#endif

    m_regularRenderJobsRegion = Platform::IntRectRegion::unionRegions(m_regularRenderJobsRegion, region);

    // Do not let the regular render queue grow past a maximum of 3 disjoint rects.
    if (m_regularRenderJobsRegion.numRects() > 2)
        m_regularRenderJobsRegion = m_regularRenderJobsRegion.extents();

    if (!isEmpty())
        m_parent->dispatchRenderJob();
}

void RenderQueue::addToScrollZoomQueue(const TileIndexList& addedTiles, TileIndexList* alreadyQueuedTiles)
{
    Platform::IntRect contentsRect = m_parent->expandedContentsRect();
    for (size_t i = 0; i < addedTiles.size(); ++i) {
        if (alreadyQueuedTiles->contains(addedTiles[i]))
            continue;

        Platform::IntRect tileRect(m_parent->frontState()->originOfTile(addedTiles[i]), m_parent->tileSize());
        if (!contentsRect.intersects(tileRect)) {
#if DEBUG_RENDER_QUEUE
                Platform::logAlways(Platform::LogLevelCritical,
                    "RenderQueue::addToScrollZoomQueue tile at %s outside of expanded contents rect %s, ignoring.",
                    tileRect.toString().c_str(),
                    contentsRect.toString().c_str());
#endif
            continue;
        }

#if DEBUG_RENDER_QUEUE
        Platform::logAlways(Platform::LogLevelCritical,
            "RenderQueue::addToScrollZoomQueue tile at %s",
            m_parent->frontState()->originOfTile(addedTiles[i]).toString().c_str());
#endif
        alreadyQueuedTiles->append(addedTiles[i]);
    }

    if (!isEmpty())
        m_parent->dispatchRenderJob();
}

void RenderQueue::quickSort(TileIndexList* queue)
{
    size_t length = queue->size();
    if (!length)
        return;

    WTF::nonCopyingSort(queue->begin(), queue->end(), TileIndexLessThan(m_primarySortDirection, m_secondarySortDirection));
}

void RenderQueue::updateSortDirection(int lastDeltaX, int lastDeltaY)
{
    bool primaryIsHorizontal = abs(lastDeltaX) >= abs(lastDeltaY);
    if (primaryIsHorizontal) {
        m_primarySortDirection = lastDeltaX <= 0 ? LeftToRight : RightToLeft;
        m_secondarySortDirection = lastDeltaY <= 0 ? TopToBottom : BottomToTop;
    } else {
        m_primarySortDirection = lastDeltaY <= 0 ? TopToBottom : BottomToTop;
        m_secondarySortDirection = lastDeltaX <= 0 ? LeftToRight : RightToLeft;
    }
}

void RenderQueue::visibleContentChanged(const Platform::IntRect& visibleContent)
{
    if (m_visibleScrollJobs.isEmpty() && m_nonVisibleScrollJobs.isEmpty()) {
        ASSERT(m_visibleScrollJobsCompleted.isEmpty() && m_nonVisibleScrollJobsCompleted.isEmpty());
        return;
    }

    TileIndexList visibleTiles = tileIndexesIntersectingRegion(visibleContent, m_parent->frontState());

    // Move visibleScrollJobs to nonVisibleScrollJobs if they do not intersect
    // the visible content rect.
    for (size_t i = 0; i < m_visibleScrollJobs.size(); ++i) {
        if (!visibleTiles.contains(m_visibleScrollJobs[i])) {
            ASSERT(!m_nonVisibleScrollJobs.contains(m_visibleScrollJobs[i]));
            m_nonVisibleScrollJobs.append(m_visibleScrollJobs[i]);
            m_visibleScrollJobs.remove(i);
            --i;
        }
    }

    // Do the same for the completed list.
    for (size_t i = 0; i < m_visibleScrollJobsCompleted.size(); ++i) {
        if (!visibleTiles.contains(m_visibleScrollJobsCompleted[i])) {
            ASSERT(!m_nonVisibleScrollJobsCompleted.contains(m_visibleScrollJobsCompleted[i]));
            m_nonVisibleScrollJobsCompleted.append(m_visibleScrollJobsCompleted[i]);
            m_visibleScrollJobsCompleted.remove(i);
            --i;
        }
    }

    // Move nonVisibleScrollJobs to visibleScrollJobs if they do intersect
    // the visible content rect.
    for (size_t i = 0; i < m_nonVisibleScrollJobs.size(); ++i) {
        if (visibleTiles.contains(m_nonVisibleScrollJobs[i])) {
            ASSERT(!m_visibleScrollJobs.contains(m_nonVisibleScrollJobs[i]));
            m_visibleScrollJobs.append(m_nonVisibleScrollJobs[i]);
            m_nonVisibleScrollJobs.remove(i);
            --i;
        }
    }

    // Do the same for the completed list.
    for (size_t i = 0; i < m_nonVisibleScrollJobsCompleted.size(); ++i) {
        if (visibleTiles.contains(m_nonVisibleScrollJobsCompleted[i])) {
            ASSERT(!m_visibleScrollJobsCompleted.contains(m_nonVisibleScrollJobsCompleted[i]));
            m_visibleScrollJobsCompleted.append(m_nonVisibleScrollJobsCompleted[i]);
            m_nonVisibleScrollJobsCompleted.remove(i);
            --i;
        }
    }

    if (m_visibleScrollJobs.isEmpty() && !m_visibleScrollJobsCompleted.isEmpty())
        scrollZoomJobsCompleted(m_visibleScrollJobs, &m_visibleScrollJobsCompleted, false /*shouldBlit*/);

    if (m_nonVisibleScrollJobs.isEmpty() && !m_nonVisibleScrollJobsCompleted.isEmpty())
        scrollZoomJobsCompleted(m_nonVisibleScrollJobs, &m_nonVisibleScrollJobsCompleted, false /*shouldBlit*/);

    // We shouldn't be empty because the early return above and the fact that this
    // method just shuffles rects from queue to queue hence the total number of
    // rects in the various queues should be conserved.
    ASSERT(!isEmpty());
}

void RenderQueue::backingStoreRectChanging(const Platform::IntRect&, const Platform::IntRect&)
{
    // We could empty them here instead of in BackingStorePrivate::setBackingStoreRect(),
    // but it seems cleaner to do it there and still avoid code to manually move them.
    ASSERT(m_visibleZoomJobs.isEmpty());
    ASSERT(m_visibleZoomJobsCompleted.isEmpty());
    ASSERT(m_visibleScrollJobs.isEmpty());
    ASSERT(m_visibleScrollJobsCompleted.isEmpty());
    ASSERT(m_nonVisibleScrollJobs.isEmpty());
    ASSERT(m_nonVisibleScrollJobsCompleted.isEmpty());

    // Empty the tile-based current batch and merge its remaining region with
    // the one from other upcoming regular render jobs. We'll pick all of them
    // up next time renderRegularRenderJobs() is called.
    m_regularRenderJobsRegion = Platform::IntRectRegion::unionRegions(m_regularRenderJobsRegion, m_currentRegularRenderJobsBatchRegion);
    m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion();
    m_currentRegularRenderJobsBatch.clear();
}

void RenderQueue::clear(const Platform::IntRectRegion& region, ClearJobsFlags clearJobsFlags)
{
    clearRegions(region, clearJobsFlags);
    clearTileIndexes(tileIndexesFullyContainedInRegion(region, m_parent->frontState()), clearJobsFlags);
}

void RenderQueue::clear(const TileIndexList& indexes, BackingStoreGeometry* geometry, ClearJobsFlags clearJobsFlags)
{
    clearRegions(tileRegion(indexes, geometry), clearJobsFlags);
    clearTileIndexes(indexes, clearJobsFlags);
}

void RenderQueue::clearRegions(const Platform::IntRectRegion& region, ClearJobsFlags clearJobsFlags)
{
    if (clearJobsFlags & ClearRegularRenderJobs) {
        m_regularRenderJobsRegion = Platform::IntRectRegion::subtractRegions(m_regularRenderJobsRegion, region);
        m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion::subtractRegions(m_currentRegularRenderJobsBatchRegion, region);
        m_regularRenderJobsNotRenderedRegion = Platform::IntRectRegion::subtractRegions(m_regularRenderJobsNotRenderedRegion, region);
    }
}

void RenderQueue::clearTileIndexes(const TileIndexList& indexes, ClearJobsFlags clearJobsFlags)
{
    if (m_visibleScrollJobs.isEmpty() && m_nonVisibleScrollJobs.isEmpty())
        ASSERT(m_visibleScrollJobsCompleted.isEmpty() && m_nonVisibleScrollJobsCompleted.isEmpty());

    // Remove all indexes from all queues that are being marked as cleared.
    if (clearJobsFlags & ClearIncompleteZoomJobs) {
        for (size_t i = 0; i < m_visibleZoomJobs.size(); ++i) {
            if (indexes.contains(m_visibleZoomJobs.at(i))) {
                m_visibleZoomJobs.remove(i);
                --i;
            }
        }
    }

    if (clearJobsFlags & ClearIncompleteScrollJobs) {
        for (size_t i = 0; i < m_visibleScrollJobs.size(); ++i) {
            if (indexes.contains(m_visibleScrollJobs.at(i))) {
                m_visibleScrollJobs.remove(i);
                --i;
            }
        }

        for (size_t i = 0; i < m_nonVisibleScrollJobs.size(); ++i) {
            if (indexes.contains(m_nonVisibleScrollJobs.at(i))) {
                m_nonVisibleScrollJobs.remove(i);
                --i;
            }
        }
    }

    if (clearJobsFlags & ClearCompletedJobs) {
        for (size_t i = 0; i < m_visibleZoomJobsCompleted.size(); ++i) {
            if (indexes.contains(m_visibleZoomJobsCompleted.at(i))) {
                m_visibleZoomJobsCompleted.remove(i);
                --i;
            }
        }

        for (size_t i = 0; i < m_visibleScrollJobsCompleted.size(); ++i) {
            if (indexes.contains(m_visibleScrollJobsCompleted.at(i))) {
                m_visibleScrollJobsCompleted.remove(i);
                --i;
            }
        }

        for (size_t i = 0; i < m_nonVisibleScrollJobsCompleted.size(); ++i) {
            if (indexes.contains(m_nonVisibleScrollJobsCompleted.at(i))) {
                m_nonVisibleScrollJobsCompleted.remove(i);
                --i;
            }
        }
    }

    if (clearJobsFlags & ClearRegularRenderJobs) {
        for (size_t i = 0; i < m_currentRegularRenderJobsBatch.size(); ++i) {
            if (indexes.contains(m_currentRegularRenderJobsBatch.at(i))) {
                m_currentRegularRenderJobsBatch.remove(i);
                --i;
            }
        }
    }

    if (m_visibleZoomJobs.isEmpty() && !m_visibleZoomJobsCompleted.isEmpty())
        scrollZoomJobsCompleted(m_visibleZoomJobs, &m_visibleZoomJobsCompleted, false /*shouldBlit*/);

    if (m_visibleScrollJobs.isEmpty() && !m_visibleScrollJobsCompleted.isEmpty())
        scrollZoomJobsCompleted(m_visibleScrollJobs, &m_visibleScrollJobsCompleted, false /*shouldBlit*/);

    if (m_nonVisibleScrollJobs.isEmpty() && !m_nonVisibleScrollJobsCompleted.isEmpty())
        scrollZoomJobsCompleted(m_nonVisibleScrollJobs, &m_nonVisibleScrollJobsCompleted, false /*shouldBlit*/);
}

bool RenderQueue::regularRenderJobsPreviouslyAttemptedButNotRendered(const Platform::IntRect& rect)
{
    return m_regularRenderJobsNotRenderedRegion.isRectInRegion(rect) != Platform::IntRectRegion::NotInRegion;
}

void RenderQueue::render(bool shouldPerformRegularRenderJobs)
{
    // We request a layout here to ensure that we're executing jobs in the correct
    // order. If we didn't request a layout here then the jobs below could result
    // in a layout and that layout can alter this queue. So request layout if needed
    // to ensure that the queues below are in constant state before performing the
    // next rendering job.

#if DEBUG_RENDER_QUEUE
    // Start the time measurement.
    double time = WTF::currentTime();
#endif

    m_parent->requestLayoutIfNeeded();
    m_parent->updateTileMatrixIfNeeded();

#if DEBUG_RENDER_QUEUE
    double elapsed = WTF::currentTime() - time;
    if (elapsed)
        Platform::logAlways(Platform::LogLevelCritical, "RenderQueue::render layout elapsed=%f", elapsed);
#endif

    // Empty the queues in a precise order of priority.
    if (!m_visibleZoomJobs.isEmpty())
        renderScrollZoomJobs(&m_visibleZoomJobs, &m_visibleZoomJobsCompleted, true /*allAtOnceIfPossible*/, true /*shouldBlitWhenCompleted*/);
    else if (!m_visibleScrollJobs.isEmpty())
        renderScrollZoomJobs(&m_visibleScrollJobs, &m_visibleScrollJobsCompleted, false /*allAtOnceIfPossible*/, true /*shouldBlitWhenCompleted*/);
    else if (shouldPerformRegularRenderJobs && (!m_currentRegularRenderJobsBatch.isEmpty() || !m_regularRenderJobsRegion.isEmpty())) {
        bool allAtOnceIfPossible = currentRegularRenderJobBatchUnderPressure();
        renderRegularRenderJobs(allAtOnceIfPossible);
    } else if (!m_nonVisibleScrollJobs.isEmpty())
        renderScrollZoomJobs(&m_nonVisibleScrollJobs, &m_nonVisibleScrollJobsCompleted, false /*allAtOnceIfPossible*/, false /*shouldBlitWhenCompleted*/);
}

void RenderQueue::renderRegularRenderJobs(bool allAtOnceIfPossible)
{
#if DEBUG_RENDER_QUEUE
    // Start the time measurement.
    double time = WTF::currentTime();
#endif

    ASSERT(!m_currentRegularRenderJobsBatch.isEmpty() || !m_regularRenderJobsRegion.isEmpty());

    if (allAtOnceIfPossible && !m_currentRegularRenderJobsBatchRegion.isEmpty()) {
        // If there is a current batch of jobs already, merge it back into the
        // whole area so that we will pick up all of it for the next run.
        m_regularRenderJobsRegion = Platform::IntRectRegion::unionRegions(m_regularRenderJobsRegion, m_currentRegularRenderJobsBatchRegion);

        // Clear this since we're about to render everything.
        m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion();
        m_currentRegularRenderJobsBatch.clear();
    }

    Platform::IntRect contentsRect = m_parent->expandedContentsRect();

    // Start new batch if needed.
    if (m_currentRegularRenderJobsBatchRegion.isEmpty()) {
        // Don't try to render anything outside the whole contents rectangle.
        m_regularRenderJobsRegion = Platform::IntRectRegion::intersectRegions(m_regularRenderJobsRegion, contentsRect);

        // Split the current regular render job region into tiles.
        // Discard regions outside of the region covered by these tiles.
        // They'll be rendered when the geometry changes.
        m_regularRenderJobsRegion = Platform::IntRectRegion::intersectRegions(m_regularRenderJobsRegion, m_parent->frontState()->backingStoreRect());
        m_currentRegularRenderJobsBatch = tileIndexesIntersectingRegion(m_regularRenderJobsRegion, m_parent->frontState());

        // Create a region object that will be checked when adding new rects before
        // this batch has been completed.
        m_currentRegularRenderJobsBatchRegion = m_regularRenderJobsRegion;

        // Clear the former region since it is now part of this batch.
        m_regularRenderJobsRegion = Platform::IntRectRegion();

#if DEBUG_RENDER_QUEUE
        Platform::logAlways(Platform::LogLevelCritical,
            "RenderQueue::renderRegularRenderJobs batch size is %d!",
            m_currentRegularRenderJobsBatch.size());
#endif
    }

    Platform::IntRectRegion changingRegion = m_currentRegularRenderJobsBatchRegion;
    Platform::IntRectRegion changingRegionScheduledForLater = m_regularRenderJobsRegion;
    Platform::IntRectRegion regionNotRenderedThisTime;

    TileIndexList* outstandingJobs = &m_currentRegularRenderJobsBatch;
    TileIndexList completedJobs;
    TileIndexList tilesToRender;

    unsigned numberOfAvailableBackBuffers = SurfacePool::globalSurfacePool()->numberOfAvailableBackBuffers();

    while (!outstandingJobs->isEmpty() && numberOfAvailableBackBuffers) {
        // Render as many tiles at once as we can handle.
        for (size_t i = 0; i < outstandingJobs->size() && numberOfAvailableBackBuffers; ++i) {
            TileIndex index = (*outstandingJobs)[i];

            BackingStoreGeometry* geometry = m_parent->frontState();
            Platform::IntRect tileRect(geometry->originOfTile(index), m_parent->tileSize());

            if (!contentsRect.intersects(tileRect)) {
                // This is a safety fallback, at this point we should only
                // encounter tiles outside of the contents rect if we're in
                // a second run of the same batch and the contents size has
                // changed since the last run. Otherwise, see the cropping above.
#if DEBUG_RENDER_QUEUE
                Platform::logAlways(Platform::LogLevelCritical,
                    "RenderQueue::renderRegularRenderJobs tile at %s outside of expanded contents rect %s, ignoring.",
                    tileRect.toString().c_str(),
                    contentsRect.toString().c_str());
#endif
                outstandingJobs->remove(i);
                --i;
                m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion::intersectRegions(m_currentRegularRenderJobsBatchRegion, contentsRect);
                continue;
            }

            if (m_parent->shouldSuppressNonVisibleRegularRenderJobs()) {
                // If a tile is not visible, remember it as not rendered to be
                // picked up later by updateTilesForScrollOrNotRenderedRegion()
                // or updateTilesAfterBackingStoreRectChange().
                Platform::IntRectRegion tileRegionNotRendered;

                if (!m_parent->isTileVisible(index, geometry)) {
                    tileRegionNotRendered = Platform::IntRectRegion::intersectRegions(tileRect, m_currentRegularRenderJobsBatchRegion);
                    regionNotRenderedThisTime = Platform::IntRectRegion::unionRegions(regionNotRenderedThisTime, tileRegionNotRendered);
                }

                if (!tileRegionNotRendered.isEmpty()) {
#if DEBUG_RENDER_QUEUE
                    Platform::logAlways(Platform::LogLevelCritical,
                        "RenderQueue::renderRegularRenderJobs region within %s not completely rendered!",
                        tileRegionNotRendered.extents().toString().c_str());
#endif
                    outstandingJobs->remove(i);
                    --i;
                    continue;
                }
            }

            tilesToRender.append(index);
            --numberOfAvailableBackBuffers;
        }

        if (tilesToRender.isEmpty())
            break;

        // This will also clear the rendered tiles in outstandingJobs.
        TileIndexList renderedTiles = m_parent->render(tilesToRender);

        // Update number of available back buffers now that we've used them.
        numberOfAvailableBackBuffers = SurfacePool::globalSurfacePool()->numberOfAvailableBackBuffers();

        ASSERT(!renderedTiles.isEmpty());
        if (renderedTiles.isEmpty()) {
#if DEBUG_RENDER_QUEUE
            Platform::logAlways(Platform::LogLevelCritical,
                "RenderQueue::renderRegularRenderJobs no tiles rendered (%d attempted), available back buffers: %d",
                tilesToRender.size(),
                numberOfAvailableBackBuffers);
#endif
            break; // Something bad happened, make sure not to try again for now.
        }

        for (size_t i = 0; i < renderedTiles.size(); ++i) {
            if (!completedJobs.contains(renderedTiles[i]))
                completedJobs.append(renderedTiles[i]);
        }

        if (!allAtOnceIfPossible)
            break; // We can do the rest next time.
    }

    const Platform::IntRectRegion renderedRegion = tileRegion(completedJobs, m_parent->frontState());

    m_regularRenderJobsNotRenderedRegion = Platform::IntRectRegion::unionRegions(m_regularRenderJobsNotRenderedRegion, regionNotRenderedThisTime);
    m_currentRegularRenderJobsBatchRegion = Platform::IntRectRegion::subtractRegions(m_currentRegularRenderJobsBatchRegion, regionNotRenderedThisTime);

#if DEBUG_RENDER_QUEUE
    // Stop the time measurement.
    double elapsed = WTF::currentTime() - time;
    Platform::logAlways(Platform::LogLevelCritical,
        "RenderQueue::renderRegularRenderJobs within %s: completed = %d, outstanding = %d, elapsed=%f",
        changingRegion.extents().toString().c_str(),
        completedJobs.size(),
        outstandingJobs->size(),
        elapsed);
#endif

    // Make sure we didn't alter state of the queues that should have been empty
    // before this method was called.
    ASSERT(m_visibleZoomJobs.isEmpty());
    ASSERT(m_visibleScrollJobs.isEmpty());

    if (m_currentRegularRenderJobsBatchRegion.isEmpty()) {
        // If the whole scheduled area has been rendered, all outstanding jobs
        // should have been cleared as well.
        ASSERT(outstandingJobs->isEmpty());
        m_currentRegularRenderJobsBatchUnderPressure = false;

        // Notify about the newly rendered content.
        // In case we picked up an unfinished batch from before, because we
        // paint a larger area it's possible that we also rendered other
        // regions that were originally scheduled for later.
        Platform::IntRectRegion wholeScheduledRegion = Platform::IntRectRegion::unionRegions(changingRegion, changingRegionScheduledForLater);
        Platform::IntRectRegion renderedChangedRegion = Platform::IntRectRegion::intersectRegions(wholeScheduledRegion, renderedRegion);

        // Blit since this batch is now complete.
        m_parent->didRenderContent(renderedChangedRegion);
    }

    if (m_parent->shouldSuppressNonVisibleRegularRenderJobs() && !regionNotRenderedThisTime.isEmpty())
        m_parent->updateTilesForScrollOrNotRenderedRegion(false /*checkLoading*/);
}

void RenderQueue::renderScrollZoomJobs(TileIndexList* outstandingJobs, TileIndexList* completedJobs, bool allAtOnceIfPossible, bool shouldBlitWhenCompleted)
{
    ASSERT(!outstandingJobs->isEmpty());

#if DEBUG_RENDER_QUEUE || DEBUG_RENDER_QUEUE_SORT
    // Start the time measurement.
    double time = WTF::currentTime();
    double elapsed;
#endif

    Platform::IntRect contentsRect = m_parent->expandedContentsRect();

    unsigned numberOfAvailableBackBuffers = SurfacePool::globalSurfacePool()->numberOfAvailableBackBuffers();

    // If we take multiple turns to render, we sort to make them appear in the right order.
    if (!allAtOnceIfPossible && outstandingJobs->size() > numberOfAvailableBackBuffers)
        quickSort(outstandingJobs);

#if DEBUG_RENDER_QUEUE_SORT
    // Stop the time measurement
    elapsed = WTF::currentTime() - time;
    Platform::logAlways(Platform::LogLevelCritical, "RenderQueue::renderScrollZoomJobs sort elapsed=%f", elapsed);
#endif

    TileIndexList tilesToRender;

    while (!outstandingJobs->isEmpty() && numberOfAvailableBackBuffers) {
        // Render as many tiles at once as we can handle.
        for (size_t i = 0; i < outstandingJobs->size() && numberOfAvailableBackBuffers; ++i) {
            TileIndex index = (*outstandingJobs)[i];
            Platform::IntRect tileRect(m_parent->frontState()->originOfTile(index), m_parent->tileSize());

            // Make sure we only try to render tiles containing contents.
            if (!contentsRect.intersects(tileRect)) {
#if DEBUG_RENDER_QUEUE
                Platform::logAlways(Platform::LogLevelCritical,
                    "RenderQueue::renderScrollZoomJobs tile at %s outside of expanded contents rect %s, ignoring.",
                    tileRect.toString().c_str(),
                    contentsRect.toString().c_str());
#endif
                outstandingJobs->remove(i);
                --i;
                continue;
            }

            tilesToRender.append((*outstandingJobs)[i]);
            --numberOfAvailableBackBuffers;
        }

        if (tilesToRender.isEmpty())
            break;

        // This will clear the rendered tiles in outstandingJobs.
        TileIndexList renderedTiles = m_parent->render(tilesToRender);

        // Update number of available back buffers now that we've used them.
        numberOfAvailableBackBuffers = SurfacePool::globalSurfacePool()->numberOfAvailableBackBuffers();

        ASSERT(!renderedTiles.isEmpty());
        if (renderedTiles.isEmpty()) {
#if DEBUG_RENDER_QUEUE
            Platform::logAlways(Platform::LogLevelCritical,
                "RenderQueue::renderScrollZoomJobs no tiles rendered (%d attempted, available back buffers: %d)",
                tilesToRender.size(),
                numberOfAvailableBackBuffers);
#endif
            break; // Something bad happened, make sure not to try again for now.
        }

        for (size_t i = 0; i < renderedTiles.size(); ++i) {
            if (!completedJobs->contains(renderedTiles[i]))
                completedJobs->append(renderedTiles[i]);
        }

        if (!allAtOnceIfPossible)
            break; // We can do the rest next time.
    }

#if DEBUG_RENDER_QUEUE
    // Stop the time measurement.
    elapsed = WTF::currentTime() - time;
    Platform::logAlways(Platform::LogLevelCritical,
        "RenderQueue::renderScrollZoomJobs completed=%d, outstanding=%d, elapsed=%f",
        completedJobs->size(),
        outstandingJobs->size(),
        elapsed);
#endif

    if (outstandingJobs->isEmpty())
        scrollZoomJobsCompleted(*outstandingJobs, completedJobs, shouldBlitWhenCompleted);
}

void RenderQueue::scrollZoomJobsCompleted(const TileIndexList& outstandingJobs, TileIndexList* completedJobs, bool shouldBlit)
{
    // Get rid of the completed list!
    ASSERT(outstandingJobs.isEmpty());
    completedJobs->clear();

    // Now blit to the screen if we are done!
    if (shouldBlit)
        m_parent->didRenderContent(m_parent->visibleContentsRect());
}

} // namespace WebKit
} // namespace BlackBerry
