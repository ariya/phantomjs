/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2012, 2013 Apple Inc. All rights reserved.
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
#include "CSSPrimitiveValue.h"

#include "CSSBasicShapes.h"
#include "CSSCalculationValue.h"
#include "CSSHelper.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "CalculationValue.h"
#include "Color.h"
#include "Counter.h"
#include "ExceptionCode.h"
#include "Font.h"
#include "LayoutUnit.h"
#include "Node.h"
#include "Pair.h"
#include "RGBColor.h"
#include "Rect.h"
#include "RenderStyle.h"
#include "StyleSheetContents.h"
#include <wtf/ASCIICType.h>
#include <wtf/DecimalNumber.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuffer.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

using namespace WTF;

namespace WebCore {
    
// Max/min values for CSS, needs to slightly smaller/larger than the true max/min values to allow for rounding without overflowing.
// Subtract two (rather than one) to allow for values to be converted to float and back without exceeding the LayoutUnit::max.
const int maxValueForCssLength = INT_MAX / kFixedPointDenominator - 2;
const int minValueForCssLength = INT_MIN / kFixedPointDenominator + 2;

static inline bool isValidCSSUnitTypeForDoubleConversion(CSSPrimitiveValue::UnitTypes unitType)
{
    switch (unitType) {
    case CSSPrimitiveValue::CSS_CALC:
    case CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_NUMBER:
    case CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_LENGTH:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_DEG:
    case CSSPrimitiveValue::CSS_DIMENSION:
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSSPrimitiveValue::CSS_DPPX:
    case CSSPrimitiveValue::CSS_DPI:
    case CSSPrimitiveValue::CSS_DPCM:
#endif
    case CSSPrimitiveValue::CSS_EMS:
    case CSSPrimitiveValue::CSS_EXS:
    case CSSPrimitiveValue::CSS_GRAD:
    case CSSPrimitiveValue::CSS_HZ:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_KHZ:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_MS:
    case CSSPrimitiveValue::CSS_NUMBER:
    case CSSPrimitiveValue::CSS_PERCENTAGE:
    case CSSPrimitiveValue::CSS_PC:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_RAD:
    case CSSPrimitiveValue::CSS_REMS:
    case CSSPrimitiveValue::CSS_CHS:
    case CSSPrimitiveValue::CSS_S:
    case CSSPrimitiveValue::CSS_TURN:
    case CSSPrimitiveValue::CSS_VW:
    case CSSPrimitiveValue::CSS_VH:
    case CSSPrimitiveValue::CSS_VMIN:
    case CSSPrimitiveValue::CSS_VMAX:
        return true;
    case CSSPrimitiveValue::CSS_ATTR:
    case CSSPrimitiveValue::CSS_COUNTER:
    case CSSPrimitiveValue::CSS_COUNTER_NAME:
#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPrimitiveValue::CSS_DASHBOARD_REGION:
#endif
#if !ENABLE(CSS_IMAGE_RESOLUTION) && !ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSSPrimitiveValue::CSS_DPPX:
    case CSSPrimitiveValue::CSS_DPI:
    case CSSPrimitiveValue::CSS_DPCM:
#endif
    case CSSPrimitiveValue::CSS_IDENT:
    case CSSPrimitiveValue::CSS_PROPERTY_ID:
    case CSSPrimitiveValue::CSS_VALUE_ID:
    case CSSPrimitiveValue::CSS_PAIR:
    case CSSPrimitiveValue::CSS_PARSER_HEXCOLOR:
    case CSSPrimitiveValue::CSS_PARSER_IDENTIFIER:
    case CSSPrimitiveValue::CSS_PARSER_INTEGER:
    case CSSPrimitiveValue::CSS_PARSER_OPERATOR:
    case CSSPrimitiveValue::CSS_RECT:
    case CSSPrimitiveValue::CSS_QUAD:
    case CSSPrimitiveValue::CSS_RGBCOLOR:
    case CSSPrimitiveValue::CSS_SHAPE:
    case CSSPrimitiveValue::CSS_STRING:
    case CSSPrimitiveValue::CSS_UNICODE_RANGE:
    case CSSPrimitiveValue::CSS_UNKNOWN:
    case CSSPrimitiveValue::CSS_URI:
#if ENABLE(CSS_VARIABLES)
    case CSSPrimitiveValue::CSS_VARIABLE_NAME:
#endif
        return false;
    }

    ASSERT_NOT_REACHED();
    return false;
}

static CSSPrimitiveValue::UnitCategory unitCategory(CSSPrimitiveValue::UnitTypes type)
{
    // Here we violate the spec (http://www.w3.org/TR/DOM-Level-2-Style/css.html#CSS-CSSPrimitiveValue) and allow conversions
    // between CSS_PX and relative lengths (see cssPixelsPerInch comment in CSSHelper.h for the topic treatment).
    switch (type) {
    case CSSPrimitiveValue::CSS_NUMBER:
        return CSSPrimitiveValue::UNumber;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        return CSSPrimitiveValue::UPercent;
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PC:
        return CSSPrimitiveValue::ULength;
    case CSSPrimitiveValue::CSS_MS:
    case CSSPrimitiveValue::CSS_S:
        return CSSPrimitiveValue::UTime;
    case CSSPrimitiveValue::CSS_DEG:
    case CSSPrimitiveValue::CSS_RAD:
    case CSSPrimitiveValue::CSS_GRAD:
    case CSSPrimitiveValue::CSS_TURN:
        return CSSPrimitiveValue::UAngle;
    case CSSPrimitiveValue::CSS_HZ:
    case CSSPrimitiveValue::CSS_KHZ:
        return CSSPrimitiveValue::UFrequency;
    case CSSPrimitiveValue::CSS_VW:
    case CSSPrimitiveValue::CSS_VH:
    case CSSPrimitiveValue::CSS_VMIN:
    case CSSPrimitiveValue::CSS_VMAX:
        return CSSPrimitiveValue::UViewportPercentageLength;
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSSPrimitiveValue::CSS_DPPX:
    case CSSPrimitiveValue::CSS_DPI:
    case CSSPrimitiveValue::CSS_DPCM:
        return CSSPrimitiveValue::UResolution;
#endif
    default:
        return CSSPrimitiveValue::UOther;
    }
}

typedef HashMap<const CSSPrimitiveValue*, String> CSSTextCache;
static CSSTextCache& cssTextCache()
{
    DEFINE_STATIC_LOCAL(CSSTextCache, cache, ());
    return cache;
}

unsigned short CSSPrimitiveValue::primitiveType() const 
{
    if (m_primitiveUnitType == CSS_PROPERTY_ID || m_primitiveUnitType == CSS_VALUE_ID)
        return CSS_IDENT;

    if (m_primitiveUnitType != CSSPrimitiveValue::CSS_CALC)
        return m_primitiveUnitType; 
    
    switch (m_value.calc->category()) {
    case CalcNumber:
        return CSSPrimitiveValue::CSS_NUMBER;
    case CalcPercent:
        return CSSPrimitiveValue::CSS_PERCENTAGE;
    case CalcLength:
        return CSSPrimitiveValue::CSS_PX;
    case CalcPercentNumber:
        return CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_NUMBER;
    case CalcPercentLength:
        return CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_LENGTH;
#if ENABLE(CSS_VARIABLES)
    case CalcVariable:
        return CSSPrimitiveValue::CSS_UNKNOWN; // The type of a calculation containing a variable cannot be known until the value of the variable is determined.
#endif
    case CalcOther:
        return CSSPrimitiveValue::CSS_UNKNOWN;
    }
    return CSSPrimitiveValue::CSS_UNKNOWN;
}

static const AtomicString& propertyName(CSSPropertyID propertyID)
{
    ASSERT_ARG(propertyID, propertyID >= 0);
    ASSERT_ARG(propertyID, (propertyID >= firstCSSProperty && propertyID < firstCSSProperty + numCSSProperties));

    if (propertyID < 0)
        return nullAtom;

    return getPropertyNameAtomicString(propertyID);
}

static const AtomicString& valueName(CSSValueID valueID)
{
    ASSERT_ARG(valueID, valueID >= 0);
    ASSERT_ARG(valueID, valueID < numCSSValueKeywords);

    if (valueID < 0)
        return nullAtom;

    static AtomicString* keywordStrings = new AtomicString[numCSSValueKeywords]; // Leaked intentionally.
    AtomicString& keywordString = keywordStrings[valueID];
    if (keywordString.isNull())
        keywordString = getValueName(valueID);
    return keywordString;
}

CSSPrimitiveValue::CSSPrimitiveValue(CSSValueID valueID)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_VALUE_ID;
    m_value.valueID = valueID;
}

CSSPrimitiveValue::CSSPrimitiveValue(CSSPropertyID propertyID)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_PROPERTY_ID;
    m_value.propertyID = propertyID;
}

CSSPrimitiveValue::CSSPrimitiveValue(int parserOperator)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_PARSER_OPERATOR;
    m_value.parserOperator = parserOperator;
}

CSSPrimitiveValue::CSSPrimitiveValue(double num, UnitTypes type)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = type;
    ASSERT(std::isfinite(num));
    m_value.num = num;
}

CSSPrimitiveValue::CSSPrimitiveValue(const String& str, UnitTypes type)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = type;
    if ((m_value.string = str.impl()))
        m_value.string->ref();
}


CSSPrimitiveValue::CSSPrimitiveValue(RGBA32 color)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_RGBCOLOR;
    m_value.rgbcolor = color;
}

CSSPrimitiveValue::CSSPrimitiveValue(const Length& length)
    : CSSValue(PrimitiveClass)
{
    switch (length.type()) {
        case Auto:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueAuto;
            break;
        case WebCore::Fixed:
            m_primitiveUnitType = CSS_PX;
            m_value.num = length.value();
            break;
        case Intrinsic:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueIntrinsic;
            break;
        case MinIntrinsic:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueMinIntrinsic;
            break;
        case MinContent:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueWebkitMinContent;
            break;
        case MaxContent:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueWebkitMaxContent;
            break;
        case FillAvailable:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueWebkitFillAvailable;
            break;
        case FitContent:
            m_primitiveUnitType = CSS_VALUE_ID;
            m_value.valueID = CSSValueWebkitFitContent;
            break;
        case Percent:
            m_primitiveUnitType = CSS_PERCENTAGE;
            ASSERT(std::isfinite(length.percent()));
            m_value.num = length.percent();
            break;
        case ViewportPercentageWidth:
            m_primitiveUnitType = CSS_VW;
            m_value.num = length.viewportPercentageLength();
            break;
        case ViewportPercentageHeight:
            m_primitiveUnitType = CSS_VH;
            m_value.num = length.viewportPercentageLength();
            break;
        case ViewportPercentageMin:
            m_primitiveUnitType = CSS_VMIN;
            m_value.num = length.viewportPercentageLength();
            break;
        case ViewportPercentageMax:
            m_primitiveUnitType = CSS_VMAX;
            m_value.num = length.viewportPercentageLength();
            break;
        case Calculated:
        case Relative:
        case Undefined:
            ASSERT_NOT_REACHED();
            break;
    }
}

void CSSPrimitiveValue::init(PassRefPtr<Counter> c)
{
    m_primitiveUnitType = CSS_COUNTER;
    m_hasCachedCSSText = false;
    m_value.counter = c.leakRef();
}

void CSSPrimitiveValue::init(PassRefPtr<Rect> r)
{
    m_primitiveUnitType = CSS_RECT;
    m_hasCachedCSSText = false;
    m_value.rect = r.leakRef();
}

void CSSPrimitiveValue::init(PassRefPtr<Quad> quad)
{
    m_primitiveUnitType = CSS_QUAD;
    m_hasCachedCSSText = false;
    m_value.quad = quad.leakRef();
}

#if ENABLE(DASHBOARD_SUPPORT)
void CSSPrimitiveValue::init(PassRefPtr<DashboardRegion> r)
{
    m_primitiveUnitType = CSS_DASHBOARD_REGION;
    m_hasCachedCSSText = false;
    m_value.region = r.leakRef();
}
#endif

void CSSPrimitiveValue::init(PassRefPtr<Pair> p)
{
    m_primitiveUnitType = CSS_PAIR;
    m_hasCachedCSSText = false;
    m_value.pair = p.leakRef();
}

void CSSPrimitiveValue::init(PassRefPtr<CSSCalcValue> c)
{
    m_primitiveUnitType = CSS_CALC;
    m_hasCachedCSSText = false;
    m_value.calc = c.leakRef();
}

void CSSPrimitiveValue::init(PassRefPtr<CSSBasicShape> shape)
{
    m_primitiveUnitType = CSS_SHAPE;
    m_hasCachedCSSText = false;
    m_value.shape = shape.leakRef();
}

CSSPrimitiveValue::~CSSPrimitiveValue()
{
    cleanup();
}

void CSSPrimitiveValue::cleanup()
{
    switch (static_cast<UnitTypes>(m_primitiveUnitType)) {
    case CSS_STRING:
    case CSS_URI:
    case CSS_ATTR:
    case CSS_COUNTER_NAME:
#if ENABLE(CSS_VARIABLES)
    case CSS_VARIABLE_NAME:
#endif
    case CSS_PARSER_HEXCOLOR:
        if (m_value.string)
            m_value.string->deref();
        break;
    case CSS_COUNTER:
        m_value.counter->deref();
        break;
    case CSS_RECT:
        m_value.rect->deref();
        break;
    case CSS_QUAD:
        m_value.quad->deref();
        break;
    case CSS_PAIR:
        m_value.pair->deref();
        break;
#if ENABLE(DASHBOARD_SUPPORT)
    case CSS_DASHBOARD_REGION:
        if (m_value.region)
            m_value.region->deref();
        break;
#endif
    case CSS_CALC:
        m_value.calc->deref();
        break;
    case CSS_CALC_PERCENTAGE_WITH_NUMBER:
    case CSS_CALC_PERCENTAGE_WITH_LENGTH:
        ASSERT_NOT_REACHED();
        break;
    case CSS_SHAPE:
        m_value.shape->deref();
        break;
    case CSS_NUMBER:
    case CSS_PARSER_INTEGER:
    case CSS_PERCENTAGE:
    case CSS_EMS:
    case CSS_EXS:
    case CSS_REMS:
    case CSS_CHS:
    case CSS_PX:
    case CSS_CM:
    case CSS_MM:
    case CSS_IN:
    case CSS_PT:
    case CSS_PC:
    case CSS_DEG:
    case CSS_RAD:
    case CSS_GRAD:
    case CSS_MS:
    case CSS_S:
    case CSS_HZ:
    case CSS_KHZ:
    case CSS_TURN:
    case CSS_VW:
    case CSS_VH:
    case CSS_VMIN:
    case CSS_VMAX:
    case CSS_DPPX:
    case CSS_DPI:
    case CSS_DPCM:
    case CSS_IDENT:
    case CSS_RGBCOLOR:
    case CSS_DIMENSION:
    case CSS_UNKNOWN:
    case CSS_UNICODE_RANGE:
    case CSS_PARSER_OPERATOR:
    case CSS_PARSER_IDENTIFIER:
    case CSS_PROPERTY_ID:
    case CSS_VALUE_ID:
        break;
    }
    m_primitiveUnitType = 0;
    if (m_hasCachedCSSText) {
        cssTextCache().remove(this);
        m_hasCachedCSSText = false;
    }
}

double CSSPrimitiveValue::computeDegrees()
{
    switch (m_primitiveUnitType) {
    case CSS_DEG:
        return getDoubleValue();
    case CSS_RAD:
        return rad2deg(getDoubleValue());
    case CSS_GRAD:
        return grad2deg(getDoubleValue());
    case CSS_TURN:
        return turn2deg(getDoubleValue());
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

template<> int CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return roundForImpreciseConversion<int>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

template<> unsigned CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return roundForImpreciseConversion<unsigned>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

template<> Length CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
#if ENABLE(SUBPIXEL_LAYOUT)
    return Length(clampTo<float>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize), minValueForCssLength, maxValueForCssLength), Fixed);
#else
    return Length(clampTo<float>(roundForImpreciseConversion<float>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize)), minValueForCssLength, maxValueForCssLength), Fixed);
#endif
}

template<> short CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return roundForImpreciseConversion<short>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

template<> unsigned short CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return roundForImpreciseConversion<unsigned short>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

template<> float CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return static_cast<float>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

template<> double CSSPrimitiveValue::computeLength(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    return computeLengthDouble(style, rootStyle, multiplier, computingFontSize);
}

double CSSPrimitiveValue::computeLengthDouble(const RenderStyle* style, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const
{
    if (m_primitiveUnitType == CSS_CALC)
        // The multiplier and factor is applied to each value in the calc expression individually
        return m_value.calc->computeLengthPx(style, rootStyle, multiplier, computingFontSize);
        
    double factor;

    switch (primitiveType()) {
        case CSS_EMS:
            factor = computingFontSize ? style->fontDescription().specifiedSize() : style->fontDescription().computedSize();
            break;
        case CSS_EXS:
            // FIXME: We have a bug right now where the zoom will be applied twice to EX units.
            // We really need to compute EX using fontMetrics for the original specifiedSize and not use
            // our actual constructed rendering font.
            if (style->fontMetrics().hasXHeight())
                factor = style->fontMetrics().xHeight();
            else
                factor = (computingFontSize ? style->fontDescription().specifiedSize() : style->fontDescription().computedSize()) / 2.0;
            break;
        case CSS_REMS:
            if (rootStyle)
                factor = computingFontSize ? rootStyle->fontDescription().specifiedSize() : rootStyle->fontDescription().computedSize();
            else
                factor = 1.0;
            break;
        case CSS_CHS:
            factor = style->fontMetrics().zeroWidth();
            break;
        case CSS_PX:
            factor = 1.0;
            break;
        case CSS_CM:
            factor = cssPixelsPerInch / 2.54; // (2.54 cm/in)
            break;
        case CSS_MM:
            factor = cssPixelsPerInch / 25.4;
            break;
        case CSS_IN:
            factor = cssPixelsPerInch;
            break;
        case CSS_PT:
            factor = cssPixelsPerInch / 72.0;
            break;
        case CSS_PC:
            // 1 pc == 12 pt
            factor = cssPixelsPerInch * 12.0 / 72.0;
            break;
        case CSS_CALC_PERCENTAGE_WITH_LENGTH:
        case CSS_CALC_PERCENTAGE_WITH_NUMBER:
            ASSERT_NOT_REACHED();
            return -1.0;
        default:
            ASSERT_NOT_REACHED();
            return -1.0;
    }

    // We do not apply the zoom factor when we are computing the value of the font-size property. The zooming
    // for font sizes is much more complicated, since we have to worry about enforcing the minimum font size preference
    // as well as enforcing the implicit "smart minimum."
    double result = getDoubleValue() * factor;
    if (computingFontSize || isFontRelativeLength())
        return result;

    return result * multiplier;
}

void CSSPrimitiveValue::setFloatValue(unsigned short, double, ExceptionCode& ec)
{
    // Keeping values immutable makes optimizations easier and allows sharing of the primitive value objects.
    // No other engine supports mutating style through this API. Computed style is always read-only anyway.
    // Supporting setter would require making primitive value copy-on-write and taking care of style invalidation.
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

static double conversionToCanonicalUnitsScaleFactor(unsigned short unitType)
{
    double factor = 1.0;
    // FIXME: the switch can be replaced by an array of scale factors.
    switch (unitType) {
        // These are "canonical" units in their respective categories.
        case CSSPrimitiveValue::CSS_PX:
        case CSSPrimitiveValue::CSS_DEG:
        case CSSPrimitiveValue::CSS_MS:
        case CSSPrimitiveValue::CSS_HZ:
            break;
        case CSSPrimitiveValue::CSS_CM:
            factor = cssPixelsPerInch / 2.54; // (2.54 cm/in)
            break;
        case CSSPrimitiveValue::CSS_DPCM:
            factor = 2.54 / cssPixelsPerInch; // (2.54 cm/in)
            break;
        case CSSPrimitiveValue::CSS_MM:
            factor = cssPixelsPerInch / 25.4;
            break;
        case CSSPrimitiveValue::CSS_IN:
            factor = cssPixelsPerInch;
            break;
        case CSSPrimitiveValue::CSS_DPI:
            factor = 1 / cssPixelsPerInch;
            break;
        case CSSPrimitiveValue::CSS_PT:
            factor = cssPixelsPerInch / 72.0;
            break;
        case CSSPrimitiveValue::CSS_PC:
            factor = cssPixelsPerInch * 12.0 / 72.0; // 1 pc == 12 pt
            break;
        case CSSPrimitiveValue::CSS_RAD:
            factor = 180 / piDouble;
            break;
        case CSSPrimitiveValue::CSS_GRAD:
            factor = 0.9;
            break;
        case CSSPrimitiveValue::CSS_TURN:
            factor = 360;
            break;
        case CSSPrimitiveValue::CSS_S:
        case CSSPrimitiveValue::CSS_KHZ:
            factor = 1000;
            break;
        default:
            break;
    }

    return factor;
}

double CSSPrimitiveValue::getDoubleValue(unsigned short unitType, ExceptionCode& ec) const
{
    double result = 0;
    bool success = getDoubleValueInternal(static_cast<UnitTypes>(unitType), &result);
    if (!success) {
        ec = INVALID_ACCESS_ERR;
        return 0.0;
    }

    ec = 0;
    return result;
}

double CSSPrimitiveValue::getDoubleValue(unsigned short unitType) const
{
    double result = 0;
    getDoubleValueInternal(static_cast<UnitTypes>(unitType), &result);
    return result;
}

double CSSPrimitiveValue::getDoubleValue() const
{ 
    return m_primitiveUnitType != CSS_CALC ? m_value.num : m_value.calc->doubleValue();
}

CSSPrimitiveValue::UnitTypes CSSPrimitiveValue::canonicalUnitTypeForCategory(UnitCategory category)
{
    // The canonical unit type is chosen according to the way CSSParser::validUnit() chooses the default unit
    // in each category (based on unitflags).
    switch (category) {
    case UNumber:
        return CSS_NUMBER;
    case ULength:
        return CSS_PX;
    case UPercent:
        return CSS_UNKNOWN; // Cannot convert between numbers and percent.
    case UTime:
        return CSS_MS;
    case UAngle:
        return CSS_DEG;
    case UFrequency:
        return CSS_HZ;
    case UViewportPercentageLength:
        return CSS_UNKNOWN; // Cannot convert between numbers and relative lengths.
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case UResolution:
        return CSS_DPPX;
#endif
    default:
        return CSS_UNKNOWN;
    }
}

bool CSSPrimitiveValue::getDoubleValueInternal(UnitTypes requestedUnitType, double* result) const
{
    if (!isValidCSSUnitTypeForDoubleConversion(static_cast<UnitTypes>(m_primitiveUnitType)) || !isValidCSSUnitTypeForDoubleConversion(requestedUnitType))
        return false;

    UnitTypes sourceUnitType = static_cast<UnitTypes>(primitiveType());
    if (requestedUnitType == sourceUnitType || requestedUnitType == CSS_DIMENSION) {
        *result = getDoubleValue();
        return true;
    }

    UnitCategory sourceCategory = unitCategory(sourceUnitType);
    ASSERT(sourceCategory != UOther);

    UnitTypes targetUnitType = requestedUnitType;
    UnitCategory targetCategory = unitCategory(targetUnitType);
    ASSERT(targetCategory != UOther);

    // Cannot convert between unrelated unit categories if one of them is not UNumber.
    if (sourceCategory != targetCategory && sourceCategory != UNumber && targetCategory != UNumber)
        return false;

    if (targetCategory == UNumber) {
        // We interpret conversion to CSS_NUMBER as conversion to a canonical unit in this value's category.
        targetUnitType = canonicalUnitTypeForCategory(sourceCategory);
        if (targetUnitType == CSS_UNKNOWN)
            return false;
    }

    if (sourceUnitType == CSS_NUMBER) {
        // We interpret conversion from CSS_NUMBER in the same way as CSSParser::validUnit() while using non-strict mode.
        sourceUnitType = canonicalUnitTypeForCategory(targetCategory);
        if (sourceUnitType == CSS_UNKNOWN)
            return false;
    }

    double convertedValue = getDoubleValue();

    // First convert the value from m_primitiveUnitType to canonical type.
    double factor = conversionToCanonicalUnitsScaleFactor(sourceUnitType);
    convertedValue *= factor;

    // Now convert from canonical type to the target unitType.
    factor = conversionToCanonicalUnitsScaleFactor(targetUnitType);
    convertedValue /= factor;

    *result = convertedValue;
    return true;
}

void CSSPrimitiveValue::setStringValue(unsigned short, const String&, ExceptionCode& ec)
{
    // Keeping values immutable makes optimizations easier and allows sharing of the primitive value objects.
    // No other engine supports mutating style through this API. Computed style is always read-only anyway.
    // Supporting setter would require making primitive value copy-on-write and taking care of style invalidation.
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

String CSSPrimitiveValue::getStringValue(ExceptionCode& ec) const
{
    ec = 0;
    switch (m_primitiveUnitType) {
        case CSS_STRING:
        case CSS_ATTR:
        case CSS_URI:
#if ENABLE(CSS_VARIABLES)
        case CSS_VARIABLE_NAME:
#endif
            return m_value.string;
        case CSS_VALUE_ID:
            return valueName(m_value.valueID);
        case CSS_PROPERTY_ID:
            return propertyName(m_value.propertyID);
        default:
            ec = INVALID_ACCESS_ERR;
            break;
    }

    return String();
}

String CSSPrimitiveValue::getStringValue() const
{
    switch (m_primitiveUnitType) {
        case CSS_STRING:
        case CSS_ATTR:
        case CSS_URI:
#if ENABLE(CSS_VARIABLES)
        case CSS_VARIABLE_NAME:
#endif
            return m_value.string;
        case CSS_VALUE_ID:
            return valueName(m_value.valueID);
        case CSS_PROPERTY_ID:
            return propertyName(m_value.propertyID);
        default:
            break;
    }

    return String();
}

Counter* CSSPrimitiveValue::getCounterValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_primitiveUnitType != CSS_COUNTER) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.counter;
}

Rect* CSSPrimitiveValue::getRectValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_primitiveUnitType != CSS_RECT) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.rect;
}

Quad* CSSPrimitiveValue::getQuadValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_primitiveUnitType != CSS_QUAD) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.quad;
}

PassRefPtr<RGBColor> CSSPrimitiveValue::getRGBColorValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_primitiveUnitType != CSS_RGBCOLOR) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    // FIMXE: This should not return a new object for each invocation.
    return RGBColor::create(m_value.rgbcolor);
}

Pair* CSSPrimitiveValue::getPairValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_primitiveUnitType != CSS_PAIR) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.pair;
}

static String formatNumber(double number, const char* suffix, unsigned suffixLength)
{
    DecimalNumber decimal(number);

    StringBuffer<LChar> buffer(decimal.bufferLengthForStringDecimal() + suffixLength);
    unsigned length = decimal.toStringDecimal(buffer.characters(), buffer.length());
    ASSERT(length + suffixLength == buffer.length());

    for (unsigned i = 0; i < suffixLength; ++i)
        buffer[length + i] = static_cast<LChar>(suffix[i]);

    return String::adopt(buffer);
}

template <unsigned characterCount>
ALWAYS_INLINE static String formatNumber(double number, const char (&characters)[characterCount])
{
    return formatNumber(number, characters, characterCount - 1);
}

String CSSPrimitiveValue::customCssText() const
{
    // FIXME: return the original value instead of a generated one (e.g. color
    // name if it was specified) - check what spec says about this

    if (m_hasCachedCSSText) {
        ASSERT(cssTextCache().contains(this));
        return cssTextCache().get(this);
    }

    String text;
    switch (m_primitiveUnitType) {
        case CSS_UNKNOWN:
            // FIXME
            break;
        case CSS_NUMBER:
        case CSS_PARSER_INTEGER:
            text = formatNumber(m_value.num, "");
            break;
        case CSS_PERCENTAGE:
            text = formatNumber(m_value.num, "%");
            break;
        case CSS_EMS:
            text = formatNumber(m_value.num, "em");
            break;
        case CSS_EXS:
            text = formatNumber(m_value.num, "ex");
            break;
        case CSS_REMS:
            text = formatNumber(m_value.num, "rem");
            break;
        case CSS_CHS:
            text = formatNumber(m_value.num, "ch");
            break;
        case CSS_PX:
            text = formatNumber(m_value.num, "px");
            break;
        case CSS_CM:
            text = formatNumber(m_value.num, "cm");
            break;
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
        case CSS_DPPX:
            text = formatNumber(m_value.num, "dppx");
            break;
        case CSS_DPI:
            text = formatNumber(m_value.num, "dpi");
            break;
        case CSS_DPCM:
            text = formatNumber(m_value.num, "dpcm");
            break;
#endif
        case CSS_MM:
            text = formatNumber(m_value.num, "mm");
            break;
        case CSS_IN:
            text = formatNumber(m_value.num, "in");
            break;
        case CSS_PT:
            text = formatNumber(m_value.num, "pt");
            break;
        case CSS_PC:
            text = formatNumber(m_value.num, "pc");
            break;
        case CSS_DEG:
            text = formatNumber(m_value.num, "deg");
            break;
        case CSS_RAD:
            text = formatNumber(m_value.num, "rad");
            break;
        case CSS_GRAD:
            text = formatNumber(m_value.num, "grad");
            break;
        case CSS_MS:
            text = formatNumber(m_value.num, "ms");
            break;
        case CSS_S:
            text = formatNumber(m_value.num, "s");
            break;
        case CSS_HZ:
            text = formatNumber(m_value.num, "hz");
            break;
        case CSS_KHZ:
            text = formatNumber(m_value.num, "khz");
            break;
        case CSS_TURN:
            text = formatNumber(m_value.num, "turn");
            break;
        case CSS_DIMENSION:
            // FIXME
            break;
        case CSS_STRING:
            text = quoteCSSStringIfNeeded(m_value.string);
            break;
        case CSS_URI:
            text = "url(" + quoteCSSURLIfNeeded(m_value.string) + ")";
            break;
        case CSS_VALUE_ID:
            text = valueName(m_value.valueID);
            break;
        case CSS_PROPERTY_ID:
            text = propertyName(m_value.propertyID);
            break;
        case CSS_ATTR: {
            StringBuilder result;
            result.reserveCapacity(6 + m_value.string->length());
            result.appendLiteral("attr(");
            result.append(m_value.string);
            result.append(')');

            text = result.toString();
            break;
        }
        case CSS_COUNTER_NAME:
            text = "counter(" + String(m_value.string) + ')';
            break;
        case CSS_COUNTER: {
            StringBuilder result;
            String separator = m_value.counter->separator();
            if (separator.isEmpty())
                result.appendLiteral("counter(");
            else
                result.appendLiteral("counters(");

            result.append(m_value.counter->identifier());
            if (!separator.isEmpty()) {
                result.appendLiteral(", ");
                result.append(quoteCSSStringIfNeeded(separator));
            }
            String listStyle = m_value.counter->listStyle();
            if (!listStyle.isEmpty()) {
                result.appendLiteral(", ");
                result.append(listStyle);
            }
            result.append(')');

            text = result.toString();
            break;
        }
        case CSS_RECT:
            text = getRectValue()->cssText();
            break;
        case CSS_QUAD:
            text = getQuadValue()->cssText();
            break;
        case CSS_RGBCOLOR:
        case CSS_PARSER_HEXCOLOR: {
            RGBA32 rgbColor = m_value.rgbcolor;
            if (m_primitiveUnitType == CSS_PARSER_HEXCOLOR)
                Color::parseHexColor(m_value.string, rgbColor);
            Color color(rgbColor);

            Vector<LChar> result;
            result.reserveInitialCapacity(32);
            bool colorHasAlpha = color.hasAlpha();
            if (colorHasAlpha)
                result.append("rgba(", 5);
            else
                result.append("rgb(", 4);

            appendNumber(result, static_cast<unsigned char>(color.red()));
            result.append(", ", 2);

            appendNumber(result, static_cast<unsigned char>(color.green()));
            result.append(", ", 2);

            appendNumber(result, static_cast<unsigned char>(color.blue()));
            if (colorHasAlpha) {
                result.append(", ", 2);

                NumberToStringBuffer buffer;
                const char* alphaString = numberToFixedPrecisionString(color.alpha() / 255.0f, 6, buffer, true);
                result.append(alphaString, strlen(alphaString));
            }

            result.append(')');
            text = String::adopt(result);
            break;
        }
        case CSS_PAIR:
            text = getPairValue()->cssText();
            break;
#if ENABLE(DASHBOARD_SUPPORT)
        case CSS_DASHBOARD_REGION: {
            StringBuilder result;
            for (DashboardRegion* region = getDashboardRegionValue(); region; region = region->m_next.get()) {
                if (!result.isEmpty())
                    result.append(' ');
                result.appendLiteral("dashboard-region(");
                result.append(region->m_label);
                if (region->m_isCircle)
                    result.appendLiteral(" circle");
                else if (region->m_isRectangle)
                    result.appendLiteral(" rectangle");
                else
                    break;
                if (region->top()->m_primitiveUnitType == CSS_VALUE_ID && region->top()->getValueID() == CSSValueInvalid) {
                    ASSERT(region->right()->m_primitiveUnitType == CSS_VALUE_ID);
                    ASSERT(region->bottom()->m_primitiveUnitType == CSS_VALUE_ID);
                    ASSERT(region->left()->m_primitiveUnitType == CSS_VALUE_ID);
                    ASSERT(region->right()->getValueID() == CSSValueInvalid);
                    ASSERT(region->bottom()->getValueID() == CSSValueInvalid);
                    ASSERT(region->left()->getValueID() == CSSValueInvalid);
                } else {
                    result.append(' ');
                    result.append(region->top()->cssText());
                    result.append(' ');
                    result.append(region->right()->cssText());
                    result.append(' ');
                    result.append(region->bottom()->cssText());
                    result.append(' ');
                    result.append(region->left()->cssText());
                }
                result.append(')');
            }
            text = result.toString();
            break;
        }
#endif
        case CSS_PARSER_OPERATOR: {
            char c = static_cast<char>(m_value.parserOperator);
            text = String(&c, 1U);
            break;
        }
        case CSS_PARSER_IDENTIFIER:
            text = quoteCSSStringIfNeeded(m_value.string);
            break;
        case CSS_CALC:
            text = m_value.calc->cssText();
            break;
        case CSS_SHAPE:
            text = m_value.shape->cssText();
            break;
        case CSS_VW:
            text = formatNumber(m_value.num, "vw");
            break;
        case CSS_VH:
            text = formatNumber(m_value.num, "vh");
            break;
        case CSS_VMIN:
            text = formatNumber(m_value.num, "vmin");
            break;
        case CSS_VMAX:
            text = formatNumber(m_value.num, "vmax");
            break;
#if ENABLE(CSS_VARIABLES)
        case CSS_VARIABLE_NAME:
            text = "-webkit-var(" + String(m_value.string) + ')';
            break;
#endif
    }

    ASSERT(!cssTextCache().contains(this));
    cssTextCache().set(this, text);
    m_hasCachedCSSText = true;
    return text;
}

#if ENABLE(CSS_VARIABLES)
String CSSPrimitiveValue::customSerializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    if (isVariableName() && variables.contains(m_value.string))
        return variables.get(m_value.string);
    if (CSSCalcValue* calcValue = cssCalcValue())
        return calcValue->customSerializeResolvingVariables(variables);
    if (Pair* pairValue = getPairValue())
        return pairValue->serializeResolvingVariables(variables);
    if (Rect* rectVal = getRectValue())
        return rectVal->serializeResolvingVariables(variables);
    if (Quad* quadVal = getQuadValue())
        return quadVal->serializeResolvingVariables(variables);
    if (CSSBasicShape* shapeValue = getShapeValue())
        return shapeValue->serializeResolvingVariables(variables);
    return customCssText();
}

bool CSSPrimitiveValue::hasVariableReference() const
{
    if (CSSCalcValue* calcValue = cssCalcValue())
        return calcValue->hasVariableReference();
    if (Pair* pairValue = getPairValue())
        return pairValue->hasVariableReference();
    if (Quad* quadValue = getQuadValue())
        return quadValue->hasVariableReference();
    if (Rect* rectValue = getRectValue())
        return rectValue->hasVariableReference();
    if (CSSBasicShape* shapeValue = getShapeValue())
        return shapeValue->hasVariableReference();
    return isVariableName();
}
#endif

void CSSPrimitiveValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const StyleSheetContents* styleSheet) const
{
    if (m_primitiveUnitType == CSS_URI)
        addSubresourceURL(urls, styleSheet->completeURL(m_value.string));
}

Length CSSPrimitiveValue::viewportPercentageLength() const
{
    ASSERT(isViewportPercentageLength());
    Length viewportLength;
    switch (m_primitiveUnitType) {
    case CSS_VW:
        viewportLength = Length(getDoubleValue(), ViewportPercentageWidth);
        break;
    case CSS_VH:
        viewportLength = Length(getDoubleValue(), ViewportPercentageHeight);
        break;
    case CSS_VMIN:
        viewportLength = Length(getDoubleValue(), ViewportPercentageMin);
        break;
    case CSS_VMAX:
        viewportLength = Length(getDoubleValue(), ViewportPercentageMax);
        break;
    default:
        break;
    }
    return viewportLength;
}

PassRefPtr<CSSPrimitiveValue> CSSPrimitiveValue::cloneForCSSOM() const
{
    RefPtr<CSSPrimitiveValue> result;

    switch (m_primitiveUnitType) {
    case CSS_STRING:
    case CSS_URI:
    case CSS_ATTR:
    case CSS_COUNTER_NAME:
        result = CSSPrimitiveValue::create(m_value.string, static_cast<UnitTypes>(m_primitiveUnitType));
        break;
    case CSS_COUNTER:
        result = CSSPrimitiveValue::create(m_value.counter->cloneForCSSOM());
        break;
    case CSS_RECT:
        result = CSSPrimitiveValue::create(m_value.rect->cloneForCSSOM());
        break;
    case CSS_QUAD:
        result = CSSPrimitiveValue::create(m_value.quad->cloneForCSSOM());
        break;
    case CSS_PAIR:
        // Pair is not exposed to the CSSOM, no need for a deep clone.
        result = CSSPrimitiveValue::create(m_value.pair);
        break;
#if ENABLE(DASHBOARD_SUPPORT)
    case CSS_DASHBOARD_REGION:
        // DashboardRegion is not exposed to the CSSOM, no need for a deep clone.
        result = CSSPrimitiveValue::create(m_value.region);
        break;
#endif
    case CSS_CALC:
        // CSSCalcValue is not exposed to the CSSOM, no need for a deep clone.
        result = CSSPrimitiveValue::create(m_value.calc);
        break;
    case CSS_SHAPE:
        // CSSShapeValue is not exposed to the CSSOM, no need for a deep clone.
        result = CSSPrimitiveValue::create(m_value.shape);
        break;
    case CSS_NUMBER:
    case CSS_PARSER_INTEGER:
    case CSS_PERCENTAGE:
    case CSS_EMS:
    case CSS_EXS:
    case CSS_REMS:
    case CSS_CHS:
    case CSS_PX:
    case CSS_CM:
    case CSS_MM:
    case CSS_IN:
    case CSS_PT:
    case CSS_PC:
    case CSS_DEG:
    case CSS_RAD:
    case CSS_GRAD:
    case CSS_MS:
    case CSS_S:
    case CSS_HZ:
    case CSS_KHZ:
    case CSS_TURN:
    case CSS_VW:
    case CSS_VH:
    case CSS_VMIN:
    case CSS_VMAX:
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSS_DPPX:
    case CSS_DPI:
    case CSS_DPCM:
#endif
        result = CSSPrimitiveValue::create(m_value.num, static_cast<UnitTypes>(m_primitiveUnitType));
        break;
    case CSS_PROPERTY_ID:
        result = CSSPrimitiveValue::createIdentifier(m_value.propertyID);
        break;
    case CSS_VALUE_ID:
        result = CSSPrimitiveValue::createIdentifier(m_value.valueID);
        break;
    case CSS_RGBCOLOR:
        result = CSSPrimitiveValue::createColor(m_value.rgbcolor);
        break;
    case CSS_DIMENSION:
    case CSS_UNKNOWN:
    case CSS_PARSER_OPERATOR:
    case CSS_PARSER_IDENTIFIER:
    case CSS_PARSER_HEXCOLOR:
        ASSERT_NOT_REACHED();
        break;
    }
    if (result)
        result->setCSSOMSafe();

    return result;
}

bool CSSPrimitiveValue::equals(const CSSPrimitiveValue& other) const
{
    if (m_primitiveUnitType != other.m_primitiveUnitType)
        return false;

    switch (m_primitiveUnitType) {
    case CSS_UNKNOWN:
        return false;
    case CSS_NUMBER:
    case CSS_PARSER_INTEGER:
    case CSS_PERCENTAGE:
    case CSS_EMS:
    case CSS_EXS:
    case CSS_REMS:
    case CSS_PX:
    case CSS_CM:
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSS_DPPX:
    case CSS_DPI:
    case CSS_DPCM:
#endif
    case CSS_MM:
    case CSS_IN:
    case CSS_PT:
    case CSS_PC:
    case CSS_DEG:
    case CSS_RAD:
    case CSS_GRAD:
    case CSS_MS:
    case CSS_S:
    case CSS_HZ:
    case CSS_KHZ:
    case CSS_TURN:
    case CSS_VW:
    case CSS_VH:
    case CSS_VMIN:
    case CSS_DIMENSION:
        return m_value.num == other.m_value.num;
    case CSS_PROPERTY_ID:
        return propertyName(m_value.propertyID) == propertyName(other.m_value.propertyID);
    case CSS_VALUE_ID:
        return valueName(m_value.valueID) == valueName(other.m_value.valueID);
    case CSS_STRING:
    case CSS_URI:
    case CSS_ATTR:
    case CSS_COUNTER_NAME:
    case CSS_PARSER_IDENTIFIER:
    case CSS_PARSER_HEXCOLOR:
#if ENABLE(CSS_VARIABLES)
    case CSS_VARIABLE_NAME:
#endif
        return equal(m_value.string, other.m_value.string);
    case CSS_COUNTER:
        return m_value.counter && other.m_value.counter && m_value.counter->equals(*other.m_value.counter);
    case CSS_RECT:
        return m_value.rect && other.m_value.rect && m_value.rect->equals(*other.m_value.rect);
    case CSS_QUAD:
        return m_value.quad && other.m_value.quad && m_value.quad->equals(*other.m_value.quad);
    case CSS_RGBCOLOR:
        return m_value.rgbcolor == other.m_value.rgbcolor;
    case CSS_PAIR:
        return m_value.pair && other.m_value.pair && m_value.pair->equals(*other.m_value.pair);
#if ENABLE(DASHBOARD_SUPPORT)
    case CSS_DASHBOARD_REGION: {
        DashboardRegion* region = getDashboardRegionValue();
        DashboardRegion* otherRegion = other.getDashboardRegionValue();
        return region ? otherRegion && region->equals(*otherRegion) : !otherRegion;
    }
#endif
    case CSS_PARSER_OPERATOR:
        return m_value.valueID == other.m_value.valueID;
    case CSS_CALC:
        return m_value.calc && other.m_value.calc && m_value.calc->equals(*other.m_value.calc);
    case CSS_SHAPE:
        return m_value.shape && other.m_value.shape && m_value.shape->equals(*other.m_value.shape);
    }
    return false;
}

} // namespace WebCore
