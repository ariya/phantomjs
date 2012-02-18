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

namespace WebCore {
    
using namespace MathMLNames;
    
enum Braces { OpeningBraceChar = 0x28, ClosingBraceChar = 0x29 };
    
static const float gOperatorPadding = 0.1f;

RenderMathMLFenced::RenderMathMLFenced(Node* fenced) 
    : RenderMathMLRow(fenced)
    , m_open(OpeningBraceChar)
    , m_close(ClosingBraceChar)
{
}

void RenderMathMLFenced::updateFromElement()
{
    Element* fenced = static_cast<Element*>(node());
 
    // FIXME: Handle open/close values with more than one character (they should be treated like text).
    AtomicString openValue = fenced->getAttribute(MathMLNames::openAttr);
    if (openValue.length() > 0)
        m_open = openValue[0];
    AtomicString closeValue = fenced->getAttribute(MathMLNames::closeAttr);
    if (closeValue.length() > 0)
        m_close = closeValue[0];
    
    AtomicString separators = static_cast<Element*>(fenced)->getAttribute(MathMLNames::separatorsAttr);
    if (!separators.isNull()) {
        Vector<UChar> characters;
        for (unsigned int i = 0; i < separators.length(); i++) {
            if (!isSpaceOrNewline(separators[i]))
                characters.append(separators[i]);
        }
        m_separators = !characters.size() ? 0 : StringImpl::create(characters.data() , characters.size());
    } else {
        // The separator defaults to a single comma.
        m_separators = StringImpl::create(",");
    }
    
    if (isEmpty())
        makeFences();
}

RefPtr<RenderStyle> RenderMathMLFenced::makeOperatorStyle() 
{
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(INLINE_BLOCK);
    newStyle->setPaddingRight(Length(static_cast<int>(gOperatorPadding * style()->fontSize()), Fixed));
    return newStyle;
}

void RenderMathMLFenced::makeFences()
{
    RenderObject* openFence = new (renderArena()) RenderMathMLOperator(node(), m_open);
    openFence->setStyle(makeOperatorStyle().release());
    RenderBlock::addChild(openFence, firstChild());
    RenderObject* closeFence = new (renderArena()) RenderMathMLOperator(node(), m_close);
    closeFence->setStyle(makeOperatorStyle().release());
    RenderBlock::addChild(closeFence);
}

void RenderMathMLFenced::addChild(RenderObject* child, RenderObject*)
{
    // make the fences if the render object is empty
    if (isEmpty())
        updateFromElement();
    
    if (m_separators.get()) {
        unsigned int count = 0;
        for (Node* position = child->node(); position; position = position->previousSibling()) {
            if (position->nodeType() == Node::ELEMENT_NODE)
                count++;
        }
                
        if (count > 1) {
            UChar separator;
            
            // Use the last separator if we've run out of specified separators.
            if ((count - 1) >= m_separators.get()->length())
                separator = (*m_separators.get())[m_separators.get()->length() - 1];
            else
                separator = (*m_separators.get())[count - 1];
                
            RenderObject* separatorObj = new (renderArena()) RenderMathMLOperator(node(), separator);
            separatorObj->setStyle(makeOperatorStyle().release());
            RenderBlock::addChild(separatorObj, lastChild());
        }
    }
    
    // If we have a block, we'll wrap it in an inline-block.
    if (child->isBlockFlow() && child->style()->display() != INLINE_BLOCK) {
        // Block objects wrapper.

        RenderBlock* block = new (renderArena()) RenderBlock(node());
        RefPtr<RenderStyle> newStyle = RenderStyle::create();
        newStyle->inheritFrom(style());
        newStyle->setDisplay(INLINE_BLOCK);
        block->setStyle(newStyle.release());
        
        RenderBlock::addChild(block, lastChild());
        block->addChild(child);    
    } else
        RenderBlock::addChild(child, lastChild());
}

}    

#endif
