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

#ifndef RenderMeter_h
#define RenderMeter_h

#if ENABLE(METER_ELEMENT)
#include "RenderBlock.h"
#include "RenderWidget.h"


namespace WebCore {

class HTMLMeterElement;

class RenderMeter : public RenderBlock {
public:
    explicit RenderMeter(HTMLElement*);
    virtual ~RenderMeter();

    HTMLMeterElement* meterElement() const;
    virtual void updateFromElement();

private:
    virtual void updateLogicalWidth() OVERRIDE;
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

    virtual const char* renderName() const { return "RenderMeter"; }
    virtual bool isMeter() const { return true; }
    virtual bool requiresForcedStyleRecalcPropagation() const { return true; }
};

inline RenderMeter* toRenderMeter(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isMeter());
    return static_cast<RenderMeter*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMeter(const RenderMeter*);

} // namespace WebCore

#endif

#endif // RenderMeter_h

