/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
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

#ifndef RenderTextControl_h
#define RenderTextControl_h

#include "RenderBlock.h"
#include "RenderFlexibleBox.h"

namespace WebCore {

class HTMLTextFormControlElement;

class RenderTextControl : public RenderBlock {
public:
    virtual ~RenderTextControl();

    HTMLTextFormControlElement* textFormControlElement() const;
    virtual PassRefPtr<RenderStyle> createInnerTextStyle(const RenderStyle* startStyle) const = 0;

    VisiblePosition visiblePositionForIndex(int index) const;

protected:
    RenderTextControl(Element*);

    // This convenience function should not be made public because innerTextElement may outlive the render tree.
    HTMLElement* innerTextElement() const;

    int scrollbarThickness() const;
    void adjustInnerTextStyle(const RenderStyle* startStyle, RenderStyle* textBlockStyle) const;

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    void hitInnerTextElement(HitTestResult&, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset);

    int textBlockLogicalWidth() const;
    int textBlockLogicalHeight() const;

    float scaleEmToUnits(int x) const;

    static bool hasValidAvgCharWidth(AtomicString family);
    virtual float getAvgCharWidth(AtomicString family);
    virtual LayoutUnit preferredContentLogicalWidth(float charWidth) const = 0;
    virtual LayoutUnit computeControlLogicalHeight(LayoutUnit lineHeight, LayoutUnit nonContentHeight) const = 0;
    virtual RenderStyle* textBaseStyle() const = 0;

    virtual void updateFromElement();
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;
    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren);

private:
    virtual const char* renderName() const { return "RenderTextControl"; }
    virtual bool isTextControl() const { return true; }
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;
    virtual void computePreferredLogicalWidths() OVERRIDE;
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { }
    virtual bool avoidsFloats() const { return true; }
    virtual bool canHaveGeneratedChildren() const OVERRIDE { return false; }
    virtual bool canBeReplacedWithInlineRunIn() const OVERRIDE;
    
    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) OVERRIDE;

    virtual bool canBeProgramaticallyScrolled() const { return true; }

    virtual bool requiresForcedStyleRecalcPropagation() const { return true; }
};

inline RenderTextControl* toRenderTextControl(RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTextControl());
    return static_cast<RenderTextControl*>(object);
}

inline const RenderTextControl* toRenderTextControl(const RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTextControl());
    return static_cast<const RenderTextControl*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTextControl(const RenderTextControl*);

// Renderer for our inner container, for <search> and others.
// We can't use RenderFlexibleBox directly, because flexboxes have a different
// baseline definition, and then inputs of different types wouldn't line up
// anymore.
class RenderTextControlInnerContainer : public RenderFlexibleBox {
public:
    explicit RenderTextControlInnerContainer(Element* element)
        : RenderFlexibleBox(element)
    { }
    virtual ~RenderTextControlInnerContainer() { }

    virtual int baselinePosition(FontBaseline baseline, bool firstLine, LineDirectionMode direction, LinePositionMode position) const OVERRIDE
    {
        return RenderBlock::baselinePosition(baseline, firstLine, direction, position);
    }
    virtual int firstLineBoxBaseline() const OVERRIDE { return RenderBlock::firstLineBoxBaseline(); }
    virtual int inlineBlockBaseline(LineDirectionMode direction) const OVERRIDE { return RenderBlock::inlineBlockBaseline(direction); }

};


} // namespace WebCore

#endif // RenderTextControl_h
