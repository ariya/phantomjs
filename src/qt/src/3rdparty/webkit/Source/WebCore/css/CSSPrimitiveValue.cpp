/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "CSSHelper.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "Color.h"
#include "Counter.h"
#include "ExceptionCode.h"
#include "Node.h"
#include "Pair.h"
#include "RGBColor.h"
#include "Rect.h"
#include "RenderStyle.h"
#include <wtf/ASCIICType.h>
#include <wtf/DecimalNumber.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuffer.h>

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

using namespace WTF;

namespace WebCore {

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

static const AtomicString& valueOrPropertyName(int valueOrPropertyID)
{
    ASSERT_ARG(valueOrPropertyID, valueOrPropertyID >= 0);
    ASSERT_ARG(valueOrPropertyID, valueOrPropertyID < numCSSValueKeywords || (valueOrPropertyID >= firstCSSProperty && valueOrPropertyID < firstCSSProperty + numCSSProperties));

    if (valueOrPropertyID < 0)
        return nullAtom;

    if (valueOrPropertyID < numCSSValueKeywords) {
        static AtomicString* cssValueKeywordStrings[numCSSValueKeywords];
        if (!cssValueKeywordStrings[valueOrPropertyID])
            cssValueKeywordStrings[valueOrPropertyID] = new AtomicString(getValueName(valueOrPropertyID));
        return *cssValueKeywordStrings[valueOrPropertyID];
    }

    if (valueOrPropertyID >= firstCSSProperty && valueOrPropertyID < firstCSSProperty + numCSSProperties) {
        static AtomicString* cssPropertyStrings[numCSSProperties];
        int propertyIndex = valueOrPropertyID - firstCSSProperty;
        if (!cssPropertyStrings[propertyIndex])
            cssPropertyStrings[propertyIndex] = new AtomicString(getPropertyName(static_cast<CSSPropertyID>(valueOrPropertyID)));
        return *cssPropertyStrings[propertyIndex];
    }

    return nullAtom;
}

CSSPrimitiveValue::CSSPrimitiveValue()
    : m_type(0)
    , m_hasCachedCSSText(false)
{
}

CSSPrimitiveValue::CSSPrimitiveValue(int ident)
    : m_type(CSS_IDENT)
    , m_hasCachedCSSText(false)
{
    m_value.ident = ident;
}

CSSPrimitiveValue::CSSPrimitiveValue(double num, UnitTypes type)
    : m_type(type)
    , m_hasCachedCSSText(false)
{
    m_value.num = num;
}

CSSPrimitiveValue::CSSPrimitiveValue(const String& str, UnitTypes type)
    : m_type(type)
    , m_hasCachedCSSText(false)
{
    if ((m_value.string = str.impl()))
        m_value.string->ref();
}

CSSPrimitiveValue::CSSPrimitiveValue(RGBA32 color)
    : m_type(CSS_RGBCOLOR)
    , m_hasCachedCSSText(false)
{
    m_value.rgbcolor = color;
}

CSSPrimitiveValue::CSSPrimitiveValue(const Length& length)
    : m_hasCachedCSSText(false)
{
    switch (length.type()) {
        case Auto:
            m_type = CSS_IDENT;
            m_value.ident = CSSValueAuto;
            break;
        case WebCore::Fixed:
            m_type = CSS_PX;
            m_value.num = length.value();
            break;
        case Intrinsic:
            m_type = CSS_IDENT;
            m_value.ident = CSSValueIntrinsic;
            break;
        case MinIntrinsic:
            m_type = CSS_IDENT;
            m_value.ident = CSSValueMinIntrinsic;
            break;
        case Percent:
            m_type = CSS_PERCENTAGE;
            m_value.num = length.percent();
            break;
        case Relative:
            ASSERT_NOT_REACHED();
            break;
    }
}

void CSSPrimitiveValue::init(PassRefPtr<Counter> c)
{
    m_type = CSS_COUNTER;
    m_hasCachedCSSText = false;
    m_value.counter = c.releaseRef();
}

void CSSPrimitiveValue::init(PassRefPtr<Rect> r)
{
    m_type = CSS_RECT;
    m_hasCachedCSSText = false;
    m_value.rect = r.releaseRef();
}

#if ENABLE(DASHBOARD_SUPPORT)
void CSSPrimitiveValue::init(PassRefPtr<DashboardRegion> r)
{
    m_type = CSS_DASHBOARD_REGION;
    m_hasCachedCSSText = false;
    m_value.region = r.releaseRef();
}
#endif

void CSSPrimitiveValue::init(PassRefPtr<Pair> p)
{
    m_type = CSS_PAIR;
    m_hasCachedCSSText = false;
    m_value.pair = p.releaseRef();
}

CSSPrimitiveValue::~CSSPrimitiveValue()
{
    cleanup();
}

void CSSPrimitiveValue::cleanup()
{
    switch (m_type) {
        case CSS_STRING:
        case CSS_URI:
        case CSS_ATTR:
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
        case CSS_PAIR:
            m_value.pair->deref();
            break;
#if ENABLE(DASHBOARD_SUPPORT)
        case CSS_DASHBOARD_REGION:
            if (m_value.region)
                m_value.region->deref();
            break;
#endif
        default:
            break;
    }

    m_type = 0;
    if (m_hasCachedCSSText) {
        cssTextCache().remove(this);
        m_hasCachedCSSText = false;
    }
}

int CSSPrimitiveValue::computeLengthInt(RenderStyle* style, RenderStyle* rootStyle)
{
    return roundForImpreciseConversion<int, INT_MAX, INT_MIN>(computeLengthDouble(style, rootStyle));
}

int CSSPrimitiveValue::computeLengthInt(RenderStyle* style, RenderStyle* rootStyle, double multiplier)
{
    return roundForImpreciseConversion<int, INT_MAX, INT_MIN>(computeLengthDouble(style, rootStyle, multiplier));
}

// Lengths expect an int that is only 28-bits, so we have to check for a
// different overflow.
int CSSPrimitiveValue::computeLengthIntForLength(RenderStyle* style, RenderStyle* rootStyle)
{
    return roundForImpreciseConversion<int, intMaxForLength, intMinForLength>(computeLengthDouble(style, rootStyle));
}

int CSSPrimitiveValue::computeLengthIntForLength(RenderStyle* style, RenderStyle* rootStyle, double multiplier)
{
    return roundForImpreciseConversion<int, intMaxForLength, intMinForLength>(computeLengthDouble(style, rootStyle, multiplier));
}

short CSSPrimitiveValue::computeLengthShort(RenderStyle* style, RenderStyle* rootStyle)
{
    return roundForImpreciseConversion<short, SHRT_MAX, SHRT_MIN>(computeLengthDouble(style, rootStyle));
}

short CSSPrimitiveValue::computeLengthShort(RenderStyle* style, RenderStyle* rootStyle, double multiplier)
{
    return roundForImpreciseConversion<short, SHRT_MAX, SHRT_MIN>(computeLengthDouble(style, rootStyle, multiplier));
}

float CSSPrimitiveValue::computeLengthFloat(RenderStyle* style, RenderStyle* rootStyle, bool computingFontSize)
{
    return static_cast<float>(computeLengthDouble(style, rootStyle, 1.0, computingFontSize));
}

float CSSPrimitiveValue::computeLengthFloat(RenderStyle* style, RenderStyle* rootStyle, double multiplier, bool computingFontSize)
{
    return static_cast<float>(computeLengthDouble(style, rootStyle, multiplier, computingFontSize));
}

double CSSPrimitiveValue::computeLengthDouble(RenderStyle* style, RenderStyle* rootStyle, double multiplier, bool computingFontSize)
{
    unsigned short type = primitiveType();

    // We do not apply the zoom factor when we are computing the value of the font-size property.  The zooming
    // for font sizes is much more complicated, since we have to worry about enforcing the minimum font size preference
    // as well as enforcing the implicit "smart minimum."  In addition the CSS property text-size-adjust is used to
    // prevent text from zooming at all.  Therefore we will not apply the zoom here if we are computing font-size.
    bool applyZoomMultiplier = !computingFontSize;

    double factor = 1.0;
    switch (type) {
        case CSS_EMS:
            applyZoomMultiplier = false;
            factor = computingFontSize ? style->fontDescription().specifiedSize() : style->fontDescription().computedSize();
            break;
        case CSS_EXS:
            // FIXME: We have a bug right now where the zoom will be applied twice to EX units.
            // We really need to compute EX using fontMetrics for the original specifiedSize and not use
            // our actual constructed rendering font.
            applyZoomMultiplier = false;
            factor = style->fontMetrics().xHeight();
            break;
        case CSS_REMS:
            applyZoomMultiplier = false;
            factor = computingFontSize ? rootStyle->fontDescription().specifiedSize() : rootStyle->fontDescription().computedSize();
            break;
        case CSS_PX:
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
        default:
            return -1.0;
    }

    double result = getDoubleValue() * factor;
    if (!applyZoomMultiplier || multiplier == 1.0)
        return result;
     
    // Any original result that was >= 1 should not be allowed to fall below 1.  This keeps border lines from
    // vanishing.
    double zoomedResult = result * multiplier;
    if (result >= 1.0)
        zoomedResult = max(1.0, zoomedResult);
    return zoomedResult;
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
        case CSSPrimitiveValue::CSS_MM:
            factor = cssPixelsPerInch / 25.4;
            break;
        case CSSPrimitiveValue::CSS_IN:
            factor = cssPixelsPerInch;
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
    default:
        return CSS_UNKNOWN;
    }
}

bool CSSPrimitiveValue::getDoubleValueInternal(UnitTypes requestedUnitType, double* result) const
{
    if (m_type < CSS_NUMBER || (m_type > CSS_DIMENSION && m_type < CSS_TURN) || requestedUnitType < CSS_NUMBER || (requestedUnitType > CSS_DIMENSION && requestedUnitType < CSS_TURN))
        return false;
    if (requestedUnitType == m_type || requestedUnitType == CSS_DIMENSION) {
        *result = m_value.num;
        return true;
    }

    UnitTypes sourceUnitType = static_cast<UnitTypes>(m_type);
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

    double convertedValue = m_value.num;

    // First convert the value from m_type to canonical type.
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
    switch (m_type) {
        case CSS_STRING:
        case CSS_ATTR:
        case CSS_URI:
            return m_value.string;
        case CSS_IDENT:
            return valueOrPropertyName(m_value.ident);
        default:
            ec = INVALID_ACCESS_ERR;
            break;
    }

    return String();
}

String CSSPrimitiveValue::getStringValue() const
{
    switch (m_type) {
        case CSS_STRING:
        case CSS_ATTR:
        case CSS_URI:
             return m_value.string;
        case CSS_IDENT:
            return valueOrPropertyName(m_value.ident);
        default:
            break;
    }

    return String();
}

Counter* CSSPrimitiveValue::getCounterValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_type != CSS_COUNTER) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.counter;
}

Rect* CSSPrimitiveValue::getRectValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_type != CSS_RECT) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.rect;
}

PassRefPtr<RGBColor> CSSPrimitiveValue::getRGBColorValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_type != CSS_RGBCOLOR) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    // FIMXE: This should not return a new object for each invocation.
    return RGBColor::create(m_value.rgbcolor);
}

Pair* CSSPrimitiveValue::getPairValue(ExceptionCode& ec) const
{
    ec = 0;
    if (m_type != CSS_PAIR) {
        ec = INVALID_ACCESS_ERR;
        return 0;
    }

    return m_value.pair;
}

unsigned short CSSPrimitiveValue::cssValueType() const
{
    return CSS_PRIMITIVE_VALUE;
}

bool CSSPrimitiveValue::parseString(const String& /*string*/, bool /*strict*/)
{
    // FIXME
    return false;
}

int CSSPrimitiveValue::getIdent() const
{
    if (m_type != CSS_IDENT)
        return 0;
    return m_value.ident;
}

static String formatNumber(double number)
{
    DecimalNumber decimal(number);
    
    StringBuffer buffer(decimal.bufferLengthForStringDecimal());
    unsigned length = decimal.toStringDecimal(buffer.characters(), buffer.length());
    ASSERT_UNUSED(length, length == buffer.length());

    return String::adopt(buffer);
}

String CSSPrimitiveValue::cssText() const
{
    // FIXME: return the original value instead of a generated one (e.g. color
    // name if it was specified) - check what spec says about this

    if (m_hasCachedCSSText) {
        ASSERT(cssTextCache().contains(this));
        return cssTextCache().get(this);
    }

    String text;
    switch (m_type) {
        case CSS_UNKNOWN:
            // FIXME
            break;
        case CSS_NUMBER:
        case CSS_PARSER_INTEGER:
            text = formatNumber(m_value.num);
            break;
        case CSS_PERCENTAGE:
            text = formatNumber(m_value.num) + "%";
            break;
        case CSS_EMS:
            text = formatNumber(m_value.num) + "em";
            break;
        case CSS_EXS:
            text = formatNumber(m_value.num) + "ex";
            break;
        case CSS_REMS:
            text = formatNumber(m_value.num) + "rem";
            break;
        case CSS_PX:
            text = formatNumber(m_value.num) + "px";
            break;
        case CSS_CM:
            text = formatNumber(m_value.num) + "cm";
            break;
        case CSS_MM:
            text = formatNumber(m_value.num) + "mm";
            break;
        case CSS_IN:
            text = formatNumber(m_value.num) + "in";
            break;
        case CSS_PT:
            text = formatNumber(m_value.num) + "pt";
            break;
        case CSS_PC:
            text = formatNumber(m_value.num) + "pc";
            break;
        case CSS_DEG:
            text = formatNumber(m_value.num) + "deg";
            break;
        case CSS_RAD:
            text = formatNumber(m_value.num) + "rad";
            break;
        case CSS_GRAD:
            text = formatNumber(m_value.num) + "grad";
            break;
        case CSS_MS:
            text = formatNumber(m_value.num) + "ms";
            break;
        case CSS_S:
            text = formatNumber(m_value.num) + "s";
            break;
        case CSS_HZ:
            text = formatNumber(m_value.num) + "hz";
            break;
        case CSS_KHZ:
            text = formatNumber(m_value.num) + "khz";
            break;
        case CSS_TURN:
            text = formatNumber(m_value.num) + "turn";
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
        case CSS_IDENT:
            text = valueOrPropertyName(m_value.ident);
            break;
        case CSS_ATTR: {
            DEFINE_STATIC_LOCAL(const String, attrParen, ("attr("));

            Vector<UChar> result;
            result.reserveInitialCapacity(6 + m_value.string->length());

            append(result, attrParen);
            append(result, m_value.string);
            result.uncheckedAppend(')');

            text = String::adopt(result);
            break;
        }
        case CSS_COUNTER_NAME:
            text = "counter(";
            text += m_value.string;
            text += ")";
            break;
        case CSS_COUNTER:
            text = "counter(";
            text += String::number(m_value.num);
            text += ")";
            // FIXME: Add list-style and separator
            break;
        case CSS_RECT: {
            DEFINE_STATIC_LOCAL(const String, rectParen, ("rect("));

            Rect* rectVal = getRectValue();
            Vector<UChar> result;
            result.reserveInitialCapacity(32);
            append(result, rectParen);

            append(result, rectVal->top()->cssText());
            result.append(' ');

            append(result, rectVal->right()->cssText());
            result.append(' ');

            append(result, rectVal->bottom()->cssText());
            result.append(' ');

            append(result, rectVal->left()->cssText());
            result.append(')');

            text = String::adopt(result);
            break;
        }
        case CSS_RGBCOLOR:
        case CSS_PARSER_HEXCOLOR: {
            DEFINE_STATIC_LOCAL(const String, commaSpace, (", "));
            DEFINE_STATIC_LOCAL(const String, rgbParen, ("rgb("));
            DEFINE_STATIC_LOCAL(const String, rgbaParen, ("rgba("));

            RGBA32 rgbColor = m_value.rgbcolor;
            if (m_type == CSS_PARSER_HEXCOLOR)
                Color::parseHexColor(m_value.string, rgbColor);
            Color color(rgbColor);

            Vector<UChar> result;
            result.reserveInitialCapacity(32);
            if (color.hasAlpha())
                append(result, rgbaParen);
            else
                append(result, rgbParen);

            appendNumber(result, static_cast<unsigned char>(color.red()));
            append(result, commaSpace);

            appendNumber(result, static_cast<unsigned char>(color.green()));
            append(result, commaSpace);

            appendNumber(result, static_cast<unsigned char>(color.blue()));
            if (color.hasAlpha()) {
                append(result, commaSpace);
                append(result, String::number(color.alpha() / 256.0f));
            }

            result.append(')');
            text = String::adopt(result);
            break;
        }
        case CSS_PAIR:
            text = m_value.pair->first()->cssText();
            text += " ";
            text += m_value.pair->second()->cssText();
            break;
#if ENABLE(DASHBOARD_SUPPORT)
        case CSS_DASHBOARD_REGION:
            for (DashboardRegion* region = getDashboardRegionValue(); region; region = region->m_next.get()) {
                if (!text.isEmpty())
                    text.append(' ');
                text += "dashboard-region(";
                text += region->m_label;
                if (region->m_isCircle)
                    text += " circle";
                else if (region->m_isRectangle)
                    text += " rectangle";
                else
                    break;
                if (region->top()->m_type == CSS_IDENT && region->top()->getIdent() == CSSValueInvalid) {
                    ASSERT(region->right()->m_type == CSS_IDENT);
                    ASSERT(region->bottom()->m_type == CSS_IDENT);
                    ASSERT(region->left()->m_type == CSS_IDENT);
                    ASSERT(region->right()->getIdent() == CSSValueInvalid);
                    ASSERT(region->bottom()->getIdent() == CSSValueInvalid);
                    ASSERT(region->left()->getIdent() == CSSValueInvalid);
                } else {
                    text.append(' ');
                    text += region->top()->cssText() + " ";
                    text += region->right()->cssText() + " ";
                    text += region->bottom()->cssText() + " ";
                    text += region->left()->cssText();
                }
                text += ")";
            }
            break;
#endif
        case CSS_PARSER_OPERATOR: {
            char c = static_cast<char>(m_value.ident);
            text = String(&c, 1U);
            break;
        }
        case CSS_PARSER_IDENTIFIER:
            text = quoteCSSStringIfNeeded(m_value.string);
            break;
    }

    ASSERT(!cssTextCache().contains(this));
    cssTextCache().set(this, text);
    m_hasCachedCSSText = true;
    return text;
}

void CSSPrimitiveValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const CSSStyleSheet* styleSheet)
{
    if (m_type == CSS_URI)
        addSubresourceURL(urls, styleSheet->completeURL(m_value.string));
}

} // namespace WebCore
