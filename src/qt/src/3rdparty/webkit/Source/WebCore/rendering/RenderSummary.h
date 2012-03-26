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

#ifndef RenderSummary_h
#define RenderSummary_h

#if ENABLE(DETAILS)

#include "RenderBlock.h"

namespace WebCore {

class RenderDetails;

class RenderSummary : public RenderBlock {
public:
    explicit RenderSummary(Node*);

private:
    virtual const char* renderName() const { return "RenderSummary"; }
    virtual bool isSummary() const { return true; }
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
};

inline RenderSummary* toRenderSummary(RenderObject* object)
{
    ASSERT(!object || object->isSummary());
    return static_cast<RenderSummary*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSummary(const RenderSummary*);

}

#endif

#endif // RenderSummary_h

