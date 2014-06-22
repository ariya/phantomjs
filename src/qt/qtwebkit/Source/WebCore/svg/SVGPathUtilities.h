/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
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

#ifndef SVGPathUtilities_h
#define SVGPathUtilities_h

#if ENABLE(SVG)
#include "SVGPathConsumer.h"
#include "SVGPoint.h"
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Path;
class SVGPathByteStream;
class SVGPathElement;
class SVGPathSeg;
class SVGPathSegList;

// String/SVGPathByteStream -> Path
bool buildPathFromString(const String&, Path&);
bool buildPathFromByteStream(SVGPathByteStream*, Path&);

// SVGPathSegList/String -> SVGPathByteStream
bool buildSVGPathByteStreamFromSVGPathSegList(const SVGPathSegList&, SVGPathByteStream*, PathParsingMode);
bool appendSVGPathByteStreamFromSVGPathSeg(PassRefPtr<SVGPathSeg>, SVGPathByteStream*, PathParsingMode);
bool buildSVGPathByteStreamFromString(const String&, SVGPathByteStream*, PathParsingMode);

// SVGPathByteStream/SVGPathSegList -> String
bool buildStringFromByteStream(SVGPathByteStream*, String&, PathParsingMode);
bool buildStringFromSVGPathSegList(const SVGPathSegList&, String&, PathParsingMode);

// SVGPathByteStream -> SVGPathSegList
bool buildSVGPathSegListFromByteStream(SVGPathByteStream*, SVGPathElement*, SVGPathSegList&, PathParsingMode);

bool buildAnimatedSVGPathByteStream(SVGPathByteStream*, SVGPathByteStream*, SVGPathByteStream*, float);
bool addToSVGPathByteStream(SVGPathByteStream*, SVGPathByteStream*, unsigned repeatCount = 1);

bool getSVGPathSegAtLengthFromSVGPathByteStream(SVGPathByteStream*, float length, unsigned& pathSeg);
bool getTotalLengthOfSVGPathByteStream(SVGPathByteStream*, float& totalLength);
bool getPointAtLengthOfSVGPathByteStream(SVGPathByteStream*, float length, SVGPoint&);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathUtilities_h
