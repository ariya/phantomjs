/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RenderProgress_h
#define RenderProgress_h

#if ENABLE(PROGRESS_TAG)
#include "RenderBlock.h"

namespace WebCore {

class HTMLProgressElement;

class RenderProgress : public RenderBlock {
public:
    RenderProgress(HTMLProgressElement*);
    virtual ~RenderProgress();

    double position() const { return m_position; }
    double animationProgress() const;
    double animationStartTime() const { return m_animationStartTime; }

    bool isDeterminate() const;

    HTMLProgressElement* progressElement() const;

private:
    virtual const char* renderName() const { return "RenderProgress"; }
    virtual bool isProgress() const { return true; }
    virtual bool requiresForcedStyleRecalcPropagation() const { return true; }
    virtual bool canHaveChildren() const { return false; }
    virtual void updateFromElement();

    void animationTimerFired(Timer<RenderProgress>*);
    void updateAnimationState();

    double m_position;
    double m_animationStartTime;
    double m_animationRepeatInterval;
    double m_animationDuration;
    bool m_animating;
    Timer<RenderProgress> m_animationTimer;
};

inline RenderProgress* toRenderProgress(RenderObject* object)
{
    ASSERT(!object || object->isProgress());
    return static_cast<RenderProgress*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderProgress(const RenderProgress*);

} // namespace WebCore

#endif

#endif // RenderProgress_h

