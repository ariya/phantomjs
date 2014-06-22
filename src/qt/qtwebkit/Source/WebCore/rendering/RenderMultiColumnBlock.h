/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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


#ifndef RenderMultiColumnBlock_h
#define RenderMultiColumnBlock_h

#include "RenderBlock.h"

namespace WebCore {

class RenderMultiColumnFlowThread;

class RenderMultiColumnBlock : public RenderBlock {
public:
    RenderMultiColumnBlock(Element*);

    LayoutUnit columnHeightAvailable() const { return m_columnHeightAvailable; }

    LayoutUnit columnWidth() const { return m_columnWidth; }
    unsigned columnCount() const { return m_columnCount; }

    RenderMultiColumnFlowThread* flowThread() const { return m_flowThread; }

    bool requiresBalancing() const { return !m_columnHeightAvailable; }

private:
    virtual bool isRenderMultiColumnBlock() const { return true; }
    
    virtual const char* renderName() const;

    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren) OVERRIDE;

    virtual void styleDidChange(StyleDifference, const RenderStyle*) OVERRIDE;
    
    virtual bool updateLogicalWidthAndColumnWidth() OVERRIDE;
    virtual void checkForPaginationLogicalHeightChange(LayoutUnit& pageLogicalHeight, bool& pageLogicalHeightChanged, bool& hasSpecifiedPageLogicalHeight) OVERRIDE;
    virtual bool relayoutForPagination(bool hasSpecifiedPageLogicalHeight, LayoutUnit pageLogicalHeight, LayoutStateMaintainer&) OVERRIDE;

    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0) OVERRIDE;

    void computeColumnCountAndWidth();

    void ensureColumnSets();

    RenderMultiColumnFlowThread* m_flowThread;
    unsigned m_columnCount;   // The default column count/width that are based off our containing block width. These values represent only the default,
    LayoutUnit m_columnWidth; // since a multi-column block that is split across variable width pages or regions will have different column counts and widths in each.
                              // These values will be cached (eventually) for multi-column blocks.
    LayoutUnit m_columnHeightAvailable; // Total height available to columns, or 0 if auto.
    bool m_inBalancingPass; // Set when relayouting for column balancing.
};

inline RenderMultiColumnBlock* toRenderMultiColumnBlock(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMultiColumnBlock());
    return static_cast<RenderMultiColumnBlock*>(object);
}

inline const RenderMultiColumnBlock* toRenderMultiColumnBlock(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMultiColumnBlock());
    return static_cast<const RenderMultiColumnBlock*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMultiColumnBlock(const RenderMultiColumnBlock*);

} // namespace WebCore

#endif // RenderMultiColumnBlock_h

