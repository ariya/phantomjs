/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderGrid_h
#define RenderGrid_h

#include "RenderBlock.h"

namespace WebCore {

class GridTrack;

class RenderGrid : public RenderBlock {
public:
    RenderGrid(Element*);
    virtual ~RenderGrid();

    virtual const char* renderName() const OVERRIDE;

    virtual void layoutBlock(bool relayoutChildren, LayoutUnit pageLogicalHeight = 0) OVERRIDE;

    virtual bool avoidsFloats() const OVERRIDE { return true; }
    virtual bool canCollapseAnonymousBlockChild() const OVERRIDE { return false; }

private:
    virtual bool isRenderGrid() const OVERRIDE { return true; }
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;
    virtual void computePreferredLogicalWidths() OVERRIDE;

    LayoutUnit computePreferredTrackWidth(const Length&, size_t) const;

    struct GridSpan {
        GridSpan(size_t initialPosition, size_t finalPosition)
            : initialPositionIndex(initialPosition)
            , finalPositionIndex(finalPosition)
        {
            ASSERT(initialPositionIndex <= finalPositionIndex);
        }

        size_t initialPositionIndex;
        size_t finalPositionIndex;
    };

    struct GridCoordinate {
        // HashMap requires a default constuctor.
        GridCoordinate()
            : columns(0, 0)
            , rows(0, 0)
        {
        }

        GridCoordinate(const GridSpan& r, const GridSpan& c)
            : columns(c)
            , rows(r)
        {
        }

        GridSpan columns;
        GridSpan rows;
    };

    class GridIterator;
    enum TrackSizingDirection { ForColumns, ForRows };
    void computedUsedBreadthOfGridTracks(TrackSizingDirection, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks);
    LayoutUnit computeUsedBreadthOfMinLength(TrackSizingDirection, const Length&) const;
    LayoutUnit computeUsedBreadthOfMaxLength(TrackSizingDirection, const Length&) const;
    LayoutUnit computeUsedBreadthOfSpecifiedLength(TrackSizingDirection, const Length&) const;
    void resolveContentBasedTrackSizingFunctions(TrackSizingDirection, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, LayoutUnit& availableLogicalSpace);

    void growGrid(TrackSizingDirection);
    void insertItemIntoGrid(RenderBox*, size_t rowTrack, size_t columnTrack);
    void insertItemIntoGrid(RenderBox*, const GridCoordinate&);
    void placeItemsOnGrid();
    void placeSpecifiedMajorAxisItemsOnGrid(Vector<RenderBox*>);
    void placeAutoMajorAxisItemsOnGrid(Vector<RenderBox*>);
    void placeAutoMajorAxisItemOnGrid(RenderBox*);
    TrackSizingDirection autoPlacementMajorAxisDirection() const;
    TrackSizingDirection autoPlacementMinorAxisDirection() const;

    void layoutGridItems();
    void clearGrid();

    typedef LayoutUnit (RenderGrid::* SizingFunction)(RenderBox*, TrackSizingDirection, Vector<GridTrack>&);
    typedef LayoutUnit (GridTrack::* AccumulatorGetter)() const;
    typedef void (GridTrack::* AccumulatorGrowFunction)(LayoutUnit);
    typedef bool (GridTrackSize::* FilterFunction)() const;
    void resolveContentBasedTrackSizingFunctionsForItems(TrackSizingDirection, Vector<GridTrack>& columnTracks, Vector<GridTrack>& rowTracks, RenderBox*, FilterFunction, SizingFunction, AccumulatorGetter, AccumulatorGrowFunction);
    void distributeSpaceToTracks(Vector<GridTrack*>&, Vector<GridTrack*>* tracksForGrowthAboveMaxBreadth, AccumulatorGetter, AccumulatorGrowFunction, LayoutUnit& availableLogicalSpace);

    const GridTrackSize& gridTrackSize(TrackSizingDirection, size_t) const;
    size_t maximumIndexInDirection(TrackSizingDirection) const;

    LayoutUnit logicalContentHeightForChild(RenderBox*, Vector<GridTrack>&);
    LayoutUnit minContentForChild(RenderBox*, TrackSizingDirection, Vector<GridTrack>& columnTracks);
    LayoutUnit maxContentForChild(RenderBox*, TrackSizingDirection, Vector<GridTrack>& columnTracks);
    LayoutPoint findChildLogicalPosition(RenderBox*, const Vector<GridTrack>& columnTracks, const Vector<GridTrack>& rowTracks);
    GridCoordinate cachedGridCoordinate(const RenderBox*) const;

    GridSpan resolveGridPositionsFromAutoPlacementPosition(const RenderBox*, TrackSizingDirection, size_t) const;
    PassOwnPtr<GridSpan> resolveGridPositionsFromStyle(const RenderBox*, TrackSizingDirection) const;
    enum GridPositionSide {
        StartSide,
        EndSide,
        BeforeSide,
        AfterSide
    };
    size_t resolveGridPositionFromStyle(const GridPosition&, GridPositionSide) const;

    LayoutUnit gridAreaBreadthForChild(const RenderBox* child, TrackSizingDirection, const Vector<GridTrack>&) const;

#ifndef NDEBUG
    bool tracksAreWiderThanMinTrackBreadth(TrackSizingDirection, const Vector<GridTrack>&);
    bool gridWasPopulated() const { return !m_grid.isEmpty() && !m_grid[0].isEmpty(); }
#endif

    size_t gridColumnCount() const
    {
        ASSERT(gridWasPopulated());
        return m_grid[0].size();
    }
    size_t gridRowCount() const
    {
        ASSERT(gridWasPopulated());
        return m_grid.size();
    }

    Vector<Vector<Vector<RenderBox*, 1> > > m_grid;
    HashMap<const RenderBox*, GridCoordinate> m_gridItemCoordinate;
};

} // namespace WebCore

#endif // RenderGrid_h
