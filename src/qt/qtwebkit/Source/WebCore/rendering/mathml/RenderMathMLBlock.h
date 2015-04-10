/*
 * Copyright (C) 2010 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2012 David Barton (dbarton@mathscribe.com). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderMathMLBlock_h
#define RenderMathMLBlock_h

#if ENABLE(MATHML)

#include "RenderFlexibleBox.h"
#include "RenderTable.h"
#include "StyleInheritedData.h"

#define ENABLE_DEBUG_MATH_LAYOUT 0

namespace WebCore {
    
class RenderMathMLOperator;

class RenderMathMLBlock : public RenderFlexibleBox {
public:
    RenderMathMLBlock(Element* container);

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;
    
    virtual bool isRenderMathMLBlock() const { return true; }
    virtual bool isRenderMathMLOperator() const { return false; }
    virtual bool isRenderMathMLRow() const { return false; }
    virtual bool isRenderMathMLMath() const { return false; }
    virtual bool isRenderMathMLFenced() const { return false; }
    virtual bool isRenderMathMLFraction() const { return false; }
    virtual bool isRenderMathMLRoot() const { return false; }
    virtual bool isRenderMathMLSpace() const { return false; }
    virtual bool isRenderMathMLSquareRoot() const { return false; }
    virtual bool isRenderMathMLSubSup() const { return false; }
    virtual bool isRenderMathMLUnderOver() const { return false; }
    
    // MathML defines an "embellished operator" as roughly an <mo> that may have subscripts,
    // superscripts, underscripts, overscripts, or a denominator (as in d/dx, where "d" is some
    // differential operator). The padding, precedence, and stretchiness of the base <mo> should
    // apply to the embellished operator as a whole. unembellishedOperator() checks for being an
    // embellished operator, and omits any embellishments.
    // FIXME: We don't yet handle all the cases in the MathML spec. See
    // https://bugs.webkit.org/show_bug.cgi?id=78617.
    virtual RenderMathMLOperator* unembellishedOperator() { return 0; }
    
    // A MathML element's preferred logical widths often depend on its children's preferred heights, not just their widths.
    // This is due to operator stretching and other layout fine tuning. We define an element's preferred height to be its
    // actual height after layout inside a very wide parent.
    bool isPreferredLogicalHeightDirty() const { return preferredLogicalWidthsDirty() || m_preferredLogicalHeight < 0; }
    // The caller must ensure !isPreferredLogicalHeightDirty().
    LayoutUnit preferredLogicalHeight() const { ASSERT(!isPreferredLogicalHeightDirty()); return m_preferredLogicalHeight; }
    static const int preferredLogicalHeightUnset = -1;
    void setPreferredLogicalHeight(LayoutUnit logicalHeight) { m_preferredLogicalHeight = logicalHeight; }
    // computePreferredLogicalWidths() in derived classes must ensure m_preferredLogicalHeight is set to < 0 or its correct value.
    virtual void computePreferredLogicalWidths() OVERRIDE;
    
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const OVERRIDE;
    
#if ENABLE(DEBUG_MATH_LAYOUT)
    virtual void paint(PaintInfo&, const LayoutPoint&);
#endif
    
    // Create a new RenderMathMLBlock, with a new style inheriting from this->style().
    RenderMathMLBlock* createAnonymousMathMLBlock(EDisplay = FLEX);
    
    void setIgnoreInAccessibilityTree(bool flag) { m_ignoreInAccessibilityTree = flag; }
    bool ignoreInAccessibilityTree() const { return m_ignoreInAccessibilityTree; }
    
private:
    virtual const char* renderName() const OVERRIDE;
    bool m_ignoreInAccessibilityTree;
    
protected:
    // Set our logical width to a large value, and compute our children's preferred logical heights.
    void computeChildrenPreferredLogicalHeights();
    // This can only be called after children have been sized by computeChildrenPreferredLogicalHeights().
    static LayoutUnit preferredLogicalHeightAfterSizing(RenderObject* child);
    
    // m_preferredLogicalHeight is dirty if it's < 0 or preferredLogicalWidthsDirty().
    LayoutUnit m_preferredLogicalHeight;
};

inline RenderMathMLBlock* toRenderMathMLBlock(RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMathMLBlock());
    return static_cast<RenderMathMLBlock*>(object);
}

inline const RenderMathMLBlock* toRenderMathMLBlock(const RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMathMLBlock());
    return static_cast<const RenderMathMLBlock*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMathMLBlock(const RenderMathMLBlock*);

class RenderMathMLTable : public RenderTable {
public:
    explicit RenderMathMLTable(Element* element) : RenderTable(element) { }
    
    virtual int firstLineBoxBaseline() const OVERRIDE;
    
private:
    virtual const char* renderName() const OVERRIDE { return "RenderMathMLTable"; }
};

// Parsing functions for MathML Length values
bool parseMathMLLength(const String&, LayoutUnit&, const RenderStyle*, bool allowNegative = true);
bool parseMathMLNamedSpace(const String&, LayoutUnit&, const RenderStyle*, bool allowNegative = true);
}

#endif // ENABLE(MATHML)
#endif // RenderMathMLBlock_h
