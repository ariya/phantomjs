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
#include "AccessibilityScrollView.h"

#include "AXObjectCache.h"
#include "AccessibilityScrollbar.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "RenderPart.h"
#include "ScrollView.h"
#include "Widget.h"

namespace WebCore {
    
AccessibilityScrollView::AccessibilityScrollView(ScrollView* view)
    : m_scrollView(view)
{
}

PassRefPtr<AccessibilityScrollView> AccessibilityScrollView::create(ScrollView* view)
{
    return adoptRef(new AccessibilityScrollView(view));
}
    
AccessibilityObject* AccessibilityScrollView::scrollBar(AccessibilityOrientation orientation) const
{
    switch (orientation) {
    case AccessibilityOrientationVertical:
        return m_verticalScrollbar ? m_verticalScrollbar.get() : 0;
    case AccessibilityOrientationHorizontal:
        return m_horizontalScrollbar ? m_horizontalScrollbar.get() : 0;
    }
    
    return 0;
}

const AccessibilityObject::AccessibilityChildrenVector& AccessibilityScrollView::children()
{
    if (!m_haveChildren)
        addChildren();
    return m_children;
}

// If this is WebKit1 then the native scroll view needs to return the
// AX information (because there are no scroll bar children in the ScrollView object in WK1).
// In WebKit2, the ScrollView object will return the AX information (because there are no platform widgets).
bool AccessibilityScrollView::isAttachment() const
{
    return m_scrollView->platformWidget();
}

Widget* AccessibilityScrollView::widgetForAttachmentView() const
{
    return m_scrollView.get();
}
    
void AccessibilityScrollView::updateChildrenIfNecessary()
{
    if (m_scrollView->horizontalScrollbar() && !m_horizontalScrollbar)
        m_horizontalScrollbar = addChildScrollbar(m_scrollView->horizontalScrollbar());
    else if (!m_scrollView->horizontalScrollbar() && m_horizontalScrollbar) {
        removeChildScrollbar(m_horizontalScrollbar.get());
        m_horizontalScrollbar = 0;
    }

    if (m_scrollView->verticalScrollbar() && !m_verticalScrollbar)
        m_verticalScrollbar = addChildScrollbar(m_scrollView->verticalScrollbar());
    else if (!m_scrollView->verticalScrollbar() && m_verticalScrollbar) {
        removeChildScrollbar(m_verticalScrollbar.get());
        m_verticalScrollbar = 0;
    }
}
    
void AccessibilityScrollView::removeChildScrollbar(AccessibilityObject* scrollbar)
{
    size_t pos = m_children.find(scrollbar);
    if (pos != WTF::notFound)
        m_children.remove(pos);
}
    
AccessibilityScrollbar* AccessibilityScrollView::addChildScrollbar(Scrollbar* scrollbar)
{
    if (!scrollbar)
        return 0;
    
    AccessibilityScrollbar* scrollBarObject = static_cast<AccessibilityScrollbar*>(axObjectCache()->getOrCreate(scrollbar));
    scrollBarObject->setParent(this);
    m_children.append(scrollBarObject);
    return scrollBarObject;
}
        
void AccessibilityScrollView::addChildren()
{
    ASSERT(!m_haveChildren);
    m_haveChildren = true;
    
    AccessibilityObject* webArea = webAreaObject();
    if (webArea)
        m_children.append(webArea);
    
    updateChildrenIfNecessary();
}

AccessibilityObject* AccessibilityScrollView::webAreaObject() const
{
    if (!m_scrollView->isFrameView())
        return 0;
    
    Document* doc = static_cast<FrameView*>(m_scrollView.get())->frame()->document();
    if (!doc || !doc->renderer())
        return 0;

    return axObjectCache()->getOrCreate(doc->renderer());
}

AccessibilityObject* AccessibilityScrollView::accessibilityHitTest(const IntPoint& point) const
{
    AccessibilityObject* webArea = webAreaObject();
    if (!webArea)
        return 0;
    
    if (m_horizontalScrollbar && m_horizontalScrollbar->elementRect().contains(point))
        return m_horizontalScrollbar.get();
    if (m_verticalScrollbar && m_verticalScrollbar->elementRect().contains(point))
        return m_verticalScrollbar.get();
    
    return webArea->accessibilityHitTest(point);
}

IntRect AccessibilityScrollView::elementRect() const
{
    return m_scrollView->frameRect();
}

FrameView* AccessibilityScrollView::documentFrameView() const
{
    if (!m_scrollView->isFrameView())
        return 0;
    
    return static_cast<FrameView*>(m_scrollView.get());
}    

AccessibilityObject* AccessibilityScrollView::parentObject() const
{
    if (!m_scrollView->isFrameView())
        return 0;
    
    HTMLFrameOwnerElement* owner = static_cast<FrameView*>(m_scrollView.get())->frame()->ownerElement();
    if (owner && owner->renderPart())
        return axObjectCache()->getOrCreate(owner->renderPart()->parent());
    
    return 0;
}
    
} // namespace WebCore    
