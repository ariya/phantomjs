/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPointList.h"

#include "FloatPoint.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

String SVGPointList::valueAsString() const
{
    StringBuilder builder;

    unsigned size = this->size();
    for (unsigned i = 0; i < size; ++i) {
        if (i > 0)
            builder.append(" "); // FIXME: Shouldn't we use commas to seperate?

        const FloatPoint& point = at(i);
        builder.append(makeString(String::number(point.x()), ' ', String::number(point.y())));
    }

    return builder.toString();
}

float inline adjustAnimatedValue(float from, float to, float progress)
{
    return (to - from) * progress + from;
}

bool SVGPointList::createAnimated(const SVGPointList& fromList, const SVGPointList& toList, SVGPointList& resultList, float progress)
{
    unsigned itemCount = fromList.size();
    if (!itemCount || itemCount != toList.size())
        return false;
    for (unsigned n = 0; n < itemCount; ++n) {
        const FloatPoint& from = fromList.at(n);
        const FloatPoint& to = toList.at(n);
        FloatPoint segment = FloatPoint(adjustAnimatedValue(from.x(), to.x(), progress),
                                        adjustAnimatedValue(from.y(), to.y(), progress));
        resultList.append(segment);
    }
    return true;
}

}

#endif // ENABLE(SVG)
