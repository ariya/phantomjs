/*
 * Copyright (C) 2005 Apple Computer
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

#ifndef RenderButton_h
#define RenderButton_h

#include "RenderFlexibleBox.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class RenderTextFragment;

// RenderButtons are just like normal flexboxes except that they will generate an anonymous block child.
// For inputs, they will also generate an anonymous RenderText and keep its style and content up
// to date as the button changes.
class RenderButton : public RenderFlexibleBox {
public:
    explicit RenderButton(Element*);
    virtual ~RenderButton();

    virtual const char* renderName() const { return "RenderButton"; }
    virtual bool isRenderButton() const { return true; }

    virtual bool canBeSelectionLeaf() const OVERRIDE { return node() && node()->rendererIsEditable(); }

    virtual void addChild(RenderObject* newChild, RenderObject *beforeChild = 0);
    virtual void removeChild(RenderObject*);
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { }
    virtual bool createsAnonymousWrapper() const { return true; }

    void setupInnerStyle(RenderStyle*);
    virtual void updateFromElement();

    virtual bool canHaveGeneratedChildren() const OVERRIDE;
    virtual bool hasControlClip() const { return true; }
    virtual LayoutRect controlClipRect(const LayoutPoint&) const;

    void setText(const String&);
    String text() const;

private:
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    virtual bool hasLineIfEmpty() const { return node() && node()->toInputElement(); }

    virtual bool requiresForcedStyleRecalcPropagation() const { return true; }

    void timerFired(Timer<RenderButton>*);

    RenderTextFragment* m_buttonText;
    RenderBlock* m_inner;

    OwnPtr<Timer<RenderButton> > m_timer;
    bool m_default;
};

inline RenderButton* toRenderButton(RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderButton());
    return static_cast<RenderButton*>(object);
}

inline const RenderButton* toRenderButton(const RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderButton());
    return static_cast<const RenderButton*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderButton(const RenderButton*);

} // namespace WebCore

#endif // RenderButton_h
