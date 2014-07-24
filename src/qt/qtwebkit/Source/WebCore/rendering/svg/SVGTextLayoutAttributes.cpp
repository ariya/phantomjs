/*
 * Copyright (C) Research In Motion Limited 2010-11. All rights reserved.
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

void SVGTextLayoutAttributes::clear()
{
    m_characterDataMap.clear();
    m_textMetricsValues.clear();
}

float SVGTextLayoutAttributes::emptyValue()
{
    static float s_emptyValue = std::numeric_limits<float>::max() - 1;
    return s_emptyValue;
}

static inline void dumpSVGCharacterDataMapValue(const char* identifier, float value, bool appendSpace = true)
{
    if (value == SVGTextLayoutAttributes::emptyValue()) {
        fprintf(stderr, "%s=x", identifier);
        if (appendSpace)
            fprintf(stderr, " ");
        return;
    }
    fprintf(stderr, "%s=%lf", identifier, value);
    if (appendSpace)
        fprintf(stderr, " ");
}

void SVGTextLayoutAttributes::dump() const
{
    fprintf(stderr, "context: %p\n", m_context);
    const SVGCharacterDataMap::const_iterator end = m_characterDataMap.end();
    for (SVGCharacterDataMap::const_iterator it = m_characterDataMap.begin(); it != end; ++it) {
        const SVGCharacterData& data = it->value;
        fprintf(stderr, " ---> pos=%i, data={", it->key);
        dumpSVGCharacterDataMapValue("x", data.x);
        dumpSVGCharacterDataMapValue("y", data.y);
        dumpSVGCharacterDataMapValue("dx", data.dx);
        dumpSVGCharacterDataMapValue("dy", data.dy);
        dumpSVGCharacterDataMapValue("rotate", data.rotate, false);
        fprintf(stderr, "}\n");
    }
}

}

#endif // ENABLE(SVG)
