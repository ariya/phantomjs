/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef RenderDetails_h
#define RenderDetails_h

#if ENABLE(DETAILS)

#include "RenderFlexibleBox.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class RenderDetails : public RenderBlock {
public:
    explicit RenderDetails(Node*);

    bool isOpen() const;

private:
    virtual const char* renderName() const { return "RenderDetails"; }
    virtual bool isDetails() const { return true; }
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void addChild(RenderObject* newChild, RenderObject *beforeChild = 0);
};

inline RenderDetails* toRenderDetails(RenderObject* object)
{
    ASSERT(!object || object->isDetails());
    return static_cast<RenderDetails*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderDetails(const RenderDetails*);

} // namespace WebCore

#endif

#endif // RenderDetails_h
