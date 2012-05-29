/**
 * Copyright (C) 2005 Apple Computer, Inc.
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

#include "config.h"
#include "RenderButton.h"

#include "Document.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "RenderTextFragment.h"
#include "RenderTheme.h"

namespace WebCore {

using namespace HTMLNames;

RenderButton::RenderButton(Node* node)
    : RenderFlexibleBox(node)
    , m_buttonText(0)
    , m_inner(0)
    , m_default(false)
{
}

RenderButton::~RenderButton()
{
}

void RenderButton::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (!m_inner) {
        // Create an anonymous block.
        ASSERT(!firstChild());
        bool isFlexibleBox = style()->display() == BOX || style()->display() == INLINE_BOX;
        m_inner = createAnonymousBlock(isFlexibleBox);
        setupInnerStyle(m_inner->style());
        RenderFlexibleBox::addChild(m_inner);
    }
    
    m_inner->addChild(newChild, beforeChild);
}

void RenderButton::removeChild(RenderObject* oldChild)
{
    if (oldChild == m_inner || !m_inner) {
        RenderFlexibleBox::removeChild(oldChild);
        m_inner = 0;
    } else
        m_inner->removeChild(oldChild);
}

void RenderButton::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (m_inner) {
        // RenderBlock::setStyle is going to apply a new style to the inner block, which
        // will have the initial box flex value, 0. The current value is 1, because we set
        // it right below. Here we change it back to 0 to avoid getting a spurious layout hint
        // because of the difference.
        m_inner->style()->setBoxFlex(0);
    }
    RenderBlock::styleWillChange(diff, newStyle);
}

void RenderButton::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);

    if (m_buttonText)
        m_buttonText->setStyle(style());
    if (m_inner) // RenderBlock handled updating the anonymous block's style.
        setupInnerStyle(m_inner->style());

    if (!m_default && theme()->isDefault(this)) {
        if (!m_timer)
            m_timer = adoptPtr(new Timer<RenderButton>(this, &RenderButton::timerFired));
        m_timer->startRepeating(0.03);
        m_default = true;
    } else if (m_default && !theme()->isDefault(this)) {
        m_default = false;
        m_timer.clear();
    }
}

void RenderButton::setupInnerStyle(RenderStyle* innerStyle) 
{
    ASSERT(innerStyle->refCount() == 1);
    // RenderBlock::createAnonymousBlock creates a new RenderStyle, so this is
    // safe to modify.
    innerStyle->setBoxFlex(1.0f);
    innerStyle->setBoxOrient(style()->boxOrient());
}

void RenderButton::updateFromElement()
{
    // If we're an input element, we may need to change our button text.
    if (node()->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
        String value = input->valueWithDefault();
        setText(value);
    }
}

bool RenderButton::canHaveChildren() const
{
    // Input elements can't have children, but button elements can.  We'll
    // write the code assuming any other button types that might emerge in the future
    // can also have children.
    return !node()->hasTagName(inputTag);
}

void RenderButton::setText(const String& str)
{
    if (str.isEmpty()) {
        if (m_buttonText) {
            m_buttonText->destroy();
            m_buttonText = 0;
        }
    } else {
        if (m_buttonText)
            m_buttonText->setText(str.impl());
        else {
            m_buttonText = new (renderArena()) RenderTextFragment(document(), str.impl());
            m_buttonText->setStyle(style());
            addChild(m_buttonText);
        }
    }
}

String RenderButton::text() const
{
    return m_buttonText ? m_buttonText->text() : 0;
}

void RenderButton::updateBeforeAfterContent(PseudoId type)
{
    if (m_inner)
        m_inner->children()->updateBeforeAfterContent(m_inner, type, this);
    else
        children()->updateBeforeAfterContent(this, type);
}

IntRect RenderButton::controlClipRect(int tx, int ty) const
{
    // Clip to the padding box to at least give content the extra padding space.
    return IntRect(tx + borderLeft(), ty + borderTop(), width() - borderLeft() - borderRight(), height() - borderTop() - borderBottom());
}

void RenderButton::timerFired(Timer<RenderButton>*)
{
    // FIXME Bug 25110: Ideally we would stop our timer when our Document
    // enters the page cache. But we currently have no way of being notified
    // when that happens, so we'll just ignore the timer firing as long as
    // we're in the cache.
    if (document()->inPageCache())
        return;

    repaint();
}

} // namespace WebCore
