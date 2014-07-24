/*
 * Copyright (C) 2002, 2003 The Karbon Developers
 * Copyright (C) 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGParserUtilities_h
#define SVGParserUtilities_h
#if ENABLE(SVG)

#include "ParserUtilities.h"
#include <wtf/HashSet.h>

typedef pair<unsigned, unsigned> UnicodeRange;
typedef Vector<UnicodeRange> UnicodeRanges;

namespace WebCore {

class FloatPoint;
class FloatRect;
class SVGPointList;

template <typename CharacterType>
bool parseSVGNumber(CharacterType* ptr, size_t length, double& number);
bool parseNumber(const LChar*& ptr, const LChar* end, float& number, bool skip = true);
bool parseNumber(const UChar*& ptr, const UChar* end, float& number, bool skip = true);
bool parseNumberFromString(const String&, float& number, bool skip = true);
bool parseNumberOptionalNumber(const String& s, float& h, float& v);
bool parseArcFlag(const LChar*& ptr, const LChar* end, bool& flag);
bool parseArcFlag(const UChar*& ptr, const UChar* end, bool& flag);
bool parseRect(const String&, FloatRect&);

template <typename CharacterType>
bool parseFloatPoint(const CharacterType*& current, const CharacterType* end, FloatPoint&);
template <typename CharacterType>
bool parseFloatPoint2(const CharacterType*& current, const CharacterType* end, FloatPoint&, FloatPoint&);
template <typename CharacterType>
bool parseFloatPoint3(const CharacterType*& current, const CharacterType* end, FloatPoint&, FloatPoint&, FloatPoint&);

// SVG allows several different whitespace characters:
// http://www.w3.org/TR/SVG/paths.html#PathDataBNF
template <typename CharacterType>
inline bool isSVGSpace(CharacterType c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

template <typename CharacterType>
inline bool skipOptionalSVGSpaces(const CharacterType*& ptr, const CharacterType* end)
{
    while (ptr < end && isSVGSpace(*ptr))
        ptr++;
    return ptr < end;
}

template <typename CharacterType>
inline bool skipOptionalSVGSpacesOrDelimiter(const CharacterType*& ptr, const CharacterType* end, char delimiter = ',')
{
    if (ptr < end && !isSVGSpace(*ptr) && *ptr != delimiter)
        return false;
    if (skipOptionalSVGSpaces(ptr, end)) {
        if (ptr < end && *ptr == delimiter) {
            ptr++;
            skipOptionalSVGSpaces(ptr, end);
        }
    }
    return ptr < end;
}

bool pointsListFromSVGData(SVGPointList& pointsList, const String& points);
Vector<String> parseDelimitedString(const String& input, const char seperator);
bool parseKerningUnicodeString(const String& input, UnicodeRanges&, HashSet<String>& stringList);
bool parseGlyphName(const String& input, HashSet<String>& values);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGParserUtilities_h
