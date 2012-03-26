/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGTextLayoutAttributes.h"

#include <stdio.h>
#include <wtf/text/CString.h>

namespace WebCore {

SVGTextLayoutAttributes::SVGTextLayoutAttributes(RenderSVGInlineText* context)
    : m_context(context)
{
}

void SVGTextLayoutAttributes::reserveCapacity(unsigned length)
{
    m_positioningLists.xValues.reserveCapacity(length);
    m_positioningLists.yValues.reserveCapacity(length);
    m_positioningLists.dxValues.reserveCapacity(length);
    m_positioningLists.dyValues.reserveCapacity(length);
    m_positioningLists.rotateValues.reserveCapacity(length);
    m_textMetricsValues.reserveCapacity(length);
}

void SVGTextLayoutAttributes::PositioningLists::fillWithEmptyValues(unsigned length)
{
    xValues.fill(SVGTextLayoutAttributes::emptyValue(), length);
    yValues.fill(SVGTextLayoutAttributes::emptyValue(), length);
    dxValues.fill(SVGTextLayoutAttributes::emptyValue(), length);
    dyValues.fill(SVGTextLayoutAttributes::emptyValue(), length);
    rotateValues.fill(SVGTextLayoutAttributes::emptyValue(), length);
}

void SVGTextLayoutAttributes::PositioningLists::appendEmptyValues()
{
    xValues.append(SVGTextLayoutAttributes::emptyValue());
    yValues.append(SVGTextLayoutAttributes::emptyValue());
    dxValues.append(SVGTextLayoutAttributes::emptyValue());
    dyValues.append(SVGTextLayoutAttributes::emptyValue());
    rotateValues.append(SVGTextLayoutAttributes::emptyValue());
}

static inline float safeValueAtPosition(const Vector<float>& values, unsigned position)
{
    return position < values.size() ? values[position] : SVGTextLayoutAttributes::emptyValue();
}

void SVGTextLayoutAttributes::PositioningLists::appendValuesFromPosition(const PositioningLists& source, unsigned position)
{
    xValues.append(safeValueAtPosition(source.xValues, position));
    yValues.append(safeValueAtPosition(source.yValues, position));
    dxValues.append(safeValueAtPosition(source.dxValues, position));
    dyValues.append(safeValueAtPosition(source.dyValues, position));
    rotateValues.append(safeValueAtPosition(source.rotateValues, position));
}

float SVGTextLayoutAttributes::emptyValue()
{
    static float s_emptyValue = std::numeric_limits<float>::max() - 1;
    return s_emptyValue;
}

static inline void dumpLayoutVector(const Vector<float>& values)
{
    if (values.isEmpty()) {
        fprintf(stderr, "empty");
        return;
    }

    unsigned size = values.size();
    for (unsigned i = 0; i < size; ++i) {
        float value = values.at(i);
        if (value == SVGTextLayoutAttributes::emptyValue())
            fprintf(stderr, "x ");
        else
            fprintf(stderr, "%lf ", value);
    }
}

void SVGTextLayoutAttributes::dump() const
{
    fprintf(stderr, "context: %p\n", m_context);

    fprintf(stderr, "x values: ");
    dumpLayoutVector(m_positioningLists.xValues);
    fprintf(stderr, "\n");

    fprintf(stderr, "y values: ");
    dumpLayoutVector(m_positioningLists.yValues);
    fprintf(stderr, "\n");

    fprintf(stderr, "dx values: ");
    dumpLayoutVector(m_positioningLists.dxValues);
    fprintf(stderr, "\n");

    fprintf(stderr, "dy values: ");
    dumpLayoutVector(m_positioningLists.dyValues);
    fprintf(stderr, "\n");

    fprintf(stderr, "rotate values: ");
    dumpLayoutVector(m_positioningLists.rotateValues);
    fprintf(stderr, "\n");

    fprintf(stderr, "character data values:\n");
    unsigned textMetricsSize = m_textMetricsValues.size();
    for (unsigned i = 0; i < textMetricsSize; ++i) {
        const SVGTextMetrics& metrics = m_textMetricsValues.at(i);
        fprintf(stderr, "| {length=%i, glyphName='%s', unicodeString='%s', width=%lf, height=%lf}\n",
                metrics.length(), metrics.glyph().name.utf8().data(), metrics.glyph().unicodeString.utf8().data(), metrics.width(), metrics.height());
    }
    fprintf(stderr, "\n");
}

}

#endif // ENABLE(SVG)
