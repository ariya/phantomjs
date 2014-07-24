/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "SVGLength.h"

#include "CSSHelper.h"
#include "CSSPrimitiveValue.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "FloatConversion.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Helper functions
static inline unsigned int storeUnit(SVGLengthMode mode, SVGLengthType type)
{
    return (mode << 4) | type;
}

static inline SVGLengthMode extractMode(unsigned int unit)
{
    unsigned int mode = unit >> 4;    
    return static_cast<SVGLengthMode>(mode);
}

static inline SVGLengthType extractType(unsigned int unit)
{
    unsigned int mode = unit >> 4;
    unsigned int type = unit ^ (mode << 4);
    return static_cast<SVGLengthType>(type);
}

static inline String lengthTypeToString(SVGLengthType type)
{
    switch (type) {
    case LengthTypeUnknown:
    case LengthTypeNumber:
        return "";    
    case LengthTypePercentage:
        return "%";
    case LengthTypeEMS:
        return "em";
    case LengthTypeEXS:
        return "ex";
    case LengthTypePX:
        return "px";
    case LengthTypeCM:
        return "cm";
    case LengthTypeMM:
        return "mm";
    case LengthTypeIN:
        return "in";
    case LengthTypePT:
        return "pt";
    case LengthTypePC:
        return "pc";
    }

    ASSERT_NOT_REACHED();
    return String();
}

inline SVGLengthType stringToLengthType(const UChar*& ptr, const UChar* end)
{
    if (ptr == end)
        return LengthTypeNumber;

    const UChar firstChar = *ptr;

    if (++ptr == end)
        return firstChar == '%' ? LengthTypePercentage : LengthTypeUnknown;

    const UChar secondChar = *ptr;

    if (++ptr != end)
        return LengthTypeUnknown;

    if (firstChar == 'e' && secondChar == 'm')
        return LengthTypeEMS;
    if (firstChar == 'e' && secondChar == 'x')
        return LengthTypeEXS;
    if (firstChar == 'p' && secondChar == 'x')
        return LengthTypePX;
    if (firstChar == 'c' && secondChar == 'm')
        return LengthTypeCM;
    if (firstChar == 'm' && secondChar == 'm')
        return LengthTypeMM;
    if (firstChar == 'i' && secondChar == 'n')
        return LengthTypeIN;
    if (firstChar == 'p' && secondChar == 't')
        return LengthTypePT;
    if (firstChar == 'p' && secondChar == 'c')
        return LengthTypePC;

    return LengthTypeUnknown;
}

SVGLength::SVGLength(SVGLengthMode mode, const String& valueAsString)
    : m_valueInSpecifiedUnits(0)
    , m_unit(storeUnit(mode, LengthTypeNumber))
{
    setValueAsString(valueAsString, IGNORE_EXCEPTION);
}

SVGLength::SVGLength(const SVGLengthContext& context, float value, SVGLengthMode mode, SVGLengthType unitType)
    : m_valueInSpecifiedUnits(0)
    , m_unit(storeUnit(mode, unitType))
{
    setValue(value, context, ASSERT_NO_EXCEPTION);
}

SVGLength::SVGLength(const SVGLength& other)
    : m_valueInSpecifiedUnits(other.m_valueInSpecifiedUnits)
    , m_unit(other.m_unit)
{
}

void SVGLength::setValueAsString(const String& valueAsString, SVGLengthMode mode, ExceptionCode& ec)
{
    m_valueInSpecifiedUnits = 0;
    m_unit = storeUnit(mode, LengthTypeNumber);
    setValueAsString(valueAsString, ec);
}

bool SVGLength::operator==(const SVGLength& other) const
{
    return m_unit == other.m_unit
        && m_valueInSpecifiedUnits == other.m_valueInSpecifiedUnits;
}

bool SVGLength::operator!=(const SVGLength& other) const
{
    return !operator==(other);
}

SVGLength SVGLength::construct(SVGLengthMode mode, const String& valueAsString, SVGParsingError& parseError, SVGLengthNegativeValuesMode negativeValuesMode)
{
    ExceptionCode ec = 0;
    SVGLength length(mode);

    length.setValueAsString(valueAsString, ec);

    if (ec)
        parseError = ParsingAttributeFailedError;
    else if (negativeValuesMode == ForbidNegativeLengths && length.valueInSpecifiedUnits() < 0)
        parseError = NegativeValueForbiddenError;

    return length;
}

SVGLengthType SVGLength::unitType() const
{
    return extractType(m_unit);
}

SVGLengthMode SVGLength::unitMode() const
{
    return extractMode(m_unit);
}

float SVGLength::value(const SVGLengthContext& context) const
{
    return value(context, IGNORE_EXCEPTION);
}

float SVGLength::value(const SVGLengthContext& context, ExceptionCode& ec) const
{
    return context.convertValueToUserUnits(m_valueInSpecifiedUnits, extractMode(m_unit), extractType(m_unit), ec);
}

void SVGLength::setValue(const SVGLengthContext& context, float value, SVGLengthMode mode, SVGLengthType unitType, ExceptionCode& ec)
{
    m_unit = storeUnit(mode, unitType);
    setValue(value, context, ec);
}

void SVGLength::setValue(float value, const SVGLengthContext& context, ExceptionCode& ec)
{
    // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
    if (extractType(m_unit) == LengthTypePercentage)
        value = value / 100;

    ec = 0;
    float convertedValue = context.convertValueFromUserUnits(value, extractMode(m_unit), extractType(m_unit), ec);
    if (!ec)
        m_valueInSpecifiedUnits = convertedValue;
}
float SVGLength::valueAsPercentage() const
{
    // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
    if (extractType(m_unit) == LengthTypePercentage)
        return m_valueInSpecifiedUnits / 100;

    return m_valueInSpecifiedUnits;
}

void SVGLength::setValueAsString(const String& string, ExceptionCode& ec)
{
    if (string.isEmpty())
        return;

    float convertedNumber = 0;
    const UChar* ptr = string.characters();
    const UChar* end = ptr + string.length();

    if (!parseNumber(ptr, end, convertedNumber, false)) {
        ec = SYNTAX_ERR;
        return;
    }

    SVGLengthType type = stringToLengthType(ptr, end);
    ASSERT(ptr <= end);
    if (type == LengthTypeUnknown) {
        ec = SYNTAX_ERR;
        return;
    }

    m_unit = storeUnit(extractMode(m_unit), type);
    m_valueInSpecifiedUnits = convertedNumber;
}

String SVGLength::valueAsString() const
{
    return String::number(m_valueInSpecifiedUnits) + lengthTypeToString(extractType(m_unit));
}

void SVGLength::newValueSpecifiedUnits(unsigned short type, float value, ExceptionCode& ec)
{
    if (type == LengthTypeUnknown || type > LengthTypePC) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_unit = storeUnit(extractMode(m_unit), static_cast<SVGLengthType>(type));
    m_valueInSpecifiedUnits = value;
}

void SVGLength::convertToSpecifiedUnits(unsigned short type, const SVGLengthContext& context, ExceptionCode& ec)
{
    if (type == LengthTypeUnknown || type > LengthTypePC) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    float valueInUserUnits = value(context, ec);
    if (ec)
        return;

    unsigned int originalUnitAndType = m_unit;    
    m_unit = storeUnit(extractMode(m_unit), static_cast<SVGLengthType>(type));
    setValue(valueInUserUnits, context, ec);
    if (!ec)
        return;

    // Eventually restore old unit and type
    m_unit = originalUnitAndType;
}

SVGLength SVGLength::fromCSSPrimitiveValue(CSSPrimitiveValue* value)
{
    ASSERT(value);

    SVGLengthType svgType;
    switch (value->primitiveType()) {
    case CSSPrimitiveValue::CSS_NUMBER:
        svgType = LengthTypeNumber;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        svgType = LengthTypePercentage;
        break;
    case CSSPrimitiveValue::CSS_EMS:
        svgType = LengthTypeEMS;
        break;
    case CSSPrimitiveValue::CSS_EXS:
        svgType = LengthTypeEXS;
        break;
    case CSSPrimitiveValue::CSS_PX:
        svgType = LengthTypePX;
        break;
    case CSSPrimitiveValue::CSS_CM:
        svgType = LengthTypeCM;
        break;
    case CSSPrimitiveValue::CSS_MM:
        svgType = LengthTypeMM;
        break;
    case CSSPrimitiveValue::CSS_IN:
        svgType = LengthTypeIN;
        break;
    case CSSPrimitiveValue::CSS_PT:
        svgType = LengthTypePT;
        break;
    case CSSPrimitiveValue::CSS_PC:
        svgType = LengthTypePC;
        break;
    case CSSPrimitiveValue::CSS_UNKNOWN:
    default:
        svgType = LengthTypeUnknown;
        break;
    };

    if (svgType == LengthTypeUnknown)
        return SVGLength();

    ExceptionCode ec = 0;
    SVGLength length;
    length.newValueSpecifiedUnits(svgType, value->getFloatValue(), ec);
    if (ec)    
        return SVGLength();

    return length;
}

PassRefPtr<CSSPrimitiveValue> SVGLength::toCSSPrimitiveValue(const SVGLength& length)
{
    CSSPrimitiveValue::UnitTypes cssType = CSSPrimitiveValue::CSS_UNKNOWN;
    switch (length.unitType()) {
    case LengthTypeUnknown:
        break;
    case LengthTypeNumber:
        cssType = CSSPrimitiveValue::CSS_NUMBER;
        break;
    case LengthTypePercentage:
        cssType = CSSPrimitiveValue::CSS_PERCENTAGE;
        break;
    case LengthTypeEMS:
        cssType = CSSPrimitiveValue::CSS_EMS;
        break;
    case LengthTypeEXS:
        cssType = CSSPrimitiveValue::CSS_EXS;
        break;
    case LengthTypePX:
        cssType = CSSPrimitiveValue::CSS_PX;
        break;
    case LengthTypeCM:
        cssType = CSSPrimitiveValue::CSS_CM;
        break;
    case LengthTypeMM:
        cssType = CSSPrimitiveValue::CSS_MM;
        break;
    case LengthTypeIN:
        cssType = CSSPrimitiveValue::CSS_IN;
        break;
    case LengthTypePT:
        cssType = CSSPrimitiveValue::CSS_PT;
        break;
    case LengthTypePC:
        cssType = CSSPrimitiveValue::CSS_PC;
        break;
    };

    return CSSPrimitiveValue::create(length.valueInSpecifiedUnits(), cssType);
}

SVGLengthMode SVGLength::lengthModeForAnimatedLengthAttribute(const QualifiedName& attrName)
{
    typedef HashMap<QualifiedName, SVGLengthMode> LengthModeForLengthAttributeMap;
    DEFINE_STATIC_LOCAL(LengthModeForLengthAttributeMap, s_lengthModeMap, ());
    
    if (s_lengthModeMap.isEmpty()) {
        s_lengthModeMap.set(SVGNames::xAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::yAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::cxAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::cyAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::dxAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::dyAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::fxAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::fyAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::rAttr, LengthModeOther);
        s_lengthModeMap.set(SVGNames::widthAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::heightAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::x1Attr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::x2Attr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::y1Attr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::y2Attr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::refXAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::refYAttr, LengthModeHeight);
        s_lengthModeMap.set(SVGNames::markerWidthAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::markerHeightAttr, LengthModeHeight);        
        s_lengthModeMap.set(SVGNames::textLengthAttr, LengthModeWidth);
        s_lengthModeMap.set(SVGNames::startOffsetAttr, LengthModeWidth);
    }
    
    if (s_lengthModeMap.contains(attrName))
        return s_lengthModeMap.get(attrName);
    
    return LengthModeOther;
}

}

#endif
