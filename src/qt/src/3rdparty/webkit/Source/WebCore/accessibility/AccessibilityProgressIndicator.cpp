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

#if ENABLE(PROGRESS_TAG)

#include "AccessibilityProgressIndicator.h"

#include "FloatConversion.h"
#include "HTMLNames.h"
#include "HTMLProgressElement.h"
#include "RenderObject.h"
#include "RenderProgress.h"

namespace WebCore {
    
using namespace HTMLNames;

AccessibilityProgressIndicator::AccessibilityProgressIndicator(RenderProgress* renderer)
    : AccessibilityRenderObject(renderer)
{
}

PassRefPtr<AccessibilityProgressIndicator> AccessibilityProgressIndicator::create(RenderProgress* renderer)
{
    return adoptRef(new AccessibilityProgressIndicator(renderer));
}

bool AccessibilityProgressIndicator::accessibilityIsIgnored() const
{
    return accessibilityIsIgnoredBase() == IgnoreObject;
}
    
float AccessibilityProgressIndicator::valueForRange() const
{
    if (element()->position() >= 0)
        return narrowPrecisionToFloat(element()->value());
    // Indeterminate progress bar should return 0.
    return 0.0f;
}

float AccessibilityProgressIndicator::maxValueForRange() const
{
    return narrowPrecisionToFloat(element()->max());
}

float AccessibilityProgressIndicator::minValueForRange() const
{
    return 0.0f;
}

HTMLProgressElement* AccessibilityProgressIndicator::element() const
{
    return toRenderProgress(m_renderer)->progressElement();
}


} // namespace WebCore

#endif // ENABLE(PROGRESS_TAG)
