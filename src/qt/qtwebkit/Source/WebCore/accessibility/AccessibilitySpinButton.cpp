/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilitySpinButton.h"

#include "AXObjectCache.h"
#include "RenderObject.h"

namespace WebCore {

PassRefPtr<AccessibilitySpinButton> AccessibilitySpinButton::create()
{
    return adoptRef(new AccessibilitySpinButton);
}
    
AccessibilitySpinButton::AccessibilitySpinButton()
    : m_spinButtonElement(0)
{
}

AccessibilitySpinButton::~AccessibilitySpinButton()
{
}
    
AccessibilityObject* AccessibilitySpinButton::incrementButton()
{
    if (!m_haveChildren)
        addChildren();

    ASSERT(m_children.size() == 2);

    return m_children[0].get();
}
   
AccessibilityObject* AccessibilitySpinButton::decrementButton()
{
    if (!m_haveChildren)
        addChildren();
    
    ASSERT(m_children.size() == 2);
    
    return m_children[1].get();    
}
    
LayoutRect AccessibilitySpinButton::elementRect() const
{
    ASSERT(m_spinButtonElement);
    
    if (!m_spinButtonElement || !m_spinButtonElement->renderer())
        return LayoutRect();
    
    Vector<FloatQuad> quads;
    m_spinButtonElement->renderer()->absoluteFocusRingQuads(quads);

    return boundingBoxForQuads(m_spinButtonElement->renderer(), quads);
}

void AccessibilitySpinButton::addChildren()
{
    m_haveChildren = true;
    
    AccessibilitySpinButtonPart* incrementor = static_cast<AccessibilitySpinButtonPart*>(axObjectCache()->getOrCreate(SpinButtonPartRole));
    incrementor->setIsIncrementor(true);
    incrementor->setParent(this);
    m_children.append(incrementor);

    AccessibilitySpinButtonPart* decrementor = static_cast<AccessibilitySpinButtonPart*>(axObjectCache()->getOrCreate(SpinButtonPartRole));
    decrementor->setIsIncrementor(false);
    decrementor->setParent(this);
    m_children.append(decrementor);
}
    
void AccessibilitySpinButton::step(int amount)
{
    ASSERT(m_spinButtonElement);
    if (!m_spinButtonElement)
        return;
    
    m_spinButtonElement->step(amount);
}

// AccessibilitySpinButtonPart 

AccessibilitySpinButtonPart::AccessibilitySpinButtonPart()
    : m_isIncrementor(false)
{
}
    
PassRefPtr<AccessibilitySpinButtonPart> AccessibilitySpinButtonPart::create()
{
    return adoptRef(new AccessibilitySpinButtonPart);
}

LayoutRect AccessibilitySpinButtonPart::elementRect() const
{
    // FIXME: This logic should exist in the render tree or elsewhere, but there is no
    // relationship that exists that can be queried.
    
    LayoutRect parentRect = parentObject()->elementRect();
    if (m_isIncrementor)
        parentRect.setHeight(parentRect.height() / 2);
    else {
        parentRect.setY(parentRect.y() + parentRect.height() / 2);        
        parentRect.setHeight(parentRect.height() / 2);        
    }
        
    return parentRect;
}

bool AccessibilitySpinButtonPart::press() const
{
    if (!m_parent || !m_parent->isSpinButton())
        return false;
    
    AccessibilitySpinButton* spinButton = toAccessibilitySpinButton(parentObject());
    if (m_isIncrementor)
        spinButton->step(1);
    else
        spinButton->step(-1);
    
    return true;
}

} // namespace WebCore
