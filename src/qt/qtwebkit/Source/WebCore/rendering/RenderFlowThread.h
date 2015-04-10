/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef RenderFlowThread_h
#define RenderFlowThread_h


#include "RenderBlock.h"
#include <wtf/HashCountedSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

struct LayerFragment;
typedef Vector<LayerFragment, 1> LayerFragments;
class RenderFlowThread;
class RenderStyle;
class RenderRegion;

typedef ListHashSet<RenderRegion*> RenderRegionList;

// RenderFlowThread is used to collect all the render objects that participate in a
// flow thread. It will also help in doing the layout. However, it will not render
// directly to screen. Instead, RenderRegion objects will redirect their paint 
// and nodeAtPoint methods to this object. Each RenderRegion will actually be a viewPort
// of the RenderFlowThread.

class RenderFlowThread: public RenderBlock {
public:
    RenderFlowThread();
    virtual ~RenderFlowThread() { };
    
    virtual bool isRenderFlowThread() const { return true; }

    virtual void layout();

    // Always create a RenderLayer for the RenderFlowThread so that we 
    // can easily avoid drawing the children directly.
    virtual bool requiresLayer() const { return true; }
    
    void removeFlowChildInfo(RenderObject*);
#ifndef NDEBUG
    bool hasChildInfo(RenderObject* child) const { return child && child->isBox() && m_regionRangeMap.contains(toRenderBox(child)); }
#endif

    virtual void addRegionToThread(RenderRegion*);
    virtual void removeRegionFromThread(RenderRegion*);
    const RenderRegionList& renderRegionList() const { return m_regionList; }

    virtual void updateLogicalWidth() OVERRIDE;
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

    void paintFlowThreadPortionInRegion(PaintInfo&, RenderRegion*, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect, const LayoutPoint&) const;
    bool hitTestFlowThreadPortionInRegion(RenderRegion*, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect, const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset) const;
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;

    bool hasRegions() const { return m_regionList.size(); }
    // Check if the content is flown into at least a region with region styling rules.
    bool hasRegionsWithStyling() const { return m_hasRegionsWithStyling; }
    void checkRegionsWithStyling();

    void validateRegions();
    void invalidateRegions();
    bool hasValidRegionInfo() const { return !m_regionsInvalidated && !m_regionList.isEmpty(); }

    static PassRefPtr<RenderStyle> createFlowThreadStyle(RenderStyle* parentStyle);

    void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void repaintRectangleInRegions(const LayoutRect&, bool immediate) const;
    
    LayoutPoint adjustedPositionRelativeToOffsetParent(const RenderBoxModelObject&, const LayoutPoint&);

    LayoutUnit pageLogicalTopForOffset(LayoutUnit);
    LayoutUnit pageLogicalWidthForOffset(LayoutUnit);
    LayoutUnit pageLogicalHeightForOffset(LayoutUnit);
    LayoutUnit pageRemainingLogicalHeightForOffset(LayoutUnit, PageBoundaryRule = IncludePageBoundary);

    virtual void setPageBreak(LayoutUnit /*offset*/, LayoutUnit /*spaceShortage*/) { }
    virtual void updateMinimumPageHeight(LayoutUnit /*offset*/, LayoutUnit /*minHeight*/) { }

    enum RegionAutoGenerationPolicy {
        AllowRegionAutoGeneration,
        DisallowRegionAutoGeneration,
    };
    RenderRegion* regionAtBlockOffset(LayoutUnit, bool extendLastRegion = false, RegionAutoGenerationPolicy = AllowRegionAutoGeneration);

    bool regionsHaveUniformLogicalWidth() const { return m_regionsHaveUniformLogicalWidth; }
    bool regionsHaveUniformLogicalHeight() const { return m_regionsHaveUniformLogicalHeight; }

    RenderRegion* mapFromFlowToRegion(TransformState&) const;

    void removeRenderBoxRegionInfo(RenderBox*);
    bool logicalWidthChangedInRegionsForBlock(const RenderBlock*);

    LayoutUnit contentLogicalWidthOfFirstRegion() const;
    LayoutUnit contentLogicalHeightOfFirstRegion() const;
    LayoutUnit contentLogicalLeftOfFirstRegion() const;
    
    RenderRegion* firstRegion() const;
    RenderRegion* lastRegion() const;

    bool previousRegionCountChanged() const { return m_previousRegionCount != m_regionList.size(); };
    void updatePreviousRegionCount() { m_previousRegionCount = m_regionList.size(); };

    void setRegionRangeForBox(const RenderBox*, LayoutUnit offsetFromLogicalTopOfFirstPage);
    void getRegionRangeForBox(const RenderBox*, RenderRegion*& startRegion, RenderRegion*& endRegion) const;

    void clearRenderObjectCustomStyle(const RenderObject*,
        const RenderRegion* oldStartRegion = 0, const RenderRegion* oldEndRegion = 0,
        const RenderRegion* newStartRegion = 0, const RenderRegion* newEndRegion = 0);

    // Check if the object is in region and the region is part of this flow thread.
    bool objectInFlowRegion(const RenderObject*, const RenderRegion*) const;

    void markAutoLogicalHeightRegionsForLayout();

    bool addForcedRegionBreak(LayoutUnit, RenderObject* breakChild, bool isBefore, LayoutUnit* offsetBreakAdjustment = 0);
    void applyBreakAfterContent(LayoutUnit);

    bool pageLogicalSizeChanged() const { return m_pageLogicalSizeChanged; }

    bool hasAutoLogicalHeightRegions() const { ASSERT(isAutoLogicalHeightRegionsCountConsistent()); return m_autoLogicalHeightRegionsCount; }
    void incrementAutoLogicalHeightRegions();
    void decrementAutoLogicalHeightRegions();

#ifndef NDEBUG
    bool isAutoLogicalHeightRegionsCountConsistent() const;
#endif

    void collectLayerFragments(LayerFragments&, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect);
    LayoutRect fragmentsBoundingBox(const LayoutRect& layerBoundingBox);

    void setInConstrainedLayoutPhase(bool value) { m_inConstrainedLayoutPhase = value; }
    bool inConstrainedLayoutPhase() const { return m_inConstrainedLayoutPhase; }

    bool needsTwoPhasesLayout() const { return m_needsTwoPhasesLayout; }
    void clearNeedsTwoPhasesLayout() { m_needsTwoPhasesLayout = false; }

    void pushFlowThreadLayoutState(const RenderObject*);
    void popFlowThreadLayoutState();
    LayoutUnit offsetFromLogicalTopOfFirstRegion(const RenderBlock*) const;

    // Used to estimate the maximum height of the flow thread.
    static LayoutUnit maxLogicalHeight() { return LayoutUnit::max() / 2; }

protected:
    virtual const char* renderName() const = 0;

    // Overridden by columns/pages to set up an initial logical width of the page width even when
    // no regions have been generated yet.
    virtual LayoutUnit initialLogicalWidth() const { return 0; };

    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const OVERRIDE;

    void updateRegionsFlowThreadPortionRect(const RenderRegion* = 0);
    bool shouldRepaint(const LayoutRect&) const;
    bool regionInRange(const RenderRegion* targetRegion, const RenderRegion* startRegion, const RenderRegion* endRegion) const;

    LayoutRect computeRegionClippingRect(const LayoutPoint&, const LayoutRect&, const LayoutRect&) const;

    void setDispatchRegionLayoutUpdateEvent(bool value) { m_dispatchRegionLayoutUpdateEvent = value; }
    bool shouldDispatchRegionLayoutUpdateEvent() { return m_dispatchRegionLayoutUpdateEvent; }
    
    void setDispatchRegionOversetChangeEvent(bool value) { m_dispatchRegionOversetChangeEvent = value; }
    bool shouldDispatchRegionOversetChangeEvent() const { return m_dispatchRegionOversetChangeEvent; }
    
    // Override if the flow thread implementation supports dispatching events when the flow layout is updated (e.g. for named flows)
    virtual void dispatchRegionLayoutUpdateEvent() { m_dispatchRegionLayoutUpdateEvent = false; }
    virtual void dispatchRegionOversetChangeEvent() { m_dispatchRegionOversetChangeEvent = false; }

    void initializeRegionsComputedAutoHeight(RenderRegion* = 0);

    virtual void autoGenerateRegionsToBlockOffset(LayoutUnit) { };

    inline bool hasCachedOffsetFromLogicalTopOfFirstRegion(const RenderBox*) const;
    inline LayoutUnit cachedOffsetFromLogicalTopOfFirstRegion(const RenderBox*) const;
    inline void setOffsetFromLogicalTopOfFirstRegion(const RenderBox*, LayoutUnit);
    inline void clearOffsetFromLogicalTopOfFirstRegion(const RenderBox*);

    inline const RenderBox* currentActiveRenderBox() const;

    RenderRegionList m_regionList;
    unsigned short m_previousRegionCount;

    class RenderRegionRange {
    public:
        RenderRegionRange()
        {
            setRange(0, 0);
        }

        RenderRegionRange(RenderRegion* start, RenderRegion* end)
        {
            setRange(start, end);
        }
        
        void setRange(RenderRegion* start, RenderRegion* end)
        {
            m_startRegion = start;
            m_endRegion = end;
        }

        RenderRegion* startRegion() const { return m_startRegion; }
        RenderRegion* endRegion() const { return m_endRegion; }

    private:
        RenderRegion* m_startRegion;
        RenderRegion* m_endRegion;
    };

    typedef PODInterval<LayoutUnit, RenderRegion*> RegionInterval;
    typedef PODIntervalTree<LayoutUnit, RenderRegion*> RegionIntervalTree;

    class RegionSearchAdapter {
    public:
        RegionSearchAdapter(LayoutUnit offset)
            : m_offset(offset)
            , m_result(0)
        {
        }
        
        const LayoutUnit& lowValue() const { return m_offset; }
        const LayoutUnit& highValue() const { return m_offset; }
        void collectIfNeeded(const RegionInterval&);

        RenderRegion* result() const { return m_result; }

    private:
        LayoutUnit m_offset;
        RenderRegion* m_result;
    };

    // A maps from RenderBox
    typedef HashMap<const RenderBox*, RenderRegionRange> RenderRegionRangeMap;
    RenderRegionRangeMap m_regionRangeMap;

    typedef HashMap<RenderObject*, RenderRegion*> RenderObjectToRegionMap;
    RenderObjectToRegionMap m_breakBeforeToRegionMap;
    RenderObjectToRegionMap m_breakAfterToRegionMap;

    typedef ListHashSet<const RenderObject*> RenderObjectStack;
    RenderObjectStack m_activeObjectsStack;
    typedef HashMap<const RenderBox*, LayoutUnit> RenderBoxToOffsetMap;
    RenderBoxToOffsetMap m_boxesToOffsetMap;

    unsigned m_autoLogicalHeightRegionsCount;

    RegionIntervalTree m_regionIntervalTree;

    bool m_regionsInvalidated : 1;
    bool m_regionsHaveUniformLogicalWidth : 1;
    bool m_regionsHaveUniformLogicalHeight : 1;
    bool m_hasRegionsWithStyling : 1;
    bool m_dispatchRegionLayoutUpdateEvent : 1;
    bool m_dispatchRegionOversetChangeEvent : 1;
    bool m_pageLogicalSizeChanged : 1;
    bool m_inConstrainedLayoutPhase : 1;
    bool m_needsTwoPhasesLayout : 1;
};

inline RenderFlowThread* toRenderFlowThread(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderFlowThread());
    return static_cast<RenderFlowThread*>(object);
}

inline const RenderFlowThread* toRenderFlowThread(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderFlowThread());
    return static_cast<const RenderFlowThread*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFlowThread(const RenderFlowThread*);

class CurrentRenderFlowThreadMaintainer {
    WTF_MAKE_NONCOPYABLE(CurrentRenderFlowThreadMaintainer);
public:
    CurrentRenderFlowThreadMaintainer(RenderFlowThread*);
    ~CurrentRenderFlowThreadMaintainer();
private:
    RenderFlowThread* m_renderFlowThread;
    RenderFlowThread* m_previousRenderFlowThread;
};

// These structures are used by PODIntervalTree for debugging.
#ifndef NDEBUG
template <> struct ValueToString<LayoutUnit> {
    static String string(const LayoutUnit value) { return String::number(value.toFloat()); }
};

template <> struct ValueToString<RenderRegion*> {
    static String string(const RenderRegion* value) { return String::format("%p", value); }
};
#endif

} // namespace WebCore

#endif // RenderFlowThread_h
