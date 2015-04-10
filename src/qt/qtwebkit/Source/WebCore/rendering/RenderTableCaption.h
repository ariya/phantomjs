/*
 * Copyright (C) 2011 Robert Hogan <robert@roberthogan.net>
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
 */

#ifndef RenderTableCaption_h
#define RenderTableCaption_h

#include "RenderBlock.h"

namespace WebCore {

class RenderTable;

class RenderTableCaption : public RenderBlock {
public:
    explicit RenderTableCaption(Element*);
    virtual ~RenderTableCaption();
    virtual LayoutUnit containingBlockLogicalWidthForContent() const OVERRIDE;
    
private:
    virtual bool isTableCaption() const OVERRIDE { return true; }

    virtual void insertedIntoTree() OVERRIDE;
    virtual void willBeRemovedFromTree() OVERRIDE;

    RenderTable* table() const;
};

inline RenderTableCaption* toRenderTableCaption(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTableCaption());
    return static_cast<RenderTableCaption*>(object);
}

inline const RenderTableCaption* toRenderTableCaption(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTableCaption());
    return static_cast<const RenderTableCaption*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTableCaption(const RenderTableCaption*);

} // namespace WebCore

#endif // RenderTableCaption_h
