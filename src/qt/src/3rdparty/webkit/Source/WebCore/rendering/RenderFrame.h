/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderFrame_h
#define RenderFrame_h

#include "RenderFrameBase.h"
#include "RenderFrameSet.h"

namespace WebCore {

class HTMLFrameElement;

class RenderFrame : public RenderFrameBase {
public:
    explicit RenderFrame(HTMLFrameElement*);

    FrameEdgeInfo edgeInfo() const;

private:
    virtual const char* renderName() const { return "RenderFrame"; }
    virtual bool isFrame() const { return true; }

    virtual void viewCleared();
};

inline RenderFrame* toRenderFrame(RenderObject* object)
{
    ASSERT(!object || object->isFrame());
    return static_cast<RenderFrame*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFrame(const RenderFrame*);

} // namespace WebCore

#endif // RenderFrame_h
