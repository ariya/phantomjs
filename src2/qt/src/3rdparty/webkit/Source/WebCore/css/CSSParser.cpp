/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "CSSParser.h"

#include "CSSBorderImageValue.h"
#include "CSSCanvasValue.h"
#include "CSSCharsetRule.h"
#include "CSSCursorImageValue.h"
#include "CSSFontFaceRule.h"
#include "CSSFontFaceSrcValue.h"
#include "CSSGradientValue.h"
#include "CSSImageValue.h"
#include "CSSImportRule.h"
#include "CSSInheritedValue.h"
#include "CSSInitialValue.h"
#include "CSSLineBoxContainValue.h"
#include "CSSMediaRule.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPageRule.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueCache.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSQuirkPrimitiveValue.h"
#include "CSSReflectValue.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CSSTimingFunctionValue.h"
#include "CSSUnicodeRangeValue.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Counter.h"
#include "Document.h"
#include "FloatConversion.h"
#include "FontFamilyValue.h"
#include "FontValue.h"
#include "HTMLParserIdioms.h"
#include "HashTools.h"
#include "MediaList.h"
#include "MediaQueryExp.h"
#include "Page.h"
#include "Pair.h"
#include "Rect.h"
#include "RenderTheme.h"
#include "ShadowValue.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"
#include "WebKitCSSTransformValue.h"
#include <limits.h>
#include <wtf/HexNumber.h>
#include <wtf/dtoa.h>
#include <wtf/text/StringBuffer.h>

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

#define YYDEBUG 0

#if YYDEBUG > 0
extern int cssyydebug;
#endif

extern int cssyyparse(void* parser);

using namespace std;
using namespace WTF;

namespace WebCore {

static const unsigned INVALID_NUM_PARSED_PROPERTIES = UINT_MAX;
static const double MAX_SCALE = 1000000;

static bool equal(const CSSParserString& a, const char* b)
{
    for (int i = 0; i < a.length; ++i) {
        if (!b[i])
            return false;
        if (a.characters[i] != b[i])
            return false;
    }
    return !b[a.length];
}

static bool equalIgnoringCase(const CSSParserString& a, const char* b)
{
    for (int i = 0; i < a.length; ++i) {
        if (!b[i])
            return false;
        ASSERT(!isASCIIUpper(b[i]));
        if (toASCIILower(a.characters[i]) != b[i])
            return false;
    }
    return !b[a.length];
}

static bool hasPrefix(const char* string, unsigned length, const char* prefix)
{
    for (unsigned i = 0; i < length; ++i) {
        if (!prefix[i])
            return true;
        if (string[i] != prefix[i])
            return false;
    }
    return false;
}

CSSParser::CSSParser(bool strictParsing)
    : m_strict(strictParsing)
    , m_important(false)
    , m_id(0)
    , m_styleSheet(0)
    , m_valueList(0)
    , m_parsedProperties(static_cast<CSSProperty**>(fastMalloc(32 * sizeof(CSSProperty*))))
    , m_numParsedProperties(0)
    , m_maxParsedProperties(32)
    , m_numParsedPropertiesBeforeMarginBox(INVALID_NUM_PARSED_PROPERTIES)
    , m_inParseShorthand(0)
    , m_currentShorthand(0)
    , m_implicitShorthand(false)
    , m_hasFontFaceOnlyValues(false)
    , m_hadSyntacticallyValidCSSRule(false)
    , m_defaultNamespace(starAtom)
    , m_inStyleRuleOrDeclaration(false)
    , m_selectorListRange(0, 0)
    , m_ruleBodyRange(0, 0)
    , m_propertyRange(UINT_MAX, UINT_MAX)
    , m_ruleRangeMap(0)
    , m_currentRuleData(0)
    , m_data(0)
    , yy_start(1)
    , m_lineNumber(0)
    , m_lastSelectorLineNumber(0)
    , m_allowImportRules(true)
    , m_allowNamespaceDeclarations(true)
{
#if YYDEBUG > 0
    cssyydebug = 1;
#endif
    CSSPropertySourceData::init();
}

CSSParser::~CSSParser()
{
    clearProperties();
    fastFree(m_parsedProperties);

    delete m_valueList;

    fastFree(m_data);

    fastDeleteAllValues(m_floatingSelectors);
    deleteAllValues(m_floatingSelectorVectors);
    deleteAllValues(m_floatingValueLists);
    deleteAllValues(m_floatingFunctions);
}

void CSSParserString::lower()
{
    // FIXME: If we need Unicode lowercasing here, then we probably want the real kind
    // that can potentially change the length of the string rather than the character
    // by character kind. If we don't need Unicode lowercasing, it would be good to
    // simplify this function.

    if (charactersAreAllASCII(characters, length)) {
        // Fast case for all-ASCII.
        for (int i = 0; i < length; i++)
            characters[i] = toASCIILower(characters[i]);
    } else {
        for (int i = 0; i < length; i++)
            characters[i] = Unicode::toLower(characters[i]);
    }
}

void CSSParser::setupParser(const char* prefix, const String& string, const char* suffix)
{
    int length = string.length() + strlen(prefix) + strlen(suffix) + 2;

    fastFree(m_data);
    m_data = static_cast<UChar*>(fastMalloc(length * sizeof(UChar)));
    for (unsigned i = 0; i < strlen(prefix); i++)
        m_data[i] = prefix[i];

    memcpy(m_data + strlen(prefix), string.characters(), string.length() * sizeof(UChar));

    unsigned start = strlen(prefix) + string.length();
    unsigned end = start + strlen(suffix);
    for (unsigned i = start; i < end; i++)
        m_data[i] = suffix[i - start];

    m_data[length - 1] = 0;
    m_data[length - 2] = 0;

    yy_hold_char = 0;
    yyleng = 0;
    yytext = yy_c_buf_p = m_data;
    yy_hold_char = *yy_c_buf_p;
    resetRuleBodyMarks();
}

void CSSParser::parseSheet(CSSStyleSheet* sheet, const String& string, int startLineNumber, StyleRuleRangeMap* ruleRangeMap)
{
    setStyleSheet(sheet);
    m_defaultNamespace = starAtom; // Reset the default namespace.
    m_ruleRangeMap = ruleRangeMap;
    if (ruleRangeMap) {
        m_currentRuleData = CSSRuleSourceData::create();
        m_currentRuleData->styleSourceData = CSSStyleSourceData::create();
    }

    m_lineNumber = startLineNumber;
    setupParser("", string, "");
    cssyyparse(this);
    m_ruleRangeMap = 0;
    m_currentRuleData = 0;
    m_rule = 0;
}

PassRefPtr<CSSRule> CSSParser::parseRule(CSSStyleSheet* sheet, const String& string)
{
    setStyleSheet(sheet);
    m_allowNamespaceDeclarations = false;
    setupParser("@-webkit-rule{", string, "} ");
    cssyyparse(this);
    return m_rule.release();
}

PassRefPtr<CSSRule> CSSParser::parseKeyframeRule(CSSStyleSheet *sheet, const String &string)
{
    setStyleSheet(sheet);
    setupParser("@-webkit-keyframe-rule{ ", string, "} ");
    cssyyparse(this);
    return m_keyframe.release();
}

static inline bool isColorPropertyID(int propertyId)
{
    switch (propertyId) {
    case CSSPropertyColor:
    case CSSPropertyBackgroundColor:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderTopColor:
    case CSSPropertyOutlineColor:
    case CSSPropertyTextLineThroughColor:
    case CSSPropertyTextOverlineColor:
    case CSSPropertyTextUnderlineColor:
    case CSSPropertyWebkitBorderAfterColor:
    case CSSPropertyWebkitBorderBeforeColor:
    case CSSPropertyWebkitBorderEndColor:
    case CSSPropertyWebkitBorderStartColor:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextStrokeColor:
        return true;
    default:
        return false;
    }
}

static bool parseColorValue(CSSMutableStyleDeclaration* declaration, int propertyId, const String& string, bool important, bool strict)
{
    if (!string.length())
        return false;
    if (!isColorPropertyID(propertyId))
        return false;
    CSSParserString cssString;
    cssString.characters = const_cast<UChar*>(string.characters());
    cssString.length = string.length();
    int valueID = cssValueKeywordID(cssString);
    bool validPrimitive = false;
    if (valueID == CSSValueWebkitText)
        validPrimitive = true;
    else if (valueID == CSSValueCurrentcolor)
        validPrimitive = true;
    else if ((valueID >= CSSValueAqua && valueID <= CSSValueWindowtext) || valueID == CSSValueMenu
             || (valueID >= CSSValueWebkitFocusRingColor && valueID < CSSValueWebkitText && !strict)) {
        validPrimitive = true;
    }

    CSSStyleSheet* stylesheet = static_cast<CSSStyleSheet*>(declaration->stylesheet());
    if (!stylesheet || !stylesheet->document())
        return false;
    if (validPrimitive) {
        CSSProperty property(propertyId, stylesheet->document()->cssPrimitiveValueCache()->createIdentifierValue(valueID), important);
        declaration->addParsedProperty(property);
        return true;
    }
    RGBA32 color;
    if (!CSSParser::parseColor(string, color, strict && string[0] != '#'))
        return false;
    CSSProperty property(propertyId, stylesheet->document()->cssPrimitiveValueCache()->createColorValue(color), important);
    declaration->addParsedProperty(property);
    return true;
}

static inline bool isSimpleLengthPropertyID(int propertyId, bool& acceptsNegativeNumbers)
{
    switch (propertyId) {
    case CSSPropertyFontSize:
    case CSSPropertyHeight:
    case CSSPropertyWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight: 
    case CSSPropertyPaddingTop:
    case CSSPropertyWebkitLogicalWidth:
    case CSSPropertyWebkitLogicalHeight:
    case CSSPropertyWebkitMinLogicalWidth:
    case CSSPropertyWebkitMinLogicalHeight:
    case CSSPropertyWebkitPaddingAfter:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingStart:
        acceptsNegativeNumbers = false;
        return true;
    case CSSPropertyBottom:
    case CSSPropertyLeft:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft: 
    case CSSPropertyMarginRight: 
    case CSSPropertyMarginTop:
    case CSSPropertyRight:
    case CSSPropertyTextIndent:
    case CSSPropertyTop:
    case CSSPropertyWebkitMarginAfter:
    case CSSPropertyWebkitMarginBefore:
    case CSSPropertyWebkitMarginEnd:
    case CSSPropertyWebkitMarginStart:
        acceptsNegativeNumbers = true;
        return true;
    default:
        return false;
    }
}

static bool parseSimpleLengthValue(CSSMutableStyleDeclaration* declaration, int propertyId, const String& string, bool important, bool strict)
{
    const UChar* characters = string.characters();
    unsigned length = string.length();
    if (!characters || !length)
        return false;
    bool acceptsNegativeNumbers;
    if (!isSimpleLengthPropertyID(propertyId, acceptsNegativeNumbers))
        return false;

    CSSPrimitiveValue::UnitTypes unit = CSSPrimitiveValue::CSS_NUMBER;
    if (length > 2 && characters[length - 2] == 'p' && characters[length - 1] == 'x') {
        length -= 2;
        unit = CSSPrimitiveValue::CSS_PX;
    } else if (length > 1 && characters[length - 1] == '%') {
        length -= 1;
        unit = CSSPrimitiveValue::CSS_PERCENTAGE;
    }

    // We rely on charactersToDouble for validation as well. The function
    // will set "ok" to "false" if the entire passed-in character range does
    // not represent a double.
    bool ok;
    double number = charactersToDouble(characters, length, &ok);
    if (!ok)
        return false;
    if (unit == CSSPrimitiveValue::CSS_NUMBER) {
        if (number && strict)
            return false;
        unit = CSSPrimitiveValue::CSS_PX;
    }
    if (number < 0 && !acceptsNegativeNumbers)
        return false;

    CSSStyleSheet* stylesheet = static_cast<CSSStyleSheet*>(declaration->stylesheet());
    if (!stylesheet || !stylesheet->document())
        return false;
    CSSProperty property(propertyId, stylesheet->document()->cssPrimitiveValueCache()->createValue(number, unit), important);
    declaration->addParsedProperty(property);
    return true;
}

bool CSSParser::parseValue(CSSMutableStyleDeclaration* declaration, int propertyId, const String& string, bool important, bool strict)
{
    if (parseSimpleLengthValue(declaration, propertyId, string, important, strict))
        return true;
    if (parseColorValue(declaration, propertyId, string, important, strict))
        return true;
    CSSParser parser(strict);
    return parser.parseValue(declaration, propertyId, string, important);
}

bool CSSParser::parseValue(CSSMutableStyleDeclaration* declaration, int propertyId, const String& string, bool important)
{
    ASSERT(!declaration->stylesheet() || declaration->stylesheet()->isCSSStyleSheet());
    setStyleSheet(static_cast<CSSStyleSheet*>(declaration->stylesheet()));

    setupParser("@-webkit-value{", string, "} ");

    m_id = propertyId;
    m_important = important;

    cssyyparse(this);

    m_rule = 0;

    bool ok = false;
    if (m_hasFontFaceOnlyValues)
        deleteFontFaceOnlyValues();
    if (m_numParsedProperties) {
        ok = true;
        declaration->addParsedProperties(m_parsedProperties, m_numParsedProperties);
        clearProperties();
    }

    return ok;
}

// color will only be changed when string contains a valid css color, making it
// possible to set up a default color.
bool CSSParser::parseColor(RGBA32& color, const String& string, bool strict)
{
    // First try creating a color specified by name, rgba(), rgb() or "#" syntax.
    if (parseColor(string, color, strict))
        return true;

    CSSParser parser(true);
    RefPtr<CSSMutableStyleDeclaration> dummyStyleDeclaration = CSSMutableStyleDeclaration::create();

    // Now try to create a color from rgba() syntax.
    if (!parser.parseColor(dummyStyleDeclaration.get(), string))
        return false;

    CSSValue* value = parser.m_parsedProperties[0]->value();
    if (value->cssValueType() != CSSValue::CSS_PRIMITIVE_VALUE)
        return false;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_RGBCOLOR)
        return false;

    color = primitiveValue->getRGBA32Value();
    return true;
}

bool CSSParser::parseColor(CSSMutableStyleDeclaration* declaration, const String& string)
{
    ASSERT(!declaration->stylesheet() || declaration->stylesheet()->isCSSStyleSheet());
    setStyleSheet(static_cast<CSSStyleSheet*>(declaration->stylesheet()));

    setupParser("@-webkit-decls{color:", string, "} ");
    cssyyparse(this);
    m_rule = 0;

    return (m_numParsedProperties && m_parsedProperties[0]->m_id == CSSPropertyColor);
}

bool CSSParser::parseSystemColor(RGBA32& color, const String& string, Document* document)
{
    if (!document || !document->page())
        return false;

    CSSParserString cssColor;
    cssColor.characters = const_cast<UChar*>(string.characters());
    cssColor.length = string.length();
    int id = cssValueKeywordID(cssColor);
    if (id <= 0)
        return false;

    color = document->page()->theme()->systemColor(id).rgb();
    return true;
}

void CSSParser::parseSelector(const String& string, Document* doc, CSSSelectorList& selectorList)
{
    RefPtr<CSSStyleSheet> dummyStyleSheet = CSSStyleSheet::create(doc);

    setStyleSheet(dummyStyleSheet.get());
    m_selectorListForParseSelector = &selectorList;

    setupParser("@-webkit-selector{", string, "}");

    cssyyparse(this);

    m_selectorListForParseSelector = 0;

    // The style sheet will be deleted right away, so it won't outlive the document.
    ASSERT(dummyStyleSheet->hasOneRef());
}

bool CSSParser::parseDeclaration(CSSMutableStyleDeclaration* declaration, const String& string, RefPtr<CSSStyleSourceData>* styleSourceData)
{
    // Length of the "@-webkit-decls{" prefix.
    static const unsigned prefixLength = 15;

    ASSERT(!declaration->stylesheet() || declaration->stylesheet()->isCSSStyleSheet());
    setStyleSheet(static_cast<CSSStyleSheet*>(declaration->stylesheet()));
    if (styleSourceData) {
        m_currentRuleData = CSSRuleSourceData::create();
        m_currentRuleData->styleSourceData = CSSStyleSourceData::create();
        m_inStyleRuleOrDeclaration = true;
    }

    setupParser("@-webkit-decls{", string, "} ");
    cssyyparse(this);
    m_rule = 0;

    bool ok = false;
    if (m_hasFontFaceOnlyValues)
        deleteFontFaceOnlyValues();
    if (m_numParsedProperties) {
        ok = true;
        declaration->addParsedProperties(m_parsedProperties, m_numParsedProperties);
        clearProperties();
    }

    if (m_currentRuleData) {
        m_currentRuleData->styleSourceData->styleBodyRange.start = 0;
        m_currentRuleData->styleSourceData->styleBodyRange.end = string.length();
        for (Vector<CSSPropertySourceData>::iterator it = m_currentRuleData->styleSourceData->propertyData.begin(), endIt = m_currentRuleData->styleSourceData->propertyData.end(); it != endIt; ++it) {
            (*it).range.start -= prefixLength;
            (*it).range.end -= prefixLength;
        }
    }

    if (styleSourceData) {
        *styleSourceData = m_currentRuleData->styleSourceData.release();
        m_currentRuleData = 0;
        m_inStyleRuleOrDeclaration = false;
    }
    return ok;
}

bool CSSParser::parseMediaQuery(MediaList* queries, const String& string)
{
    if (string.isEmpty())
        return true;

    ASSERT(!m_mediaQuery);

    // can't use { because tokenizer state switches from mediaquery to initial state when it sees { token.
    // instead insert one " " (which is WHITESPACE in CSSGrammar.y)
    setupParser("@-webkit-mediaquery ", string, "} ");
    cssyyparse(this);

    bool ok = false;
    if (m_mediaQuery) {
        ok = true;
        queries->appendMediaQuery(m_mediaQuery.release());
    }

    return ok;
}


void CSSParser::addProperty(int propId, PassRefPtr<CSSValue> value, bool important)
{
    OwnPtr<CSSProperty> prop(adoptPtr(new CSSProperty(propId, value, important, m_currentShorthand, m_implicitShorthand)));
    if (m_numParsedProperties >= m_maxParsedProperties) {
        m_maxParsedProperties += 32;
        if (m_maxParsedProperties > UINT_MAX / sizeof(CSSProperty*))
            return;
        m_parsedProperties = static_cast<CSSProperty**>(fastRealloc(m_parsedProperties,
            m_maxParsedProperties * sizeof(CSSProperty*)));
    }
    m_parsedProperties[m_numParsedProperties++] = prop.leakPtr();
}

void CSSParser::rollbackLastProperties(int num)
{
    ASSERT(num >= 0);
    ASSERT(m_numParsedProperties >= static_cast<unsigned>(num));

    for (int i = 0; i < num; ++i)
        delete m_parsedProperties[--m_numParsedProperties];
}

void CSSParser::clearProperties()
{
    for (unsigned i = 0; i < m_numParsedProperties; i++)
        delete m_parsedProperties[i];
    m_numParsedProperties = 0;
    m_numParsedPropertiesBeforeMarginBox = INVALID_NUM_PARSED_PROPERTIES;
    m_hasFontFaceOnlyValues = false;
}
    
void CSSParser::setStyleSheet(CSSStyleSheet* styleSheet)
{
    m_styleSheet = styleSheet;
    m_primitiveValueCache = document() ? document()->cssPrimitiveValueCache() : CSSPrimitiveValueCache::create();
}

Document* CSSParser::document() const
{
    StyleBase* root = m_styleSheet;
    while (root && root->parent())
        root = root->parent();
    if (!root)
        return 0;
    if (!root->isCSSStyleSheet())
        return 0;
    return static_cast<CSSStyleSheet*>(root)->document();
}

bool CSSParser::validUnit(CSSParserValue* value, Units unitflags, bool strict)
{
    bool b = false;
    switch (value->unit) {
    case CSSPrimitiveValue::CSS_NUMBER:
        b = (unitflags & FNumber);
        if (!b && ((unitflags & (FLength | FAngle | FTime)) && (value->fValue == 0 || !strict))) {
            value->unit = (unitflags & FLength) ? CSSPrimitiveValue::CSS_PX :
                          ((unitflags & FAngle) ? CSSPrimitiveValue::CSS_DEG : CSSPrimitiveValue::CSS_MS);
            b = true;
        }
        if (!b && (unitflags & FInteger) && value->isInt)
            b = true;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        b = (unitflags & FPercent);
        break;
    case CSSParserValue::Q_EMS:
    case CSSPrimitiveValue::CSS_EMS:
    case CSSPrimitiveValue::CSS_REMS:
    case CSSPrimitiveValue::CSS_EXS:
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PC:
        b = (unitflags & FLength);
        break;
    case CSSPrimitiveValue::CSS_MS:
    case CSSPrimitiveValue::CSS_S:
        b = (unitflags & FTime);
        break;
    case CSSPrimitiveValue::CSS_DEG:
    case CSSPrimitiveValue::CSS_RAD:
    case CSSPrimitiveValue::CSS_GRAD:
    case CSSPrimitiveValue::CSS_TURN:
        b = (unitflags & FAngle);
        break;
    case CSSPrimitiveValue::CSS_HZ:
    case CSSPrimitiveValue::CSS_KHZ:
    case CSSPrimitiveValue::CSS_DIMENSION:
    default:
        break;
    }
    if (b && unitflags & FNonNeg && value->fValue < 0)
        b = false;
    return b;
}

static int unitFromString(CSSParserValue* value)
{
    if (value->unit != CSSPrimitiveValue::CSS_IDENT || value->id)
        return 0;

    if (equal(value->string, "em"))
        return CSSPrimitiveValue::CSS_EMS;
    if (equal(value->string, "rem"))
        return CSSPrimitiveValue::CSS_REMS;
    if (equal(value->string, "ex"))
        return CSSPrimitiveValue::CSS_EXS;
    if (equal(value->string, "px"))
        return CSSPrimitiveValue::CSS_PX;
    if (equal(value->string, "cm"))
        return CSSPrimitiveValue::CSS_CM;
    if (equal(value->string, "mm"))
        return CSSPrimitiveValue::CSS_MM;
    if (equal(value->string, "in"))
        return CSSPrimitiveValue::CSS_IN;
    if (equal(value->string, "pt"))
        return CSSPrimitiveValue::CSS_PT;
    if (equal(value->string, "pc"))
        return CSSPrimitiveValue::CSS_PC;
    if (equal(value->string, "deg"))
        return CSSPrimitiveValue::CSS_DEG;
    if (equal(value->string, "rad"))
        return CSSPrimitiveValue::CSS_RAD;
    if (equal(value->string, "grad"))
        return CSSPrimitiveValue::CSS_GRAD;
    if (equal(value->string, "turn"))
        return CSSPrimitiveValue::CSS_TURN;
    if (equal(value->string, "ms"))
        return CSSPrimitiveValue::CSS_MS;
    if (equal(value->string, "s"))
        return CSSPrimitiveValue::CSS_S;
    if (equal(value->string, "Hz"))
        return CSSPrimitiveValue::CSS_HZ;
    if (equal(value->string, "kHz"))
        return CSSPrimitiveValue::CSS_KHZ;

    return 0;
}

void CSSParser::checkForOrphanedUnits()
{
    if (m_strict || inShorthand())
        return;

    // The purpose of this code is to implement the WinIE quirk that allows unit types to be separated from their numeric values
    // by whitespace, so e.g., width: 20 px instead of width:20px.  This is invalid CSS, so we don't do this in strict mode.
    CSSParserValue* numericVal = 0;
    unsigned size = m_valueList->size();
    for (unsigned i = 0; i < size; i++) {
        CSSParserValue* value = m_valueList->valueAt(i);

        if (numericVal) {
            // Change the unit type of the numeric val to match.
            int unit = unitFromString(value);
            if (unit) {
                numericVal->unit = unit;
                numericVal = 0;

                // Now delete the bogus unit value.
                m_valueList->deleteValueAt(i);
                i--; // We're safe even though |i| is unsigned, since we only hit this code if we had a previous numeric value (so |i| is always > 0 here).
                size--;
                continue;
            }
        }

        numericVal = (value->unit == CSSPrimitiveValue::CSS_NUMBER) ? value : 0;
    }
}

bool CSSParser::parseValue(int propId, bool important)
{
    if (!m_valueList)
        return false;

    CSSParserValue* value = m_valueList->current();

    if (!value)
        return false;

    int id = value->id;

    // In quirks mode, we will look for units that have been incorrectly separated from the number they belong to
    // by a space.  We go ahead and associate the unit with the number even though it is invalid CSS.
    checkForOrphanedUnits();

    int num = inShorthand() ? 1 : m_valueList->size();

    if (id == CSSValueInherit) {
        if (num != 1)
            return false;
        addProperty(propId, CSSInheritedValue::create(), important);
        return true;
    }
    else if (id == CSSValueInitial) {
        if (num != 1)
            return false;
        addProperty(propId, CSSInitialValue::createExplicit(), important);
        return true;
    }

    bool validPrimitive = false;
    RefPtr<CSSValue> parsedValue;

    switch (static_cast<CSSPropertyID>(propId)) {
        /* The comment to the left defines all valid value of this properties as defined
         * in CSS 2, Appendix F. Property index
         */

        /* All the CSS properties are not supported by the renderer at the moment.
         * Note that all the CSS2 Aural properties are only checked, if CSS_AURAL is defined
         * (see parseAuralValues). As we don't support them at all this seems reasonable.
         */

    case CSSPropertySize:                 // <length>{1,2} | auto | [ <page-size> || [ portrait | landscape] ]
        return parseSize(propId, important);

    case CSSPropertyQuotes:               // [<string> <string>]+ | none | inherit
        if (id)
            validPrimitive = true;
        else
            return parseQuotes(propId, important);
        break;
    case CSSPropertyUnicodeBidi: // normal | embed | bidi-override | isolate | inherit
        if (id == CSSValueNormal
            || id == CSSValueEmbed
            || id == CSSValueBidiOverride
            || id == CSSValueWebkitIsolate)
            validPrimitive = true;
        break;

    case CSSPropertyPosition:             // static | relative | absolute | fixed | inherit
        if (id == CSSValueStatic ||
             id == CSSValueRelative ||
             id == CSSValueAbsolute ||
             id == CSSValueFixed)
            validPrimitive = true;
        break;

    case CSSPropertyPageBreakAfter:     // auto | always | avoid | left | right | inherit
    case CSSPropertyPageBreakBefore:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
        if (id == CSSValueAuto ||
             id == CSSValueAlways ||
             id == CSSValueAvoid ||
             id == CSSValueLeft ||
             id == CSSValueRight)
            validPrimitive = true;
        break;

    case CSSPropertyPageBreakInside:    // avoid | auto | inherit
    case CSSPropertyWebkitColumnBreakInside:
        if (id == CSSValueAuto || id == CSSValueAvoid)
            validPrimitive = true;
        break;

    case CSSPropertyEmptyCells:          // show | hide | inherit
        if (id == CSSValueShow ||
             id == CSSValueHide)
            validPrimitive = true;
        break;

    case CSSPropertyContent:              // [ <string> | <uri> | <counter> | attr(X) | open-quote |
        // close-quote | no-open-quote | no-close-quote ]+ | inherit
        return parseContent(propId, important);

    case CSSPropertyWhiteSpace:          // normal | pre | nowrap | inherit
        if (id == CSSValueNormal ||
            id == CSSValuePre ||
            id == CSSValuePreWrap ||
            id == CSSValuePreLine ||
            id == CSSValueNowrap)
            validPrimitive = true;
        break;

    case CSSPropertyClip:                 // <shape> | auto | inherit
        if (id == CSSValueAuto)
            validPrimitive = true;
        else if (value->unit == CSSParserValue::Function)
            return parseShape(propId, important);
        break;

    /* Start of supported CSS properties with validation. This is needed for parseShorthand to work
     * correctly and allows optimization in WebCore::applyRule(..)
     */
    case CSSPropertyCaptionSide:         // top | bottom | left | right | inherit
        if (id == CSSValueLeft || id == CSSValueRight ||
            id == CSSValueTop || id == CSSValueBottom)
            validPrimitive = true;
        break;

    case CSSPropertyBorderCollapse:      // collapse | separate | inherit
        if (id == CSSValueCollapse || id == CSSValueSeparate)
            validPrimitive = true;
        break;

    case CSSPropertyVisibility:           // visible | hidden | collapse | inherit
        if (id == CSSValueVisible || id == CSSValueHidden || id == CSSValueCollapse)
            validPrimitive = true;
        break;

    case CSSPropertyOverflow: {
        ShorthandScope scope(this, propId);
        if (num != 1 || !parseValue(CSSPropertyOverflowX, important))
            return false;
        CSSValue* value = m_parsedProperties[m_numParsedProperties - 1]->value();
        addProperty(CSSPropertyOverflowY, value, important);
        return true;
    }
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:           // visible | hidden | scroll | auto | marquee | overlay | inherit
        if (id == CSSValueVisible || id == CSSValueHidden || id == CSSValueScroll || id == CSSValueAuto ||
            id == CSSValueOverlay || id == CSSValueWebkitMarquee)
            validPrimitive = true;
        break;

    case CSSPropertyListStylePosition:  // inside | outside | inherit
        if (id == CSSValueInside || id == CSSValueOutside)
            validPrimitive = true;
        break;

    case CSSPropertyListStyleType:
        // See section CSS_PROP_LIST_STYLE_TYPE of file CSSValueKeywords.in
        // for the list of supported list-style-types.
        if ((id >= CSSValueDisc && id <= CSSValueKatakanaIroha) || id == CSSValueNone)
            validPrimitive = true;
        break;

    case CSSPropertyDisplay:
        // inline | block | list-item | run-in | inline-block | table |
        // inline-table | table-row-group | table-header-group | table-footer-group | table-row |
        // table-column-group | table-column | table-cell | table-caption | box | inline-box | none | inherit
#if ENABLE(WCSS)
        if ((id >= CSSValueInline && id <= CSSValueWapMarquee) || id == CSSValueNone)
#else
        if ((id >= CSSValueInline && id <= CSSValueWebkitInlineBox) || id == CSSValueNone)
#endif
            validPrimitive = true;
        break;

    case CSSPropertyDirection:            // ltr | rtl | inherit
        if (id == CSSValueLtr || id == CSSValueRtl)
            validPrimitive = true;
        break;

    case CSSPropertyTextTransform:       // capitalize | uppercase | lowercase | none | inherit
        if ((id >= CSSValueCapitalize && id <= CSSValueLowercase) || id == CSSValueNone)
            validPrimitive = true;
        break;

    case CSSPropertyFloat:                // left | right | none | inherit + center for buggy CSS
        if (id == CSSValueLeft || id == CSSValueRight ||
             id == CSSValueNone || id == CSSValueCenter)
            validPrimitive = true;
        break;

    case CSSPropertyClear:                // none | left | right | both | inherit
        if (id == CSSValueNone || id == CSSValueLeft ||
             id == CSSValueRight|| id == CSSValueBoth)
            validPrimitive = true;
        break;

    case CSSPropertyTextAlign:
        // left | right | center | justify | webkit_left | webkit_right | webkit_center | webkit_match_parent |
        // start | end | <string> | inherit
        if ((id >= CSSValueWebkitAuto && id <= CSSValueWebkitMatchParent) || id == CSSValueStart || id == CSSValueEnd
             || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyOutlineStyle:        // (<border-style> except hidden) | auto | inherit
        if (id == CSSValueAuto || id == CSSValueNone || (id >= CSSValueInset && id <= CSSValueDouble))
            validPrimitive = true;
        break;

    case CSSPropertyBorderTopStyle:     //// <border-style> | inherit
    case CSSPropertyBorderRightStyle:   //   Defined as:    none | hidden | dotted | dashed |
    case CSSPropertyBorderBottomStyle:  //   solid | double | groove | ridge | inset | outset
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitColumnRuleStyle:
        if (id >= CSSValueNone && id <= CSSValueDouble)
            validPrimitive = true;
        break;

    case CSSPropertyFontWeight:  // normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
        return parseFontWeight(important);

    case CSSPropertyBorderSpacing: {
        const int properties[2] = { CSSPropertyWebkitBorderHorizontalSpacing,
                                    CSSPropertyWebkitBorderVerticalSpacing };
        if (num == 1) {
            ShorthandScope scope(this, CSSPropertyBorderSpacing);
            if (!parseValue(properties[0], important))
                return false;
            CSSValue* value = m_parsedProperties[m_numParsedProperties-1]->value();
            addProperty(properties[1], value, important);
            return true;
        }
        else if (num == 2) {
            ShorthandScope scope(this, CSSPropertyBorderSpacing);
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important))
                return false;
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
        validPrimitive = validUnit(value, FLength | FNonNeg, m_strict);
        break;
    case CSSPropertyOutlineColor:        // <color> | invert | inherit
        // Outline color has "invert" as additional keyword.
        // Also, we want to allow the special focus color even in strict parsing mode.
        if (id == CSSValueInvert || id == CSSValueWebkitFocusRingColor) {
            validPrimitive = true;
            break;
        }
        /* nobreak */
    case CSSPropertyBackgroundColor: // <color> | inherit
    case CSSPropertyBorderTopColor: // <color> | inherit
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyWebkitBorderStartColor:
    case CSSPropertyWebkitBorderEndColor:
    case CSSPropertyWebkitBorderBeforeColor:
    case CSSPropertyWebkitBorderAfterColor:
    case CSSPropertyColor: // <color> | inherit
    case CSSPropertyTextLineThroughColor: // CSS3 text decoration colors
    case CSSPropertyTextUnderlineColor:
    case CSSPropertyTextOverlineColor:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextStrokeColor:
        if (id == CSSValueWebkitText)
            validPrimitive = true; // Always allow this, even when strict parsing is on,
                                    // since we use this in our UA sheets.
        else if (id == CSSValueCurrentcolor)
            validPrimitive = true;
        else if ((id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu ||
             (id >= CSSValueWebkitFocusRingColor && id < CSSValueWebkitText && !m_strict)) {
            validPrimitive = true;
        } else {
            parsedValue = parseColor();
            if (parsedValue)
                m_valueList->next();
        }
        break;

    case CSSPropertyCursor: {
        // [<uri>,]*  [ auto | crosshair | default | pointer | progress | move | e-resize | ne-resize |
        // nw-resize | n-resize | se-resize | sw-resize | s-resize | w-resize | ew-resize |
        // ns-resize | nesw-resize | nwse-resize | col-resize | row-resize | text | wait | help |
        // vertical-text | cell | context-menu | alias | copy | no-drop | not-allowed | -webkit-zoom-in
        // -webkit-zoom-out | all-scroll | -webkit-grab | -webkit-grabbing ] ] | inherit
        RefPtr<CSSValueList> list;
        while (value && value->unit == CSSPrimitiveValue::CSS_URI) {
            if (!list)
                list = CSSValueList::createCommaSeparated();
            String uri = value->string;
            Vector<int> coords;
            value = m_valueList->next();
            while (value && value->unit == CSSPrimitiveValue::CSS_NUMBER) {
                coords.append(int(value->fValue));
                value = m_valueList->next();
            }
            IntPoint hotSpot(-1, -1);
            int nrcoords = coords.size();
            if (nrcoords > 0 && nrcoords != 2)
                return false;
            if (nrcoords == 2)
                hotSpot = IntPoint(coords[0], coords[1]);

            if (!uri.isNull() && m_styleSheet) {
                // FIXME: The completeURL call should be done when using the CSSCursorImageValue,
                // not when creating it.
                list->append(CSSCursorImageValue::create(m_styleSheet->completeURL(uri), hotSpot));
            }

            if ((m_strict && !value) || (value && !(value->unit == CSSParserValue::Operator && value->iValue == ',')))
                return false;
            value = m_valueList->next(); // comma
        }
        if (list) {
            if (!value) { // no value after url list (MSIE 5 compatibility)
                if (list->length() != 1)
                    return false;
            } else if (!m_strict && value->id == CSSValueHand) // MSIE 5 compatibility :/
                list->append(primitiveValueCache()->createIdentifierValue(CSSValuePointer));
            else if (value && ((value->id >= CSSValueAuto && value->id <= CSSValueWebkitGrabbing) || value->id == CSSValueCopy || value->id == CSSValueNone))
                list->append(primitiveValueCache()->createIdentifierValue(value->id));
            m_valueList->next();
            parsedValue = list.release();
            break;
        }
        id = value->id;
        if (!m_strict && value->id == CSSValueHand) { // MSIE 5 compatibility :/
            id = CSSValuePointer;
            validPrimitive = true;
        } else if ((value->id >= CSSValueAuto && value->id <= CSSValueWebkitGrabbing) || value->id == CSSValueCopy || value->id == CSSValueNone)
            validPrimitive = true;
        break;
    }

    case CSSPropertyBackgroundAttachment:
    case CSSPropertyBackgroundClip:
    case CSSPropertyWebkitBackgroundClip:
    case CSSPropertyWebkitBackgroundComposite:
    case CSSPropertyBackgroundImage:
    case CSSPropertyBackgroundOrigin:
    case CSSPropertyWebkitBackgroundOrigin:
    case CSSPropertyBackgroundPosition:
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyBackgroundSize:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyBackgroundRepeat:
    case CSSPropertyBackgroundRepeatX:
    case CSSPropertyBackgroundRepeatY:
    case CSSPropertyWebkitMaskAttachment:
    case CSSPropertyWebkitMaskClip:
    case CSSPropertyWebkitMaskComposite:
    case CSSPropertyWebkitMaskImage:
    case CSSPropertyWebkitMaskOrigin:
    case CSSPropertyWebkitMaskPosition:
    case CSSPropertyWebkitMaskPositionX:
    case CSSPropertyWebkitMaskPositionY:
    case CSSPropertyWebkitMaskSize:
    case CSSPropertyWebkitMaskRepeat:
    case CSSPropertyWebkitMaskRepeatX:
    case CSSPropertyWebkitMaskRepeatY: {
        RefPtr<CSSValue> val1;
        RefPtr<CSSValue> val2;
        int propId1, propId2;
        bool result = false;
        if (parseFillProperty(propId, propId1, propId2, val1, val2)) {
            OwnPtr<ShorthandScope> shorthandScope;
            if (propId == CSSPropertyBackgroundPosition ||
                propId == CSSPropertyBackgroundRepeat ||
                propId == CSSPropertyWebkitMaskPosition ||
                propId == CSSPropertyWebkitMaskRepeat) {
                shorthandScope = adoptPtr(new ShorthandScope(this, propId));
            }
            addProperty(propId1, val1.release(), important);
            if (val2)
                addProperty(propId2, val2.release(), important);
            result = true;
        }
        m_implicitShorthand = false;
        return result;
    }
    case CSSPropertyListStyleImage:     // <uri> | none | inherit
        if (id == CSSValueNone) {
            parsedValue = CSSImageValue::create();
            m_valueList->next();
        } else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            if (m_styleSheet) {
                // FIXME: The completeURL call should be done when using the CSSImageValue,
                // not when creating it.
                parsedValue = CSSImageValue::create(m_styleSheet->completeURL(value->string));
                m_valueList->next();
            }
        } else if (isGeneratedImageValue(value)) {
            if (parseGeneratedImage(parsedValue))
                m_valueList->next();
            else
                return false;
        }
        break;

    case CSSPropertyWebkitTextStrokeWidth:
    case CSSPropertyOutlineWidth:        // <border-width> | inherit
    case CSSPropertyBorderTopWidth:     //// <border-width> | inherit
    case CSSPropertyBorderRightWidth:   //   Which is defined as
    case CSSPropertyBorderBottomWidth:  //   thin | medium | thick | <length>
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyWebkitBorderStartWidth:
    case CSSPropertyWebkitBorderEndWidth:
    case CSSPropertyWebkitBorderBeforeWidth:
    case CSSPropertyWebkitBorderAfterWidth:
    case CSSPropertyWebkitColumnRuleWidth:
        if (id == CSSValueThin || id == CSSValueMedium || id == CSSValueThick)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength | FNonNeg, m_strict);
        break;

    case CSSPropertyLetterSpacing:       // normal | <length> | inherit
    case CSSPropertyWordSpacing:         // normal | <length> | inherit
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength, m_strict);
        break;

    case CSSPropertyWordBreak:          // normal | break-all | break-word (this is a custom extension)
        if (id == CSSValueNormal || id == CSSValueBreakAll || id == CSSValueBreakWord)
            validPrimitive = true;
        break;

    case CSSPropertyWordWrap:           // normal | break-word
        if (id == CSSValueNormal || id == CSSValueBreakWord)
            validPrimitive = true;
        break;
    case CSSPropertySpeak:           // none | normal | spell-out | digits | literal-punctuation | no-punctuation | inherit
        if (id == CSSValueNone || id == CSSValueNormal || id == CSSValueSpellOut || id == CSSValueDigits 
            || id == CSSValueLiteralPunctuation || id == CSSValueNoPunctuation)
            validPrimitive = true;
        break;
            
    case CSSPropertyTextIndent:          // <length> | <percentage> | inherit
        validPrimitive = (!id && validUnit(value, FLength | FPercent, m_strict));
        break;

    case CSSPropertyPaddingTop:          //// <padding-width> | inherit
    case CSSPropertyPaddingRight:        //   Which is defined as
    case CSSPropertyPaddingBottom:       //   <length> | <percentage>
    case CSSPropertyPaddingLeft:         ////
    case CSSPropertyWebkitPaddingStart:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingAfter:
        validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg, m_strict));
        break;

    case CSSPropertyMaxHeight:           // <length> | <percentage> | none | inherit
    case CSSPropertyMaxWidth:            // <length> | <percentage> | none | inherit
    case CSSPropertyWebkitMaxLogicalWidth:
    case CSSPropertyWebkitMaxLogicalHeight:
        if (id == CSSValueNone || id == CSSValueIntrinsic || id == CSSValueMinIntrinsic) {
            validPrimitive = true;
            break;
        }
        /* nobreak */
    case CSSPropertyMinHeight:           // <length> | <percentage> | inherit
    case CSSPropertyMinWidth:            // <length> | <percentage> | inherit
    case CSSPropertyWebkitMinLogicalWidth:
    case CSSPropertyWebkitMinLogicalHeight:
        if (id == CSSValueIntrinsic || id == CSSValueMinIntrinsic)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg, m_strict));
        break;

    case CSSPropertyFontSize:
        // <absolute-size> | <relative-size> | <length> | <percentage> | inherit
        if (id >= CSSValueXxSmall && id <= CSSValueLarger)
            validPrimitive = true;
        else
            validPrimitive = (validUnit(value, FLength | FPercent | FNonNeg, m_strict));
        break;

    case CSSPropertyFontStyle:           // normal | italic | oblique | inherit
        return parseFontStyle(important);

    case CSSPropertyFontVariant:         // normal | small-caps | inherit
        return parseFontVariant(important);

    case CSSPropertyVerticalAlign:
        // baseline | sub | super | top | text-top | middle | bottom | text-bottom |
        // <percentage> | <length> | inherit

        if (id >= CSSValueBaseline && id <= CSSValueWebkitBaselineMiddle)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent, m_strict));
        break;

    case CSSPropertyHeight:               // <length> | <percentage> | auto | inherit
    case CSSPropertyWidth:                // <length> | <percentage> | auto | inherit
    case CSSPropertyWebkitLogicalWidth:  
    case CSSPropertyWebkitLogicalHeight:
        if (id == CSSValueAuto || id == CSSValueIntrinsic || id == CSSValueMinIntrinsic)
            validPrimitive = true;
        else
            // ### handle multilength case where we allow relative units
            validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg, m_strict));
        break;

    case CSSPropertyBottom:               // <length> | <percentage> | auto | inherit
    case CSSPropertyLeft:                 // <length> | <percentage> | auto | inherit
    case CSSPropertyRight:                // <length> | <percentage> | auto | inherit
    case CSSPropertyTop:                  // <length> | <percentage> | auto | inherit
    case CSSPropertyMarginTop:           //// <margin-width> | inherit
    case CSSPropertyMarginRight:         //   Which is defined as
    case CSSPropertyMarginBottom:        //   <length> | <percentage> | auto | inherit
    case CSSPropertyMarginLeft:          ////
    case CSSPropertyWebkitMarginStart:
    case CSSPropertyWebkitMarginEnd:
    case CSSPropertyWebkitMarginBefore:
    case CSSPropertyWebkitMarginAfter:
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent, m_strict));
        break;

    case CSSPropertyZIndex:              // auto | <integer> | inherit
        if (id == CSSValueAuto) {
            validPrimitive = true;
            break;
        }
        /* nobreak */
    case CSSPropertyOrphans:              // <integer> | inherit
    case CSSPropertyWidows:               // <integer> | inherit
        // ### not supported later on
        validPrimitive = (!id && validUnit(value, FInteger, false));
        break;

    case CSSPropertyLineHeight:          // normal | <number> | <length> | <percentage> | inherit
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FNumber | FLength | FPercent | FNonNeg, m_strict));
        break;
    case CSSPropertyCounterIncrement:    // [ <identifier> <integer>? ]+ | none | inherit
        if (id != CSSValueNone)
            return parseCounter(propId, 1, important);
        validPrimitive = true;
        break;
     case CSSPropertyCounterReset:        // [ <identifier> <integer>? ]+ | none | inherit
        if (id != CSSValueNone)
            return parseCounter(propId, 0, important);
        validPrimitive = true;
        break;
    case CSSPropertyFontFamily:
        // [[ <family-name> | <generic-family> ],]* [<family-name> | <generic-family>] | inherit
    {
        parsedValue = parseFontFamily();
        break;
    }

    case CSSPropertyTextDecoration:
    case CSSPropertyWebkitTextDecorationsInEffect:
        // none | [ underline || overline || line-through || blink ] | inherit
        if (id == CSSValueNone) {
            validPrimitive = true;
        } else {
            RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
            bool isValid = true;
            while (isValid && value) {
                switch (value->id) {
                case CSSValueBlink:
                    break;
                case CSSValueUnderline:
                case CSSValueOverline:
                case CSSValueLineThrough:
                    list->append(primitiveValueCache()->createIdentifierValue(value->id));
                    break;
                default:
                    isValid = false;
                }
                value = m_valueList->next();
            }
            if (list->length() && isValid) {
                parsedValue = list.release();
                m_valueList->next();
            }
        }
        break;

    case CSSPropertyZoom:          // normal | reset | document | <number> | <percentage> | inherit
        if (id == CSSValueNormal || id == CSSValueReset || id == CSSValueDocument)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FNumber | FPercent | FNonNeg, true));
        break;

    case CSSPropertyTableLayout:         // auto | fixed | inherit
        if (id == CSSValueAuto || id == CSSValueFixed)
            validPrimitive = true;
        break;

    case CSSPropertySrc:  // Only used within @font-face, so cannot use inherit | initial or be !important.  This is a list of urls or local references.
        return parseFontFaceSrc();

    case CSSPropertyUnicodeRange:
        return parseFontFaceUnicodeRange();

    /* CSS3 properties */
    case CSSPropertyWebkitAppearance:
        if ((id >= CSSValueCheckbox && id <= CSSValueTextarea) || id == CSSValueNone)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitMaskBoxImage:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            RefPtr<CSSValue> result;
            if (parseBorderImage(propId, important, result)) {
                addProperty(propId, result, important);
                return true;
            }
        }
        break;
    case CSSPropertyBorderTopRightRadius:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius: {
        if (num != 1 && num != 2)
            return false;
        validPrimitive = validUnit(value, FLength | FPercent, m_strict);
        if (!validPrimitive)
            return false;
        RefPtr<CSSPrimitiveValue> parsedValue1 = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
        RefPtr<CSSPrimitiveValue> parsedValue2;
        if (num == 2) {
            value = m_valueList->next();
            validPrimitive = validUnit(value, FLength | FPercent, m_strict);
            if (!validPrimitive)
                return false;
            parsedValue2 = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
        } else
            parsedValue2 = parsedValue1;

        RefPtr<Pair> pair = Pair::create(parsedValue1.release(), parsedValue2.release());
        RefPtr<CSSPrimitiveValue> val = primitiveValueCache()->createValue(pair.release());
        addProperty(propId, val.release(), important);
        return true;
    }
    case CSSPropertyBorderRadius:
    case CSSPropertyWebkitBorderRadius:
        return parseBorderRadius(propId, important);
    case CSSPropertyOutlineOffset:
        validPrimitive = validUnit(value, FLength | FPercent, m_strict);
        break;
    case CSSPropertyTextShadow: // CSS2 property, dropped in CSS2.1, back in CSS3, so treat as CSS3
    case CSSPropertyBoxShadow:
    case CSSPropertyWebkitBoxShadow:
        if (id == CSSValueNone)
            validPrimitive = true;
        else
            return parseShadow(propId, important);
        break;
    case CSSPropertyWebkitBoxReflect:
        if (id == CSSValueNone)
            validPrimitive = true;
        else
            return parseReflect(propId, important);
        break;
    case CSSPropertyOpacity:
        validPrimitive = validUnit(value, FNumber, m_strict);
        break;
    case CSSPropertyWebkitBoxAlign:
        if (id == CSSValueStretch || id == CSSValueStart || id == CSSValueEnd ||
            id == CSSValueCenter || id == CSSValueBaseline)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBoxDirection:
        if (id == CSSValueNormal || id == CSSValueReverse)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBoxLines:
        if (id == CSSValueSingle || id == CSSValueMultiple)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBoxOrient:
        if (id == CSSValueHorizontal || id == CSSValueVertical ||
            id == CSSValueInlineAxis || id == CSSValueBlockAxis)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBoxPack:
        if (id == CSSValueStart || id == CSSValueEnd ||
            id == CSSValueCenter || id == CSSValueJustify)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBoxFlex:
        validPrimitive = validUnit(value, FNumber, m_strict);
        break;
    case CSSPropertyWebkitBoxFlexGroup:
    case CSSPropertyWebkitBoxOrdinalGroup:
        validPrimitive = validUnit(value, FInteger | FNonNeg, true);
        break;
    case CSSPropertyBoxSizing:
        validPrimitive = id == CSSValueBorderBox || id == CSSValueContentBox;
        break;
    case CSSPropertyWebkitColorCorrection:
        validPrimitive = id == CSSValueSrgb || id == CSSValueDefault;
        break;
    case CSSPropertyWebkitMarquee: {
        const int properties[5] = { CSSPropertyWebkitMarqueeDirection, CSSPropertyWebkitMarqueeIncrement,
                                    CSSPropertyWebkitMarqueeRepetition,
                                    CSSPropertyWebkitMarqueeStyle, CSSPropertyWebkitMarqueeSpeed };
        return parseShorthand(propId, properties, 5, important);
    }
    case CSSPropertyWebkitMarqueeDirection:
        if (id == CSSValueForwards || id == CSSValueBackwards || id == CSSValueAhead ||
            id == CSSValueReverse || id == CSSValueLeft || id == CSSValueRight || id == CSSValueDown ||
            id == CSSValueUp || id == CSSValueAuto)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitMarqueeIncrement:
        if (id == CSSValueSmall || id == CSSValueLarge || id == CSSValueMedium)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength | FPercent, m_strict);
        break;
    case CSSPropertyWebkitMarqueeStyle:
        if (id == CSSValueNone || id == CSSValueSlide || id == CSSValueScroll || id == CSSValueAlternate)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitMarqueeRepetition:
        if (id == CSSValueInfinite)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FInteger | FNonNeg, m_strict);
        break;
    case CSSPropertyWebkitMarqueeSpeed:
        if (id == CSSValueNormal || id == CSSValueSlow || id == CSSValueFast)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FTime | FInteger | FNonNeg, m_strict);
        break;
#if ENABLE(WCSS)
    case CSSPropertyWapMarqueeDir:
        if (id == CSSValueLtr || id == CSSValueRtl)
            validPrimitive = true;
        break;
    case CSSPropertyWapMarqueeStyle:
        if (id == CSSValueNone || id == CSSValueSlide || id == CSSValueScroll || id == CSSValueAlternate)
            validPrimitive = true;
        break;
    case CSSPropertyWapMarqueeLoop:
        if (id == CSSValueInfinite)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FInteger | FNonNeg, m_strict);
        break;
    case CSSPropertyWapMarqueeSpeed:
        if (id == CSSValueNormal || id == CSSValueSlow || id == CSSValueFast)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FTime | FInteger | FNonNeg, m_strict);
        break;
#endif
    case CSSPropertyWebkitUserDrag: // auto | none | element
        if (id == CSSValueAuto || id == CSSValueNone || id == CSSValueElement)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitUserModify: // read-only | read-write
        if (id == CSSValueReadOnly || id == CSSValueReadWrite || id == CSSValueReadWritePlaintextOnly)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitUserSelect: // auto | none | text
        if (id == CSSValueAuto || id == CSSValueNone || id == CSSValueText)
            validPrimitive = true;
        break;
    case CSSPropertyTextOverflow: // clip | ellipsis
        if (id == CSSValueClip || id == CSSValueEllipsis)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitTransform:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            PassRefPtr<CSSValue> val = parseTransform();
            if (val) {
                addProperty(propId, val, important);
                return true;
            }
            return false;
        }
        break;
    case CSSPropertyWebkitTransformOrigin:
    case CSSPropertyWebkitTransformOriginX:
    case CSSPropertyWebkitTransformOriginY:
    case CSSPropertyWebkitTransformOriginZ: {
        RefPtr<CSSValue> val1;
        RefPtr<CSSValue> val2;
        RefPtr<CSSValue> val3;
        int propId1, propId2, propId3;
        if (parseTransformOrigin(propId, propId1, propId2, propId3, val1, val2, val3)) {
            addProperty(propId1, val1.release(), important);
            if (val2)
                addProperty(propId2, val2.release(), important);
            if (val3)
                addProperty(propId3, val3.release(), important);
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitTransformStyle:
        if (value->id == CSSValueFlat || value->id == CSSValuePreserve3d)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitBackfaceVisibility:
        if (value->id == CSSValueVisible || value->id == CSSValueHidden)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitPerspective:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            // Accepting valueless numbers is a quirk of the -webkit prefixed version of the property.
            if (validUnit(value, FNumber | FLength | FNonNeg, m_strict)) {
                RefPtr<CSSValue> val = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
                if (val) {
                    addProperty(propId, val.release(), important);
                    return true;
                }
                return false;
            }
        }
        break;
    case CSSPropertyWebkitPerspectiveOrigin:
    case CSSPropertyWebkitPerspectiveOriginX:
    case CSSPropertyWebkitPerspectiveOriginY: {
        RefPtr<CSSValue> val1;
        RefPtr<CSSValue> val2;
        int propId1, propId2;
        if (parsePerspectiveOrigin(propId, propId1, propId2, val1, val2)) {
            addProperty(propId1, val1.release(), important);
            if (val2)
                addProperty(propId2, val2.release(), important);
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitAnimationDelay:
    case CSSPropertyWebkitAnimationDirection:
    case CSSPropertyWebkitAnimationDuration:
    case CSSPropertyWebkitAnimationFillMode:
    case CSSPropertyWebkitAnimationName:
    case CSSPropertyWebkitAnimationPlayState:
    case CSSPropertyWebkitAnimationIterationCount:
    case CSSPropertyWebkitAnimationTimingFunction:
    case CSSPropertyWebkitTransitionDelay:
    case CSSPropertyWebkitTransitionDuration:
    case CSSPropertyWebkitTransitionTimingFunction:
    case CSSPropertyWebkitTransitionProperty: {
        RefPtr<CSSValue> val;
        if (parseAnimationProperty(propId, val)) {
            addProperty(propId, val.release(), important);
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitMarginCollapse: {
        const int properties[2] = { CSSPropertyWebkitMarginBeforeCollapse,
            CSSPropertyWebkitMarginAfterCollapse };
        if (num == 1) {
            ShorthandScope scope(this, CSSPropertyWebkitMarginCollapse);
            if (!parseValue(properties[0], important))
                return false;
            CSSValue* value = m_parsedProperties[m_numParsedProperties-1]->value();
            addProperty(properties[1], value, important);
            return true;
        }
        else if (num == 2) {
            ShorthandScope scope(this, CSSPropertyWebkitMarginCollapse);
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important))
                return false;
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
        if (id == CSSValueCollapse || id == CSSValueSeparate || id == CSSValueDiscard)
            validPrimitive = true;
        break;
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextUnderlineMode:
        if (id == CSSValueContinuous || id == CSSValueSkipWhiteSpace)
            validPrimitive = true;
        break;
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextUnderlineStyle:
        if (id == CSSValueNone || id == CSSValueSolid || id == CSSValueDouble ||
            id == CSSValueDashed || id == CSSValueDotDash || id == CSSValueDotDotDash ||
            id == CSSValueWave)
            validPrimitive = true;
        break;
    case CSSPropertyTextRendering: // auto | optimizeSpeed | optimizeLegibility | geometricPrecision
        if (id == CSSValueAuto || id == CSSValueOptimizespeed || id == CSSValueOptimizelegibility
            || id == CSSValueGeometricprecision)
            validPrimitive = true;
        break;
    case CSSPropertyTextLineThroughWidth:
    case CSSPropertyTextOverlineWidth:
    case CSSPropertyTextUnderlineWidth:
        if (id == CSSValueAuto || id == CSSValueNormal || id == CSSValueThin ||
            id == CSSValueMedium || id == CSSValueThick)
            validPrimitive = true;
        else
            validPrimitive = !id && validUnit(value, FNumber | FLength | FPercent, m_strict);
        break;
    case CSSPropertyResize: // none | both | horizontal | vertical | auto
        if (id == CSSValueNone || id == CSSValueBoth || id == CSSValueHorizontal || id == CSSValueVertical || id == CSSValueAuto)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitColumnCount:
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = !id && validUnit(value, FInteger | FNonNeg, false);
        break;
    case CSSPropertyWebkitColumnGap:         // normal | <length>
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength | FNonNeg, m_strict);
        break;
    case CSSPropertyWebkitColumnSpan:        // all | 1
        if (id == CSSValueAll)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FNumber | FNonNeg, m_strict) && value->fValue == 1;
        break;
    case CSSPropertyWebkitColumnWidth:         // auto | <length>
        if (id == CSSValueAuto)
            validPrimitive = true;
        else // Always parse this property in strict mode, since it would be ambiguous otherwise when used in the 'columns' shorthand property.
            validPrimitive = validUnit(value, FLength, true);
        break;
    case CSSPropertyPointerEvents:
        // none | visiblePainted | visibleFill | visibleStroke | visible |
        // painted | fill | stroke | auto | all | inherit
        if (id == CSSValueVisible || id == CSSValueNone || id == CSSValueAll || id == CSSValueAuto ||
            (id >= CSSValueVisiblepainted && id <= CSSValueStroke))
            validPrimitive = true;
        break;

    // End of CSS3 properties

    // Apple specific properties.  These will never be standardized and are purely to
    // support custom WebKit-based Apple applications.
    case CSSPropertyWebkitLineClamp:
        // When specifying number of lines, don't allow 0 as a valid value
        // When specifying either type of unit, require non-negative integers
        validPrimitive = (!id && (value->unit == CSSPrimitiveValue::CSS_PERCENTAGE || value->fValue) && validUnit(value, FInteger | FPercent | FNonNeg, false));
        break;
    case CSSPropertyWebkitTextSizeAdjust:
        if (id == CSSValueAuto || id == CSSValueNone)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitRtlOrdering:
        if (id == CSSValueLogical || id == CSSValueVisual)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitFontSizeDelta:           // <length>
        validPrimitive = validUnit(value, FLength, m_strict);
        break;

    case CSSPropertyWebkitNbspMode:     // normal | space
        if (id == CSSValueNormal || id == CSSValueSpace)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitLineBreak:   // normal | after-white-space
        if (id == CSSValueNormal || id == CSSValueAfterWhiteSpace)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitMatchNearestMailBlockquoteColor:   // normal | match
        if (id == CSSValueNormal || id == CSSValueMatch)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHighlight:
        if (id == CSSValueNone || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphens:
        if (id == CSSValueNone || id == CSSValueManual || id == CSSValueAuto)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphenateCharacter:
        if (id == CSSValueAuto || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphenateLimitBefore:
    case CSSPropertyWebkitHyphenateLimitAfter:
        if (id == CSSValueAuto || validUnit(value, FInteger | FNonNeg, true))
            validPrimitive = true;
        break;

    case CSSPropertyWebkitLocale:
        if (id == CSSValueAuto || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitBorderFit:
        if (id == CSSValueBorder || id == CSSValueLines)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitTextSecurity:
        // disc | circle | square | none | inherit
        if (id == CSSValueDisc || id == CSSValueCircle || id == CSSValueSquare|| id == CSSValueNone)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitFontSmoothing:
        if (id == CSSValueAuto || id == CSSValueNone
            || id == CSSValueAntialiased || id == CSSValueSubpixelAntialiased)
            validPrimitive = true;
        break;

#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPropertyWebkitDashboardRegion: // <dashboard-region> | <dashboard-region>
        if (value->unit == CSSParserValue::Function || id == CSSValueNone)
            return parseDashboardRegions(propId, important);
        break;
#endif
    // End Apple-specific properties

        /* shorthand properties */
    case CSSPropertyBackground: {
        // Position must come before color in this array because a plain old "0" is a legal color
        // in quirks mode but it's usually the X coordinate of a position.
        // FIXME: Add CSSPropertyBackgroundSize to the shorthand.
        const int properties[] = { CSSPropertyBackgroundImage, CSSPropertyBackgroundRepeat,
                                   CSSPropertyBackgroundAttachment, CSSPropertyBackgroundPosition, CSSPropertyBackgroundOrigin,
                                   CSSPropertyBackgroundClip, CSSPropertyBackgroundColor };
        return parseFillShorthand(propId, properties, 7, important);
    }
    case CSSPropertyWebkitMask: {
        const int properties[] = { CSSPropertyWebkitMaskImage, CSSPropertyWebkitMaskRepeat,
                                   CSSPropertyWebkitMaskAttachment, CSSPropertyWebkitMaskPosition,
                                   CSSPropertyWebkitMaskOrigin, CSSPropertyWebkitMaskClip };
        return parseFillShorthand(propId, properties, 6, important);
    }
    case CSSPropertyBorder:
        // [ 'border-width' || 'border-style' || <color> ] | inherit
    {
        const int properties[3] = { CSSPropertyBorderWidth, CSSPropertyBorderStyle,
                                    CSSPropertyBorderColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyBorderTop:
        // [ 'border-top-width' || 'border-style' || <color> ] | inherit
    {
        const int properties[3] = { CSSPropertyBorderTopWidth, CSSPropertyBorderTopStyle,
                                    CSSPropertyBorderTopColor};
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyBorderRight:
        // [ 'border-right-width' || 'border-style' || <color> ] | inherit
    {
        const int properties[3] = { CSSPropertyBorderRightWidth, CSSPropertyBorderRightStyle,
                                    CSSPropertyBorderRightColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyBorderBottom:
        // [ 'border-bottom-width' || 'border-style' || <color> ] | inherit
    {
        const int properties[3] = { CSSPropertyBorderBottomWidth, CSSPropertyBorderBottomStyle,
                                    CSSPropertyBorderBottomColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyBorderLeft:
        // [ 'border-left-width' || 'border-style' || <color> ] | inherit
    {
        const int properties[3] = { CSSPropertyBorderLeftWidth, CSSPropertyBorderLeftStyle,
                                    CSSPropertyBorderLeftColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitBorderStart:
    {
        const int properties[3] = { CSSPropertyWebkitBorderStartWidth, CSSPropertyWebkitBorderStartStyle,
            CSSPropertyWebkitBorderStartColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitBorderEnd:
    {
        const int properties[3] = { CSSPropertyWebkitBorderEndWidth, CSSPropertyWebkitBorderEndStyle,
            CSSPropertyWebkitBorderEndColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitBorderBefore:
    {
        const int properties[3] = { CSSPropertyWebkitBorderBeforeWidth, CSSPropertyWebkitBorderBeforeStyle,
            CSSPropertyWebkitBorderBeforeColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitBorderAfter:
    {
        const int properties[3] = { CSSPropertyWebkitBorderAfterWidth, CSSPropertyWebkitBorderAfterStyle,
            CSSPropertyWebkitBorderAfterColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyOutline:
        // [ 'outline-color' || 'outline-style' || 'outline-width' ] | inherit
    {
        const int properties[3] = { CSSPropertyOutlineWidth, CSSPropertyOutlineStyle,
                                    CSSPropertyOutlineColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyBorderColor:
        // <color>{1,4} | inherit
    {
        const int properties[4] = { CSSPropertyBorderTopColor, CSSPropertyBorderRightColor,
                                    CSSPropertyBorderBottomColor, CSSPropertyBorderLeftColor };
        return parse4Values(propId, properties, important);
    }
    case CSSPropertyBorderWidth:
        // <border-width>{1,4} | inherit
    {
        const int properties[4] = { CSSPropertyBorderTopWidth, CSSPropertyBorderRightWidth,
                                    CSSPropertyBorderBottomWidth, CSSPropertyBorderLeftWidth };
        return parse4Values(propId, properties, important);
    }
    case CSSPropertyBorderStyle:
        // <border-style>{1,4} | inherit
    {
        const int properties[4] = { CSSPropertyBorderTopStyle, CSSPropertyBorderRightStyle,
                                    CSSPropertyBorderBottomStyle, CSSPropertyBorderLeftStyle };
        return parse4Values(propId, properties, important);
    }
    case CSSPropertyMargin:
        // <margin-width>{1,4} | inherit
    {
        const int properties[4] = { CSSPropertyMarginTop, CSSPropertyMarginRight,
                                    CSSPropertyMarginBottom, CSSPropertyMarginLeft };
        return parse4Values(propId, properties, important);
    }
    case CSSPropertyPadding:
        // <padding-width>{1,4} | inherit
    {
        const int properties[4] = { CSSPropertyPaddingTop, CSSPropertyPaddingRight,
                                    CSSPropertyPaddingBottom, CSSPropertyPaddingLeft };
        return parse4Values(propId, properties, important);
    }
    case CSSPropertyFont:
        // [ [ 'font-style' || 'font-variant' || 'font-weight' ]? 'font-size' [ / 'line-height' ]?
        // 'font-family' ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
        if (id >= CSSValueCaption && id <= CSSValueStatusBar)
            validPrimitive = true;
        else
            return parseFont(important);
        break;
    case CSSPropertyListStyle:
    {
        const int properties[3] = { CSSPropertyListStyleType, CSSPropertyListStylePosition,
                                    CSSPropertyListStyleImage };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitColumns: {
        const int properties[2] = { CSSPropertyWebkitColumnWidth, CSSPropertyWebkitColumnCount };
        return parseShorthand(propId, properties, 2, important);
    }
    case CSSPropertyWebkitColumnRule: {
        const int properties[3] = { CSSPropertyWebkitColumnRuleWidth, CSSPropertyWebkitColumnRuleStyle,
                                    CSSPropertyWebkitColumnRuleColor };
        return parseShorthand(propId, properties, 3, important);
    }
    case CSSPropertyWebkitTextStroke: {
        const int properties[2] = { CSSPropertyWebkitTextStrokeWidth, CSSPropertyWebkitTextStrokeColor };
        return parseShorthand(propId, properties, 2, important);
    }
    case CSSPropertyWebkitAnimation:
        return parseAnimationShorthand(important);
    case CSSPropertyWebkitTransition:
        return parseTransitionShorthand(important);
    case CSSPropertyInvalid:
        return false;
    case CSSPropertyPage:
        return parsePage(propId, important);
    case CSSPropertyFontStretch:
    case CSSPropertyTextLineThrough:
    case CSSPropertyTextOverline:
    case CSSPropertyTextUnderline:
        return false;
#if ENABLE(WCSS)
    case CSSPropertyWapInputFormat:
        validPrimitive = true;
        break;
    case CSSPropertyWapInputRequired:
        parsedValue = parseWCSSInputProperty();
        break;
#endif

    // CSS Text Layout Module Level 3: Vertical writing support
    case CSSPropertyWebkitWritingMode:
        if (id >= CSSValueHorizontalTb && id <= CSSValueHorizontalBt)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitTextCombine:
        if (id == CSSValueNone || id == CSSValueHorizontal)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitTextEmphasis: {
        const int properties[] = { CSSPropertyWebkitTextEmphasisStyle, CSSPropertyWebkitTextEmphasisColor };
        return parseShorthand(propId, properties, WTF_ARRAY_LENGTH(properties), important);
    }

    case CSSPropertyWebkitTextEmphasisPosition:
        if (id == CSSValueOver || id == CSSValueUnder)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitTextEmphasisStyle:
        return parseTextEmphasisStyle(important);

    case CSSPropertyWebkitTextOrientation:
        // FIXME: For now just support upright and vertical-right.
        if (id == CSSValueVerticalRight || id == CSSValueUpright)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitLineBoxContain:
        if (id == CSSValueNone)
            validPrimitive = true;
        else
            return parseLineBoxContain(important);
        break;

#if ENABLE(SVG)
    default:
        return parseSVGValue(propId, important);
#endif
    }

    if (validPrimitive) {
        if (id != 0)
            parsedValue = primitiveValueCache()->createIdentifierValue(id);
        else if (value->unit == CSSPrimitiveValue::CSS_STRING)
            parsedValue = primitiveValueCache()->createValue(value->string, (CSSPrimitiveValue::UnitTypes) value->unit);
        else if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
            parsedValue = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
        else if (value->unit >= CSSPrimitiveValue::CSS_TURN && value->unit <= CSSPrimitiveValue::CSS_REMS)
            parsedValue = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
        else if (value->unit >= CSSParserValue::Q_EMS)
            parsedValue = CSSQuirkPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_EMS);
        m_valueList->next();
    }
    if (parsedValue) {
        if (!m_valueList->current() || inShorthand()) {
            addProperty(propId, parsedValue.release(), important);
            return true;
        }
    }
    return false;
}

#if ENABLE(WCSS)
PassRefPtr<CSSValue> CSSParser::parseWCSSInputProperty()
{
    RefPtr<CSSValue> parsedValue = 0;
    CSSParserValue* value = m_valueList->current();
    String inputProperty;
    if (value->unit == CSSPrimitiveValue::CSS_STRING || value->unit == CSSPrimitiveValue::CSS_IDENT)
        inputProperty = String(value->string);

    if (!inputProperty.isEmpty())
       parsedValue = primitiveValueCache()->createValue(inputProperty, CSSPrimitiveValue::CSS_STRING);

    while (m_valueList->next()) {
    // pass all other values, if any. If we don't do this,
    // the parser will think that it's not done and won't process this property
    }

    return parsedValue;
}
#endif

void CSSParser::addFillValue(RefPtr<CSSValue>& lval, PassRefPtr<CSSValue> rval)
{
    if (lval) {
        if (lval->isValueList())
            static_cast<CSSValueList*>(lval.get())->append(rval);
        else {
            PassRefPtr<CSSValue> oldlVal(lval.release());
            PassRefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            list->append(oldlVal);
            list->append(rval);
            lval = list;
        }
    }
    else
        lval = rval;
}

static bool parseBackgroundClip(CSSParserValue* parserValue, RefPtr<CSSValue>& cssValue, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (parserValue->id == CSSValueBorderBox || parserValue->id == CSSValuePaddingBox
        || parserValue->id == CSSValueContentBox || parserValue->id == CSSValueWebkitText) {
        cssValue = primitiveValueCache->createIdentifierValue(parserValue->id);
        return true;
    }
    return false;
}

const int cMaxFillProperties = 9;

bool CSSParser::parseFillShorthand(int propId, const int* properties, int numProperties, bool important)
{
    ASSERT(numProperties <= cMaxFillProperties);
    if (numProperties > cMaxFillProperties)
        return false;

    ShorthandScope scope(this, propId);

    bool parsedProperty[cMaxFillProperties] = { false };
    RefPtr<CSSValue> values[cMaxFillProperties];
    RefPtr<CSSValue> clipValue;
    RefPtr<CSSValue> positionYValue;
    RefPtr<CSSValue> repeatYValue;
    bool foundClip = false;
    int i;

    while (m_valueList->current()) {
        CSSParserValue* val = m_valueList->current();
        if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            // We hit the end.  Fill in all remaining values with the initial value.
            m_valueList->next();
            for (i = 0; i < numProperties; ++i) {
                if (properties[i] == CSSPropertyBackgroundColor && parsedProperty[i])
                    // Color is not allowed except as the last item in a list for backgrounds.
                    // Reject the entire property.
                    return false;

                if (!parsedProperty[i] && properties[i] != CSSPropertyBackgroundColor) {
                    addFillValue(values[i], CSSInitialValue::createImplicit());
                    if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                        addFillValue(positionYValue, CSSInitialValue::createImplicit());
                    if (properties[i] == CSSPropertyBackgroundRepeat || properties[i] == CSSPropertyWebkitMaskRepeat)
                        addFillValue(repeatYValue, CSSInitialValue::createImplicit());
                    if ((properties[i] == CSSPropertyBackgroundOrigin || properties[i] == CSSPropertyWebkitMaskOrigin) && !parsedProperty[i]) {
                        // If background-origin wasn't present, then reset background-clip also.
                        addFillValue(clipValue, CSSInitialValue::createImplicit());
                    }                    
                }
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
        }

        bool found = false;
        for (i = 0; !found && i < numProperties; ++i) {
            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val1;
                RefPtr<CSSValue> val2;
                int propId1, propId2;
                CSSParserValue* parserValue = m_valueList->current();
                if (parseFillProperty(properties[i], propId1, propId2, val1, val2)) {
                    parsedProperty[i] = found = true;
                    addFillValue(values[i], val1.release());
                    if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                        addFillValue(positionYValue, val2.release());
                    if (properties[i] == CSSPropertyBackgroundRepeat || properties[i] == CSSPropertyWebkitMaskRepeat)
                        addFillValue(repeatYValue, val2.release());
                    if (properties[i] == CSSPropertyBackgroundOrigin || properties[i] == CSSPropertyWebkitMaskOrigin) {
                        // Reparse the value as a clip, and see if we succeed.
                        if (parseBackgroundClip(parserValue, val1, primitiveValueCache()))
                            addFillValue(clipValue, val1.release()); // The property parsed successfully.
                        else
                            addFillValue(clipValue, CSSInitialValue::createImplicit()); // Some value was used for origin that is not supported by clip. Just reset clip instead.
                    }
                    if (properties[i] == CSSPropertyBackgroundClip || properties[i] == CSSPropertyWebkitMaskClip) {
                        // Update clipValue
                        addFillValue(clipValue, val1.release());
                        foundClip = true;
                    }
                }
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    // Fill in any remaining properties with the initial value.
    for (i = 0; i < numProperties; ++i) {
        if (!parsedProperty[i]) {
            addFillValue(values[i], CSSInitialValue::createImplicit());
            if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                addFillValue(positionYValue, CSSInitialValue::createImplicit());
            if (properties[i] == CSSPropertyBackgroundRepeat || properties[i] == CSSPropertyWebkitMaskRepeat)
                addFillValue(repeatYValue, CSSInitialValue::createImplicit());
            if ((properties[i] == CSSPropertyBackgroundOrigin || properties[i] == CSSPropertyWebkitMaskOrigin) && !parsedProperty[i]) {
                // If background-origin wasn't present, then reset background-clip also.
                addFillValue(clipValue, CSSInitialValue::createImplicit());
            }
        }
    }

    // Now add all of the properties we found.
    for (i = 0; i < numProperties; i++) {
        if (properties[i] == CSSPropertyBackgroundPosition) {
            addProperty(CSSPropertyBackgroundPositionX, values[i].release(), important);
            // it's OK to call positionYValue.release() since we only see CSSPropertyBackgroundPosition once
            addProperty(CSSPropertyBackgroundPositionY, positionYValue.release(), important);
        } else if (properties[i] == CSSPropertyWebkitMaskPosition) {
            addProperty(CSSPropertyWebkitMaskPositionX, values[i].release(), important);
            // it's OK to call positionYValue.release() since we only see CSSPropertyWebkitMaskPosition once
            addProperty(CSSPropertyWebkitMaskPositionY, positionYValue.release(), important);
        } else if (properties[i] == CSSPropertyBackgroundRepeat) {
            addProperty(CSSPropertyBackgroundRepeatX, values[i].release(), important);
            // it's OK to call repeatYValue.release() since we only see CSSPropertyBackgroundPosition once
            addProperty(CSSPropertyBackgroundRepeatY, repeatYValue.release(), important);
        } else if (properties[i] == CSSPropertyWebkitMaskRepeat) {
            addProperty(CSSPropertyWebkitMaskRepeatX, values[i].release(), important);
            // it's OK to call repeatYValue.release() since we only see CSSPropertyBackgroundPosition once
            addProperty(CSSPropertyWebkitMaskRepeatY, repeatYValue.release(), important);
        } else if ((properties[i] == CSSPropertyBackgroundClip || properties[i] == CSSPropertyWebkitMaskClip) && !foundClip)
            // Value is already set while updating origin
            continue;
        else
            addProperty(properties[i], values[i].release(), important);

        // Add in clip values when we hit the corresponding origin property.
        if (properties[i] == CSSPropertyBackgroundOrigin && !foundClip)
            addProperty(CSSPropertyBackgroundClip, clipValue.release(), important);
        else if (properties[i] == CSSPropertyWebkitMaskOrigin && !foundClip)
            addProperty(CSSPropertyWebkitMaskClip, clipValue.release(), important);
    }

    return true;
}

void CSSParser::addAnimationValue(RefPtr<CSSValue>& lval, PassRefPtr<CSSValue> rval)
{
    if (lval) {
        if (lval->isValueList())
            static_cast<CSSValueList*>(lval.get())->append(rval);
        else {
            PassRefPtr<CSSValue> oldVal(lval.release());
            PassRefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            list->append(oldVal);
            list->append(rval);
            lval = list;
        }
    }
    else
        lval = rval;
}

bool CSSParser::parseAnimationShorthand(bool important)
{
    const int properties[] = {  CSSPropertyWebkitAnimationName,
                                CSSPropertyWebkitAnimationDuration,
                                CSSPropertyWebkitAnimationTimingFunction,
                                CSSPropertyWebkitAnimationDelay,
                                CSSPropertyWebkitAnimationIterationCount,
                                CSSPropertyWebkitAnimationDirection,
                                CSSPropertyWebkitAnimationFillMode };
    const int numProperties = WTF_ARRAY_LENGTH(properties);

    ShorthandScope scope(this, CSSPropertyWebkitAnimation);

    bool parsedProperty[numProperties] = { false }; // compiler will repeat false as necessary
    RefPtr<CSSValue> values[numProperties];

    int i;
    while (m_valueList->current()) {
        CSSParserValue* val = m_valueList->current();
        if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            // We hit the end.  Fill in all remaining values with the initial value.
            m_valueList->next();
            for (i = 0; i < numProperties; ++i) {
                if (!parsedProperty[i])
                    addAnimationValue(values[i], CSSInitialValue::createImplicit());
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
        }

        bool found = false;
        for (i = 0; !found && i < numProperties; ++i) {
            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val;
                if (parseAnimationProperty(properties[i], val)) {
                    parsedProperty[i] = found = true;
                    addAnimationValue(values[i], val.release());
                }
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    // Fill in any remaining properties with the initial value.
    for (i = 0; i < numProperties; ++i) {
        if (!parsedProperty[i])
            addAnimationValue(values[i], CSSInitialValue::createImplicit());
    }

    // Now add all of the properties we found.
    for (i = 0; i < numProperties; i++)
        addProperty(properties[i], values[i].release(), important);

    return true;
}

bool CSSParser::parseTransitionShorthand(bool important)
{
    const int properties[] = { CSSPropertyWebkitTransitionProperty,
                               CSSPropertyWebkitTransitionDuration,
                               CSSPropertyWebkitTransitionTimingFunction,
                               CSSPropertyWebkitTransitionDelay };
    const int numProperties = WTF_ARRAY_LENGTH(properties);

    ShorthandScope scope(this, CSSPropertyWebkitTransition);

    bool parsedProperty[numProperties] = { false }; // compiler will repeat false as necessary
    RefPtr<CSSValue> values[numProperties];

    int i;
    while (m_valueList->current()) {
        CSSParserValue* val = m_valueList->current();
        if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            // We hit the end.  Fill in all remaining values with the initial value.
            m_valueList->next();
            for (i = 0; i < numProperties; ++i) {
                if (!parsedProperty[i])
                    addAnimationValue(values[i], CSSInitialValue::createImplicit());
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
        }

        bool found = false;
        for (i = 0; !found && i < numProperties; ++i) {
            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val;
                if (parseAnimationProperty(properties[i], val)) {
                    parsedProperty[i] = found = true;
                    addAnimationValue(values[i], val.release());
                }
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    // Fill in any remaining properties with the initial value.
    for (i = 0; i < numProperties; ++i) {
        if (!parsedProperty[i])
            addAnimationValue(values[i], CSSInitialValue::createImplicit());
    }

    // Now add all of the properties we found.
    for (i = 0; i < numProperties; i++)
        addProperty(properties[i], values[i].release(), important);

    return true;
}

bool CSSParser::parseShorthand(int propId, const int *properties, int numProperties, bool important)
{
    // We try to match as many properties as possible
    // We set up an array of booleans to mark which property has been found,
    // and we try to search for properties until it makes no longer any sense.
    ShorthandScope scope(this, propId);

    bool found = false;
    bool fnd[6]; // Trust me ;)
    for (int i = 0; i < numProperties; i++)
        fnd[i] = false;

    while (m_valueList->current()) {
        found = false;
        for (int propIndex = 0; !found && propIndex < numProperties; ++propIndex) {
            if (!fnd[propIndex]) {
                if (parseValue(properties[propIndex], important))
                    fnd[propIndex] = found = true;
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    // Fill in any remaining properties with the initial value.
    m_implicitShorthand = true;
    for (int i = 0; i < numProperties; ++i) {
        if (!fnd[i])
            addProperty(properties[i], CSSInitialValue::createImplicit(), important);
    }
    m_implicitShorthand = false;

    return true;
}

bool CSSParser::parse4Values(int propId, const int *properties,  bool important)
{
    /* From the CSS 2 specs, 8.3
     * If there is only one value, it applies to all sides. If there are two values, the top and
     * bottom margins are set to the first value and the right and left margins are set to the second.
     * If there are three values, the top is set to the first value, the left and right are set to the
     * second, and the bottom is set to the third. If there are four values, they apply to the top,
     * right, bottom, and left, respectively.
     */

    int num = inShorthand() ? 1 : m_valueList->size();

    ShorthandScope scope(this, propId);

    // the order is top, right, bottom, left
    switch (num) {
        case 1: {
            if (!parseValue(properties[0], important))
                return false;
            CSSValue *value = m_parsedProperties[m_numParsedProperties-1]->value();
            m_implicitShorthand = true;
            addProperty(properties[1], value, important);
            addProperty(properties[2], value, important);
            addProperty(properties[3], value, important);
            m_implicitShorthand = false;
            break;
        }
        case 2: {
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important))
                return false;
            CSSValue *value = m_parsedProperties[m_numParsedProperties-2]->value();
            m_implicitShorthand = true;
            addProperty(properties[2], value, important);
            value = m_parsedProperties[m_numParsedProperties-2]->value();
            addProperty(properties[3], value, important);
            m_implicitShorthand = false;
            break;
        }
        case 3: {
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important) || !parseValue(properties[2], important))
                return false;
            CSSValue *value = m_parsedProperties[m_numParsedProperties-2]->value();
            m_implicitShorthand = true;
            addProperty(properties[3], value, important);
            m_implicitShorthand = false;
            break;
        }
        case 4: {
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important) ||
                !parseValue(properties[2], important) || !parseValue(properties[3], important))
                return false;
            break;
        }
        default: {
            return false;
        }
    }

    return true;
}

// auto | <identifier>
bool CSSParser::parsePage(int propId, bool important)
{
    ASSERT(propId == CSSPropertyPage);

    if (m_valueList->size() != 1)
        return false;

    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    if (value->id == CSSValueAuto) {
        addProperty(propId, primitiveValueCache()->createIdentifierValue(value->id), important);
        return true;
    } else if (value->id == 0 && value->unit == CSSPrimitiveValue::CSS_IDENT) {
        addProperty(propId, primitiveValueCache()->createValue(value->string, CSSPrimitiveValue::CSS_STRING), important);
        return true;
    }
    return false;
}

// <length>{1,2} | auto | [ <page-size> || [ portrait | landscape] ]
bool CSSParser::parseSize(int propId, bool important)
{
    ASSERT(propId == CSSPropertySize);

    if (m_valueList->size() > 2)
        return false;

    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    RefPtr<CSSValueList> parsedValues = CSSValueList::createSpaceSeparated();

    // First parameter.
    SizeParameterType paramType = parseSizeParameter(parsedValues.get(), value, None);
    if (paramType == None)
        return false;

    // Second parameter, if any.
    value = m_valueList->next();
    if (value) {
        paramType = parseSizeParameter(parsedValues.get(), value, paramType);
        if (paramType == None)
            return false;
    }

    addProperty(propId, parsedValues.release(), important);
    return true;
}

CSSParser::SizeParameterType CSSParser::parseSizeParameter(CSSValueList* parsedValues, CSSParserValue* value, SizeParameterType prevParamType)
{
    switch (value->id) {
    case CSSValueAuto:
        if (prevParamType == None) {
            parsedValues->append(primitiveValueCache()->createIdentifierValue(value->id));
            return Auto;
        }
        return None;
    case CSSValueLandscape:
    case CSSValuePortrait:
        if (prevParamType == None || prevParamType == PageSize) {
            parsedValues->append(primitiveValueCache()->createIdentifierValue(value->id));
            return Orientation;
        }
        return None;
    case CSSValueA3:
    case CSSValueA4:
    case CSSValueA5:
    case CSSValueB4:
    case CSSValueB5:
    case CSSValueLedger:
    case CSSValueLegal:
    case CSSValueLetter:
        if (prevParamType == None || prevParamType == Orientation) {
            // Normalize to Page Size then Orientation order by prepending.
            // This is not specified by the CSS3 Paged Media specification, but for simpler processing later (CSSStyleSelector::applyPageSizeProperty).
            parsedValues->prepend(primitiveValueCache()->createIdentifierValue(value->id));
            return PageSize;
        }
        return None;
    case 0:
        if (validUnit(value, FLength | FNonNeg, m_strict) && (prevParamType == None || prevParamType == Length)) {
            parsedValues->append(primitiveValueCache()->createValue(value->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(value->unit)));
            return Length;
        }
        return None;
    default:
        return None;
    }
}

// [ <string> <string> ]+ | inherit | none
// inherit and none are handled in parseValue.
bool CSSParser::parseQuotes(int propId, bool important)
{
    RefPtr<CSSValueList> values = CSSValueList::createCommaSeparated();
    while (CSSParserValue* val = m_valueList->current()) {
        RefPtr<CSSValue> parsedValue;
        if (val->unit == CSSPrimitiveValue::CSS_STRING)
            parsedValue = CSSPrimitiveValue::create(val->string, CSSPrimitiveValue::CSS_STRING);
        else
            break;
        values->append(parsedValue.release());
        m_valueList->next();
    }
    if (values->length()) {
        addProperty(propId, values.release(), important);
        m_valueList->next();
        return true;
    }
    return false;
}

// [ <string> | <uri> | <counter> | attr(X) | open-quote | close-quote | no-open-quote | no-close-quote ]+ | inherit
// in CSS 2.1 this got somewhat reduced:
// [ <string> | attr(X) | open-quote | close-quote | no-open-quote | no-close-quote ]+ | inherit
bool CSSParser::parseContent(int propId, bool important)
{
    RefPtr<CSSValueList> values = CSSValueList::createCommaSeparated();

    while (CSSParserValue* val = m_valueList->current()) {
        RefPtr<CSSValue> parsedValue;
        if (val->unit == CSSPrimitiveValue::CSS_URI && m_styleSheet) {
            // url
            // FIXME: The completeURL call should be done when using the CSSImageValue,
            // not when creating it.
            parsedValue = CSSImageValue::create(m_styleSheet->completeURL(val->string));
        } else if (val->unit == CSSParserValue::Function) {
            // attr(X) | counter(X [,Y]) | counters(X, Y, [,Z]) | -webkit-gradient(...)
            CSSParserValueList* args = val->function->args.get();
            if (!args)
                return false;
            if (equalIgnoringCase(val->function->name, "attr(")) {
                parsedValue = parseAttr(args);
                if (!parsedValue)
                    return false;
            } else if (equalIgnoringCase(val->function->name, "counter(")) {
                parsedValue = parseCounterContent(args, false);
                if (!parsedValue)
                    return false;
            } else if (equalIgnoringCase(val->function->name, "counters(")) {
                parsedValue = parseCounterContent(args, true);
                if (!parsedValue)
                    return false;
            } else if (isGeneratedImageValue(val)) {
                if (!parseGeneratedImage(parsedValue))
                    return false;
            } else
                return false;
        } else if (val->unit == CSSPrimitiveValue::CSS_IDENT) {
            // open-quote
            // close-quote
            // no-open-quote
            // no-close-quote
            // inherit
            // FIXME: These are not yet implemented (http://bugs.webkit.org/show_bug.cgi?id=6503).
            // none
            // normal
            switch (val->id) {
            case CSSValueOpenQuote:
            case CSSValueCloseQuote:
            case CSSValueNoOpenQuote:
            case CSSValueNoCloseQuote:
            case CSSValueNone:
            case CSSValueNormal:
                parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
            }
        } else if (val->unit == CSSPrimitiveValue::CSS_STRING) {
            parsedValue = primitiveValueCache()->createValue(val->string, CSSPrimitiveValue::CSS_STRING);
        }
        if (!parsedValue)
            break;
        values->append(parsedValue.release());
        m_valueList->next();
    }

    if (values->length()) {
        addProperty(propId, values.release(), important);
        m_valueList->next();
        return true;
    }

    return false;
}

PassRefPtr<CSSValue> CSSParser::parseAttr(CSSParserValueList* args)
{
    if (args->size() != 1)
        return 0;

    CSSParserValue* a = args->current();

    if (a->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;

    String attrName = a->string;
    // CSS allows identifiers with "-" at the start, like "-webkit-mask-image".
    // But HTML attribute names can't have those characters, and we should not
    // even parse them inside attr().
    if (attrName[0] == '-')
        return 0;

    if (document() && document()->isHTMLDocument())
        attrName = attrName.lower();

    return primitiveValueCache()->createValue(attrName, CSSPrimitiveValue::CSS_ATTR);
}

PassRefPtr<CSSValue> CSSParser::parseBackgroundColor()
{
    int id = m_valueList->current()->id;
    if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu || id == CSSValueCurrentcolor ||
        (id >= CSSValueGrey && id < CSSValueWebkitText && !m_strict))
       return primitiveValueCache()->createIdentifierValue(id);
    return parseColor();
}

bool CSSParser::parseFillImage(RefPtr<CSSValue>& value)
{
    if (m_valueList->current()->id == CSSValueNone) {
        value = CSSImageValue::create();
        return true;
    }
    if (m_valueList->current()->unit == CSSPrimitiveValue::CSS_URI) {
        // FIXME: The completeURL call should be done when using the CSSImageValue,
        // not when creating it.
        if (m_styleSheet)
            value = CSSImageValue::create(m_styleSheet->completeURL(m_valueList->current()->string));
        return true;
    }

    if (isGeneratedImageValue(m_valueList->current()))
        return parseGeneratedImage(value);

    return false;
}

PassRefPtr<CSSValue> CSSParser::parseFillPositionX(CSSParserValueList* valueList)
{
    int id = valueList->current()->id;
    if (id == CSSValueLeft || id == CSSValueRight || id == CSSValueCenter) {
        int percent = 0;
        if (id == CSSValueRight)
            percent = 100;
        else if (id == CSSValueCenter)
            percent = 50;
        return primitiveValueCache()->createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength, m_strict))
        return primitiveValueCache()->createValue(valueList->current()->fValue,
                                                  (CSSPrimitiveValue::UnitTypes)valueList->current()->unit);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseFillPositionY(CSSParserValueList* valueList)
{
    int id = valueList->current()->id;
    if (id == CSSValueTop || id == CSSValueBottom || id == CSSValueCenter) {
        int percent = 0;
        if (id == CSSValueBottom)
            percent = 100;
        else if (id == CSSValueCenter)
            percent = 50;
        return primitiveValueCache()->createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength, m_strict))
        return primitiveValueCache()->createValue(valueList->current()->fValue,
                                                  (CSSPrimitiveValue::UnitTypes)valueList->current()->unit);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseFillPositionComponent(CSSParserValueList* valueList, unsigned& cumulativeFlags, FillPositionFlag& individualFlag)
{
    int id = valueList->current()->id;
    if (id == CSSValueLeft || id == CSSValueTop || id == CSSValueRight || id == CSSValueBottom || id == CSSValueCenter) {
        int percent = 0;
        if (id == CSSValueLeft || id == CSSValueRight) {
            if (cumulativeFlags & XFillPosition)
                return 0;
            cumulativeFlags |= XFillPosition;
            individualFlag = XFillPosition;
            if (id == CSSValueRight)
                percent = 100;
        }
        else if (id == CSSValueTop || id == CSSValueBottom) {
            if (cumulativeFlags & YFillPosition)
                return 0;
            cumulativeFlags |= YFillPosition;
            individualFlag = YFillPosition;
            if (id == CSSValueBottom)
                percent = 100;
        } else if (id == CSSValueCenter) {
            // Center is ambiguous, so we're not sure which position we've found yet, an x or a y.
            percent = 50;
            cumulativeFlags |= AmbiguousFillPosition;
            individualFlag = AmbiguousFillPosition;
        }
        return primitiveValueCache()->createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength, m_strict)) {
        if (!cumulativeFlags) {
            cumulativeFlags |= XFillPosition;
            individualFlag = XFillPosition;
        } else if (cumulativeFlags & (XFillPosition | AmbiguousFillPosition)) {
            cumulativeFlags |= YFillPosition;
            individualFlag = YFillPosition;
        } else
            return 0;
        return primitiveValueCache()->createValue(valueList->current()->fValue,
                                                  (CSSPrimitiveValue::UnitTypes)valueList->current()->unit);
    }
    return 0;
}

void CSSParser::parseFillPosition(CSSParserValueList* valueList, RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2)
{
    CSSParserValue* value = valueList->current();

    // Parse the first value.  We're just making sure that it is one of the valid keywords or a percentage/length.
    unsigned cumulativeFlags = 0;
    FillPositionFlag value1Flag = InvalidFillPosition;
    FillPositionFlag value2Flag = InvalidFillPosition;
    value1 = parseFillPositionComponent(valueList, cumulativeFlags, value1Flag);
    if (!value1)
        return;

    // It only takes one value for background-position to be correctly parsed if it was specified in a shorthand (since we
    // can assume that any other values belong to the rest of the shorthand).  If we're not parsing a shorthand, though, the
    // value was explicitly specified for our property.
    value = valueList->next();

    // First check for the comma.  If so, we are finished parsing this value or value pair.
    if (value && value->unit == CSSParserValue::Operator && value->iValue == ',')
        value = 0;

    if (value) {
        value2 = parseFillPositionComponent(valueList, cumulativeFlags, value2Flag);
        if (value2)
            valueList->next();
        else {
            if (!inShorthand()) {
                value1.clear();
                return;
            }
        }
    }

    if (!value2)
        // Only one value was specified.  If that value was not a keyword, then it sets the x position, and the y position
        // is simply 50%.  This is our default.
        // For keywords, the keyword was either an x-keyword (left/right), a y-keyword (top/bottom), or an ambiguous keyword (center).
        // For left/right/center, the default of 50% in the y is still correct.
        value2 = primitiveValueCache()->createValue(50, CSSPrimitiveValue::CSS_PERCENTAGE);

    if (value1Flag == YFillPosition || value2Flag == XFillPosition)
        value1.swap(value2);
}

void CSSParser::parseFillRepeat(RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2)
{
    CSSParserValue* value = m_valueList->current();

    int id = m_valueList->current()->id;
    if (id == CSSValueRepeatX) {
        m_implicitShorthand = true;
        value1 = primitiveValueCache()->createIdentifierValue(CSSValueRepeat);
        value2 = primitiveValueCache()->createIdentifierValue(CSSValueNoRepeat);
        m_valueList->next();
        return;
    }
    if (id == CSSValueRepeatY) {
        m_implicitShorthand = true;
        value1 = primitiveValueCache()->createIdentifierValue(CSSValueNoRepeat);
        value2 = primitiveValueCache()->createIdentifierValue(CSSValueRepeat);
        m_valueList->next();
        return;
    }
    if (id == CSSValueRepeat || id == CSSValueNoRepeat || id == CSSValueRound || id == CSSValueSpace)
        value1 = primitiveValueCache()->createIdentifierValue(id);
    else {
        value1 = 0;
        return;
    }

    value = m_valueList->next();

    // First check for the comma.  If so, we are finished parsing this value or value pair.
    if (value && value->unit == CSSParserValue::Operator && value->iValue == ',')
        value = 0;

    if (value)
        id = m_valueList->current()->id;

    if (value && (id == CSSValueRepeat || id == CSSValueNoRepeat || id == CSSValueRound || id == CSSValueSpace)) {
        value2 = primitiveValueCache()->createIdentifierValue(id);
        m_valueList->next();
    } else {
        // If only one value was specified, value2 is the same as value1.
        m_implicitShorthand = true;
        value2 = primitiveValueCache()->createIdentifierValue(static_cast<CSSPrimitiveValue*>(value1.get())->getIdent());
    }
}

PassRefPtr<CSSValue> CSSParser::parseFillSize(int propId, bool& allowComma)
{
    allowComma = true;
    CSSParserValue* value = m_valueList->current();

    if (value->id == CSSValueContain || value->id == CSSValueCover)
        return primitiveValueCache()->createIdentifierValue(value->id);

    RefPtr<CSSPrimitiveValue> parsedValue1;

    if (value->id == CSSValueAuto)
        parsedValue1 = primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_UNKNOWN);
    else {
        if (!validUnit(value, FLength | FPercent, m_strict))
            return 0;
        parsedValue1 = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
    }

    CSSPropertyID property = static_cast<CSSPropertyID>(propId);
    RefPtr<CSSPrimitiveValue> parsedValue2;
    if ((value = m_valueList->next())) {
        if (value->id == CSSValueAuto)
            parsedValue2 = primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_UNKNOWN);
        else if (value->unit == CSSParserValue::Operator && value->iValue == ',')
            allowComma = false;
        else {
            if (!validUnit(value, FLength | FPercent, m_strict))
                return 0;
            parsedValue2 = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
        }
    }
    if (!parsedValue2) {
        if (property == CSSPropertyWebkitBackgroundSize || property == CSSPropertyWebkitMaskSize)
            parsedValue2 = parsedValue1;
        else
            parsedValue2 = primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_UNKNOWN);
    }

    return primitiveValueCache()->createValue(Pair::create(parsedValue1.release(), parsedValue2.release()));
}

bool CSSParser::parseFillProperty(int propId, int& propId1, int& propId2,
                                  RefPtr<CSSValue>& retValue1, RefPtr<CSSValue>& retValue2)
{
    RefPtr<CSSValueList> values;
    RefPtr<CSSValueList> values2;
    CSSParserValue* val;
    RefPtr<CSSValue> value;
    RefPtr<CSSValue> value2;

    bool allowComma = false;

    retValue1 = retValue2 = 0;
    propId1 = propId;
    propId2 = propId;
    if (propId == CSSPropertyBackgroundPosition) {
        propId1 = CSSPropertyBackgroundPositionX;
        propId2 = CSSPropertyBackgroundPositionY;
    } else if (propId == CSSPropertyWebkitMaskPosition) {
        propId1 = CSSPropertyWebkitMaskPositionX;
        propId2 = CSSPropertyWebkitMaskPositionY;
    } else if (propId == CSSPropertyBackgroundRepeat) {
        propId1 = CSSPropertyBackgroundRepeatX;
        propId2 = CSSPropertyBackgroundRepeatY;
    } else if (propId == CSSPropertyWebkitMaskRepeat) {
        propId1 = CSSPropertyWebkitMaskRepeatX;
        propId2 = CSSPropertyWebkitMaskRepeatY;
    }

    while ((val = m_valueList->current())) {
        RefPtr<CSSValue> currValue;
        RefPtr<CSSValue> currValue2;

        if (allowComma) {
            if (val->unit != CSSParserValue::Operator || val->iValue != ',')
                return false;
            m_valueList->next();
            allowComma = false;
        } else {
            allowComma = true;
            switch (propId) {
                case CSSPropertyBackgroundColor:
                    currValue = parseBackgroundColor();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyBackgroundAttachment:
                case CSSPropertyWebkitMaskAttachment:
                    if (val->id == CSSValueScroll || val->id == CSSValueFixed || val->id == CSSValueLocal) {
                        currValue = primitiveValueCache()->createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundImage:
                case CSSPropertyWebkitMaskImage:
                    if (parseFillImage(currValue))
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitBackgroundClip:
                case CSSPropertyWebkitBackgroundOrigin:
                case CSSPropertyWebkitMaskClip:
                case CSSPropertyWebkitMaskOrigin:
                    // The first three values here are deprecated and do not apply to the version of the property that has
                    // the -webkit- prefix removed.
                    if (val->id == CSSValueBorder || val->id == CSSValuePadding || val->id == CSSValueContent ||
                        val->id == CSSValueBorderBox || val->id == CSSValuePaddingBox || val->id == CSSValueContentBox ||
                        ((propId == CSSPropertyWebkitBackgroundClip || propId == CSSPropertyWebkitMaskClip) &&
                         (val->id == CSSValueText || val->id == CSSValueWebkitText))) {
                        currValue = primitiveValueCache()->createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundClip:
                    if (parseBackgroundClip(val, currValue, primitiveValueCache()))
                        m_valueList->next();
                    break;
                case CSSPropertyBackgroundOrigin:
                    if (val->id == CSSValueBorderBox || val->id == CSSValuePaddingBox || val->id == CSSValueContentBox) {
                        currValue = primitiveValueCache()->createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundPosition:
                case CSSPropertyWebkitMaskPosition:
                    parseFillPosition(m_valueList, currValue, currValue2);
                    // parseFillPosition advances the m_valueList pointer
                    break;
                case CSSPropertyBackgroundPositionX:
                case CSSPropertyWebkitMaskPositionX: {
                    currValue = parseFillPositionX(m_valueList);
                    if (currValue)
                        m_valueList->next();
                    break;
                }
                case CSSPropertyBackgroundPositionY:
                case CSSPropertyWebkitMaskPositionY: {
                    currValue = parseFillPositionY(m_valueList);
                    if (currValue)
                        m_valueList->next();
                    break;
                }
                case CSSPropertyWebkitBackgroundComposite:
                case CSSPropertyWebkitMaskComposite:
                    if ((val->id >= CSSValueClear && val->id <= CSSValuePlusLighter) || val->id == CSSValueHighlight) {
                        currValue = primitiveValueCache()->createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundRepeat:
                case CSSPropertyWebkitMaskRepeat:
                    parseFillRepeat(currValue, currValue2);
                    // parseFillRepeat advances the m_valueList pointer
                    break;
                case CSSPropertyBackgroundSize:
                case CSSPropertyWebkitBackgroundSize:
                case CSSPropertyWebkitMaskSize: {
                    currValue = parseFillSize(propId, allowComma);
                    if (currValue)
                        m_valueList->next();
                    break;
                }
            }
            if (!currValue)
                return false;

            if (value && !values) {
                values = CSSValueList::createCommaSeparated();
                values->append(value.release());
            }

            if (value2 && !values2) {
                values2 = CSSValueList::createCommaSeparated();
                values2->append(value2.release());
            }

            if (values)
                values->append(currValue.release());
            else
                value = currValue.release();
            if (currValue2) {
                if (values2)
                    values2->append(currValue2.release());
                else
                    value2 = currValue2.release();
            }
        }

        // When parsing any fill shorthand property, we let it handle building up the lists for all
        // properties.
        if (inShorthand())
            break;
    }

    if (values && values->length()) {
        retValue1 = values.release();
        if (values2 && values2->length())
            retValue2 = values2.release();
        return true;
    }
    if (value) {
        retValue1 = value.release();
        retValue2 = value2.release();
        return true;
    }
    return false;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationDelay()
{
    CSSParserValue* value = m_valueList->current();
    if (validUnit(value, FTime, m_strict))
        return primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationDirection()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNormal || value->id == CSSValueAlternate)
        return primitiveValueCache()->createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationDuration()
{
    CSSParserValue* value = m_valueList->current();
    if (validUnit(value, FTime | FNonNeg, m_strict))
        return primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationFillMode()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNone || value->id == CSSValueForwards || value->id == CSSValueBackwards || value->id == CSSValueBoth)
        return primitiveValueCache()->createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationIterationCount()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueInfinite)
        return primitiveValueCache()->createIdentifierValue(value->id);
    if (validUnit(value, FInteger | FNonNeg, m_strict))
        return primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationName()
{
    CSSParserValue* value = m_valueList->current();
    if (value->unit == CSSPrimitiveValue::CSS_STRING || value->unit == CSSPrimitiveValue::CSS_IDENT) {
        if (value->id == CSSValueNone || (value->unit == CSSPrimitiveValue::CSS_STRING && equalIgnoringCase(value->string, "none"))) {
            return primitiveValueCache()->createIdentifierValue(CSSValueNone);
        } else {
            return primitiveValueCache()->createValue(value->string, CSSPrimitiveValue::CSS_STRING);
        }
    }
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationPlayState()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueRunning || value->id == CSSValuePaused)
        return primitiveValueCache()->createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationProperty()
{
    CSSParserValue* value = m_valueList->current();
    if (value->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;
    int result = cssPropertyID(value->string);
    if (result)
        return primitiveValueCache()->createIdentifierValue(result);
    if (equalIgnoringCase(value->string, "all"))
        return primitiveValueCache()->createIdentifierValue(CSSValueAll);
    if (equalIgnoringCase(value->string, "none"))
        return primitiveValueCache()->createIdentifierValue(CSSValueNone);
    return 0;
}

bool CSSParser::parseTransformOriginShorthand(RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2, RefPtr<CSSValue>& value3)
{
    parseFillPosition(m_valueList, value1, value2);

    // now get z
    if (m_valueList->current()) {
        if (validUnit(m_valueList->current(), FLength, m_strict)) {
            value3 = primitiveValueCache()->createValue(m_valueList->current()->fValue,
                                             (CSSPrimitiveValue::UnitTypes)m_valueList->current()->unit);
            m_valueList->next();
            return true;
        }
        return false;
    }
    return true;
}

bool CSSParser::parseCubicBezierTimingFunctionValue(CSSParserValueList*& args, double& result)
{
    CSSParserValue* v = args->current();
    if (!validUnit(v, FNumber, m_strict))
        return false;
    result = v->fValue;
    if (result < 0 || result > 1.0)
        return false;
    v = args->next();
    if (!v)
        // The last number in the function has no comma after it, so we're done.
        return true;
    if (v->unit != CSSParserValue::Operator && v->iValue != ',')
        return false;
    v = args->next();
    return true;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationTimingFunction()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueEase || value->id == CSSValueLinear || value->id == CSSValueEaseIn || value->id == CSSValueEaseOut
        || value->id == CSSValueEaseInOut || value->id == CSSValueStepStart || value->id == CSSValueStepEnd)
        return primitiveValueCache()->createIdentifierValue(value->id);

    // We must be a function.
    if (value->unit != CSSParserValue::Function)
        return 0;

    CSSParserValueList* args = value->function->args.get();

    if (equalIgnoringCase(value->function->name, "steps(")) {
        // For steps, 1 or 2 params must be specified (comma-separated)
        if (!args || (args->size() != 1 && args->size() != 3))
            return 0;

        // There are two values.
        int numSteps;
        bool stepAtStart = false;

        CSSParserValue* v = args->current();
        if (!validUnit(v, FInteger, m_strict))
            return 0;
        numSteps = (int) min(v->fValue, (double)INT_MAX);
        if (numSteps < 1)
            return 0;
        v = args->next();

        if (v) {
            // There is a comma so we need to parse the second value
            if (v->unit != CSSParserValue::Operator && v->iValue != ',')
                return 0;
            v = args->next();
            if (v->id != CSSValueStart && v->id != CSSValueEnd)
                return 0;
            stepAtStart = v->id == CSSValueStart;
        }

        return CSSStepsTimingFunctionValue::create(numSteps, stepAtStart);
    }
    
    if (equalIgnoringCase(value->function->name, "cubic-bezier(")) {
        // For cubic bezier, 4 values must be specified.
        if (!args || args->size() != 7)
            return 0;

        // There are two points specified.  The values must be between 0 and 1.
        double x1, y1, x2, y2;

        if (!parseCubicBezierTimingFunctionValue(args, x1))
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, y1))
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, x2))
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, y2))
            return 0;

        return CSSCubicBezierTimingFunctionValue::create(x1, y1, x2, y2);
    }
    
    return 0;
}

bool CSSParser::parseAnimationProperty(int propId, RefPtr<CSSValue>& result)
{
    RefPtr<CSSValueList> values;
    CSSParserValue* val;
    RefPtr<CSSValue> value;
    bool allowComma = false;

    result = 0;

    while ((val = m_valueList->current())) {
        RefPtr<CSSValue> currValue;
        if (allowComma) {
            if (val->unit != CSSParserValue::Operator || val->iValue != ',')
                return false;
            m_valueList->next();
            allowComma = false;
        }
        else {
            switch (propId) {
                case CSSPropertyWebkitAnimationDelay:
                case CSSPropertyWebkitTransitionDelay:
                    currValue = parseAnimationDelay();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationDirection:
                    currValue = parseAnimationDirection();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationDuration:
                case CSSPropertyWebkitTransitionDuration:
                    currValue = parseAnimationDuration();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationFillMode:
                    currValue = parseAnimationFillMode();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationIterationCount:
                    currValue = parseAnimationIterationCount();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationName:
                    currValue = parseAnimationName();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationPlayState:
                    currValue = parseAnimationPlayState();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitTransitionProperty:
                    currValue = parseAnimationProperty();
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationTimingFunction:
                case CSSPropertyWebkitTransitionTimingFunction:
                    currValue = parseAnimationTimingFunction();
                    if (currValue)
                        m_valueList->next();
                    break;
            }

            if (!currValue)
                return false;

            if (value && !values) {
                values = CSSValueList::createCommaSeparated();
                values->append(value.release());
            }

            if (values)
                values->append(currValue.release());
            else
                value = currValue.release();

            allowComma = true;
        }

        // When parsing the 'transition' shorthand property, we let it handle building up the lists for all
        // properties.
        if (inShorthand())
            break;
    }

    if (values && values->length()) {
        result = values.release();
        return true;
    }
    if (value) {
        result = value.release();
        return true;
    }
    return false;
}



#if ENABLE(DASHBOARD_SUPPORT)

#define DASHBOARD_REGION_NUM_PARAMETERS  6
#define DASHBOARD_REGION_SHORT_NUM_PARAMETERS  2

static CSSParserValue* skipCommaInDashboardRegion(CSSParserValueList *args)
{
    if (args->size() == (DASHBOARD_REGION_NUM_PARAMETERS*2-1) ||
         args->size() == (DASHBOARD_REGION_SHORT_NUM_PARAMETERS*2-1)) {
        CSSParserValue* current = args->current();
        if (current->unit == CSSParserValue::Operator && current->iValue == ',')
            return args->next();
    }
    return args->current();
}

bool CSSParser::parseDashboardRegions(int propId, bool important)
{
    bool valid = true;

    CSSParserValue* value = m_valueList->current();

    if (value->id == CSSValueNone) {
        if (m_valueList->next())
            return false;
        addProperty(propId, primitiveValueCache()->createIdentifierValue(value->id), important);
        return valid;
    }

    RefPtr<DashboardRegion> firstRegion = DashboardRegion::create();
    DashboardRegion* region = 0;

    while (value) {
        if (region == 0) {
            region = firstRegion.get();
        } else {
            RefPtr<DashboardRegion> nextRegion = DashboardRegion::create();
            region->m_next = nextRegion;
            region = nextRegion.get();
        }

        if (value->unit != CSSParserValue::Function) {
            valid = false;
            break;
        }

        // Commas count as values, so allow:
        // dashboard-region(label, type, t, r, b, l) or dashboard-region(label type t r b l)
        // dashboard-region(label, type, t, r, b, l) or dashboard-region(label type t r b l)
        // also allow
        // dashboard-region(label, type) or dashboard-region(label type)
        // dashboard-region(label, type) or dashboard-region(label type)
        CSSParserValueList* args = value->function->args.get();
        if (!equalIgnoringCase(value->function->name, "dashboard-region(") || !args) {
            valid = false;
            break;
        }

        int numArgs = args->size();
        if ((numArgs != DASHBOARD_REGION_NUM_PARAMETERS && numArgs != (DASHBOARD_REGION_NUM_PARAMETERS*2-1)) &&
            (numArgs != DASHBOARD_REGION_SHORT_NUM_PARAMETERS && numArgs != (DASHBOARD_REGION_SHORT_NUM_PARAMETERS*2-1))) {
            valid = false;
            break;
        }

        // First arg is a label.
        CSSParserValue* arg = args->current();
        if (arg->unit != CSSPrimitiveValue::CSS_IDENT) {
            valid = false;
            break;
        }

        region->m_label = arg->string;

        // Second arg is a type.
        arg = args->next();
        arg = skipCommaInDashboardRegion(args);
        if (arg->unit != CSSPrimitiveValue::CSS_IDENT) {
            valid = false;
            break;
        }

        if (equalIgnoringCase(arg->string, "circle"))
            region->m_isCircle = true;
        else if (equalIgnoringCase(arg->string, "rectangle"))
            region->m_isRectangle = true;
        else {
            valid = false;
            break;
        }

        region->m_geometryType = arg->string;

        if (numArgs == DASHBOARD_REGION_SHORT_NUM_PARAMETERS || numArgs == (DASHBOARD_REGION_SHORT_NUM_PARAMETERS*2-1)) {
            // This originally used CSSValueInvalid by accident. It might be more logical to use something else.
            RefPtr<CSSPrimitiveValue> amount = primitiveValueCache()->createIdentifierValue(CSSValueInvalid);

            region->setTop(amount);
            region->setRight(amount);
            region->setBottom(amount);
            region->setLeft(amount);
        } else {
            // Next four arguments must be offset numbers
            int i;
            for (i = 0; i < 4; i++) {
                arg = args->next();
                arg = skipCommaInDashboardRegion(args);

                valid = arg->id == CSSValueAuto || validUnit(arg, FLength, m_strict);
                if (!valid)
                    break;

                RefPtr<CSSPrimitiveValue> amount = arg->id == CSSValueAuto ?
                    primitiveValueCache()->createIdentifierValue(CSSValueAuto) :
                    primitiveValueCache()->createValue(arg->fValue, (CSSPrimitiveValue::UnitTypes) arg->unit);

                if (i == 0)
                    region->setTop(amount);
                else if (i == 1)
                    region->setRight(amount);
                else if (i == 2)
                    region->setBottom(amount);
                else
                    region->setLeft(amount);
            }
        }

        if (args->next())
            return false;

        value = m_valueList->next();
    }

    if (valid)
        addProperty(propId, primitiveValueCache()->createValue(firstRegion.release()), important);

    return valid;
}

#endif /* ENABLE(DASHBOARD_SUPPORT) */

PassRefPtr<CSSValue> CSSParser::parseCounterContent(CSSParserValueList* args, bool counters)
{
    unsigned numArgs = args->size();
    if (counters && numArgs != 3 && numArgs != 5)
        return 0;
    if (!counters && numArgs != 1 && numArgs != 3)
        return 0;

    CSSParserValue* i = args->current();
    if (i->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;
    RefPtr<CSSPrimitiveValue> identifier = primitiveValueCache()->createValue(i->string, CSSPrimitiveValue::CSS_STRING);

    RefPtr<CSSPrimitiveValue> separator;
    if (!counters)
        separator = primitiveValueCache()->createValue(String(), CSSPrimitiveValue::CSS_STRING);
    else {
        i = args->next();
        if (i->unit != CSSParserValue::Operator || i->iValue != ',')
            return 0;

        i = args->next();
        if (i->unit != CSSPrimitiveValue::CSS_STRING)
            return 0;

        separator = primitiveValueCache()->createValue(i->string, (CSSPrimitiveValue::UnitTypes) i->unit);
    }

    RefPtr<CSSPrimitiveValue> listStyle;
    i = args->next();
    if (!i) // Make the list style default decimal
        listStyle = primitiveValueCache()->createValue(CSSValueDecimal - CSSValueDisc, CSSPrimitiveValue::CSS_NUMBER);
    else {
        if (i->unit != CSSParserValue::Operator || i->iValue != ',')
            return 0;

        i = args->next();
        if (i->unit != CSSPrimitiveValue::CSS_IDENT)
            return 0;

        short ls = 0;
        if (i->id == CSSValueNone)
            ls = CSSValueKatakanaIroha - CSSValueDisc + 1;
        else if (i->id >= CSSValueDisc && i->id <= CSSValueKatakanaIroha)
            ls = i->id - CSSValueDisc;
        else
            return 0;

        listStyle = primitiveValueCache()->createValue(ls, (CSSPrimitiveValue::UnitTypes) i->unit);
    }

    return primitiveValueCache()->createValue(Counter::create(identifier.release(), listStyle.release(), separator.release()));
}

bool CSSParser::parseShape(int propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    CSSParserValueList* args = value->function->args.get();

    if (!equalIgnoringCase(value->function->name, "rect(") || !args)
        return false;

    // rect(t, r, b, l) || rect(t r b l)
    if (args->size() != 4 && args->size() != 7)
        return false;
    RefPtr<Rect> rect = Rect::create();
    bool valid = true;
    int i = 0;
    CSSParserValue* a = args->current();
    while (a) {
        valid = a->id == CSSValueAuto || validUnit(a, FLength, m_strict);
        if (!valid)
            break;
        RefPtr<CSSPrimitiveValue> length = a->id == CSSValueAuto ?
            primitiveValueCache()->createIdentifierValue(CSSValueAuto) :
            primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes) a->unit);
        if (i == 0)
            rect->setTop(length);
        else if (i == 1)
            rect->setRight(length);
        else if (i == 2)
            rect->setBottom(length);
        else
            rect->setLeft(length);
        a = args->next();
        if (a && args->size() == 7) {
            if (a->unit == CSSParserValue::Operator && a->iValue == ',') {
                a = args->next();
            } else {
                valid = false;
                break;
            }
        }
        i++;
    }
    if (valid) {
        addProperty(propId, primitiveValueCache()->createValue(rect.release()), important);
        m_valueList->next();
        return true;
    }
    return false;
}

// [ 'font-style' || 'font-variant' || 'font-weight' ]? 'font-size' [ / 'line-height' ]? 'font-family'
bool CSSParser::parseFont(bool important)
{
    bool valid = true;
    CSSParserValue *value = m_valueList->current();
    RefPtr<FontValue> font = FontValue::create();
    // optional font-style, font-variant and font-weight
    while (value) {
        int id = value->id;
        if (id) {
            if (id == CSSValueNormal) {
                // do nothing, it's the inital value for all three
            } else if (id == CSSValueItalic || id == CSSValueOblique) {
                if (font->style)
                    return false;
                font->style = primitiveValueCache()->createIdentifierValue(id);
            } else if (id == CSSValueSmallCaps) {
                if (font->variant)
                    return false;
                font->variant = primitiveValueCache()->createIdentifierValue(id);
            } else if (id >= CSSValueBold && id <= CSSValueLighter) {
                if (font->weight)
                    return false;
                font->weight = primitiveValueCache()->createIdentifierValue(id);
            } else {
                valid = false;
            }
        } else if (!font->weight && validUnit(value, FInteger | FNonNeg, true)) {
            int weight = (int)value->fValue;
            int val = 0;
            if (weight == 100)
                val = CSSValue100;
            else if (weight == 200)
                val = CSSValue200;
            else if (weight == 300)
                val = CSSValue300;
            else if (weight == 400)
                val = CSSValue400;
            else if (weight == 500)
                val = CSSValue500;
            else if (weight == 600)
                val = CSSValue600;
            else if (weight == 700)
                val = CSSValue700;
            else if (weight == 800)
                val = CSSValue800;
            else if (weight == 900)
                val = CSSValue900;

            if (val)
                font->weight = primitiveValueCache()->createIdentifierValue(val);
            else
                valid = false;
        } else {
            valid = false;
        }
        if (!valid)
            break;
        value = m_valueList->next();
    }
    if (!value)
        return false;

    // set undefined values to default
    if (!font->style)
        font->style = primitiveValueCache()->createIdentifierValue(CSSValueNormal);
    if (!font->variant)
        font->variant = primitiveValueCache()->createIdentifierValue(CSSValueNormal);
    if (!font->weight)
        font->weight = primitiveValueCache()->createIdentifierValue(CSSValueNormal);

    // now a font size _must_ come
    // <absolute-size> | <relative-size> | <length> | <percentage> | inherit
    if (value->id >= CSSValueXxSmall && value->id <= CSSValueLarger)
        font->size = primitiveValueCache()->createIdentifierValue(value->id);
    else if (validUnit(value, FLength | FPercent | FNonNeg, m_strict))
        font->size = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
    value = m_valueList->next();
    if (!font->size || !value)
        return false;

    if (value->unit == CSSParserValue::Operator && value->iValue == '/') {
        // line-height
        value = m_valueList->next();
        if (!value)
            return false;
        if (value->id == CSSValueNormal) {
            // default value, nothing to do
        } else if (validUnit(value, FNumber | FLength | FPercent | FNonNeg, m_strict))
            font->lineHeight = primitiveValueCache()->createValue(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
        else
            return false;
        value = m_valueList->next();
        if (!value)
            return false;
    }

    if (!font->lineHeight)
        font->lineHeight = primitiveValueCache()->createIdentifierValue(CSSValueNormal);

    // font family must come now
    font->family = parseFontFamily();

    if (m_valueList->current() || !font->family)
        return false;

    addProperty(CSSPropertyFont, font.release(), important);
    return true;
}

PassRefPtr<CSSValueList> CSSParser::parseFontFamily()
{
    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    CSSParserValue* value = m_valueList->current();

    FontFamilyValue* currFamily = 0;
    while (value) {
        CSSParserValue* nextValue = m_valueList->next();
        bool nextValBreaksFont = !nextValue ||
                                 (nextValue->unit == CSSParserValue::Operator && nextValue->iValue == ',');
        bool nextValIsFontName = nextValue &&
            ((nextValue->id >= CSSValueSerif && nextValue->id <= CSSValueWebkitBody) ||
            (nextValue->unit == CSSPrimitiveValue::CSS_STRING || nextValue->unit == CSSPrimitiveValue::CSS_IDENT));

        if (value->id >= CSSValueSerif && value->id <= CSSValueWebkitBody) {
            if (currFamily)
                currFamily->appendSpaceSeparated(value->string.characters, value->string.length);
            else if (nextValBreaksFont || !nextValIsFontName)
                list->append(primitiveValueCache()->createIdentifierValue(value->id));
            else {
                RefPtr<FontFamilyValue> newFamily = FontFamilyValue::create(value->string);
                currFamily = newFamily.get();
                list->append(newFamily.release());
            }
        } else if (value->unit == CSSPrimitiveValue::CSS_STRING) {
            // Strings never share in a family name.
            currFamily = 0;
            list->append(FontFamilyValue::create(value->string));
        } else if (value->unit == CSSPrimitiveValue::CSS_IDENT) {
            if (currFamily)
                currFamily->appendSpaceSeparated(value->string.characters, value->string.length);
            else if (nextValBreaksFont || !nextValIsFontName)
                list->append(FontFamilyValue::create(value->string));
            else {
                RefPtr<FontFamilyValue> newFamily = FontFamilyValue::create(value->string);
                currFamily = newFamily.get();
                list->append(newFamily.release());
            }
        } else {
            break;
        }

        if (!nextValue)
            break;

        if (nextValBreaksFont) {
            value = m_valueList->next();
            currFamily = 0;
        }
        else if (nextValIsFontName)
            value = nextValue;
        else
            break;
    }
    if (!list->length())
        list = 0;
    return list.release();
}

bool CSSParser::parseFontStyle(bool important)
{
    RefPtr<CSSValueList> values;
    if (m_valueList->size() > 1)
        values = CSSValueList::createCommaSeparated();
    CSSParserValue* val;
    bool expectComma = false;
    while ((val = m_valueList->current())) {
        RefPtr<CSSPrimitiveValue> parsedValue;
        if (!expectComma) {
            expectComma = true;
            if (val->id == CSSValueNormal || val->id == CSSValueItalic || val->id == CSSValueOblique)
                parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
            else if (val->id == CSSValueAll && !values) {
                // 'all' is only allowed in @font-face and with no other values. Make a value list to
                // indicate that we are in the @font-face case.
                values = CSSValueList::createCommaSeparated();
                parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
            }
        } else if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            expectComma = false;
            m_valueList->next();
            continue;
        }

        if (!parsedValue)
            return false;

        m_valueList->next();

        if (values)
            values->append(parsedValue.release());
        else {
            addProperty(CSSPropertyFontStyle, parsedValue.release(), important);
            return true;
        }
    }

    if (values && values->length()) {
        m_hasFontFaceOnlyValues = true;
        addProperty(CSSPropertyFontStyle, values.release(), important);
        return true;
    }

    return false;
}

bool CSSParser::parseFontVariant(bool important)
{
    RefPtr<CSSValueList> values;
    if (m_valueList->size() > 1)
        values = CSSValueList::createCommaSeparated();
    CSSParserValue* val;
    bool expectComma = false;
    while ((val = m_valueList->current())) {
        RefPtr<CSSPrimitiveValue> parsedValue;
        if (!expectComma) {
            expectComma = true;
            if (val->id == CSSValueNormal || val->id == CSSValueSmallCaps)
                parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
            else if (val->id == CSSValueAll && !values) {
                // 'all' is only allowed in @font-face and with no other values. Make a value list to
                // indicate that we are in the @font-face case.
                values = CSSValueList::createCommaSeparated();
                parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
            }
        } else if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            expectComma = false;
            m_valueList->next();
            continue;
        }

        if (!parsedValue)
            return false;

        m_valueList->next();

        if (values)
            values->append(parsedValue.release());
        else {
            addProperty(CSSPropertyFontVariant, parsedValue.release(), important);
            return true;
        }
    }

    if (values && values->length()) {
        m_hasFontFaceOnlyValues = true;
        addProperty(CSSPropertyFontVariant, values.release(), important);
        return true;
    }

    return false;
}

bool CSSParser::parseFontWeight(bool important)
{
    RefPtr<CSSValueList> values;
    if (m_valueList->size() > 1)
        values = CSSValueList::createCommaSeparated();
    CSSParserValue* val;
    bool expectComma = false;
    while ((val = m_valueList->current())) {
        RefPtr<CSSPrimitiveValue> parsedValue;
        if (!expectComma) {
            expectComma = true;
            if (val->unit == CSSPrimitiveValue::CSS_IDENT) {
                if (val->id >= CSSValueNormal && val->id <= CSSValue900)
                    parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
                else if (val->id == CSSValueAll && !values) {
                    // 'all' is only allowed in @font-face and with no other values. Make a value list to
                    // indicate that we are in the @font-face case.
                    values = CSSValueList::createCommaSeparated();
                    parsedValue = primitiveValueCache()->createIdentifierValue(val->id);
                }
            } else if (validUnit(val, FInteger | FNonNeg, false)) {
                int weight = static_cast<int>(val->fValue);
                if (!(weight % 100) && weight >= 100 && weight <= 900)
                    parsedValue = primitiveValueCache()->createIdentifierValue(CSSValue100 + weight / 100 - 1);
            }
        } else if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            expectComma = false;
            m_valueList->next();
            continue;
        }

        if (!parsedValue)
            return false;

        m_valueList->next();

        if (values)
            values->append(parsedValue.release());
        else {
            addProperty(CSSPropertyFontWeight, parsedValue.release(), important);
            return true;
        }
    }

    if (values && values->length()) {
        m_hasFontFaceOnlyValues = true;
        addProperty(CSSPropertyFontWeight, values.release(), important);
        return true;
    }

    return false;
}

static bool isValidFormatFunction(CSSParserValue* val)
{
    CSSParserValueList* args = val->function->args.get();
    return equalIgnoringCase(val->function->name, "format(") && (args->current()->unit == CSSPrimitiveValue::CSS_STRING || args->current()->unit == CSSPrimitiveValue::CSS_IDENT);
}

bool CSSParser::parseFontFaceSrc()
{
    RefPtr<CSSValueList> values(CSSValueList::createCommaSeparated());
    CSSParserValue* val;
    bool expectComma = false;
    bool allowFormat = false;
    bool failed = false;
    RefPtr<CSSFontFaceSrcValue> uriValue;
    while ((val = m_valueList->current())) {
        RefPtr<CSSFontFaceSrcValue> parsedValue;
        if (val->unit == CSSPrimitiveValue::CSS_URI && !expectComma && m_styleSheet) {
            // FIXME: The completeURL call should be done when using the CSSFontFaceSrcValue,
            // not when creating it.
            parsedValue = CSSFontFaceSrcValue::create(m_styleSheet->completeURL(val->string));
            uriValue = parsedValue;
            allowFormat = true;
            expectComma = true;
        } else if (val->unit == CSSParserValue::Function) {
            // There are two allowed functions: local() and format().
            CSSParserValueList* args = val->function->args.get();
            if (args && args->size() == 1) {
                if (equalIgnoringCase(val->function->name, "local(") && !expectComma && (args->current()->unit == CSSPrimitiveValue::CSS_STRING || args->current()->unit == CSSPrimitiveValue::CSS_IDENT)) {
                    expectComma = true;
                    allowFormat = false;
                    CSSParserValue* a = args->current();
                    uriValue.clear();
                    parsedValue = CSSFontFaceSrcValue::createLocal(a->string);
                } else if (allowFormat && uriValue && isValidFormatFunction(val)) {
                    expectComma = true;
                    allowFormat = false;
                    uriValue->setFormat(args->current()->string);
                    uriValue.clear();
                    m_valueList->next();
                    continue;
                }
            }
        } else if (val->unit == CSSParserValue::Operator && val->iValue == ',' && expectComma) {
            expectComma = false;
            allowFormat = false;
            uriValue.clear();
            m_valueList->next();
            continue;
        }

        if (parsedValue)
            values->append(parsedValue.release());
        else {
            failed = true;
            break;
        }
        m_valueList->next();
    }

    if (values->length() && !failed) {
        addProperty(CSSPropertySrc, values.release(), m_important);
        m_valueList->next();
        return true;
    }

    return false;
}

bool CSSParser::parseFontFaceUnicodeRange()
{
    RefPtr<CSSValueList> values = CSSValueList::createCommaSeparated();
    bool failed = false;
    bool operatorExpected = false;
    for (; m_valueList->current(); m_valueList->next(), operatorExpected = !operatorExpected) {
        if (operatorExpected) {
            if (m_valueList->current()->unit == CSSParserValue::Operator && m_valueList->current()->iValue == ',')
                continue;
            failed = true;
            break;
        }
        if (m_valueList->current()->unit != CSSPrimitiveValue::CSS_UNICODE_RANGE) {
            failed = true;
            break;
        }

        String rangeString = m_valueList->current()->string;
        UChar32 from = 0;
        UChar32 to = 0;
        unsigned length = rangeString.length();

        if (length < 3) {
            failed = true;
            break;
        }

        unsigned i = 2;
        while (i < length) {
            UChar c = rangeString[i];
            if (c == '-' || c == '?')
                break;
            from *= 16;
            if (c >= '0' && c <= '9')
                from += c - '0';
            else if (c >= 'A' && c <= 'F')
                from += 10 + c - 'A';
            else if (c >= 'a' && c <= 'f')
                from += 10 + c - 'a';
            else {
                failed = true;
                break;
            }
            i++;
        }
        if (failed)
            break;

        if (i == length)
            to = from;
        else if (rangeString[i] == '?') {
            unsigned span = 1;
            while (i < length && rangeString[i] == '?') {
                span *= 16;
                from *= 16;
                i++;
            }
            if (i < length)
                failed = true;
            to = from + span - 1;
        } else {
            if (length < i + 2) {
                failed = true;
                break;
            }
            i++;
            while (i < length) {
                UChar c = rangeString[i];
                to *= 16;
                if (c >= '0' && c <= '9')
                    to += c - '0';
                else if (c >= 'A' && c <= 'F')
                    to += 10 + c - 'A';
                else if (c >= 'a' && c <= 'f')
                    to += 10 + c - 'a';
                else {
                    failed = true;
                    break;
                }
                i++;
            }
            if (failed)
                break;
        }
        if (from <= to)
            values->append(CSSUnicodeRangeValue::create(from, to));
    }
    if (failed || !values->length())
        return false;
    addProperty(CSSPropertyUnicodeRange, values.release(), m_important);
    return true;
}

// Returns the number of characters which form a valid double
// and are terminated by the given terminator character
static int checkForValidDouble(const UChar* string, const UChar* end, const char terminator)
{
    int length = end - string;
    if (length < 1)
        return 0;

    bool decimalMarkSeen = false;
    int processedLength = 0;

    for (int i = 0; i < length; ++i) {
        if (string[i] == terminator) {
            processedLength = i;
            break;
        }
        if (!isASCIIDigit(string[i])) {
            if (!decimalMarkSeen && string[i] == '.')
                decimalMarkSeen = true;
            else
                return 0;
        }
    }

    if (decimalMarkSeen && processedLength == 1)
        return 0;

    return processedLength;
}

// Returns the number of characters consumed for parsing a valid double
// terminated by the given terminator character
static int parseDouble(const UChar* string, const UChar* end, const char terminator, double& value)
{
    int length = checkForValidDouble(string, end, terminator);
    if (!length)
        return 0;

    int position = 0;
    double localValue = 0;

    // The consumed characters here are guaranteed to be
    // ASCII digits with or without a decimal mark
    for (; position < length; ++position) {
        if (string[position] == '.')
            break;
        localValue = localValue * 10 + string[position] - '0';
    }

    if (++position == length) {
        value = localValue;
        return length;
    }

    double fraction = 0;
    double scale = 1;

    while (position < length && scale < MAX_SCALE) {
        fraction = fraction * 10 + string[position++] - '0';
        scale *= 10;
    }

    value = localValue + fraction / scale;
    return length;
}

static bool parseColorIntOrPercentage(const UChar*& string, const UChar* end, const char terminator, CSSPrimitiveValue::UnitTypes& expect, int& value)
{
    const UChar* current = string;
    double localValue = 0;
    bool negative = false;
    while (current != end && isHTMLSpace(*current))
        current++;
    if (current != end && *current == '-') {
        negative = true;
        current++;
    }
    if (current == end || !isASCIIDigit(*current))
        return false;
    while (current != end && isASCIIDigit(*current)) {
        double newValue = localValue * 10 + *current++ - '0';
        if (newValue >= 255) {
            // Clamp values at 255.
            localValue = 255;
            while (current != end && isASCIIDigit(*current))
                ++current;
            break;
        }
        localValue = newValue;
    }

    if (current == end)
        return false;

    if (expect == CSSPrimitiveValue::CSS_NUMBER && (*current == '.' || *current == '%'))
        return false;

    if (*current == '.') {
        // We already parsed the integral part, try to parse
        // the fraction part of the percentage value.
        double percentage = 0;
        int numCharactersParsed = parseDouble(current, end, '%', percentage);
        if (!numCharactersParsed)
            return false;
        current += numCharactersParsed;
        if (*current != '%')
            return false;
        localValue += percentage;
    }

    if (expect == CSSPrimitiveValue::CSS_PERCENTAGE && *current != '%')
        return false;

    if (*current == '%') {
        expect = CSSPrimitiveValue::CSS_PERCENTAGE;
        localValue = localValue / 100.0 * 256.0;
        // Clamp values at 255 for percentages over 100%
        if (localValue > 255)
            localValue = 255;
        current++;
    } else
        expect = CSSPrimitiveValue::CSS_NUMBER;

    while (current != end && isHTMLSpace(*current))
        current++;
    if (current == end || *current++ != terminator)
        return false;
    // Clamp negative values at zero.
    value = negative ? 0 : static_cast<int>(localValue);
    string = current;
    return true;
}

static inline bool isTenthAlpha(const UChar* string, const int length)
{
    // "0.X"
    if (length == 3 && string[0] == '0' && string[1] == '.' && isASCIIDigit(string[2]))
        return true;

    // ".X"
    if (length == 2 && string[0] == '.' && isASCIIDigit(string[1]))
        return true;

    return false;
}

static inline bool parseAlphaValue(const UChar*& string, const UChar* end, const char terminator, int& value)
{
    while (string != end && isHTMLSpace(*string))
        string++;

    bool negative = false;

    if (string != end && *string == '-') {
        negative = true;
        string++;
    }

    value = 0;

    int length = end - string;
    if (length < 2)
        return false;

    if (string[length - 1] != terminator)
        return false;

    if (string[0] != '0' && string[0] != '1' && string[0] != '.') {
        if (checkForValidDouble(string, end, terminator)) {
            value = negative ? 0 : 255;
            string = end;
            return true;
        }
        return false;
    }

    if (length == 2 && string[0] != '.') {
        value = !negative && string[0] == '1' ? 255 : 0;
        string = end;
        return true;
    }

    if (isTenthAlpha(string, length - 1)) {
        static const int tenthAlphaValues[] = { 0, 25, 51, 76, 102, 127, 153, 179, 204, 230 };
        value = negative ? 0 : tenthAlphaValues[string[length - 2] - '0'];
        string = end;
        return true;
    }

    double alpha = 0;
    if (!parseDouble(string, end, terminator, alpha))
        return false;
    value = negative ? 0 : static_cast<int>(alpha * nextafter(256.0, 0.0));
    string = end;
    return true;
}

static inline bool mightBeRGBA(const UChar* characters, unsigned length)
{
    if (length < 5)
        return false;
    return characters[4] == '('
        && (characters[0] | 0x20) == 'r'
        && (characters[1] | 0x20) == 'g'
        && (characters[2] | 0x20) == 'b'
        && (characters[3] | 0x20) == 'a';
}

static inline bool mightBeRGB(const UChar* characters, unsigned length)
{
    if (length < 4)
        return false;
    return characters[3] == '('
        && (characters[0] | 0x20) == 'r'
        && (characters[1] | 0x20) == 'g'
        && (characters[2] | 0x20) == 'b';
}

bool CSSParser::parseColor(const String &name, RGBA32& rgb, bool strict)
{
    const UChar* characters = name.characters();
    unsigned length = name.length();
    CSSPrimitiveValue::UnitTypes expect = CSSPrimitiveValue::CSS_UNKNOWN;

    if (!strict && length >= 3) {
        if (name[0] == '#') {
            if (Color::parseHexColor(characters + 1, length - 1, rgb))
                return true;
        } else {
            if (Color::parseHexColor(characters, length, rgb))
                return true;
        }
    }

    // Try rgba() syntax.
    if (mightBeRGBA(characters, length)) {
        const UChar* current = characters + 5;
        const UChar* end = characters + length;
        int red;
        int green;
        int blue;
        int alpha;

        if (!parseColorIntOrPercentage(current, end, ',', expect, red))
            return false;
        if (!parseColorIntOrPercentage(current, end, ',', expect, green))
            return false;
        if (!parseColorIntOrPercentage(current, end, ',', expect, blue))
            return false;
        if (!parseAlphaValue(current, end, ')', alpha))
            return false;
        if (current != end)
            return false;
        rgb = makeRGBA(red, green, blue, alpha);
        return true;
    }

    // Try rgb() syntax.
    if (mightBeRGB(characters, length)) {
        const UChar* current = characters + 4;
        const UChar* end = characters + length;
        int red;
        int green;
        int blue;
        if (!parseColorIntOrPercentage(current, end, ',', expect, red))
            return false;
        if (!parseColorIntOrPercentage(current, end, ',', expect, green))
            return false;
        if (!parseColorIntOrPercentage(current, end, ')', expect, blue))
            return false;
        if (current != end)
            return false;
        rgb = makeRGB(red, green, blue);
        return true;
    }

    // Try named colors.
    Color tc;
    tc.setNamedColor(name);
    if (tc.isValid()) {
        rgb = tc.rgb();
        return true;
    }
    return false;
}

static inline int colorIntFromValue(CSSParserValue* v)
{
    if (v->fValue <= 0.0)
        return 0;

    if (v->unit == CSSPrimitiveValue::CSS_PERCENTAGE) {
        if (v->fValue >= 100.0)
            return 255;
        return static_cast<int>(v->fValue * 256.0 / 100.0);
    }

    if (v->fValue >= 255.0)
        return 255;

    return static_cast<int>(v->fValue);
}

bool CSSParser::parseColorParameters(CSSParserValue* value, int* colorArray, bool parseAlpha)
{
    CSSParserValueList* args = value->function->args.get();
    CSSParserValue* v = args->current();
    Units unitType = FUnknown;
    // Get the first value and its type
    if (validUnit(v, FInteger, true))
        unitType = FInteger;
    else if (validUnit(v, FPercent, true))
        unitType = FPercent;
    else
        return false;
    colorArray[0] = colorIntFromValue(v);
    for (int i = 1; i < 3; i++) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, unitType, true))
            return false;
        colorArray[i] = colorIntFromValue(v);
    }
    if (parseAlpha) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FNumber, true))
            return false;
        // Convert the floating pointer number of alpha to an integer in the range [0, 256),
        // with an equal distribution across all 256 values.
        colorArray[3] = static_cast<int>(max(0.0, min(1.0, v->fValue)) * nextafter(256.0, 0.0));
    }
    return true;
}

// The CSS3 specification defines the format of a HSL color as
// hsl(<number>, <percent>, <percent>)
// and with alpha, the format is
// hsla(<number>, <percent>, <percent>, <number>)
// The first value, HUE, is in an angle with a value between 0 and 360
bool CSSParser::parseHSLParameters(CSSParserValue* value, double* colorArray, bool parseAlpha)
{
    CSSParserValueList* args = value->function->args.get();
    CSSParserValue* v = args->current();
    // Get the first value
    if (!validUnit(v, FNumber, true))
        return false;
    // normalize the Hue value and change it to be between 0 and 1.0
    colorArray[0] = (((static_cast<int>(v->fValue) % 360) + 360) % 360) / 360.0;
    for (int i = 1; i < 3; i++) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FPercent, true))
            return false;
        colorArray[i] = max(0.0, min(100.0, v->fValue)) / 100.0; // needs to be value between 0 and 1.0
    }
    if (parseAlpha) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FNumber, true))
            return false;
        colorArray[3] = max(0.0, min(1.0, v->fValue));
    }
    return true;
}

PassRefPtr<CSSPrimitiveValue> CSSParser::parseColor(CSSParserValue* value)
{
    RGBA32 c = Color::transparent;
    if (!parseColorFromValue(value ? value : m_valueList->current(), c))
        return 0;
    return primitiveValueCache()->createColorValue(c);
}

bool CSSParser::parseColorFromValue(CSSParserValue* value, RGBA32& c)
{
    if (!m_strict && value->unit == CSSPrimitiveValue::CSS_NUMBER &&
        value->fValue >= 0. && value->fValue < 1000000.) {
        String str = String::format("%06d", (int)(value->fValue+.5));
        if (!CSSParser::parseColor(str, c, m_strict))
            return false;
    } else if (value->unit == CSSPrimitiveValue::CSS_PARSER_HEXCOLOR ||
                value->unit == CSSPrimitiveValue::CSS_IDENT ||
                (!m_strict && value->unit == CSSPrimitiveValue::CSS_DIMENSION)) {
        if (!CSSParser::parseColor(value->string, c, m_strict && value->unit == CSSPrimitiveValue::CSS_IDENT))
            return false;
    } else if (value->unit == CSSParserValue::Function &&
                value->function->args != 0 &&
                value->function->args->size() == 5 /* rgb + two commas */ &&
                equalIgnoringCase(value->function->name, "rgb(")) {
        int colorValues[3];
        if (!parseColorParameters(value, colorValues, false))
            return false;
        c = makeRGB(colorValues[0], colorValues[1], colorValues[2]);
    } else {
        if (value->unit == CSSParserValue::Function &&
                value->function->args != 0 &&
                value->function->args->size() == 7 /* rgba + three commas */ &&
                equalIgnoringCase(value->function->name, "rgba(")) {
            int colorValues[4];
            if (!parseColorParameters(value, colorValues, true))
                return false;
            c = makeRGBA(colorValues[0], colorValues[1], colorValues[2], colorValues[3]);
        } else if (value->unit == CSSParserValue::Function &&
                    value->function->args != 0 &&
                    value->function->args->size() == 5 /* hsl + two commas */ &&
                    equalIgnoringCase(value->function->name, "hsl(")) {
            double colorValues[3];
            if (!parseHSLParameters(value, colorValues, false))
                return false;
            c = makeRGBAFromHSLA(colorValues[0], colorValues[1], colorValues[2], 1.0);
        } else if (value->unit == CSSParserValue::Function &&
                    value->function->args != 0 &&
                    value->function->args->size() == 7 /* hsla + three commas */ &&
                    equalIgnoringCase(value->function->name, "hsla(")) {
            double colorValues[4];
            if (!parseHSLParameters(value, colorValues, true))
                return false;
            c = makeRGBAFromHSLA(colorValues[0], colorValues[1], colorValues[2], colorValues[3]);
        } else
            return false;
    }

    return true;
}

// This class tracks parsing state for shadow values.  If it goes out of scope (e.g., due to an early return)
// without the allowBreak bit being set, then it will clean up all of the objects and destroy them.
struct ShadowParseContext {
    ShadowParseContext(CSSPropertyID prop, CSSPrimitiveValueCache* primitiveValueCache)
        : property(prop)
        , m_primitiveValueCache(primitiveValueCache)
        , allowX(true)
        , allowY(false)
        , allowBlur(false)
        , allowSpread(false)
        , allowColor(true)
        , allowStyle(prop == CSSPropertyWebkitBoxShadow || prop == CSSPropertyBoxShadow)
        , allowBreak(true)
    {
    }

    bool allowLength() { return allowX || allowY || allowBlur || allowSpread; }

    void commitValue()
    {
        // Handle the ,, case gracefully by doing nothing.
        if (x || y || blur || spread || color || style) {
            if (!values)
                values = CSSValueList::createCommaSeparated();

            // Construct the current shadow value and add it to the list.
            values->append(ShadowValue::create(x.release(), y.release(), blur.release(), spread.release(), style.release(), color.release()));
        }

        // Now reset for the next shadow value.
        x = 0;
        y = 0;
        blur = 0;
        spread = 0;
        style = 0;
        color = 0;

        allowX = true;
        allowColor = true;
        allowBreak = true;
        allowY = false;
        allowBlur = false;
        allowSpread = false;
        allowStyle = property == CSSPropertyWebkitBoxShadow || property == CSSPropertyBoxShadow;
    }

    void commitLength(CSSParserValue* v)
    {
        RefPtr<CSSPrimitiveValue> val = m_primitiveValueCache->createValue(v->fValue, (CSSPrimitiveValue::UnitTypes)v->unit);

        if (allowX) {
            x = val.release();
            allowX = false;
            allowY = true;
            allowColor = false;
            allowStyle = false;
            allowBreak = false;
        } else if (allowY) {
            y = val.release();
            allowY = false;
            allowBlur = true;
            allowColor = true;
            allowStyle = property == CSSPropertyWebkitBoxShadow || property == CSSPropertyBoxShadow;
            allowBreak = true;
        } else if (allowBlur) {
            blur = val.release();
            allowBlur = false;
            allowSpread = property == CSSPropertyWebkitBoxShadow || property == CSSPropertyBoxShadow;
        } else if (allowSpread) {
            spread = val.release();
            allowSpread = false;
        }
    }

    void commitColor(PassRefPtr<CSSPrimitiveValue> val)
    {
        color = val;
        allowColor = false;
        if (allowX) {
            allowStyle = false;
            allowBreak = false;
        } else {
            allowBlur = false;
            allowSpread = false;
            allowStyle = property == CSSPropertyWebkitBoxShadow || property == CSSPropertyBoxShadow;
        }
    }

    void commitStyle(CSSParserValue* v)
    {
        style = m_primitiveValueCache->createIdentifierValue(v->id);
        allowStyle = false;
        if (allowX)
            allowBreak = false;
        else {
            allowBlur = false;
            allowSpread = false;
            allowColor = false;
        }
    }

    CSSPropertyID property;
    CSSPrimitiveValueCache* m_primitiveValueCache;

    RefPtr<CSSValueList> values;
    RefPtr<CSSPrimitiveValue> x;
    RefPtr<CSSPrimitiveValue> y;
    RefPtr<CSSPrimitiveValue> blur;
    RefPtr<CSSPrimitiveValue> spread;
    RefPtr<CSSPrimitiveValue> style;
    RefPtr<CSSPrimitiveValue> color;

    bool allowX;
    bool allowY;
    bool allowBlur;
    bool allowSpread;
    bool allowColor;
    bool allowStyle; // inset or not.
    bool allowBreak;
};

bool CSSParser::parseShadow(int propId, bool important)
{
    ShadowParseContext context(static_cast<CSSPropertyID>(propId), primitiveValueCache());
    CSSParserValue* val;
    while ((val = m_valueList->current())) {
        // Check for a comma break first.
        if (val->unit == CSSParserValue::Operator) {
            if (val->iValue != ',' || !context.allowBreak)
                // Other operators aren't legal or we aren't done with the current shadow
                // value.  Treat as invalid.
                return false;
#if ENABLE(SVG)
            // -webkit-svg-shadow does not support multiple values.
            if (static_cast<CSSPropertyID>(propId) == CSSPropertyWebkitSvgShadow)
                return false;
#endif
            // The value is good.  Commit it.
            context.commitValue();
        } else if (validUnit(val, FLength, true)) {
            // We required a length and didn't get one. Invalid.
            if (!context.allowLength())
                return false;

            // A length is allowed here.  Construct the value and add it.
            context.commitLength(val);
        } else if (val->id == CSSValueInset) {
            if (!context.allowStyle)
                return false;

            context.commitStyle(val);
        } else {
            // The only other type of value that's ok is a color value.
            RefPtr<CSSPrimitiveValue> parsedColor;
            bool isColor = ((val->id >= CSSValueAqua && val->id <= CSSValueWindowtext) || val->id == CSSValueMenu ||
                            (val->id >= CSSValueWebkitFocusRingColor && val->id <= CSSValueWebkitText && !m_strict));
            if (isColor) {
                if (!context.allowColor)
                    return false;
                parsedColor = primitiveValueCache()->createIdentifierValue(val->id);
            }

            if (!parsedColor)
                // It's not built-in. Try to parse it as a color.
                parsedColor = parseColor(val);

            if (!parsedColor || !context.allowColor)
                return false; // This value is not a color or length and is invalid or
                              // it is a color, but a color isn't allowed at this point.

            context.commitColor(parsedColor.release());
        }

        m_valueList->next();
    }

    if (context.allowBreak) {
        context.commitValue();
        if (context.values->length()) {
            addProperty(propId, context.values.release(), important);
            m_valueList->next();
            return true;
        }
    }

    return false;
}

bool CSSParser::parseReflect(int propId, bool important)
{
    // box-reflect: <direction> <offset> <mask>

    // Direction comes first.
    CSSParserValue* val = m_valueList->current();
    CSSReflectionDirection direction;
    switch (val->id) {
        case CSSValueAbove:
            direction = ReflectionAbove;
            break;
        case CSSValueBelow:
            direction = ReflectionBelow;
            break;
        case CSSValueLeft:
            direction = ReflectionLeft;
            break;
        case CSSValueRight:
            direction = ReflectionRight;
            break;
        default:
            return false;
    }

    // The offset comes next.
    val = m_valueList->next();
    RefPtr<CSSPrimitiveValue> offset;
    if (!val)
        offset = primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_PX);
    else {
        if (!validUnit(val, FLength | FPercent, m_strict))
            return false;
        offset = primitiveValueCache()->createValue(val->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(val->unit));
    }

    // Now for the mask.
    RefPtr<CSSValue> mask;
    val = m_valueList->next();
    if (val) {
        if (!parseBorderImage(propId, important, mask))
            return false;
    }

    RefPtr<CSSReflectValue> reflectValue = CSSReflectValue::create(direction, offset.release(), mask.release());
    addProperty(propId, reflectValue.release(), important);
    m_valueList->next();
    return true;
}

struct BorderImageParseContext {
    BorderImageParseContext(CSSPrimitiveValueCache* primitiveValueCache)
    : m_primitiveValueCache(primitiveValueCache)
    , m_allowBreak(false)
    , m_allowNumber(false)
    , m_allowSlash(false)
    , m_allowWidth(false)
    , m_allowRule(false)
    , m_borderTop(0)
    , m_borderRight(0)
    , m_borderBottom(0)
    , m_borderLeft(0)
    , m_horizontalRule(0)
    , m_verticalRule(0)
    {}

    bool allowBreak() const { return m_allowBreak; }
    bool allowNumber() const { return m_allowNumber; }
    bool allowSlash() const { return m_allowSlash; }
    bool allowWidth() const { return m_allowWidth; }
    bool allowRule() const { return m_allowRule; }

    void commitImage(PassRefPtr<CSSValue> image) { m_image = image; m_allowNumber = true; }
    void commitNumber(CSSParserValue* v)
    {
        PassRefPtr<CSSPrimitiveValue> val = m_primitiveValueCache->createValue(v->fValue, (CSSPrimitiveValue::UnitTypes)v->unit);
        if (!m_top)
            m_top = val;
        else if (!m_right)
            m_right = val;
        else if (!m_bottom)
            m_bottom = val;
        else {
            ASSERT(!m_left);
            m_left = val;
        }

        m_allowBreak = m_allowSlash = m_allowRule = true;
        m_allowNumber = !m_left;
    }
    void commitSlash() { m_allowBreak = m_allowSlash = m_allowNumber = false; m_allowWidth = true; }
    void commitWidth(CSSParserValue* val)
    {
        if (!m_borderTop)
            m_borderTop = val;
        else if (!m_borderRight)
            m_borderRight = val;
        else if (!m_borderBottom)
            m_borderBottom = val;
        else {
            ASSERT(!m_borderLeft);
            m_borderLeft = val;
        }

        m_allowBreak = m_allowRule = true;
        m_allowWidth = !m_borderLeft;
    }
    void commitRule(int keyword)
    {
        if (!m_horizontalRule)
            m_horizontalRule = keyword;
        else if (!m_verticalRule)
            m_verticalRule = keyword;
        m_allowRule = !m_verticalRule;
    }
    PassRefPtr<CSSValue> commitBorderImage(CSSParser* p, bool important)
    {
        // We need to clone and repeat values for any omissions.
        if (!m_right) {
            m_right = m_primitiveValueCache->createValue(m_top->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_top->primitiveType());
            m_bottom = m_primitiveValueCache->createValue(m_top->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_top->primitiveType());
            m_left = m_primitiveValueCache->createValue(m_top->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_top->primitiveType());
        }
        if (!m_bottom) {
            m_bottom = m_primitiveValueCache->createValue(m_top->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_top->primitiveType());
            m_left = m_primitiveValueCache->createValue(m_right->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_right->primitiveType());
        }
        if (!m_left)
             m_left = m_primitiveValueCache->createValue(m_right->getDoubleValue(), (CSSPrimitiveValue::UnitTypes)m_right->primitiveType());

        // Now build a rect value to hold all four of our primitive values.
        RefPtr<Rect> rect = Rect::create();
        rect->setTop(m_top);
        rect->setRight(m_right);
        rect->setBottom(m_bottom);
        rect->setLeft(m_left);

        // Fill in STRETCH as the default if it wasn't specified.
        if (!m_horizontalRule)
            m_horizontalRule = CSSValueStretch;

        // The vertical rule should match the horizontal rule if unspecified.
        if (!m_verticalRule)
            m_verticalRule = m_horizontalRule;

        // Now we have to deal with the border widths.  The best way to deal with these is to actually put these values into a value
        // list and then make our parsing machinery do the parsing.
        if (m_borderTop) {
            CSSParserValueList newList;
            newList.addValue(*m_borderTop);
            if (m_borderRight)
                newList.addValue(*m_borderRight);
            if (m_borderBottom)
                newList.addValue(*m_borderBottom);
            if (m_borderLeft)
                newList.addValue(*m_borderLeft);
            CSSParserValueList* oldList = p->m_valueList;
            p->m_valueList = &newList;
            p->parseValue(CSSPropertyBorderWidth, important);
            p->m_valueList = oldList;
        }

        // Make our new border image value now.
        return CSSBorderImageValue::create(m_image, rect.release(), m_horizontalRule, m_verticalRule);
    }
    
    CSSPrimitiveValueCache* m_primitiveValueCache;

    bool m_allowBreak;
    bool m_allowNumber;
    bool m_allowSlash;
    bool m_allowWidth;
    bool m_allowRule;

    RefPtr<CSSValue> m_image;

    RefPtr<CSSPrimitiveValue> m_top;
    RefPtr<CSSPrimitiveValue> m_right;
    RefPtr<CSSPrimitiveValue> m_bottom;
    RefPtr<CSSPrimitiveValue> m_left;

    CSSParserValue* m_borderTop;
    CSSParserValue* m_borderRight;
    CSSParserValue* m_borderBottom;
    CSSParserValue* m_borderLeft;

    int m_horizontalRule;
    int m_verticalRule;
};

bool CSSParser::parseBorderImage(int propId, bool important, RefPtr<CSSValue>& result)
{
    // Look for an image initially.  If the first value is not a URI, then we're done.
    BorderImageParseContext context(primitiveValueCache());
    CSSParserValue* val = m_valueList->current();
    if (val->unit == CSSPrimitiveValue::CSS_URI && m_styleSheet) {
        // FIXME: The completeURL call should be done when using the CSSImageValue,
        // not when creating it.
        context.commitImage(CSSImageValue::create(m_styleSheet->completeURL(val->string)));
    } else if (isGeneratedImageValue(val)) {
        RefPtr<CSSValue> value;
        if (parseGeneratedImage(value))
            context.commitImage(value);
        else
            return false;
    } else
        return false;

    while ((val = m_valueList->next())) {
        if (context.allowNumber() && validUnit(val, FInteger | FNonNeg | FPercent, true)) {
            context.commitNumber(val);
        } else if (propId == CSSPropertyWebkitBorderImage && context.allowSlash() && val->unit == CSSParserValue::Operator && val->iValue == '/') {
            context.commitSlash();
        } else if (context.allowWidth() &&
            (val->id == CSSValueThin || val->id == CSSValueMedium || val->id == CSSValueThick || validUnit(val, FLength, m_strict))) {
            context.commitWidth(val);
        } else if (context.allowRule() &&
            (val->id == CSSValueStretch || val->id == CSSValueRound || val->id == CSSValueRepeat)) {
            context.commitRule(val->id);
        } else {
            // Something invalid was encountered.
            return false;
        }
    }

    if (context.allowNumber() && propId != CSSPropertyWebkitBorderImage) {
        // Allow the slices to be omitted for images that don't fit to a border.  We just set the slices to be 0.
        context.m_top = primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_NUMBER);
        context.m_allowBreak = true;
    }

    if (context.allowBreak()) {
        // Need to fully commit as a single value.
        result = context.commitBorderImage(this, important);
        return true;
    }

    return false;
}

static void completeBorderRadii(RefPtr<CSSPrimitiveValue> radii[4])
{
    if (radii[3])
        return;
    if (!radii[2]) {
        if (!radii[1])
            radii[1] = radii[0];
        radii[2] = radii[0];
    }
    radii[3] = radii[1];
}

bool CSSParser::parseBorderRadius(int propId, bool important)
{
    unsigned num = m_valueList->size();
    if (num > 9)
        return false;

    ShorthandScope scope(this, propId);
    RefPtr<CSSPrimitiveValue> radii[2][4];

    unsigned indexAfterSlash = 0;
    for (unsigned i = 0; i < num; ++i) {
        CSSParserValue* value = m_valueList->valueAt(i);
        if (value->unit == CSSParserValue::Operator) {
            if (value->iValue != '/')
                return false;

            if (!i || indexAfterSlash || i + 1 == num || num > i + 5)
                return false;

            indexAfterSlash = i + 1;
            completeBorderRadii(radii[0]);
            continue;
        }

        if (i - indexAfterSlash >= 4)
            return false;

        if (!validUnit(value, FLength | FPercent, m_strict))
            return false;

        RefPtr<CSSPrimitiveValue> radius = primitiveValueCache()->createValue(value->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(value->unit));

        if (!indexAfterSlash) {
            radii[0][i] = radius;

            // Legacy syntax: -webkit-border-radius: l1 l2; is equivalent to border-radius: l1 / l2;
            if (num == 2 && propId == CSSPropertyWebkitBorderRadius) {
                indexAfterSlash = 1;
                completeBorderRadii(radii[0]);
            }
        } else
            radii[1][i - indexAfterSlash] = radius.release();
    }

    if (!indexAfterSlash) {
        completeBorderRadii(radii[0]);
        for (unsigned i = 0; i < 4; ++i)
            radii[1][i] = radii[0][i];
    } else
        completeBorderRadii(radii[1]);

    m_implicitShorthand = true;
    addProperty(CSSPropertyBorderTopLeftRadius, primitiveValueCache()->createValue(Pair::create(radii[0][0].release(), radii[1][0].release())), important);
    addProperty(CSSPropertyBorderTopRightRadius, primitiveValueCache()->createValue(Pair::create(radii[0][1].release(), radii[1][1].release())), important);
    addProperty(CSSPropertyBorderBottomRightRadius, primitiveValueCache()->createValue(Pair::create(radii[0][2].release(), radii[1][2].release())), important);
    addProperty(CSSPropertyBorderBottomLeftRadius, primitiveValueCache()->createValue(Pair::create(radii[0][3].release(), radii[1][3].release())), important);
    m_implicitShorthand = false;
    return true;
}

bool CSSParser::parseCounter(int propId, int defaultValue, bool important)
{
    enum { ID, VAL } state = ID;

    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    RefPtr<CSSPrimitiveValue> counterName;

    while (true) {
        CSSParserValue* val = m_valueList->current();
        switch (state) {
            case ID:
                if (val && val->unit == CSSPrimitiveValue::CSS_IDENT) {
                    counterName = primitiveValueCache()->createValue(val->string, CSSPrimitiveValue::CSS_STRING);
                    state = VAL;
                    m_valueList->next();
                    continue;
                }
                break;
            case VAL: {
                int i = defaultValue;
                if (val && val->unit == CSSPrimitiveValue::CSS_NUMBER) {
                    i = clampToInteger(val->fValue);
                    m_valueList->next();
                }

                list->append(primitiveValueCache()->createValue(Pair::create(counterName.release(),
                    primitiveValueCache()->createValue(i, CSSPrimitiveValue::CSS_NUMBER))));
                state = ID;
                continue;
            }
        }
        break;
    }

    if (list->length() > 0) {
        addProperty(propId, list.release(), important);
        return true;
    }

    return false;
}

// This should go away once we drop support for -webkit-gradient
static PassRefPtr<CSSPrimitiveValue> parseDeprecatedGradientPoint(CSSParserValue* a, bool horizontal, CSSPrimitiveValueCache* primitiveValueCache)
{
    RefPtr<CSSPrimitiveValue> result;
    if (a->unit == CSSPrimitiveValue::CSS_IDENT) {
        if ((equalIgnoringCase(a->string, "left") && horizontal) || 
            (equalIgnoringCase(a->string, "top") && !horizontal))
            result = primitiveValueCache->createValue(0., CSSPrimitiveValue::CSS_PERCENTAGE);
        else if ((equalIgnoringCase(a->string, "right") && horizontal) ||
                 (equalIgnoringCase(a->string, "bottom") && !horizontal))
            result = primitiveValueCache->createValue(100., CSSPrimitiveValue::CSS_PERCENTAGE);
        else if (equalIgnoringCase(a->string, "center"))
            result = primitiveValueCache->createValue(50., CSSPrimitiveValue::CSS_PERCENTAGE);
    } else if (a->unit == CSSPrimitiveValue::CSS_NUMBER || a->unit == CSSPrimitiveValue::CSS_PERCENTAGE)
        result = primitiveValueCache->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes)a->unit);
    return result;
}

static bool parseDeprecatedGradientColorStop(CSSParser* p, CSSParserValue* a, CSSGradientColorStop& stop)
{
    if (a->unit != CSSParserValue::Function)
        return false;

    if (!equalIgnoringCase(a->function->name, "from(") &&
        !equalIgnoringCase(a->function->name, "to(") &&
        !equalIgnoringCase(a->function->name, "color-stop("))
        return false;

    CSSParserValueList* args = a->function->args.get();
    if (!args)
        return false;

    if (equalIgnoringCase(a->function->name, "from(") || 
        equalIgnoringCase(a->function->name, "to(")) {
        // The "from" and "to" stops expect 1 argument.
        if (args->size() != 1)
            return false;

        if (equalIgnoringCase(a->function->name, "from("))
            stop.m_position = p->primitiveValueCache()->createValue(0, CSSPrimitiveValue::CSS_NUMBER);
        else
            stop.m_position = p->primitiveValueCache()->createValue(1, CSSPrimitiveValue::CSS_NUMBER);

        int id = args->current()->id;
        if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu)
            stop.m_color = p->primitiveValueCache()->createIdentifierValue(id);
        else
            stop.m_color = p->parseColor(args->current());
        if (!stop.m_color)
            return false;
    }

    // The "color-stop" function expects 3 arguments.
    if (equalIgnoringCase(a->function->name, "color-stop(")) {
        if (args->size() != 3)
            return false;

        CSSParserValue* stopArg = args->current();
        if (stopArg->unit == CSSPrimitiveValue::CSS_PERCENTAGE)
            stop.m_position = p->primitiveValueCache()->createValue(stopArg->fValue / 100, CSSPrimitiveValue::CSS_NUMBER);
        else if (stopArg->unit == CSSPrimitiveValue::CSS_NUMBER)
            stop.m_position = p->primitiveValueCache()->createValue(stopArg->fValue, CSSPrimitiveValue::CSS_NUMBER);
        else
            return false;

        stopArg = args->next();
        if (stopArg->unit != CSSParserValue::Operator || stopArg->iValue != ',')
            return false;

        stopArg = args->next();
        int id = stopArg->id;
        if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu)
            stop.m_color = p->primitiveValueCache()->createIdentifierValue(id);
        else
            stop.m_color = p->parseColor(stopArg);
        if (!stop.m_color)
            return false;
    }

    return true;
}

bool CSSParser::parseDeprecatedGradient(RefPtr<CSSValue>& gradient)
{
    // Walk the arguments.
    CSSParserValueList* args = m_valueList->current()->function->args.get();
    if (!args || args->size() == 0)
        return false;

    // The first argument is the gradient type.  It is an identifier.
    CSSGradientType gradientType;
    CSSParserValue* a = args->current();
    if (!a || a->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;
    if (equalIgnoringCase(a->string, "linear"))
        gradientType = CSSLinearGradient;
    else if (equalIgnoringCase(a->string, "radial"))
        gradientType = CSSRadialGradient;
    else
        return false;

    RefPtr<CSSGradientValue> result;
    switch (gradientType) {
        case CSSLinearGradient:
            result = CSSLinearGradientValue::create(NonRepeating, true);
            break;
        case CSSRadialGradient:
            result = CSSRadialGradientValue::create(NonRepeating, true);
            break;
    }

    // Comma.
    a = args->next();
    if (!a || a->unit != CSSParserValue::Operator || a->iValue != ',')
        return false;

    // Next comes the starting point for the gradient as an x y pair.  There is no
    // comma between the x and the y values.
    // First X.  It can be left, right, number or percent.
    a = args->next();
    if (!a)
        return false;
    RefPtr<CSSPrimitiveValue> point = parseDeprecatedGradientPoint(a, true, primitiveValueCache());
    if (!point)
        return false;
    result->setFirstX(point.release());

    // First Y.  It can be top, bottom, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, false, primitiveValueCache());
    if (!point)
        return false;
    result->setFirstY(point.release());

    // Comma after the first point.
    a = args->next();
    if (!a || a->unit != CSSParserValue::Operator || a->iValue != ',')
        return false;

    // For radial gradients only, we now expect a numeric radius.
    if (gradientType == CSSRadialGradient) {
        a = args->next();
        if (!a || a->unit != CSSPrimitiveValue::CSS_NUMBER)
            return false;
        static_cast<CSSRadialGradientValue*>(result.get())->setFirstRadius(primitiveValueCache()->createValue(a->fValue, CSSPrimitiveValue::CSS_NUMBER));

        // Comma after the first radius.
        a = args->next();
        if (!a || a->unit != CSSParserValue::Operator || a->iValue != ',')
            return false;
    }

    // Next is the ending point for the gradient as an x, y pair.
    // Second X.  It can be left, right, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, true, primitiveValueCache());
    if (!point)
        return false;
    result->setSecondX(point.release());

    // Second Y.  It can be top, bottom, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, false, primitiveValueCache());
    if (!point)
        return false;
    result->setSecondY(point.release());

    // For radial gradients only, we now expect the second radius.
    if (gradientType == CSSRadialGradient) {
        // Comma after the second point.
        a = args->next();
        if (!a || a->unit != CSSParserValue::Operator || a->iValue != ',')
            return false;

        a = args->next();
        if (!a || a->unit != CSSPrimitiveValue::CSS_NUMBER)
            return false;
        static_cast<CSSRadialGradientValue*>(result.get())->setSecondRadius(primitiveValueCache()->createValue(a->fValue, CSSPrimitiveValue::CSS_NUMBER));
    }

    // We now will accept any number of stops (0 or more).
    a = args->next();
    while (a) {
        // Look for the comma before the next stop.
        if (a->unit != CSSParserValue::Operator || a->iValue != ',')
            return false;

        // Now examine the stop itself.
        a = args->next();
        if (!a)
            return false;

        // The function name needs to be one of "from", "to", or "color-stop."
        CSSGradientColorStop stop;
        if (!parseDeprecatedGradientColorStop(this, a, stop))
            return false;
        result->addStop(stop);

        // Advance
        a = args->next();
    }

    gradient = result.release();
    return true;
}

static PassRefPtr<CSSPrimitiveValue> valueFromSideKeyword(CSSParserValue* a, bool& isHorizontal, CSSPrimitiveValueCache* primitiveValueCache)
{
    if (a->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;

    switch (a->id) {
        case CSSValueLeft:
        case CSSValueRight:
            isHorizontal = true;
            break;
        case CSSValueTop:
        case CSSValueBottom:
            isHorizontal = false;
            break;
        default:
            return 0;
    }
    return primitiveValueCache->createIdentifierValue(a->id);
}

static PassRefPtr<CSSPrimitiveValue> parseGradientColorOrKeyword(CSSParser* p, CSSParserValue* value)
{
    int id = value->id;
    if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu)
        return p->primitiveValueCache()->createIdentifierValue(id);

    return p->parseColor(value);
}

bool CSSParser::parseLinearGradient(RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSLinearGradientValue> result = CSSLinearGradientValue::create(repeating);
    
    // Walk the arguments.
    CSSParserValueList* args = m_valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;
    // Look for angle.
    if (validUnit(a, FAngle, true)) {
        result->setAngle(primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes)a->unit));
        
        a = args->next();
        expectComma = true;
    } else {
        // Look one or two optional keywords that indicate a side or corner.
        RefPtr<CSSPrimitiveValue> startX, startY;
        
        RefPtr<CSSPrimitiveValue> location;
        bool isHorizontal = false;
        if ((location = valueFromSideKeyword(a, isHorizontal, primitiveValueCache()))) {
            if (isHorizontal)
                startX = location;
            else
                startY = location;
            
            a = args->next();
            if (a) {
                if ((location = valueFromSideKeyword(a, isHorizontal, primitiveValueCache()))) {
                    if (isHorizontal) {
                        if (startX)
                            return false;
                        startX = location;
                    } else {
                        if (startY)
                            return false;
                        startY = location;
                    }

                    a = args->next();
                }
            }

            expectComma = true;
        }
        
        if (!startX && !startY)
            startY = primitiveValueCache()->createIdentifierValue(CSSValueTop);
            
        result->setFirstX(startX.release());
        result->setFirstY(startY.release());
    }

    if (!parseGradientColorStops(args, result.get(), expectComma))
        return false;

    Vector<CSSGradientColorStop>& stops = result->stops();
    if (stops.isEmpty())
        return false;

    gradient = result.release();
    return true;
}

bool CSSParser::parseRadialGradient(RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSRadialGradientValue> result = CSSRadialGradientValue::create(repeating);
    
    // Walk the arguments.
    CSSParserValueList* args = m_valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;

    // Optional background-position
    RefPtr<CSSValue> centerX;
    RefPtr<CSSValue> centerY;
    // parseFillPosition advances the args next pointer.
    parseFillPosition(args, centerX, centerY);
    a = args->current();
    
    if (centerX || centerY) {
        // Comma
        if (a->unit != CSSParserValue::Operator || a->iValue != ',')
            return false;

        a = args->next();
        if (!a)
            return false;
    }
    
    ASSERT(!centerX || centerX->isPrimitiveValue());
    ASSERT(!centerY || centerY->isPrimitiveValue());

    result->setFirstX(static_cast<CSSPrimitiveValue*>(centerX.get()));
    result->setSecondX(static_cast<CSSPrimitiveValue*>(centerX.get()));
    // CSS3 radial gradients always share the same start and end point.
    result->setFirstY(static_cast<CSSPrimitiveValue*>(centerY.get()));
    result->setSecondY(static_cast<CSSPrimitiveValue*>(centerY.get()));
    
    RefPtr<CSSPrimitiveValue> shapeValue;
    RefPtr<CSSPrimitiveValue> sizeValue;

    // Optional shape and/or size in any order.
    for (int i = 0; i < 2; ++i) {
        if (a->unit != CSSPrimitiveValue::CSS_IDENT)
            break;
    
        bool foundValue = false;
        switch (a->id) {
        case CSSValueCircle:
        case CSSValueEllipse:
            shapeValue = primitiveValueCache()->createIdentifierValue(a->id);
            foundValue = true;
            break;
        case CSSValueClosestSide:
        case CSSValueClosestCorner:
        case CSSValueFarthestSide:
        case CSSValueFarthestCorner:
        case CSSValueContain:
        case CSSValueCover:
            sizeValue = primitiveValueCache()->createIdentifierValue(a->id);
            foundValue = true;
            break;
        }

        if (foundValue) {
            a = args->next();
            if (!a)
                return false;

            expectComma = true;
        }
    }
    
    result->setShape(shapeValue);
    result->setSizingBehavior(sizeValue);
    
    // Or, two lengths or percentages
    RefPtr<CSSPrimitiveValue> horizontalSize;
    RefPtr<CSSPrimitiveValue> verticalSize;

    if (!shapeValue && !sizeValue) {
        if (validUnit(a, FLength | FPercent, m_strict)) {
            horizontalSize = primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes) a->unit);
            a = args->next();
            if (!a)
                return false;

            expectComma = true;
        }

        if (validUnit(a, FLength | FPercent, m_strict)) {
            verticalSize = primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes) a->unit);

            a = args->next();
            if (!a)
                return false;
            expectComma = true;
        }
    }

    // Must have neither or both.
    if (!horizontalSize != !verticalSize)
        return false;
    
    result->setEndHorizontalSize(horizontalSize);
    result->setEndVerticalSize(verticalSize);

    if (!parseGradientColorStops(args, result.get(), expectComma))
        return false;

    gradient = result.release();
    return true;
}

bool CSSParser::parseGradientColorStops(CSSParserValueList* valueList, CSSGradientValue* gradient, bool expectComma)
{
    CSSParserValue* a = valueList->current();

    // Now look for color stops.
    while (a) {
        // Look for the comma before the next stop.
        if (expectComma) {
            if (a->unit != CSSParserValue::Operator || a->iValue != ',')
                return false;

            a = valueList->next();
            if (!a)
                return false;
        }

        // <color-stop> = <color> [ <percentage> | <length> ]?
        CSSGradientColorStop stop;
        stop.m_color = parseGradientColorOrKeyword(this, a);
        if (!stop.m_color)
            return false;

        a = valueList->next();
        if (a) {
            if (validUnit(a, FLength | FPercent, m_strict)) {
                stop.m_position = primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes)a->unit);
                a = valueList->next();
            }
        }
        
        gradient->addStop(stop);
        expectComma = true;
    }

    // Must have 2 or more stops to be valid.
    return gradient->stops().size() > 1;
}

bool CSSParser::isGeneratedImageValue(CSSParserValue* val) const
{
    if (val->unit != CSSParserValue::Function)
        return false;

    return equalIgnoringCase(val->function->name, "-webkit-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-linear-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-repeating-linear-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-radial-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-repeating-radial-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-canvas(");
}

bool CSSParser::parseGeneratedImage(RefPtr<CSSValue>& value)
{
    CSSParserValue* val = m_valueList->current();

    if (val->unit != CSSParserValue::Function)
        return false;

    if (equalIgnoringCase(val->function->name, "-webkit-gradient("))
        return parseDeprecatedGradient(value);

    if (equalIgnoringCase(val->function->name, "-webkit-linear-gradient("))
        return parseLinearGradient(value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "-webkit-repeating-linear-gradient("))
        return parseLinearGradient(value, Repeating);

    if (equalIgnoringCase(val->function->name, "-webkit-radial-gradient("))
        return parseRadialGradient(value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "-webkit-repeating-radial-gradient("))
        return parseRadialGradient(value, Repeating);

    if (equalIgnoringCase(val->function->name, "-webkit-canvas("))
        return parseCanvas(value);

    return false;
}

bool CSSParser::parseCanvas(RefPtr<CSSValue>& canvas)
{
    RefPtr<CSSCanvasValue> result = CSSCanvasValue::create();

    // Walk the arguments.
    CSSParserValueList* args = m_valueList->current()->function->args.get();
    if (!args || args->size() != 1)
        return false;

    // The first argument is the canvas name.  It is an identifier.
    CSSParserValue* a = args->current();
    if (!a || a->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;
    result->setName(a->string);
    canvas = result;
    return true;
}

class TransformOperationInfo {
public:
    TransformOperationInfo(const CSSParserString& name)
    : m_type(WebKitCSSTransformValue::UnknownTransformOperation)
    , m_argCount(1)
    , m_allowSingleArgument(false)
    , m_unit(CSSParser::FUnknown)
    {
        if (equalIgnoringCase(name, "scale(") || equalIgnoringCase(name, "scalex(") || equalIgnoringCase(name, "scaley(") || equalIgnoringCase(name, "scalez(")) {
            m_unit = CSSParser::FNumber;
            if (equalIgnoringCase(name, "scale("))
                m_type = WebKitCSSTransformValue::ScaleTransformOperation;
            else if (equalIgnoringCase(name, "scalex("))
                m_type = WebKitCSSTransformValue::ScaleXTransformOperation;
            else if (equalIgnoringCase(name, "scaley("))
                m_type = WebKitCSSTransformValue::ScaleYTransformOperation;
            else
                m_type = WebKitCSSTransformValue::ScaleZTransformOperation;
        } else if (equalIgnoringCase(name, "scale3d(")) {
            m_type = WebKitCSSTransformValue::Scale3DTransformOperation;
            m_argCount = 5;
            m_unit = CSSParser::FNumber;
        } else if (equalIgnoringCase(name, "rotate(")) {
            m_type = WebKitCSSTransformValue::RotateTransformOperation;
            m_unit = CSSParser::FAngle;
        } else if (equalIgnoringCase(name, "rotatex(") ||
                   equalIgnoringCase(name, "rotatey(") ||
                   equalIgnoringCase(name, "rotatez(")) {
            m_unit = CSSParser::FAngle;
            if (equalIgnoringCase(name, "rotatex("))
                m_type = WebKitCSSTransformValue::RotateXTransformOperation;
            else if (equalIgnoringCase(name, "rotatey("))
                m_type = WebKitCSSTransformValue::RotateYTransformOperation;
            else
                m_type = WebKitCSSTransformValue::RotateZTransformOperation;
        } else if (equalIgnoringCase(name, "rotate3d(")) {
            m_type = WebKitCSSTransformValue::Rotate3DTransformOperation;
            m_argCount = 7;
            m_unit = CSSParser::FNumber;
        } else if (equalIgnoringCase(name, "skew(") || equalIgnoringCase(name, "skewx(") || equalIgnoringCase(name, "skewy(")) {
            m_unit = CSSParser::FAngle;
            if (equalIgnoringCase(name, "skew("))
                m_type = WebKitCSSTransformValue::SkewTransformOperation;
            else if (equalIgnoringCase(name, "skewx("))
                m_type = WebKitCSSTransformValue::SkewXTransformOperation;
            else
                m_type = WebKitCSSTransformValue::SkewYTransformOperation;
        } else if (equalIgnoringCase(name, "translate(") || equalIgnoringCase(name, "translatex(") || equalIgnoringCase(name, "translatey(") || equalIgnoringCase(name, "translatez(")) {
            m_unit = CSSParser::FLength | CSSParser::FPercent;
            if (equalIgnoringCase(name, "translate("))
                m_type = WebKitCSSTransformValue::TranslateTransformOperation;
            else if (equalIgnoringCase(name, "translatex("))
                m_type = WebKitCSSTransformValue::TranslateXTransformOperation;
            else if (equalIgnoringCase(name, "translatey("))
                m_type = WebKitCSSTransformValue::TranslateYTransformOperation;
            else
                m_type = WebKitCSSTransformValue::TranslateZTransformOperation;
        } else if (equalIgnoringCase(name, "translate3d(")) {
            m_type = WebKitCSSTransformValue::Translate3DTransformOperation;
            m_argCount = 5;
            m_unit = CSSParser::FLength | CSSParser::FPercent;
        } else if (equalIgnoringCase(name, "matrix(")) {
            m_type = WebKitCSSTransformValue::MatrixTransformOperation;
            m_argCount = 11;
            m_unit = CSSParser::FNumber;
        } else if (equalIgnoringCase(name, "matrix3d(")) {
            m_type = WebKitCSSTransformValue::Matrix3DTransformOperation;
            m_argCount = 31;
            m_unit = CSSParser::FNumber;
        } else if (equalIgnoringCase(name, "perspective(")) {
            m_type = WebKitCSSTransformValue::PerspectiveTransformOperation;
            m_unit = CSSParser::FNumber;
        }

        if (equalIgnoringCase(name, "scale(") || equalIgnoringCase(name, "skew(") || equalIgnoringCase(name, "translate(")) {
            m_allowSingleArgument = true;
            m_argCount = 3;
        }
    }

    WebKitCSSTransformValue::TransformOperationType type() const { return m_type; }
    unsigned argCount() const { return m_argCount; }
    CSSParser::Units unit() const { return m_unit; }

    bool unknown() const { return m_type == WebKitCSSTransformValue::UnknownTransformOperation; }
    bool hasCorrectArgCount(unsigned argCount) { return m_argCount == argCount || (m_allowSingleArgument && argCount == 1); }

private:
    WebKitCSSTransformValue::TransformOperationType m_type;
    unsigned m_argCount;
    bool m_allowSingleArgument;
    CSSParser::Units m_unit;
};

PassRefPtr<CSSValueList> CSSParser::parseTransform()
{
    if (!m_valueList)
        return 0;

    // The transform is a list of functional primitives that specify transform operations.
    // We collect a list of WebKitCSSTransformValues, where each value specifies a single operation.
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->unit != CSSParserValue::Function || !value->function)
            return 0;

        // Every primitive requires at least one argument.
        CSSParserValueList* args = value->function->args.get();
        if (!args)
            return 0;

        // See if the specified primitive is one we understand.
        TransformOperationInfo info(value->function->name);
        if (info.unknown())
            return 0;

        if (!info.hasCorrectArgCount(args->size()))
            return 0;

        // Create the new WebKitCSSTransformValue for this operation and add it to our list.
        RefPtr<WebKitCSSTransformValue> transformValue = WebKitCSSTransformValue::create(info.type());
        list->append(transformValue);

        // Snag our values.
        CSSParserValue* a = args->current();
        unsigned argNumber = 0;
        while (a) {
            CSSParser::Units unit = info.unit();

            if (info.type() == WebKitCSSTransformValue::Rotate3DTransformOperation && argNumber == 3) {
                // 4th param of rotate3d() is an angle rather than a bare number, validate it as such
                if (!validUnit(a, FAngle, true))
                    return 0;
            } else if (info.type() == WebKitCSSTransformValue::Translate3DTransformOperation && argNumber == 2) {
                // 3rd param of translate3d() cannot be a percentage
                if (!validUnit(a, FLength, true))
                    return 0;
            } else if (info.type() == WebKitCSSTransformValue::TranslateZTransformOperation && argNumber == 0) {
                // 1st param of translateZ() cannot be a percentage
                if (!validUnit(a, FLength, true))
                    return 0;
            } else if (info.type() == WebKitCSSTransformValue::PerspectiveTransformOperation && argNumber == 0) {
                // 1st param of perspective() must be a non-negative number (deprecated) or length.
                if (!validUnit(a, FNumber | FLength | FNonNeg, true))
                    return 0;
            } else if (!validUnit(a, unit, true))
                return 0;

            // Add the value to the current transform operation.
            transformValue->append(primitiveValueCache()->createValue(a->fValue, (CSSPrimitiveValue::UnitTypes) a->unit));

            a = args->next();
            if (!a)
                break;
            if (a->unit != CSSParserValue::Operator || a->iValue != ',')
                return 0;
            a = args->next();

            argNumber++;
        }
    }

    return list.release();
}

bool CSSParser::parseTransformOrigin(int propId, int& propId1, int& propId2, int& propId3, RefPtr<CSSValue>& value, RefPtr<CSSValue>& value2, RefPtr<CSSValue>& value3)
{
    propId1 = propId;
    propId2 = propId;
    propId3 = propId;
    if (propId == CSSPropertyWebkitTransformOrigin) {
        propId1 = CSSPropertyWebkitTransformOriginX;
        propId2 = CSSPropertyWebkitTransformOriginY;
        propId3 = CSSPropertyWebkitTransformOriginZ;
    }

    switch (propId) {
        case CSSPropertyWebkitTransformOrigin:
            if (!parseTransformOriginShorthand(value, value2, value3))
                return false;
            // parseTransformOriginShorthand advances the m_valueList pointer
            break;
        case CSSPropertyWebkitTransformOriginX: {
            value = parseFillPositionX(m_valueList);
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitTransformOriginY: {
            value = parseFillPositionY(m_valueList);
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitTransformOriginZ: {
            if (validUnit(m_valueList->current(), FLength, m_strict))
                value = primitiveValueCache()->createValue(m_valueList->current()->fValue, (CSSPrimitiveValue::UnitTypes)m_valueList->current()->unit);
            if (value)
                m_valueList->next();
            break;
        }
    }

    return value;
}

bool CSSParser::parsePerspectiveOrigin(int propId, int& propId1, int& propId2, RefPtr<CSSValue>& value, RefPtr<CSSValue>& value2)
{
    propId1 = propId;
    propId2 = propId;
    if (propId == CSSPropertyWebkitPerspectiveOrigin) {
        propId1 = CSSPropertyWebkitPerspectiveOriginX;
        propId2 = CSSPropertyWebkitPerspectiveOriginY;
    }

    switch (propId) {
        case CSSPropertyWebkitPerspectiveOrigin:
            parseFillPosition(m_valueList, value, value2);
            break;
        case CSSPropertyWebkitPerspectiveOriginX: {
            value = parseFillPositionX(m_valueList);
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitPerspectiveOriginY: {
            value = parseFillPositionY(m_valueList);
            if (value)
                m_valueList->next();
            break;
        }
    }

    return value;
}

bool CSSParser::parseTextEmphasisStyle(bool important)
{
    unsigned valueListSize = m_valueList->size();

    RefPtr<CSSPrimitiveValue> fill;
    RefPtr<CSSPrimitiveValue> shape;

    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->unit == CSSPrimitiveValue::CSS_STRING) {
            if (fill || shape || (valueListSize != 1 && !inShorthand()))
                return false;
            addProperty(CSSPropertyWebkitTextEmphasisStyle, primitiveValueCache()->createValue(value->string, CSSPrimitiveValue::CSS_STRING), important);
            m_valueList->next();
            return true;
        }

        if (value->id == CSSValueNone) {
            if (fill || shape || (valueListSize != 1 && !inShorthand()))
                return false;
            addProperty(CSSPropertyWebkitTextEmphasisStyle, primitiveValueCache()->createIdentifierValue(CSSValueNone), important);
            m_valueList->next();
            return true;
        }

        if (value->id == CSSValueOpen || value->id == CSSValueFilled) {
            if (fill)
                return false;
            fill = primitiveValueCache()->createIdentifierValue(value->id);
        } else if (value->id == CSSValueDot || value->id == CSSValueCircle || value->id == CSSValueDoubleCircle || value->id == CSSValueTriangle || value->id == CSSValueSesame) {
            if (shape)
                return false;
            shape = primitiveValueCache()->createIdentifierValue(value->id);
        } else if (!inShorthand())
            return false;
        else
            break;
    }

    if (fill && shape) {
        RefPtr<CSSValueList> parsedValues = CSSValueList::createSpaceSeparated();
        parsedValues->append(fill.release());
        parsedValues->append(shape.release());
        addProperty(CSSPropertyWebkitTextEmphasisStyle, parsedValues.release(), important);
        return true;
    }
    if (fill) {
        addProperty(CSSPropertyWebkitTextEmphasisStyle, fill.release(), important);
        return true;
    }
    if (shape) {
        addProperty(CSSPropertyWebkitTextEmphasisStyle, shape.release(), important);
        return true;
    }

    return false;
}

bool CSSParser::parseLineBoxContain(bool important)
{
    LineBoxContain lineBoxContain = LineBoxContainNone;

    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->id == CSSValueBlock) {
            if (lineBoxContain & LineBoxContainBlock)
                return false;
            lineBoxContain |= LineBoxContainBlock;
        } else if (value->id == CSSValueInline) {
            if (lineBoxContain & LineBoxContainInline)
                return false;
            lineBoxContain |= LineBoxContainInline;
        } else if (value->id == CSSValueFont) {
            if (lineBoxContain & LineBoxContainFont)
                return false;
            lineBoxContain |= LineBoxContainFont;
        } else if (value->id == CSSValueGlyphs) {
            if (lineBoxContain & LineBoxContainGlyphs)
                return false;
            lineBoxContain |= LineBoxContainGlyphs;
        } else if (value->id == CSSValueReplaced) {
            if (lineBoxContain & LineBoxContainReplaced)
                return false;
            lineBoxContain |= LineBoxContainReplaced;
        } else if (value->id == CSSValueInlineBox) {
            if (lineBoxContain & LineBoxContainInlineBox)
                return false;
            lineBoxContain |= LineBoxContainInlineBox;
        } else
            return false;
    }
    
    if (!lineBoxContain)
        return false;

    addProperty(CSSPropertyWebkitLineBoxContain, CSSLineBoxContainValue::create(lineBoxContain), important);
    return true;
}

static inline int yyerror(const char*) { return 1; }

#define END_TOKEN 0

#include "CSSGrammar.h"

int CSSParser::lex(void* yylvalWithoutType)
{
    YYSTYPE* yylval = static_cast<YYSTYPE*>(yylvalWithoutType);
    int length;

    lex();

    UChar* t = text(&length);

    switch (token()) {
    case WHITESPACE:
    case SGML_CD:
    case INCLUDES:
    case DASHMATCH:
        break;

    case URI:
    case STRING:
    case IDENT:
    case NTH:
    case HEX:
    case IDSEL:
    case DIMEN:
    case UNICODERANGE:
    case FUNCTION:
    case ANYFUNCTION:
    case NOTFUNCTION:
    case CALCFUNCTION:
    case MINFUNCTION:
    case MAXFUNCTION:
        yylval->string.characters = t;
        yylval->string.length = length;
        break;

    case IMPORT_SYM:
    case PAGE_SYM:
    case MEDIA_SYM:
    case FONT_FACE_SYM:
    case CHARSET_SYM:
    case NAMESPACE_SYM:
    case WEBKIT_KEYFRAMES_SYM:

    case IMPORTANT_SYM:
        break;

    case QEMS:
        length--;
    case GRADS:
    case TURNS:
        length--;
    case DEGS:
    case RADS:
    case KHERTZ:
    case REMS:
        length--;
    case MSECS:
    case HERTZ:
    case EMS:
    case EXS:
    case PXS:
    case CMS:
    case MMS:
    case INS:
    case PTS:
    case PCS:
        length--;
    case SECS:
    case PERCENTAGE:
        length--;
    case FLOATTOKEN:
    case INTEGER:
        yylval->number = charactersToDouble(t, length);
        break;

    default:
        break;
    }

    return token();
}

void CSSParser::recheckAtKeyword(const UChar* str, int len)
{
    String ruleName(str, len);
    if (equalIgnoringCase(ruleName, "@import"))
        yyTok = IMPORT_SYM;
    else if (equalIgnoringCase(ruleName, "@page"))
        yyTok = PAGE_SYM;
    else if (equalIgnoringCase(ruleName, "@media"))
        yyTok = MEDIA_SYM;
    else if (equalIgnoringCase(ruleName, "@font-face"))
        yyTok = FONT_FACE_SYM;
    else if (equalIgnoringCase(ruleName, "@charset"))
        yyTok = CHARSET_SYM;
    else if (equalIgnoringCase(ruleName, "@namespace"))
        yyTok = NAMESPACE_SYM;
    else if (equalIgnoringCase(ruleName, "@-webkit-keyframes"))
        yyTok = WEBKIT_KEYFRAMES_SYM;
    else if (equalIgnoringCase(ruleName, "@-webkit-mediaquery"))
        yyTok = WEBKIT_MEDIAQUERY_SYM;
}

UChar* CSSParser::text(int *length)
{
    UChar* start = yytext;
    int l = yyleng;
    switch (yyTok) {
    case STRING:
        l--;
        /* nobreak */
    case HEX:
    case IDSEL:
        start++;
        l--;
        break;
    case URI:
        // "url("{w}{string}{w}")"
        // "url("{w}{url}{w}")"
        // strip "url(" and ")"
        start += 4;
        l -= 5;
        // strip {w}
        while (l && isHTMLSpace(*start)) {
            ++start;
            --l;
        }
        while (l && isHTMLSpace(start[l - 1]))
            --l;
        if (l && (*start == '"' || *start == '\'')) {
            ASSERT(l >= 2 && start[l - 1] == *start);
            ++start;
            l -= 2;
        }
        break;
    default:
        break;
    }

    // process escapes
    UChar* out = start;
    UChar* escape = 0;

    bool sawEscape = false;

    for (int i = 0; i < l; i++) {
        UChar* current = start + i;
        if (escape == current - 1) {
            if (isASCIIHexDigit(*current))
                continue;
            if (yyTok == STRING &&
                 (*current == '\n' || *current == '\r' || *current == '\f')) {
                // ### handle \r\n case
                if (*current != '\r')
                    escape = 0;
                continue;
            }
            // in all other cases copy the char to output
            // ###
            *out++ = *current;
            escape = 0;
            continue;
        }
        if (escape == current - 2 && yyTok == STRING &&
             *(current-1) == '\r' && *current == '\n') {
            escape = 0;
            continue;
        }
        if (escape > current - 7 && isASCIIHexDigit(*current))
            continue;
        if (escape) {
            // add escaped char
            unsigned uc = 0;
            escape++;
            while (escape < current) {
                uc *= 16;
                uc += toASCIIHexValue(*escape);
                escape++;
            }
            // can't handle chars outside ucs2
            if (uc > 0xffff)
                uc = 0xfffd;
            *out++ = uc;
            escape = 0;
            if (isHTMLSpace(*current))
                continue;
        }
        if (!escape && *current == '\\') {
            escape = current;
            sawEscape = true;
            continue;
        }
        *out++ = *current;
    }
    if (escape) {
        // add escaped char
        unsigned uc = 0;
        escape++;
        while (escape < start+l) {
            uc *= 16;
            uc += toASCIIHexValue(*escape);
            escape++;
        }
        // can't handle chars outside ucs2
        if (uc > 0xffff)
            uc = 0xfffd;
        *out++ = uc;
    }

    *length = out - start;

    // If we have an unrecognized @-keyword, and if we handled any escapes at all, then
    // we should attempt to adjust yyTok to the correct type.
    if (yyTok == ATKEYWORD && sawEscape)
        recheckAtKeyword(start, *length);

    return start;
}

void CSSParser::countLines()
{
    for (UChar* current = yytext; current < yytext + yyleng; ++current) {
        if (*current == '\n')
            ++m_lineNumber;
    }
}

CSSParserSelector* CSSParser::createFloatingSelector()
{
    CSSParserSelector* selector = new CSSParserSelector;
    m_floatingSelectors.add(selector);
    return selector;
}

PassOwnPtr<CSSParserSelector> CSSParser::sinkFloatingSelector(CSSParserSelector* selector)
{
    if (selector) {
        ASSERT(m_floatingSelectors.contains(selector));
        m_floatingSelectors.remove(selector);
    }
    return adoptPtr(selector);
}

Vector<OwnPtr<CSSParserSelector> >* CSSParser::createFloatingSelectorVector()
{
    Vector<OwnPtr<CSSParserSelector> >* selectorVector = new Vector<OwnPtr<CSSParserSelector> >;
    m_floatingSelectorVectors.add(selectorVector);
    return selectorVector;
}

PassOwnPtr<Vector<OwnPtr<CSSParserSelector> > > CSSParser::sinkFloatingSelectorVector(Vector<OwnPtr<CSSParserSelector> >* selectorVector)
{
    if (selectorVector) {
        ASSERT(m_floatingSelectorVectors.contains(selectorVector));
        m_floatingSelectorVectors.remove(selectorVector);
    }
    return adoptPtr(selectorVector);
}

CSSParserValueList* CSSParser::createFloatingValueList()
{
    CSSParserValueList* list = new CSSParserValueList;
    m_floatingValueLists.add(list);
    return list;
}

CSSParserValueList* CSSParser::sinkFloatingValueList(CSSParserValueList* list)
{
    if (list) {
        ASSERT(m_floatingValueLists.contains(list));
        m_floatingValueLists.remove(list);
    }
    return list;
}

CSSParserFunction* CSSParser::createFloatingFunction()
{
    CSSParserFunction* function = new CSSParserFunction;
    m_floatingFunctions.add(function);
    return function;
}

CSSParserFunction* CSSParser::sinkFloatingFunction(CSSParserFunction* function)
{
    if (function) {
        ASSERT(m_floatingFunctions.contains(function));
        m_floatingFunctions.remove(function);
    }
    return function;
}

CSSParserValue& CSSParser::sinkFloatingValue(CSSParserValue& value)
{
    if (value.unit == CSSParserValue::Function) {
        ASSERT(m_floatingFunctions.contains(value.function));
        m_floatingFunctions.remove(value.function);
    }
    return value;
}

MediaQueryExp* CSSParser::createFloatingMediaQueryExp(const AtomicString& mediaFeature, CSSParserValueList* values)
{
    m_floatingMediaQueryExp = MediaQueryExp::create(mediaFeature, values);
    return m_floatingMediaQueryExp.get();
}

PassOwnPtr<MediaQueryExp> CSSParser::sinkFloatingMediaQueryExp(MediaQueryExp* expression)
{
    ASSERT_UNUSED(expression, expression == m_floatingMediaQueryExp);
    return m_floatingMediaQueryExp.release();
}

Vector<OwnPtr<MediaQueryExp> >* CSSParser::createFloatingMediaQueryExpList()
{
    m_floatingMediaQueryExpList = adoptPtr(new Vector<OwnPtr<MediaQueryExp> >);
    return m_floatingMediaQueryExpList.get();
}

PassOwnPtr<Vector<OwnPtr<MediaQueryExp> > > CSSParser::sinkFloatingMediaQueryExpList(Vector<OwnPtr<MediaQueryExp> >* list)
{
    ASSERT_UNUSED(list, list == m_floatingMediaQueryExpList);
    return m_floatingMediaQueryExpList.release();
}

MediaQuery* CSSParser::createFloatingMediaQuery(MediaQuery::Restrictor restrictor, const String& mediaType, PassOwnPtr<Vector<OwnPtr<MediaQueryExp> > > expressions)
{
    m_floatingMediaQuery = adoptPtr(new MediaQuery(restrictor, mediaType, expressions));
    return m_floatingMediaQuery.get();
}

MediaQuery* CSSParser::createFloatingMediaQuery(PassOwnPtr<Vector<OwnPtr<MediaQueryExp> > > expressions)
{
    return createFloatingMediaQuery(MediaQuery::None, "all", expressions);
}

PassOwnPtr<MediaQuery> CSSParser::sinkFloatingMediaQuery(MediaQuery* query)
{
    ASSERT_UNUSED(query, query == m_floatingMediaQuery);
    return m_floatingMediaQuery.release();
}

MediaList* CSSParser::createMediaList()
{
    RefPtr<MediaList> list = MediaList::create();
    MediaList* result = list.get();
    m_parsedStyleObjects.append(list.release());
    return result;
}

CSSRule* CSSParser::createCharsetRule(const CSSParserString& charset)
{
    if (!m_styleSheet)
        return 0;
    RefPtr<CSSCharsetRule> rule = CSSCharsetRule::create(m_styleSheet, charset);
    CSSCharsetRule* result = rule.get();
    m_parsedStyleObjects.append(rule.release());
    return result;
}

CSSRule* CSSParser::createImportRule(const CSSParserString& url, MediaList* media)
{
    if (!media || !m_styleSheet || !m_allowImportRules)
        return 0;
    RefPtr<CSSImportRule> rule = CSSImportRule::create(m_styleSheet, url, media);
    CSSImportRule* result = rule.get();
    m_parsedStyleObjects.append(rule.release());
    return result;
}

CSSRule* CSSParser::createMediaRule(MediaList* media, CSSRuleList* rules)
{
    if (!media || !rules || !m_styleSheet)
        return 0;
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    RefPtr<CSSMediaRule> rule = CSSMediaRule::create(m_styleSheet, media, rules);
    CSSMediaRule* result = rule.get();
    m_parsedStyleObjects.append(rule.release());
    return result;
}

CSSRuleList* CSSParser::createRuleList()
{
    RefPtr<CSSRuleList> list = CSSRuleList::create();
    CSSRuleList* listPtr = list.get();

    m_parsedRuleLists.append(list.release());
    return listPtr;
}

WebKitCSSKeyframesRule* CSSParser::createKeyframesRule()
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    RefPtr<WebKitCSSKeyframesRule> rule = WebKitCSSKeyframesRule::create(m_styleSheet);
    WebKitCSSKeyframesRule* rulePtr = rule.get();
    m_parsedStyleObjects.append(rule.release());
    return rulePtr;
}

CSSRule* CSSParser::createStyleRule(Vector<OwnPtr<CSSParserSelector> >* selectors)
{
    CSSStyleRule* result = 0;
    markRuleBodyEnd();
    if (selectors) {
        m_allowImportRules = m_allowNamespaceDeclarations = false;
        RefPtr<CSSStyleRule> rule = CSSStyleRule::create(m_styleSheet, m_lastSelectorLineNumber);
        rule->adoptSelectorVector(*selectors);
        if (m_hasFontFaceOnlyValues)
            deleteFontFaceOnlyValues();
        rule->setDeclaration(CSSMutableStyleDeclaration::create(rule.get(), m_parsedProperties, m_numParsedProperties));
        result = rule.get();
        m_parsedStyleObjects.append(rule.release());
        if (m_ruleRangeMap) {
            ASSERT(m_currentRuleData);
            m_currentRuleData->styleSourceData->styleBodyRange = m_ruleBodyRange;
            m_currentRuleData->selectorListRange = m_selectorListRange;
            m_ruleRangeMap->set(result, m_currentRuleData.release());
            m_currentRuleData = CSSRuleSourceData::create();
            m_currentRuleData->styleSourceData = CSSStyleSourceData::create();
            m_inStyleRuleOrDeclaration = false;
        }
    }
    resetSelectorListMarks();
    resetRuleBodyMarks();
    clearProperties();
    return result;
}

CSSRule* CSSParser::createFontFaceRule()
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    for (unsigned i = 0; i < m_numParsedProperties; ++i) {
        CSSProperty* property = m_parsedProperties[i];
        int id = property->id();
        if ((id == CSSPropertyFontWeight || id == CSSPropertyFontStyle || id == CSSPropertyFontVariant) && property->value()->isPrimitiveValue()) {
            RefPtr<CSSValue> value = property->m_value.release();
            property->m_value = CSSValueList::createCommaSeparated();
            static_cast<CSSValueList*>(property->value())->append(value.release());
        } else if (id == CSSPropertyFontFamily && (!property->value()->isValueList() || static_cast<CSSValueList*>(property->value())->length() != 1)) {
            // Unlike font-family property, font-family descriptor in @font-face rule 
            // has to be a value list with exactly one family name. It cannot have a
            // have 'initial' value and cannot 'inherit' from parent.
            // See http://dev.w3.org/csswg/css3-fonts/#font-family-desc
            clearProperties();
            return 0;
        }
    }
    RefPtr<CSSFontFaceRule> rule = CSSFontFaceRule::create(m_styleSheet);
    rule->setDeclaration(CSSMutableStyleDeclaration::create(rule.get(), m_parsedProperties, m_numParsedProperties));
    clearProperties();
    CSSFontFaceRule* result = rule.get();
    m_parsedStyleObjects.append(rule.release());
    return result;
}

void CSSParser::addNamespace(const AtomicString& prefix, const AtomicString& uri)
{
    if (!m_styleSheet || !m_allowNamespaceDeclarations)
        return;
    m_allowImportRules = false;
    m_styleSheet->addNamespace(this, prefix, uri);
}

void CSSParser::updateSpecifiersWithElementName(const AtomicString& namespacePrefix, const AtomicString& elementName, CSSParserSelector* specifiers)
{
    AtomicString determinedNamespace = namespacePrefix != nullAtom && m_styleSheet ? m_styleSheet->determineNamespace(namespacePrefix) : m_defaultNamespace;
    QualifiedName tag = QualifiedName(namespacePrefix, elementName, determinedNamespace);
    if (!specifiers->isUnknownPseudoElement()) {
        specifiers->setTag(tag);
        return;
    }

    CSSParserSelector* lastShadowDescendant = specifiers;
    CSSParserSelector* history = specifiers;
    while (history->tagHistory()) {
        history = history->tagHistory();
        if (history->hasShadowDescendant())
            lastShadowDescendant = history;
    }

    if (lastShadowDescendant->tagHistory()) {
        lastShadowDescendant->tagHistory()->setTag(tag);
        return;
    }

    // No need to create an extra element name selector if we are matching any element
    // in any namespace.
    if (elementName == starAtom && m_defaultNamespace == starAtom)
        return;

    OwnPtr<CSSParserSelector> elementNameSelector = adoptPtr(new CSSParserSelector);
    elementNameSelector->setTag(tag);
    lastShadowDescendant->setTagHistory(elementNameSelector.release());
    lastShadowDescendant->setRelation(CSSSelector::ShadowDescendant);
}

CSSParserSelector* CSSParser::updateSpecifiers(CSSParserSelector* specifiers, CSSParserSelector* newSpecifier)
{
    if (newSpecifier->isUnknownPseudoElement()) {
        // Unknown pseudo element always goes at the top of selector chain.
        newSpecifier->appendTagHistory(CSSSelector::ShadowDescendant, sinkFloatingSelector(specifiers));
        return newSpecifier;
    }
    if (specifiers->isUnknownPseudoElement()) {
        // Specifiers for unknown pseudo element go right behind it in the chain.
        specifiers->insertTagHistory(CSSSelector::SubSelector, sinkFloatingSelector(newSpecifier), CSSSelector::ShadowDescendant);
        return specifiers;
    }
    specifiers->appendTagHistory(CSSSelector::SubSelector, sinkFloatingSelector(newSpecifier));
    return specifiers;
}

CSSRule* CSSParser::createPageRule(PassOwnPtr<CSSParserSelector> pageSelector)
{
    // FIXME: Margin at-rules are ignored.
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    CSSPageRule* pageRule = 0;
    if (pageSelector) {
        RefPtr<CSSPageRule> rule = CSSPageRule::create(m_styleSheet, m_lastSelectorLineNumber);
        Vector<OwnPtr<CSSParserSelector> > selectorVector;
        selectorVector.append(pageSelector);
        rule->adoptSelectorVector(selectorVector);
        rule->setDeclaration(CSSMutableStyleDeclaration::create(rule.get(), m_parsedProperties, m_numParsedProperties));
        pageRule = rule.get();
        m_parsedStyleObjects.append(rule.release());
    }
    clearProperties();
    return pageRule;
}

CSSRule* CSSParser::createMarginAtRule(CSSSelector::MarginBoxType /* marginBox */)
{
    // FIXME: Implement margin at-rule here, using:
    //        - marginBox: margin box
    //        - m_parsedProperties: properties at [m_numParsedPropertiesBeforeMarginBox, m_numParsedProperties) are for this at-rule.
    // Don't forget to also update the action for page symbol in CSSGrammar.y such that margin at-rule data is cleared if page_selector is invalid.

    endDeclarationsForMarginBox();
    return 0; // until this method is implemented.
}

void CSSParser::startDeclarationsForMarginBox()
{
    m_numParsedPropertiesBeforeMarginBox = m_numParsedProperties;
}

void CSSParser::endDeclarationsForMarginBox()
{
    ASSERT(m_numParsedPropertiesBeforeMarginBox != INVALID_NUM_PARSED_PROPERTIES);
    rollbackLastProperties(m_numParsedProperties - m_numParsedPropertiesBeforeMarginBox);
    m_numParsedPropertiesBeforeMarginBox = INVALID_NUM_PARSED_PROPERTIES;
}

void CSSParser::deleteFontFaceOnlyValues()
{
    ASSERT(m_hasFontFaceOnlyValues);
    int deletedProperties = 0;

    for (unsigned i = 0; i < m_numParsedProperties; ++i) {
        CSSProperty* property = m_parsedProperties[i];
        int id = property->id();
        if ((id == CSSPropertyFontWeight || id == CSSPropertyFontStyle || id == CSSPropertyFontVariant) && property->value()->isValueList()) {
            delete property;
            deletedProperties++;
        } else if (deletedProperties)
            m_parsedProperties[i - deletedProperties] = m_parsedProperties[i];
    }

    m_numParsedProperties -= deletedProperties;
}

WebKitCSSKeyframeRule* CSSParser::createKeyframeRule(CSSParserValueList* keys)
{
    // Create a key string from the passed keys
    String keyString;
    for (unsigned i = 0; i < keys->size(); ++i) {
        float key = (float) keys->valueAt(i)->fValue;
        if (i != 0)
            keyString += ",";
        keyString += String::number(key);
        keyString += "%";
    }

    RefPtr<WebKitCSSKeyframeRule> keyframe = WebKitCSSKeyframeRule::create(m_styleSheet);
    keyframe->setKeyText(keyString);
    keyframe->setDeclaration(CSSMutableStyleDeclaration::create(0, m_parsedProperties, m_numParsedProperties));

    clearProperties();

    WebKitCSSKeyframeRule* keyframePtr = keyframe.get();
    m_parsedStyleObjects.append(keyframe.release());
    return keyframePtr;
}

void CSSParser::invalidBlockHit()
{
    if (m_styleSheet && !m_hadSyntacticallyValidCSSRule)
        m_styleSheet->setHasSyntacticallyValidCSSHeader(false);
}

void CSSParser::updateLastSelectorLineAndPosition()
{
    m_lastSelectorLineNumber = m_lineNumber;
    markRuleBodyStart();
}

void CSSParser::markSelectorListStart()
{
    m_selectorListRange.start = yytext - m_data;
}

void CSSParser::markSelectorListEnd()
{
    if (!m_currentRuleData)
        return;
    UChar* listEnd = yytext;
    while (listEnd > m_data + 1) {
        if (isHTMLSpace(*(listEnd - 1)))
            --listEnd;
        else
            break;
    }
    m_selectorListRange.end = listEnd - m_data;
}

void CSSParser::markRuleBodyStart()
{
    unsigned offset = yytext - m_data;
    if (*yytext == '{')
        ++offset; // Skip the rule body opening brace.
    if (offset > m_ruleBodyRange.start)
        m_ruleBodyRange.start = offset;
    m_inStyleRuleOrDeclaration = true;
}

void CSSParser::markRuleBodyEnd()
{
    unsigned offset = yytext - m_data;
    if (offset > m_ruleBodyRange.end)
        m_ruleBodyRange.end = offset;
}

void CSSParser::markPropertyStart()
{
    if (!m_inStyleRuleOrDeclaration)
        return;
    m_propertyRange.start = yytext - m_data;
}

void CSSParser::markPropertyEnd(bool isImportantFound, bool isPropertyParsed)
{
    if (!m_inStyleRuleOrDeclaration)
        return;
    unsigned offset = yytext - m_data;
    if (*yytext == ';') // Include semicolon into the property text.
        ++offset;
    m_propertyRange.end = offset;
    if (m_propertyRange.start != UINT_MAX && m_currentRuleData) {
        // This stuff is only executed when the style data retrieval is requested by client.
        const unsigned start = m_propertyRange.start;
        const unsigned end = m_propertyRange.end;
        ASSERT(start < end);
        String propertyString = String(m_data + start, end - start).stripWhiteSpace();
        if (propertyString.endsWith(";", true))
            propertyString = propertyString.left(propertyString.length() - 1);
        Vector<String> propertyComponents;
        size_t colonIndex = propertyString.find(":");
        ASSERT(colonIndex != notFound);

        String name = propertyString.left(colonIndex).stripWhiteSpace();
        String value = propertyString.substring(colonIndex + 1, propertyString.length()).stripWhiteSpace();
        // The property range is relative to the declaration start offset.
        m_currentRuleData->styleSourceData->propertyData.append(
            CSSPropertySourceData(name, value, isImportantFound, isPropertyParsed, SourceRange(start - m_ruleBodyRange.start, end - m_ruleBodyRange.start)));
    }
    resetPropertyMarks();
}

static int cssPropertyID(const UChar* propertyName, unsigned length)
{
    if (!length)
        return 0;
    if (length > maxCSSPropertyNameLength)
        return 0;

    char buffer[maxCSSPropertyNameLength + 1 + 1]; // 1 to turn "apple"/"khtml" into "webkit", 1 for null character

    for (unsigned i = 0; i != length; ++i) {
        UChar c = propertyName[i];
        if (c == 0 || c >= 0x7F)
            return 0; // illegal character
        buffer[i] = toASCIILower(c);
    }
    buffer[length] = '\0';

    const char* name = buffer;
    if (buffer[0] == '-') {
        // If the prefix is -apple- or -khtml-, change it to -webkit-.
        // This makes the string one character longer.
        if (hasPrefix(buffer, length, "-apple-") || hasPrefix(buffer, length, "-khtml-")) {
            memmove(buffer + 7, buffer + 6, length + 1 - 6);
            memcpy(buffer, "-webkit", 7);
            ++length;
        }

#if PLATFORM(IOS)
        if (!strcmp(buffer, "-webkit-hyphenate-locale")) {
            // Worked in iOS 4.2.
            const char* const webkitLocale = "-webkit-locale";
            name = webkitLocale;
            length = strlen(webkitLocale);
        }
#endif
    }

    const Property* hashTableEntry = findProperty(name, length);
    return hashTableEntry ? hashTableEntry->id : 0;
}

int cssPropertyID(const String& string)
{
    return cssPropertyID(string.characters(), string.length());
}

int cssPropertyID(const CSSParserString& string)
{
    return cssPropertyID(string.characters, string.length);
}

int cssValueKeywordID(const CSSParserString& string)
{
    unsigned length = string.length;
    if (!length)
        return 0;
    if (length > maxCSSValueKeywordLength)
        return 0;

    char buffer[maxCSSValueKeywordLength + 1 + 1]; // 1 to turn "apple"/"khtml" into "webkit", 1 for null character

    for (unsigned i = 0; i != length; ++i) {
        UChar c = string.characters[i];
        if (c == 0 || c >= 0x7F)
            return 0; // illegal character
        buffer[i] = WTF::toASCIILower(c);
    }
    buffer[length] = '\0';

    if (buffer[0] == '-') {
        // If the prefix is -apple- or -khtml-, change it to -webkit-.
        // This makes the string one character longer.
        if (hasPrefix(buffer, length, "-apple-") || hasPrefix(buffer, length, "-khtml-")) {
            memmove(buffer + 7, buffer + 6, length + 1 - 6);
            memcpy(buffer, "-webkit", 7);
            ++length;
        }
    }

    const Value* hashTableEntry = findValue(buffer, length);
    return hashTableEntry ? hashTableEntry->id : 0;
}

// "ident" from the CSS tokenizer, minus backslash-escape sequences
static bool isCSSTokenizerIdentifier(const String& string)
{
    const UChar* p = string.characters();
    const UChar* end = p + string.length();

    // -?
    if (p != end && p[0] == '-')
        ++p;

    // {nmstart}
    if (p == end || !(p[0] == '_' || p[0] >= 128 || isASCIIAlpha(p[0])))
        return false;
    ++p;

    // {nmchar}*
    for (; p != end; ++p) {
        if (!(p[0] == '_' || p[0] == '-' || p[0] >= 128 || isASCIIAlphanumeric(p[0])))
            return false;
    }

    return true;
}

// "url" from the CSS tokenizer, minus backslash-escape sequences
static bool isCSSTokenizerURL(const String& string)
{
    const UChar* p = string.characters();
    const UChar* end = p + string.length();

    for (; p != end; ++p) {
        UChar c = p[0];
        switch (c) {
            case '!':
            case '#':
            case '$':
            case '%':
            case '&':
                break;
            default:
                if (c < '*')
                    return false;
                if (c <= '~')
                    break;
                if (c < 128)
                    return false;
        }
    }

    return true;
}

// We use single quotes for now because markup.cpp uses double quotes.
String quoteCSSString(const String& string)
{
    // For efficiency, we first pre-calculate the length of the quoted string, then we build the actual one.
    // Please see below for the actual logic.
    unsigned quotedStringSize = 2; // Two quotes surrounding the entire string.
    bool afterEscape = false;
    for (unsigned i = 0; i < string.length(); ++i) {
        UChar ch = string[i];
        if (ch == '\\' || ch == '\'') {
            quotedStringSize += 2;
            afterEscape = false;
        } else if (ch < 0x20 || ch == 0x7F) {
            quotedStringSize += 2 + (ch >= 0x10);
            afterEscape = true;
        } else {
            quotedStringSize += 1 + (afterEscape && (isASCIIHexDigit(ch) || ch == ' '));
            afterEscape = false;
        }
    }

    StringBuffer buffer(quotedStringSize);
    unsigned index = 0;
    buffer[index++] = '\'';
    afterEscape = false;
    for (unsigned i = 0; i < string.length(); ++i) {
        UChar ch = string[i];
        if (ch == '\\' || ch == '\'') {
            buffer[index++] = '\\';
            buffer[index++] = ch;
            afterEscape = false;
        } else if (ch < 0x20 || ch == 0x7F) { // Control characters.
            buffer[index++] = '\\';
            placeByteAsHexCompressIfPossible(ch, buffer, index, Lowercase);
            afterEscape = true;
        } else {
            // Space character may be required to separate backslash-escape sequence and normal characters.
            if (afterEscape && (isASCIIHexDigit(ch) || ch == ' '))
                buffer[index++] = ' ';
            buffer[index++] = ch;
            afterEscape = false;
        }
    }
    buffer[index++] = '\'';

    ASSERT(quotedStringSize == index);
    return String::adopt(buffer);
}

String quoteCSSStringIfNeeded(const String& string)
{
    return isCSSTokenizerIdentifier(string) ? string : quoteCSSString(string);
}

String quoteCSSURLIfNeeded(const String& string)
{
    return isCSSTokenizerURL(string) ? string : quoteCSSString(string);
}

bool isValidNthToken(const CSSParserString& token)
{
    // The tokenizer checks for the construct of an+b.
    // nth can also accept "n", "odd" or "even" but should not accept any other token.
    return equalIgnoringCase(token, "odd") || equalIgnoringCase(token, "even") || equalIgnoringCase(token, "n");
}

#define YY_DECL int CSSParser::lex()
#define yyconst const
typedef int yy_state_type;
typedef unsigned YY_CHAR;
// The following line makes sure we treat non-Latin-1 Unicode characters correctly.
#define YY_SC_TO_UI(c) (c > 0xff ? 0xff : c)
#define YY_DO_BEFORE_ACTION \
        yytext = yy_bp; \
        yyleng = (int) (yy_cp - yy_bp); \
        yy_hold_char = *yy_cp; \
        *yy_cp = 0; \
        yy_c_buf_p = yy_cp;
#define YY_BREAK break;
#define ECHO
#define YY_RULE_SETUP
#define INITIAL 0
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)
#define yyterminate() yyTok = END_TOKEN; return yyTok
#define YY_FATAL_ERROR(a)
// The following line is needed to build the tokenizer with a condition stack.
// The macro is used in the tokenizer grammar with lines containing
// BEGIN(mediaqueries) and BEGIN(initial). yy_start acts as index to
// tokenizer transition table, and 'mediaqueries' and 'initial' are
// offset multipliers that specify which transitions are active
// in the tokenizer during in each condition (tokenizer state).
#define BEGIN yy_start = 1 + 2 *

#include "tokenizer.cpp"

}
