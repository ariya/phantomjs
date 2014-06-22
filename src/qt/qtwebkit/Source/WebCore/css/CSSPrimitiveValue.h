/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
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

#ifndef CSSPrimitiveValue_h
#define CSSPrimitiveValue_h

#include "CSSPropertyNames.h"
#include "CSSValue.h"
#include "CSSValueKeywords.h"
#include "Color.h"
#include <wtf/Forward.h>
#include <wtf/MathExtras.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class CSSCalcValue;
class Counter;
class DashboardRegion;
class Pair;
class Quad;
class RGBColor;
class Rect;
class RenderStyle;
class CSSBasicShape;

struct Length;

// Dimension calculations are imprecise, often resulting in values of e.g.
// 44.99998. We need to go ahead and round if we're really close to the next
// integer value.
template<typename T> inline T roundForImpreciseConversion(double value)
{
    value += (value < 0) ? -0.01 : +0.01;
    return ((value > std::numeric_limits<T>::max()) || (value < std::numeric_limits<T>::min())) ? 0 : static_cast<T>(value);
}

template<> inline float roundForImpreciseConversion(double value)
{
    double ceiledValue = ceil(value);
    double proximityToNextInt = ceiledValue - value;
    if (proximityToNextInt <= 0.01 && value > 0)
        return static_cast<float>(ceiledValue);
    if (proximityToNextInt >= 0.99 && value < 0)
        return static_cast<float>(floor(value));
    return static_cast<float>(value);
}

class CSSPrimitiveValue : public CSSValue {
public:
    enum UnitTypes {
        CSS_UNKNOWN = 0,
        CSS_NUMBER = 1,
        CSS_PERCENTAGE = 2,
        CSS_EMS = 3,
        CSS_EXS = 4,
        CSS_PX = 5,
        CSS_CM = 6,
        CSS_MM = 7,
        CSS_IN = 8,
        CSS_PT = 9,
        CSS_PC = 10,
        CSS_DEG = 11,
        CSS_RAD = 12,
        CSS_GRAD = 13,
        CSS_MS = 14,
        CSS_S = 15,
        CSS_HZ = 16,
        CSS_KHZ = 17,
        CSS_DIMENSION = 18,
        CSS_STRING = 19,
        CSS_URI = 20,
        CSS_IDENT = 21,
        CSS_ATTR = 22,
        CSS_COUNTER = 23,
        CSS_RECT = 24,
        CSS_RGBCOLOR = 25,
        // From CSS Values and Units. Viewport-percentage Lengths (vw/vh/vmin/vmax).
        CSS_VW = 26,
        CSS_VH = 27,
        CSS_VMIN = 28,
        CSS_VMAX = 29,
        CSS_DPPX = 30,
        CSS_DPI = 31,
        CSS_DPCM = 32,
        CSS_PAIR = 100, // We envision this being exposed as a means of getting computed style values for pairs (border-spacing/radius, background-position, etc.)
#if ENABLE(DASHBOARD_SUPPORT)
        CSS_DASHBOARD_REGION = 101, // FIXME: Dashboard region should not be a primitive value.
#endif
        CSS_UNICODE_RANGE = 102,

        // These next types are just used internally to allow us to translate back and forth from CSSPrimitiveValues to CSSParserValues.
        CSS_PARSER_OPERATOR = 103,
        CSS_PARSER_INTEGER = 104,
        CSS_PARSER_HEXCOLOR = 105,

        // This is used internally for unknown identifiers
        CSS_PARSER_IDENTIFIER = 106,

        // These are from CSS3 Values and Units, but that isn't a finished standard yet
        CSS_TURN = 107,
        CSS_REMS = 108,
        CSS_CHS = 109,

        // This is used internally for counter names (as opposed to counter values)
        CSS_COUNTER_NAME = 110,

        // This is used by the CSS Shapes draft
        CSS_SHAPE = 111,

        // Used by border images.
        CSS_QUAD = 112,

        CSS_CALC = 113,
        CSS_CALC_PERCENTAGE_WITH_NUMBER = 114,
        CSS_CALC_PERCENTAGE_WITH_LENGTH = 115,

#if ENABLE(CSS_VARIABLES)
        CSS_VARIABLE_NAME = 116,
#endif
        CSS_PROPERTY_ID = 117,
        CSS_VALUE_ID = 118
    };

    // This enum follows the CSSParser::Units enum augmented with UNIT_FREQUENCY for frequencies.
    enum UnitCategory {
        UNumber,
        UPercent,
        ULength,
        UAngle,
        UTime,
        UFrequency,
        UViewportPercentageLength,
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
        UResolution,
#endif
        UOther
    };

    bool isAngle() const
    {
        return m_primitiveUnitType == CSS_DEG
               || m_primitiveUnitType == CSS_RAD
               || m_primitiveUnitType == CSS_GRAD
               || m_primitiveUnitType == CSS_TURN;
    }
    bool isAttr() const { return m_primitiveUnitType == CSS_ATTR; }
    bool isCounter() const { return m_primitiveUnitType == CSS_COUNTER; }
    bool isFontIndependentLength() const { return m_primitiveUnitType >= CSS_PX && m_primitiveUnitType <= CSS_PC; }
    bool isFontRelativeLength() const
    {
        return m_primitiveUnitType == CSS_EMS
            || m_primitiveUnitType == CSS_EXS
            || m_primitiveUnitType == CSS_REMS
            || m_primitiveUnitType == CSS_CHS;
    }
    bool isLength() const
    {
        unsigned short type = primitiveType();
        return (type >= CSS_EMS && type <= CSS_PC) || type == CSS_REMS || type == CSS_CHS;
    }
    bool isNumber() const { return primitiveType() == CSS_NUMBER; }
    bool isPercentage() const { return primitiveType() == CSS_PERCENTAGE; }
    bool isPx() const { return primitiveType() == CSS_PX; }
    bool isRect() const { return m_primitiveUnitType == CSS_RECT; }
    bool isRGBColor() const { return m_primitiveUnitType == CSS_RGBCOLOR; }
    bool isShape() const { return m_primitiveUnitType == CSS_SHAPE; }
    bool isString() const { return m_primitiveUnitType == CSS_STRING; }
    bool isTime() const { return m_primitiveUnitType == CSS_S || m_primitiveUnitType == CSS_MS; }
    bool isURI() const { return m_primitiveUnitType == CSS_URI; }
    bool isCalculated() const { return m_primitiveUnitType == CSS_CALC; }
    bool isCalculatedPercentageWithNumber() const { return primitiveType() == CSS_CALC_PERCENTAGE_WITH_NUMBER; }
    bool isCalculatedPercentageWithLength() const { return primitiveType() == CSS_CALC_PERCENTAGE_WITH_LENGTH; }
    bool isDotsPerInch() const { return primitiveType() == CSS_DPI; }
    bool isDotsPerPixel() const { return primitiveType() == CSS_DPPX; }
    bool isDotsPerCentimeter() const { return primitiveType() == CSS_DPCM; }
    bool isResolution() const
    {
        unsigned short type = primitiveType();
        return type >= CSS_DPPX && type <= CSS_DPCM;
    }

#if ENABLE(CSS_VARIABLES)
    bool isVariableName() const { return primitiveType() == CSS_VARIABLE_NAME; }
#endif
    bool isViewportPercentageLength() const { return m_primitiveUnitType >= CSS_VW && m_primitiveUnitType <= CSS_VMAX; }
    bool isValueID() const { return m_primitiveUnitType == CSS_VALUE_ID; }
    
    static PassRefPtr<CSSPrimitiveValue> createIdentifier(CSSValueID valueID) { return adoptRef(new CSSPrimitiveValue(valueID)); }
    static PassRefPtr<CSSPrimitiveValue> createIdentifier(CSSPropertyID propertyID) { return adoptRef(new CSSPrimitiveValue(propertyID)); }
    static PassRefPtr<CSSPrimitiveValue> createParserOperator(int parserOperator) { return adoptRef(new CSSPrimitiveValue(parserOperator)); }

    static PassRefPtr<CSSPrimitiveValue> createColor(unsigned rgbValue) { return adoptRef(new CSSPrimitiveValue(rgbValue)); }
    static PassRefPtr<CSSPrimitiveValue> create(double value, UnitTypes type) { return adoptRef(new CSSPrimitiveValue(value, type)); }
    static PassRefPtr<CSSPrimitiveValue> create(const String& value, UnitTypes type) { return adoptRef(new CSSPrimitiveValue(value, type)); }

    template<typename T> static PassRefPtr<CSSPrimitiveValue> create(T value)
    {
        return adoptRef(new CSSPrimitiveValue(value));
    }

    // This value is used to handle quirky margins in reflow roots (body, td, and th) like WinIE.
    // The basic idea is that a stylesheet can use the value __qem (for quirky em) instead of em.
    // When the quirky value is used, if you're in quirks mode, the margin will collapse away
    // inside a table cell.
    static PassRefPtr<CSSPrimitiveValue> createAllowingMarginQuirk(double value, UnitTypes type)
    {
        CSSPrimitiveValue* quirkValue = new CSSPrimitiveValue(value, type);
        quirkValue->m_isQuirkValue = true;
        return adoptRef(quirkValue);
    }

    ~CSSPrimitiveValue();

    void cleanup();

    unsigned short primitiveType() const;

    double computeDegrees();

    enum TimeUnit { Seconds, Milliseconds };
    template <typename T, TimeUnit timeUnit> T computeTime()
    {
        if (timeUnit == Seconds && m_primitiveUnitType == CSS_S)
            return getValue<T>();
        if (timeUnit == Seconds && m_primitiveUnitType == CSS_MS)
            return getValue<T>() / 1000;
        if (timeUnit == Milliseconds && m_primitiveUnitType == CSS_MS)
            return getValue<T>();
        if (timeUnit == Milliseconds && m_primitiveUnitType == CSS_S)
            return getValue<T>() * 1000;
        ASSERT_NOT_REACHED();
        return 0;
    }

    /*
     * computes a length in pixels out of the given CSSValue. Need the RenderStyle to get
     * the fontinfo in case val is defined in em or ex.
     *
     * The metrics have to be a bit different for screen and printer output.
     * For screen output we assume 1 inch == 72 px, for printer we assume 300 dpi
     *
     * this is screen/printer dependent, so we probably need a config option for this,
     * and some tool to calibrate.
     */
    template<typename T> T computeLength(const RenderStyle* currStyle, const RenderStyle* rootStyle, float multiplier = 1.0f, bool computingFontSize = false) const;

    // Converts to a Length, mapping various unit types appropriately.
    template<int> Length convertToLength(const RenderStyle* currStyle, const RenderStyle* rootStyle, double multiplier = 1.0, bool computingFontSize = false) const;

    // use with care!!!
    void setPrimitiveType(unsigned short type) { m_primitiveUnitType = type; }

    double getDoubleValue(unsigned short unitType, ExceptionCode&) const;
    double getDoubleValue(unsigned short unitType) const;
    double getDoubleValue() const;

    void setFloatValue(unsigned short unitType, double floatValue, ExceptionCode&);
    float getFloatValue(unsigned short unitType, ExceptionCode& ec) const { return getValue<float>(unitType, ec); }
    float getFloatValue(unsigned short unitType) const { return getValue<float>(unitType); }
    float getFloatValue() const { return getValue<float>(); }

    int getIntValue(unsigned short unitType, ExceptionCode& ec) const { return getValue<int>(unitType, ec); }
    int getIntValue(unsigned short unitType) const { return getValue<int>(unitType); }
    int getIntValue() const { return getValue<int>(); }

    template<typename T> inline T getValue(unsigned short unitType, ExceptionCode& ec) const { return clampTo<T>(getDoubleValue(unitType, ec)); }
    template<typename T> inline T getValue(unsigned short unitType) const { return clampTo<T>(getDoubleValue(unitType)); }
    template<typename T> inline T getValue() const { return clampTo<T>(getDoubleValue()); }

    void setStringValue(unsigned short stringType, const String& stringValue, ExceptionCode&);
    String getStringValue(ExceptionCode&) const;
    String getStringValue() const;

    Counter* getCounterValue(ExceptionCode&) const;
    Counter* getCounterValue() const { return m_primitiveUnitType != CSS_COUNTER ? 0 : m_value.counter; }

    Rect* getRectValue(ExceptionCode&) const;
    Rect* getRectValue() const { return m_primitiveUnitType != CSS_RECT ? 0 : m_value.rect; }

    Quad* getQuadValue(ExceptionCode&) const;
    Quad* getQuadValue() const { return m_primitiveUnitType != CSS_QUAD ? 0 : m_value.quad; }

    PassRefPtr<RGBColor> getRGBColorValue(ExceptionCode&) const;
    RGBA32 getRGBA32Value() const { return m_primitiveUnitType != CSS_RGBCOLOR ? 0 : m_value.rgbcolor; }

    Pair* getPairValue(ExceptionCode&) const;
    Pair* getPairValue() const { return m_primitiveUnitType != CSS_PAIR ? 0 : m_value.pair; }

#if ENABLE(DASHBOARD_SUPPORT)
    DashboardRegion* getDashboardRegionValue() const { return m_primitiveUnitType != CSS_DASHBOARD_REGION ? 0 : m_value.region; }
#endif

    CSSBasicShape* getShapeValue() const { return m_primitiveUnitType != CSS_SHAPE ? 0 : m_value.shape; }
    
    CSSCalcValue* cssCalcValue() const { return m_primitiveUnitType != CSS_CALC ? 0 : m_value.calc; }

    CSSPropertyID getPropertyID() const { return m_primitiveUnitType == CSS_PROPERTY_ID ? m_value.propertyID : CSSPropertyInvalid; }
    CSSValueID getValueID() const { return m_primitiveUnitType == CSS_VALUE_ID ? m_value.valueID : CSSValueInvalid; }

    template<typename T> inline operator T() const; // Defined in CSSPrimitiveValueMappings.h

    String customCssText() const;
#if ENABLE(CSS_VARIABLES)
    String customSerializeResolvingVariables(const HashMap<AtomicString, String>&) const;
    bool hasVariableReference() const;
#endif

    bool isQuirkValue() { return m_isQuirkValue; }

    void addSubresourceStyleURLs(ListHashSet<KURL>&, const StyleSheetContents*) const;

    Length viewportPercentageLength() const;
    
    PassRefPtr<CSSPrimitiveValue> cloneForCSSOM() const;
    void setCSSOMSafe() { m_isCSSOMSafe = true; }

    bool equals(const CSSPrimitiveValue&) const;

private:
    CSSPrimitiveValue(CSSValueID);
    CSSPrimitiveValue(CSSPropertyID);
    // FIXME: int vs. unsigned overloading is too subtle to distinguish the color and operator cases.
    CSSPrimitiveValue(int parserOperator);
    CSSPrimitiveValue(unsigned color); // RGB value
    CSSPrimitiveValue(const Length&);
    CSSPrimitiveValue(const String&, UnitTypes);
    CSSPrimitiveValue(double, UnitTypes);

    template<typename T> CSSPrimitiveValue(T); // Defined in CSSPrimitiveValueMappings.h
    template<typename T> CSSPrimitiveValue(T* val)
        : CSSValue(PrimitiveClass)
    {
        init(PassRefPtr<T>(val));
    }

    template<typename T> CSSPrimitiveValue(PassRefPtr<T> val)
        : CSSValue(PrimitiveClass)
    {
        init(val);
    }

    static void create(int); // compile-time guard
    static void create(unsigned); // compile-time guard
    template<typename T> operator T*(); // compile-time guard

    static UnitTypes canonicalUnitTypeForCategory(UnitCategory category);

    void init(PassRefPtr<Counter>);
    void init(PassRefPtr<Rect>);
    void init(PassRefPtr<Pair>);
    void init(PassRefPtr<Quad>);
    void init(PassRefPtr<DashboardRegion>); // FIXME: Dashboard region should not be a primitive value.
    void init(PassRefPtr<CSSBasicShape>);
    void init(PassRefPtr<CSSCalcValue>);
    bool getDoubleValueInternal(UnitTypes targetUnitType, double* result) const;

    double computeLengthDouble(const RenderStyle* currentStyle, const RenderStyle* rootStyle, float multiplier, bool computingFontSize) const;

    union {
        CSSPropertyID propertyID;
        CSSValueID valueID;
        int parserOperator;
        double num;
        StringImpl* string;
        Counter* counter;
        Rect* rect;
        Quad* quad;
        unsigned rgbcolor;
        Pair* pair;
        DashboardRegion* region;
        CSSBasicShape* shape;
        CSSCalcValue* calc;
    } m_value;
};

} // namespace WebCore

#endif // CSSPrimitiveValue_h
