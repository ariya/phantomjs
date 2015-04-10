/*
 * Copyright (C) 2010 Alex Milowski (alex@milowski.com). All rights reserved.
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

#include "config.h"

#if ENABLE(MATHML)

#include "RenderMathMLSubSup.h"

#include "MathMLNames.h"

namespace WebCore {
    
using namespace MathMLNames;

RenderMathMLSubSup::RenderMathMLSubSup(Element* element) 
    : RenderMathMLBlock(element)
    , m_scripts(0)
{
    // Determine what kind of sub/sup expression we have by element name
    if (element->hasLocalName(MathMLNames::msubTag))
        m_kind = Sub;
    else if (element->hasLocalName(MathMLNames::msupTag))
        m_kind = Super;
    else {
        ASSERT(element->hasLocalName(MathMLNames::msubsupTag));
        m_kind = SubSup;
    }
}

RenderBoxModelObject* RenderMathMLSubSup::base() const
{
    RenderObject* baseWrapper = firstChild();
    if (!baseWrapper)
        return 0;
    RenderObject* base = baseWrapper->firstChild();
    if (!base || !base->isBoxModelObject())
        return 0;
    return toRenderBoxModelObject(base);
}

void RenderMathMLSubSup::fixAnonymousStyles()
{
    // Set the base wrapper's style so that baseHeight in layout() will be an unstretched height.
    ASSERT(firstChild() && firstChild()->style()->refCount() == 1);
    firstChild()->style()->setAlignSelf(AlignFlexStart);
    
    ASSERT(m_scripts && m_scripts->style()->refCount() == 1);
    RenderStyle* scriptsStyle = m_scripts->style();
    scriptsStyle->setFlexDirection(FlowColumn);
    scriptsStyle->setJustifyContent(m_kind == Sub ? JustifyFlexEnd : m_kind == Super ? JustifyFlexStart : JustifySpaceBetween);
    // Set this wrapper's font-size for its line-height & baseline position, for its children.
    scriptsStyle->setFontSize(static_cast<int>(0.75 * style()->fontSize()));
}

// FIXME: Handle arbitrary addChild/removeChild correctly throughout MathML.
void RenderMathMLSubSup::addChild(RenderObject* child, RenderObject* beforeChild)
{
    if (isEmpty()) {
        RenderMathMLBlock* baseWrapper = createAnonymousMathMLBlock();
        RenderMathMLBlock::addChild(baseWrapper);
        
        m_scripts = createAnonymousMathMLBlock();
        RenderMathMLBlock::addChild(m_scripts);
        
        fixAnonymousStyles();
    }
    
    if (firstChild()->isEmpty())
        firstChild()->addChild(child);
    else
        m_scripts->addChild(child, beforeChild && beforeChild->parent() == m_scripts ? beforeChild : 0);
}

void RenderMathMLSubSup::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderMathMLBlock::styleDidChange(diff, oldStyle);
    
    if (!isEmpty())
        fixAnonymousStyles();
}

RenderMathMLOperator* RenderMathMLSubSup::unembellishedOperator()
{
    RenderBoxModelObject* base = this->base();
    if (!base || !base->isRenderMathMLBlock())
        return 0;
    return toRenderMathMLBlock(base)->unembellishedOperator();
}

void RenderMathMLSubSup::layout()
{
    RenderMathMLBlock::layout();

    RenderMathMLBlock* baseWrapper = toRenderMathMLBlock(firstChild());
    if (!baseWrapper || !m_scripts)
        return;
    RenderBox* base = baseWrapper->firstChildBox();
    if (!base)
        return;

    // Our layout rules include: Don't let the superscript go below the "axis" (half x-height above the
    // baseline), or the subscript above the axis. Also, don't let the superscript's top edge be
    // below the base's top edge, or the subscript's bottom edge above the base's bottom edge.
    //
    // FIXME: Check any subscriptshift or superscriptshift attributes, and maybe use more sophisticated
    // heuristics from TeX or elsewhere. See https://bugs.webkit.org/show_bug.cgi?id=79274#c5.

    LayoutUnit baseHeight = base->logicalHeight();
    LayoutUnit baseBaseline = base->firstLineBoxBaseline();
    if (baseBaseline == -1)
        baseBaseline = baseHeight;
    LayoutUnit axis = style()->fontMetrics().xHeight() / 2;
    int fontSize = style()->fontSize();

    ASSERT(baseWrapper->style()->hasOneRef());
    bool needsSecondLayout = false;

    if (RenderBox* superscript = m_kind == Sub ? 0 : m_scripts->lastChildBox()) {
        LayoutUnit superscriptHeight = superscript->logicalHeight();
        LayoutUnit superscriptBaseline = superscript->firstLineBoxBaseline();
        if (superscriptBaseline == -1)
            superscriptBaseline = superscriptHeight;
        LayoutUnit minBaseline = max<LayoutUnit>(fontSize / 3 + 1 + superscriptBaseline, superscriptHeight + axis);

        Length newPadding = Length(max<LayoutUnit>(minBaseline - baseBaseline, 0), Fixed);
        if (newPadding != baseWrapper->style()->paddingTop()) {
            baseWrapper->style()->setPaddingTop(newPadding);
            needsSecondLayout = true;
        }
    }

    if (RenderBox* subscript = m_kind == Super ? 0 : m_scripts->firstChildBox()) {
        LayoutUnit subscriptHeight = subscript->logicalHeight();
        LayoutUnit subscriptBaseline = subscript->firstLineBoxBaseline();
        if (subscriptBaseline == -1)
            subscriptBaseline = subscriptHeight;
        LayoutUnit baseExtendUnderBaseline = baseHeight - baseBaseline;
        LayoutUnit subscriptUnderItsBaseline = subscriptHeight - subscriptBaseline;
        LayoutUnit minExtendUnderBaseline = max<LayoutUnit>(fontSize / 5 + 1 + subscriptUnderItsBaseline, subscriptHeight - axis);

        Length newPadding = Length(max<LayoutUnit>(minExtendUnderBaseline - baseExtendUnderBaseline, 0), Fixed);
        if (newPadding != baseWrapper->style()->paddingBottom()) {
            baseWrapper->style()->setPaddingBottom(newPadding);
            needsSecondLayout = true;
        }
    }

    if (!needsSecondLayout)
        return;

    setNeedsLayout(true, MarkOnlyThis);
    baseWrapper->setChildNeedsLayout(true, MarkOnlyThis);

    RenderMathMLBlock::layout();
}

}    

#endif // ENABLE(MATHML)
