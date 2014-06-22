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

#ifndef RenderQueue_h
#define RenderQueue_h

#include "TileIndex.h"

#include <BlackBerryPlatformIntRectRegion.h>
#include <BlackBerryPlatformPrimitives.h>

namespace BlackBerry {
namespace WebKit {

class BackingStorePrivate;
class BackingStoreGeometry;

enum SortDirection {
    LeftToRight = 0,
    RightToLeft,
    TopToBottom,
    BottomToTop,
    NumSortDirections
};

class RenderQueue {
public:
    enum JobType { VisibleZoom, VisibleScroll, RegularRender, NonVisibleScroll };

    enum ClearJobsFlags {
        ClearRegularRenderJobs = 1 << 0,
        ClearIncompleteScrollJobs = 1 << 1,
        ClearIncompleteZoomJobs = 1 << 2,
        ClearCompletedJobs = 1 << 3,

        ClearAnyJobs = 0xFFFFFFFF,

        DontClearRegularRenderJobs = ClearAnyJobs & ~ClearRegularRenderJobs,
        DontClearCompletedJobs = ClearAnyJobs & ~ClearCompletedJobs,
    };

    RenderQueue(BackingStorePrivate*);

    void reset();

    bool isEmpty(bool shouldPerformRegularRenderJobs = true) const;

    bool hasCurrentRegularRenderJob() const;
    bool hasCurrentVisibleZoomJob() const;
    bool hasCurrentVisibleScrollJob() const;
    bool isCurrentVisibleZoomJob(const TileIndex&) const;
    bool isCurrentVisibleZoomJobCompleted(const TileIndex&) const;
    bool isCurrentVisibleScrollJob(const TileIndex&) const;
    bool isCurrentVisibleScrollJobCompleted(const TileIndex&) const;
    bool isCurrentRegularRenderJob(const TileIndex&, BackingStoreGeometry*) const;

    bool currentRegularRenderJobBatchUnderPressure() const;
    void setCurrentRegularRenderJobBatchUnderPressure(bool);

    void eventQueueCycled();

    void addToQueue(JobType, const Platform::IntRectRegion&);

    void updateSortDirection(int lastDeltaX, int lastDeltaY);
    void visibleContentChanged(const Platform::IntRect&);
    void backingStoreRectChanging(const Platform::IntRect& oldRect, const Platform::IntRect& newRect);
    void clear(const TileIndexList&, BackingStoreGeometry*, ClearJobsFlags);
    void clear(const Platform::IntRectRegion&, ClearJobsFlags);
    bool regularRenderJobsPreviouslyAttemptedButNotRendered(const Platform::IntRect&);
    Platform::IntRectRegion regularRenderJobsNotRenderedRegion() const { return m_regularRenderJobsNotRenderedRegion; }

    void render(bool shouldPerformRegularRenderJobs = true);

private:
    TileIndexList tileIndexesIntersectingRegion(const Platform::IntRectRegion&, BackingStoreGeometry*) const;
    TileIndexList tileIndexesFullyContainedInRegion(const Platform::IntRectRegion&, BackingStoreGeometry*) const;
    Platform::IntRectRegion tileRegion(const TileIndexList&, BackingStoreGeometry*) const;

    void clearRegions(const Platform::IntRectRegion&, ClearJobsFlags);
    void clearTileIndexes(const TileIndexList&, ClearJobsFlags);

    // Render items from the queue.
    void renderRegularRenderJobs(bool allAtOnceIfPossible);
    void renderScrollZoomJobs(TileIndexList* outstandingJobs, TileIndexList* completedJobs, bool allAtOnceIfPossible, bool shouldBlitWhenCompleted);
    void scrollZoomJobsCompleted(const TileIndexList& outstandingJobs, TileIndexList* completedJobs, bool shouldBlit);

    // Internal method to add to the various queues.
    void addToRegularQueue(const Platform::IntRectRegion&);
    void addToScrollZoomQueue(const TileIndexList&, TileIndexList* queue);
    void quickSort(TileIndexList*);

    BackingStorePrivate* m_parent;

    // The highest priority queue.
    TileIndexList m_visibleZoomJobs;
    TileIndexList m_visibleZoomJobsCompleted;
    TileIndexList m_visibleScrollJobs;
    TileIndexList m_visibleScrollJobsCompleted;
    // The lowest priority queue.
    TileIndexList m_nonVisibleScrollJobs;
    TileIndexList m_nonVisibleScrollJobsCompleted;
    // The regular render jobs are in the middle.
    Platform::IntRectRegion m_regularRenderJobsRegion;
    TileIndexList m_currentRegularRenderJobsBatch;
    Platform::IntRectRegion m_currentRegularRenderJobsBatchRegion;
    bool m_rectsAddedToRegularRenderJobsInCurrentCycle;
    bool m_currentRegularRenderJobsBatchUnderPressure;

    // Holds the region of the page that we attempt to render, but the
    // backingstore was not in the right place at the time. This will
    // be checked before we try to restore a tile to it's last rendered
    // place.
    Platform::IntRectRegion m_regularRenderJobsNotRenderedRegion;

    SortDirection m_primarySortDirection;
    SortDirection m_secondarySortDirection;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // RenderQueue_h
