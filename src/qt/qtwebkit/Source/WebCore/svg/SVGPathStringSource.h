/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef SVGPathStringSource_h
#define SVGPathStringSource_h

#if ENABLE(SVG)
#include "SVGPathSource.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SVGPathStringSource : public SVGPathSource {
public:
    static PassOwnPtr<SVGPathStringSource> create(const String& string)
    {
        return adoptPtr(new SVGPathStringSource(string));
    }

private:
    SVGPathStringSource(const String&);

    virtual bool hasMoreData() const;
    virtual bool moveToNextToken();
    virtual bool parseSVGSegmentType(SVGPathSegType&);
    virtual SVGPathSegType nextCommand(SVGPathSegType previousCommand);

    virtual bool parseMoveToSegment(FloatPoint&);
    virtual bool parseLineToSegment(FloatPoint&);
    virtual bool parseLineToHorizontalSegment(float&);
    virtual bool parseLineToVerticalSegment(float&);
    virtual bool parseCurveToCubicSegment(FloatPoint&, FloatPoint&, FloatPoint&);
    virtual bool parseCurveToCubicSmoothSegment(FloatPoint&, FloatPoint&);
    virtual bool parseCurveToQuadraticSegment(FloatPoint&, FloatPoint&);
    virtual bool parseCurveToQuadraticSmoothSegment(FloatPoint&);
    virtual bool parseArcToSegment(float&, float&, float&, bool&, bool&, FloatPoint&);

    String m_string;
    bool m_is8BitSource;

    union {
        const LChar* m_character8;
        const UChar* m_character16;
    } m_current;
    union {
        const LChar* m_character8;
        const UChar* m_character16;
    } m_end;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathStringSource_h
