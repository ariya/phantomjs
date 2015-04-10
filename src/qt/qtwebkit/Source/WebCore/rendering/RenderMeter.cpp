/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#if ENABLE(METER_ELEMENT)
#include "RenderMeter.h"

#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "RenderTheme.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderMeter::RenderMeter(HTMLElement* element)
    : RenderBlock(element)
{
}

RenderMeter::~RenderMeter()
{
}

HTMLMeterElement* RenderMeter::meterElement() const
{
    ASSERT(node());

    if (isHTMLMeterElement(node()))
        return toHTMLMeterElement(node());

    ASSERT(node()->shadowHost());
    return toHTMLMeterElement(node()->shadowHost());
}

void RenderMeter::updateLogicalWidth()
{
    RenderBox::updateLogicalWidth();

    IntSize frameSize = theme()->meterSizeForBounds(this, pixelSnappedIntRect(frameRect()));
    setLogicalWidth(isHorizontalWritingMode() ? frameSize.width() : frameSize.height());
}

void RenderMeter::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    RenderBox::computeLogicalHeight(logicalHeight, logicalTop, computedValues);

    LayoutRect frame = frameRect();
    if (isHorizontalWritingMode())
        frame.setHeight(computedValues.m_extent);
    else
        frame.setWidth(computedValues.m_extent);
    IntSize frameSize = theme()->meterSizeForBounds(this, pixelSnappedIntRect(frame));
    computedValues.m_extent = isHorizontalWritingMode() ? frameSize.height() : frameSize.width();
}

void RenderMeter::updateFromElement()
{
    repaint();
}

} // namespace WebCore

#endif
