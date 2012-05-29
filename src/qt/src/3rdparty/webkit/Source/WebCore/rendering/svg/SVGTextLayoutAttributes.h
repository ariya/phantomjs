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

#ifndef SVGTextLayoutAttributes_h
#define SVGTextLayoutAttributes_h

#if ENABLE(SVG)
#include "SVGTextMetrics.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class RenderSVGInlineText;

class SVGTextLayoutAttributes {
public:
    struct PositioningLists {
        void fillWithEmptyValues(unsigned length);
        void appendEmptyValues();
        void appendValuesFromPosition(const PositioningLists&, unsigned position);

        Vector<float> xValues;
        Vector<float> yValues;
        Vector<float> dxValues;
        Vector<float> dyValues;
        Vector<float> rotateValues;
    };

    SVGTextLayoutAttributes(RenderSVGInlineText* context = 0);

    void reserveCapacity(unsigned length);
    void dump() const;

    static float emptyValue();

    RenderSVGInlineText* context() const { return m_context; }

    PositioningLists& positioningLists() { return m_positioningLists; }
    const PositioningLists& positioningLists() const { return m_positioningLists; }

    Vector<float>& xValues() { return m_positioningLists.xValues; }
    const Vector<float>& xValues() const { return m_positioningLists.xValues; }

    Vector<float>& yValues() { return m_positioningLists.yValues; }
    const Vector<float>& yValues() const { return m_positioningLists.yValues; }

    Vector<float>& dxValues() { return m_positioningLists.dxValues; }
    const Vector<float>& dxValues() const { return m_positioningLists.dxValues; }

    Vector<float>& dyValues() { return m_positioningLists.dyValues; }
    const Vector<float>& dyValues() const { return m_positioningLists.dyValues; }

    Vector<float>& rotateValues() { return m_positioningLists.rotateValues; }
    const Vector<float>& rotateValues() const { return m_positioningLists.rotateValues; }

    Vector<SVGTextMetrics>& textMetricsValues() { return m_textMetricsValues; }
    const Vector<SVGTextMetrics>& textMetricsValues() const { return m_textMetricsValues; }

private:
    RenderSVGInlineText* m_context;
    PositioningLists m_positioningLists;
    Vector<SVGTextMetrics> m_textMetricsValues;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
