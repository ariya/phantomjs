/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
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

#include "RenderMathMLFenced.h"

#include "FontSelector.h"
#include "MathMLNames.h"
#include "RenderInline.h"
#include "RenderMathMLOperator.h"
#include "RenderText.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {
    
using namespace MathMLNames;
    
enum Braces { OpeningBraceChar = 0x28, ClosingBraceChar = 0x29 };
    
static const float gSeparatorMarginEndEms = 0.25f;
static const float gFenceMarginEms = 0.1f;

RenderMathMLFenced::RenderMathMLFenced(Element* element)
    : RenderMathMLRow(element)
    , m_open(OpeningBraceChar)
    , m_close(ClosingBraceChar)
    , m_closeFenceRenderer(0)
{
}

void RenderMathMLFenced::updateFromElement()
{
    Element* fenced = toElement(node());
 
    // FIXME: Handle open/close values with more than one character (they should be treated like text).
    AtomicString openValue = fenced->getAttribute(MathMLNames::openAttr);
    if (openValue.length() > 0)
        m_open = openValue[0];
    AtomicString closeValue = fenced->getAttribute(MathMLNames::closeAttr);
    if (closeValue.length() > 0)
        m_close = closeValue[0];
    
    AtomicString separators = fenced->getAttribute(MathMLNames::separatorsAttr);
    if (!separators.isNull()) {
        StringBuilder characters;
        for (unsigned int i = 0; i < separators.length(); i++) {
            if (!isSpaceOrNewline(separators[i]))
                characters.append(separators[i]);
        }
        m_separators = !characters.length() ? 0 : characters.toString().impl();
    } else {
        // The separator defaults to a single comma.
        m_separators = StringImpl::create(",");
    }
    
    if (isEmpty())
        makeFences();
}

RenderMathMLOperator* RenderMathMLFenced::createMathMLOperator(UChar uChar, RenderMathMLOperator::OperatorType operatorType)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(style(), FLEX);
    newStyle->setFlexDirection(FlowColumn);
    newStyle->setMarginEnd(Length((operatorType == RenderMathMLOperator::Fence ? gFenceMarginEms : gSeparatorMarginEndEms) * style()->fontSize(), Fixed));
    if (operatorType == RenderMathMLOperator::Fence)
        newStyle->setMarginStart(Length(gFenceMarginEms * style()->fontSize(), Fixed));
    RenderMathMLOperator* newOperator = new (renderArena()) RenderMathMLOperator(toElement(node()), uChar);
    newOperator->setOperatorType(operatorType);
    newOperator->setStyle(newStyle.release());
    return newOperator;
}

void RenderMathMLFenced::makeFences()
{
    RenderMathMLRow::addChild(createMathMLOperator(m_open, RenderMathMLOperator::Fence), firstChild());
    m_closeFenceRenderer = createMathMLOperator(m_close, RenderMathMLOperator::Fence);
    RenderMathMLRow::addChild(m_closeFenceRenderer);
}

void RenderMathMLFenced::addChild(RenderObject* child, RenderObject* beforeChild)
{
    // make the fences if the render object is empty
    if (isEmpty())
        updateFromElement();
    
    // FIXME: Adding or removing a child should possibly cause all later separators to shift places if they're different,
    // as later child positions change by +1 or -1.
    
    RenderObject* separatorRenderer = 0;
    if (m_separators.get()) {
        unsigned int count = 0;
        for (Node* position = child->node(); position; position = position->previousSibling()) {
            if (position->isElementNode())
                count++;
        }
        if (!beforeChild) {
            // We're adding at the end (before the closing fence), so a new separator would go before the new child, not after it.
            --count;
        }
        // |count| is now the number of element children that will be before our new separator, i.e. it's the 1-based index of the separator.
        
        if (count > 0) {
            UChar separator;
            
            // Use the last separator if we've run out of specified separators.
            if (count > m_separators.get()->length())
                separator = (*m_separators.get())[m_separators.get()->length() - 1];
            else
                separator = (*m_separators.get())[count - 1];
                
            separatorRenderer = createMathMLOperator(separator, RenderMathMLOperator::Separator);
        }
    }
    
    if (beforeChild) {
        // Adding |x| before an existing |y| e.g. in element (y) - first insert our new child |x|, then its separator, to get (x, y).
        RenderMathMLRow::addChild(child, beforeChild);
        if (separatorRenderer)
            RenderMathMLRow::addChild(separatorRenderer, beforeChild);
    } else {
        // Adding |y| at the end of an existing element e.g. (x) - insert the separator first before the closing fence, then |y|, to get (x, y).
        if (separatorRenderer)
            RenderMathMLRow::addChild(separatorRenderer, m_closeFenceRenderer);
        RenderMathMLRow::addChild(child, m_closeFenceRenderer);
    }
}

// FIXME: Change createMathMLOperator() above to create an isAnonymous() operator, and remove this styleDidChange() function.
void RenderMathMLFenced::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderMathMLBlock::styleDidChange(diff, oldStyle);
    
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->node() == node()) {
            ASSERT(child->style()->refCount() == 1);
            child->style()->inheritFrom(style());
            bool isFence = child == firstChild() || child == lastChild();
            child->style()->setMarginEnd(Length((isFence ? gFenceMarginEms : gSeparatorMarginEndEms) * style()->fontSize(), Fixed));
            if (isFence)
                child->style()->setMarginStart(Length(gFenceMarginEms * style()->fontSize(), Fixed));
        }
    }
}

}    

#endif
