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

#if ENABLE(METER_TAG)

#include "RenderMeter.h"

#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "RenderTheme.h"
#include "ShadowElement.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderMeter::RenderMeter(HTMLMeterElement* element)
    : RenderBlock(element)
{
}

RenderMeter::~RenderMeter()
{
}

void RenderMeter::computeLogicalWidth()
{
    RenderBox::computeLogicalWidth();
    setWidth(theme()->meterSizeForBounds(this, frameRect()).width());
}

void RenderMeter::computeLogicalHeight()
{
    RenderBox::computeLogicalHeight();
    setHeight(theme()->meterSizeForBounds(this, frameRect()).height());
}

double RenderMeter::valueRatio() const
{
    return static_cast<HTMLMeterElement*>(node())->valueRatio();
}

} // namespace WebCore

#endif
