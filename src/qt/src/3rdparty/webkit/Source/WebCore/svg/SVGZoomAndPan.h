/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGZoomAndPan_h
#define SVGZoomAndPan_h

#if ENABLE(SVG)

#include <wtf/unicode/Unicode.h>

namespace WebCore {

class Attribute;
class QualifiedName;

class SVGZoomAndPan {
public:
    enum SVGZoomAndPanType {
        SVG_ZOOMANDPAN_UNKNOWN = 0,
        SVG_ZOOMANDPAN_DISABLE = 1,
        SVG_ZOOMANDPAN_MAGNIFY = 2
    };

    SVGZoomAndPan() : m_zoomAndPan(SVG_ZOOMANDPAN_MAGNIFY) { }
    virtual ~SVGZoomAndPan() { }

    unsigned short zoomAndPan() const { return m_zoomAndPan; }
    virtual void setZoomAndPan(unsigned short zoomAndPan);

    bool parseMappedAttribute(Attribute*);
    bool isKnownAttribute(const QualifiedName&);

    bool parseZoomAndPan(const UChar*& start, const UChar* end);

private:
    unsigned short m_zoomAndPan;
};

} // namespace WebCore

#endif // ENABLE(SVG)

#endif // SVGZoomAndPan_h
