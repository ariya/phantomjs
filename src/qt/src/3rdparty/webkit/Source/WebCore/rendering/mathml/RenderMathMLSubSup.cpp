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

#include "FontSelector.h"
#include "MathMLNames.h"
#include "RenderInline.h"
#include "RenderTable.h"
#include "RenderTableCell.h"
#include "RenderTableRow.h"
#include "RenderTableSection.h"
#include "RenderText.h"

namespace WebCore {
    
using namespace MathMLNames;

static const int gTopAdjustDivisor = 3;
static const int gSubsupScriptMargin = 1;
static const float gSubSupStretch = 1.2f;

RenderMathMLSubSup::RenderMathMLSubSup(Element* element) 
    : RenderMathMLBlock(element)
    , m_scripts(0)
{
    // Determine what kind of under/over expression we have by element name
    if (element->hasLocalName(MathMLNames::msubTag))
        m_kind = Sub;
    else if (element->hasLocalName(MathMLNames::msupTag))
        m_kind = Sup;
    else if (element->hasLocalName(MathMLNames::msubsupTag))
        m_kind = SubSup;
    else 
        m_kind = SubSup;
}

void RenderMathMLSubSup::addChild(RenderObject* child, RenderObject* beforeChild)
{
    if (firstChild()) {
        // We already have a base, so this is the super/subscripts being added.
        
        if (m_kind == SubSup) {
            if (!m_scripts) {
                m_scripts = new (renderArena()) RenderMathMLBlock(node());
                RefPtr<RenderStyle> scriptsStyle = RenderStyle::create();
                scriptsStyle->inheritFrom(style());
                scriptsStyle->setDisplay(INLINE_BLOCK);
                scriptsStyle->setVerticalAlign(TOP);
                scriptsStyle->setMarginLeft(Length(gSubsupScriptMargin, Fixed));
                scriptsStyle->setTextAlign(LEFT);
                m_scripts->setStyle(scriptsStyle.release());
                RenderMathMLBlock::addChild(m_scripts, beforeChild);
            }
            
            RenderBlock* script = new (renderArena()) RenderMathMLBlock(node());
            RefPtr<RenderStyle> scriptStyle = RenderStyle::create();
            scriptStyle->inheritFrom(m_scripts->style());
            scriptStyle->setDisplay(BLOCK);
            script->setStyle(scriptStyle.release());
            
            m_scripts->addChild(script, m_scripts->firstChild());
            script->addChild(child);
        } else
            RenderMathMLBlock::addChild(child, beforeChild);
        
    } else {
        RenderMathMLBlock* wrapper = new (renderArena()) RenderMathMLBlock(node());
        RefPtr<RenderStyle> wrapperStyle = RenderStyle::create();
        wrapperStyle->inheritFrom(style());
        wrapperStyle->setDisplay(INLINE_BLOCK);
        wrapperStyle->setVerticalAlign(BASELINE);
        wrapper->setStyle(wrapperStyle.release());
        RenderMathMLBlock::addChild(wrapper, beforeChild);
        wrapper->addChild(child);
        
    }
}

void RenderMathMLSubSup::stretchToHeight(int height)
{
    RenderObject* base = firstChild();
    if (!base || !base->firstChild())
        return;
    
    if (base->firstChild() && base->firstChild()->isRenderMathMLBlock()) {
        RenderMathMLBlock* block = toRenderMathMLBlock(base->firstChild());
        block->stretchToHeight(static_cast<int>(gSubSupStretch * height));
        
        // Adjust the script placement after we stretch
        if (height > 0 && m_kind == SubSup && m_scripts) {
            RenderObject* script = m_scripts->firstChild();
            if (script) {
                // Calculate the script height without the container margins.
                RenderObject* top = script;
                int topHeight = getBoxModelObjectHeight(top->firstChild());
                int topAdjust = topHeight / gTopAdjustDivisor;
                top->style()->setMarginTop(Length(-topAdjust, Fixed));
                top->style()->setMarginBottom(Length(height - topHeight + topAdjust, Fixed));
                if (top->isBoxModelObject()) {
                    RenderBoxModelObject* topBox = toRenderBoxModelObject(top);
                    topBox->updateBoxModelInfoFromStyle();
                }
                m_scripts->setNeedsLayout(true);
                setNeedsLayout(true);
            }
        }
        
    }
}

int RenderMathMLSubSup::nonOperatorHeight() const 
{
    if (m_kind == SubSup) 
       return static_cast<int>(style()->fontSize()*gSubSupStretch);
    return static_cast<int>(style()->fontSize());
}

void RenderMathMLSubSup::layout() 
{
    if (firstChild())
        firstChild()->setNeedsLayout(true);
    if (m_scripts) 
        m_scripts->setNeedsLayout(true);
    
    RenderBlock::layout();
    
    if (m_kind == SubSup) {
        if (RenderObject* base = firstChild()) {
            int maxHeight = 0;
            RenderObject* current = base->firstChild();
            while (current) {
                int height = getBoxModelObjectHeight(current);
                if (height > maxHeight)
                    maxHeight = height;
                current = current->nextSibling();
            }
            int heightDiff = m_scripts ? (m_scripts->offsetHeight() - maxHeight) / 2 : 0;
            if (heightDiff < 0) 
                heightDiff = 0;
            base->style()->setPaddingTop(Length(heightDiff, Fixed));
            base->setNeedsLayout(true);
        }
        setNeedsLayout(true);
        RenderBlock::layout();
    }    
}

int RenderMathMLSubSup::baselinePosition(FontBaseline, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    RenderObject* base = firstChild();
    if (!base) 
        return offsetHeight();
    
    int baseline = offsetHeight();
    if (!base || !base->isBoxModelObject()) 
        return baseline;

    switch (m_kind) {
    case SubSup:
        base = base->firstChild();
        if (!base)
            break;

        if (m_scripts && base->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(base);
            
            int topAdjust = (m_scripts->offsetHeight() - box->offsetHeight()) / 2;
        
            // FIXME: The last bit of this calculation should be more exact.  Why is the 2-3px scaled for zoom necessary?
            // The baseline is top spacing of the base + the baseline of the base + adjusted space for zoom
            float zoomFactor = style()->effectiveZoom();
            return topAdjust + box->baselinePosition(AlphabeticBaseline, firstLine, direction, linePositionMode) + static_cast<int>((zoomFactor > 1.25 ? 2 : 3) * zoomFactor);
        }
        break;
    case Sup: 
    case Sub:
        RenderBoxModelObject* box = toRenderBoxModelObject(base);
        baseline = box->baselinePosition(AlphabeticBaseline, firstLine, direction, linePositionMode);
        break;
    }
    
    return baseline;
    
}
    
}    

#endif // ENABLE(MATHML)
