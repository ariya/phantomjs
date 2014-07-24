/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "CSSAspectRatioValue.h"
#include "CSSBasicShapes.h"
#include "CSSBorderImage.h"
#include "CSSCanvasValue.h"
#include "CSSCrossfadeValue.h"
#include "CSSCursorImageValue.h"
#include "CSSFontFaceRule.h"
#include "CSSFontFaceSrcValue.h"
#include "CSSFunctionValue.h"
#include "CSSGradientValue.h"
#include "CSSImageValue.h"
#include "CSSInheritedValue.h"
#include "CSSInitialValue.h"
#include "CSSLineBoxContainValue.h"
#include "CSSMediaRule.h"
#include "CSSPageRule.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertySourceData.h"
#include "CSSReflectValue.h"
#include "CSSSelector.h"
#include "CSSStyleSheet.h"
#include "CSSTimingFunctionValue.h"
#include "CSSUnicodeRangeValue.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#if ENABLE(CSS_VARIABLES)
#include "CSSVariableValue.h"
#endif
#include "Counter.h"
#include "Document.h"
#include "FloatConversion.h"
#include "FontFeatureValue.h"
#include "FontValue.h"
#include "HTMLParserIdioms.h"
#include "HashTools.h"
#include "HistogramSupport.h"
#include "MediaList.h"
#include "MediaQueryExp.h"
#include "Page.h"
#include "PageConsole.h"
#include "Pair.h"
#include "Rect.h"
#include "RenderTheme.h"
#include "RuntimeEnabledFeatures.h"
#include "SVGParserUtilities.h"
#include "Settings.h"
#include "ShadowValue.h"
#include "StylePropertySet.h"
#include "StylePropertyShorthand.h"
#include "StyleRule.h"
#include "StyleRuleImport.h"
#include "StyleSheetContents.h"
#include "TextEncoding.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"
#include "WebKitCSSRegionRule.h"
#include "WebKitCSSTransformValue.h"
#include <limits.h>
#include <wtf/BitArray.h>
#include <wtf/HexNumber.h>
#include <wtf/dtoa.h>
#include <wtf/text/StringBuffer.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringImpl.h>

#if ENABLE(CSS_IMAGE_SET)
#include "CSSImageSetValue.h"
#endif

#if ENABLE(CSS_FILTERS)
#include "WebKitCSSFilterValue.h"
#if ENABLE(SVG)
#include "WebKitCSSSVGDocumentValue.h"
#endif
#endif

#if ENABLE(CSS_SHADERS)
#include "WebKitCSSArrayFunctionValue.h"
#include "WebKitCSSMatFunctionValue.h"
#include "WebKitCSSMixFunctionValue.h"
#include "WebKitCSSShaderValue.h"
#endif

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

#define YYDEBUG 0

#if YYDEBUG > 0
extern int cssyydebug;
#endif

extern int cssyyparse(WebCore::CSSParser*);

using namespace std;
using namespace WTF;

namespace {

enum PropertyType {
    PropertyExplicit,
    PropertyImplicit
};

class ImplicitScope {
    WTF_MAKE_NONCOPYABLE(ImplicitScope);
public:
    ImplicitScope(WebCore::CSSParser* parser, PropertyType propertyType)
        : m_parser(parser)
    {
        m_parser->m_implicitShorthand = propertyType == PropertyImplicit;
    }

    ~ImplicitScope()
    {
        m_parser->m_implicitShorthand = false;
    }

private:
    WebCore::CSSParser* m_parser;
};

} // namespace

namespace WebCore {

static const unsigned INVALID_NUM_PARSED_PROPERTIES = UINT_MAX;
static const double MAX_SCALE = 1000000;

template <unsigned N>
static bool equal(const CSSParserString& a, const char (&b)[N])
{
    unsigned length = N - 1; // Ignore the trailing null character
    if (a.length() != length)
        return false;

    return a.is8Bit() ? WTF::equal(a.characters8(), reinterpret_cast<const LChar*>(b), length) : WTF::equal(a.characters16(), reinterpret_cast<const LChar*>(b), length);
}

template <unsigned N>
static bool equalIgnoringCase(const CSSParserString& a, const char (&b)[N])
{
    unsigned length = N - 1; // Ignore the trailing null character
    if (a.length() != length)
        return false;

    return a.is8Bit() ? WTF::equalIgnoringCase(b, a.characters8(), length) : WTF::equalIgnoringCase(b, a.characters16(), length);
}

template <unsigned N>
static bool equalIgnoringCase(CSSParserValue* value, const char (&b)[N])
{
    ASSERT(value->unit == CSSPrimitiveValue::CSS_IDENT || value->unit == CSSPrimitiveValue::CSS_STRING);
    return equalIgnoringCase(value->string, b);
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

static PassRefPtr<CSSPrimitiveValue> createPrimitiveValuePair(PassRefPtr<CSSPrimitiveValue> first, PassRefPtr<CSSPrimitiveValue> second)
{
    return cssValuePool().createValue(Pair::create(first, second));
}

class AnimationParseContext {
public:
    AnimationParseContext()
        : m_animationPropertyKeywordAllowed(true)
        , m_firstAnimationCommitted(false)
        , m_hasSeenAnimationPropertyKeyword(false)
    {
    }

    void commitFirstAnimation()
    {
        m_firstAnimationCommitted = true;
    }

    bool hasCommittedFirstAnimation() const
    {
        return m_firstAnimationCommitted;
    }

    void commitAnimationPropertyKeyword()
    {
        m_animationPropertyKeywordAllowed = false;
    }

    bool animationPropertyKeywordAllowed() const
    {
        return m_animationPropertyKeywordAllowed;
    }

    bool hasSeenAnimationPropertyKeyword() const
    {
        return m_hasSeenAnimationPropertyKeyword;
    }

    void sawAnimationPropertyKeyword()
    {
        m_hasSeenAnimationPropertyKeyword = true;
    }

private:
    bool m_animationPropertyKeywordAllowed;
    bool m_firstAnimationCommitted;
    bool m_hasSeenAnimationPropertyKeyword;
};

const CSSParserContext& strictCSSParserContext()
{
    DEFINE_STATIC_LOCAL(CSSParserContext, strictContext, (CSSStrictMode));
    return strictContext;
}

CSSParserContext::CSSParserContext(CSSParserMode mode, const KURL& baseURL)
    : baseURL(baseURL)
    , mode(mode)
    , isHTMLDocument(false)
    , isCSSCustomFilterEnabled(false)
    , isCSSStickyPositionEnabled(false)
    , isCSSRegionsEnabled(false)
    , isCSSCompositingEnabled(false)
    , isCSSGridLayoutEnabled(false)
#if ENABLE(CSS_VARIABLES)
    , isCSSVariablesEnabled(false)
#endif
    , needsSiteSpecificQuirks(false)
    , enforcesCSSMIMETypeInNoQuirksMode(true)
    , useLegacyBackgroundSizeShorthandBehavior(false)
{
}

CSSParserContext::CSSParserContext(Document* document, const KURL& baseURL, const String& charset)
    : baseURL(baseURL.isNull() ? document->baseURL() : baseURL)
    , charset(charset)
    , mode(document->inQuirksMode() ? CSSQuirksMode : CSSStrictMode)
    , isHTMLDocument(document->isHTMLDocument())
    , isCSSCustomFilterEnabled(document->settings() ? document->settings()->isCSSCustomFilterEnabled() : false)
    , isCSSStickyPositionEnabled(document->cssStickyPositionEnabled())
    , isCSSRegionsEnabled(document->cssRegionsEnabled())
    , isCSSCompositingEnabled(document->cssCompositingEnabled())
    , isCSSGridLayoutEnabled(document->cssGridLayoutEnabled())
#if ENABLE(CSS_VARIABLES)
    , isCSSVariablesEnabled(document->settings() ? document->settings()->cssVariablesEnabled() : false)
#endif
    , needsSiteSpecificQuirks(document->settings() ? document->settings()->needsSiteSpecificQuirks() : false)
    , enforcesCSSMIMETypeInNoQuirksMode(!document->settings() || document->settings()->enforceCSSMIMETypeInNoQuirksMode())
    , useLegacyBackgroundSizeShorthandBehavior(document->settings() ? document->settings()->useLegacyBackgroundSizeShorthandBehavior() : false)
{
}

bool operator==(const CSSParserContext& a, const CSSParserContext& b)
{
    return a.baseURL == b.baseURL
        && a.charset == b.charset
        && a.mode == b.mode
        && a.isHTMLDocument == b.isHTMLDocument
        && a.isCSSCustomFilterEnabled == b.isCSSCustomFilterEnabled
        && a.isCSSStickyPositionEnabled == b.isCSSStickyPositionEnabled
        && a.isCSSRegionsEnabled == b.isCSSRegionsEnabled
        && a.isCSSCompositingEnabled == b.isCSSCompositingEnabled
        && a.isCSSGridLayoutEnabled == b.isCSSGridLayoutEnabled
#if ENABLE(CSS_VARIABLES)
        && a.isCSSVariablesEnabled == b.isCSSVariablesEnabled
#endif
        && a.needsSiteSpecificQuirks == b.needsSiteSpecificQuirks
        && a.enforcesCSSMIMETypeInNoQuirksMode == b.enforcesCSSMIMETypeInNoQuirksMode
        && a.useLegacyBackgroundSizeShorthandBehavior == b.useLegacyBackgroundSizeShorthandBehavior;
}

CSSParser::CSSParser(const CSSParserContext& context)
    : m_context(context)
    , m_important(false)
    , m_id(CSSPropertyInvalid)
    , m_styleSheet(0)
#if ENABLE(CSS3_CONDITIONAL_RULES)
    , m_supportsCondition(false)
#endif
    , m_selectorListForParseSelector(0)
    , m_numParsedPropertiesBeforeMarginBox(INVALID_NUM_PARSED_PROPERTIES)
    , m_inParseShorthand(0)
    , m_currentShorthand(CSSPropertyInvalid)
    , m_implicitShorthand(false)
    , m_hasFontFaceOnlyValues(false)
    , m_hadSyntacticallyValidCSSRule(false)
    , m_logErrors(false)
    , m_ignoreErrorsInDeclaration(false)
#if ENABLE(CSS_SHADERS)
    , m_inFilterRule(false)
#endif
    , m_defaultNamespace(starAtom)
    , m_parsedTextPrefixLength(0)
    , m_propertyRange(UINT_MAX, UINT_MAX)
    , m_ruleSourceDataResult(0)
    , m_parsingMode(NormalMode)
    , m_is8BitSource(false)
    , m_currentCharacter8(0)
    , m_currentCharacter16(0)
    , m_length(0)
    , m_token(0)
    , m_lineNumber(0)
    , m_tokenStartLineNumber(0)
    , m_lastSelectorLineNumber(0)
    , m_allowImportRules(true)
    , m_allowNamespaceDeclarations(true)
#if ENABLE(CSS_DEVICE_ADAPTATION)
    , m_inViewport(false)
#endif
{
#if YYDEBUG > 0
    cssyydebug = 1;
#endif
    m_tokenStart.ptr8 = 0;
}

CSSParser::~CSSParser()
{
    clearProperties();

    deleteAllValues(m_floatingSelectors);
    deleteAllValues(m_floatingSelectorVectors);
    deleteAllValues(m_floatingValueLists);
    deleteAllValues(m_floatingFunctions);
}

template <typename CharacterType>
ALWAYS_INLINE static void makeLower(const CharacterType* input, CharacterType* output, unsigned length)
{
    // FIXME: If we need Unicode lowercasing here, then we probably want the real kind
    // that can potentially change the length of the string rather than the character
    // by character kind. If we don't need Unicode lowercasing, it would be good to
    // simplify this function.

    if (charactersAreAllASCII(input, length)) {
        // Fast case for all-ASCII.
        for (unsigned i = 0; i < length; i++)
            output[i] = toASCIILower(input[i]);
    } else {
        for (unsigned i = 0; i < length; i++)
            output[i] = Unicode::toLower(input[i]);
    }
}

void CSSParserString::lower()
{
    if (is8Bit()) {
        makeLower(characters8(), characters8(), length());
        return;
    }

    makeLower(characters16(), characters16(), length());
}

#if ENABLE(CSS_VARIABLES)
AtomicString CSSParserString::substring(unsigned position, unsigned length) const
{
    ASSERT(m_length >= position + length);

    if (is8Bit())
        return AtomicString(characters8() + position, length);
    return AtomicString(characters16() + position, length);
}
#endif

void CSSParser::setupParser(const char* prefix, unsigned prefixLength, const String& string, const char* suffix, unsigned suffixLength)
{
    m_parsedTextPrefixLength = prefixLength;
    unsigned stringLength = string.length();
    unsigned length = stringLength + m_parsedTextPrefixLength + suffixLength + 1;
    m_length = length;

    if (!stringLength || string.is8Bit()) {
        m_dataStart8 = adoptArrayPtr(new LChar[length]);
        for (unsigned i = 0; i < m_parsedTextPrefixLength; i++)
            m_dataStart8[i] = prefix[i];

        if (stringLength)
            memcpy(m_dataStart8.get() + m_parsedTextPrefixLength, string.characters8(), stringLength * sizeof(LChar));

        unsigned start = m_parsedTextPrefixLength + stringLength;
        unsigned end = start + suffixLength;
        for (unsigned i = start; i < end; i++)
            m_dataStart8[i] = suffix[i - start];

        m_dataStart8[length - 1] = 0;

        m_is8BitSource = true;
        m_currentCharacter8 = m_dataStart8.get();
        m_currentCharacter16 = 0;
        setTokenStart<LChar>(m_currentCharacter8);
        m_lexFunc = &CSSParser::realLex<LChar>;
        return;
    }

    m_dataStart16 = adoptArrayPtr(new UChar[length]);
    for (unsigned i = 0; i < m_parsedTextPrefixLength; i++)
        m_dataStart16[i] = prefix[i];

    ASSERT(stringLength);
    memcpy(m_dataStart16.get() + m_parsedTextPrefixLength, string.characters16(), stringLength * sizeof(UChar));

    unsigned start = m_parsedTextPrefixLength + stringLength;
    unsigned end = start + suffixLength;
    for (unsigned i = start; i < end; i++)
        m_dataStart16[i] = suffix[i - start];

    m_dataStart16[length - 1] = 0;

    m_is8BitSource = false;
    m_currentCharacter8 = 0;
    m_currentCharacter16 = m_dataStart16.get();
    setTokenStart<UChar>(m_currentCharacter16);
    m_lexFunc = &CSSParser::realLex<UChar>;
}

void CSSParser::parseSheet(StyleSheetContents* sheet, const String& string, int startLineNumber, RuleSourceDataList* ruleSourceDataResult, bool logErrors)
{
    setStyleSheet(sheet);
    m_defaultNamespace = starAtom; // Reset the default namespace.
    if (ruleSourceDataResult)
        m_currentRuleDataStack = adoptPtr(new RuleSourceDataList());
    m_ruleSourceDataResult = ruleSourceDataResult;

    m_logErrors = logErrors && sheet->singleOwnerDocument() && !sheet->baseURL().isEmpty() && sheet->singleOwnerDocument()->page();
    m_ignoreErrorsInDeclaration = false;
    m_lineNumber = startLineNumber;
    setupParser("", string, "");
    cssyyparse(this);
    sheet->shrinkToFit();
    m_currentRuleDataStack.clear();
    m_ruleSourceDataResult = 0;
    m_rule = 0;
    m_ignoreErrorsInDeclaration = false;
    m_logErrors = false;
}

PassRefPtr<StyleRuleBase> CSSParser::parseRule(StyleSheetContents* sheet, const String& string)
{
    setStyleSheet(sheet);
    m_allowNamespaceDeclarations = false;
    setupParser("@-webkit-rule{", string, "} ");
    cssyyparse(this);
    return m_rule.release();
}

PassRefPtr<StyleKeyframe> CSSParser::parseKeyframeRule(StyleSheetContents* sheet, const String& string)
{
    setStyleSheet(sheet);
    setupParser("@-webkit-keyframe-rule{ ", string, "} ");
    cssyyparse(this);
    return m_keyframe.release();
}

#if ENABLE(CSS3_CONDITIONAL_RULES)
bool CSSParser::parseSupportsCondition(const String& string)
{
    m_supportsCondition = false;
    setupParser("@-webkit-supports-condition{ ", string, "} ");
    cssyyparse(this);
    return m_supportsCondition;
}
#endif

static inline bool isColorPropertyID(CSSPropertyID propertyId)
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
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationColor:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextStrokeColor:
        return true;
    default:
        return false;
    }
}

static bool parseColorValue(MutableStylePropertySet* declaration, CSSPropertyID propertyId, const String& string, bool important, CSSParserMode cssParserMode)
{
    ASSERT(!string.isEmpty());
    bool strict = isStrictParserMode(cssParserMode);
    if (!isColorPropertyID(propertyId))
        return false;
    CSSParserString cssString;
    cssString.init(string);
    CSSValueID valueID = cssValueKeywordID(cssString);
    bool validPrimitive = false;
    if (valueID == CSSValueWebkitText)
        validPrimitive = true;
    else if (valueID == CSSValueCurrentcolor)
        validPrimitive = true;
    else if ((valueID >= CSSValueAqua && valueID <= CSSValueWindowtext) || valueID == CSSValueMenu
             || (valueID >= CSSValueWebkitFocusRingColor && valueID < CSSValueWebkitText && !strict)) {
        validPrimitive = true;
    }

    if (validPrimitive) {
        RefPtr<CSSValue> value = cssValuePool().createIdentifierValue(valueID);
        declaration->addParsedProperty(CSSProperty(propertyId, value.release(), important));
        return true;
    }
    RGBA32 color;
    if (!CSSParser::fastParseColor(color, string, strict && string[0] != '#'))
        return false;
    RefPtr<CSSValue> value = cssValuePool().createColorValue(color);
    declaration->addParsedProperty(CSSProperty(propertyId, value.release(), important));
    return true;
}

static inline bool isSimpleLengthPropertyID(CSSPropertyID propertyId, bool& acceptsNegativeNumbers)
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
#if ENABLE(CSS_SHAPES)
    case CSSPropertyWebkitShapeMargin:
    case CSSPropertyWebkitShapePadding:
        acceptsNegativeNumbers = false;
        return RuntimeEnabledFeatures::cssShapesEnabled();
#endif
    case CSSPropertyBottom:
    case CSSPropertyLeft:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft:
    case CSSPropertyMarginRight:
    case CSSPropertyMarginTop:
    case CSSPropertyRight:
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

template <typename CharacterType>
static inline bool parseSimpleLength(const CharacterType* characters, unsigned& length, CSSPrimitiveValue::UnitTypes& unit, double& number)
{
    if (length > 2 && (characters[length - 2] | 0x20) == 'p' && (characters[length - 1] | 0x20) == 'x') {
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
    number = charactersToDouble(characters, length, &ok);
    return ok;
}

static bool parseSimpleLengthValue(MutableStylePropertySet* declaration, CSSPropertyID propertyId, const String& string, bool important, CSSParserMode cssParserMode)
{
    ASSERT(!string.isEmpty());
    bool acceptsNegativeNumbers;
    if (!isSimpleLengthPropertyID(propertyId, acceptsNegativeNumbers))
        return false;

    unsigned length = string.length();
    double number;
    CSSPrimitiveValue::UnitTypes unit = CSSPrimitiveValue::CSS_NUMBER;

    if (string.is8Bit()) {
        if (!parseSimpleLength(string.characters8(), length, unit, number))
            return false;
    } else {
        if (!parseSimpleLength(string.characters16(), length, unit, number))
            return false;
    }

    if (unit == CSSPrimitiveValue::CSS_NUMBER) {
        if (number && isStrictParserMode(cssParserMode))
            return false;
        unit = CSSPrimitiveValue::CSS_PX;
    }
    if (number < 0 && !acceptsNegativeNumbers)
        return false;

    RefPtr<CSSValue> value = cssValuePool().createValue(number, unit);
    declaration->addParsedProperty(CSSProperty(propertyId, value.release(), important));
    return true;
}

static inline bool isValidKeywordPropertyAndValue(CSSPropertyID propertyId, int valueID, const CSSParserContext& parserContext)
{
    if (!valueID)
        return false;

    switch (propertyId) {
    case CSSPropertyBorderCollapse: // collapse | separate | inherit
        if (valueID == CSSValueCollapse || valueID == CSSValueSeparate)
            return true;
        break;
    case CSSPropertyBorderTopStyle: // <border-style> | inherit
    case CSSPropertyBorderRightStyle: // Defined as: none | hidden | dotted | dashed |
    case CSSPropertyBorderBottomStyle: // solid | double | groove | ridge | inset | outset
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitColumnRuleStyle:
        if (valueID >= CSSValueNone && valueID <= CSSValueDouble)
            return true;
        break;
    case CSSPropertyBoxSizing:
         if (valueID == CSSValueBorderBox || valueID == CSSValueContentBox)
             return true;
         break;
    case CSSPropertyCaptionSide: // top | bottom | left | right | inherit
        if (valueID == CSSValueLeft || valueID == CSSValueRight || valueID == CSSValueTop || valueID == CSSValueBottom)
            return true;
        break;
    case CSSPropertyClear: // none | left | right | both | inherit
        if (valueID == CSSValueNone || valueID == CSSValueLeft || valueID == CSSValueRight || valueID == CSSValueBoth)
            return true;
        break;
    case CSSPropertyDirection: // ltr | rtl | inherit
        if (valueID == CSSValueLtr || valueID == CSSValueRtl)
            return true;
        break;
    case CSSPropertyDisplay:
        // inline | block | list-item | run-in | inline-block | table |
        // inline-table | table-row-group | table-header-group | table-footer-group | table-row |
        // table-column-group | table-column | table-cell | table-caption | -webkit-box | -webkit-inline-box | none | inherit
        // -webkit-flex | -webkit-inline-flex | -webkit-grid | -webkit-inline-grid
        if ((valueID >= CSSValueInline && valueID <= CSSValueWebkitInlineFlex) || valueID == CSSValueNone)
            return true;
        if (parserContext.isCSSGridLayoutEnabled && (valueID == CSSValueWebkitGrid || valueID == CSSValueWebkitInlineGrid))
            return true;
        break;

    case CSSPropertyEmptyCells: // show | hide | inherit
        if (valueID == CSSValueShow || valueID == CSSValueHide)
            return true;
        break;
    case CSSPropertyFloat: // left | right | none | center (for buggy CSS, maps to none)
        if (valueID == CSSValueLeft || valueID == CSSValueRight || valueID == CSSValueNone || valueID == CSSValueCenter)
            return true;
        break;
    case CSSPropertyFontStyle: // normal | italic | oblique | inherit
        if (valueID == CSSValueNormal || valueID == CSSValueItalic || valueID == CSSValueOblique)
            return true;
        break;
    case CSSPropertyImageRendering: // auto | optimizeSpeed | optimizeQuality | -webkit-crisp-edges | -webkit-optimize-contrast
        if (valueID == CSSValueAuto || valueID == CSSValueOptimizespeed || valueID == CSSValueOptimizequality
            || valueID == CSSValueWebkitCrispEdges || valueID == CSSValueWebkitOptimizeContrast)
            return true;
        break;
    case CSSPropertyListStylePosition: // inside | outside | inherit
        if (valueID == CSSValueInside || valueID == CSSValueOutside)
            return true;
        break;
    case CSSPropertyListStyleType:
        // See section CSS_PROP_LIST_STYLE_TYPE of file CSSValueKeywords.in
        // for the list of supported list-style-types.
        if ((valueID >= CSSValueDisc && valueID <= CSSValueKatakanaIroha) || valueID == CSSValueNone)
            return true;
        break;
    case CSSPropertyOutlineStyle: // (<border-style> except hidden) | auto | inherit
        if (valueID == CSSValueAuto || valueID == CSSValueNone || (valueID >= CSSValueInset && valueID <= CSSValueDouble))
            return true;
        break;
    case CSSPropertyOverflowWrap: // normal | break-word
    case CSSPropertyWordWrap:
        if (valueID == CSSValueNormal || valueID == CSSValueBreakWord)
            return true;
        break;
    case CSSPropertyOverflowX: // visible | hidden | scroll | auto | marquee | overlay | inherit
        if (valueID == CSSValueVisible || valueID == CSSValueHidden || valueID == CSSValueScroll || valueID == CSSValueAuto || valueID == CSSValueOverlay || valueID == CSSValueWebkitMarquee)
            return true;
        break;
    case CSSPropertyOverflowY: // visible | hidden | scroll | auto | marquee | overlay | inherit | -webkit-paged-x | -webkit-paged-y
        if (valueID == CSSValueVisible || valueID == CSSValueHidden || valueID == CSSValueScroll || valueID == CSSValueAuto || valueID == CSSValueOverlay || valueID == CSSValueWebkitMarquee || valueID == CSSValueWebkitPagedX || valueID == CSSValueWebkitPagedY)
            return true;
        break;
    case CSSPropertyPageBreakAfter: // auto | always | avoid | left | right | inherit
    case CSSPropertyPageBreakBefore:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
        if (valueID == CSSValueAuto || valueID == CSSValueAlways || valueID == CSSValueAvoid || valueID == CSSValueLeft || valueID == CSSValueRight)
            return true;
        break;
    case CSSPropertyPageBreakInside: // avoid | auto | inherit
    case CSSPropertyWebkitColumnBreakInside:
        if (valueID == CSSValueAuto || valueID == CSSValueAvoid)
            return true;
        break;
    case CSSPropertyPointerEvents:
        // none | visiblePainted | visibleFill | visibleStroke | visible |
        // painted | fill | stroke | auto | all | inherit
        if (valueID == CSSValueVisible || valueID == CSSValueNone || valueID == CSSValueAll || valueID == CSSValueAuto || (valueID >= CSSValueVisiblepainted && valueID <= CSSValueStroke))
            return true;
        break;
    case CSSPropertyPosition: // static | relative | absolute | fixed | sticky | inherit
        if (valueID == CSSValueStatic || valueID == CSSValueRelative || valueID == CSSValueAbsolute || valueID == CSSValueFixed
#if ENABLE(CSS_STICKY_POSITION)
            || (parserContext.isCSSStickyPositionEnabled && valueID == CSSValueWebkitSticky)
#endif
            )
            return true;
        break;
    case CSSPropertyResize: // none | both | horizontal | vertical | auto
        if (valueID == CSSValueNone || valueID == CSSValueBoth || valueID == CSSValueHorizontal || valueID == CSSValueVertical || valueID == CSSValueAuto)
            return true;
        break;
    case CSSPropertySpeak: // none | normal | spell-out | digits | literal-punctuation | no-punctuation | inherit
        if (valueID == CSSValueNone || valueID == CSSValueNormal || valueID == CSSValueSpellOut || valueID == CSSValueDigits || valueID == CSSValueLiteralPunctuation || valueID == CSSValueNoPunctuation)
            return true;
        break;
    case CSSPropertyTableLayout: // auto | fixed | inherit
        if (valueID == CSSValueAuto || valueID == CSSValueFixed)
            return true;
        break;
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextUnderlineMode:
        if (valueID == CSSValueContinuous || valueID == CSSValueSkipWhiteSpace)
            return true;
        break;
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextUnderlineStyle:
        if (valueID == CSSValueNone || valueID == CSSValueSolid || valueID == CSSValueDouble || valueID == CSSValueDashed || valueID == CSSValueDotDash || valueID == CSSValueDotDotDash || valueID == CSSValueWave)
            return true;
        break;
    case CSSPropertyTextOverflow: // clip | ellipsis
        if (valueID == CSSValueClip || valueID == CSSValueEllipsis)
            return true;
        break;
    case CSSPropertyTextRendering: // auto | optimizeSpeed | optimizeLegibility | geometricPrecision
        if (valueID == CSSValueAuto || valueID == CSSValueOptimizespeed || valueID == CSSValueOptimizelegibility || valueID == CSSValueGeometricprecision)
            return true;
        break;
    case CSSPropertyTextTransform: // capitalize | uppercase | lowercase | none | inherit
        if ((valueID >= CSSValueCapitalize && valueID <= CSSValueLowercase) || valueID == CSSValueNone)
            return true;
        break;
    case CSSPropertyVisibility: // visible | hidden | collapse | inherit
        if (valueID == CSSValueVisible || valueID == CSSValueHidden || valueID == CSSValueCollapse)
            return true;
        break;
    case CSSPropertyWebkitAppearance:
        if ((valueID >= CSSValueCheckbox && valueID <= CSSValueTextarea) || valueID == CSSValueNone)
            return true;
        break;
    case CSSPropertyWebkitBackfaceVisibility:
        if (valueID == CSSValueVisible || valueID == CSSValueHidden)
            return true;
        break;
#if ENABLE(CSS_COMPOSITING)
    case CSSPropertyWebkitBlendMode:
        if (parserContext.isCSSCompositingEnabled && (valueID == CSSValueNormal || valueID == CSSValueMultiply || valueID == CSSValueScreen
            || valueID == CSSValueOverlay || valueID == CSSValueDarken || valueID == CSSValueLighten ||  valueID == CSSValueColorDodge
            || valueID == CSSValueColorBurn || valueID == CSSValueHardLight || valueID == CSSValueSoftLight || valueID == CSSValueDifference
            || valueID == CSSValueExclusion || valueID == CSSValueHue || valueID == CSSValueSaturation || valueID == CSSValueColor
            || valueID == CSSValueLuminosity))
            return true;
        break;
#endif
    case CSSPropertyWebkitBorderFit:
        if (valueID == CSSValueBorder || valueID == CSSValueLines)
            return true;
        break;
    case CSSPropertyWebkitBoxAlign:
        if (valueID == CSSValueStretch || valueID == CSSValueStart || valueID == CSSValueEnd || valueID == CSSValueCenter || valueID == CSSValueBaseline)
            return true;
        break;
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    case CSSPropertyWebkitBoxDecorationBreak:
         if (valueID == CSSValueClone || valueID == CSSValueSlice)
             return true;
         break;
#endif
    case CSSPropertyWebkitBoxDirection:
        if (valueID == CSSValueNormal || valueID == CSSValueReverse)
            return true;
        break;
    case CSSPropertyWebkitBoxLines:
        if (valueID == CSSValueSingle || valueID == CSSValueMultiple)
                return true;
        break;
    case CSSPropertyWebkitBoxOrient:
        if (valueID == CSSValueHorizontal || valueID == CSSValueVertical || valueID == CSSValueInlineAxis || valueID == CSSValueBlockAxis)
            return true;
        break;
    case CSSPropertyWebkitBoxPack:
        if (valueID == CSSValueStart || valueID == CSSValueEnd || valueID == CSSValueCenter || valueID == CSSValueJustify)
            return true;
        break;
    case CSSPropertyWebkitColorCorrection:
        if (valueID == CSSValueSrgb || valueID == CSSValueDefault)
            return true;
        break;
    case CSSPropertyWebkitAlignContent:
         if (valueID == CSSValueFlexStart || valueID == CSSValueFlexEnd || valueID == CSSValueCenter || valueID == CSSValueSpaceBetween || valueID == CSSValueSpaceAround || valueID == CSSValueStretch)
             return true;
         break;
    case CSSPropertyWebkitAlignItems:
        if (valueID == CSSValueFlexStart || valueID == CSSValueFlexEnd || valueID == CSSValueCenter || valueID == CSSValueBaseline || valueID == CSSValueStretch)
            return true;
        break;
    case CSSPropertyWebkitAlignSelf:
        if (valueID == CSSValueAuto || valueID == CSSValueFlexStart || valueID == CSSValueFlexEnd || valueID == CSSValueCenter || valueID == CSSValueBaseline || valueID == CSSValueStretch)
            return true;
        break;
    case CSSPropertyWebkitFlexDirection:
        if (valueID == CSSValueRow || valueID == CSSValueRowReverse || valueID == CSSValueColumn || valueID == CSSValueColumnReverse)
            return true;
        break;
    case CSSPropertyWebkitFlexWrap:
        if (valueID == CSSValueNowrap || valueID == CSSValueWrap || valueID == CSSValueWrapReverse)
             return true;
        break;
    case CSSPropertyWebkitJustifyContent:
        if (valueID == CSSValueFlexStart || valueID == CSSValueFlexEnd || valueID == CSSValueCenter || valueID == CSSValueSpaceBetween || valueID == CSSValueSpaceAround)
            return true;
        break;
    case CSSPropertyWebkitFontKerning:
        if (valueID == CSSValueAuto || valueID == CSSValueNormal || valueID == CSSValueNone)
            return true;
        break;
    case CSSPropertyWebkitFontSmoothing:
        if (valueID == CSSValueAuto || valueID == CSSValueNone || valueID == CSSValueAntialiased || valueID == CSSValueSubpixelAntialiased)
            return true;
        break;
    case CSSPropertyWebkitHyphens:
        if (valueID == CSSValueNone || valueID == CSSValueManual || valueID == CSSValueAuto)
            return true;
        break;
    case CSSPropertyWebkitGridAutoFlow:
        if (valueID == CSSValueNone || valueID == CSSValueRow || valueID == CSSValueColumn)
            return true;
        break;
    case CSSPropertyWebkitLineAlign:
        if (valueID == CSSValueNone || valueID == CSSValueEdges)
            return true;
        break;
    case CSSPropertyWebkitLineBreak: // auto | loose | normal | strict | after-white-space
        if (valueID == CSSValueAuto || valueID == CSSValueLoose || valueID == CSSValueNormal || valueID == CSSValueStrict || valueID == CSSValueAfterWhiteSpace)
            return true;
        break;
    case CSSPropertyWebkitLineSnap:
        if (valueID == CSSValueNone || valueID == CSSValueBaseline || valueID == CSSValueContain)
            return true;
        break;
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
        if (valueID == CSSValueCollapse || valueID == CSSValueSeparate || valueID == CSSValueDiscard)
            return true;
        break;
    case CSSPropertyWebkitMarqueeDirection:
        if (valueID == CSSValueForwards || valueID == CSSValueBackwards || valueID == CSSValueAhead || valueID == CSSValueReverse || valueID == CSSValueLeft || valueID == CSSValueRight || valueID == CSSValueDown
            || valueID == CSSValueUp || valueID == CSSValueAuto)
            return true;
        break;
    case CSSPropertyWebkitMarqueeStyle:
        if (valueID == CSSValueNone || valueID == CSSValueSlide || valueID == CSSValueScroll || valueID == CSSValueAlternate)
            return true;
        break;
    case CSSPropertyWebkitNbspMode: // normal | space
        if (valueID == CSSValueNormal || valueID == CSSValueSpace)
            return true;
        break;
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling:
        if (valueID == CSSValueAuto || valueID == CSSValueTouch)
            return true;
        break;
#endif
    case CSSPropertyWebkitPrintColorAdjust:
        if (valueID == CSSValueExact || valueID == CSSValueEconomy)
            return true;
        break;
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
        if (parserContext.isCSSRegionsEnabled && (valueID == CSSValueAuto || valueID == CSSValueAlways || valueID == CSSValueAvoid || valueID == CSSValueLeft || valueID == CSSValueRight))
            return true;
        break;
    case CSSPropertyWebkitRegionBreakInside:
        if (parserContext.isCSSRegionsEnabled && (valueID == CSSValueAuto || valueID == CSSValueAvoid))
            return true;
        break;
    case CSSPropertyWebkitRegionFragment:
        if (parserContext.isCSSRegionsEnabled && (valueID == CSSValueAuto || valueID == CSSValueBreak))
            return true;
        break;
#endif
    case CSSPropertyWebkitRtlOrdering:
        if (valueID == CSSValueLogical || valueID == CSSValueVisual)
            return true;
        break;

    case CSSPropertyWebkitRubyPosition:
        if (valueID == CSSValueBefore || valueID == CSSValueAfter)
            return true;
        break;

#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextAlignLast:
        // auto | start | end | left | right | center | justify
        if ((valueID >= CSSValueLeft && valueID <= CSSValueJustify) || valueID == CSSValueStart || valueID == CSSValueEnd || valueID == CSSValueAuto)
            return true;
        break;
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextCombine:
        if (valueID == CSSValueNone || valueID == CSSValueHorizontal)
            return true;
        break;
    case CSSPropertyWebkitTextEmphasisPosition:
        if (valueID == CSSValueOver || valueID == CSSValueUnder)
            return true;
        break;
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextJustify:
        // auto | none | inter-word | inter-ideograph | inter-cluster | distribute | kashida
        if ((valueID >= CSSValueInterWord && valueID <= CSSValueKashida) || valueID == CSSValueAuto || valueID == CSSValueNone)
            return true;
        break;
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextSecurity:
        // disc | circle | square | none | inherit
        if (valueID == CSSValueDisc || valueID == CSSValueCircle || valueID == CSSValueSquare || valueID == CSSValueNone)
            return true;
        break;
    case CSSPropertyWebkitTransformStyle:
        if (valueID == CSSValueFlat || valueID == CSSValuePreserve3d)
            return true;
        break;
    case CSSPropertyWebkitUserDrag: // auto | none | element
        if (valueID == CSSValueAuto || valueID == CSSValueNone || valueID == CSSValueElement)
            return true;
        break;
    case CSSPropertyWebkitUserModify: // read-only | read-write
        if (valueID == CSSValueReadOnly || valueID == CSSValueReadWrite || valueID == CSSValueReadWritePlaintextOnly)
            return true;
        break;
    case CSSPropertyWebkitUserSelect: // auto | none | text | all
        if (valueID == CSSValueAuto || valueID == CSSValueNone || valueID == CSSValueText || valueID == CSSValueAll)
            return true;
        break;
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrapFlow:
        if (!RuntimeEnabledFeatures::cssExclusionsEnabled())
            return false;
        if (valueID == CSSValueAuto || valueID == CSSValueBoth || valueID == CSSValueStart || valueID == CSSValueEnd || valueID == CSSValueMaximum || valueID == CSSValueClear)
            return true;
        break;
    case CSSPropertyWebkitWrapThrough:
        if (!RuntimeEnabledFeatures::cssExclusionsEnabled())
            return false;
        if (valueID == CSSValueWrap || valueID == CSSValueNone)
            return true;
        break;
#endif
    case CSSPropertyWebkitWritingMode:
        if (valueID >= CSSValueHorizontalTb && valueID <= CSSValueHorizontalBt)
            return true;
        break;
    case CSSPropertyWhiteSpace: // normal | pre | nowrap | inherit
        if (valueID == CSSValueNormal || valueID == CSSValuePre || valueID == CSSValuePreWrap || valueID == CSSValuePreLine || valueID == CSSValueNowrap)
            return true;
        break;
    case CSSPropertyWordBreak: // normal | break-all | break-word (this is a custom extension)
        if (valueID == CSSValueNormal || valueID == CSSValueBreakAll || valueID == CSSValueBreakWord)
            return true;
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
    return false;
}

static inline bool isKeywordPropertyID(CSSPropertyID propertyId)
{
    switch (propertyId) {
    case CSSPropertyBorderBottomStyle:
    case CSSPropertyBorderCollapse:
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyBorderRightStyle:
    case CSSPropertyBorderTopStyle:
    case CSSPropertyBoxSizing:
    case CSSPropertyCaptionSide:
    case CSSPropertyClear:
    case CSSPropertyDirection:
    case CSSPropertyDisplay:
    case CSSPropertyEmptyCells:
    case CSSPropertyFloat:
    case CSSPropertyFontStyle:
    case CSSPropertyImageRendering:
    case CSSPropertyListStylePosition:
    case CSSPropertyListStyleType:
    case CSSPropertyOutlineStyle:
    case CSSPropertyOverflowWrap:
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyPointerEvents:
    case CSSPropertyPosition:
    case CSSPropertyResize:
    case CSSPropertySpeak:
    case CSSPropertyTableLayout:
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextOverflow:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextRendering:
    case CSSPropertyTextTransform:
    case CSSPropertyTextUnderlineMode:
    case CSSPropertyTextUnderlineStyle:
    case CSSPropertyVisibility:
    case CSSPropertyWebkitAppearance:
#if ENABLE(CSS_COMPOSITING)
    case CSSPropertyWebkitBlendMode:
#endif
    case CSSPropertyWebkitBackfaceVisibility:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderFit:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBoxAlign:
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    case CSSPropertyWebkitBoxDecorationBreak:
#endif
    case CSSPropertyWebkitBoxDirection:
    case CSSPropertyWebkitBoxLines:
    case CSSPropertyWebkitBoxOrient:
    case CSSPropertyWebkitBoxPack:
    case CSSPropertyWebkitColorCorrection:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitColumnRuleStyle:
    case CSSPropertyWebkitAlignContent:
    case CSSPropertyWebkitAlignItems:
    case CSSPropertyWebkitAlignSelf:
    case CSSPropertyWebkitFlexDirection:
    case CSSPropertyWebkitFlexWrap:
    case CSSPropertyWebkitJustifyContent:
    case CSSPropertyWebkitFontKerning:
    case CSSPropertyWebkitFontSmoothing:
    case CSSPropertyWebkitHyphens:
    case CSSPropertyWebkitGridAutoFlow:
    case CSSPropertyWebkitLineAlign:
    case CSSPropertyWebkitLineBreak:
    case CSSPropertyWebkitLineSnap:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarqueeDirection:
    case CSSPropertyWebkitMarqueeStyle:
    case CSSPropertyWebkitNbspMode:
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling:
#endif
    case CSSPropertyWebkitPrintColorAdjust:
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
    case CSSPropertyWebkitRegionBreakInside:
    case CSSPropertyWebkitRegionFragment:
#endif
    case CSSPropertyWebkitRtlOrdering:
    case CSSPropertyWebkitRubyPosition:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextAlignLast:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextCombine:
    case CSSPropertyWebkitTextEmphasisPosition:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextJustify:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextSecurity:
    case CSSPropertyWebkitTransformStyle:
    case CSSPropertyWebkitUserDrag:
    case CSSPropertyWebkitUserModify:
    case CSSPropertyWebkitUserSelect:
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrapFlow:
    case CSSPropertyWebkitWrapThrough:
#endif
    case CSSPropertyWebkitWritingMode:
    case CSSPropertyWhiteSpace:
    case CSSPropertyWordBreak:
    case CSSPropertyWordWrap:
        return true;
    default:
        return false;
    }
}

static bool parseKeywordValue(MutableStylePropertySet* declaration, CSSPropertyID propertyId, const String& string, bool important, const CSSParserContext& parserContext)
{
    ASSERT(!string.isEmpty());

    if (!isKeywordPropertyID(propertyId)) {
        // All properties accept the values of "initial" and "inherit".
        String lowerCaseString = string.lower();
        if (lowerCaseString != "initial" && lowerCaseString != "inherit")
            return false;

        // Parse initial/inherit shorthands using the CSSParser.
        if (shorthandForProperty(propertyId).length())
            return false;
    }

    CSSParserString cssString;
    cssString.init(string);
    CSSValueID valueID = cssValueKeywordID(cssString);

    if (!valueID)
        return false;

    RefPtr<CSSValue> value;
    if (valueID == CSSValueInherit)
        value = cssValuePool().createInheritedValue();
    else if (valueID == CSSValueInitial)
        value = cssValuePool().createExplicitInitialValue();
    else if (isValidKeywordPropertyAndValue(propertyId, valueID, parserContext))
        value = cssValuePool().createIdentifierValue(valueID);
    else
        return false;

    declaration->addParsedProperty(CSSProperty(propertyId, value.release(), important));
    return true;
}

template <typename CharacterType>
static bool parseTransformArguments(WebKitCSSTransformValue* transformValue, CharacterType* characters, unsigned length, unsigned start, unsigned expectedCount)
{
    while (expectedCount) {
        size_t end = WTF::find(characters, length, expectedCount == 1 ? ')' : ',', start);
        if (end == notFound || (expectedCount == 1 && end != length - 1))
            return false;
        unsigned argumentLength = end - start;
        CSSPrimitiveValue::UnitTypes unit = CSSPrimitiveValue::CSS_NUMBER;
        double number;
        if (!parseSimpleLength(characters + start, argumentLength, unit, number))
            return false;
        if (unit != CSSPrimitiveValue::CSS_PX && (number || unit != CSSPrimitiveValue::CSS_NUMBER))
            return false;
        transformValue->append(cssValuePool().createValue(number, unit));
        start = end + 1;
        --expectedCount;
    }
    return true;
}

static bool parseTranslateTransformValue(MutableStylePropertySet* properties, CSSPropertyID propertyID, const String& string, bool important)
{
    if (propertyID != CSSPropertyWebkitTransform)
        return false;
    static const unsigned shortestValidTransformStringLength = 12;
    static const unsigned likelyMultipartTransformStringLengthCutoff = 32;
    if (string.length() < shortestValidTransformStringLength || string.length() > likelyMultipartTransformStringLengthCutoff)
        return false;
    if (!string.startsWith("translate", false))
        return false;
    UChar c9 = toASCIILower(string[9]);
    UChar c10 = toASCIILower(string[10]);

    WebKitCSSTransformValue::TransformOperationType transformType;
    unsigned expectedArgumentCount = 1;
    unsigned argumentStart = 11;
    if (c9 == 'x' && c10 == '(')
        transformType = WebKitCSSTransformValue::TranslateXTransformOperation;
    else if (c9 == 'y' && c10 == '(')
        transformType = WebKitCSSTransformValue::TranslateYTransformOperation;
    else if (c9 == 'z' && c10 == '(')
        transformType = WebKitCSSTransformValue::TranslateZTransformOperation;
    else if (c9 == '(') {
        transformType = WebKitCSSTransformValue::TranslateTransformOperation;
        expectedArgumentCount = 2;
        argumentStart = 10;
    } else if (c9 == '3' && c10 == 'd' && string[11] == '(') {
        transformType = WebKitCSSTransformValue::Translate3DTransformOperation;
        expectedArgumentCount = 3;
        argumentStart = 12;
    } else
        return false;

    RefPtr<WebKitCSSTransformValue> transformValue = WebKitCSSTransformValue::create(transformType);
    bool success;
    if (string.is8Bit())
        success = parseTransformArguments(transformValue.get(), string.characters8(), string.length(), argumentStart, expectedArgumentCount);
    else
        success = parseTransformArguments(transformValue.get(), string.characters16(), string.length(), argumentStart, expectedArgumentCount);
    if (!success)
        return false;
    RefPtr<CSSValueList> result = CSSValueList::createSpaceSeparated();
    result->append(transformValue.release());
    properties->addParsedProperty(CSSProperty(CSSPropertyWebkitTransform, result.release(), important));
    return true;
}

PassRefPtr<CSSValueList> CSSParser::parseFontFaceValue(const AtomicString& string)
{
    if (string.isEmpty())
        return 0;
    RefPtr<MutableStylePropertySet> dummyStyle = MutableStylePropertySet::create();
    if (!parseValue(dummyStyle.get(), CSSPropertyFontFamily, string, false, CSSQuirksMode, 0))
        return 0;
    return static_pointer_cast<CSSValueList>(dummyStyle->getPropertyCSSValue(CSSPropertyFontFamily));
}

#if ENABLE(CSS_VARIABLES)
bool CSSParser::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, Document* document)
{
    ASSERT(!string.isEmpty());

    CSSParserContext context(document);

    if (parseSimpleLengthValue(declaration, propertyID, string, important, context.mode))
        return true;
    if (parseColorValue(declaration, propertyID, string, important, context.mode))
        return true;
    if (parseKeywordValue(declaration, propertyID, string, important, context))
        return true;

    CSSParser parser(context);
    return parser.parseValue(declaration, propertyID, string, important, static_cast<StyleSheetContents*>(0));
}
#endif

bool CSSParser::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, CSSParserMode cssParserMode, StyleSheetContents* contextStyleSheet)
{
    ASSERT(!string.isEmpty());
    if (parseSimpleLengthValue(declaration, propertyID, string, important, cssParserMode))
        return true;
    if (parseColorValue(declaration, propertyID, string, important, cssParserMode))
        return true;

    CSSParserContext context(cssParserMode);
    if (contextStyleSheet) {
        context = contextStyleSheet->parserContext();
        context.mode = cssParserMode;
    }

    if (parseKeywordValue(declaration, propertyID, string, important, context))
        return true;
    if (parseTranslateTransformValue(declaration, propertyID, string, important))
        return true;

    CSSParser parser(context);
    return parser.parseValue(declaration, propertyID, string, important, contextStyleSheet);
}

bool CSSParser::parseValue(MutableStylePropertySet* declaration, CSSPropertyID propertyID, const String& string, bool important, StyleSheetContents* contextStyleSheet)
{
    setStyleSheet(contextStyleSheet);

    setupParser("@-webkit-value{", string, "} ");

    m_id = propertyID;
    m_important = important;

    cssyyparse(this);

    m_rule = 0;

    bool ok = false;
    if (m_hasFontFaceOnlyValues)
        deleteFontFaceOnlyValues();
    if (!m_parsedProperties.isEmpty()) {
        ok = true;
        declaration->addParsedProperties(m_parsedProperties);
        clearProperties();
    }

    return ok;
}

// The color will only be changed when string contains a valid CSS color, so callers
// can set it to a default color and ignore the boolean result.
bool CSSParser::parseColor(RGBA32& color, const String& string, bool strict)
{
    // First try creating a color specified by name, rgba(), rgb() or "#" syntax.
    if (fastParseColor(color, string, strict))
        return true;

    CSSParser parser(CSSStrictMode);

    // In case the fast-path parser didn't understand the color, try the full parser.
    if (!parser.parseColor(string))
        return false;

    CSSValue* value = parser.m_parsedProperties.first().value();
    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (!primitiveValue->isRGBColor())
        return false;

    color = primitiveValue->getRGBA32Value();
    return true;
}

bool CSSParser::parseColor(const String& string)
{
    setupParser("@-webkit-decls{color:", string, "} ");
    cssyyparse(this);
    m_rule = 0;

    return !m_parsedProperties.isEmpty() && m_parsedProperties.first().id() == CSSPropertyColor;
}

bool CSSParser::parseSystemColor(RGBA32& color, const String& string, Document* document)
{
    if (!document || !document->page())
        return false;

    CSSParserString cssColor;
    cssColor.init(string);
    CSSValueID id = cssValueKeywordID(cssColor);
    if (id <= 0)
        return false;

    color = document->page()->theme()->systemColor(id).rgb();
    return true;
}

void CSSParser::parseSelector(const String& string, CSSSelectorList& selectorList)
{
    m_selectorListForParseSelector = &selectorList;

    setupParser("@-webkit-selector{", string, "}");

    cssyyparse(this);

    m_selectorListForParseSelector = 0;
}

PassRefPtr<ImmutableStylePropertySet> CSSParser::parseInlineStyleDeclaration(const String& string, Element* element)
{
    CSSParserContext context = element->document()->elementSheet()->contents()->parserContext();
    context.mode = strictToCSSParserMode(element->isHTMLElement() && !element->document()->inQuirksMode());
    return CSSParser(context).parseDeclaration(string, element->document()->elementSheet()->contents());
}

PassRefPtr<ImmutableStylePropertySet> CSSParser::parseDeclaration(const String& string, StyleSheetContents* contextStyleSheet)
{
    setStyleSheet(contextStyleSheet);

    setupParser("@-webkit-decls{", string, "} ");
    cssyyparse(this);
    m_rule = 0;

    if (m_hasFontFaceOnlyValues)
        deleteFontFaceOnlyValues();

    RefPtr<ImmutableStylePropertySet> style = createStylePropertySet();
    clearProperties();
    return style.release();
}


bool CSSParser::parseDeclaration(MutableStylePropertySet* declaration, const String& string, PassRefPtr<CSSRuleSourceData> prpRuleSourceData, StyleSheetContents* contextStyleSheet)
{
    // Length of the "@-webkit-decls{" prefix.
    static const unsigned prefixLength = 15;

    setStyleSheet(contextStyleSheet);

    RefPtr<CSSRuleSourceData> ruleSourceData = prpRuleSourceData;
    if (ruleSourceData) {
        m_currentRuleDataStack = adoptPtr(new RuleSourceDataList());
        m_currentRuleDataStack->append(ruleSourceData);
    }

    setupParser("@-webkit-decls{", string, "} ");
    cssyyparse(this);
    m_rule = 0;

    bool ok = false;
    if (m_hasFontFaceOnlyValues)
        deleteFontFaceOnlyValues();
    if (!m_parsedProperties.isEmpty()) {
        ok = true;
        declaration->addParsedProperties(m_parsedProperties);
        clearProperties();
    }

    if (ruleSourceData) {
        ASSERT(m_currentRuleDataStack->size() == 1);
        ruleSourceData->ruleBodyRange.start = 0;
        ruleSourceData->ruleBodyRange.end = string.length();
        for (size_t i = 0, size = ruleSourceData->styleSourceData->propertyData.size(); i < size; ++i) {
            CSSPropertySourceData& propertyData = ruleSourceData->styleSourceData->propertyData.at(i);
            propertyData.range.start -= prefixLength;
            propertyData.range.end -= prefixLength;
        }

        fixUnparsedPropertyRanges(ruleSourceData.get());
        m_currentRuleDataStack.clear();
    }

    return ok;
}

PassOwnPtr<MediaQuery> CSSParser::parseMediaQuery(const String& string)
{
    if (string.isEmpty())
        return nullptr;

    ASSERT(!m_mediaQuery);

    // can't use { because tokenizer state switches from mediaquery to initial state when it sees { token.
    // instead insert one " " (which is WHITESPACE in CSSGrammar.y)
    setupParser("@-webkit-mediaquery ", string, "} ");
    cssyyparse(this);

    return m_mediaQuery.release();
}

#if ENABLE(CSS_VARIABLES)
static inline void filterProperties(bool important, const CSSParser::ParsedPropertyVector& input, Vector<CSSProperty, 256>& output, size_t& unusedEntries, BitArray<numCSSProperties>& seenProperties, HashSet<AtomicString>& seenVariables)
#else
static inline void filterProperties(bool important, const CSSParser::ParsedPropertyVector& input, Vector<CSSProperty, 256>& output, size_t& unusedEntries, BitArray<numCSSProperties>& seenProperties)
#endif
{
    // Add properties in reverse order so that highest priority definitions are reached first. Duplicate definitions can then be ignored when found.
    for (int i = input.size() - 1; i >= 0; --i) {
        const CSSProperty& property = input[i];
        if (property.isImportant() != important)
            continue;
#if ENABLE(CSS_VARIABLES)
        if (property.id() == CSSPropertyVariable) {
            const AtomicString& name = static_cast<CSSVariableValue*>(property.value())->name();
            if (seenVariables.contains(name))
                continue;
            seenVariables.add(name);
            output[--unusedEntries] = property;
            continue;
        }
#endif
        const unsigned propertyIDIndex = property.id() - firstCSSProperty;
        if (seenProperties.get(propertyIDIndex))
            continue;
        seenProperties.set(propertyIDIndex);
        output[--unusedEntries] = property;
    }
}

PassRefPtr<ImmutableStylePropertySet> CSSParser::createStylePropertySet()
{
    BitArray<numCSSProperties> seenProperties;
    size_t unusedEntries = m_parsedProperties.size();
    Vector<CSSProperty, 256> results(unusedEntries);

    // Important properties have higher priority, so add them first. Duplicate definitions can then be ignored when found.
#if ENABLE(CSS_VARIABLES)
    HashSet<AtomicString> seenVariables;
    filterProperties(true, m_parsedProperties, results, unusedEntries, seenProperties, seenVariables);
    filterProperties(false, m_parsedProperties, results, unusedEntries, seenProperties, seenVariables);
#else
    filterProperties(true, m_parsedProperties, results, unusedEntries, seenProperties);
    filterProperties(false, m_parsedProperties, results, unusedEntries, seenProperties);
#endif
    if (unusedEntries)
        results.remove(0, unusedEntries);

    return ImmutableStylePropertySet::create(results.data(), results.size(), m_context.mode);
}

void CSSParser::addPropertyWithPrefixingVariant(CSSPropertyID propId, PassRefPtr<CSSValue> value, bool important, bool implicit)
{
    RefPtr<CSSValue> val = value.get();
    addProperty(propId, value, important, implicit);

    CSSPropertyID prefixingVariant = prefixingVariantForPropertyId(propId);
    if (prefixingVariant == propId)
        return;
    addProperty(prefixingVariant, val.release(), important, implicit);
}

void CSSParser::addProperty(CSSPropertyID propId, PassRefPtr<CSSValue> value, bool important, bool implicit)
{
    m_parsedProperties.append(CSSProperty(propId, value, important, m_currentShorthand, m_implicitShorthand || implicit));
}

void CSSParser::rollbackLastProperties(int num)
{
    ASSERT(num >= 0);
    ASSERT(m_parsedProperties.size() >= static_cast<unsigned>(num));
    m_parsedProperties.shrink(m_parsedProperties.size() - num);
}

void CSSParser::clearProperties()
{
    m_parsedProperties.clear();
    m_numParsedPropertiesBeforeMarginBox = INVALID_NUM_PARSED_PROPERTIES;
    m_hasFontFaceOnlyValues = false;
}

KURL CSSParser::completeURL(const CSSParserContext& context, const String& url)
{
    if (url.isNull())
        return KURL();
    if (context.charset.isEmpty())
        return KURL(context.baseURL, url);
    return KURL(context.baseURL, url, context.charset);
}

KURL CSSParser::completeURL(const String& url) const
{
    return completeURL(m_context, url);
}

bool CSSParser::validCalculationUnit(CSSParserValue* value, Units unitflags, ReleaseParsedCalcValueCondition releaseCalc)
{
    bool mustBeNonNegative = unitflags & FNonNeg;

    if (!parseCalculation(value, mustBeNonNegative ? CalculationRangeNonNegative : CalculationRangeAll))
        return false;

    bool b = false;
    switch (m_parsedCalculation->category()) {
    case CalcLength:
        b = (unitflags & FLength);
        break;
    case CalcPercent:
        b = (unitflags & FPercent);
        if (b && mustBeNonNegative && m_parsedCalculation->isNegative())
            b = false;
        break;
    case CalcNumber:
        b = (unitflags & FNumber);
        if (!b && (unitflags & FInteger) && m_parsedCalculation->isInt())
            b = true;
        if (b && mustBeNonNegative && m_parsedCalculation->isNegative())
            b = false;
        break;
    case CalcPercentLength:
        b = (unitflags & FPercent) && (unitflags & FLength);
        break;
    case CalcPercentNumber:
        b = (unitflags & FPercent) && (unitflags & FNumber);
        break;
#if ENABLE(CSS_VARIABLES)
    case CalcVariable:
        b = true;
        break;
#endif
    case CalcOther:
        break;
    }
    if (!b || releaseCalc == ReleaseParsedCalcValue)
        m_parsedCalculation.release();
    return b;    
}

inline bool CSSParser::shouldAcceptUnitLessValues(CSSParserValue* value, Units unitflags, CSSParserMode cssParserMode)
{
    // Qirks mode and svg presentation attributes accept unit less values.
    return (unitflags & (FLength | FAngle | FTime)) && (!value->fValue || cssParserMode == CSSQuirksMode || cssParserMode == SVGAttributeMode);
}

bool CSSParser::validUnit(CSSParserValue* value, Units unitflags, CSSParserMode cssParserMode, ReleaseParsedCalcValueCondition releaseCalc)
{
    if (isCalculation(value))
        return validCalculationUnit(value, unitflags, releaseCalc);
        
    bool b = false;
    switch (value->unit) {
#if ENABLE(CSS_VARIABLES)
    case CSSPrimitiveValue::CSS_VARIABLE_NAME:
        // Variables are checked at the point they are dereferenced because unit type is not available here.
        b = true;
        break;
#endif
    case CSSPrimitiveValue::CSS_NUMBER:
        b = (unitflags & FNumber);
        if (!b && shouldAcceptUnitLessValues(value, unitflags, cssParserMode)) {
            value->unit = (unitflags & FLength) ? CSSPrimitiveValue::CSS_PX :
                          ((unitflags & FAngle) ? CSSPrimitiveValue::CSS_DEG : CSSPrimitiveValue::CSS_MS);
            b = true;
        }
        if (!b && (unitflags & FInteger) && value->isInt)
            b = true;
        if (!b && (unitflags & FPositiveInteger) && value->isInt && value->fValue > 0)
            b = true;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        b = (unitflags & FPercent);
        break;
    case CSSParserValue::Q_EMS:
    case CSSPrimitiveValue::CSS_EMS:
    case CSSPrimitiveValue::CSS_REMS:
    case CSSPrimitiveValue::CSS_CHS:
    case CSSPrimitiveValue::CSS_EXS:
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PC:
    case CSSPrimitiveValue::CSS_VW:
    case CSSPrimitiveValue::CSS_VH:
    case CSSPrimitiveValue::CSS_VMIN:
    case CSSPrimitiveValue::CSS_VMAX:
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
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    case CSSPrimitiveValue::CSS_DPPX:
    case CSSPrimitiveValue::CSS_DPI:
    case CSSPrimitiveValue::CSS_DPCM:
        b = (unitflags & FResolution);
        break;
#endif
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

inline PassRefPtr<CSSPrimitiveValue> CSSParser::createPrimitiveNumericValue(CSSParserValue* value)
{
#if ENABLE(CSS_VARIABLES)
    if (value->unit == CSSPrimitiveValue::CSS_VARIABLE_NAME)
        return createPrimitiveVariableNameValue(value);
#endif

    if (m_parsedCalculation) {
        ASSERT(isCalculation(value));
        return CSSPrimitiveValue::create(m_parsedCalculation.release());
    }

#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    ASSERT((value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
        || (value->unit >= CSSPrimitiveValue::CSS_TURN && value->unit <= CSSPrimitiveValue::CSS_CHS)
        || (value->unit >= CSSPrimitiveValue::CSS_VW && value->unit <= CSSPrimitiveValue::CSS_VMAX)
        || (value->unit >= CSSPrimitiveValue::CSS_DPPX && value->unit <= CSSPrimitiveValue::CSS_DPCM));
#else
    ASSERT((value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
        || (value->unit >= CSSPrimitiveValue::CSS_TURN && value->unit <= CSSPrimitiveValue::CSS_CHS)
        || (value->unit >= CSSPrimitiveValue::CSS_VW && value->unit <= CSSPrimitiveValue::CSS_VMAX));
#endif
    return cssValuePool().createValue(value->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(value->unit));
}

inline PassRefPtr<CSSPrimitiveValue> CSSParser::createPrimitiveStringValue(CSSParserValue* value)
{
    ASSERT(value->unit == CSSPrimitiveValue::CSS_STRING || value->unit == CSSPrimitiveValue::CSS_IDENT);
    return cssValuePool().createValue(value->string, CSSPrimitiveValue::CSS_STRING);
}

#if ENABLE(CSS_VARIABLES)
inline PassRefPtr<CSSPrimitiveValue> CSSParser::createPrimitiveVariableNameValue(CSSParserValue* value)
{
    ASSERT(value->unit == CSSPrimitiveValue::CSS_VARIABLE_NAME);
    return CSSPrimitiveValue::create(value->string, CSSPrimitiveValue::CSS_VARIABLE_NAME);
}
#endif

static inline bool isComma(CSSParserValue* value)
{ 
    return value && value->unit == CSSParserValue::Operator && value->iValue == ','; 
}

static inline bool isForwardSlashOperator(CSSParserValue* value)
{
    ASSERT(value);
    return value->unit == CSSParserValue::Operator && value->iValue == '/';
}

bool CSSParser::validWidth(CSSParserValue* value)
{
    int id = value->id;
    if (id == CSSValueIntrinsic || id == CSSValueMinIntrinsic || id == CSSValueWebkitMinContent || id == CSSValueWebkitMaxContent || id == CSSValueWebkitFillAvailable || id == CSSValueWebkitFitContent)
        return true;
    return !id && validUnit(value, FLength | FPercent | FNonNeg);
}

// FIXME: Combine this with validWidth when we support fit-content, et al, for heights.
bool CSSParser::validHeight(CSSParserValue* value)
{
    int id = value->id;
    if (id == CSSValueIntrinsic || id == CSSValueMinIntrinsic)
        return true;
    return !id && validUnit(value, FLength | FPercent | FNonNeg);
}

inline PassRefPtr<CSSPrimitiveValue> CSSParser::parseValidPrimitive(CSSValueID identifier, CSSParserValue* value)
{
    if (identifier)
        return cssValuePool().createIdentifierValue(identifier);
    if (value->unit == CSSPrimitiveValue::CSS_STRING)
        return createPrimitiveStringValue(value);
    if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
        return createPrimitiveNumericValue(value);
    if (value->unit >= CSSPrimitiveValue::CSS_TURN && value->unit <= CSSPrimitiveValue::CSS_CHS)
        return createPrimitiveNumericValue(value);
    if (value->unit >= CSSPrimitiveValue::CSS_VW && value->unit <= CSSPrimitiveValue::CSS_VMAX)
        return createPrimitiveNumericValue(value);
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
    if (value->unit >= CSSPrimitiveValue::CSS_DPPX && value->unit <= CSSPrimitiveValue::CSS_DPCM)
        return createPrimitiveNumericValue(value);
#endif
#if ENABLE(CSS_VARIABLES)
    if (value->unit == CSSPrimitiveValue::CSS_VARIABLE_NAME)
        return createPrimitiveVariableNameValue(value);
#endif
    if (value->unit >= CSSParserValue::Q_EMS)
        return CSSPrimitiveValue::createAllowingMarginQuirk(value->fValue, CSSPrimitiveValue::CSS_EMS);
    if (isCalculation(value))
        return CSSPrimitiveValue::create(m_parsedCalculation.release());

    return 0;
}

void CSSParser::addExpandedPropertyForValue(CSSPropertyID propId, PassRefPtr<CSSValue> prpValue, bool important)
{
    const StylePropertyShorthand& shorthand = shorthandForProperty(propId);
    unsigned shorthandLength = shorthand.length();
    if (!shorthandLength) {
        addProperty(propId, prpValue, important);
        return;
    }

    RefPtr<CSSValue> value = prpValue;
    ShorthandScope scope(this, propId);
    const CSSPropertyID* longhands = shorthand.properties();
    for (unsigned i = 0; i < shorthandLength; ++i)
        addProperty(longhands[i], value, important);
}

bool CSSParser::parseValue(CSSPropertyID propId, bool important)
{
    if (!m_valueList)
        return false;

    CSSParserValue* value = m_valueList->current();

    if (!value)
        return false;

    // Note: m_parsedCalculation is used to pass the calc value to validUnit and then cleared at the end of this function.
    // FIXME: This is to avoid having to pass parsedCalc to all validUnit callers.
    ASSERT(!m_parsedCalculation);
    
    CSSValueID id = value->id;

    int num = inShorthand() ? 1 : m_valueList->size();

    if (id == CSSValueInherit) {
        if (num != 1)
            return false;
        addExpandedPropertyForValue(propId, cssValuePool().createInheritedValue(), important);
        return true;
    }
    else if (id == CSSValueInitial) {
        if (num != 1)
            return false;
        addExpandedPropertyForValue(propId, cssValuePool().createExplicitInitialValue(), important);
        return true;
    }

#if ENABLE(CSS_VARIABLES)
    if (!id && value->unit == CSSPrimitiveValue::CSS_VARIABLE_NAME && num == 1) {
        addProperty(propId, createPrimitiveVariableNameValue(value), important);
        m_valueList->next();
        return true;
    }
    ASSERT(propId != CSSPropertyVariable);
#endif

    if (isKeywordPropertyID(propId)) {
        if (!isValidKeywordPropertyAndValue(propId, id, m_context))
            return false;
        if (m_valueList->next() && !inShorthand())
            return false;
        addProperty(propId, cssValuePool().createIdentifierValue(id), important);
        return true;
    }

#if ENABLE(CSS_DEVICE_ADAPTATION)
    if (inViewport())
        return parseViewportProperty(propId, important);
#endif

    bool validPrimitive = false;
    RefPtr<CSSValue> parsedValue;

    switch (propId) {
    case CSSPropertySize:                 // <length>{1,2} | auto | [ <page-size> || [ portrait | landscape] ]
        return parseSize(propId, important);

    case CSSPropertyQuotes:               // [<string> <string>]+ | none | inherit
        if (id)
            validPrimitive = true;
        else
            return parseQuotes(propId, important);
        break;
    case CSSPropertyUnicodeBidi: // normal | embed | bidi-override | isolate | isolate-override | plaintext | inherit
        if (id == CSSValueNormal
            || id == CSSValueEmbed
            || id == CSSValueBidiOverride
            || id == CSSValueWebkitIsolate
            || id == CSSValueWebkitIsolateOverride
            || id == CSSValueWebkitPlaintext)
            validPrimitive = true;
        break;

    case CSSPropertyContent:              // [ <string> | <uri> | <counter> | attr(X) | open-quote |
        // close-quote | no-open-quote | no-close-quote ]+ | inherit
        return parseContent(propId, important);

    case CSSPropertyClip:                 // <shape> | auto | inherit
        if (id == CSSValueAuto)
            validPrimitive = true;
        else if (value->unit == CSSParserValue::Function)
            return parseClipShape(propId, important);
        break;

    /* Start of supported CSS properties with validation. This is needed for parseShorthand to work
     * correctly and allows optimization in WebCore::applyRule(..)
     */
    case CSSPropertyOverflow: {
        ShorthandScope scope(this, propId);
        if (num != 1 || !parseValue(CSSPropertyOverflowY, important))
            return false;

        RefPtr<CSSValue> overflowXValue;

        // FIXME: -webkit-paged-x or -webkit-paged-y only apply to overflow-y. If this value has been
        // set using the shorthand, then for now overflow-x will default to auto, but once we implement
        // pagination controls, it should default to hidden. If the overflow-y value is anything but
        // paged-x or paged-y, then overflow-x and overflow-y should have the same value.
        if (id == CSSValueWebkitPagedX || id == CSSValueWebkitPagedY)
            overflowXValue = cssValuePool().createIdentifierValue(CSSValueAuto);
        else
            overflowXValue = m_parsedProperties.last().value();
        addProperty(CSSPropertyOverflowX, overflowXValue.release(), important);
        return true;
    }

    case CSSPropertyTextAlign:
        // left | right | center | justify | -webkit-left | -webkit-right | -webkit-center | -webkit-match-parent
        // | start | end | inherit | -webkit-auto (converted to start)
        // NOTE: <string> is not supported.
        if ((id >= CSSValueWebkitAuto && id <= CSSValueWebkitMatchParent) || id == CSSValueStart || id == CSSValueEnd)
            validPrimitive = true;
        break;

    case CSSPropertyFontWeight:  { // normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
        if (m_valueList->size() != 1)
            return false;
        return parseFontWeight(important);
    }
    case CSSPropertyBorderSpacing: {
        if (num == 1) {
            ShorthandScope scope(this, CSSPropertyBorderSpacing);
            if (!parseValue(CSSPropertyWebkitBorderHorizontalSpacing, important))
                return false;
            CSSValue* value = m_parsedProperties.last().value();
            addProperty(CSSPropertyWebkitBorderVerticalSpacing, value, important);
            return true;
        }
        else if (num == 2) {
            ShorthandScope scope(this, CSSPropertyBorderSpacing);
            if (!parseValue(CSSPropertyWebkitBorderHorizontalSpacing, important) || !parseValue(CSSPropertyWebkitBorderVerticalSpacing, important))
                return false;
            return true;
        }
        return false;
    }
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
        validPrimitive = validUnit(value, FLength | FNonNeg);
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
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationColor:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextStrokeColor:
        if (id == CSSValueWebkitText)
            validPrimitive = true; // Always allow this, even when strict parsing is on,
                                    // since we use this in our UA sheets.
        else if (id == CSSValueCurrentcolor)
            validPrimitive = true;
        else if ((id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu ||
             (id >= CSSValueWebkitFocusRingColor && id < CSSValueWebkitText && inQuirksMode())) {
            validPrimitive = true;
        } else {
            parsedValue = parseColor();
            if (parsedValue)
                m_valueList->next();
        }
        break;

    case CSSPropertyCursor: {
        // Grammar defined by CSS3 UI and modified by CSS4 images:
        // [ [<image> [<x> <y>]?,]*
        // [ auto | crosshair | default | pointer | progress | move | e-resize | ne-resize |
        // nw-resize | n-resize | se-resize | sw-resize | s-resize | w-resize | ew-resize |
        // ns-resize | nesw-resize | nwse-resize | col-resize | row-resize | text | wait | help |
        // vertical-text | cell | context-menu | alias | copy | no-drop | not-allowed | -webkit-zoom-in
        // -webkit-zoom-out | all-scroll | -webkit-grab | -webkit-grabbing ] ] | inherit
        RefPtr<CSSValueList> list;
        while (value) {
            RefPtr<CSSValue> image = 0;
            if (value->unit == CSSPrimitiveValue::CSS_URI) {
                String uri = value->string;
                if (!uri.isNull())
                    image = CSSImageValue::create(completeURL(uri));
#if ENABLE(CSS_IMAGE_SET) && ENABLE(MOUSE_CURSOR_SCALE)
            } else if (value->unit == CSSParserValue::Function && equalIgnoringCase(value->function->name, "-webkit-image-set(")) {
                image = parseImageSet();
                if (!image)
                    break;
#endif
            } else
                break;

            Vector<int> coords;
            value = m_valueList->next();
            while (value && value->unit == CSSPrimitiveValue::CSS_NUMBER) {
                coords.append(int(value->fValue));
                value = m_valueList->next();
            }
            bool hasHotSpot = false;
            IntPoint hotSpot(-1, -1);
            int nrcoords = coords.size();
            if (nrcoords > 0 && nrcoords != 2)
                return false;
            if (nrcoords == 2) {
                hasHotSpot = true;
                hotSpot = IntPoint(coords[0], coords[1]);
            }

            if (!list)
                list = CSSValueList::createCommaSeparated();

            if (image)
                list->append(CSSCursorImageValue::create(image, hasHotSpot, hotSpot));

            if ((inStrictMode() && !value) || (value && !(value->unit == CSSParserValue::Operator && value->iValue == ',')))
                return false;
            value = m_valueList->next(); // comma
        }
        if (list) {
            if (!value) { // no value after url list (MSIE 5 compatibility)
                if (list->length() != 1)
                    return false;
            } else if (inQuirksMode() && value->id == CSSValueHand) // MSIE 5 compatibility :/
                list->append(cssValuePool().createIdentifierValue(CSSValuePointer));
            else if ((value->id >= CSSValueAuto && value->id <= CSSValueWebkitGrabbing) || value->id == CSSValueCopy || value->id == CSSValueNone)
                list->append(cssValuePool().createIdentifierValue(value->id));
            m_valueList->next();
            parsedValue = list.release();
            break;
        } else if (value) {
            id = value->id;
            if (inQuirksMode() && value->id == CSSValueHand) { // MSIE 5 compatibility :/
                id = CSSValuePointer;
                validPrimitive = true;
            } else if ((value->id >= CSSValueAuto && value->id <= CSSValueWebkitGrabbing) || value->id == CSSValueCopy || value->id == CSSValueNone)
                validPrimitive = true;
        } else {
            ASSERT_NOT_REACHED();
            return false;
        }
        break;
    }

#if ENABLE(CURSOR_VISIBILITY)
    case CSSPropertyWebkitCursorVisibility:
        if (id == CSSValueAuto || id == CSSValueAutoHide)
            validPrimitive = true;
        break;
#endif

    case CSSPropertyBackgroundAttachment:
    case CSSPropertyWebkitBackgroundBlendMode:
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
    case CSSPropertyWebkitMaskRepeatY:
    {
        RefPtr<CSSValue> val1;
        RefPtr<CSSValue> val2;
        CSSPropertyID propId1, propId2;
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
    case CSSPropertyBorderImageSource:
    case CSSPropertyWebkitMaskBoxImageSource:
        if (id == CSSValueNone) {
            parsedValue = cssValuePool().createIdentifierValue(CSSValueNone);
            m_valueList->next();
        } else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = CSSImageValue::create(completeURL(value->string));
            m_valueList->next();
        } else if (isGeneratedImageValue(value)) {
            if (parseGeneratedImage(m_valueList.get(), parsedValue))
                m_valueList->next();
            else
                return false;
        }
#if ENABLE(CSS_IMAGE_SET)
        else if (value->unit == CSSParserValue::Function && equalIgnoringCase(value->function->name, "-webkit-image-set(")) {
            parsedValue = parseImageSet();
            if (!parsedValue)
                return false;
            m_valueList->next();
        }
#endif
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
            validPrimitive = validUnit(value, FLength | FNonNeg);
        break;

    case CSSPropertyLetterSpacing:       // normal | <length> | inherit
    case CSSPropertyWordSpacing:         // normal | <length> | inherit
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength);
        break;

    case CSSPropertyTextIndent:
        parsedValue = parseTextIndent();
        break;

    case CSSPropertyPaddingTop:          //// <padding-width> | inherit
    case CSSPropertyPaddingRight:        //   Which is defined as
    case CSSPropertyPaddingBottom:       //   <length> | <percentage>
    case CSSPropertyPaddingLeft:         ////
    case CSSPropertyWebkitPaddingStart:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingAfter:
        validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg));
        break;

    case CSSPropertyMaxWidth:
    case CSSPropertyWebkitMaxLogicalWidth:
        validPrimitive = (id == CSSValueNone || validWidth(value));
        break;

    case CSSPropertyMinWidth:
    case CSSPropertyWebkitMinLogicalWidth:
        validPrimitive = validWidth(value);
        break;

    case CSSPropertyWidth:
    case CSSPropertyWebkitLogicalWidth:
        validPrimitive = (id == CSSValueAuto || validWidth(value));
        break;

    case CSSPropertyMaxHeight:
    case CSSPropertyWebkitMaxLogicalHeight:
        validPrimitive = (id == CSSValueNone || validHeight(value));
        break;

    case CSSPropertyMinHeight:
    case CSSPropertyWebkitMinLogicalHeight:
        validPrimitive = validHeight(value);
        break;

    case CSSPropertyHeight:
    case CSSPropertyWebkitLogicalHeight:
        validPrimitive = (id == CSSValueAuto || validHeight(value));
        break;

    case CSSPropertyFontSize:
        return parseFontSize(important);

    case CSSPropertyFontVariant:         // normal | small-caps | inherit
        return parseFontVariant(important);

    case CSSPropertyVerticalAlign:
        // baseline | sub | super | top | text-top | middle | bottom | text-bottom |
        // <percentage> | <length> | inherit

        if (id >= CSSValueBaseline && id <= CSSValueWebkitBaselineMiddle)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent));
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
            validPrimitive = (!id && validUnit(value, FLength | FPercent));
        break;

    case CSSPropertyZIndex:              // auto | <integer> | inherit
        if (id == CSSValueAuto) {
            validPrimitive = true;
            break;
        }
        /* nobreak */
    case CSSPropertyOrphans: // <integer> | inherit | auto (We've added support for auto for backwards compatibility)
    case CSSPropertyWidows: // <integer> | inherit | auto (Ditto)
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FInteger, CSSQuirksMode));
        break;

    case CSSPropertyLineHeight:
        return parseLineHeight(important);
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
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationLine:
#endif // CSS3_TEXT
        // none | [ underline || overline || line-through || blink ] | inherit
        return parseTextDecoration(propId, important);

#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationStyle:
        // solid | double | dotted | dashed | wavy
        if (id == CSSValueSolid || id == CSSValueDouble || id == CSSValueDotted || id == CSSValueDashed || id == CSSValueWavy)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitTextUnderlinePosition:
        // auto | alphabetic | under
        return parseTextUnderlinePosition(important);
#endif // CSS3_TEXT

    case CSSPropertyZoom:          // normal | reset | document | <number> | <percentage> | inherit
        if (id == CSSValueNormal || id == CSSValueReset || id == CSSValueDocument)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FNumber | FPercent | FNonNeg, CSSStrictMode));
        break;

    case CSSPropertySrc: // Only used within @font-face and @-webkit-filter, so cannot use inherit | initial or be !important. This is a list of urls or local references.
#if ENABLE(CSS_SHADERS)
        if (m_inFilterRule)
            return parseFilterRuleSrc();
#endif
        return parseFontFaceSrc();

#if ENABLE(CSS_SHADERS)
    case CSSPropertyMix:
        // The mix property is just supported inside of an @filter rule.
        if (!m_inFilterRule)
            return false;
        return parseFilterRuleMix();
    case CSSPropertyParameters:
        // The parameters property is just supported inside of an @filter rule.
        if (!m_inFilterRule)
            return false;
        return parseFilterRuleParameters();
#endif

    case CSSPropertyUnicodeRange:
        return parseFontFaceUnicodeRange();

    /* CSS3 properties */

    case CSSPropertyBorderImage: {
        RefPtr<CSSValue> result;
        return parseBorderImage(propId, result, important);
    }
    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitMaskBoxImage: {
        RefPtr<CSSValue> result;
        if (parseBorderImage(propId, result)) {
            addProperty(propId, result, important);
            return true;
        }
        break;
    }
    case CSSPropertyBorderImageOutset:
    case CSSPropertyWebkitMaskBoxImageOutset: {
        RefPtr<CSSPrimitiveValue> result;
        if (parseBorderImageOutset(result)) {
            addProperty(propId, result, important);
            return true;
        }
        break;
    }
    case CSSPropertyBorderImageRepeat:
    case CSSPropertyWebkitMaskBoxImageRepeat: {
        RefPtr<CSSValue> result;
        if (parseBorderImageRepeat(result)) {
            addProperty(propId, result, important);
            return true;
        }
        break;
    }
    case CSSPropertyBorderImageSlice:
    case CSSPropertyWebkitMaskBoxImageSlice: {
        RefPtr<CSSBorderImageSliceValue> result;
        if (parseBorderImageSlice(propId, result)) {
            addProperty(propId, result, important);
            return true;
        }
        break;
    }
    case CSSPropertyBorderImageWidth:
    case CSSPropertyWebkitMaskBoxImageWidth: {
        RefPtr<CSSPrimitiveValue> result;
        if (parseBorderImageWidth(result)) {
            addProperty(propId, result, important);
            return true;
        }
        break;
    }
    case CSSPropertyBorderTopRightRadius:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius: {
        if (num != 1 && num != 2)
            return false;
        validPrimitive = validUnit(value, FLength | FPercent | FNonNeg);
        if (!validPrimitive)
            return false;
        RefPtr<CSSPrimitiveValue> parsedValue1 = createPrimitiveNumericValue(value);
        RefPtr<CSSPrimitiveValue> parsedValue2;
        if (num == 2) {
            value = m_valueList->next();
            validPrimitive = validUnit(value, FLength | FPercent | FNonNeg);
            if (!validPrimitive)
                return false;
            parsedValue2 = createPrimitiveNumericValue(value);
        } else
            parsedValue2 = parsedValue1;

        addProperty(propId, createPrimitiveValuePair(parsedValue1.release(), parsedValue2.release()), important);
        return true;
    }
    case CSSPropertyTabSize:
        validPrimitive = validUnit(value, FInteger | FNonNeg);
        break;
    case CSSPropertyWebkitAspectRatio:
        return parseAspectRatio(important);
    case CSSPropertyBorderRadius:
    case CSSPropertyWebkitBorderRadius:
        return parseBorderRadius(propId, important);
    case CSSPropertyOutlineOffset:
        validPrimitive = validUnit(value, FLength | FPercent);
        break;
    case CSSPropertyTextShadow: // CSS2 property, dropped in CSS2.1, back in CSS3, so treat as CSS3
    case CSSPropertyBoxShadow:
    case CSSPropertyWebkitBoxShadow:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            RefPtr<CSSValueList> shadowValueList = parseShadow(m_valueList.get(), propId);
            if (shadowValueList) {
                addProperty(propId, shadowValueList.release(), important);
                m_valueList->next();
                return true;
            }
            return false;
        }
        break;
    case CSSPropertyWebkitBoxReflect:
        if (id == CSSValueNone)
            validPrimitive = true;
        else
            return parseReflect(propId, important);
        break;
    case CSSPropertyOpacity:
        validPrimitive = validUnit(value, FNumber);
        break;
    case CSSPropertyWebkitBoxFlex:
        validPrimitive = validUnit(value, FNumber);
        break;
    case CSSPropertyWebkitBoxFlexGroup:
        validPrimitive = validUnit(value, FInteger | FNonNeg, CSSStrictMode);
        break;
    case CSSPropertyWebkitBoxOrdinalGroup:
        validPrimitive = validUnit(value, FInteger | FNonNeg, CSSStrictMode) && value->fValue;
        break;
#if ENABLE(CSS_FILTERS)
    case CSSPropertyWebkitFilter:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            RefPtr<CSSValue> val = parseFilter();
            if (val) {
                addProperty(propId, val, important);
                return true;
            }
            return false;
        }
        break;
#endif
#if ENABLE(CSS_COMPOSITING)
    case CSSPropertyWebkitBlendMode:
        if (cssCompositingEnabled())
            validPrimitive = true;
        break;
#endif
    case CSSPropertyWebkitFlex: {
        ShorthandScope scope(this, propId);
        if (id == CSSValueNone) {
            addProperty(CSSPropertyWebkitFlexGrow, cssValuePool().createValue(0, CSSPrimitiveValue::CSS_NUMBER), important);
            addProperty(CSSPropertyWebkitFlexShrink, cssValuePool().createValue(0, CSSPrimitiveValue::CSS_NUMBER), important);
            addProperty(CSSPropertyWebkitFlexBasis, cssValuePool().createIdentifierValue(CSSValueAuto), important);
            return true;
        }
        return parseFlex(m_valueList.get(), important);
    }
    case CSSPropertyWebkitFlexBasis:
        // FIXME: Support intrinsic dimensions too.
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg));
        break;
    case CSSPropertyWebkitFlexGrow:
    case CSSPropertyWebkitFlexShrink:
        validPrimitive = validUnit(value, FNumber | FNonNeg);
        break;
    case CSSPropertyWebkitOrder:
        if (validUnit(value, FInteger, CSSStrictMode)) {
            // We restrict the smallest value to int min + 2 because we use int min and int min + 1 as special values in a hash set.
            parsedValue = cssValuePool().createValue(max(static_cast<double>(std::numeric_limits<int>::min() + 2), value->fValue),
                                                             static_cast<CSSPrimitiveValue::UnitTypes>(value->unit));
            m_valueList->next();
        }
        break;
    case CSSPropertyWebkitMarquee:
        return parseShorthand(propId, webkitMarqueeShorthand(), important);
    case CSSPropertyWebkitMarqueeIncrement:
        if (id == CSSValueSmall || id == CSSValueLarge || id == CSSValueMedium)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength | FPercent);
        break;
    case CSSPropertyWebkitMarqueeRepetition:
        if (id == CSSValueInfinite)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FInteger | FNonNeg);
        break;
    case CSSPropertyWebkitMarqueeSpeed:
        if (id == CSSValueNormal || id == CSSValueSlow || id == CSSValueFast)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FTime | FInteger | FNonNeg);
        break;
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitFlowInto:
        if (!cssRegionsEnabled())
            return false;
        return parseFlowThread(propId, important);
    case CSSPropertyWebkitFlowFrom:
        if (!cssRegionsEnabled())
            return false;
        return parseRegionThread(propId, important);
#endif
    case CSSPropertyWebkitTransform:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            RefPtr<CSSValue> transformValue = parseTransform();
            if (transformValue) {
                addProperty(propId, transformValue.release(), important);
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
        CSSPropertyID propId1, propId2, propId3;
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
    case CSSPropertyWebkitPerspective:
        if (id == CSSValueNone)
            validPrimitive = true;
        else {
            // Accepting valueless numbers is a quirk of the -webkit prefixed version of the property.
            if (validUnit(value, FNumber | FLength | FNonNeg)) {
                RefPtr<CSSValue> val = createPrimitiveNumericValue(value);
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
        CSSPropertyID propId1, propId2;
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
    case CSSPropertyTransitionDelay:
    case CSSPropertyTransitionDuration:
    case CSSPropertyTransitionTimingFunction:
    case CSSPropertyTransitionProperty:
    case CSSPropertyWebkitTransitionDelay:
    case CSSPropertyWebkitTransitionDuration:
    case CSSPropertyWebkitTransitionTimingFunction:
    case CSSPropertyWebkitTransitionProperty: {
        RefPtr<CSSValue> val;
        AnimationParseContext context;
        if (parseAnimationProperty(propId, val, context)) {
            addPropertyWithPrefixingVariant(propId, val.release(), important);
            return true;
        }
        return false;
    }

    case CSSPropertyWebkitGridAutoColumns:
    case CSSPropertyWebkitGridAutoRows:
        if (!cssGridLayoutEnabled())
            return false;
        parsedValue = parseGridTrackSize();
        break;

    case CSSPropertyWebkitGridDefinitionColumns:
    case CSSPropertyWebkitGridDefinitionRows:
        if (!cssGridLayoutEnabled())
            return false;
        return parseGridTrackList(propId, important);

    case CSSPropertyWebkitGridStart:
    case CSSPropertyWebkitGridEnd:
    case CSSPropertyWebkitGridBefore:
    case CSSPropertyWebkitGridAfter:
        if (!cssGridLayoutEnabled())
            return false;

        validPrimitive = id == CSSValueAuto || (validUnit(value, FInteger) && value->fValue);
        break;

    case CSSPropertyWebkitGridColumn:
    case CSSPropertyWebkitGridRow: {
        if (!cssGridLayoutEnabled())
            return false;

        return parseGridItemPositionShorthand(propId, important);
    }

    case CSSPropertyWebkitMarginCollapse: {
        if (num == 1) {
            ShorthandScope scope(this, CSSPropertyWebkitMarginCollapse);
            if (!parseValue(webkitMarginCollapseShorthand().properties()[0], important))
                return false;
            CSSValue* value = m_parsedProperties.last().value();
            addProperty(webkitMarginCollapseShorthand().properties()[1], value, important);
            return true;
        }
        else if (num == 2) {
            ShorthandScope scope(this, CSSPropertyWebkitMarginCollapse);
            if (!parseValue(webkitMarginCollapseShorthand().properties()[0], important) || !parseValue(webkitMarginCollapseShorthand().properties()[1], important))
                return false;
            return true;
        }
        return false;
    }
    case CSSPropertyTextLineThroughWidth:
    case CSSPropertyTextOverlineWidth:
    case CSSPropertyTextUnderlineWidth:
        if (id == CSSValueAuto || id == CSSValueNormal || id == CSSValueThin ||
            id == CSSValueMedium || id == CSSValueThick)
            validPrimitive = true;
        else
            validPrimitive = !id && validUnit(value, FNumber | FLength | FPercent);
        break;
    case CSSPropertyWebkitColumnCount:
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = !id && validUnit(value, FPositiveInteger, CSSQuirksMode);
        break;
    case CSSPropertyWebkitColumnGap:         // normal | <length>
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FLength | FNonNeg);
        break;
    case CSSPropertyWebkitColumnAxis:
        if (id == CSSValueHorizontal || id == CSSValueVertical || id == CSSValueAuto)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitColumnProgression:
        if (id == CSSValueNormal || id == CSSValueReverse)
            validPrimitive = true;
        break;
    case CSSPropertyWebkitColumnSpan: // none | all | 1 (will be dropped in the unprefixed property)
        if (id == CSSValueAll || id == CSSValueNone)
            validPrimitive = true;
        else
            validPrimitive = validUnit(value, FNumber | FNonNeg) && value->fValue == 1;
        break;
    case CSSPropertyWebkitColumnWidth:         // auto | <length>
        if (id == CSSValueAuto)
            validPrimitive = true;
        else // Always parse this property in strict mode, since it would be ambiguous otherwise when used in the 'columns' shorthand property.
            validPrimitive = validUnit(value, FLength | FNonNeg, CSSStrictMode) && value->fValue;
        break;
    // End of CSS3 properties

    // Apple specific properties.  These will never be standardized and are purely to
    // support custom WebKit-based Apple applications.
    case CSSPropertyWebkitLineClamp:
        // When specifying number of lines, don't allow 0 as a valid value
        // When specifying either type of unit, require non-negative integers
        validPrimitive = (!id && (value->unit == CSSPrimitiveValue::CSS_PERCENTAGE || value->fValue) && validUnit(value, FInteger | FPercent | FNonNeg, CSSQuirksMode));
        break;

    case CSSPropertyWebkitFontSizeDelta:           // <length>
        validPrimitive = validUnit(value, FLength);
        break;

    case CSSPropertyWebkitHighlight:
        if (id == CSSValueNone || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphenateCharacter:
        if (id == CSSValueAuto || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphenateLimitBefore:
    case CSSPropertyWebkitHyphenateLimitAfter:
        if (id == CSSValueAuto || validUnit(value, FInteger | FNonNeg, CSSStrictMode))
            validPrimitive = true;
        break;

    case CSSPropertyWebkitHyphenateLimitLines:
        if (id == CSSValueNoLimit || validUnit(value, FInteger | FNonNeg, CSSStrictMode))
            validPrimitive = true;
        break;

    case CSSPropertyWebkitLineGrid:
        if (id == CSSValueNone)
            validPrimitive = true;
        else if (value->unit == CSSPrimitiveValue::CSS_IDENT) {
            String lineGridValue = String(value->string);
            if (!lineGridValue.isEmpty()) {
                addProperty(propId, cssValuePool().createValue(lineGridValue, CSSPrimitiveValue::CSS_STRING), important);
                return true;
            }
        }
        break;
    case CSSPropertyWebkitLocale:
        if (id == CSSValueAuto || value->unit == CSSPrimitiveValue::CSS_STRING)
            validPrimitive = true;
        break;

#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPropertyWebkitDashboardRegion: // <dashboard-region> | <dashboard-region>
        if (value->unit == CSSParserValue::Function || id == CSSValueNone)
            return parseDashboardRegions(propId, important);
        break;
#endif
    // End Apple-specific properties

#if ENABLE(DRAGGABLE_REGION)
    case CSSPropertyWebkitAppRegion:
        if (id >= CSSValueDrag && id <= CSSValueNoDrag)
            validPrimitive = true;
        break;
#endif

#if ENABLE(TOUCH_EVENTS)
    case CSSPropertyWebkitTapHighlightColor:
        if ((id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu
            || (id >= CSSValueWebkitFocusRingColor && id < CSSValueWebkitText && inQuirksMode())) {
            validPrimitive = true;
        } else {
            parsedValue = parseColor();
            if (parsedValue)
                m_valueList->next();
        }
        break;
#endif

        /* shorthand properties */
    case CSSPropertyBackground: {
        // Position must come before color in this array because a plain old "0" is a legal color
        // in quirks mode but it's usually the X coordinate of a position.
        const CSSPropertyID properties[] = { CSSPropertyBackgroundImage, CSSPropertyBackgroundRepeat,
                                   CSSPropertyBackgroundAttachment, CSSPropertyBackgroundPosition, CSSPropertyBackgroundOrigin,
                                   CSSPropertyBackgroundClip, CSSPropertyBackgroundColor, CSSPropertyBackgroundSize };
        return parseFillShorthand(propId, properties, WTF_ARRAY_LENGTH(properties), important);
    }
    case CSSPropertyWebkitMask: {
        const CSSPropertyID properties[] = { CSSPropertyWebkitMaskImage, CSSPropertyWebkitMaskRepeat,
            CSSPropertyWebkitMaskPosition, CSSPropertyWebkitMaskOrigin, CSSPropertyWebkitMaskClip, CSSPropertyWebkitMaskSize };
        return parseFillShorthand(propId, properties, WTF_ARRAY_LENGTH(properties), important);
    }
    case CSSPropertyBorder:
        // [ 'border-width' || 'border-style' || <color> ] | inherit
    {
        if (parseShorthand(propId, borderAbridgedShorthand(), important)) {
            // The CSS3 Borders and Backgrounds specification says that border also resets border-image. It's as
            // though a value of none was specified for the image.
            addExpandedPropertyForValue(CSSPropertyBorderImage, cssValuePool().createImplicitInitialValue(), important);
            return true;
        }
        return false;
    }
    case CSSPropertyBorderTop:
        // [ 'border-top-width' || 'border-style' || <color> ] | inherit
        return parseShorthand(propId, borderTopShorthand(), important);
    case CSSPropertyBorderRight:
        // [ 'border-right-width' || 'border-style' || <color> ] | inherit
        return parseShorthand(propId, borderRightShorthand(), important);
    case CSSPropertyBorderBottom:
        // [ 'border-bottom-width' || 'border-style' || <color> ] | inherit
        return parseShorthand(propId, borderBottomShorthand(), important);
    case CSSPropertyBorderLeft:
        // [ 'border-left-width' || 'border-style' || <color> ] | inherit
        return parseShorthand(propId, borderLeftShorthand(), important);
    case CSSPropertyWebkitBorderStart:
        return parseShorthand(propId, webkitBorderStartShorthand(), important);
    case CSSPropertyWebkitBorderEnd:
        return parseShorthand(propId, webkitBorderEndShorthand(), important);
    case CSSPropertyWebkitBorderBefore:
        return parseShorthand(propId, webkitBorderBeforeShorthand(), important);
    case CSSPropertyWebkitBorderAfter:
        return parseShorthand(propId, webkitBorderAfterShorthand(), important);
    case CSSPropertyOutline:
        // [ 'outline-color' || 'outline-style' || 'outline-width' ] | inherit
        return parseShorthand(propId, outlineShorthand(), important);
    case CSSPropertyBorderColor:
        // <color>{1,4} | inherit
        return parse4Values(propId, borderColorShorthand().properties(), important);
    case CSSPropertyBorderWidth:
        // <border-width>{1,4} | inherit
        return parse4Values(propId, borderWidthShorthand().properties(), important);
    case CSSPropertyBorderStyle:
        // <border-style>{1,4} | inherit
        return parse4Values(propId, borderStyleShorthand().properties(), important);
    case CSSPropertyMargin:
        // <margin-width>{1,4} | inherit
        return parse4Values(propId, marginShorthand().properties(), important);
    case CSSPropertyPadding:
        // <padding-width>{1,4} | inherit
        return parse4Values(propId, paddingShorthand().properties(), important);
    case CSSPropertyWebkitFlexFlow:
        return parseShorthand(propId, webkitFlexFlowShorthand(), important);
    case CSSPropertyFont:
        // [ [ 'font-style' || 'font-variant' || 'font-weight' ]? 'font-size' [ / 'line-height' ]?
        // 'font-family' ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
        if (id >= CSSValueCaption && id <= CSSValueStatusBar)
            validPrimitive = true;
        else
            return parseFont(important);
        break;
    case CSSPropertyListStyle:
        return parseShorthand(propId, listStyleShorthand(), important);
    case CSSPropertyWebkitColumns:
        return parseShorthand(propId, webkitColumnsShorthand(), important);
    case CSSPropertyWebkitColumnRule:
        return parseShorthand(propId, webkitColumnRuleShorthand(), important);
    case CSSPropertyWebkitTextStroke:
        return parseShorthand(propId, webkitTextStrokeShorthand(), important);
    case CSSPropertyWebkitAnimation:
        return parseAnimationShorthand(important);
    case CSSPropertyTransition:
    case CSSPropertyWebkitTransition:
        return parseTransitionShorthand(propId, important);
    case CSSPropertyInvalid:
        return false;
    case CSSPropertyPage:
        return parsePage(propId, important);
#if ENABLE(CSS_SHADERS)
    case CSSPropertyGeometry:
        return m_inFilterRule ? parseGeometry(propId, value, important) : false;
#endif
    case CSSPropertyFontStretch:
    case CSSPropertyTextLineThrough:
    case CSSPropertyTextOverline:
    case CSSPropertyTextUnderline:
        return false;
    // CSS Text Layout Module Level 3: Vertical writing support
    case CSSPropertyWebkitTextEmphasis:
        return parseShorthand(propId, webkitTextEmphasisShorthand(), important);

    case CSSPropertyWebkitTextEmphasisStyle:
        return parseTextEmphasisStyle(important);

    case CSSPropertyWebkitTextOrientation:
        // FIXME: For now just support sideways, sideways-right, upright and vertical-right.
        if (id == CSSValueSideways || id == CSSValueSidewaysRight || id == CSSValueVerticalRight || id == CSSValueUpright)
            validPrimitive = true;
        break;

    case CSSPropertyWebkitLineBoxContain:
        if (id == CSSValueNone)
            validPrimitive = true;
        else
            return parseLineBoxContain(important);
        break;
    case CSSPropertyWebkitFontFeatureSettings:
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            return parseFontFeatureSettings(important);
        break;

    case CSSPropertyWebkitFontVariantLigatures:
        if (id == CSSValueNormal)
            validPrimitive = true;
        else
            return parseFontVariantLigatures(important);
        break;
    case CSSPropertyWebkitClipPath:
        if (id == CSSValueNone)
            validPrimitive = true;
        else if (value->unit == CSSParserValue::Function)
            return parseBasicShape(propId, important);
#if ENABLE(SVG)
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = CSSPrimitiveValue::create(value->string, CSSPrimitiveValue::CSS_URI);
            addProperty(propId, parsedValue.release(), important);
            return true;
        }
#endif
        break;
#if ENABLE(CSS_SHAPES)
    case CSSPropertyWebkitShapeInside:
    case CSSPropertyWebkitShapeOutside:
        if (!RuntimeEnabledFeatures::cssShapesEnabled())
            return false;
        if (id == CSSValueAuto)
            validPrimitive = true;
        else if (propId == CSSPropertyWebkitShapeInside && id == CSSValueOutsideShape)
            validPrimitive = true;
        else if (value->unit == CSSParserValue::Function)
            return parseBasicShape(propId, important);
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = CSSImageValue::create(completeURL(value->string));
            m_valueList->next();
        }
        break;
    case CSSPropertyWebkitShapeMargin:
    case CSSPropertyWebkitShapePadding:
        validPrimitive = (RuntimeEnabledFeatures::cssShapesEnabled() && !id && validUnit(value, FLength | FNonNeg));
        break;
#endif
#if ENABLE(CSS_IMAGE_ORIENTATION)
    case CSSPropertyImageOrientation:
        validPrimitive = !id && validUnit(value, FAngle);
        break;
#endif
#if ENABLE(CSS_IMAGE_RESOLUTION)
    case CSSPropertyImageResolution:
        parsedValue = parseImageResolution();
        break;
#endif
    case CSSPropertyBorderBottomStyle:
    case CSSPropertyBorderCollapse:
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyBorderRightStyle:
    case CSSPropertyBorderTopStyle:
    case CSSPropertyBoxSizing:
    case CSSPropertyCaptionSide:
    case CSSPropertyClear:
    case CSSPropertyDirection:
    case CSSPropertyDisplay:
    case CSSPropertyEmptyCells:
    case CSSPropertyFloat:
    case CSSPropertyFontStyle:
    case CSSPropertyImageRendering:
    case CSSPropertyListStylePosition:
    case CSSPropertyListStyleType:
    case CSSPropertyOutlineStyle:
    case CSSPropertyOverflowWrap:
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyPointerEvents:
    case CSSPropertyPosition:
    case CSSPropertyResize:
    case CSSPropertySpeak:
    case CSSPropertyTableLayout:
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextOverflow:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextRendering:
    case CSSPropertyTextTransform:
    case CSSPropertyTextUnderlineMode:
    case CSSPropertyTextUnderlineStyle:
#if ENABLE(CSS_VARIABLES)
    case CSSPropertyVariable:
#endif
    case CSSPropertyVisibility:
    case CSSPropertyWebkitAppearance:
    case CSSPropertyWebkitBackfaceVisibility:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderFit:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBoxAlign:
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    case CSSPropertyWebkitBoxDecorationBreak:
#endif
    case CSSPropertyWebkitBoxDirection:
    case CSSPropertyWebkitBoxLines:
    case CSSPropertyWebkitBoxOrient:
    case CSSPropertyWebkitBoxPack:
    case CSSPropertyWebkitColorCorrection:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitColumnRuleStyle:
    case CSSPropertyWebkitAlignContent:
    case CSSPropertyWebkitAlignItems:
    case CSSPropertyWebkitAlignSelf:
    case CSSPropertyWebkitFlexDirection:
    case CSSPropertyWebkitFlexWrap:
    case CSSPropertyWebkitJustifyContent:
    case CSSPropertyWebkitFontKerning:
    case CSSPropertyWebkitFontSmoothing:
    case CSSPropertyWebkitHyphens:
    case CSSPropertyWebkitGridAutoFlow:
    case CSSPropertyWebkitLineAlign:
    case CSSPropertyWebkitLineBreak:
    case CSSPropertyWebkitLineSnap:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarqueeDirection:
    case CSSPropertyWebkitMarqueeStyle:
    case CSSPropertyWebkitNbspMode:
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling:
#endif
    case CSSPropertyWebkitPrintColorAdjust:
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
    case CSSPropertyWebkitRegionBreakInside:
    case CSSPropertyWebkitRegionFragment:
#endif
    case CSSPropertyWebkitRtlOrdering:
    case CSSPropertyWebkitRubyPosition:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextAlignLast:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextCombine:
    case CSSPropertyWebkitTextEmphasisPosition:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextJustify:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextSecurity:
    case CSSPropertyWebkitTransformStyle:
    case CSSPropertyWebkitUserDrag:
    case CSSPropertyWebkitUserModify:
    case CSSPropertyWebkitUserSelect:
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrapFlow:
    case CSSPropertyWebkitWrapThrough:
#endif
    case CSSPropertyWebkitWritingMode:
    case CSSPropertyWhiteSpace:
    case CSSPropertyWordBreak:
    case CSSPropertyWordWrap:
        // These properties should be handled before in isValidKeywordPropertyAndValue().
        ASSERT_NOT_REACHED();
        return false;
#if ENABLE(CSS_DEVICE_ADAPTATION)
    // Properties bellow are validated inside parseViewportProperty, because we
    // check for parser state inViewportScope. We need to invalidate if someone
    // adds them outside a @viewport rule.
    case CSSPropertyMaxZoom:
    case CSSPropertyMinZoom:
    case CSSPropertyOrientation:
    case CSSPropertyUserZoom:
        validPrimitive = false;
        break;
#endif
#if ENABLE(SVG)
    default:
        return parseSVGValue(propId, important);
#endif
    }

    if (validPrimitive) {
        parsedValue = parseValidPrimitive(id, value);
        m_valueList->next();
    }
    ASSERT(!m_parsedCalculation);
    if (parsedValue) {
        if (!m_valueList->current() || inShorthand()) {
            addProperty(propId, parsedValue.release(), important);
            return true;
        }
    }
    return false;
}

void CSSParser::addFillValue(RefPtr<CSSValue>& lval, PassRefPtr<CSSValue> rval)
{
    if (lval) {
        if (lval->isBaseValueList())
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

static bool parseBackgroundClip(CSSParserValue* parserValue, RefPtr<CSSValue>& cssValue)
{
    if (parserValue->id == CSSValueBorderBox || parserValue->id == CSSValuePaddingBox
        || parserValue->id == CSSValueContentBox || parserValue->id == CSSValueWebkitText) {
        cssValue = cssValuePool().createIdentifierValue(parserValue->id);
        return true;
    }
    return false;
}

bool CSSParser::useLegacyBackgroundSizeShorthandBehavior() const
{
    return m_context.useLegacyBackgroundSizeShorthandBehavior;
}

const int cMaxFillProperties = 9;

bool CSSParser::parseFillShorthand(CSSPropertyID propId, const CSSPropertyID* properties, int numProperties, bool important)
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
    bool foundPositionCSSProperty = false;

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
                    addFillValue(values[i], cssValuePool().createImplicitInitialValue());
                    if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                        addFillValue(positionYValue, cssValuePool().createImplicitInitialValue());
                    if (properties[i] == CSSPropertyBackgroundRepeat || properties[i] == CSSPropertyWebkitMaskRepeat)
                        addFillValue(repeatYValue, cssValuePool().createImplicitInitialValue());
                    if ((properties[i] == CSSPropertyBackgroundOrigin || properties[i] == CSSPropertyWebkitMaskOrigin) && !parsedProperty[i]) {
                        // If background-origin wasn't present, then reset background-clip also.
                        addFillValue(clipValue, cssValuePool().createImplicitInitialValue());
                    }
                }
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
        }

        bool sizeCSSPropertyExpected = false;
        if (isForwardSlashOperator(val) && foundPositionCSSProperty) {
            sizeCSSPropertyExpected = true;
            m_valueList->next();
        }

        foundPositionCSSProperty = false;
        bool found = false;
        for (i = 0; !found && i < numProperties; ++i) {

            if (sizeCSSPropertyExpected && (properties[i] != CSSPropertyBackgroundSize && properties[i] != CSSPropertyWebkitMaskSize))
                continue;
            if (!sizeCSSPropertyExpected && (properties[i] == CSSPropertyBackgroundSize || properties[i] == CSSPropertyWebkitMaskSize))
                continue;

            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val1;
                RefPtr<CSSValue> val2;
                CSSPropertyID propId1, propId2;
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
                        if (parseBackgroundClip(parserValue, val1))
                            addFillValue(clipValue, val1.release()); // The property parsed successfully.
                        else
                            addFillValue(clipValue, cssValuePool().createImplicitInitialValue()); // Some value was used for origin that is not supported by clip. Just reset clip instead.
                    }
                    if (properties[i] == CSSPropertyBackgroundClip || properties[i] == CSSPropertyWebkitMaskClip) {
                        // Update clipValue
                        addFillValue(clipValue, val1.release());
                        foundClip = true;
                    }
                    if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                        foundPositionCSSProperty = true;
                }
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    // Now add all of the properties we found.
    for (i = 0; i < numProperties; i++) {
        // Fill in any remaining properties with the initial value.
        if (!parsedProperty[i]) {
            addFillValue(values[i], cssValuePool().createImplicitInitialValue());
            if (properties[i] == CSSPropertyBackgroundPosition || properties[i] == CSSPropertyWebkitMaskPosition)
                addFillValue(positionYValue, cssValuePool().createImplicitInitialValue());
            if (properties[i] == CSSPropertyBackgroundRepeat || properties[i] == CSSPropertyWebkitMaskRepeat)
                addFillValue(repeatYValue, cssValuePool().createImplicitInitialValue());
            if (properties[i] == CSSPropertyBackgroundOrigin || properties[i] == CSSPropertyWebkitMaskOrigin) {
                // If background-origin wasn't present, then reset background-clip also.
                addFillValue(clipValue, cssValuePool().createImplicitInitialValue());
            }
        }
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
        else if (properties[i] == CSSPropertyBackgroundSize && !parsedProperty[i] && useLegacyBackgroundSizeShorthandBehavior())
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

#if ENABLE(CSS_VARIABLES)
bool CSSParser::cssVariablesEnabled() const
{
    return m_context.isCSSVariablesEnabled;
}

void CSSParser::storeVariableDeclaration(const CSSParserString& name, PassOwnPtr<CSSParserValueList> value, bool important)
{
    // When CSSGrammar.y encounters an invalid declaration it passes null for the CSSParserValueList, just bail.
    if (!value)
        return;
    
    static const unsigned prefixLength = sizeof("-webkit-var-") - 1;
    
    ASSERT(name.length() > prefixLength);
    AtomicString variableName = name.substring(prefixLength, name.length() - prefixLength);

    StringBuilder builder;
    for (unsigned i = 0, size = value->size(); i < size; i++) {
        if (i)
            builder.append(' ');
        RefPtr<CSSValue> cssValue = value->valueAt(i)->createCSSValue();
        if (!cssValue)
            return;
        builder.append(cssValue->cssText());
    }
    addProperty(CSSPropertyVariable, CSSVariableValue::create(variableName, builder.toString().lower()), important, false);
}
#endif

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
    const StylePropertyShorthand& animationProperties = webkitAnimationShorthandForParsing();
    const unsigned numProperties = 7;

    // The list of properties in the shorthand should be the same
    // length as the list with animation name in last position, even though they are
    // in a different order.
    ASSERT(numProperties == webkitAnimationShorthandForParsing().length());
    ASSERT(numProperties == webkitAnimationShorthand().length());

    ShorthandScope scope(this, CSSPropertyWebkitAnimation);

    bool parsedProperty[numProperties] = { false };
    AnimationParseContext context;
    RefPtr<CSSValue> values[numProperties];

    unsigned i;
    while (m_valueList->current()) {
        CSSParserValue* val = m_valueList->current();
        if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            // We hit the end.  Fill in all remaining values with the initial value.
            m_valueList->next();
            for (i = 0; i < numProperties; ++i) {
                if (!parsedProperty[i])
                    addAnimationValue(values[i], cssValuePool().createImplicitInitialValue());
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
            context.commitFirstAnimation();
        }

        bool found = false;
        for (i = 0; i < numProperties; ++i) {
            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val;
                if (parseAnimationProperty(animationProperties.properties()[i], val, context)) {
                    parsedProperty[i] = found = true;
                    addAnimationValue(values[i], val.release());
                    break;
                }
            }

            // There are more values to process but 'none' or 'all' were already defined as the animation property, the declaration becomes invalid.
            if (!context.animationPropertyKeywordAllowed() && context.hasCommittedFirstAnimation())
                return false;
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    for (i = 0; i < numProperties; ++i) {
        // If we didn't find the property, set an intial value.
        if (!parsedProperty[i])
            addAnimationValue(values[i], cssValuePool().createImplicitInitialValue());

        addProperty(animationProperties.properties()[i], values[i].release(), important);
    }

    return true;
}

bool CSSParser::parseTransitionShorthand(CSSPropertyID propId, bool important)
{
    const unsigned numProperties = 4;
    const StylePropertyShorthand& shorthand = shorthandForProperty(propId);
    ASSERT(numProperties == shorthand.length());

    ShorthandScope scope(this, propId);

    bool parsedProperty[numProperties] = { false };
    AnimationParseContext context;
    RefPtr<CSSValue> values[numProperties];

    unsigned i;
    while (m_valueList->current()) {
        CSSParserValue* val = m_valueList->current();
        if (val->unit == CSSParserValue::Operator && val->iValue == ',') {
            // We hit the end. Fill in all remaining values with the initial value.
            m_valueList->next();
            for (i = 0; i < numProperties; ++i) {
                if (!parsedProperty[i])
                    addAnimationValue(values[i], cssValuePool().createImplicitInitialValue());
                parsedProperty[i] = false;
            }
            if (!m_valueList->current())
                break;
            context.commitFirstAnimation();
        }

        bool found = false;
        for (i = 0; !found && i < numProperties; ++i) {
            if (!parsedProperty[i]) {
                RefPtr<CSSValue> val;
                if (parseAnimationProperty(shorthand.properties()[i], val, context)) {
                    parsedProperty[i] = found = true;
                    addAnimationValue(values[i], val.release());
                }

                // There are more values to process but 'none' or 'all' were already defined as the animation property, the declaration becomes invalid.
                if (!context.animationPropertyKeywordAllowed() && context.hasCommittedFirstAnimation())
                    return false;
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
            addAnimationValue(values[i], cssValuePool().createImplicitInitialValue());
    }

    // Now add all of the properties we found.
    for (i = 0; i < numProperties; i++)
        addPropertyWithPrefixingVariant(shorthand.properties()[i], values[i].release(), important);

    return true;
}

bool CSSParser::parseShorthand(CSSPropertyID propId, const StylePropertyShorthand& shorthand, bool important)
{
    // We try to match as many properties as possible
    // We set up an array of booleans to mark which property has been found,
    // and we try to search for properties until it makes no longer any sense.
    ShorthandScope scope(this, propId);

    bool found = false;
    unsigned propertiesParsed = 0;
    bool propertyFound[6]= { false, false, false, false, false, false }; // 6 is enough size.

    while (m_valueList->current()) {
        found = false;
        for (unsigned propIndex = 0; !found && propIndex < shorthand.length(); ++propIndex) {
            if (!propertyFound[propIndex] && parseValue(shorthand.properties()[propIndex], important)) {
                    propertyFound[propIndex] = found = true;
                    propertiesParsed++;
            }
        }

        // if we didn't find at least one match, this is an
        // invalid shorthand and we have to ignore it
        if (!found)
            return false;
    }

    if (propertiesParsed == shorthand.length())
        return true;

    // Fill in any remaining properties with the initial value.
    ImplicitScope implicitScope(this, PropertyImplicit);
    const StylePropertyShorthand* const* const propertiesForInitialization = shorthand.propertiesForInitialization();
    for (unsigned i = 0; i < shorthand.length(); ++i) {
        if (propertyFound[i])
            continue;

        if (propertiesForInitialization) {
            const StylePropertyShorthand& initProperties = *(propertiesForInitialization[i]);
            for (unsigned propIndex = 0; propIndex < initProperties.length(); ++propIndex)
                addProperty(initProperties.properties()[propIndex], cssValuePool().createImplicitInitialValue(), important);
        } else
            addProperty(shorthand.properties()[i], cssValuePool().createImplicitInitialValue(), important);
    }

    return true;
}

bool CSSParser::parse4Values(CSSPropertyID propId, const CSSPropertyID *properties,  bool important)
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
            CSSValue* value = m_parsedProperties.last().value();
            ImplicitScope implicitScope(this, PropertyImplicit);
            addProperty(properties[1], value, important);
            addProperty(properties[2], value, important);
            addProperty(properties[3], value, important);
            break;
        }
        case 2: {
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important))
                return false;
            CSSValue* value = m_parsedProperties[m_parsedProperties.size() - 2].value();
            ImplicitScope implicitScope(this, PropertyImplicit);
            addProperty(properties[2], value, important);
            value = m_parsedProperties[m_parsedProperties.size() - 2].value();
            addProperty(properties[3], value, important);
            break;
        }
        case 3: {
            if (!parseValue(properties[0], important) || !parseValue(properties[1], important) || !parseValue(properties[2], important))
                return false;
            CSSValue* value = m_parsedProperties[m_parsedProperties.size() - 2].value();
            ImplicitScope implicitScope(this, PropertyImplicit);
            addProperty(properties[3], value, important);
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
bool CSSParser::parsePage(CSSPropertyID propId, bool important)
{
    ASSERT(propId == CSSPropertyPage);

    if (m_valueList->size() != 1)
        return false;

    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    if (value->id == CSSValueAuto) {
        addProperty(propId, cssValuePool().createIdentifierValue(value->id), important);
        return true;
    } else if (value->id == 0 && value->unit == CSSPrimitiveValue::CSS_IDENT) {
        addProperty(propId, createPrimitiveStringValue(value), important);
        return true;
    }
    return false;
}

// <length>{1,2} | auto | [ <page-size> || [ portrait | landscape] ]
bool CSSParser::parseSize(CSSPropertyID propId, bool important)
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
            parsedValues->append(cssValuePool().createIdentifierValue(value->id));
            return Auto;
        }
        return None;
    case CSSValueLandscape:
    case CSSValuePortrait:
        if (prevParamType == None || prevParamType == PageSize) {
            parsedValues->append(cssValuePool().createIdentifierValue(value->id));
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
            // This is not specified by the CSS3 Paged Media specification, but for simpler processing later (StyleResolver::applyPageSizeProperty).
            parsedValues->prepend(cssValuePool().createIdentifierValue(value->id));
            return PageSize;
        }
        return None;
    case 0:
        if (validUnit(value, FLength | FNonNeg) && (prevParamType == None || prevParamType == Length)) {
            parsedValues->append(createPrimitiveNumericValue(value));
            return Length;
        }
        return None;
    default:
        return None;
    }
}

// [ <string> <string> ]+ | inherit | none
// inherit and none are handled in parseValue.
bool CSSParser::parseQuotes(CSSPropertyID propId, bool important)
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
bool CSSParser::parseContent(CSSPropertyID propId, bool important)
{
    RefPtr<CSSValueList> values = CSSValueList::createCommaSeparated();

    while (CSSParserValue* val = m_valueList->current()) {
        RefPtr<CSSValue> parsedValue;
        if (val->unit == CSSPrimitiveValue::CSS_URI) {
            // url
            parsedValue = CSSImageValue::create(completeURL(val->string));
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
#if ENABLE(CSS_IMAGE_SET)
            } else if (equalIgnoringCase(val->function->name, "-webkit-image-set(")) {
                parsedValue = parseImageSet();
                if (!parsedValue)
                    return false;
#endif
            } else if (isGeneratedImageValue(val)) {
                if (!parseGeneratedImage(m_valueList.get(), parsedValue))
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
                parsedValue = cssValuePool().createIdentifierValue(val->id);
            default:
                break;
            }
        } else if (val->unit == CSSPrimitiveValue::CSS_STRING) {
            parsedValue = createPrimitiveStringValue(val);
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

    if (m_context.isHTMLDocument)
        attrName = attrName.lower();

    return cssValuePool().createValue(attrName, CSSPrimitiveValue::CSS_ATTR);
}

PassRefPtr<CSSValue> CSSParser::parseBackgroundColor()
{
    CSSValueID id = m_valueList->current()->id;
    if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu || id == CSSValueCurrentcolor ||
        (id >= CSSValueGrey && id < CSSValueWebkitText && inQuirksMode()))
        return cssValuePool().createIdentifierValue(id);
    return parseColor();
}

bool CSSParser::parseFillImage(CSSParserValueList* valueList, RefPtr<CSSValue>& value)
{
    if (valueList->current()->id == CSSValueNone) {
        value = cssValuePool().createIdentifierValue(CSSValueNone);
        return true;
    }
    if (valueList->current()->unit == CSSPrimitiveValue::CSS_URI) {
        value = CSSImageValue::create(completeURL(valueList->current()->string));
        return true;
    }

    if (isGeneratedImageValue(valueList->current()))
        return parseGeneratedImage(valueList, value);
    
#if ENABLE(CSS_IMAGE_SET)
    if (valueList->current()->unit == CSSParserValue::Function && equalIgnoringCase(valueList->current()->function->name, "-webkit-image-set(")) {
        value = parseImageSet();
        if (value)
            return true;
    }
#endif

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
        return cssValuePool().createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength))
        return createPrimitiveNumericValue(valueList->current());
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
        return cssValuePool().createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength))
        return createPrimitiveNumericValue(valueList->current());
    return 0;
}

PassRefPtr<CSSPrimitiveValue> CSSParser::parseFillPositionComponent(CSSParserValueList* valueList, unsigned& cumulativeFlags, FillPositionFlag& individualFlag, FillPositionParsingMode parsingMode)
{
    CSSValueID id = valueList->current()->id;
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

        if (parsingMode == ResolveValuesAsKeyword)
            return cssValuePool().createIdentifierValue(id);

        return cssValuePool().createValue(percent, CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    if (validUnit(valueList->current(), FPercent | FLength)) {
        if (!cumulativeFlags) {
            cumulativeFlags |= XFillPosition;
            individualFlag = XFillPosition;
        } else if (cumulativeFlags & (XFillPosition | AmbiguousFillPosition)) {
            cumulativeFlags |= YFillPosition;
            individualFlag = YFillPosition;
        } else {
            if (m_parsedCalculation)
                m_parsedCalculation.release();
            return 0;
        }
        return createPrimitiveNumericValue(valueList->current());
    }
    return 0;
}

static bool isValueConflictingWithCurrentEdge(int value1, int value2)
{
    if ((value1 == CSSValueLeft || value1 == CSSValueRight) && (value2 == CSSValueLeft || value2 == CSSValueRight))
        return true;

    if ((value1 == CSSValueTop || value1 == CSSValueBottom) && (value2 == CSSValueTop || value2 == CSSValueBottom))
        return true;

    return false;
}

static bool isFillPositionKeyword(CSSValueID value)
{
    return value == CSSValueLeft || value == CSSValueTop || value == CSSValueBottom || value == CSSValueRight || value == CSSValueCenter;
}

void CSSParser::parse4ValuesFillPosition(CSSParserValueList* valueList, RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2, PassRefPtr<CSSPrimitiveValue> parsedValue1, PassRefPtr<CSSPrimitiveValue> parsedValue2)
{
    // [ left | right ] [ <percentage] | <length> ] && [ top | bottom ] [ <percentage> | <length> ]
    // In the case of 4 values <position> requires the second value to be a length or a percentage.
    if (isFillPositionKeyword(parsedValue2->getValueID()))
        return;

    unsigned cumulativeFlags = 0;
    FillPositionFlag value3Flag = InvalidFillPosition;
    RefPtr<CSSPrimitiveValue> value3 = parseFillPositionComponent(valueList, cumulativeFlags, value3Flag, ResolveValuesAsKeyword);
    if (!value3)
        return;

    CSSValueID ident1 = parsedValue1->getValueID();
    CSSValueID ident3 = value3->getValueID();

    if (ident1 == CSSValueCenter)
        return;

    if (!isFillPositionKeyword(ident3) || ident3 == CSSValueCenter)
        return;

    // We need to check if the values are not conflicting, e.g. they are not on the same edge. It is
    // needed as the second call to parseFillPositionComponent was on purpose not checking it. In the
    // case of two values top 20px is invalid but in the case of 4 values it becomes valid.
    if (isValueConflictingWithCurrentEdge(ident1, ident3))
        return;

    valueList->next();

    cumulativeFlags = 0;
    FillPositionFlag value4Flag = InvalidFillPosition;
    RefPtr<CSSPrimitiveValue> value4 = parseFillPositionComponent(valueList, cumulativeFlags, value4Flag, ResolveValuesAsKeyword);
    if (!value4)
        return;

    // 4th value must be a length or a percentage.
    if (isFillPositionKeyword(value4->getValueID()))
        return;

    value1 = createPrimitiveValuePair(parsedValue1, parsedValue2);
    value2 = createPrimitiveValuePair(value3, value4);

    if (ident1 == CSSValueTop || ident1 == CSSValueBottom)
        value1.swap(value2);

    valueList->next();
}
void CSSParser::parse3ValuesFillPosition(CSSParserValueList* valueList, RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2, PassRefPtr<CSSPrimitiveValue> parsedValue1, PassRefPtr<CSSPrimitiveValue> parsedValue2)
{
    unsigned cumulativeFlags = 0;
    FillPositionFlag value3Flag = InvalidFillPosition;
    RefPtr<CSSPrimitiveValue> value3 = parseFillPositionComponent(valueList, cumulativeFlags, value3Flag, ResolveValuesAsKeyword);

    // value3 is not an expected value, we return.
    if (!value3)
        return;

    valueList->next();

    bool swapNeeded = false;
    CSSValueID ident1 = parsedValue1->getValueID();
    CSSValueID ident2 = parsedValue2->getValueID();
    CSSValueID ident3 = value3->getValueID();

    CSSValueID firstPositionKeyword;
    CSSValueID secondPositionKeyword;

    if (ident1 == CSSValueCenter) {
        // <position> requires the first 'center' to be followed by a keyword.
        if (!isFillPositionKeyword(ident2))
            return;

        // If 'center' is the first keyword then the last one needs to be a length.
        if (isFillPositionKeyword(ident3))
            return;

        firstPositionKeyword = CSSValueLeft;
        if (ident2 == CSSValueLeft || ident2 == CSSValueRight) {
            firstPositionKeyword = CSSValueTop;
            swapNeeded = true;
        }
        value1 = createPrimitiveValuePair(cssValuePool().createIdentifierValue(firstPositionKeyword), cssValuePool().createValue(50, CSSPrimitiveValue::CSS_PERCENTAGE));
        value2 = createPrimitiveValuePair(parsedValue2, value3);
    } else if (ident3 == CSSValueCenter) {
        if (isFillPositionKeyword(ident2))
            return;

        secondPositionKeyword = CSSValueTop;
        if (ident1 == CSSValueTop || ident1 == CSSValueBottom) {
            secondPositionKeyword = CSSValueLeft;
            swapNeeded = true;
        }
        value1 = createPrimitiveValuePair(parsedValue1, parsedValue2);
        value2 = createPrimitiveValuePair(cssValuePool().createIdentifierValue(secondPositionKeyword), cssValuePool().createValue(50, CSSPrimitiveValue::CSS_PERCENTAGE));
    } else {
        RefPtr<CSSPrimitiveValue> firstPositionValue;
        RefPtr<CSSPrimitiveValue> secondPositionValue;

        if (isFillPositionKeyword(ident2)) {
            // To match CSS grammar, we should only accept: [ center | left | right | bottom | top ] [ left | right | top | bottom ] [ <percentage> | <length> ].
            ASSERT(ident2 != CSSValueCenter);

            if (isFillPositionKeyword(ident3))
                return;

            secondPositionValue = value3;
            secondPositionKeyword = ident2;
            firstPositionValue = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_PERCENTAGE);
        } else {
            // Per CSS, we should only accept: [ right | left | top | bottom ] [ <percentage> | <length> ] [ center | left | right | bottom | top ].
            if (!isFillPositionKeyword(ident3))
                return;

            firstPositionValue = parsedValue2;
            secondPositionKeyword = ident3;
            secondPositionValue = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_PERCENTAGE);
        }

        if (isValueConflictingWithCurrentEdge(ident1, secondPositionKeyword))
            return;

        value1 = createPrimitiveValuePair(parsedValue1, firstPositionValue);
        value2 = createPrimitiveValuePair(cssValuePool().createIdentifierValue(secondPositionKeyword), secondPositionValue);
    }

    if (ident1 == CSSValueTop || ident1 == CSSValueBottom || swapNeeded)
        value1.swap(value2);

#ifndef NDEBUG
    CSSPrimitiveValue* first = static_cast<CSSPrimitiveValue*>(value1.get());
    CSSPrimitiveValue* second = static_cast<CSSPrimitiveValue*>(value2.get());
    ident1 = first->getPairValue()->first()->getValueID();
    ident2 = second->getPairValue()->first()->getValueID();
    ASSERT(ident1 == CSSValueLeft || ident1 == CSSValueRight);
    ASSERT(ident2 == CSSValueBottom || ident2 == CSSValueTop);
#endif
}

inline bool CSSParser::isPotentialPositionValue(CSSParserValue* value)
{
    return isFillPositionKeyword(value->id) || validUnit(value, FPercent | FLength, ReleaseParsedCalcValue);
}

void CSSParser::parseFillPosition(CSSParserValueList* valueList, RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2)
{
    unsigned numberOfValues = 0;
    for (unsigned i = valueList->currentIndex(); i < valueList->size(); ++i, ++numberOfValues) {
        CSSParserValue* current = valueList->valueAt(i);
        if (isComma(current) || !current || isForwardSlashOperator(current) || !isPotentialPositionValue(current))
            break;
    }

    if (numberOfValues > 4)
        return;

    // If we are parsing two values, we can safely call the CSS 2.1 parsing function and return.
    if (numberOfValues <= 2) {
        parse2ValuesFillPosition(valueList, value1, value2);
        return;
    }

    ASSERT(numberOfValues > 2 && numberOfValues <= 4);

    CSSParserValue* value = valueList->current();

    // <position> requires the first value to be a background keyword.
    if (!isFillPositionKeyword(value->id))
        return;

    // Parse the first value. We're just making sure that it is one of the valid keywords or a percentage/length.
    unsigned cumulativeFlags = 0;
    FillPositionFlag value1Flag = InvalidFillPosition;
    FillPositionFlag value2Flag = InvalidFillPosition;
    value1 = parseFillPositionComponent(valueList, cumulativeFlags, value1Flag, ResolveValuesAsKeyword);
    if (!value1)
        return;

    value = valueList->next();

    // In case we are parsing more than two values, relax the check inside of parseFillPositionComponent. top 20px is
    // a valid start for <position>.
    cumulativeFlags = AmbiguousFillPosition;
    value2 = parseFillPositionComponent(valueList, cumulativeFlags, value2Flag, ResolveValuesAsKeyword);
    if (value2)
        valueList->next();
    else {
        value1.clear();
        return;
    }

    RefPtr<CSSPrimitiveValue> parsedValue1 = static_cast<CSSPrimitiveValue*>(value1.get());
    RefPtr<CSSPrimitiveValue> parsedValue2 = static_cast<CSSPrimitiveValue*>(value2.get());

    value1.clear();
    value2.clear();

    // Per CSS3 syntax, <position> can't have 'center' as its second keyword as we have more arguments to follow.
    if (parsedValue2->getValueID() == CSSValueCenter)
        return;

    if (numberOfValues == 3)
        parse3ValuesFillPosition(valueList, value1, value2, parsedValue1.release(), parsedValue2.release());
    else
        parse4ValuesFillPosition(valueList, value1, value2, parsedValue1.release(), parsedValue2.release());
}

void CSSParser::parse2ValuesFillPosition(CSSParserValueList* valueList, RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2)
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
    if (isComma(value))
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
        // Only one value was specified. If that value was not a keyword, then it sets the x position, and the y position
        // is simply 50%. This is our default.
        // For keywords, the keyword was either an x-keyword (left/right), a y-keyword (top/bottom), or an ambiguous keyword (center).
        // For left/right/center, the default of 50% in the y is still correct.
        value2 = cssValuePool().createValue(50, CSSPrimitiveValue::CSS_PERCENTAGE);

    if (value1Flag == YFillPosition || value2Flag == XFillPosition)
        value1.swap(value2);
}

void CSSParser::parseFillRepeat(RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2)
{
    CSSValueID id = m_valueList->current()->id;
    if (id == CSSValueRepeatX) {
        m_implicitShorthand = true;
        value1 = cssValuePool().createIdentifierValue(CSSValueRepeat);
        value2 = cssValuePool().createIdentifierValue(CSSValueNoRepeat);
        m_valueList->next();
        return;
    }
    if (id == CSSValueRepeatY) {
        m_implicitShorthand = true;
        value1 = cssValuePool().createIdentifierValue(CSSValueNoRepeat);
        value2 = cssValuePool().createIdentifierValue(CSSValueRepeat);
        m_valueList->next();
        return;
    }
    if (id == CSSValueRepeat || id == CSSValueNoRepeat || id == CSSValueRound || id == CSSValueSpace)
        value1 = cssValuePool().createIdentifierValue(id);
    else {
        value1 = 0;
        return;
    }

    CSSParserValue* value = m_valueList->next();

    // Parse the second value if one is available
    if (value && !isComma(value)) {
        id = value->id;
        if (id == CSSValueRepeat || id == CSSValueNoRepeat || id == CSSValueRound || id == CSSValueSpace) {
            value2 = cssValuePool().createIdentifierValue(id);
            m_valueList->next();
            return;
        }
    }

    // If only one value was specified, value2 is the same as value1.
    m_implicitShorthand = true;
    value2 = cssValuePool().createIdentifierValue(static_cast<CSSPrimitiveValue*>(value1.get())->getValueID());
}

PassRefPtr<CSSValue> CSSParser::parseFillSize(CSSPropertyID propId, bool& allowComma)
{
    allowComma = true;
    CSSParserValue* value = m_valueList->current();

    if (value->id == CSSValueContain || value->id == CSSValueCover)
        return cssValuePool().createIdentifierValue(value->id);

    RefPtr<CSSPrimitiveValue> parsedValue1;

    if (value->id == CSSValueAuto)
        parsedValue1 = cssValuePool().createIdentifierValue(CSSValueAuto);
    else {
        if (!validUnit(value, FLength | FPercent))
            return 0;
        parsedValue1 = createPrimitiveNumericValue(value);
    }

    RefPtr<CSSPrimitiveValue> parsedValue2;
    if ((value = m_valueList->next())) {
        if (value->unit == CSSParserValue::Operator && value->iValue == ',')
            allowComma = false;
        else if (value->id != CSSValueAuto) {
            if (!validUnit(value, FLength | FPercent)) {
                if (!inShorthand())
                    return 0;
                // We need to rewind the value list, so that when it is advanced we'll end up back at this value.
                m_valueList->previous();
            } else
                parsedValue2 = createPrimitiveNumericValue(value);
        }
    } else if (!parsedValue2 && propId == CSSPropertyWebkitBackgroundSize) {
        // For backwards compatibility we set the second value to the first if it is omitted.
        // We only need to do this for -webkit-background-size. It should be safe to let masks match
        // the real property.
        parsedValue2 = parsedValue1;
    }

    if (!parsedValue2)
        return parsedValue1;
    return createPrimitiveValuePair(parsedValue1.release(), parsedValue2.release());
}

bool CSSParser::parseFillProperty(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2,
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
            if (!isComma(val))
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
                    if (val->id == CSSValueScroll || val->id == CSSValueFixed || val->id == CSSValueLocal) {
                        currValue = cssValuePool().createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundImage:
                case CSSPropertyWebkitMaskImage:
                    if (parseFillImage(m_valueList.get(), currValue))
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
                        currValue = cssValuePool().createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundClip:
                    if (parseBackgroundClip(val, currValue))
                        m_valueList->next();
                    break;
                case CSSPropertyBackgroundOrigin:
                    if (val->id == CSSValueBorderBox || val->id == CSSValuePaddingBox || val->id == CSSValueContentBox) {
                        currValue = cssValuePool().createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyBackgroundPosition:
                case CSSPropertyWebkitMaskPosition:
                    parseFillPosition(m_valueList.get(), currValue, currValue2);
                    // parseFillPosition advances the m_valueList pointer.
                    break;
                case CSSPropertyBackgroundPositionX:
                case CSSPropertyWebkitMaskPositionX: {
                    currValue = parseFillPositionX(m_valueList.get());
                    if (currValue)
                        m_valueList->next();
                    break;
                }
                case CSSPropertyBackgroundPositionY:
                case CSSPropertyWebkitMaskPositionY: {
                    currValue = parseFillPositionY(m_valueList.get());
                    if (currValue)
                        m_valueList->next();
                    break;
                }
                case CSSPropertyWebkitBackgroundComposite:
                case CSSPropertyWebkitMaskComposite:
                    if (val->id >= CSSValueClear && val->id <= CSSValuePlusLighter) {
                        currValue = cssValuePool().createIdentifierValue(val->id);
                        m_valueList->next();
                    }
                    break;
                case CSSPropertyWebkitBackgroundBlendMode:
                    if (cssCompositingEnabled() && (val->id == CSSValueNormal || val->id == CSSValueMultiply
                        || val->id == CSSValueScreen || val->id == CSSValueOverlay || val->id == CSSValueDarken
                        || val->id == CSSValueLighten ||  val->id == CSSValueColorDodge || val->id == CSSValueColorBurn
                        || val->id == CSSValueHardLight || val->id == CSSValueSoftLight || val->id == CSSValueDifference
                        || val->id == CSSValueExclusion || val->id == CSSValueHue || val->id == CSSValueSaturation
                        || val->id == CSSValueColor || val->id == CSSValueLuminosity)) {
                        currValue = cssValuePool().createIdentifierValue(val->id);
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
                default:
                    break;
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
    if (validUnit(value, FTime))
        return createPrimitiveNumericValue(value);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationDirection()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNormal || value->id == CSSValueAlternate || value->id == CSSValueReverse || value->id == CSSValueAlternateReverse)
        return cssValuePool().createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationDuration()
{
    CSSParserValue* value = m_valueList->current();
    if (validUnit(value, FTime | FNonNeg))
        return createPrimitiveNumericValue(value);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationFillMode()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNone || value->id == CSSValueForwards || value->id == CSSValueBackwards || value->id == CSSValueBoth)
        return cssValuePool().createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationIterationCount()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueInfinite)
        return cssValuePool().createIdentifierValue(value->id);
    if (validUnit(value, FNumber | FNonNeg))
        return createPrimitiveNumericValue(value);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationName()
{
    CSSParserValue* value = m_valueList->current();
    if (value->unit == CSSPrimitiveValue::CSS_STRING || value->unit == CSSPrimitiveValue::CSS_IDENT) {
        if (value->id == CSSValueNone || (value->unit == CSSPrimitiveValue::CSS_STRING && equalIgnoringCase(value, "none"))) {
            return cssValuePool().createIdentifierValue(CSSValueNone);
        } else {
            return createPrimitiveStringValue(value);
        }
    }
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationPlayState()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueRunning || value->id == CSSValuePaused)
        return cssValuePool().createIdentifierValue(value->id);
    return 0;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationProperty(AnimationParseContext& context)
{
    CSSParserValue* value = m_valueList->current();
    if (value->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;
    CSSPropertyID result = cssPropertyID(value->string);
    if (result)
        return cssValuePool().createIdentifierValue(result);
    if (equalIgnoringCase(value, "all")) {
        context.sawAnimationPropertyKeyword();
        return cssValuePool().createIdentifierValue(CSSValueAll);
    }
    if (equalIgnoringCase(value, "none")) {
        context.commitAnimationPropertyKeyword();
        context.sawAnimationPropertyKeyword();
        return cssValuePool().createIdentifierValue(CSSValueNone);
    }
    return 0;
}

bool CSSParser::parseTransformOriginShorthand(RefPtr<CSSValue>& value1, RefPtr<CSSValue>& value2, RefPtr<CSSValue>& value3)
{
    parse2ValuesFillPosition(m_valueList.get(), value1, value2);

    // now get z
    if (m_valueList->current()) {
        if (validUnit(m_valueList->current(), FLength)) {
            value3 = createPrimitiveNumericValue(m_valueList->current());
            m_valueList->next();
            return true;
        }
        return false;
    }
    value3 = cssValuePool().createImplicitInitialValue();
    return true;
}

bool CSSParser::parseCubicBezierTimingFunctionValue(CSSParserValueList*& args, double& result)
{
    CSSParserValue* v = args->current();
    if (!validUnit(v, FNumber))
        return false;
    result = v->fValue;
    v = args->next();
    if (!v)
        // The last number in the function has no comma after it, so we're done.
        return true;
    if (!isComma(v))
        return false;
    args->next();
    return true;
}

PassRefPtr<CSSValue> CSSParser::parseAnimationTimingFunction()
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueEase || value->id == CSSValueLinear || value->id == CSSValueEaseIn || value->id == CSSValueEaseOut
        || value->id == CSSValueEaseInOut || value->id == CSSValueStepStart || value->id == CSSValueStepEnd)
        return cssValuePool().createIdentifierValue(value->id);

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
        if (!validUnit(v, FInteger))
            return 0;
        numSteps = clampToInteger(v->fValue);
        if (numSteps < 1)
            return 0;
        v = args->next();

        if (v) {
            // There is a comma so we need to parse the second value
            if (!isComma(v))
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

        // There are two points specified. The x values must be between 0 and 1 but the y values can exceed this range.
        double x1, y1, x2, y2;

        if (!parseCubicBezierTimingFunctionValue(args, x1))
            return 0;
        if (x1 < 0 || x1 > 1)
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, y1))
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, x2))
            return 0;
        if (x2 < 0 || x2 > 1)
            return 0;
        if (!parseCubicBezierTimingFunctionValue(args, y2))
            return 0;

        return CSSCubicBezierTimingFunctionValue::create(x1, y1, x2, y2);
    }

    return 0;
}

bool CSSParser::parseAnimationProperty(CSSPropertyID propId, RefPtr<CSSValue>& result, AnimationParseContext& context)
{
    RefPtr<CSSValueList> values;
    CSSParserValue* val;
    RefPtr<CSSValue> value;
    bool allowComma = false;

    result = 0;

    while ((val = m_valueList->current())) {
        RefPtr<CSSValue> currValue;
        if (allowComma) {
            if (!isComma(val))
                return false;
            m_valueList->next();
            allowComma = false;
        }
        else {
            switch (propId) {
                case CSSPropertyWebkitAnimationDelay:
                case CSSPropertyTransitionDelay:
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
                case CSSPropertyTransitionDuration:
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
                case CSSPropertyTransitionProperty:
                case CSSPropertyWebkitTransitionProperty:
                    currValue = parseAnimationProperty(context);
                    if (value && !context.animationPropertyKeywordAllowed())
                        return false;
                    if (currValue)
                        m_valueList->next();
                    break;
                case CSSPropertyWebkitAnimationTimingFunction:
                case CSSPropertyTransitionTimingFunction:
                case CSSPropertyWebkitTransitionTimingFunction:
                    currValue = parseAnimationTimingFunction();
                    if (currValue)
                        m_valueList->next();
                    break;
                default:
                    ASSERT_NOT_REACHED();
                    return false;
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

bool CSSParser::parseGridItemPositionShorthand(CSSPropertyID shorthandId, bool important)
{
    ShorthandScope scope(this, shorthandId);
    const StylePropertyShorthand& shorthand = shorthandForProperty(shorthandId);
    ASSERT(shorthand.length() == 2);
    if (!parseValue(shorthand.properties()[0], important))
        return false;

    if (!m_valueList->current()) {
        // Only one value was specified, the opposite value should be set to 'auto'.
        // FIXME: If the first property was <ident>, the opposite value should be the same <ident>.
        addProperty(shorthand.properties()[1], cssValuePool().createIdentifierValue(CSSValueAuto), important);
        return true;
    }

    if (!isForwardSlashOperator(m_valueList->current()))
        return false;

    if (!m_valueList->next())
        return false;

    return parseValue(shorthand.properties()[1], important);
}

bool CSSParser::parseGridTrackList(CSSPropertyID propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNone) {
        if (m_valueList->next())
            return false;

        addProperty(propId, cssValuePool().createIdentifierValue(value->id), important);
        return true;
    }

    RefPtr<CSSValueList> values = CSSValueList::createSpaceSeparated();
    while (m_valueList->current()) {
        RefPtr<CSSPrimitiveValue> primitiveValue = parseGridTrackSize();
        if (!primitiveValue)
            return false;

        values->append(primitiveValue.release());
    }
    addProperty(propId, values.release(), important);
    return true;
}

PassRefPtr<CSSPrimitiveValue> CSSParser::parseGridTrackSize()
{
    CSSParserValue* currentValue = m_valueList->current();
    m_valueList->next();

    if (currentValue->id == CSSValueAuto)
        return cssValuePool().createIdentifierValue(CSSValueAuto);

    if (currentValue->unit == CSSParserValue::Function && equalIgnoringCase(currentValue->function->name, "minmax(")) {
        // The spec defines the following grammar: minmax( <track-breadth> , <track-breadth> )
        CSSParserValueList* arguments = currentValue->function->args.get();
        if (!arguments || arguments->size() != 3 || !isComma(arguments->valueAt(1)))
            return 0;

        RefPtr<CSSPrimitiveValue> minTrackBreadth = parseGridBreadth(arguments->valueAt(0));
        if (!minTrackBreadth)
            return 0;

        RefPtr<CSSPrimitiveValue> maxTrackBreadth = parseGridBreadth(arguments->valueAt(2));
        if (!maxTrackBreadth)
            return 0;

        return createPrimitiveValuePair(minTrackBreadth, maxTrackBreadth);
    }

    if (PassRefPtr<CSSPrimitiveValue> trackBreadth = parseGridBreadth(currentValue))
        return trackBreadth;

    return 0;
}

PassRefPtr<CSSPrimitiveValue> CSSParser::parseGridBreadth(CSSParserValue* currentValue)
{
    if (currentValue->id == CSSValueWebkitMinContent || currentValue->id == CSSValueWebkitMaxContent)
        return cssValuePool().createIdentifierValue(currentValue->id);

    if (!validUnit(currentValue, FLength | FPercent))
        return 0;

    return createPrimitiveNumericValue(currentValue);
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

bool CSSParser::parseDashboardRegions(CSSPropertyID propId, bool important)
{
    bool valid = true;

    CSSParserValue* value = m_valueList->current();

    if (value->id == CSSValueNone) {
        if (m_valueList->next())
            return false;
        addProperty(propId, cssValuePool().createIdentifierValue(value->id), important);
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

        // Commas count as values, so allow (function name is dashboard-region for DASHBOARD_SUPPORT feature):
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

        if (equalIgnoringCase(arg, "circle"))
            region->m_isCircle = true;
        else if (equalIgnoringCase(arg, "rectangle"))
            region->m_isRectangle = true;
        else {
            valid = false;
            break;
        }

        region->m_geometryType = arg->string;

        if (numArgs == DASHBOARD_REGION_SHORT_NUM_PARAMETERS || numArgs == (DASHBOARD_REGION_SHORT_NUM_PARAMETERS*2-1)) {
            // This originally used CSSValueInvalid by accident. It might be more logical to use something else.
            RefPtr<CSSPrimitiveValue> amount = cssValuePool().createIdentifierValue(CSSValueInvalid);

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

                valid = arg->id == CSSValueAuto || validUnit(arg, FLength);
                if (!valid)
                    break;

                RefPtr<CSSPrimitiveValue> amount = arg->id == CSSValueAuto ?
                    cssValuePool().createIdentifierValue(CSSValueAuto) :
                    createPrimitiveNumericValue(arg);

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
        addProperty(propId, cssValuePool().createValue(firstRegion.release()), important);

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
    RefPtr<CSSPrimitiveValue> identifier = createPrimitiveStringValue(i);

    RefPtr<CSSPrimitiveValue> separator;
    if (!counters)
        separator = cssValuePool().createValue(String(), CSSPrimitiveValue::CSS_STRING);
    else {
        i = args->next();
        if (i->unit != CSSParserValue::Operator || i->iValue != ',')
            return 0;

        i = args->next();
        if (i->unit != CSSPrimitiveValue::CSS_STRING)
            return 0;

        separator = createPrimitiveStringValue(i);
    }

    RefPtr<CSSPrimitiveValue> listStyle;
    i = args->next();
    if (!i) // Make the list style default decimal
        listStyle = cssValuePool().createIdentifierValue(CSSValueDecimal);
    else {
        if (i->unit != CSSParserValue::Operator || i->iValue != ',')
            return 0;

        i = args->next();
        if (i->unit != CSSPrimitiveValue::CSS_IDENT)
            return 0;

        CSSValueID listStyleID = CSSValueInvalid;
        if (i->id == CSSValueNone || (i->id >= CSSValueDisc && i->id <= CSSValueKatakanaIroha))
            listStyleID = i->id;
        else
            return 0;

        listStyle = cssValuePool().createIdentifierValue(listStyleID);
    }

    return cssValuePool().createValue(Counter::create(identifier.release(), listStyle.release(), separator.release()));
}

bool CSSParser::parseClipShape(CSSPropertyID propId, bool important)
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
        valid = a->id == CSSValueAuto || validUnit(a, FLength);
        if (!valid)
            break;
        RefPtr<CSSPrimitiveValue> length = a->id == CSSValueAuto ?
            cssValuePool().createIdentifierValue(CSSValueAuto) :
            createPrimitiveNumericValue(a);
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
        addProperty(propId, cssValuePool().createValue(rect.release()), important);
        m_valueList->next();
        return true;
    }
    return false;
}

PassRefPtr<CSSBasicShape> CSSParser::parseBasicShapeRectangle(CSSParserValueList* args)
{
    ASSERT(args);

    // rect(x, y, width, height, [[rx], ry])
    if (args->size() != 7 && args->size() != 9 && args->size() != 11)
        return 0;

    RefPtr<CSSBasicShapeRectangle> shape = CSSBasicShapeRectangle::create();

    unsigned argumentNumber = 0;
    CSSParserValue* argument = args->current();
    while (argument) {
        Units unitFlags = FLength | FPercent;
        if (argumentNumber > 1) {
            // Arguments width, height, rx, and ry cannot be negative.
            unitFlags = unitFlags | FNonNeg;
        }
        if (!validUnit(argument, unitFlags))
            return 0;

        RefPtr<CSSPrimitiveValue> length = createPrimitiveNumericValue(argument);
        ASSERT(argumentNumber < 6);
        switch (argumentNumber) {
        case 0:
            shape->setX(length);
            break;
        case 1:
            shape->setY(length);
            break;
        case 2:
            shape->setWidth(length);
            break;
        case 3:
            shape->setHeight(length);
            break;
        case 4:
            shape->setRadiusX(length);
            break;
        case 5:
            shape->setRadiusY(length);
            break;
        }
        argument = args->next();
        if (argument) {
            if (!isComma(argument))
                return 0;

            argument = args->next();
        }
        argumentNumber++;
    }

    if (argumentNumber < 4)
        return 0;
    return shape;
}

PassRefPtr<CSSBasicShape> CSSParser::parseBasicShapeInsetRectangle(CSSParserValueList* args)
{
    ASSERT(args);

    // inset-rectangle(top, right, bottom, left, [[rx], ry])
    if (args->size() != 7 && args->size() != 9 && args->size() != 11)
        return 0;

    RefPtr<CSSBasicShapeInsetRectangle> shape = CSSBasicShapeInsetRectangle::create();

    unsigned argumentNumber = 0;
    CSSParserValue* argument = args->current();
    while (argument) {
        Units unitFlags = FLength | FPercent | FNonNeg;
        if (!validUnit(argument, unitFlags))
            return 0;

        RefPtr<CSSPrimitiveValue> length = createPrimitiveNumericValue(argument);
        ASSERT(argumentNumber < 6);
        switch (argumentNumber) {
        case 0:
            shape->setTop(length);
            break;
        case 1:
            shape->setRight(length);
            break;
        case 2:
            shape->setBottom(length);
            break;
        case 3:
            shape->setLeft(length);
            break;
        case 4:
            shape->setRadiusX(length);
            break;
        case 5:
            shape->setRadiusY(length);
            break;
        }
        argument = args->next();
        if (argument) {
            if (!isComma(argument))
                return 0;

            argument = args->next();
        }
        argumentNumber++;
    }

    if (argumentNumber < 4)
        return 0;
    return shape;
}

PassRefPtr<CSSBasicShape> CSSParser::parseBasicShapeCircle(CSSParserValueList* args)
{
    ASSERT(args);

    // circle(centerX, centerY, radius)
    if (args->size() != 5)
        return 0;

    RefPtr<CSSBasicShapeCircle> shape = CSSBasicShapeCircle::create();

    unsigned argumentNumber = 0;
    CSSParserValue* argument = args->current();
    while (argument) {
        Units unitFlags = FLength | FPercent;
        if (argumentNumber == 2) {
            // Argument radius cannot be negative.
            unitFlags = unitFlags | FNonNeg;
        }

        if (!validUnit(argument, unitFlags))
            return 0;

        RefPtr<CSSPrimitiveValue> length = createPrimitiveNumericValue(argument);
        ASSERT(argumentNumber < 3);
        switch (argumentNumber) {
        case 0:
            shape->setCenterX(length);
            break;
        case 1:
            shape->setCenterY(length);
            break;
        case 2:
            shape->setRadius(length);
            break;
        }

        argument = args->next();
        if (argument) {
            if (!isComma(argument))
                return 0;
            argument = args->next();
        }
        argumentNumber++;
    }

    if (argumentNumber < 3)
        return 0;
    return shape;
}

PassRefPtr<CSSBasicShape> CSSParser::parseBasicShapeEllipse(CSSParserValueList* args)
{
    ASSERT(args);

    // ellipse(centerX, centerY, radiusX, radiusY)
    if (args->size() != 7)
        return 0;

    RefPtr<CSSBasicShapeEllipse> shape = CSSBasicShapeEllipse::create();
    unsigned argumentNumber = 0;
    CSSParserValue* argument = args->current();
    while (argument) {
        Units unitFlags = FLength | FPercent;
        if (argumentNumber > 1) {
            // Arguments radiusX and radiusY cannot be negative.
            unitFlags = unitFlags | FNonNeg;
        }
        if (!validUnit(argument, unitFlags))
            return 0;

        RefPtr<CSSPrimitiveValue> length = createPrimitiveNumericValue(argument);
        ASSERT(argumentNumber < 4);
        switch (argumentNumber) {
        case 0:
            shape->setCenterX(length);
            break;
        case 1:
            shape->setCenterY(length);
            break;
        case 2:
            shape->setRadiusX(length);
            break;
        case 3:
            shape->setRadiusY(length);
            break;
        }

        argument = args->next();
        if (argument) {
            if (!isComma(argument))
                return 0;
            argument = args->next();
        }
        argumentNumber++;
    }

    if (argumentNumber < 4)
        return 0;
    return shape;
}

PassRefPtr<CSSBasicShape> CSSParser::parseBasicShapePolygon(CSSParserValueList* args)
{
    ASSERT(args);

    unsigned size = args->size();
    if (!size)
        return 0;

    RefPtr<CSSBasicShapePolygon> shape = CSSBasicShapePolygon::create();

    CSSParserValue* argument = args->current();
    if (argument->id == CSSValueEvenodd || argument->id == CSSValueNonzero) {
        shape->setWindRule(argument->id == CSSValueEvenodd ? RULE_EVENODD : RULE_NONZERO);

        if (!isComma(args->next()))
            return 0;

        argument = args->next();
        size -= 2;
    }

    // <length> <length>, ... <length> <length> -> each pair has 3 elements except the last one
    if (!size || (size % 3) - 2)
        return 0;

    CSSParserValue* argumentX = argument;
    while (argumentX) {
        if (!validUnit(argumentX, FLength | FPercent))
            return 0;

        CSSParserValue* argumentY = args->next();
        if (!argumentY || !validUnit(argumentY, FLength | FPercent))
            return 0;

        RefPtr<CSSPrimitiveValue> xLength = createPrimitiveNumericValue(argumentX);
        RefPtr<CSSPrimitiveValue> yLength = createPrimitiveNumericValue(argumentY);

        shape->appendPoint(xLength.release(), yLength.release());

        CSSParserValue* commaOrNull = args->next();
        if (!commaOrNull)
            argumentX = 0;
        else if (!isComma(commaOrNull)) 
            return 0;
        else 
            argumentX = args->next();
    }

    return shape;
}

bool CSSParser::parseBasicShape(CSSPropertyID propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    ASSERT(value->unit == CSSParserValue::Function);
    CSSParserValueList* args = value->function->args.get();

    if (!args)
        return false;

    RefPtr<CSSBasicShape> shape;
    if (equalIgnoringCase(value->function->name, "rectangle("))
        shape = parseBasicShapeRectangle(args);
    else if (equalIgnoringCase(value->function->name, "circle("))
        shape = parseBasicShapeCircle(args);
    else if (equalIgnoringCase(value->function->name, "ellipse("))
        shape = parseBasicShapeEllipse(args);
    else if (equalIgnoringCase(value->function->name, "polygon("))
        shape = parseBasicShapePolygon(args);
    else if (equalIgnoringCase(value->function->name, "inset-rectangle("))
        shape = parseBasicShapeInsetRectangle(args);

    if (!shape)
        return false;

    addProperty(propId, cssValuePool().createValue(shape.release()), important);
    m_valueList->next();
    return true;
}

// [ 'font-style' || 'font-variant' || 'font-weight' ]? 'font-size' [ / 'line-height' ]? 'font-family'
bool CSSParser::parseFont(bool important)
{
    // Let's check if there is an inherit or initial somewhere in the shorthand.
    for (unsigned i = 0; i < m_valueList->size(); ++i) {
        if (m_valueList->valueAt(i)->id == CSSValueInherit || m_valueList->valueAt(i)->id == CSSValueInitial)
            return false;
    }

    ShorthandScope scope(this, CSSPropertyFont);
    // Optional font-style, font-variant and font-weight.
    bool fontStyleParsed = false;
    bool fontVariantParsed = false;
    bool fontWeightParsed = false;
    CSSParserValue* value;
    while ((value = m_valueList->current())) {
        if (!fontStyleParsed && isValidKeywordPropertyAndValue(CSSPropertyFontStyle, value->id, m_context)) {
            addProperty(CSSPropertyFontStyle, cssValuePool().createIdentifierValue(value->id), important);
            fontStyleParsed = true;
        } else if (!fontVariantParsed && (value->id == CSSValueNormal || value->id == CSSValueSmallCaps)) {
            // Font variant in the shorthand is particular, it only accepts normal or small-caps.
            addProperty(CSSPropertyFontVariant, cssValuePool().createIdentifierValue(value->id), important);
            fontVariantParsed = true;
        } else if (!fontWeightParsed && parseFontWeight(important))
            fontWeightParsed = true;
        else
            break;
        m_valueList->next();
    }

    if (!value)
        return false;

    if (!fontStyleParsed)
        addProperty(CSSPropertyFontStyle, cssValuePool().createIdentifierValue(CSSValueNormal), important, true);
    if (!fontVariantParsed)
        addProperty(CSSPropertyFontVariant, cssValuePool().createIdentifierValue(CSSValueNormal), important, true);
    if (!fontWeightParsed)
        addProperty(CSSPropertyFontWeight, cssValuePool().createIdentifierValue(CSSValueNormal), important, true);

    // Now a font size _must_ come.
    // <absolute-size> | <relative-size> | <length> | <percentage> | inherit
    if (!parseFontSize(important))
        return false;

    value = m_valueList->current();
    if (!value)
        return false;

    if (isForwardSlashOperator(value)) {
        // The line-height property.
        value = m_valueList->next();
        if (!value)
            return false;
        if (!parseLineHeight(important))
            return false;
    } else
        addProperty(CSSPropertyLineHeight, cssValuePool().createIdentifierValue(CSSValueNormal), important, true);

    // Font family must come now.
    RefPtr<CSSValue> parsedFamilyValue = parseFontFamily();
    if (!parsedFamilyValue)
        return false;

    addProperty(CSSPropertyFontFamily, parsedFamilyValue.release(), important);

    // FIXME: http://www.w3.org/TR/2011/WD-css3-fonts-20110324/#font-prop requires that
    // "font-stretch", "font-size-adjust", and "font-kerning" be reset to their initial values
    // but we don't seem to support them at the moment. They should also be added here once implemented.
    if (m_valueList->current())
        return false;

    return true;
}

class FontFamilyValueBuilder {
public:
    FontFamilyValueBuilder(CSSValueList* list)
        : m_list(list)
    {
    }

    void add(const CSSParserString& string)
    {
        if (!m_builder.isEmpty())
            m_builder.append(' ');

        if (string.is8Bit()) {
            m_builder.append(string.characters8(), string.length());
            return;
        }

        m_builder.append(string.characters16(), string.length());
    }

    void commit()
    {
        if (m_builder.isEmpty())
            return;
        m_list->append(cssValuePool().createFontFamilyValue(m_builder.toString()));
        m_builder.clear();
    }

private:
    StringBuilder m_builder;
    CSSValueList* m_list;
};

PassRefPtr<CSSValueList> CSSParser::parseFontFamily()
{
    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    CSSParserValue* value = m_valueList->current();

    FontFamilyValueBuilder familyBuilder(list.get());
    bool inFamily = false;

    while (value) {
        CSSParserValue* nextValue = m_valueList->next();
        bool nextValBreaksFont = !nextValue ||
                                 (nextValue->unit == CSSParserValue::Operator && nextValue->iValue == ',');
        bool nextValIsFontName = nextValue &&
            ((nextValue->id >= CSSValueSerif && nextValue->id <= CSSValueWebkitBody) ||
            (nextValue->unit == CSSPrimitiveValue::CSS_STRING || nextValue->unit == CSSPrimitiveValue::CSS_IDENT));

        bool valueIsKeyword = value->id == CSSValueInitial || value->id == CSSValueInherit || value->id == CSSValueDefault;
        if (valueIsKeyword && !inFamily) {
            if (nextValBreaksFont)
                value = m_valueList->next();
            else if (nextValIsFontName)
                value = nextValue;
            continue;
        }

        if (value->id >= CSSValueSerif && value->id <= CSSValueWebkitBody) {
            if (inFamily)
                familyBuilder.add(value->string);
            else if (nextValBreaksFont || !nextValIsFontName)
                list->append(cssValuePool().createIdentifierValue(value->id));
            else {
                familyBuilder.commit();
                familyBuilder.add(value->string);
                inFamily = true;
            }
        } else if (value->unit == CSSPrimitiveValue::CSS_STRING) {
            // Strings never share in a family name.
            inFamily = false;
            familyBuilder.commit();
            list->append(cssValuePool().createFontFamilyValue(value->string));
        } else if (value->unit == CSSPrimitiveValue::CSS_IDENT) {
            if (inFamily)
                familyBuilder.add(value->string);
            else if (nextValBreaksFont || !nextValIsFontName)
                list->append(cssValuePool().createFontFamilyValue(value->string));
            else {
                familyBuilder.commit();
                familyBuilder.add(value->string);
                inFamily = true;
            }
        } else {
            break;
        }

        if (!nextValue)
            break;

        if (nextValBreaksFont) {
            value = m_valueList->next();
            familyBuilder.commit();
            inFamily = false;
        }
        else if (nextValIsFontName)
            value = nextValue;
        else
            break;
    }
    familyBuilder.commit();

    if (!list->length())
        list = 0;
    return list.release();
}

bool CSSParser::parseLineHeight(bool important)
{
    CSSParserValue* value = m_valueList->current();
    CSSValueID id = value->id;
    bool validPrimitive = false;
    // normal | <number> | <length> | <percentage> | inherit
    if (id == CSSValueNormal)
        validPrimitive = true;
    else
        validPrimitive = (!id && validUnit(value, FNumber | FLength | FPercent | FNonNeg));
    if (validPrimitive && (!m_valueList->next() || inShorthand()))
        addProperty(CSSPropertyLineHeight, parseValidPrimitive(id, value), important);
    return validPrimitive;
}

bool CSSParser::parseFontSize(bool important)
{
    CSSParserValue* value = m_valueList->current();
    CSSValueID id = value->id;
    bool validPrimitive = false;
    // <absolute-size> | <relative-size> | <length> | <percentage> | inherit
    if (id >= CSSValueXxSmall && id <= CSSValueLarger)
        validPrimitive = true;
    else
        validPrimitive = validUnit(value, FLength | FPercent | FNonNeg);
    if (validPrimitive && (!m_valueList->next() || inShorthand()))
        addProperty(CSSPropertyFontSize, parseValidPrimitive(id, value), important);
    return validPrimitive;
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
                parsedValue = cssValuePool().createIdentifierValue(val->id);
            else if (val->id == CSSValueAll && !values) {
                // 'all' is only allowed in @font-face and with no other values. Make a value list to
                // indicate that we are in the @font-face case.
                values = CSSValueList::createCommaSeparated();
                parsedValue = cssValuePool().createIdentifierValue(val->id);
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

static CSSValueID createFontWeightValueKeyword(int weight)
{
    ASSERT(!(weight % 100) && weight >= 100 && weight <= 900);
    CSSValueID value = static_cast<CSSValueID>(CSSValue100 + weight / 100 - 1);
    ASSERT(value >= CSSValue100 && value <= CSSValue900);
    return value;
}

bool CSSParser::parseFontWeight(bool important)
{
    CSSParserValue* value = m_valueList->current();
    if ((value->id >= CSSValueNormal) && (value->id <= CSSValue900)) {
        addProperty(CSSPropertyFontWeight, cssValuePool().createIdentifierValue(value->id), important);
        return true;
    }
    if (validUnit(value, FInteger | FNonNeg, CSSQuirksMode)) {
        int weight = static_cast<int>(value->fValue);
        if (!(weight % 100) && weight >= 100 && weight <= 900) {
            addProperty(CSSPropertyFontWeight, cssValuePool().createIdentifierValue(createFontWeightValueKeyword(weight)), important);
            return true;
        }
    }
    return false;
}

bool CSSParser::parseFontFaceSrcURI(CSSValueList* valueList)
{
    RefPtr<CSSFontFaceSrcValue> uriValue(CSSFontFaceSrcValue::create(completeURL(m_valueList->current()->string)));

    CSSParserValue* value = m_valueList->next();
    if (!value) {
        valueList->append(uriValue.release());
        return true;
    }
    if (value->unit == CSSParserValue::Operator && value->iValue == ',') {
        m_valueList->next();
        valueList->append(uriValue.release());
        return true;
    }

    if (value->unit != CSSParserValue::Function || !equalIgnoringCase(value->function->name, "format("))
        return false;

    // FIXME: http://www.w3.org/TR/2011/WD-css3-fonts-20111004/ says that format() contains a comma-separated list of strings,
    // but CSSFontFaceSrcValue stores only one format. Allowing one format for now.
    CSSParserValueList* args = value->function->args.get();
    if (!args || args->size() != 1 || (args->current()->unit != CSSPrimitiveValue::CSS_STRING && args->current()->unit != CSSPrimitiveValue::CSS_IDENT))
        return false;
    uriValue->setFormat(args->current()->string);
    valueList->append(uriValue.release());
    value = m_valueList->next();
    if (value && value->unit == CSSParserValue::Operator && value->iValue == ',')
        m_valueList->next();
    return true;
}

bool CSSParser::parseFontFaceSrcLocal(CSSValueList* valueList)
{
    CSSParserValueList* args = m_valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    if (args->size() == 1 && args->current()->unit == CSSPrimitiveValue::CSS_STRING)
        valueList->append(CSSFontFaceSrcValue::createLocal(args->current()->string));
    else if (args->current()->unit == CSSPrimitiveValue::CSS_IDENT) {
        StringBuilder builder;
        for (CSSParserValue* localValue = args->current(); localValue; localValue = args->next()) {
            if (localValue->unit != CSSPrimitiveValue::CSS_IDENT)
                return false;
            if (!builder.isEmpty())
                builder.append(' ');
            builder.append(localValue->string);
        }
        valueList->append(CSSFontFaceSrcValue::createLocal(builder.toString()));
    } else
        return false;

    if (CSSParserValue* value = m_valueList->next()) {
        if (value->unit == CSSParserValue::Operator && value->iValue == ',')
            m_valueList->next();
    }
    return true;
}

bool CSSParser::parseFontFaceSrc()
{
    RefPtr<CSSValueList> values(CSSValueList::createCommaSeparated());

    while (CSSParserValue* value = m_valueList->current()) {
        if (value->unit == CSSPrimitiveValue::CSS_URI) {
            if (!parseFontFaceSrcURI(values.get()))
                return false;
        } else if (value->unit == CSSParserValue::Function && equalIgnoringCase(value->function->name, "local(")) {
            if (!parseFontFaceSrcLocal(values.get()))
                return false;
        } else
            return false;
    }
    if (!values->length())
        return false;

    addProperty(CSSPropertySrc, values.release(), m_important);
    m_valueList->next();
    return true;
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
template <typename CharacterType>
static int checkForValidDouble(const CharacterType* string, const CharacterType* end, const char terminator)
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
template <typename CharacterType>
static int parseDouble(const CharacterType* string, const CharacterType* end, const char terminator, double& value)
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

template <typename CharacterType>
static bool parseColorIntOrPercentage(const CharacterType*& string, const CharacterType* end, const char terminator, CSSPrimitiveValue::UnitTypes& expect, int& value)
{
    const CharacterType* current = string;
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

template <typename CharacterType>
static inline bool isTenthAlpha(const CharacterType* string, const int length)
{
    // "0.X"
    if (length == 3 && string[0] == '0' && string[1] == '.' && isASCIIDigit(string[2]))
        return true;

    // ".X"
    if (length == 2 && string[0] == '.' && isASCIIDigit(string[1]))
        return true;

    return false;
}

template <typename CharacterType>
static inline bool parseAlphaValue(const CharacterType*& string, const CharacterType* end, const char terminator, int& value)
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

    if (string[length - 1] != terminator || !isASCIIDigit(string[length - 2]))
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

template <typename CharacterType>
static inline bool mightBeRGBA(const CharacterType* characters, unsigned length)
{
    if (length < 5)
        return false;
    return characters[4] == '('
        && isASCIIAlphaCaselessEqual(characters[0], 'r')
        && isASCIIAlphaCaselessEqual(characters[1], 'g')
        && isASCIIAlphaCaselessEqual(characters[2], 'b')
        && isASCIIAlphaCaselessEqual(characters[3], 'a');
}

template <typename CharacterType>
static inline bool mightBeRGB(const CharacterType* characters, unsigned length)
{
    if (length < 4)
        return false;
    return characters[3] == '('
        && isASCIIAlphaCaselessEqual(characters[0], 'r')
        && isASCIIAlphaCaselessEqual(characters[1], 'g')
        && isASCIIAlphaCaselessEqual(characters[2], 'b');
}

template <typename CharacterType>
static inline bool fastParseColorInternal(RGBA32& rgb, const CharacterType* characters, unsigned length , bool strict)
{
    CSSPrimitiveValue::UnitTypes expect = CSSPrimitiveValue::CSS_UNKNOWN;
    
    if (!strict && length >= 3) {
        if (characters[0] == '#') {
            if (Color::parseHexColor(characters + 1, length - 1, rgb))
                return true;
        } else {
            if (Color::parseHexColor(characters, length, rgb))
                return true;
        }
    }

    // Try rgba() syntax.
    if (mightBeRGBA(characters, length)) {
        const CharacterType* current = characters + 5;
        const CharacterType* end = characters + length;
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
        const CharacterType* current = characters + 4;
        const CharacterType* end = characters + length;
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

    return false;
}

template<typename StringType>
bool CSSParser::fastParseColor(RGBA32& rgb, const StringType& name, bool strict)
{
    unsigned length = name.length();
    bool parseResult;

    if (!length)
        return false;

    if (name.is8Bit())
        parseResult = fastParseColorInternal(rgb, name.characters8(), length, strict);
    else
        parseResult = fastParseColorInternal(rgb, name.characters16(), length, strict);

    if (parseResult)
        return true;

    // Try named colors.
    Color tc;
    tc.setNamedColor(name);
    if (tc.isValid()) {
        rgb = tc.rgb();
        return true;
    }
    return false;
}
    
inline double CSSParser::parsedDouble(CSSParserValue *v, ReleaseParsedCalcValueCondition releaseCalc)
{
    const double result = m_parsedCalculation ? m_parsedCalculation->doubleValue() : v->fValue;
    if (releaseCalc == ReleaseParsedCalcValue)
        m_parsedCalculation.release();
    return result;
}

bool CSSParser::isCalculation(CSSParserValue* value) 
{
    return (value->unit == CSSParserValue::Function) 
        && (equalIgnoringCase(value->function->name, "calc(")
            || equalIgnoringCase(value->function->name, "-webkit-calc(")
            || equalIgnoringCase(value->function->name, "-webkit-min(")
            || equalIgnoringCase(value->function->name, "-webkit-max("));
}

inline int CSSParser::colorIntFromValue(CSSParserValue* v)
{
    bool isPercent;
    
    if (m_parsedCalculation)
        isPercent = m_parsedCalculation->category() == CalcPercent;
    else
        isPercent = v->unit == CSSPrimitiveValue::CSS_PERCENTAGE;

    const double value = parsedDouble(v, ReleaseParsedCalcValue);
    
    if (value <= 0.0)
        return 0;

    if (isPercent) {
        if (value >= 100.0)
            return 255;
        return static_cast<int>(value * 256.0 / 100.0);
    }

    if (value >= 255.0)
        return 255;

    return static_cast<int>(value);
}

bool CSSParser::parseColorParameters(CSSParserValue* value, int* colorArray, bool parseAlpha)
{
    CSSParserValueList* args = value->function->args.get();
    CSSParserValue* v = args->current();
    Units unitType = FUnknown;
    // Get the first value and its type
    if (validUnit(v, FInteger, CSSStrictMode))
        unitType = FInteger;
    else if (validUnit(v, FPercent, CSSStrictMode))
        unitType = FPercent;
    else
        return false;
    
    colorArray[0] = colorIntFromValue(v);
    for (int i = 1; i < 3; i++) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, unitType, CSSStrictMode))
            return false;
        colorArray[i] = colorIntFromValue(v);
    }
    if (parseAlpha) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FNumber, CSSStrictMode))
            return false;
        const double value = parsedDouble(v, ReleaseParsedCalcValue);
        // Convert the floating pointer number of alpha to an integer in the range [0, 256),
        // with an equal distribution across all 256 values.
        colorArray[3] = static_cast<int>(max(0.0, min(1.0, value)) * nextafter(256.0, 0.0));
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
    if (!validUnit(v, FNumber, CSSStrictMode))
        return false;
    // normalize the Hue value and change it to be between 0 and 1.0
    colorArray[0] = (((static_cast<int>(parsedDouble(v, ReleaseParsedCalcValue)) % 360) + 360) % 360) / 360.0;
    for (int i = 1; i < 3; i++) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FPercent, CSSStrictMode))
            return false;
        colorArray[i] = max(0.0, min(100.0, parsedDouble(v, ReleaseParsedCalcValue))) / 100.0; // needs to be value between 0 and 1.0
    }
    if (parseAlpha) {
        v = args->next();
        if (v->unit != CSSParserValue::Operator && v->iValue != ',')
            return false;
        v = args->next();
        if (!validUnit(v, FNumber, CSSStrictMode))
            return false;
        colorArray[3] = max(0.0, min(1.0, parsedDouble(v, ReleaseParsedCalcValue)));
    }
    return true;
}

PassRefPtr<CSSPrimitiveValue> CSSParser::parseColor(CSSParserValue* value)
{
    RGBA32 c = Color::transparent;
    if (!parseColorFromValue(value ? value : m_valueList->current(), c))
        return 0;
    return cssValuePool().createColorValue(c);
}

bool CSSParser::parseColorFromValue(CSSParserValue* value, RGBA32& c)
{
    if (inQuirksMode() && value->unit == CSSPrimitiveValue::CSS_NUMBER
        && value->fValue >= 0. && value->fValue < 1000000.) {
        String str = String::format("%06d", static_cast<int>((value->fValue+.5)));
        // FIXME: This should be strict parsing for SVG as well.
        if (!fastParseColor(c, str, inStrictMode()))
            return false;
    } else if (value->unit == CSSPrimitiveValue::CSS_PARSER_HEXCOLOR ||
                value->unit == CSSPrimitiveValue::CSS_IDENT ||
                (inQuirksMode() && value->unit == CSSPrimitiveValue::CSS_DIMENSION)) {
        if (!fastParseColor(c, value->string, inStrictMode() && value->unit == CSSPrimitiveValue::CSS_IDENT))
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
    ShadowParseContext(CSSPropertyID prop, CSSParser* parser)
        : property(prop)
        , m_parser(parser)
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
        RefPtr<CSSPrimitiveValue> val = m_parser->createPrimitiveNumericValue(v);

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
        style = cssValuePool().createIdentifierValue(v->id);
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
    CSSParser* m_parser;

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

PassRefPtr<CSSValueList> CSSParser::parseShadow(CSSParserValueList* valueList, CSSPropertyID propId)
{
    ShadowParseContext context(propId, this);
    CSSParserValue* val;
    while ((val = valueList->current())) {
        // Check for a comma break first.
        if (val->unit == CSSParserValue::Operator) {
            if (val->iValue != ',' || !context.allowBreak)
                // Other operators aren't legal or we aren't done with the current shadow
                // value.  Treat as invalid.
                return 0;
#if ENABLE(SVG)
            // -webkit-svg-shadow does not support multiple values.
            if (propId == CSSPropertyWebkitSvgShadow)
                return 0;
#endif
            // The value is good.  Commit it.
            context.commitValue();
        } else if (validUnit(val, FLength, CSSStrictMode)) {
            // We required a length and didn't get one. Invalid.
            if (!context.allowLength())
                return 0;

            // Blur radius must be non-negative.
            if (context.allowBlur && !validUnit(val, FLength | FNonNeg, CSSStrictMode))
                return 0;

            // A length is allowed here.  Construct the value and add it.
            context.commitLength(val);
        } else if (val->id == CSSValueInset) {
            if (!context.allowStyle)
                return 0;

            context.commitStyle(val);
        } else {
            // The only other type of value that's ok is a color value.
            RefPtr<CSSPrimitiveValue> parsedColor;
            bool isColor = ((val->id >= CSSValueAqua && val->id <= CSSValueWindowtext) || val->id == CSSValueMenu
                            || (val->id >= CSSValueWebkitFocusRingColor && val->id <= CSSValueWebkitText && inQuirksMode())
                            || val->id == CSSValueCurrentcolor);
            if (isColor) {
                if (!context.allowColor)
                    return 0;
                parsedColor = cssValuePool().createIdentifierValue(val->id);
            }

            if (!parsedColor)
                // It's not built-in. Try to parse it as a color.
                parsedColor = parseColor(val);

            if (!parsedColor || !context.allowColor)
                return 0; // This value is not a color or length and is invalid or
                          // it is a color, but a color isn't allowed at this point.

            context.commitColor(parsedColor.release());
        }

        valueList->next();
    }

    if (context.allowBreak) {
        context.commitValue();
        if (context.values && context.values->length())
            return context.values.release();
    }

    return 0;
}

bool CSSParser::parseReflect(CSSPropertyID propId, bool important)
{
    // box-reflect: <direction> <offset> <mask>

    // Direction comes first.
    CSSParserValue* val = m_valueList->current();
    RefPtr<CSSPrimitiveValue> direction;
#if ENABLE(CSS_VARIABLES)
    if (val->unit == CSSPrimitiveValue::CSS_VARIABLE_NAME)
        direction = createPrimitiveVariableNameValue(val);
    else
#endif
    switch (val->id) {
        case CSSValueAbove:
        case CSSValueBelow:
        case CSSValueLeft:
        case CSSValueRight:
            direction = cssValuePool().createIdentifierValue(val->id);
            break;
        default:
            return false;
    }

    // The offset comes next.
    val = m_valueList->next();
    RefPtr<CSSPrimitiveValue> offset;
    if (!val)
        offset = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_PX);
    else {
        if (!validUnit(val, FLength | FPercent))
            return false;
        offset = createPrimitiveNumericValue(val);
    }

    // Now for the mask.
    RefPtr<CSSValue> mask;
    val = m_valueList->next();
    if (val) {
        if (!parseBorderImage(propId, mask))
            return false;
    }

    RefPtr<CSSReflectValue> reflectValue = CSSReflectValue::create(direction.release(), offset.release(), mask.release());
    addProperty(propId, reflectValue.release(), important);
    m_valueList->next();
    return true;
}

bool CSSParser::parseFlex(CSSParserValueList* args, bool important)
{
    if (!args || !args->size() || args->size() > 3)
        return false;
    static const double unsetValue = -1;
    double flexGrow = unsetValue;
    double flexShrink = unsetValue;
    RefPtr<CSSPrimitiveValue> flexBasis;

    while (CSSParserValue* arg = args->current()) {
        if (validUnit(arg, FNumber | FNonNeg)) {
            if (flexGrow == unsetValue)
                flexGrow = arg->fValue;
            else if (flexShrink == unsetValue)
                flexShrink = arg->fValue;
            else if (!arg->fValue) {
                // flex only allows a basis of 0 (sans units) if flex-grow and flex-shrink values have already been set.
                flexBasis = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_PX);
            } else {
                // We only allow 3 numbers without units if the last value is 0. E.g., flex:1 1 1 is invalid.
                return false;
            }
        } else if (!flexBasis && (arg->id == CSSValueAuto || validUnit(arg, FLength | FPercent | FNonNeg)))
            flexBasis = parseValidPrimitive(arg->id, arg);
        else {
            // Not a valid arg for flex.
            return false;
        }
        args->next();
    }

    if (flexGrow == unsetValue)
        flexGrow = 1;
    if (flexShrink == unsetValue)
        flexShrink = 1;
    if (!flexBasis)
        flexBasis = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_PX);

    addProperty(CSSPropertyWebkitFlexGrow, cssValuePool().createValue(clampToFloat(flexGrow), CSSPrimitiveValue::CSS_NUMBER), important);
    addProperty(CSSPropertyWebkitFlexShrink, cssValuePool().createValue(clampToFloat(flexShrink), CSSPrimitiveValue::CSS_NUMBER), important);
    addProperty(CSSPropertyWebkitFlexBasis, flexBasis, important);
    return true;
}

struct BorderImageParseContext {
    BorderImageParseContext()
    : m_canAdvance(false)
    , m_allowCommit(true)
    , m_allowImage(true)
    , m_allowImageSlice(true)
    , m_allowRepeat(true)
    , m_allowForwardSlashOperator(false)
    , m_requireWidth(false)
    , m_requireOutset(false)
    {}

    bool canAdvance() const { return m_canAdvance; }
    void setCanAdvance(bool canAdvance) { m_canAdvance = canAdvance; }

    bool allowCommit() const { return m_allowCommit; }
    bool allowImage() const { return m_allowImage; }
    bool allowImageSlice() const { return m_allowImageSlice; }
    bool allowRepeat() const { return m_allowRepeat; }
    bool allowForwardSlashOperator() const { return m_allowForwardSlashOperator; }

    bool requireWidth() const { return m_requireWidth; }
    bool requireOutset() const { return m_requireOutset; }

    void commitImage(PassRefPtr<CSSValue> image)
    {
        m_image = image;
        m_canAdvance = true;
        m_allowCommit = true;
        m_allowImage = m_allowForwardSlashOperator = m_requireWidth = m_requireOutset = false;
        m_allowImageSlice = !m_imageSlice;
        m_allowRepeat = !m_repeat;
    }
    void commitImageSlice(PassRefPtr<CSSBorderImageSliceValue> slice)
    {
        m_imageSlice = slice;
        m_canAdvance = true;
        m_allowCommit = m_allowForwardSlashOperator = true;
        m_allowImageSlice = m_requireWidth = m_requireOutset = false;
        m_allowImage = !m_image;
        m_allowRepeat = !m_repeat;
    }
    void commitForwardSlashOperator()
    {
        m_canAdvance = true;
        m_allowCommit = m_allowImage = m_allowImageSlice = m_allowRepeat = m_allowForwardSlashOperator = false;
        if (!m_borderSlice) {
            m_requireWidth = true;
            m_requireOutset = false;
        } else {
            m_requireOutset = true;
            m_requireWidth = false;
        }
    }
    void commitBorderWidth(PassRefPtr<CSSPrimitiveValue> slice)
    {
        m_borderSlice = slice;
        m_canAdvance = true;
        m_allowCommit = m_allowForwardSlashOperator = true;
        m_allowImageSlice = m_requireWidth = m_requireOutset = false;
        m_allowImage = !m_image;
        m_allowRepeat = !m_repeat;
    }
    void commitBorderOutset(PassRefPtr<CSSPrimitiveValue> outset)
    {
        m_outset = outset;
        m_canAdvance = true;
        m_allowCommit = true;
        m_allowImageSlice = m_allowForwardSlashOperator = m_requireWidth = m_requireOutset = false;
        m_allowImage = !m_image;
        m_allowRepeat = !m_repeat;
    }
    void commitRepeat(PassRefPtr<CSSValue> repeat)
    {
        m_repeat = repeat;
        m_canAdvance = true;
        m_allowCommit = true;
        m_allowRepeat = m_allowForwardSlashOperator = m_requireWidth = m_requireOutset = false;
        m_allowImageSlice = !m_imageSlice;
        m_allowImage = !m_image;
    }

    PassRefPtr<CSSValue> commitWebKitBorderImage()
    {
        return createBorderImageValue(m_image, m_imageSlice, m_borderSlice, m_outset, m_repeat);
    }

    void commitBorderImage(CSSParser* parser, bool important)
    {
        commitBorderImageProperty(CSSPropertyBorderImageSource, parser, m_image, important);
        commitBorderImageProperty(CSSPropertyBorderImageSlice, parser, m_imageSlice, important);
        commitBorderImageProperty(CSSPropertyBorderImageWidth, parser, m_borderSlice, important);
        commitBorderImageProperty(CSSPropertyBorderImageOutset, parser, m_outset, important);
        commitBorderImageProperty(CSSPropertyBorderImageRepeat, parser, m_repeat, important);
    }

    void commitBorderImageProperty(CSSPropertyID propId, CSSParser* parser, PassRefPtr<CSSValue> value, bool important)
    {
        if (value)
            parser->addProperty(propId, value, important);
        else
            parser->addProperty(propId, cssValuePool().createImplicitInitialValue(), important, true);
    }

    bool m_canAdvance;

    bool m_allowCommit;
    bool m_allowImage;
    bool m_allowImageSlice;
    bool m_allowRepeat;
    bool m_allowForwardSlashOperator;

    bool m_requireWidth;
    bool m_requireOutset;

    RefPtr<CSSValue> m_image;
    RefPtr<CSSBorderImageSliceValue> m_imageSlice;
    RefPtr<CSSPrimitiveValue> m_borderSlice;
    RefPtr<CSSPrimitiveValue> m_outset;

    RefPtr<CSSValue> m_repeat;
};

bool CSSParser::parseBorderImage(CSSPropertyID propId, RefPtr<CSSValue>& result, bool important)
{
    ShorthandScope scope(this, propId);
    BorderImageParseContext context;
    while (CSSParserValue* val = m_valueList->current()) {
        context.setCanAdvance(false);

        if (!context.canAdvance() && context.allowForwardSlashOperator() && isForwardSlashOperator(val))
            context.commitForwardSlashOperator();

        if (!context.canAdvance() && context.allowImage()) {
            if (val->unit == CSSPrimitiveValue::CSS_URI)
                context.commitImage(CSSImageValue::create(completeURL(val->string)));
            else if (isGeneratedImageValue(val)) {
                RefPtr<CSSValue> value;
                if (parseGeneratedImage(m_valueList.get(), value))
                    context.commitImage(value.release());
                else
                    return false;
#if ENABLE(CSS_IMAGE_SET)
            } else if (val->unit == CSSParserValue::Function && equalIgnoringCase(val->function->name, "-webkit-image-set(")) {
                RefPtr<CSSValue> value = parseImageSet();
                if (value)
                    context.commitImage(value.release());
                else
                    return false;
#endif
            } else if (val->id == CSSValueNone)
                context.commitImage(cssValuePool().createIdentifierValue(CSSValueNone));
        }

        if (!context.canAdvance() && context.allowImageSlice()) {
            RefPtr<CSSBorderImageSliceValue> imageSlice;
            if (parseBorderImageSlice(propId, imageSlice))
                context.commitImageSlice(imageSlice.release());
        }

        if (!context.canAdvance() && context.allowRepeat()) {
            RefPtr<CSSValue> repeat;
            if (parseBorderImageRepeat(repeat))
                context.commitRepeat(repeat.release());
        }

        if (!context.canAdvance() && context.requireWidth()) {
            RefPtr<CSSPrimitiveValue> borderSlice;
            if (parseBorderImageWidth(borderSlice))
                context.commitBorderWidth(borderSlice.release());
        }

        if (!context.canAdvance() && context.requireOutset()) {
            RefPtr<CSSPrimitiveValue> borderOutset;
            if (parseBorderImageOutset(borderOutset))
                context.commitBorderOutset(borderOutset.release());
        }

        if (!context.canAdvance())
            return false;

        m_valueList->next();
    }

    if (context.allowCommit()) {
        if (propId == CSSPropertyBorderImage)
            context.commitBorderImage(this, important);
        else
            // Need to fully commit as a single value.
            result = context.commitWebKitBorderImage();
        return true;
    }

    return false;
}

static bool isBorderImageRepeatKeyword(int id)
{
    return id == CSSValueStretch || id == CSSValueRepeat || id == CSSValueSpace || id == CSSValueRound;
}

bool CSSParser::parseBorderImageRepeat(RefPtr<CSSValue>& result)
{
    RefPtr<CSSPrimitiveValue> firstValue;
    RefPtr<CSSPrimitiveValue> secondValue;
    CSSParserValue* val = m_valueList->current();
    if (!val)
        return false;
    if (isBorderImageRepeatKeyword(val->id))
        firstValue = cssValuePool().createIdentifierValue(val->id);
    else
        return false;

    val = m_valueList->next();
    if (val) {
        if (isBorderImageRepeatKeyword(val->id))
            secondValue = cssValuePool().createIdentifierValue(val->id);
        else if (!inShorthand()) {
            // If we're not parsing a shorthand then we are invalid.
            return false;
        } else {
            // We need to rewind the value list, so that when its advanced we'll
            // end up back at this value.
            m_valueList->previous();
            secondValue = firstValue;
        }
    } else
        secondValue = firstValue;

    result = createPrimitiveValuePair(firstValue, secondValue);
    return true;
}

class BorderImageSliceParseContext {
public:
    BorderImageSliceParseContext(CSSParser* parser)
    : m_parser(parser)
    , m_allowNumber(true)
    , m_allowFill(true)
    , m_allowFinalCommit(false)
    , m_fill(false)
    { }

    bool allowNumber() const { return m_allowNumber; }
    bool allowFill() const { return m_allowFill; }
    bool allowFinalCommit() const { return m_allowFinalCommit; }
    CSSPrimitiveValue* top() const { return m_top.get(); }

    void commitNumber(CSSParserValue* v)
    {
        RefPtr<CSSPrimitiveValue> val = m_parser->createPrimitiveNumericValue(v);
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

        m_allowNumber = !m_left;
        m_allowFinalCommit = true;
    }

    void commitFill() { m_fill = true; m_allowFill = false; m_allowNumber = !m_top; }

    PassRefPtr<CSSBorderImageSliceValue> commitBorderImageSlice()
    {
        // We need to clone and repeat values for any omissions.
        ASSERT(m_top);
        if (!m_right) {
            m_right = m_top;
            m_bottom = m_top;
            m_left = m_top;
        }
        if (!m_bottom) {
            m_bottom = m_top;
            m_left = m_right;
        }
        if (!m_left)
            m_left = m_right;

        // Now build a rect value to hold all four of our primitive values.
        RefPtr<Quad> quad = Quad::create();
        quad->setTop(m_top);
        quad->setRight(m_right);
        quad->setBottom(m_bottom);
        quad->setLeft(m_left);

        // Make our new border image value now.
        return CSSBorderImageSliceValue::create(cssValuePool().createValue(quad.release()), m_fill);
    }

private:
    CSSParser* m_parser;

    bool m_allowNumber;
    bool m_allowFill;
    bool m_allowFinalCommit;

    RefPtr<CSSPrimitiveValue> m_top;
    RefPtr<CSSPrimitiveValue> m_right;
    RefPtr<CSSPrimitiveValue> m_bottom;
    RefPtr<CSSPrimitiveValue> m_left;

    bool m_fill;
};

bool CSSParser::parseBorderImageSlice(CSSPropertyID propId, RefPtr<CSSBorderImageSliceValue>& result)
{
    BorderImageSliceParseContext context(this);
    CSSParserValue* val;
    while ((val = m_valueList->current())) {
        // FIXME calc() http://webkit.org/b/16662 : calc is parsed but values are not created yet.
        if (context.allowNumber() && !isCalculation(val) && validUnit(val, FInteger | FNonNeg | FPercent, CSSStrictMode)) {
            context.commitNumber(val);
        } else if (context.allowFill() && val->id == CSSValueFill)
            context.commitFill();
        else if (!inShorthand()) {
            // If we're not parsing a shorthand then we are invalid.
            return false;
        } else {
            if (context.allowFinalCommit()) {
                // We're going to successfully parse, but we don't want to consume this token.
                m_valueList->previous();
            }
            break;
        }
        m_valueList->next();
    }

    if (context.allowFinalCommit()) {
        // FIXME: For backwards compatibility, -webkit-border-image, -webkit-mask-box-image and -webkit-box-reflect have to do a fill by default.
        // FIXME: What do we do with -webkit-box-reflect and -webkit-mask-box-image? Probably just have to leave them filling...
        if (propId == CSSPropertyWebkitBorderImage || propId == CSSPropertyWebkitMaskBoxImage || propId == CSSPropertyWebkitBoxReflect)
            context.commitFill();

        // Need to fully commit as a single value.
        result = context.commitBorderImageSlice();
        return true;
    }

    return false;
}

class BorderImageQuadParseContext {
public:
    BorderImageQuadParseContext(CSSParser* parser)
    : m_parser(parser)
    , m_allowNumber(true)
    , m_allowFinalCommit(false)
    { }

    bool allowNumber() const { return m_allowNumber; }
    bool allowFinalCommit() const { return m_allowFinalCommit; }
    CSSPrimitiveValue* top() const { return m_top.get(); }

    void commitNumber(CSSParserValue* v)
    {
        RefPtr<CSSPrimitiveValue> val;
        if (v->id == CSSValueAuto)
            val = cssValuePool().createIdentifierValue(v->id);
        else
            val = m_parser->createPrimitiveNumericValue(v);

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

        m_allowNumber = !m_left;
        m_allowFinalCommit = true;
    }

    void setAllowFinalCommit() { m_allowFinalCommit = true; }
    void setTop(PassRefPtr<CSSPrimitiveValue> val) { m_top = val; }

    PassRefPtr<CSSPrimitiveValue> commitBorderImageQuad()
    {
        // We need to clone and repeat values for any omissions.
        ASSERT(m_top);
        if (!m_right) {
            m_right = m_top;
            m_bottom = m_top;
            m_left = m_top;
        }
        if (!m_bottom) {
            m_bottom = m_top;
            m_left = m_right;
        }
        if (!m_left)
            m_left = m_right;

        // Now build a quad value to hold all four of our primitive values.
        RefPtr<Quad> quad = Quad::create();
        quad->setTop(m_top);
        quad->setRight(m_right);
        quad->setBottom(m_bottom);
        quad->setLeft(m_left);

        // Make our new value now.
        return cssValuePool().createValue(quad.release());
    }

private:
    CSSParser* m_parser;

    bool m_allowNumber;
    bool m_allowFinalCommit;

    RefPtr<CSSPrimitiveValue> m_top;
    RefPtr<CSSPrimitiveValue> m_right;
    RefPtr<CSSPrimitiveValue> m_bottom;
    RefPtr<CSSPrimitiveValue> m_left;
};

bool CSSParser::parseBorderImageQuad(Units validUnits, RefPtr<CSSPrimitiveValue>& result)
{
    BorderImageQuadParseContext context(this);
    CSSParserValue* val;
    while ((val = m_valueList->current())) {
        if (context.allowNumber() && (validUnit(val, validUnits, CSSStrictMode) || val->id == CSSValueAuto)) {
            context.commitNumber(val);
        } else if (!inShorthand()) {
            // If we're not parsing a shorthand then we are invalid.
            return false;
        } else {
            if (context.allowFinalCommit())
                m_valueList->previous(); // The shorthand loop will advance back to this point.
            break;
        }
        m_valueList->next();
    }

    if (context.allowFinalCommit()) {
        // Need to fully commit as a single value.
        result = context.commitBorderImageQuad();
        return true;
    }
    return false;
}

bool CSSParser::parseBorderImageWidth(RefPtr<CSSPrimitiveValue>& result)
{
    return parseBorderImageQuad(FLength | FInteger | FNonNeg | FPercent, result);
}

bool CSSParser::parseBorderImageOutset(RefPtr<CSSPrimitiveValue>& result)
{
    return parseBorderImageQuad(FLength | FInteger | FNonNeg, result);
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

bool CSSParser::parseBorderRadius(CSSPropertyID propId, bool important)
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

        if (!validUnit(value, FLength | FPercent | FNonNeg))
            return false;

        RefPtr<CSSPrimitiveValue> radius = createPrimitiveNumericValue(value);

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

    ImplicitScope implicitScope(this, PropertyImplicit);
    addProperty(CSSPropertyBorderTopLeftRadius, createPrimitiveValuePair(radii[0][0].release(), radii[1][0].release()), important);
    addProperty(CSSPropertyBorderTopRightRadius, createPrimitiveValuePair(radii[0][1].release(), radii[1][1].release()), important);
    addProperty(CSSPropertyBorderBottomRightRadius, createPrimitiveValuePair(radii[0][2].release(), radii[1][2].release()), important);
    addProperty(CSSPropertyBorderBottomLeftRadius, createPrimitiveValuePair(radii[0][3].release(), radii[1][3].release()), important);
    return true;
}

bool CSSParser::parseAspectRatio(bool important)
{
    unsigned num = m_valueList->size();
    if (num == 1 && m_valueList->valueAt(0)->id == CSSValueNone) {
        addProperty(CSSPropertyWebkitAspectRatio, cssValuePool().createIdentifierValue(CSSValueNone), important);
        return true;
    }

    if (num != 3)
        return false;

    CSSParserValue* lvalue = m_valueList->valueAt(0);
    CSSParserValue* op = m_valueList->valueAt(1);
    CSSParserValue* rvalue = m_valueList->valueAt(2);

    if (!isForwardSlashOperator(op))
        return false;

    if (!validUnit(lvalue, FNumber | FNonNeg) || !validUnit(rvalue, FNumber | FNonNeg))
        return false;

    if (!lvalue->fValue || !rvalue->fValue)
        return false;

    addProperty(CSSPropertyWebkitAspectRatio, CSSAspectRatioValue::create(narrowPrecisionToFloat(lvalue->fValue), narrowPrecisionToFloat(rvalue->fValue)), important);

    return true;
}

bool CSSParser::parseCounter(CSSPropertyID propId, int defaultValue, bool important)
{
    enum { ID, VAL } state = ID;

    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    RefPtr<CSSPrimitiveValue> counterName;

    while (true) {
        CSSParserValue* val = m_valueList->current();
        switch (state) {
            case ID:
                if (val && val->unit == CSSPrimitiveValue::CSS_IDENT) {
                    counterName = createPrimitiveStringValue(val);
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

                list->append(createPrimitiveValuePair(counterName.release(),
                    cssValuePool().createValue(i, CSSPrimitiveValue::CSS_NUMBER)));
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
static PassRefPtr<CSSPrimitiveValue> parseDeprecatedGradientPoint(CSSParserValue* a, bool horizontal)
{
    RefPtr<CSSPrimitiveValue> result;
    if (a->unit == CSSPrimitiveValue::CSS_IDENT) {
        if ((equalIgnoringCase(a, "left") && horizontal)
            || (equalIgnoringCase(a, "top") && !horizontal))
            result = cssValuePool().createValue(0., CSSPrimitiveValue::CSS_PERCENTAGE);
        else if ((equalIgnoringCase(a, "right") && horizontal)
                 || (equalIgnoringCase(a, "bottom") && !horizontal))
            result = cssValuePool().createValue(100., CSSPrimitiveValue::CSS_PERCENTAGE);
        else if (equalIgnoringCase(a, "center"))
            result = cssValuePool().createValue(50., CSSPrimitiveValue::CSS_PERCENTAGE);
    } else if (a->unit == CSSPrimitiveValue::CSS_NUMBER || a->unit == CSSPrimitiveValue::CSS_PERCENTAGE)
        result = cssValuePool().createValue(a->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(a->unit));
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

    if (equalIgnoringCase(a->function->name, "from(")
        || equalIgnoringCase(a->function->name, "to(")) {
        // The "from" and "to" stops expect 1 argument.
        if (args->size() != 1)
            return false;

        if (equalIgnoringCase(a->function->name, "from("))
            stop.m_position = cssValuePool().createValue(0, CSSPrimitiveValue::CSS_NUMBER);
        else
            stop.m_position = cssValuePool().createValue(1, CSSPrimitiveValue::CSS_NUMBER);

        CSSValueID id = args->current()->id;
        if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu)
            stop.m_color = cssValuePool().createIdentifierValue(id);
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
            stop.m_position = cssValuePool().createValue(stopArg->fValue / 100, CSSPrimitiveValue::CSS_NUMBER);
        else if (stopArg->unit == CSSPrimitiveValue::CSS_NUMBER)
            stop.m_position = cssValuePool().createValue(stopArg->fValue, CSSPrimitiveValue::CSS_NUMBER);
        else
            return false;

        stopArg = args->next();
        if (stopArg->unit != CSSParserValue::Operator || stopArg->iValue != ',')
            return false;

        stopArg = args->next();
        CSSValueID id = stopArg->id;
        if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu)
            stop.m_color = cssValuePool().createIdentifierValue(id);
        else
            stop.m_color = p->parseColor(stopArg);
        if (!stop.m_color)
            return false;
    }

    return true;
}

bool CSSParser::parseDeprecatedGradient(CSSParserValueList* valueList, RefPtr<CSSValue>& gradient)
{
    // Walk the arguments.
    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || args->size() == 0)
        return false;

    // The first argument is the gradient type.  It is an identifier.
    CSSGradientType gradientType;
    CSSParserValue* a = args->current();
    if (!a || a->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;
    if (equalIgnoringCase(a, "linear"))
        gradientType = CSSDeprecatedLinearGradient;
    else if (equalIgnoringCase(a, "radial"))
        gradientType = CSSDeprecatedRadialGradient;
    else
        return false;

    RefPtr<CSSGradientValue> result;
    switch (gradientType) {
    case CSSDeprecatedLinearGradient:
        result = CSSLinearGradientValue::create(NonRepeating, gradientType);
        break;
    case CSSDeprecatedRadialGradient:
        result = CSSRadialGradientValue::create(NonRepeating, gradientType);
        break;
    default:
        // The rest of the gradient types shouldn't appear here.
        ASSERT_NOT_REACHED();
    }

    // Comma.
    a = args->next();
    if (!isComma(a))
        return false;

    // Next comes the starting point for the gradient as an x y pair.  There is no
    // comma between the x and the y values.
    // First X.  It can be left, right, number or percent.
    a = args->next();
    if (!a)
        return false;
    RefPtr<CSSPrimitiveValue> point = parseDeprecatedGradientPoint(a, true);
    if (!point)
        return false;
    result->setFirstX(point.release());

    // First Y.  It can be top, bottom, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, false);
    if (!point)
        return false;
    result->setFirstY(point.release());

    // Comma after the first point.
    a = args->next();
    if (!isComma(a))
        return false;

    // For radial gradients only, we now expect a numeric radius.
    if (gradientType == CSSDeprecatedRadialGradient) {
        a = args->next();
        if (!a || a->unit != CSSPrimitiveValue::CSS_NUMBER)
            return false;
        static_cast<CSSRadialGradientValue*>(result.get())->setFirstRadius(createPrimitiveNumericValue(a));

        // Comma after the first radius.
        a = args->next();
        if (!isComma(a))
            return false;
    }

    // Next is the ending point for the gradient as an x, y pair.
    // Second X.  It can be left, right, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, true);
    if (!point)
        return false;
    result->setSecondX(point.release());

    // Second Y.  It can be top, bottom, number or percent.
    a = args->next();
    if (!a)
        return false;
    point = parseDeprecatedGradientPoint(a, false);
    if (!point)
        return false;
    result->setSecondY(point.release());

    // For radial gradients only, we now expect the second radius.
    if (gradientType == CSSDeprecatedRadialGradient) {
        // Comma after the second point.
        a = args->next();
        if (!isComma(a))
            return false;

        a = args->next();
        if (!a || a->unit != CSSPrimitiveValue::CSS_NUMBER)
            return false;
        static_cast<CSSRadialGradientValue*>(result.get())->setSecondRadius(createPrimitiveNumericValue(a));
    }

    // We now will accept any number of stops (0 or more).
    a = args->next();
    while (a) {
        // Look for the comma before the next stop.
        if (!isComma(a))
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

static PassRefPtr<CSSPrimitiveValue> valueFromSideKeyword(CSSParserValue* a, bool& isHorizontal)
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
    return cssValuePool().createIdentifierValue(a->id);
}

static PassRefPtr<CSSPrimitiveValue> parseGradientColorOrKeyword(CSSParser* p, CSSParserValue* value)
{
    CSSValueID id = value->id;
    if (id == CSSValueWebkitText || (id >= CSSValueAqua && id <= CSSValueWindowtext) || id == CSSValueMenu || id == CSSValueCurrentcolor)
        return cssValuePool().createIdentifierValue(id);

    return p->parseColor(value);
}

bool CSSParser::parseDeprecatedLinearGradient(CSSParserValueList* valueList, RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSLinearGradientValue> result = CSSLinearGradientValue::create(repeating, CSSPrefixedLinearGradient);

    // Walk the arguments.
    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;
    // Look for angle.
    if (validUnit(a, FAngle, CSSStrictMode)) {
        result->setAngle(createPrimitiveNumericValue(a));

        args->next();
        expectComma = true;
    } else {
        // Look one or two optional keywords that indicate a side or corner.
        RefPtr<CSSPrimitiveValue> startX, startY;

        RefPtr<CSSPrimitiveValue> location;
        bool isHorizontal = false;
        if ((location = valueFromSideKeyword(a, isHorizontal))) {
            if (isHorizontal)
                startX = location;
            else
                startY = location;

            if ((a = args->next())) {
                if ((location = valueFromSideKeyword(a, isHorizontal))) {
                    if (isHorizontal) {
                        if (startX)
                            return false;
                        startX = location;
                    } else {
                        if (startY)
                            return false;
                        startY = location;
                    }

                    args->next();
                }
            }

            expectComma = true;
        }

        if (!startX && !startY)
            startY = cssValuePool().createIdentifierValue(CSSValueTop);

        result->setFirstX(startX.release());
        result->setFirstY(startY.release());
    }

    if (!parseGradientColorStops(args, result.get(), expectComma))
        return false;

    if (!result->stopCount())
        return false;

    gradient = result.release();
    return true;
}

bool CSSParser::parseDeprecatedRadialGradient(CSSParserValueList* valueList, RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSRadialGradientValue> result = CSSRadialGradientValue::create(repeating, CSSPrefixedRadialGradient);

    // Walk the arguments.
    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;

    // Optional background-position
    RefPtr<CSSValue> centerX;
    RefPtr<CSSValue> centerY;
    // parse2ValuesFillPosition advances the args next pointer.
    parse2ValuesFillPosition(args, centerX, centerY);
    a = args->current();
    if (!a)
        return false;

    if (centerX || centerY) {
        // Comma
        if (!isComma(a))
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
            shapeValue = cssValuePool().createIdentifierValue(a->id);
            foundValue = true;
            break;
        case CSSValueClosestSide:
        case CSSValueClosestCorner:
        case CSSValueFarthestSide:
        case CSSValueFarthestCorner:
        case CSSValueContain:
        case CSSValueCover:
            sizeValue = cssValuePool().createIdentifierValue(a->id);
            foundValue = true;
            break;
        default:
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
        if (validUnit(a, FLength | FPercent)) {
            horizontalSize = createPrimitiveNumericValue(a);
            a = args->next();
            if (!a)
                return false;

            expectComma = true;
        }

        if (validUnit(a, FLength | FPercent)) {
            verticalSize = createPrimitiveNumericValue(a);

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

bool CSSParser::parseLinearGradient(CSSParserValueList* valueList, RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSLinearGradientValue> result = CSSLinearGradientValue::create(repeating, CSSLinearGradient);

    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;
    // Look for angle.
    if (validUnit(a, FAngle, CSSStrictMode)) {
        result->setAngle(createPrimitiveNumericValue(a));

        args->next();
        expectComma = true;
    } else if (a->unit == CSSPrimitiveValue::CSS_IDENT && equalIgnoringCase(a, "to")) {
        // to [ [left | right] || [top | bottom] ]
        a = args->next();
        if (!a)
            return false;

        RefPtr<CSSPrimitiveValue> endX, endY;
        RefPtr<CSSPrimitiveValue> location;
        bool isHorizontal = false;

        location = valueFromSideKeyword(a, isHorizontal);
        if (!location)
            return false;

        if (isHorizontal)
            endX = location;
        else
            endY = location;

        a = args->next();
        if (!a)
            return false;

        location = valueFromSideKeyword(a, isHorizontal);
        if (location) {
            if (isHorizontal) {
                if (endX)
                    return false;
                endX = location;
            } else {
                if (endY)
                    return false;
                endY = location;
            }

            args->next();
        }

        expectComma = true;
        result->setFirstX(endX.release());
        result->setFirstY(endY.release());
    }

    if (!parseGradientColorStops(args, result.get(), expectComma))
        return false;

    if (!result->stopCount())
        return false;

    gradient = result.release();
    return true;
}

bool CSSParser::parseRadialGradient(CSSParserValueList* valueList, RefPtr<CSSValue>& gradient, CSSGradientRepeat repeating)
{
    RefPtr<CSSRadialGradientValue> result = CSSRadialGradientValue::create(repeating, CSSRadialGradient);

    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || !args->size())
        return false;

    CSSParserValue* a = args->current();
    if (!a)
        return false;

    bool expectComma = false;

    RefPtr<CSSPrimitiveValue> shapeValue;
    RefPtr<CSSPrimitiveValue> sizeValue;
    RefPtr<CSSPrimitiveValue> horizontalSize;
    RefPtr<CSSPrimitiveValue> verticalSize;

    // First part of grammar, the size/shape clause:
    // [ circle || <length> ] |
    // [ ellipse || [ <length> | <percentage> ]{2} ] |
    // [ [ circle | ellipse] || <size-keyword> ]
    for (int i = 0; i < 3; ++i) {
        if (a->unit == CSSPrimitiveValue::CSS_IDENT) {
            bool badIdent = false;
            switch (a->id) {
            case CSSValueCircle:
            case CSSValueEllipse:
                if (shapeValue)
                    return false;
                shapeValue = cssValuePool().createIdentifierValue(a->id);
                break;
            case CSSValueClosestSide:
            case CSSValueClosestCorner:
            case CSSValueFarthestSide:
            case CSSValueFarthestCorner:
                if (sizeValue || horizontalSize)
                    return false;
                sizeValue = cssValuePool().createIdentifierValue(a->id);
                break;
            default:
                badIdent = true;
            }

            if (badIdent)
                break;

            a = args->next();
            if (!a)
                return false;
        } else if (validUnit(a, FLength | FPercent)) {

            if (sizeValue || horizontalSize)
                return false;
            horizontalSize = createPrimitiveNumericValue(a);

            a = args->next();
            if (!a)
                return false;

            if (validUnit(a, FLength | FPercent)) {
                verticalSize = createPrimitiveNumericValue(a);
                ++i;
                a = args->next();
                if (!a)
                    return false;
            }
        } else
            break;
    }

    // You can specify size as a keyword or a length/percentage, not both.
    if (sizeValue && horizontalSize)
        return false;
    // Circles must have 0 or 1 lengths.
    if (shapeValue && shapeValue->getValueID() == CSSValueCircle && verticalSize)
        return false;
    // Ellipses must have 0 or 2 length/percentages.
    if (shapeValue && shapeValue->getValueID() == CSSValueEllipse && horizontalSize && !verticalSize)
        return false;
    // If there's only one size, it must be a length.
    if (!verticalSize && horizontalSize && horizontalSize->isPercentage())
        return false;

    result->setShape(shapeValue);
    result->setSizingBehavior(sizeValue);
    result->setEndHorizontalSize(horizontalSize);
    result->setEndVerticalSize(verticalSize);

    // Second part of grammar, the center-position clause:
    // at <position>
    RefPtr<CSSValue> centerX;
    RefPtr<CSSValue> centerY;
    if (a->unit == CSSPrimitiveValue::CSS_IDENT && equalIgnoringCase(a, "at")) {
        a = args->next();
        if (!a)
            return false;

        parseFillPosition(args, centerX, centerY);
        ASSERT(centerX->isPrimitiveValue());
        ASSERT(centerY->isPrimitiveValue());
        if (!(centerX && centerY))
            return false;

        a = args->current();
        if (!a)
            return false;
        result->setFirstX(static_cast<CSSPrimitiveValue*>(centerX.get()));
        result->setFirstY(static_cast<CSSPrimitiveValue*>(centerY.get()));
        // Right now, CSS radial gradients have the same start and end centers.
        result->setSecondX(static_cast<CSSPrimitiveValue*>(centerX.get()));
        result->setSecondY(static_cast<CSSPrimitiveValue*>(centerY.get()));
    }

    if (shapeValue || sizeValue || horizontalSize || centerX || centerY)
        expectComma = true;

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
            if (!isComma(a))
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
            if (validUnit(a, FLength | FPercent)) {
                stop.m_position = createPrimitiveNumericValue(a);
                a = valueList->next();
            }
        }

        gradient->addStop(stop);
        expectComma = true;
    }

    // Must have 2 or more stops to be valid.
    return gradient->stopCount() >= 2;
}

bool CSSParser::isGeneratedImageValue(CSSParserValue* val) const
{
    if (val->unit != CSSParserValue::Function)
        return false;

    return equalIgnoringCase(val->function->name, "-webkit-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-linear-gradient(")
        || equalIgnoringCase(val->function->name, "linear-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-repeating-linear-gradient(")
        || equalIgnoringCase(val->function->name, "repeating-linear-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-radial-gradient(")
        || equalIgnoringCase(val->function->name, "radial-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-repeating-radial-gradient(")
        || equalIgnoringCase(val->function->name, "repeating-radial-gradient(")
        || equalIgnoringCase(val->function->name, "-webkit-canvas(")
        || equalIgnoringCase(val->function->name, "-webkit-cross-fade(");
}

bool CSSParser::parseGeneratedImage(CSSParserValueList* valueList, RefPtr<CSSValue>& value)
{
    CSSParserValue* val = valueList->current();

    if (val->unit != CSSParserValue::Function)
        return false;

    if (equalIgnoringCase(val->function->name, "-webkit-gradient("))
        return parseDeprecatedGradient(valueList, value);

    if (equalIgnoringCase(val->function->name, "-webkit-linear-gradient("))
        return parseDeprecatedLinearGradient(valueList, value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "linear-gradient("))
        return parseLinearGradient(valueList, value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "-webkit-repeating-linear-gradient("))
        return parseDeprecatedLinearGradient(valueList, value, Repeating);

    if (equalIgnoringCase(val->function->name, "repeating-linear-gradient("))
        return parseLinearGradient(valueList, value, Repeating);

    if (equalIgnoringCase(val->function->name, "-webkit-radial-gradient("))
        return parseDeprecatedRadialGradient(valueList, value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "radial-gradient("))
        return parseRadialGradient(valueList, value, NonRepeating);

    if (equalIgnoringCase(val->function->name, "-webkit-repeating-radial-gradient("))
        return parseDeprecatedRadialGradient(valueList, value, Repeating);

    if (equalIgnoringCase(val->function->name, "repeating-radial-gradient("))
        return parseRadialGradient(valueList, value, Repeating);

    if (equalIgnoringCase(val->function->name, "-webkit-canvas("))
        return parseCanvas(valueList, value);

    if (equalIgnoringCase(val->function->name, "-webkit-cross-fade("))
        return parseCrossfade(valueList, value);

    return false;
}

bool CSSParser::parseCrossfade(CSSParserValueList* valueList, RefPtr<CSSValue>& crossfade)
{
    RefPtr<CSSCrossfadeValue> result;

    // Walk the arguments.
    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || args->size() != 5)
        return false;
    CSSParserValue* a = args->current();
    RefPtr<CSSValue> fromImageValue;
    RefPtr<CSSValue> toImageValue;

    // The first argument is the "from" image. It is a fill image.
    if (!a || !parseFillImage(args, fromImageValue))
        return false;
    a = args->next();

    // Skip a comma
    if (!isComma(a))
        return false;
    a = args->next();

    // The second argument is the "to" image. It is a fill image.
    if (!a || !parseFillImage(args, toImageValue))
        return false;
    a = args->next();

    // Skip a comma
    if (!isComma(a))
        return false;
    a = args->next();

    // The third argument is the crossfade value. It is a percentage or a fractional number.
    RefPtr<CSSPrimitiveValue> percentage;
    if (!a)
        return false;
    
    if (a->unit == CSSPrimitiveValue::CSS_PERCENTAGE)
        percentage = cssValuePool().createValue(clampTo<double>(a->fValue / 100, 0, 1), CSSPrimitiveValue::CSS_NUMBER);
    else if (a->unit == CSSPrimitiveValue::CSS_NUMBER)
        percentage = cssValuePool().createValue(clampTo<double>(a->fValue, 0, 1), CSSPrimitiveValue::CSS_NUMBER);
    else
        return false;

    result = CSSCrossfadeValue::create(fromImageValue, toImageValue);
    result->setPercentage(percentage);

    crossfade = result;

    return true;
}

bool CSSParser::parseCanvas(CSSParserValueList* valueList, RefPtr<CSSValue>& canvas)
{
    // Walk the arguments.
    CSSParserValueList* args = valueList->current()->function->args.get();
    if (!args || args->size() != 1)
        return false;

    // The first argument is the canvas name.  It is an identifier.
    CSSParserValue* value = args->current();
    if (!value || value->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;

    canvas = CSSCanvasValue::create(value->string);
    return true;
}

#if ENABLE(CSS_IMAGE_RESOLUTION)
PassRefPtr<CSSValue> CSSParser::parseImageResolution()
{
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    bool haveResolution = false;
    bool haveFromImage = false;
    bool haveSnap = false;

    CSSParserValue* value = m_valueList->current();
    while (value) {
        if (!haveFromImage && value->id == CSSValueFromImage) {
            list->append(cssValuePool().createIdentifierValue(value->id));
            haveFromImage = true;
        } else if (!haveSnap && value->id == CSSValueSnap) {
            list->append(cssValuePool().createIdentifierValue(value->id));
            haveSnap = true;
        } else if (!haveResolution && validUnit(value, FResolution | FNonNeg) && value->fValue > 0) {
            list->append(createPrimitiveNumericValue(value));
            haveResolution = true;
        } else
            return 0;
        value = m_valueList->next();
    }
    if (!list->length())
        return 0;
    if (!haveFromImage && !haveResolution)
        return 0;
    return list.release();
}
#endif

#if ENABLE(CSS_IMAGE_SET)
PassRefPtr<CSSValue> CSSParser::parseImageSet()
{
    CSSParserValue* value = m_valueList->current();
    ASSERT(value->unit == CSSParserValue::Function);

    CSSParserValueList* functionArgs = value->function->args.get();
    if (!functionArgs || !functionArgs->size() || !functionArgs->current())
        return 0;

    RefPtr<CSSImageSetValue> imageSet = CSSImageSetValue::create();
    CSSParserValue* arg = functionArgs->current();
    while (arg) {
        if (arg->unit != CSSPrimitiveValue::CSS_URI)
            return 0;

        imageSet->append(CSSImageValue::create(completeURL(arg->string)));
        arg = functionArgs->next();
        if (!arg || arg->unit != CSSPrimitiveValue::CSS_DIMENSION)
            return 0;

        double imageScaleFactor = 0;
        const String& string = arg->string;
        unsigned length = string.length();
        if (!length)
            return 0;
        if (string.is8Bit()) {
            const LChar* start = string.characters8();
            parseDouble(start, start + length, 'x', imageScaleFactor);
        } else {
            const UChar* start = string.characters16();
            parseDouble(start, start + length, 'x', imageScaleFactor);
        }
        if (imageScaleFactor <= 0)
            return 0;
        imageSet->append(cssValuePool().createValue(imageScaleFactor, CSSPrimitiveValue::CSS_NUMBER));

        // If there are no more arguments, we're done.
        arg = functionArgs->next();
        if (!arg)
            break;

        // If there are more arguments, they should be after a comma.
        if (!isComma(arg))
            return 0;

        // Skip the comma and move on to the next argument.
        arg = functionArgs->next();
    }

    return imageSet.release();
}
#endif

class TransformOperationInfo {
public:
    TransformOperationInfo(const CSSParserString& name)
        : m_type(WebKitCSSTransformValue::UnknownTransformOperation)
        , m_argCount(1)
        , m_allowSingleArgument(false)
        , m_unit(CSSParser::FUnknown)
    {
        const UChar* characters;
        unsigned nameLength = name.length();

        const unsigned longestNameLength = 12;
        UChar characterBuffer[longestNameLength];
        if (name.is8Bit()) {
            unsigned length = std::min(longestNameLength, nameLength);
            const LChar* characters8 = name.characters8();
            for (unsigned i = 0; i < length; ++i)
                characterBuffer[i] = characters8[i];
            characters = characterBuffer;
        } else
            characters = name.characters16();

        switch (nameLength) {
        case 5:
            // Valid name: skew(.
            if (((characters[0] == 's') || (characters[0] == 'S'))
                & ((characters[1] == 'k') || (characters[1] == 'K'))
                & ((characters[2] == 'e') || (characters[2] == 'E'))
                & ((characters[3] == 'w') || (characters[3] == 'W'))
                & (characters[4] == '(')) {
                m_unit = CSSParser::FAngle;
                m_type = WebKitCSSTransformValue::SkewTransformOperation;
                m_allowSingleArgument = true;
                m_argCount = 3;
            }
            break;
        case 6:
            // Valid names: skewx(, skewy(, scale(.
            if ((characters[1] == 'c') || (characters[1] == 'C')) {
                if (((characters[0] == 's') || (characters[0] == 'S'))
                    & ((characters[2] == 'a') || (characters[2] == 'A'))
                    & ((characters[3] == 'l') || (characters[3] == 'L'))
                    & ((characters[4] == 'e') || (characters[4] == 'E'))
                    & (characters[5] == '(')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::ScaleTransformOperation;
                    m_allowSingleArgument = true;
                    m_argCount = 3;
                }
            } else if (((characters[0] == 's') || (characters[0] == 'S'))
                       & ((characters[1] == 'k') || (characters[1] == 'K'))
                       & ((characters[2] == 'e') || (characters[2] == 'E'))
                       & ((characters[3] == 'w') || (characters[3] == 'W'))
                       & (characters[5] == '(')) {
                if ((characters[4] == 'x') || (characters[4] == 'X')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::SkewXTransformOperation;
                } else if ((characters[4] == 'y') || (characters[4] == 'Y')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::SkewYTransformOperation;
                }
            }
            break;
        case 7:
            // Valid names: matrix(, rotate(, scalex(, scaley(, scalez(.
            if ((characters[0] == 'm') || (characters[0] == 'M')) {
                if (((characters[1] == 'a') || (characters[1] == 'A'))
                    & ((characters[2] == 't') || (characters[2] == 'T'))
                    & ((characters[3] == 'r') || (characters[3] == 'R'))
                    & ((characters[4] == 'i') || (characters[4] == 'I'))
                    & ((characters[5] == 'x') || (characters[5] == 'X'))
                    & (characters[6] == '(')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::MatrixTransformOperation;
                    m_argCount = 11;
                }
            } else if ((characters[0] == 'r') || (characters[0] == 'R')) {
                if (((characters[1] == 'o') || (characters[1] == 'O'))
                    & ((characters[2] == 't') || (characters[2] == 'T'))
                    & ((characters[3] == 'a') || (characters[3] == 'A'))
                    & ((characters[4] == 't') || (characters[4] == 'T'))
                    & ((characters[5] == 'e') || (characters[5] == 'E'))
                    & (characters[6] == '(')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::RotateTransformOperation;
                }
            } else if (((characters[0] == 's') || (characters[0] == 'S'))
                       & ((characters[1] == 'c') || (characters[1] == 'C'))
                       & ((characters[2] == 'a') || (characters[2] == 'A'))
                       & ((characters[3] == 'l') || (characters[3] == 'L'))
                       & ((characters[4] == 'e') || (characters[4] == 'E'))
                       & (characters[6] == '(')) {
                if ((characters[5] == 'x') || (characters[5] == 'X')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::ScaleXTransformOperation;
                } else if ((characters[5] == 'y') || (characters[5] == 'Y')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::ScaleYTransformOperation;
                } else if ((characters[5] == 'z') || (characters[5] == 'Z')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::ScaleZTransformOperation;
                }
            }
            break;
        case 8:
            // Valid names: rotatex(, rotatey(, rotatez(, scale3d(.
            if ((characters[0] == 's') || (characters[0] == 'S')) {
                if (((characters[1] == 'c') || (characters[1] == 'C'))
                    & ((characters[2] == 'a') || (characters[2] == 'A'))
                    & ((characters[3] == 'l') || (characters[3] == 'L'))
                    & ((characters[4] == 'e') || (characters[4] == 'E'))
                    & (characters[5] == '3')
                    & ((characters[6] == 'd') || (characters[6] == 'D'))
                    & (characters[7] == '(')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::Scale3DTransformOperation;
                    m_argCount = 5;
                }
            } else if (((characters[0] == 'r') || (characters[0] == 'R'))
                       & ((characters[1] == 'o') || (characters[1] == 'O'))
                       & ((characters[2] == 't') || (characters[2] == 'T'))
                       & ((characters[3] == 'a') || (characters[3] == 'A'))
                       & ((characters[4] == 't') || (characters[4] == 'T'))
                       & ((characters[5] == 'e') || (characters[5] == 'E'))
                       & (characters[7] == '(')) {
                if ((characters[6] == 'x') || (characters[6] == 'X')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::RotateXTransformOperation;
                } else if ((characters[6] == 'y') || (characters[6] == 'Y')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::RotateYTransformOperation;
                } else if ((characters[6] == 'z') || (characters[6] == 'Z')) {
                    m_unit = CSSParser::FAngle;
                    m_type = WebKitCSSTransformValue::RotateZTransformOperation;
                }
            }
            break;
        case 9:
            // Valid names: matrix3d(, rotate3d(.
            if ((characters[0] == 'm') || (characters[0] == 'M')) {
                if (((characters[1] == 'a') || (characters[1] == 'A'))
                    & ((characters[2] == 't') || (characters[2] == 'T'))
                    & ((characters[3] == 'r') || (characters[3] == 'R'))
                    & ((characters[4] == 'i') || (characters[4] == 'I'))
                    & ((characters[5] == 'x') || (characters[5] == 'X'))
                    & (characters[6] == '3')
                    & ((characters[7] == 'd') || (characters[7] == 'D'))
                    & (characters[8] == '(')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::Matrix3DTransformOperation;
                    m_argCount = 31;
                }
            } else if (((characters[0] == 'r') || (characters[0] == 'R'))
                       & ((characters[1] == 'o') || (characters[1] == 'O'))
                       & ((characters[2] == 't') || (characters[2] == 'T'))
                       & ((characters[3] == 'a') || (characters[3] == 'A'))
                       & ((characters[4] == 't') || (characters[4] == 'T'))
                       & ((characters[5] == 'e') || (characters[5] == 'E'))
                       & (characters[6] == '3')
                       & ((characters[7] == 'd') || (characters[7] == 'D'))
                       & (characters[8] == '(')) {
                m_unit = CSSParser::FNumber;
                m_type = WebKitCSSTransformValue::Rotate3DTransformOperation;
                m_argCount = 7;
            }
            break;
        case 10:
            // Valid name: translate(.
            if (((characters[0] == 't') || (characters[0] == 'T'))
                & ((characters[1] == 'r') || (characters[1] == 'R'))
                & ((characters[2] == 'a') || (characters[2] == 'A'))
                & ((characters[3] == 'n') || (characters[3] == 'N'))
                & ((characters[4] == 's') || (characters[4] == 'S'))
                & ((characters[5] == 'l') || (characters[5] == 'L'))
                & ((characters[6] == 'a') || (characters[6] == 'A'))
                & ((characters[7] == 't') || (characters[7] == 'T'))
                & ((characters[8] == 'e') || (characters[8] == 'E'))
                & (characters[9] == '(')) {
                m_unit = CSSParser::FLength | CSSParser::FPercent;
                m_type = WebKitCSSTransformValue::TranslateTransformOperation;
                m_allowSingleArgument = true;
                m_argCount = 3;
            }
            break;
        case 11:
            // Valid names: translatex(, translatey(, translatez(.
            if (((characters[0] == 't') || (characters[0] == 'T'))
                & ((characters[1] == 'r') || (characters[1] == 'R'))
                & ((characters[2] == 'a') || (characters[2] == 'A'))
                & ((characters[3] == 'n') || (characters[3] == 'N'))
                & ((characters[4] == 's') || (characters[4] == 'S'))
                & ((characters[5] == 'l') || (characters[5] == 'L'))
                & ((characters[6] == 'a') || (characters[6] == 'A'))
                & ((characters[7] == 't') || (characters[7] == 'T'))
                & ((characters[8] == 'e') || (characters[8] == 'E'))
                & (characters[10] == '(')) {
                if ((characters[9] == 'x') || (characters[9] == 'X')) {
                    m_unit = CSSParser::FLength | CSSParser::FPercent;
                    m_type = WebKitCSSTransformValue::TranslateXTransformOperation;
                } else if ((characters[9] == 'y') || (characters[9] == 'Y')) {
                    m_unit = CSSParser::FLength | CSSParser::FPercent;
                    m_type = WebKitCSSTransformValue::TranslateYTransformOperation;
                } else if ((characters[9] == 'z') || (characters[9] == 'Z')) {
                    m_unit = CSSParser::FLength | CSSParser::FPercent;
                    m_type = WebKitCSSTransformValue::TranslateZTransformOperation;
                }
            }
            break;
        case 12:
            // Valid names: perspective(, translate3d(.
            if ((characters[0] == 'p') || (characters[0] == 'P')) {
                if (((characters[1] == 'e') || (characters[1] == 'E'))
                    & ((characters[2] == 'r') || (characters[2] == 'R'))
                    & ((characters[3] == 's') || (characters[3] == 'S'))
                    & ((characters[4] == 'p') || (characters[4] == 'P'))
                    & ((characters[5] == 'e') || (characters[5] == 'E'))
                    & ((characters[6] == 'c') || (characters[6] == 'C'))
                    & ((characters[7] == 't') || (characters[7] == 'T'))
                    & ((characters[8] == 'i') || (characters[8] == 'I'))
                    & ((characters[9] == 'v') || (characters[9] == 'V'))
                    & ((characters[10] == 'e') || (characters[10] == 'E'))
                    & (characters[11] == '(')) {
                    m_unit = CSSParser::FNumber;
                    m_type = WebKitCSSTransformValue::PerspectiveTransformOperation;
                }
            } else if (((characters[0] == 't') || (characters[0] == 'T'))
                       & ((characters[1] == 'r') || (characters[1] == 'R'))
                       & ((characters[2] == 'a') || (characters[2] == 'A'))
                       & ((characters[3] == 'n') || (characters[3] == 'N'))
                       & ((characters[4] == 's') || (characters[4] == 'S'))
                       & ((characters[5] == 'l') || (characters[5] == 'L'))
                       & ((characters[6] == 'a') || (characters[6] == 'A'))
                       & ((characters[7] == 't') || (characters[7] == 'T'))
                       & ((characters[8] == 'e') || (characters[8] == 'E'))
                       & (characters[9] == '3')
                       & ((characters[10] == 'd') || (characters[10] == 'D'))
                       & (characters[11] == '(')) {
                m_unit = CSSParser::FLength | CSSParser::FPercent;
                m_type = WebKitCSSTransformValue::Translate3DTransformOperation;
                m_argCount = 5;
            }
            break;
        } // end switch ()
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

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        RefPtr<CSSValue> parsedTransformValue = parseTransformValue(value);
        if (!parsedTransformValue)
            return 0;

        list->append(parsedTransformValue.release());
    }

    return list.release();
}

PassRefPtr<CSSValue> CSSParser::parseTransformValue(CSSParserValue *value)
{
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

    // The transform is a list of functional primitives that specify transform operations.
    // We collect a list of WebKitCSSTransformValues, where each value specifies a single operation.

    // Create the new WebKitCSSTransformValue for this operation and add it to our list.
    RefPtr<WebKitCSSTransformValue> transformValue = WebKitCSSTransformValue::create(info.type());

    // Snag our values.
    CSSParserValue* a = args->current();
    unsigned argNumber = 0;
    while (a) {
        CSSParser::Units unit = info.unit();

        if (info.type() == WebKitCSSTransformValue::Rotate3DTransformOperation && argNumber == 3) {
            // 4th param of rotate3d() is an angle rather than a bare number, validate it as such
            if (!validUnit(a, FAngle, CSSStrictMode))
                return 0;
        } else if (info.type() == WebKitCSSTransformValue::Translate3DTransformOperation && argNumber == 2) {
            // 3rd param of translate3d() cannot be a percentage
            if (!validUnit(a, FLength, CSSStrictMode))
                return 0;
        } else if (info.type() == WebKitCSSTransformValue::TranslateZTransformOperation && !argNumber) {
            // 1st param of translateZ() cannot be a percentage
            if (!validUnit(a, FLength, CSSStrictMode))
                return 0;
        } else if (info.type() == WebKitCSSTransformValue::PerspectiveTransformOperation && !argNumber) {
            // 1st param of perspective() must be a non-negative number (deprecated) or length.
            if (!validUnit(a, FNumber | FLength | FNonNeg, CSSStrictMode))
                return 0;
        } else if (!validUnit(a, unit, CSSStrictMode))
            return 0;

        // Add the value to the current transform operation.
        transformValue->append(createPrimitiveNumericValue(a));

        a = args->next();
        if (!a)
            break;
        if (a->unit != CSSParserValue::Operator || a->iValue != ',')
            return 0;
        a = args->next();

        argNumber++;
    }

    return transformValue.release();
}

bool CSSParser::isBlendMode(CSSValueID valueID)
{
    return (valueID >= CSSValueMultiply && valueID <= CSSValueLuminosity)
        || valueID == CSSValueNormal
        || valueID == CSSValueOverlay;
}

bool CSSParser::isCompositeOperator(CSSValueID valueID)
{
    // FIXME: Add CSSValueDestination and CSSValueLighter when the Compositing spec updates.
    return valueID >= CSSValueClear && valueID <= CSSValueXor;
}

#if ENABLE(CSS_FILTERS)

static void filterInfoForName(const CSSParserString& name, WebKitCSSFilterValue::FilterOperationType& filterType, unsigned& maximumArgumentCount)
{
    if (equalIgnoringCase(name, "grayscale("))
        filterType = WebKitCSSFilterValue::GrayscaleFilterOperation;
    else if (equalIgnoringCase(name, "sepia("))
        filterType = WebKitCSSFilterValue::SepiaFilterOperation;
    else if (equalIgnoringCase(name, "saturate("))
        filterType = WebKitCSSFilterValue::SaturateFilterOperation;
    else if (equalIgnoringCase(name, "hue-rotate("))
        filterType = WebKitCSSFilterValue::HueRotateFilterOperation;
    else if (equalIgnoringCase(name, "invert("))
        filterType = WebKitCSSFilterValue::InvertFilterOperation;
    else if (equalIgnoringCase(name, "opacity("))
        filterType = WebKitCSSFilterValue::OpacityFilterOperation;
    else if (equalIgnoringCase(name, "brightness("))
        filterType = WebKitCSSFilterValue::BrightnessFilterOperation;
    else if (equalIgnoringCase(name, "contrast("))
        filterType = WebKitCSSFilterValue::ContrastFilterOperation;
    else if (equalIgnoringCase(name, "blur("))
        filterType = WebKitCSSFilterValue::BlurFilterOperation;
    else if (equalIgnoringCase(name, "drop-shadow(")) {
        filterType = WebKitCSSFilterValue::DropShadowFilterOperation;
        maximumArgumentCount = 4;  // x-offset, y-offset, blur-radius, color -- spread and inset style not allowed.
    }
#if ENABLE(CSS_SHADERS)
    else if (equalIgnoringCase(name, "custom("))
        filterType = WebKitCSSFilterValue::CustomFilterOperation;
#endif
}

#if ENABLE(CSS_SHADERS)
static bool acceptCommaOperator(CSSParserValueList* argsList)
{
    if (CSSParserValue* arg = argsList->current()) {
        if (!isComma(arg))
            return false;
        argsList->next();
    }
    return true;
}

PassRefPtr<WebKitCSSArrayFunctionValue> CSSParser::parseCustomFilterArrayFunction(CSSParserValue* value)
{
    ASSERT(value->unit == CSSParserValue::Function && value->function);

    if (!equalIgnoringCase(value->function->name, "array("))
        return 0;

    CSSParserValueList* arrayArgsParserValueList = value->function->args.get();
    if (!arrayArgsParserValueList || !arrayArgsParserValueList->size())
        return 0;

    // array() values are comma separated.
    RefPtr<WebKitCSSArrayFunctionValue> arrayFunction = WebKitCSSArrayFunctionValue::create();
    while (true) {
        // We parse pairs <Value, Comma> at each step.
        CSSParserValue* currentParserValue = arrayArgsParserValueList->current();
        if (!currentParserValue || !validUnit(currentParserValue, FNumber, CSSStrictMode))
            return 0;

        RefPtr<CSSValue> arrayValue = cssValuePool().createValue(currentParserValue->fValue, CSSPrimitiveValue::CSS_NUMBER);
        arrayFunction->append(arrayValue.release());

        CSSParserValue* nextParserValue = arrayArgsParserValueList->next();
        if (!nextParserValue)
            break;

        if (!isComma(nextParserValue))
            return 0;

        arrayArgsParserValueList->next();
    }

    return arrayFunction;
}

PassRefPtr<WebKitCSSMatFunctionValue> CSSParser::parseMatValue(CSSParserValue* value)
{
    if (value->unit != CSSParserValue::Function || !value->function)
        return 0;

    unsigned numberOfValues = 0;
    if (equalIgnoringCase(value->function->name, "mat2("))
        numberOfValues = 4;
    else if (equalIgnoringCase(value->function->name, "mat3("))
        numberOfValues = 9;
    else if (equalIgnoringCase(value->function->name, "mat4("))
        numberOfValues = 16;
    else
        return 0;

    CSSParserValueList* args = value->function->args.get();
    if (!args || args->size() != (numberOfValues * 2 - 1))
        return 0;

    RefPtr<WebKitCSSMatFunctionValue> matValueList = WebKitCSSMatFunctionValue::create();
    CSSParserValue* arg = args->current();
    while (arg) {
        if (!validUnit(arg, FNumber, CSSStrictMode))
            return 0;
        matValueList->append(cssValuePool().createValue(arg->fValue, CSSPrimitiveValue::CSS_NUMBER));
        arg = args->next();

        if (!arg)
            break;

        if (!isComma(arg))
            return 0;

        arg = args->next();
    }

    if (!matValueList || matValueList->length() != numberOfValues)
        return 0;

    return matValueList.release();
}

PassRefPtr<WebKitCSSMixFunctionValue> CSSParser::parseMixFunction(CSSParserValue* value)
{
    ASSERT(value->unit == CSSParserValue::Function && value->function);

    if (!equalIgnoringCase(value->function->name, "mix("))
        return 0;

    CSSParserValueList* argsList = value->function->args.get();
    if (!argsList)
        return 0;

    unsigned numArgs = argsList->size();
    if (numArgs < 1 || numArgs > 3)
        return 0;
    
    RefPtr<WebKitCSSMixFunctionValue> mixFunction = WebKitCSSMixFunctionValue::create();

    bool hasBlendMode = false;
    bool hasAlphaCompositing = false;
    CSSParserValue* arg;
    while ((arg = argsList->current())) {
        RefPtr<CSSValue> value;

        unsigned argNumber = argsList->currentIndex();
        if (!argNumber) {
            if (arg->unit == CSSPrimitiveValue::CSS_URI) {
                KURL shaderURL = completeURL(arg->string);
                value = WebKitCSSShaderValue::create(shaderURL.string());
            }
        } else if (argNumber == 1 || argNumber == 2) {
            if (!hasBlendMode && isBlendMode(arg->id)) {
                hasBlendMode = true;
                value = cssValuePool().createIdentifierValue(arg->id);
            } else if (!hasAlphaCompositing && isCompositeOperator(arg->id)) {
                hasAlphaCompositing = true;
                value = cssValuePool().createIdentifierValue(arg->id);
            }
        }

        if (!value)
            return 0;

        mixFunction->append(value.release());

        arg = argsList->next();
    }

    return mixFunction;
}

PassRefPtr<CSSValueList> CSSParser::parseCustomFilterParameters(CSSParserValueList* argsList)
{
    //
    // params:      [<param-def>[,<param-def>*]]
    // param-def:   <param-name>wsp<param-value>
    // param-name:  <ident>
    // param-value: true|false[wsp+true|false]{0-3} |
    //              <number>[wsp+<number>]{0-3} |
    //              <array> |
    //              <transform> |
    //              <texture(<uri>)>
    // array: 'array('<number>[wsp<number>]*')'
    // css-3d-transform: <transform-function>;[<transform-function>]*
    // transform:   <css-3d-transform> | <mat>
    // mat:         'mat2('<number>(,<number>){3}')' | 
    //              'mat3('<number>(,<number>){8}')' | 
    //              'mat4('<number>(,<number>){15}')' )
    //

    RefPtr<CSSValueList> paramList = CSSValueList::createCommaSeparated();

    while (CSSParserValue* arg = argsList->current()) {
        if (arg->unit != CSSPrimitiveValue::CSS_IDENT)
            return 0;

        RefPtr<CSSValueList> parameter = CSSValueList::createSpaceSeparated();
        parameter->append(createPrimitiveStringValue(arg));

        arg = argsList->next();
        if (!arg)
            return 0;

        RefPtr<CSSValue> parameterValue;

        if (arg->unit == CSSParserValue::Function && arg->function) {
            // FIXME: Implement parsing for the other parameter types.
            // textures: https://bugs.webkit.org/show_bug.cgi?id=71442
            if (equalIgnoringCase(arg->function->name, "array(")) {
                parameterValue = parseCustomFilterArrayFunction(arg);
                // This parsing step only consumes function arguments,
                // argsList is therefore moved forward explicitely.
                argsList->next();
            } else {
                // Parse mat2, mat3 and mat4 functions.
                parameterValue = parseMatValue(arg);
                if (!parameterValue && arg)
                    parameterValue = parseCustomFilterTransform(argsList);
                else if (parameterValue)
                    argsList->next();
            }
        } else if (validUnit(arg, FNumber, CSSStrictMode)) {
            RefPtr<CSSValueList> paramValueList = CSSValueList::createSpaceSeparated();
            while (arg) {
                // If we hit a comma, it means that we finished this parameter's values.
                if (isComma(arg))
                    break;
                if (!validUnit(arg, FNumber, CSSStrictMode))
                    return 0;
                paramValueList->append(cssValuePool().createValue(arg->fValue, CSSPrimitiveValue::CSS_NUMBER));
                arg = argsList->next();
            }
            if (!paramValueList->length() || paramValueList->length() > 4)
                return 0;
            parameterValue = paramValueList.release();
        }
        if (!parameterValue && arg) {
            // All parameter values need to be CSSValueLists.
            RefPtr<CSSValueList> paramValueList = CSSValueList::createSpaceSeparated();
            RefPtr<CSSPrimitiveValue> colorValue = parseColor(arg);
            if (!colorValue)
                return 0;
            paramValueList->append(colorValue.release());
            parameterValue = paramValueList.release();
            arg = argsList->next();
        }

        if (!parameterValue || !acceptCommaOperator(argsList))
            return 0;

        parameter->append(parameterValue.release());
        paramList->append(parameter.release());
    }

    return paramList;   
}

PassRefPtr<WebKitCSSFilterValue> CSSParser::parseCustomFilterFunctionWithAtRuleReferenceSyntax(CSSParserValue* value)
{
    //
    // Custom filter function "at-rule reference" syntax:
    //
    // custom(<filter-name>wsp[,wsp<params>])
    //
    // filter-name: <filter-name>
    // params: See the comment in CSSParser::parseCustomFilterParameters.
    //

    ASSERT(value->function);

    CSSParserValueList* argsList = value->function->args.get();
    if (!argsList || !argsList->size())
        return 0;

    // 1. Parse the filter name.
    CSSParserValue* arg = argsList->current();
    if (arg->unit != CSSPrimitiveValue::CSS_IDENT)
        return 0;

    RefPtr<WebKitCSSFilterValue> filterValue = WebKitCSSFilterValue::create(WebKitCSSFilterValue::CustomFilterOperation);

    RefPtr<CSSValue> filterName = createPrimitiveStringValue(arg);
    filterValue->append(filterName);
    argsList->next();

    if (!acceptCommaOperator(argsList))
        return 0;

    // 2. Parse the parameters.
    RefPtr<CSSValueList> paramList = parseCustomFilterParameters(argsList);
    if (!paramList)
        return 0;

    if (paramList->length())
        filterValue->append(paramList.release());

    return filterValue;
}

// FIXME: The custom filters "inline" syntax is deprecated. We will remove it eventually.
PassRefPtr<WebKitCSSFilterValue> CSSParser::parseCustomFilterFunctionWithInlineSyntax(CSSParserValue* value)
{
    //
    // Custom filter function "inline" syntax:
    //
    // custom(<vertex-shader>[wsp<fragment-shader>][,<vertex-mesh>][,<params>])
    //
    // vertexShader:    <uri> | none
    // fragmentShader:  <uri> | none | mix(<uri> [ <blend-mode> || <alpha-compositing> ]?)
    //
    // blend-mode: normal | multiply | screen | overlay | darken | lighten | color-dodge |
    //             color-burn | hard-light | soft-light | difference | exclusion | hue |
    //             saturation | color | luminosity
    // alpha-compositing: clear | src | dst | src-over | dst-over | src-in | dst-in |
    //                    src-out | dst-out | src-atop | dst-atop | xor | plus
    //
    // vertexMesh:  +<integer>{1,2}[wsp<box>][wsp'detached']
    // box: filter-box | border-box | padding-box | content-box
    //
    // params: See the comment in CSSParser::parseCustomFilterParameters.
    //

    ASSERT(value->function);

    CSSParserValueList* argsList = value->function->args.get();
    if (!argsList)
        return 0;

    RefPtr<WebKitCSSFilterValue> filterValue = WebKitCSSFilterValue::create(WebKitCSSFilterValue::CustomFilterOperation);

    // 1. Parse the shader URLs: <vertex-shader>[wsp<fragment-shader>]
    RefPtr<CSSValueList> shadersList = CSSValueList::createSpaceSeparated();
    bool hadAtLeastOneCustomShader = false;
    CSSParserValue* arg;
    while ((arg = argsList->current())) {
        RefPtr<CSSValue> value;
        if (arg->id == CSSValueNone)
            value = cssValuePool().createIdentifierValue(CSSValueNone);
        else if (arg->unit == CSSPrimitiveValue::CSS_URI) {
            KURL shaderURL = completeURL(arg->string);
            value = WebKitCSSShaderValue::create(shaderURL.string());
            hadAtLeastOneCustomShader = true;
        } else if (argsList->currentIndex() == 1 && arg->unit == CSSParserValue::Function) {
            if (!(value = parseMixFunction(arg)))
                return 0;
            hadAtLeastOneCustomShader = true;
        }

        if (!value)
            break;
        shadersList->append(value.release());
        argsList->next();
    }

    if (!shadersList->length() || !hadAtLeastOneCustomShader || shadersList->length() > 2 || !acceptCommaOperator(argsList))
        return 0;
        
    filterValue->append(shadersList.release());
    
    // 2. Parse the mesh size <vertex-mesh>
    RefPtr<CSSValueList> meshSizeList = CSSValueList::createSpaceSeparated();
    
    while ((arg = argsList->current())) {
        if (!validUnit(arg, FInteger | FNonNeg, CSSStrictMode))
            break;
        int integerValue = clampToInteger(arg->fValue);
        // According to the specification we can only accept positive non-zero values.
        if (integerValue < 1)
            return 0;
        meshSizeList->append(cssValuePool().createValue(integerValue, CSSPrimitiveValue::CSS_NUMBER));
        argsList->next();
    }
    
    if (meshSizeList->length() > 2)
        return 0;
    
    // FIXME: For legacy content, we accept the mesh box types. We don't do anything else with them.
    // Eventually, we'll remove them completely.
    // https://bugs.webkit.org/show_bug.cgi?id=103778
    if ((arg = argsList->current()) && (arg->id == CSSValueBorderBox || arg->id == CSSValuePaddingBox
        || arg->id == CSSValueContentBox || arg->id == CSSValueFilterBox))
        argsList->next();
    
    if ((arg = argsList->current()) && arg->id == CSSValueDetached) {
        meshSizeList->append(cssValuePool().createIdentifierValue(arg->id));
        argsList->next();
    }
    
    if (meshSizeList->length()) {
        if (!acceptCommaOperator(argsList))
            return 0;
        filterValue->append(meshSizeList.release());
    }
    
    // 3. Parse the parameters.
    RefPtr<CSSValueList> paramList = parseCustomFilterParameters(argsList);
    if (!paramList)
        return 0;

    if (paramList->length())
        filterValue->append(paramList.release());
    
    return filterValue;
}

PassRefPtr<WebKitCSSFilterValue> CSSParser::parseCustomFilterFunction(CSSParserValue* value)
{
    ASSERT(value->function);

    // Look ahead to determine which syntax the custom function is using.
    // Both the at-rule reference syntax and the inline syntax require at least one argument.
    CSSParserValueList* argsList = value->function->args.get();
    if (!argsList || !argsList->size())
        return 0;

    // The at-rule reference syntax expects a single ident or an ident followed by a comma. 
    // e.g. custom(my-filter) or custom(my-filter, ...)
    // In contrast, when the inline syntax starts with an ident like "none", it expects a uri or a mix function next.
    // e.g. custom(none url(...)) or custom(none mix(...)
    bool isAtRuleReferenceSyntax = argsList->valueAt(0)->unit == CSSPrimitiveValue::CSS_IDENT 
        && (argsList->size() == 1 || isComma(argsList->valueAt(1)));
    return isAtRuleReferenceSyntax ? parseCustomFilterFunctionWithAtRuleReferenceSyntax(value) : parseCustomFilterFunctionWithInlineSyntax(value);
}

PassRefPtr<CSSValueList> CSSParser::parseCustomFilterTransform(CSSParserValueList* valueList)
{
    if (!valueList)
        return 0;

    // CSS Shaders' custom() transforms are space separated and comma terminated.
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (CSSParserValue* value = valueList->current(); value; value = valueList->next()) {
        if (isComma(value))
            break;

        RefPtr<CSSValue> parsedTransformValue = parseTransformValue(value);
        if (!parsedTransformValue)
            return 0;

        list->append(parsedTransformValue.release());
    }

    return list.release();
}

PassRefPtr<WebKitCSSShaderValue> CSSParser::parseFilterRuleSrcUriAndFormat(CSSParserValueList* valueList)
{
    CSSParserValue* value = valueList->current();
    ASSERT(value && value->unit == CSSPrimitiveValue::CSS_URI);
    RefPtr<WebKitCSSShaderValue> shaderValue = WebKitCSSShaderValue::create(completeURL(value->string));

    value = valueList->next();
    if (value && value->unit == CSSParserValue::Function && equalIgnoringCase(value->function->name, "format(")) {
        CSSParserValueList* args = value->function->args.get();
        if (!args || args->size() != 1)
            return 0;

        CSSParserValue* arg = args->current();
        if (arg->unit != CSSPrimitiveValue::CSS_STRING)
            return 0;

        shaderValue->setFormat(arg->string);
        valueList->next();
    }

    return shaderValue.release();
}

bool CSSParser::parseFilterRuleSrc()
{
    RefPtr<CSSValueList> srcList = CSSValueList::createCommaSeparated();

    CSSParserValue* value = m_valueList->current();
    while (value) {
        if (value->unit != CSSPrimitiveValue::CSS_URI)
            return false;

        RefPtr<WebKitCSSShaderValue> shaderValue = parseFilterRuleSrcUriAndFormat(m_valueList.get());
        if (!shaderValue)
            return false;
        srcList->append(shaderValue.release());

        if (!acceptCommaOperator(m_valueList.get()))
            return false;

        value = m_valueList->current();
    }

    if (!srcList->length())
        return false;

    addProperty(CSSPropertySrc, srcList.release(), m_important);
    return true;
}

bool CSSParser::parseFilterRuleMix()
{
    if (m_valueList->size() > 2)
        return false;

    RefPtr<CSSValueList> mixList = CSSValueList::createSpaceSeparated();

    bool hasBlendMode = false;
    bool hasAlphaCompositing = false;
    CSSParserValue* value = m_valueList->current();
    while (value) {
        RefPtr<CSSValue> mixValue;
        if (!hasBlendMode && isBlendMode(value->id)) {
            hasBlendMode = true;
            mixValue = cssValuePool().createIdentifierValue(value->id);
        } else if (!hasAlphaCompositing && isCompositeOperator(value->id)) {
            hasAlphaCompositing = true;
            mixValue = cssValuePool().createIdentifierValue(value->id);
        }

        if (!mixValue)
            return false;

        mixList->append(mixValue.release());
        value = m_valueList->next();
    }

    addProperty(CSSPropertyMix, mixList.release(), m_important);
    return true;
}

bool CSSParser::parseGeometry(CSSPropertyID propId, CSSParserValue* value, bool important)
{
    ASSERT(propId == CSSPropertyGeometry);

    // <geometry-shape> = grid(<integer>{1,2} || [ detached | attached ]?)
    if (value->unit != CSSParserValue::Function || !equalIgnoringCase(value->function->name, "grid("))
        return false;

    ASSERT(value->function->args);

    // grid() function should have from 1 to 3 arguments.
    unsigned size = value->function->args->size();
    if (!size || size > 3)
        return false;

    CSSParserValueList* gridParserValueList = value->function->args.get();
    CSSParserValue* gridParserValue = gridParserValueList->current();
    RefPtr<CSSValueList> geometryList = CSSValueList::createSpaceSeparated();

    bool hasDimensions = false;
    bool hasConnectivity = false;

    while (gridParserValue) {
        if (hasDimensions && hasConnectivity) {
            geometryList.release();
            return false;
        }

        if (gridParserValue->id == CSSValueAttached || gridParserValue->id == CSSValueDetached) {
            hasConnectivity = true;
            geometryList->append(cssValuePool().createIdentifierValue(gridParserValue->id));
            gridParserValue = gridParserValueList->next();
        } else if (!hasDimensions && parseGridDimensions(gridParserValueList, geometryList)) {
            hasDimensions = true;
            gridParserValue = gridParserValueList->current();
        } else {
            geometryList.release();
            return false;
        }
    }

    addProperty(propId, geometryList.release(), important);
    return hasDimensions;
}

bool CSSParser::parseGridDimensions(CSSParserValueList* args, RefPtr<CSSValueList>& gridValueList)
{
    ASSERT(args);

    // There must be at least one valid numeric value.
    CSSParserValue* arg = args->current();
    if (!arg || !validUnit(arg, FPositiveInteger))
        return false;

    // A valid numeric value is parsed and then we move on.
    gridValueList->append(createPrimitiveNumericValue(arg));
    arg = args->next();

    // If the next argument is not numeric, we are done parsing the grid dimensions.
    if (!arg || !validUnit(arg, FPositiveInteger))
        return true;

    // Commit the second numeric value we found.
    gridValueList->append(createPrimitiveNumericValue(arg));
    args->next();
    return true;
}

bool CSSParser::parseFilterRuleParameters()
{
    RefPtr<CSSValueList> paramsList = parseCustomFilterParameters(m_valueList.get());
    if (!paramsList)
        return false;

    addProperty(CSSPropertyParameters, paramsList.release(), m_important);
    return true;
}

StyleRuleBase* CSSParser::createFilterRule(const CSSParserString& filterName)
{
    RefPtr<StyleRuleFilter> rule = StyleRuleFilter::create(filterName);
    rule->setProperties(createStylePropertySet());
    clearProperties();
    StyleRuleFilter* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return result;
}

#endif // ENABLE(CSS_SHADERS)

PassRefPtr<WebKitCSSFilterValue> CSSParser::parseBuiltinFilterArguments(CSSParserValueList* args, WebKitCSSFilterValue::FilterOperationType filterType)
{
    RefPtr<WebKitCSSFilterValue> filterValue = WebKitCSSFilterValue::create(filterType);
    ASSERT(args);

    switch (filterType) {    
    case WebKitCSSFilterValue::GrayscaleFilterOperation:
    case WebKitCSSFilterValue::SepiaFilterOperation:
    case WebKitCSSFilterValue::SaturateFilterOperation:
    case WebKitCSSFilterValue::InvertFilterOperation:
    case WebKitCSSFilterValue::OpacityFilterOperation:
    case WebKitCSSFilterValue::ContrastFilterOperation: {
        // One optional argument, 0-1 or 0%-100%, if missing use 100%.
        if (args->size() > 1)
            return 0;

        if (args->size()) {
            CSSParserValue* value = args->current();
            if (!validUnit(value, FNumber | FPercent | FNonNeg, CSSStrictMode))
                return 0;
                
            double amount = value->fValue;
            
            // Saturate and Contrast allow values over 100%.
            if (filterType != WebKitCSSFilterValue::SaturateFilterOperation
                && filterType != WebKitCSSFilterValue::ContrastFilterOperation) {
                double maxAllowed = value->unit == CSSPrimitiveValue::CSS_PERCENTAGE ? 100.0 : 1.0;
                if (amount > maxAllowed)
                    return 0;
            }

            filterValue->append(cssValuePool().createValue(amount, static_cast<CSSPrimitiveValue::UnitTypes>(value->unit)));
        }
        break;
    }
    case WebKitCSSFilterValue::BrightnessFilterOperation: {
        // One optional argument, if missing use 100%.
        if (args->size() > 1)
            return 0;

        if (args->size()) {
            CSSParserValue* value = args->current();
            if (!validUnit(value, FNumber | FPercent, CSSStrictMode))
                return 0;

            filterValue->append(cssValuePool().createValue(value->fValue, static_cast<CSSPrimitiveValue::UnitTypes>(value->unit)));
        }
        break;
    }
    case WebKitCSSFilterValue::HueRotateFilterOperation: {
        // hue-rotate() takes one optional angle.
        if (args->size() > 1)
            return 0;
        
        if (args->size()) {
            CSSParserValue* argument = args->current();
            if (!validUnit(argument, FAngle, CSSStrictMode))
                return 0;
        
            filterValue->append(createPrimitiveNumericValue(argument));
        }
        break;
    }
    case WebKitCSSFilterValue::BlurFilterOperation: {
        // Blur takes a single length. Zero parameters are allowed.
        if (args->size() > 1)
            return 0;
        
        if (args->size()) {
            CSSParserValue* argument = args->current();
            if (!validUnit(argument, FLength | FNonNeg, CSSStrictMode))
                return 0;

            filterValue->append(createPrimitiveNumericValue(argument));
        }
        break;
    }
    case WebKitCSSFilterValue::DropShadowFilterOperation: {
        // drop-shadow() takes a single shadow.
        RefPtr<CSSValueList> shadowValueList = parseShadow(args, CSSPropertyWebkitFilter);
        if (!shadowValueList || shadowValueList->length() != 1)
            return 0;
        
        filterValue->append((shadowValueList.release())->itemWithoutBoundsCheck(0));
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
    return filterValue.release();
}

PassRefPtr<CSSValueList> CSSParser::parseFilter()
{
    if (!m_valueList)
        return 0;

    // The filter is a list of functional primitives that specify individual operations.
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->unit != CSSPrimitiveValue::CSS_URI && (value->unit != CSSParserValue::Function || !value->function))
            return 0;

        WebKitCSSFilterValue::FilterOperationType filterType = WebKitCSSFilterValue::UnknownFilterOperation;

        // See if the specified primitive is one we understand.
        if (value->unit == CSSPrimitiveValue::CSS_URI) {
#if ENABLE(SVG)
            RefPtr<WebKitCSSFilterValue> referenceFilterValue = WebKitCSSFilterValue::create(WebKitCSSFilterValue::ReferenceFilterOperation);
            list->append(referenceFilterValue);
            referenceFilterValue->append(WebKitCSSSVGDocumentValue::create(value->string));
#endif
        } else {
            const CSSParserString name = value->function->name;
            unsigned maximumArgumentCount = 1;

            filterInfoForName(name, filterType, maximumArgumentCount);

            if (filterType == WebKitCSSFilterValue::UnknownFilterOperation)
                return 0;

#if ENABLE(CSS_SHADERS)
            if (filterType == WebKitCSSFilterValue::CustomFilterOperation) {
                // Make sure parsing fails if custom filters are disabled.
                if (!m_context.isCSSCustomFilterEnabled)
                    return 0;
                
                RefPtr<WebKitCSSFilterValue> filterValue = parseCustomFilterFunction(value);
                if (!filterValue)
                    return 0;
                list->append(filterValue.release());
                continue;
            }
#endif
            CSSParserValueList* args = value->function->args.get();
            if (!args)
                return 0;

            RefPtr<WebKitCSSFilterValue> filterValue = parseBuiltinFilterArguments(args, filterType);
            if (!filterValue)
                return 0;
            
            list->append(filterValue);
        }
    }

    return list.release();
}
#endif

#if ENABLE(CSS_REGIONS)
static bool validFlowName(const String& flowName)
{
    return !(equalIgnoringCase(flowName, "auto")
            || equalIgnoringCase(flowName, "default")
            || equalIgnoringCase(flowName, "inherit")
            || equalIgnoringCase(flowName, "initial")
            || equalIgnoringCase(flowName, "none"));
}
#endif

bool CSSParser::cssRegionsEnabled() const
{
    return m_context.isCSSRegionsEnabled;
}

bool CSSParser::cssCompositingEnabled() const
{
    return m_context.isCSSCompositingEnabled;
}

bool CSSParser::cssGridLayoutEnabled() const
{
    return m_context.isCSSGridLayoutEnabled;
}

#if ENABLE(CSS_REGIONS)
bool CSSParser::parseFlowThread(const String& flowName)
{
    setupParser("@-webkit-decls{-webkit-flow-into:", flowName, "}");
    cssyyparse(this);

    m_rule = 0;

    return ((m_parsedProperties.size() == 1) && (m_parsedProperties.first().id() == CSSPropertyWebkitFlowInto));
}

// none | <ident>
bool CSSParser::parseFlowThread(CSSPropertyID propId, bool important)
{
    ASSERT(propId == CSSPropertyWebkitFlowInto);
    ASSERT(cssRegionsEnabled());

    if (m_valueList->size() != 1)
        return false;

    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    if (value->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;

    if (value->id == CSSValueNone) {
        addProperty(propId, cssValuePool().createIdentifierValue(value->id), important);
        return true;
    }

    String inputProperty = String(value->string);
    if (!inputProperty.isEmpty()) {
        if (!validFlowName(inputProperty))
            return false;
        addProperty(propId, cssValuePool().createValue(inputProperty, CSSPrimitiveValue::CSS_STRING), important);
    } else
        addProperty(propId, cssValuePool().createIdentifierValue(CSSValueNone), important);

    return true;
}

// -webkit-flow-from: none | <ident>
bool CSSParser::parseRegionThread(CSSPropertyID propId, bool important)
{
    ASSERT(propId == CSSPropertyWebkitFlowFrom);
    ASSERT(cssRegionsEnabled());

    if (m_valueList->size() != 1)
        return false;

    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    if (value->unit != CSSPrimitiveValue::CSS_IDENT)
        return false;

    if (value->id == CSSValueNone)
        addProperty(propId, cssValuePool().createIdentifierValue(value->id), important);
    else {
        String inputProperty = String(value->string);
        if (!inputProperty.isEmpty()) {
            if (!validFlowName(inputProperty))
                return false;
            addProperty(propId, cssValuePool().createValue(inputProperty, CSSPrimitiveValue::CSS_STRING), important);
        } else
            addProperty(propId, cssValuePool().createIdentifierValue(CSSValueNone), important);
    }

    return true;
}
#endif

bool CSSParser::parseTransformOrigin(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2, CSSPropertyID& propId3, RefPtr<CSSValue>& value, RefPtr<CSSValue>& value2, RefPtr<CSSValue>& value3)
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
            value = parseFillPositionX(m_valueList.get());
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitTransformOriginY: {
            value = parseFillPositionY(m_valueList.get());
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitTransformOriginZ: {
            if (validUnit(m_valueList->current(), FLength))
                value = createPrimitiveNumericValue(m_valueList->current());
            if (value)
                m_valueList->next();
            break;
        }
        default:
            ASSERT_NOT_REACHED();
            return false;
    }

    return value;
}

bool CSSParser::parsePerspectiveOrigin(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2, RefPtr<CSSValue>& value, RefPtr<CSSValue>& value2)
{
    propId1 = propId;
    propId2 = propId;
    if (propId == CSSPropertyWebkitPerspectiveOrigin) {
        propId1 = CSSPropertyWebkitPerspectiveOriginX;
        propId2 = CSSPropertyWebkitPerspectiveOriginY;
    }

    switch (propId) {
        case CSSPropertyWebkitPerspectiveOrigin:
            if (m_valueList->size() > 2)
                return false;
            parse2ValuesFillPosition(m_valueList.get(), value, value2);
            break;
        case CSSPropertyWebkitPerspectiveOriginX: {
            value = parseFillPositionX(m_valueList.get());
            if (value)
                m_valueList->next();
            break;
        }
        case CSSPropertyWebkitPerspectiveOriginY: {
            value = parseFillPositionY(m_valueList.get());
            if (value)
                m_valueList->next();
            break;
        }
        default:
            ASSERT_NOT_REACHED();
            return false;
    }

    return value;
}

void CSSParser::addTextDecorationProperty(CSSPropertyID propId, PassRefPtr<CSSValue> value, bool important)
{
#if ENABLE(CSS3_TEXT)
    // The text-decoration-line property takes priority over text-decoration, unless the latter has important priority set.
    if (propId == CSSPropertyTextDecoration && !important && m_currentShorthand == CSSPropertyInvalid) {
        for (unsigned i = 0; i < m_parsedProperties.size(); ++i) {
            if (m_parsedProperties[i].id() == CSSPropertyWebkitTextDecorationLine)
                return;
        }
    }
#endif // CSS3_TEXT
    addProperty(propId, value, important);
}

bool CSSParser::parseTextDecoration(CSSPropertyID propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    if (value->id == CSSValueNone) {
        addTextDecorationProperty(propId, cssValuePool().createIdentifierValue(CSSValueNone), important);
        m_valueList->next();
        return true;
    }

    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    bool isValid = true;
    while (isValid && value) {
        switch (value->id) {
        case CSSValueBlink:
        case CSSValueLineThrough:
        case CSSValueOverline:
        case CSSValueUnderline:
            list->append(cssValuePool().createIdentifierValue(value->id));
            break;
        default:
            isValid = false;
            break;
        }
        if (isValid)
            value = m_valueList->next();
    }

    if (list->length() && isValid) {
        addTextDecorationProperty(propId, list.release(), important);
        return true;
    }

    return false;
}

#if ENABLE(CSS3_TEXT)
bool CSSParser::parseTextUnderlinePosition(bool important)
{
    // The text-underline-position property has sintax "auto | alphabetic | [ under || [ left | right ] ]".
    // However, values 'left' and 'right' are not implemented yet, so we will parse sintax
    // "auto | alphabetic | under" for now.
    CSSParserValue* value = m_valueList->current();
    switch (value->id) {
    case CSSValueAuto:
    case CSSValueAlphabetic:
    case CSSValueUnder:
        if (m_valueList->next())
            return false;

        addProperty(CSSPropertyWebkitTextUnderlinePosition, cssValuePool().createIdentifierValue(value->id), important);
        return true;
    default:
        break;
    }
    return false;
}
#endif // CSS3_TEXT

bool CSSParser::parseTextEmphasisStyle(bool important)
{
    unsigned valueListSize = m_valueList->size();

    RefPtr<CSSPrimitiveValue> fill;
    RefPtr<CSSPrimitiveValue> shape;

    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->unit == CSSPrimitiveValue::CSS_STRING) {
            if (fill || shape || (valueListSize != 1 && !inShorthand()))
                return false;
            addProperty(CSSPropertyWebkitTextEmphasisStyle, createPrimitiveStringValue(value), important);
            m_valueList->next();
            return true;
        }

        if (value->id == CSSValueNone) {
            if (fill || shape || (valueListSize != 1 && !inShorthand()))
                return false;
            addProperty(CSSPropertyWebkitTextEmphasisStyle, cssValuePool().createIdentifierValue(CSSValueNone), important);
            m_valueList->next();
            return true;
        }

        if (value->id == CSSValueOpen || value->id == CSSValueFilled) {
            if (fill)
                return false;
            fill = cssValuePool().createIdentifierValue(value->id);
        } else if (value->id == CSSValueDot || value->id == CSSValueCircle || value->id == CSSValueDoubleCircle || value->id == CSSValueTriangle || value->id == CSSValueSesame) {
            if (shape)
                return false;
            shape = cssValuePool().createIdentifierValue(value->id);
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

PassRefPtr<CSSValue> CSSParser::parseTextIndent()
{
    // <length> | <percentage> | inherit  when CSS3_TEXT is disabled.
    // [ <length> | <percentage> ] && [ -webkit-hanging || -webkit-each-line ]? | inherit  when CSS3_TEXT is enabled.
    RefPtr<CSSValueList> list = CSSValueList::createSpaceSeparated();
    bool hasLengthOrPercentage = false;
#if ENABLE(CSS3_TEXT)
    bool hasEachLine = false;
    bool hasHanging = false;
#endif

    CSSParserValue* value = m_valueList->current();
    while (value) {
        if (!hasLengthOrPercentage && validUnit(value, FLength | FPercent)) {
            list->append(createPrimitiveNumericValue(value));
            hasLengthOrPercentage = true;
        }
#if ENABLE(CSS3_TEXT)
        else if (!hasEachLine && value->id == CSSValueWebkitEachLine) {
            list->append(cssValuePool().createIdentifierValue(CSSValueWebkitEachLine));
            hasEachLine = true;
        } else if (!hasHanging && value->id == CSSValueWebkitHanging) {
            list->append(cssValuePool().createIdentifierValue(CSSValueWebkitHanging));
            hasHanging = true;
        }
#endif
        else
            return 0;

        value = m_valueList->next();
    }

    if (!hasLengthOrPercentage)
        return 0;

    return list.release();
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

bool CSSParser::parseFontFeatureTag(CSSValueList* settings)
{
    // Feature tag name consists of 4-letter characters.
    static const unsigned tagNameLength = 4;

    CSSParserValue* value = m_valueList->current();
    // Feature tag name comes first
    if (value->unit != CSSPrimitiveValue::CSS_STRING)
        return false;
    if (value->string.length() != tagNameLength)
        return false;
    for (unsigned i = 0; i < tagNameLength; ++i) {
        // Limits the range of characters to 0x20-0x7E, following the tag name rules defiend in the OpenType specification.
        UChar character = value->string[i];
        if (character < 0x20 || character > 0x7E)
            return false;
    }

    String tag = value->string;
    int tagValue = 1;
    // Feature tag values could follow: <integer> | on | off
    value = m_valueList->next();
    if (value) {
        if (value->unit == CSSPrimitiveValue::CSS_NUMBER && value->isInt && value->fValue >= 0) {
            tagValue = clampToInteger(value->fValue);
            if (tagValue < 0)
                return false;
            m_valueList->next();
        } else if (value->id == CSSValueOn || value->id == CSSValueOff) {
            tagValue = value->id == CSSValueOn;
            m_valueList->next();
        }
    }
    settings->append(FontFeatureValue::create(tag, tagValue));
    return true;
}

bool CSSParser::parseFontFeatureSettings(bool important)
{
    if (m_valueList->size() == 1 && m_valueList->current()->id == CSSValueNormal) {
        RefPtr<CSSPrimitiveValue> normalValue = cssValuePool().createIdentifierValue(CSSValueNormal);
        m_valueList->next();
        addProperty(CSSPropertyWebkitFontFeatureSettings, normalValue.release(), important);
        return true;
    }

    RefPtr<CSSValueList> settings = CSSValueList::createCommaSeparated();
    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (!parseFontFeatureTag(settings.get()))
            return false;

        // If the list isn't parsed fully, the current value should be comma.
        value = m_valueList->current();
        if (value && !isComma(value))
            return false;
    }
    if (settings->length()) {
        addProperty(CSSPropertyWebkitFontFeatureSettings, settings.release(), important);
        return true;
    }
    return false;
}

bool CSSParser::parseFontVariantLigatures(bool important)
{
    RefPtr<CSSValueList> ligatureValues = CSSValueList::createSpaceSeparated();
    bool sawCommonLigaturesValue = false;
    bool sawDiscretionaryLigaturesValue = false;
    bool sawHistoricalLigaturesValue = false;

    for (CSSParserValue* value = m_valueList->current(); value; value = m_valueList->next()) {
        if (value->unit != CSSPrimitiveValue::CSS_IDENT)
            return false;

        switch (value->id) {
        case CSSValueNoCommonLigatures:
        case CSSValueCommonLigatures:
            if (sawCommonLigaturesValue)
                return false;
            sawCommonLigaturesValue = true;
            ligatureValues->append(cssValuePool().createIdentifierValue(value->id));
            break;
        case CSSValueNoDiscretionaryLigatures:
        case CSSValueDiscretionaryLigatures:
            if (sawDiscretionaryLigaturesValue)
                return false;
            sawDiscretionaryLigaturesValue = true;
            ligatureValues->append(cssValuePool().createIdentifierValue(value->id));
            break;
        case CSSValueNoHistoricalLigatures:
        case CSSValueHistoricalLigatures:
            if (sawHistoricalLigaturesValue)
                return false;
            sawHistoricalLigaturesValue = true;
            ligatureValues->append(cssValuePool().createIdentifierValue(value->id));
            break;
        default:
            return false;
        }
    }

    if (!ligatureValues->length())
        return false;

    addProperty(CSSPropertyWebkitFontVariantLigatures, ligatureValues.release(), important);
    return true;
}

bool CSSParser::parseCalculation(CSSParserValue* value, CalculationPermittedValueRange range) 
{
    ASSERT(isCalculation(value));
    
    CSSParserValueList* args = value->function->args.get();
    if (!args || !args->size())
        return false;

    ASSERT(!m_parsedCalculation);
    m_parsedCalculation = CSSCalcValue::create(value->function->name, args, range);
    
    if (!m_parsedCalculation)
        return false;
    
    return true;
}

#define END_TOKEN 0

#include "CSSGrammar.h"

enum CharacterType {
    // Types for the main switch.

    // The first 4 types must be grouped together, as they
    // represent the allowed chars in an identifier.
    CharacterCaselessU,
    CharacterIdentifierStart,
    CharacterNumber,
    CharacterDash,

    CharacterOther,
    CharacterNull,
    CharacterWhiteSpace,
    CharacterEndMediaQuery,
    CharacterEndNthChild,
    CharacterQuote,
    CharacterExclamationMark,
    CharacterHashmark,
    CharacterDollar,
    CharacterAsterisk,
    CharacterPlus,
    CharacterDot,
    CharacterSlash,
    CharacterLess,
    CharacterAt,
    CharacterBackSlash,
    CharacterXor,
    CharacterVerticalBar,
    CharacterTilde,
};

// 128 ASCII codes
static const CharacterType typesOfASCIICharacters[128] = {
/*   0 - Null               */ CharacterNull,
/*   1 - Start of Heading   */ CharacterOther,
/*   2 - Start of Text      */ CharacterOther,
/*   3 - End of Text        */ CharacterOther,
/*   4 - End of Transm.     */ CharacterOther,
/*   5 - Enquiry            */ CharacterOther,
/*   6 - Acknowledgment     */ CharacterOther,
/*   7 - Bell               */ CharacterOther,
/*   8 - Back Space         */ CharacterOther,
/*   9 - Horizontal Tab     */ CharacterWhiteSpace,
/*  10 - Line Feed          */ CharacterWhiteSpace,
/*  11 - Vertical Tab       */ CharacterOther,
/*  12 - Form Feed          */ CharacterWhiteSpace,
/*  13 - Carriage Return    */ CharacterWhiteSpace,
/*  14 - Shift Out          */ CharacterOther,
/*  15 - Shift In           */ CharacterOther,
/*  16 - Data Line Escape   */ CharacterOther,
/*  17 - Device Control 1   */ CharacterOther,
/*  18 - Device Control 2   */ CharacterOther,
/*  19 - Device Control 3   */ CharacterOther,
/*  20 - Device Control 4   */ CharacterOther,
/*  21 - Negative Ack.      */ CharacterOther,
/*  22 - Synchronous Idle   */ CharacterOther,
/*  23 - End of Transmit    */ CharacterOther,
/*  24 - Cancel             */ CharacterOther,
/*  25 - End of Medium      */ CharacterOther,
/*  26 - Substitute         */ CharacterOther,
/*  27 - Escape             */ CharacterOther,
/*  28 - File Separator     */ CharacterOther,
/*  29 - Group Separator    */ CharacterOther,
/*  30 - Record Separator   */ CharacterOther,
/*  31 - Unit Separator     */ CharacterOther,
/*  32 - Space              */ CharacterWhiteSpace,
/*  33 - !                  */ CharacterExclamationMark,
/*  34 - "                  */ CharacterQuote,
/*  35 - #                  */ CharacterHashmark,
/*  36 - $                  */ CharacterDollar,
/*  37 - %                  */ CharacterOther,
/*  38 - &                  */ CharacterOther,
/*  39 - '                  */ CharacterQuote,
/*  40 - (                  */ CharacterOther,
/*  41 - )                  */ CharacterEndNthChild,
/*  42 - *                  */ CharacterAsterisk,
/*  43 - +                  */ CharacterPlus,
/*  44 - ,                  */ CharacterOther,
/*  45 - -                  */ CharacterDash,
/*  46 - .                  */ CharacterDot,
/*  47 - /                  */ CharacterSlash,
/*  48 - 0                  */ CharacterNumber,
/*  49 - 1                  */ CharacterNumber,
/*  50 - 2                  */ CharacterNumber,
/*  51 - 3                  */ CharacterNumber,
/*  52 - 4                  */ CharacterNumber,
/*  53 - 5                  */ CharacterNumber,
/*  54 - 6                  */ CharacterNumber,
/*  55 - 7                  */ CharacterNumber,
/*  56 - 8                  */ CharacterNumber,
/*  57 - 9                  */ CharacterNumber,
/*  58 - :                  */ CharacterOther,
/*  59 - ;                  */ CharacterEndMediaQuery,
/*  60 - <                  */ CharacterLess,
/*  61 - =                  */ CharacterOther,
/*  62 - >                  */ CharacterOther,
/*  63 - ?                  */ CharacterOther,
/*  64 - @                  */ CharacterAt,
/*  65 - A                  */ CharacterIdentifierStart,
/*  66 - B                  */ CharacterIdentifierStart,
/*  67 - C                  */ CharacterIdentifierStart,
/*  68 - D                  */ CharacterIdentifierStart,
/*  69 - E                  */ CharacterIdentifierStart,
/*  70 - F                  */ CharacterIdentifierStart,
/*  71 - G                  */ CharacterIdentifierStart,
/*  72 - H                  */ CharacterIdentifierStart,
/*  73 - I                  */ CharacterIdentifierStart,
/*  74 - J                  */ CharacterIdentifierStart,
/*  75 - K                  */ CharacterIdentifierStart,
/*  76 - L                  */ CharacterIdentifierStart,
/*  77 - M                  */ CharacterIdentifierStart,
/*  78 - N                  */ CharacterIdentifierStart,
/*  79 - O                  */ CharacterIdentifierStart,
/*  80 - P                  */ CharacterIdentifierStart,
/*  81 - Q                  */ CharacterIdentifierStart,
/*  82 - R                  */ CharacterIdentifierStart,
/*  83 - S                  */ CharacterIdentifierStart,
/*  84 - T                  */ CharacterIdentifierStart,
/*  85 - U                  */ CharacterCaselessU,
/*  86 - V                  */ CharacterIdentifierStart,
/*  87 - W                  */ CharacterIdentifierStart,
/*  88 - X                  */ CharacterIdentifierStart,
/*  89 - Y                  */ CharacterIdentifierStart,
/*  90 - Z                  */ CharacterIdentifierStart,
/*  91 - [                  */ CharacterOther,
/*  92 - \                  */ CharacterBackSlash,
/*  93 - ]                  */ CharacterOther,
/*  94 - ^                  */ CharacterXor,
/*  95 - _                  */ CharacterIdentifierStart,
/*  96 - `                  */ CharacterOther,
/*  97 - a                  */ CharacterIdentifierStart,
/*  98 - b                  */ CharacterIdentifierStart,
/*  99 - c                  */ CharacterIdentifierStart,
/* 100 - d                  */ CharacterIdentifierStart,
/* 101 - e                  */ CharacterIdentifierStart,
/* 102 - f                  */ CharacterIdentifierStart,
/* 103 - g                  */ CharacterIdentifierStart,
/* 104 - h                  */ CharacterIdentifierStart,
/* 105 - i                  */ CharacterIdentifierStart,
/* 106 - j                  */ CharacterIdentifierStart,
/* 107 - k                  */ CharacterIdentifierStart,
/* 108 - l                  */ CharacterIdentifierStart,
/* 109 - m                  */ CharacterIdentifierStart,
/* 110 - n                  */ CharacterIdentifierStart,
/* 111 - o                  */ CharacterIdentifierStart,
/* 112 - p                  */ CharacterIdentifierStart,
/* 113 - q                  */ CharacterIdentifierStart,
/* 114 - r                  */ CharacterIdentifierStart,
/* 115 - s                  */ CharacterIdentifierStart,
/* 116 - t                  */ CharacterIdentifierStart,
/* 117 - u                  */ CharacterCaselessU,
/* 118 - v                  */ CharacterIdentifierStart,
/* 119 - w                  */ CharacterIdentifierStart,
/* 120 - x                  */ CharacterIdentifierStart,
/* 121 - y                  */ CharacterIdentifierStart,
/* 122 - z                  */ CharacterIdentifierStart,
/* 123 - {                  */ CharacterEndMediaQuery,
/* 124 - |                  */ CharacterVerticalBar,
/* 125 - }                  */ CharacterOther,
/* 126 - ~                  */ CharacterTilde,
/* 127 - Delete             */ CharacterOther,
};

// Utility functions for the CSS tokenizer.

template <typename CharacterType>
static inline bool isCSSLetter(CharacterType character)
{
    return character >= 128 || typesOfASCIICharacters[character] <= CharacterDash;
}

template <typename CharacterType>
static inline bool isCSSEscape(CharacterType character)
{
    return character >= ' ' && character != 127;
}

template <typename CharacterType>
static inline bool isURILetter(CharacterType character)
{
    return (character >= '*' && character != 127) || (character >= '#' && character <= '&') || character == '!';
}

template <typename CharacterType>
static inline bool isIdentifierStartAfterDash(CharacterType* currentCharacter)
{
    return isASCIIAlpha(currentCharacter[0]) || currentCharacter[0] == '_' || currentCharacter[0] >= 128
        || (currentCharacter[0] == '\\' && isCSSEscape(currentCharacter[1]));
}

template <typename CharacterType>
static inline bool isEqualToCSSIdentifier(CharacterType* cssString, const char* constantString)
{
    // Compare an character memory data with a zero terminated string.
    do {
        // The input must be part of an identifier if constantChar or constString
        // contains '-'. Otherwise toASCIILowerUnchecked('\r') would be equal to '-'.
        ASSERT((*constantString >= 'a' && *constantString <= 'z') || *constantString == '-');
        ASSERT(*constantString != '-' || isCSSLetter(*cssString));
        if (toASCIILowerUnchecked(*cssString++) != (*constantString++))
            return false;
    } while (*constantString);
    return true;
}

template <typename CharacterType>
static inline bool isEqualToCSSCaseSensitiveIdentifier(CharacterType* string, const char* constantString)
{
    do {
        if (*string++ != *constantString++)
            return false;
    } while (*constantString);
    return true;
}

template <typename CharacterType>
static CharacterType* checkAndSkipEscape(CharacterType* currentCharacter)
{
    // Returns with 0, if escape check is failed. Otherwise
    // it returns with the following character.
    ASSERT(*currentCharacter == '\\');

    ++currentCharacter;
    if (!isCSSEscape(*currentCharacter))
        return 0;

    if (isASCIIHexDigit(*currentCharacter)) {
        int length = 6;

        do {
            ++currentCharacter;
        } while (isASCIIHexDigit(*currentCharacter) && --length);

        // Optional space after the escape sequence.
        if (isHTMLSpace(*currentCharacter))
            ++currentCharacter;
        return currentCharacter;
    }
    return currentCharacter + 1;
}

template <typename CharacterType>
static inline CharacterType* skipWhiteSpace(CharacterType* currentCharacter)
{
    while (isHTMLSpace(*currentCharacter))
        ++currentCharacter;
    return currentCharacter;
}

// Main CSS tokenizer functions.

template <>
LChar* CSSParserString::characters<LChar>() const { return characters8(); }

template <>
UChar* CSSParserString::characters<UChar>() const { return characters16(); }

template <>
inline LChar*& CSSParser::currentCharacter<LChar>()
{
    return m_currentCharacter8;
}

template <>
inline UChar*& CSSParser::currentCharacter<UChar>()
{
    return m_currentCharacter16;
}

UChar*& CSSParser::currentCharacter16()
{
    if (!m_currentCharacter16) {
        m_dataStart16 = adoptArrayPtr(new UChar[m_length]);
        m_currentCharacter16 = m_dataStart16.get();
    }

    return m_currentCharacter16;
}

template <>
inline LChar* CSSParser::tokenStart<LChar>()
{
    return m_tokenStart.ptr8;
}

template <>
inline UChar* CSSParser::tokenStart<UChar>()
{
    return m_tokenStart.ptr16;
}

CSSParser::Location CSSParser::currentLocation()
{
    Location location;
    location.lineNumber = m_tokenStartLineNumber;
    if (is8BitSource())
        location.token.init(tokenStart<LChar>(), currentCharacter<LChar>() - tokenStart<LChar>());
    else
        location.token.init(tokenStart<UChar>(), currentCharacter<UChar>() - tokenStart<UChar>());
    return location;
}

template <typename CharacterType>
inline bool CSSParser::isIdentifierStart()
{
    // Check whether an identifier is started.
    return isIdentifierStartAfterDash((*currentCharacter<CharacterType>() != '-') ? currentCharacter<CharacterType>() : currentCharacter<CharacterType>() + 1);
}

template <typename CharacterType>
static inline CharacterType* checkAndSkipString(CharacterType* currentCharacter, int quote)
{
    // Returns with 0, if string check is failed. Otherwise
    // it returns with the following character. This is necessary
    // since we cannot revert escape sequences, thus strings
    // must be validated before parsing.
    while (true) {
        if (UNLIKELY(*currentCharacter == quote)) {
            // String parsing is successful.
            return currentCharacter + 1;
        }
        if (UNLIKELY(!*currentCharacter)) {
            // String parsing is successful up to end of input.
            return currentCharacter;
        }
        if (UNLIKELY(*currentCharacter <= '\r' && (*currentCharacter == '\n' || (*currentCharacter | 0x1) == '\r'))) {
            // String parsing is failed for character '\n', '\f' or '\r'.
            return 0;
        }

        if (LIKELY(currentCharacter[0] != '\\'))
            ++currentCharacter;
        else if (currentCharacter[1] == '\n' || currentCharacter[1] == '\f')
            currentCharacter += 2;
        else if (currentCharacter[1] == '\r')
            currentCharacter += currentCharacter[2] == '\n' ? 3 : 2;
        else {
            currentCharacter = checkAndSkipEscape(currentCharacter);
            if (!currentCharacter)
                return 0;
        }
    }
}

template <typename CharacterType>
unsigned CSSParser::parseEscape(CharacterType*& src)
{
    ASSERT(*src == '\\' && isCSSEscape(src[1]));

    unsigned unicode = 0;

    ++src;
    if (isASCIIHexDigit(*src)) {

        int length = 6;

        do {
            unicode = (unicode << 4) + toASCIIHexValue(*src++);
        } while (--length && isASCIIHexDigit(*src));

        // Characters above 0x10ffff are not handled.
        if (unicode > 0x10ffff)
            unicode = 0xfffd;

        // Optional space after the escape sequence.
        if (isHTMLSpace(*src))
            ++src;

        return unicode;
    }

    return *currentCharacter<CharacterType>()++;
}

template <>
inline void CSSParser::UnicodeToChars<LChar>(LChar*& result, unsigned unicode)
{
    ASSERT(unicode <= 0xff);
    *result = unicode;

    ++result;
}

template <>
inline void CSSParser::UnicodeToChars<UChar>(UChar*& result, unsigned unicode)
{
    // Replace unicode with a surrogate pairs when it is bigger than 0xffff
    if (U16_LENGTH(unicode) == 2) {
        *result++ = U16_LEAD(unicode);
        *result = U16_TRAIL(unicode);
    } else
        *result = unicode;

    ++result;
}

template <typename SrcCharacterType, typename DestCharacterType>
inline bool CSSParser::parseIdentifierInternal(SrcCharacterType*& src, DestCharacterType*& result, bool& hasEscape)
{
    hasEscape = false;
    do {
        if (LIKELY(*src != '\\'))
            *result++ = *src++;
        else {
            hasEscape = true;
            SrcCharacterType* savedEscapeStart = src;
            unsigned unicode = parseEscape<SrcCharacterType>(src);
            if (unicode > 0xff && sizeof(DestCharacterType) == 1) {
                src = savedEscapeStart;
                return false;
            }
            UnicodeToChars(result, unicode);
        }
    } while (isCSSLetter(src[0]) || (src[0] == '\\' && isCSSEscape(src[1])));

    return true;
}

template <typename CharacterType>
inline void CSSParser::parseIdentifier(CharacterType*& result, CSSParserString& resultString, bool& hasEscape)
{
    // If a valid identifier start is found, we can safely
    // parse the identifier until the next invalid character.
    ASSERT(isIdentifierStart<CharacterType>());

    CharacterType* start = currentCharacter<CharacterType>();
    if (UNLIKELY(!parseIdentifierInternal(currentCharacter<CharacterType>(), result, hasEscape))) {
        // Found an escape we couldn't handle with 8 bits, copy what has been recognized and continue
        ASSERT(is8BitSource());
        UChar*& result16 = currentCharacter16();
        UChar* start16 = result16;
        int i = 0;
        for (; i < result - start; i++)
            result16[i] = start[i];

        result16 += i;

        parseIdentifierInternal(currentCharacter<CharacterType>(), result16, hasEscape);

        resultString.init(start16, result16 - start16);

        return;
    }

    resultString.init(start, result - start);
}

template <typename SrcCharacterType, typename DestCharacterType>
inline bool CSSParser::parseStringInternal(SrcCharacterType*& src, DestCharacterType*& result, UChar quote)
{
    while (true) {
        if (UNLIKELY(*src == quote)) {
            // String parsing is done.
            ++src;
            return true;
        }
        if (UNLIKELY(!*src)) {
            // String parsing is done, but don't advance pointer if at the end of input.
            return true;
        }
        ASSERT(*src > '\r' || (*src < '\n' && *src) || *src == '\v');

        if (LIKELY(src[0] != '\\'))
            *result++ = *src++;
        else if (src[1] == '\n' || src[1] == '\f')
            src += 2;
        else if (src[1] == '\r')
            src += src[2] == '\n' ? 3 : 2;
        else {
            SrcCharacterType* savedEscapeStart = src;
            unsigned unicode = parseEscape<SrcCharacterType>(src);
            if (unicode > 0xff && sizeof(DestCharacterType) == 1) {
                src = savedEscapeStart;
                return false;
            }
            UnicodeToChars(result, unicode);
        }
    }

    return true;
}

template <typename CharacterType>
inline void CSSParser::parseString(CharacterType*& result, CSSParserString& resultString, UChar quote)
{
    CharacterType* start = currentCharacter<CharacterType>();

    if (UNLIKELY(!parseStringInternal(currentCharacter<CharacterType>(), result, quote))) {
        // Found an escape we couldn't handle with 8 bits, copy what has been recognized and continue
        ASSERT(is8BitSource());
        UChar*& result16 = currentCharacter16();
        UChar* start16 = result16;
        int i = 0;
        for (; i < result - start; i++)
            result16[i] = start[i];

        result16 += i;

        parseStringInternal(currentCharacter<CharacterType>(), result16, quote);

        resultString.init(start16, result16 - start16);
        return;
    }

    resultString.init(start, result - start);
}

template <typename CharacterType>
inline bool CSSParser::findURI(CharacterType*& start, CharacterType*& end, UChar& quote)
{
    start = skipWhiteSpace(currentCharacter<CharacterType>());
    
    if (*start == '"' || *start == '\'') {
        quote = *start++;
        end = checkAndSkipString(start, quote);
        if (!end)
            return false;
    } else {
        quote = 0;
        end = start;
        while (isURILetter(*end)) {
            if (LIKELY(*end != '\\'))
                ++end;
            else {
                end = checkAndSkipEscape(end);
                if (!end)
                    return false;
            }
        }
    }

    end = skipWhiteSpace(end);
    if (*end != ')')
        return false;
    
    return true;
}

template <typename SrcCharacterType, typename DestCharacterType>
inline bool CSSParser::parseURIInternal(SrcCharacterType*& src, DestCharacterType*& dest, UChar quote)
{
    if (quote) {
        ASSERT(quote == '"' || quote == '\'');
        return parseStringInternal(src, dest, quote);
    }
    
    while (isURILetter(*src)) {
        if (LIKELY(*src != '\\'))
            *dest++ = *src++;
        else {
            unsigned unicode = parseEscape<SrcCharacterType>(src);
            if (unicode > 0xff && sizeof(SrcCharacterType) == 1)
                return false;
            UnicodeToChars(dest, unicode);
        }
    }

    return true;
}

template <typename CharacterType>
inline void CSSParser::parseURI(CSSParserString& string)
{
    CharacterType* uriStart;
    CharacterType* uriEnd;
    UChar quote;
    if (!findURI(uriStart, uriEnd, quote))
        return;
    
    CharacterType* dest = currentCharacter<CharacterType>() = uriStart;
    if (LIKELY(parseURIInternal(currentCharacter<CharacterType>(), dest, quote)))
        string.init(uriStart, dest - uriStart);
    else {
        // An escape sequence was encountered that can't be stored in 8 bits.
        // Reset the current character to the start of the URI and re-parse with
        // a 16-bit destination.
        ASSERT(is8BitSource());
        UChar* uriStart16 = currentCharacter16();
        currentCharacter<CharacterType>() = uriStart;
        bool result = parseURIInternal(currentCharacter<CharacterType>(), currentCharacter16(), quote);
        ASSERT_UNUSED(result, result);
        string.init(uriStart16, currentCharacter16() - uriStart16);
    }

    currentCharacter<CharacterType>() = uriEnd + 1;
    m_token = URI;
}

template <typename CharacterType>
inline bool CSSParser::parseUnicodeRange()
{
    CharacterType* character = currentCharacter<CharacterType>() + 1;
    int length = 6;
    ASSERT(*currentCharacter<CharacterType>() == '+');

    while (isASCIIHexDigit(*character) && length) {
        ++character;
        --length;
    }

    if (length && *character == '?') {
        // At most 5 hex digit followed by a question mark.
        do {
            ++character;
            --length;
        } while (*character == '?' && length);
        currentCharacter<CharacterType>() = character;
        return true;
    }

    if (length < 6) {
        // At least one hex digit.
        if (character[0] == '-' && isASCIIHexDigit(character[1])) {
            // Followed by a dash and a hex digit.
            ++character;
            length = 6;
            do {
                ++character;
            } while (--length && isASCIIHexDigit(*character));
        }
        currentCharacter<CharacterType>() = character;
        return true;
    }
    return false;
}

template <typename CharacterType>
bool CSSParser::parseNthChild()
{
    CharacterType* character = currentCharacter<CharacterType>();

    while (isASCIIDigit(*character))
        ++character;
    if (isASCIIAlphaCaselessEqual(*character, 'n')) {
        currentCharacter<CharacterType>() = character + 1;
        return true;
    }
    return false;
}

template <typename CharacterType>
bool CSSParser::parseNthChildExtra()
{
    CharacterType* character = skipWhiteSpace(currentCharacter<CharacterType>());
    if (*character != '+' && *character != '-')
        return false;

    character = skipWhiteSpace(character + 1);
    if (!isASCIIDigit(*character))
        return false;

    do {
        ++character;
    } while (isASCIIDigit(*character));

    currentCharacter<CharacterType>() = character;
    return true;
}

template <typename CharacterType>
inline bool CSSParser::detectFunctionTypeToken(int length)
{
    ASSERT(length > 0);
    CharacterType* name = tokenStart<CharacterType>();

    switch (length) {
    case 3:
        if (isASCIIAlphaCaselessEqual(name[0], 'n') && isASCIIAlphaCaselessEqual(name[1], 'o') && isASCIIAlphaCaselessEqual(name[2], 't')) {
            m_token = NOTFUNCTION;
            return true;
        }
        if (isASCIIAlphaCaselessEqual(name[0], 'u') && isASCIIAlphaCaselessEqual(name[1], 'r') && isASCIIAlphaCaselessEqual(name[2], 'l')) {
            m_token = URI;
            return true;
        }
#if ENABLE(VIDEO_TRACK)
        if (isASCIIAlphaCaselessEqual(name[0], 'c') && isASCIIAlphaCaselessEqual(name[1], 'u') && isASCIIAlphaCaselessEqual(name[2], 'e')) {
            m_token = CUEFUNCTION;
            return true;
        }
#endif
        return false;

    case 4:
        if (isEqualToCSSIdentifier(name, "calc")) {
            m_token = CALCFUNCTION;
            return true;
        }
        return false;

    case 9:
        if (isEqualToCSSIdentifier(name, "nth-child")) {
            m_parsingMode = NthChildMode;
            return true;
        }
        return false;

    case 11:
        if (isEqualToCSSIdentifier(name, "nth-of-type")) {
            m_parsingMode = NthChildMode;
            return true;
        }
        return false;

    case 14:
        if (isEqualToCSSIdentifier(name, "nth-last-child")) {
            m_parsingMode = NthChildMode;
            return true;
        }
        return false;

    case 16:
        if (isEqualToCSSIdentifier(name, "nth-last-of-type")) {
            m_parsingMode = NthChildMode;
            return true;
        }
        return false;
    }

    return false;
}

template <typename CharacterType>
inline void CSSParser::detectMediaQueryToken(int length)
{
    ASSERT(m_parsingMode == MediaQueryMode);
    CharacterType* name = tokenStart<CharacterType>();

    if (length == 3) {
        if (isASCIIAlphaCaselessEqual(name[0], 'a') && isASCIIAlphaCaselessEqual(name[1], 'n') && isASCIIAlphaCaselessEqual(name[2], 'd'))
            m_token = MEDIA_AND;
        else if (isASCIIAlphaCaselessEqual(name[0], 'n') && isASCIIAlphaCaselessEqual(name[1], 'o') && isASCIIAlphaCaselessEqual(name[2], 't'))
            m_token = MEDIA_NOT;
    } else if (length == 4) {
        if (isASCIIAlphaCaselessEqual(name[0], 'o') && isASCIIAlphaCaselessEqual(name[1], 'n')
                && isASCIIAlphaCaselessEqual(name[2], 'l') && isASCIIAlphaCaselessEqual(name[3], 'y'))
            m_token = MEDIA_ONLY;
    }
}

template <typename CharacterType>
inline void CSSParser::detectNumberToken(CharacterType* type, int length)
{
    ASSERT(length > 0);

    switch (toASCIILowerUnchecked(type[0])) {
    case 'c':
        if (length == 2 && isASCIIAlphaCaselessEqual(type[1], 'm'))
            m_token = CMS;
        else if (length == 2 && isASCIIAlphaCaselessEqual(type[1], 'h'))
            m_token = CHS;
        return;

    case 'd':
        if (length == 3 && isASCIIAlphaCaselessEqual(type[1], 'e') && isASCIIAlphaCaselessEqual(type[2], 'g'))
            m_token = DEGS;
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
        else if (length > 2 && isASCIIAlphaCaselessEqual(type[1], 'p')) {
            if (length == 4) {
                // There is a discussion about the name of this unit on www-style.
                // Keep this compile time guard in place until that is resolved.
                // http://lists.w3.org/Archives/Public/www-style/2012May/0915.html
                if (isASCIIAlphaCaselessEqual(type[2], 'p') && isASCIIAlphaCaselessEqual(type[3], 'x'))
                    m_token = DPPX;
                else if (isASCIIAlphaCaselessEqual(type[2], 'c') && isASCIIAlphaCaselessEqual(type[3], 'm'))
                    m_token = DPCM;
            } else if (length == 3 && isASCIIAlphaCaselessEqual(type[2], 'i'))
                    m_token = DPI;
        }
#endif
        return;

    case 'e':
        if (length == 2) {
            if (isASCIIAlphaCaselessEqual(type[1], 'm'))
                m_token = EMS;
            else if (isASCIIAlphaCaselessEqual(type[1], 'x'))
                m_token = EXS;
        }
        return;

    case 'g':
        if (length == 4 && isASCIIAlphaCaselessEqual(type[1], 'r')
                && isASCIIAlphaCaselessEqual(type[2], 'a') && isASCIIAlphaCaselessEqual(type[3], 'd'))
            m_token = GRADS;
        return;

    case 'h':
        if (length == 2 && isASCIIAlphaCaselessEqual(type[1], 'z'))
            m_token = HERTZ;
        return;

    case 'i':
        if (length == 2 && isASCIIAlphaCaselessEqual(type[1], 'n'))
            m_token = INS;
        return;

    case 'k':
        if (length == 3 && isASCIIAlphaCaselessEqual(type[1], 'h') && isASCIIAlphaCaselessEqual(type[2], 'z'))
            m_token = KHERTZ;
        return;

    case 'm':
        if (length == 2) {
            if (isASCIIAlphaCaselessEqual(type[1], 'm'))
                m_token = MMS;
            else if (isASCIIAlphaCaselessEqual(type[1], 's'))
                m_token = MSECS;
        }
        return;

    case 'p':
        if (length == 2) {
            if (isASCIIAlphaCaselessEqual(type[1], 'x'))
                m_token = PXS;
            else if (isASCIIAlphaCaselessEqual(type[1], 't'))
                m_token = PTS;
            else if (isASCIIAlphaCaselessEqual(type[1], 'c'))
                m_token = PCS;
        }
        return;

    case 'r':
        if (length == 3) {
            if (isASCIIAlphaCaselessEqual(type[1], 'a') && isASCIIAlphaCaselessEqual(type[2], 'd'))
                m_token = RADS;
            else if (isASCIIAlphaCaselessEqual(type[1], 'e') && isASCIIAlphaCaselessEqual(type[2], 'm'))
                m_token = REMS;
        }
        return;

    case 's':
        if (length == 1)
            m_token = SECS;
        return;

    case 't':
        if (length == 4 && isASCIIAlphaCaselessEqual(type[1], 'u')
                && isASCIIAlphaCaselessEqual(type[2], 'r') && isASCIIAlphaCaselessEqual(type[3], 'n'))
            m_token = TURNS;
        return;
    case 'v':
        if (length == 2) {
            if (isASCIIAlphaCaselessEqual(type[1], 'w'))
                m_token = VW;
            else if (isASCIIAlphaCaselessEqual(type[1], 'h'))
                m_token = VH;
        } else if (length == 4 && isASCIIAlphaCaselessEqual(type[1], 'm')) {
            if (isASCIIAlphaCaselessEqual(type[2], 'i') && isASCIIAlphaCaselessEqual(type[3], 'n'))
                m_token = VMIN;
            else if (isASCIIAlphaCaselessEqual(type[2], 'a') && isASCIIAlphaCaselessEqual(type[3], 'x'))
                m_token = VMAX;
        }
        return;

    default:
        if (type[0] == '_' && length == 5 && type[1] == '_' && isASCIIAlphaCaselessEqual(type[2], 'q')
                && isASCIIAlphaCaselessEqual(type[3], 'e') && isASCIIAlphaCaselessEqual(type[4], 'm'))
            m_token = QEMS;
        return;
    }
}

template <typename CharacterType>
inline void CSSParser::detectDashToken(int length)
{
    CharacterType* name = tokenStart<CharacterType>();

    if (length == 11) {
        if (isASCIIAlphaCaselessEqual(name[10], 'y') && isEqualToCSSIdentifier(name + 1, "webkit-an"))
            m_token = ANYFUNCTION;
        else if (isASCIIAlphaCaselessEqual(name[10], 'n') && isEqualToCSSIdentifier(name + 1, "webkit-mi"))
            m_token = MINFUNCTION;
        else if (isASCIIAlphaCaselessEqual(name[10], 'x') && isEqualToCSSIdentifier(name + 1, "webkit-ma"))
            m_token = MAXFUNCTION;
#if ENABLE(CSS_VARIABLES)
        else if (cssVariablesEnabled() && isASCIIAlphaCaselessEqual(name[10], 'r') && isEqualToCSSIdentifier(name + 1, "webkit-va"))
            m_token = VARFUNCTION;
#endif
    } else if (length == 12 && isEqualToCSSIdentifier(name + 1, "webkit-calc"))
        m_token = CALCFUNCTION;
#if ENABLE(SHADOW_DOM)
    else if (length == 19 && isEqualToCSSIdentifier(name + 1, "webkit-distributed"))
        m_token = DISTRIBUTEDFUNCTION;
#endif
}

template <typename CharacterType>
inline void CSSParser::detectAtToken(int length, bool hasEscape)
{
    CharacterType* name = tokenStart<CharacterType>();
    ASSERT(name[0] == '@' && length >= 2);

    // charset, font-face, import, media, namespace, page, supports,
    // -webkit-keyframes, and -webkit-mediaquery are not affected by hasEscape.
    switch (toASCIILowerUnchecked(name[1])) {
    case 'b':
        if (hasEscape)
            return;

        switch (length) {
        case 12:
            if (isEqualToCSSIdentifier(name + 2, "ottom-left"))
                m_token = BOTTOMLEFT_SYM;
            return;

        case 13:
            if (isEqualToCSSIdentifier(name + 2, "ottom-right"))
                m_token = BOTTOMRIGHT_SYM;
            return;

        case 14:
            if (isEqualToCSSIdentifier(name + 2, "ottom-center"))
                m_token = BOTTOMCENTER_SYM;
            return;

        case 19:
            if (isEqualToCSSIdentifier(name + 2, "ottom-left-corner"))
                m_token = BOTTOMLEFTCORNER_SYM;
            return;

        case 20:
            if (isEqualToCSSIdentifier(name + 2, "ottom-right-corner"))
                m_token = BOTTOMRIGHTCORNER_SYM;
            return;
        }
        return;

    case 'c':
        if (length == 8 && isEqualToCSSIdentifier(name + 2, "harset"))
            m_token = CHARSET_SYM;
        return;

    case 'f':
        if (length == 10 && isEqualToCSSIdentifier(name + 2, "ont-face"))
            m_token = FONT_FACE_SYM;
        return;

#if ENABLE(SHADOW_DOM)
    case 'h':
        if (length == 5 && isEqualToCSSIdentifier(name + 2, "ost"))
            m_token = HOST_SYM;
        return;
#endif

    case 'i':
        if (length == 7 && isEqualToCSSIdentifier(name + 2, "mport")) {
            m_parsingMode = MediaQueryMode;
            m_token = IMPORT_SYM;
        }
        return;

    case 'l':
        if (hasEscape)
            return;

        if (length == 9) {
            if (isEqualToCSSIdentifier(name + 2, "eft-top"))
                m_token = LEFTTOP_SYM;
        } else if (length == 12) {
            // Checking the last character first could further reduce the possibile cases.
            if (isASCIIAlphaCaselessEqual(name[11], 'e') && isEqualToCSSIdentifier(name + 2, "eft-middl"))
                m_token = LEFTMIDDLE_SYM;
            else if (isASCIIAlphaCaselessEqual(name[11], 'm') && isEqualToCSSIdentifier(name + 2, "eft-botto"))
                m_token = LEFTBOTTOM_SYM;
        }
        return;

    case 'm':
        if (length == 6 && isEqualToCSSIdentifier(name + 2, "edia")) {
            m_parsingMode = MediaQueryMode;
            m_token = MEDIA_SYM;
        }
        return;

    case 'n':
        if (length == 10 && isEqualToCSSIdentifier(name + 2, "amespace"))
            m_token = NAMESPACE_SYM;
        return;

    case 'p':
        if (length == 5 && isEqualToCSSIdentifier(name + 2, "age"))
            m_token = PAGE_SYM;
        return;

    case 'r':
        if (hasEscape)
            return;

        if (length == 10) {
            if (isEqualToCSSIdentifier(name + 2, "ight-top"))
                m_token = RIGHTTOP_SYM;
        } else if (length == 13) {
            // Checking the last character first could further reduce the possibile cases.
            if (isASCIIAlphaCaselessEqual(name[12], 'e') && isEqualToCSSIdentifier(name + 2, "ight-middl"))
                m_token = RIGHTMIDDLE_SYM;
            else if (isASCIIAlphaCaselessEqual(name[12], 'm') && isEqualToCSSIdentifier(name + 2, "ight-botto"))
                m_token = RIGHTBOTTOM_SYM;
        }
        return;

#if ENABLE(CSS3_CONDITIONAL_RULES)
    case 's':
        if (length == 9 && isEqualToCSSIdentifier(name + 2, "upports")) {
            m_parsingMode = SupportsMode;
            m_token = SUPPORTS_SYM;
        }
        return;
#endif

    case 't':
        if (hasEscape)
            return;

        switch (length) {
        case 9:
            if (isEqualToCSSIdentifier(name + 2, "op-left"))
                m_token = TOPLEFT_SYM;
            return;

        case 10:
            if (isEqualToCSSIdentifier(name + 2, "op-right"))
                m_token = TOPRIGHT_SYM;
            return;

        case 11:
            if (isEqualToCSSIdentifier(name + 2, "op-center"))
                m_token = TOPCENTER_SYM;
            return;

        case 16:
            if (isEqualToCSSIdentifier(name + 2, "op-left-corner"))
                m_token = TOPLEFTCORNER_SYM;
            return;

        case 17:
            if (isEqualToCSSIdentifier(name + 2, "op-right-corner"))
                m_token = TOPRIGHTCORNER_SYM;
            return;
        }
        return;

    case '-':
        switch (length) {
        case 13:
            if (!hasEscape && isEqualToCSSIdentifier(name + 2, "webkit-rule"))
                m_token = WEBKIT_RULE_SYM;
            return;

        case 14:
            if (hasEscape)
                return;

            // Checking the last character first could further reduce the possibile cases.
            if (isASCIIAlphaCaselessEqual(name[13], 's') && isEqualToCSSIdentifier(name + 2, "webkit-decl"))
                m_token = WEBKIT_DECLS_SYM;
            else if (isASCIIAlphaCaselessEqual(name[13], 'e') && isEqualToCSSIdentifier(name + 2, "webkit-valu"))
                m_token = WEBKIT_VALUE_SYM;
            return;

        case 15:
            if (hasEscape)
                return;

#if ENABLE(CSS_REGIONS)
            if (isASCIIAlphaCaselessEqual(name[14], 'n') && isEqualToCSSIdentifier(name + 2, "webkit-regio")) {
                m_token = WEBKIT_REGION_RULE_SYM;
                return;
            }
#endif
#if ENABLE(CSS_SHADERS)
            if (isASCIIAlphaCaselessEqual(name[14], 'r') && isEqualToCSSIdentifier(name + 2, "webkit-filte")) {
                m_token = WEBKIT_FILTER_RULE_SYM;
                return;
            }
#endif
            return;

        case 17:
            if (hasEscape)
                return;

            if (isASCIIAlphaCaselessEqual(name[16], 'r') && isEqualToCSSIdentifier(name + 2, "webkit-selecto"))
                m_token = WEBKIT_SELECTOR_SYM;
#if ENABLE(CSS_DEVICE_ADAPTATION)
            else if (isASCIIAlphaCaselessEqual(name[16], 't') && isEqualToCSSIdentifier(name + 2, "webkit-viewpor"))
                m_token = WEBKIT_VIEWPORT_RULE_SYM;
#endif
            return;

        case 18:
            if (isEqualToCSSIdentifier(name + 2, "webkit-keyframes"))
                m_token = WEBKIT_KEYFRAMES_SYM;
            return;

        case 19:
            if (isEqualToCSSIdentifier(name + 2, "webkit-mediaquery")) {
                m_parsingMode = MediaQueryMode;
                m_token = WEBKIT_MEDIAQUERY_SYM;
            }
            return;

        case 22:
            if (!hasEscape && isEqualToCSSIdentifier(name + 2, "webkit-keyframe-rule"))
                m_token = WEBKIT_KEYFRAME_RULE_SYM;
            return;

        case 27:
#if ENABLE(CSS3_CONDITIONAL_RULES)
            if (isEqualToCSSIdentifier(name + 2, "webkit-supports-condition")) {
                m_parsingMode = SupportsMode;
                m_token = WEBKIT_SUPPORTS_CONDITION_SYM;
            }
#endif
            return;
        }
    }
}

#if ENABLE(CSS3_CONDITIONAL_RULES)
template <typename CharacterType>
inline void CSSParser::detectSupportsToken(int length)
{
    ASSERT(m_parsingMode == SupportsMode);
    CharacterType* name = tokenStart<CharacterType>();

    if (length == 2) {
        if (isASCIIAlphaCaselessEqual(name[0], 'o') && isASCIIAlphaCaselessEqual(name[1], 'r'))
            m_token = SUPPORTS_OR;
    } else if (length == 3) {
        if (isASCIIAlphaCaselessEqual(name[0], 'a') && isASCIIAlphaCaselessEqual(name[1], 'n') && isASCIIAlphaCaselessEqual(name[2], 'd'))
            m_token = SUPPORTS_AND;
        else if (isASCIIAlphaCaselessEqual(name[0], 'n') && isASCIIAlphaCaselessEqual(name[1], 'o') && isASCIIAlphaCaselessEqual(name[2], 't'))
            m_token = SUPPORTS_NOT;
    }
}
#endif

template <typename SrcCharacterType>
int CSSParser::realLex(void* yylvalWithoutType)
{
    YYSTYPE* yylval = static_cast<YYSTYPE*>(yylvalWithoutType);
    // Write pointer for the next character.
    SrcCharacterType* result;
    CSSParserString resultString;
    bool hasEscape;

    // The input buffer is terminated by a \0 character, so
    // it is safe to read one character ahead of a known non-null.
#ifndef NDEBUG
    // In debug we check with an ASSERT that the length is > 0 for string types.
    yylval->string.clear();
#endif

restartAfterComment:
    result = currentCharacter<SrcCharacterType>();
    setTokenStart(result);
    m_tokenStartLineNumber = m_lineNumber;
    m_token = *currentCharacter<SrcCharacterType>();
    ++currentCharacter<SrcCharacterType>();

    switch ((m_token <= 127) ? typesOfASCIICharacters[m_token] : CharacterIdentifierStart) {
    case CharacterCaselessU:
        if (UNLIKELY(*currentCharacter<SrcCharacterType>() == '+'))
            if (parseUnicodeRange<SrcCharacterType>()) {
                m_token = UNICODERANGE;
                yylval->string.init(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
                break;
            }
        // Fall through to CharacterIdentifierStart.

    case CharacterIdentifierStart:
        --currentCharacter<SrcCharacterType>();
        parseIdentifier(result, yylval->string, hasEscape);
        m_token = IDENT;

        if (UNLIKELY(*currentCharacter<SrcCharacterType>() == '(')) {
#if ENABLE(CSS3_CONDITIONAL_RULES)
            if (m_parsingMode == SupportsMode && !hasEscape) {
                detectSupportsToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
                if (m_token != IDENT)
                    break;
            }
#endif
            m_token = FUNCTION;
            bool shouldSkipParenthesis = true;
            if (!hasEscape) {
                bool detected = detectFunctionTypeToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
                if (!detected && m_parsingMode == MediaQueryMode) {
                    // ... and(max-width: 480px) ... looks like a function, but in fact it is not,
                    // so run more detection code in the MediaQueryMode.
                    detectMediaQueryToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
                    shouldSkipParenthesis = false;
                }
            }

            if (LIKELY(shouldSkipParenthesis)) {
                ++currentCharacter<SrcCharacterType>();
                ++result;
                ++yylval->string.m_length;
            }

            if (token() == URI) {
                m_token = FUNCTION;
                // Check whether it is really an URI.
                if (yylval->string.is8Bit())
                    parseURI<LChar>(yylval->string);
                else
                    parseURI<UChar>(yylval->string);
            }
        } else if (UNLIKELY(m_parsingMode != NormalMode) && !hasEscape) {
            if (m_parsingMode == MediaQueryMode)
                detectMediaQueryToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
#if ENABLE(CSS3_CONDITIONAL_RULES)
            else if (m_parsingMode == SupportsMode)
                detectSupportsToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
#endif
            else if (m_parsingMode == NthChildMode && isASCIIAlphaCaselessEqual(tokenStart<SrcCharacterType>()[0], 'n')) {
                if (result - tokenStart<SrcCharacterType>() == 1) {
                    // String "n" is IDENT but "n+1" is NTH.
                    if (parseNthChildExtra<SrcCharacterType>()) {
                        m_token = NTH;
                        yylval->string.m_length = currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>();
                    }
                } else if (result - tokenStart<SrcCharacterType>() >= 2 && tokenStart<SrcCharacterType>()[1] == '-') {
                    // String "n-" is IDENT but "n-1" is NTH.
                    // Set currentCharacter to '-' to continue parsing.
                    SrcCharacterType* nextCharacter = result;
                    currentCharacter<SrcCharacterType>() = tokenStart<SrcCharacterType>() + 1;
                    if (parseNthChildExtra<SrcCharacterType>()) {
                        m_token = NTH;
                        yylval->string.setLength(currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
                    } else {
                        // Revert the change to currentCharacter if unsuccessful.
                        currentCharacter<SrcCharacterType>() = nextCharacter;
                    }
                }
            }
        }
        break;

    case CharacterDot:
        if (!isASCIIDigit(currentCharacter<SrcCharacterType>()[0]))
            break;
        // Fall through to CharacterNumber.

    case CharacterNumber: {
        bool dotSeen = (m_token == '.');

        while (true) {
            if (!isASCIIDigit(currentCharacter<SrcCharacterType>()[0])) {
                // Only one dot is allowed for a number,
                // and it must be followed by a digit.
                if (currentCharacter<SrcCharacterType>()[0] != '.' || dotSeen || !isASCIIDigit(currentCharacter<SrcCharacterType>()[1]))
                    break;
                dotSeen = true;
            }
            ++currentCharacter<SrcCharacterType>();
        }

        if (UNLIKELY(m_parsingMode == NthChildMode) && !dotSeen && isASCIIAlphaCaselessEqual(*currentCharacter<SrcCharacterType>(), 'n')) {
            // "[0-9]+n" is always an NthChild.
            ++currentCharacter<SrcCharacterType>();
            parseNthChildExtra<SrcCharacterType>();
            m_token = NTH;
            yylval->string.init(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
            break;
        }

#if ENABLE(SVG)
        // Use SVG parser for numbers on SVG presentation attributes.
        if (m_context.mode == SVGAttributeMode) {
            // We need to take care of units like 'em' or 'ex'.
            SrcCharacterType* character = currentCharacter<SrcCharacterType>();
            if (isASCIIAlphaCaselessEqual(*character, 'e')) {
                ASSERT(character - tokenStart<SrcCharacterType>() > 0);
                ++character;
                if (*character == '-' || *character == '+' || isASCIIDigit(*character)) {
                    ++character;
                    while (isASCIIDigit(*character))
                        ++character;
                    // Use FLOATTOKEN if the string contains exponents.
                    dotSeen = true;
                    currentCharacter<SrcCharacterType>() = character;
                }
            }
            if (!parseSVGNumber(tokenStart<SrcCharacterType>(), character - tokenStart<SrcCharacterType>(), yylval->number))
                break;
        } else
#endif
            yylval->number = charactersToDouble(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
 
        // Type of the function.
        if (isIdentifierStart<SrcCharacterType>()) {
            SrcCharacterType* type = currentCharacter<SrcCharacterType>();
            result = currentCharacter<SrcCharacterType>();

            parseIdentifier(result, resultString, hasEscape);

            m_token = DIMEN;
            if (!hasEscape)
                detectNumberToken(type, currentCharacter<SrcCharacterType>() - type);

            if (m_token == DIMEN) {
                // The decoded number is overwritten, but this is intentional.
                yylval->string.init(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
            }
        } else if (*currentCharacter<SrcCharacterType>() == '%') {
            // Although the CSS grammar says {num}% we follow
            // webkit at the moment which uses {num}%+.
            do {
                ++currentCharacter<SrcCharacterType>();
            } while (*currentCharacter<SrcCharacterType>() == '%');
            m_token = PERCENTAGE;
        } else
            m_token = dotSeen ? FLOATTOKEN : INTEGER;
        break;
    }

    case CharacterDash:
        if (isIdentifierStartAfterDash(currentCharacter<SrcCharacterType>())) {
            --currentCharacter<SrcCharacterType>();
            parseIdentifier(result, resultString, hasEscape);
            m_token = IDENT;

#if ENABLE(CSS_VARIABLES)
            if (cssVariablesEnabled() && isEqualToCSSCaseSensitiveIdentifier(tokenStart<SrcCharacterType>() + 1, "webkit-var") && tokenStart<SrcCharacterType>()[11] == '-' && isIdentifierStartAfterDash(tokenStart<SrcCharacterType>() + 12))
                m_token = VAR_DEFINITION;
            else
#endif
            if (*currentCharacter<SrcCharacterType>() == '(') {
                m_token = FUNCTION;
                if (!hasEscape)
                    detectDashToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>());
                ++currentCharacter<SrcCharacterType>();
                ++result;
            } else if (UNLIKELY(m_parsingMode == NthChildMode) && !hasEscape && isASCIIAlphaCaselessEqual(tokenStart<SrcCharacterType>()[1], 'n')) {
                if (result - tokenStart<SrcCharacterType>() == 2) {
                    // String "-n" is IDENT but "-n+1" is NTH.
                    if (parseNthChildExtra<SrcCharacterType>()) {
                        m_token = NTH;
                        result = currentCharacter<SrcCharacterType>();
                    }
                } else if (result - tokenStart<SrcCharacterType>() >= 3 && tokenStart<SrcCharacterType>()[2] == '-') {
                    // String "-n-" is IDENT but "-n-1" is NTH.
                    // Set currentCharacter to second '-' of '-n-' to continue parsing.
                    SrcCharacterType* nextCharacter = result;
                    currentCharacter<SrcCharacterType>() = tokenStart<SrcCharacterType>() + 2;
                    if (parseNthChildExtra<SrcCharacterType>()) {
                        m_token = NTH;
                        result = currentCharacter<SrcCharacterType>();
                    } else {
                        // Revert the change to currentCharacter if unsuccessful.
                        currentCharacter<SrcCharacterType>() = nextCharacter;
                    }
                }
            }
            resultString.setLength(result - tokenStart<SrcCharacterType>());
            yylval->string = resultString;
        } else if (currentCharacter<SrcCharacterType>()[0] == '-' && currentCharacter<SrcCharacterType>()[1] == '>') {
            currentCharacter<SrcCharacterType>() += 2;
            m_token = SGML_CD;
        } else if (UNLIKELY(m_parsingMode == NthChildMode)) {
            // "-[0-9]+n" is always an NthChild.
            if (parseNthChild<SrcCharacterType>()) {
                parseNthChildExtra<SrcCharacterType>();
                m_token = NTH;
                yylval->string.init(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
            }
        }
        break;

    case CharacterOther:
        // m_token is simply the current character.
        break;

    case CharacterNull:
        // Do not advance pointer at the end of input.
        --currentCharacter<SrcCharacterType>();
        break;

    case CharacterWhiteSpace:
        m_token = WHITESPACE;
        // Might start with a '\n'.
        --currentCharacter<SrcCharacterType>();
        do {
            if (*currentCharacter<SrcCharacterType>() == '\n')
                ++m_lineNumber;
            ++currentCharacter<SrcCharacterType>();
        } while (*currentCharacter<SrcCharacterType>() <= ' ' && (typesOfASCIICharacters[*currentCharacter<SrcCharacterType>()] == CharacterWhiteSpace));
        break;

    case CharacterEndMediaQuery:
        if (m_parsingMode == MediaQueryMode)
            m_parsingMode = NormalMode;
        break;

    case CharacterEndNthChild:
        if (m_parsingMode == NthChildMode)
            m_parsingMode = NormalMode;
        break;

    case CharacterQuote:
        if (checkAndSkipString(currentCharacter<SrcCharacterType>(), m_token)) {
            ++result;
            parseString<SrcCharacterType>(result, yylval->string, m_token);
            m_token = STRING;
        }
        break;

    case CharacterExclamationMark: {
        SrcCharacterType* start = skipWhiteSpace(currentCharacter<SrcCharacterType>());
        if (isEqualToCSSIdentifier(start, "important")) {
            m_token = IMPORTANT_SYM;
            currentCharacter<SrcCharacterType>() = start + 9;
        }
        break;
    }

    case CharacterHashmark: {
        SrcCharacterType* start = currentCharacter<SrcCharacterType>();
        result = currentCharacter<SrcCharacterType>();

        if (isASCIIDigit(*currentCharacter<SrcCharacterType>())) {
            // This must be a valid hex number token.
            do {
                ++currentCharacter<SrcCharacterType>();
            } while (isASCIIHexDigit(*currentCharacter<SrcCharacterType>()));
            m_token = HEX;
            yylval->string.init(start, currentCharacter<SrcCharacterType>() - start);
        } else if (isIdentifierStart<SrcCharacterType>()) {
            m_token = IDSEL;
            parseIdentifier(result, yylval->string, hasEscape);
            if (!hasEscape) {
                // Check whether the identifier is also a valid hex number.
                SrcCharacterType* current = start;
                m_token = HEX;
                do {
                    if (!isASCIIHexDigit(*current)) {
                        m_token = IDSEL;
                        break;
                    }
                    ++current;
                } while (current < result);
            }
        }
        break;
    }

    case CharacterSlash:
        // Ignore comments. They are not even considered as white spaces.
        if (*currentCharacter<SrcCharacterType>() == '*') {
            ++currentCharacter<SrcCharacterType>();
            while (currentCharacter<SrcCharacterType>()[0] != '*' || currentCharacter<SrcCharacterType>()[1] != '/') {
                if (*currentCharacter<SrcCharacterType>() == '\n')
                    ++m_lineNumber;
                if (*currentCharacter<SrcCharacterType>() == '\0') {
                    // Unterminated comments are simply ignored.
                    currentCharacter<SrcCharacterType>() -= 2;
                    break;
                }
                ++currentCharacter<SrcCharacterType>();
            }
            currentCharacter<SrcCharacterType>() += 2;
            goto restartAfterComment;
        }
        break;

    case CharacterDollar:
        if (*currentCharacter<SrcCharacterType>() == '=') {
            ++currentCharacter<SrcCharacterType>();
            m_token = ENDSWITH;
        }
        break;

    case CharacterAsterisk:
        if (*currentCharacter<SrcCharacterType>() == '=') {
            ++currentCharacter<SrcCharacterType>();
            m_token = CONTAINS;
        }
        break;

    case CharacterPlus:
        if (UNLIKELY(m_parsingMode == NthChildMode)) {
            // Simplest case. "+[0-9]*n" is always NthChild.
            if (parseNthChild<SrcCharacterType>()) {
                parseNthChildExtra<SrcCharacterType>();
                m_token = NTH;
                yylval->string.init(tokenStart<SrcCharacterType>(), currentCharacter<SrcCharacterType>() - tokenStart<SrcCharacterType>());
            }
        }
        break;

    case CharacterLess:
        if (currentCharacter<SrcCharacterType>()[0] == '!' && currentCharacter<SrcCharacterType>()[1] == '-' && currentCharacter<SrcCharacterType>()[2] == '-') {
            currentCharacter<SrcCharacterType>() += 3;
            m_token = SGML_CD;
        }
        break;

    case CharacterAt:
        if (isIdentifierStart<SrcCharacterType>()) {
            m_token = ATKEYWORD;
            ++result;
            parseIdentifier(result, resultString, hasEscape);
            detectAtToken<SrcCharacterType>(result - tokenStart<SrcCharacterType>(), hasEscape);
        }
        break;

    case CharacterBackSlash:
        if (isCSSEscape(*currentCharacter<SrcCharacterType>())) {
            --currentCharacter<SrcCharacterType>();
            parseIdentifier(result, yylval->string, hasEscape);
            m_token = IDENT;
        }
        break;

    case CharacterXor:
        if (*currentCharacter<SrcCharacterType>() == '=') {
            ++currentCharacter<SrcCharacterType>();
            m_token = BEGINSWITH;
        }
        break;

    case CharacterVerticalBar:
        if (*currentCharacter<SrcCharacterType>() == '=') {
            ++currentCharacter<SrcCharacterType>();
            m_token = DASHMATCH;
        }
        break;

    case CharacterTilde:
        if (*currentCharacter<SrcCharacterType>() == '=') {
            ++currentCharacter<SrcCharacterType>();
            m_token = INCLUDES;
        }
        break;

    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return token();
}

CSSParserSelector* CSSParser::createFloatingSelectorWithTagName(const QualifiedName& tagQName)
{
    CSSParserSelector* selector = new CSSParserSelector(tagQName);
    m_floatingSelectors.add(selector);
    return selector;
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

PassOwnPtr<CSSParserValueList> CSSParser::sinkFloatingValueList(CSSParserValueList* list)
{
    if (list) {
        ASSERT(m_floatingValueLists.contains(list));
        m_floatingValueLists.remove(list);
    }
    return adoptPtr(list);
}

CSSParserFunction* CSSParser::createFloatingFunction()
{
    CSSParserFunction* function = new CSSParserFunction;
    m_floatingFunctions.add(function);
    return function;
}

PassOwnPtr<CSSParserFunction> CSSParser::sinkFloatingFunction(CSSParserFunction* function)
{
    if (function) {
        ASSERT(m_floatingFunctions.contains(function));
        m_floatingFunctions.remove(function);
    }
    return adoptPtr(function);
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

Vector<RefPtr<StyleKeyframe> >* CSSParser::createFloatingKeyframeVector()
{
    m_floatingKeyframeVector = adoptPtr(new Vector<RefPtr<StyleKeyframe> >());
    return m_floatingKeyframeVector.get();
}

PassOwnPtr<Vector<RefPtr<StyleKeyframe> > > CSSParser::sinkFloatingKeyframeVector(Vector<RefPtr<StyleKeyframe> >* keyframeVector)
{
    ASSERT_UNUSED(keyframeVector, m_floatingKeyframeVector == keyframeVector);
    return m_floatingKeyframeVector.release();
}

MediaQuerySet* CSSParser::createMediaQuerySet()
{
    RefPtr<MediaQuerySet> queries = MediaQuerySet::create();
    MediaQuerySet* result = queries.get();
    m_parsedMediaQuerySets.append(queries.release());
    return result;
}

StyleRuleBase* CSSParser::createImportRule(const CSSParserString& url, MediaQuerySet* media)
{
    if (!media || !m_allowImportRules) {
        popRuleData();
        return 0;
    }
    RefPtr<StyleRuleImport> rule = StyleRuleImport::create(url, media);
    StyleRuleImport* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return result;
}

StyleRuleBase* CSSParser::createMediaRule(MediaQuerySet* media, RuleList* rules)
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    RefPtr<StyleRuleMedia> rule;
    RuleList emptyRules;
    if (!media) {
        // To comply with w3c test suite expectation, create an empty media query
        // even when it is syntactically incorrect.
        rule = StyleRuleMedia::create(MediaQuerySet::create(), emptyRules);
    } else
        rule = StyleRuleMedia::create(media, rules ? *rules : emptyRules);
    StyleRuleMedia* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return result;
}

StyleRuleBase* CSSParser::createEmptyMediaRule(RuleList* rules)
{
    return createMediaRule(MediaQuerySet::create().get(), rules);
}

#if ENABLE(CSS3_CONDITIONAL_RULES)
StyleRuleBase* CSSParser::createSupportsRule(bool conditionIsSupported, RuleList* rules)
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;

    RefPtr<CSSRuleSourceData> data = popSupportsRuleData();
    RefPtr<StyleRuleSupports> rule;
    String conditionText;
    unsigned conditionOffset = data->ruleHeaderRange.start + 9;
    unsigned conditionLength = data->ruleHeaderRange.length() - 9;

    if (is8BitSource())
        conditionText = String(m_dataStart8.get() + conditionOffset, conditionLength).stripWhiteSpace();
    else
        conditionText = String(m_dataStart16.get() + conditionOffset, conditionLength).stripWhiteSpace();

    if (rules)
        rule = StyleRuleSupports::create(conditionText, conditionIsSupported, *rules);
    else {
        RuleList emptyRules;
        rule = StyleRuleSupports::create(conditionText, conditionIsSupported, emptyRules);
    }

    StyleRuleSupports* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();

    return result;
}

void CSSParser::markSupportsRuleHeaderStart()
{
    if (!m_supportsRuleDataStack)
        m_supportsRuleDataStack = adoptPtr(new RuleSourceDataList());

    RefPtr<CSSRuleSourceData> data = CSSRuleSourceData::create(CSSRuleSourceData::SUPPORTS_RULE);
    data->ruleHeaderRange.start = tokenStartOffset();
    m_supportsRuleDataStack->append(data);
}

void CSSParser::markSupportsRuleHeaderEnd()
{
    ASSERT(m_supportsRuleDataStack && !m_supportsRuleDataStack->isEmpty());

    if (is8BitSource())
        m_supportsRuleDataStack->last()->ruleHeaderRange.end = tokenStart<LChar>() - m_dataStart8.get();
    else
        m_supportsRuleDataStack->last()->ruleHeaderRange.end = tokenStart<UChar>() - m_dataStart16.get();
}

PassRefPtr<CSSRuleSourceData> CSSParser::popSupportsRuleData()
{
    ASSERT(m_supportsRuleDataStack && !m_supportsRuleDataStack->isEmpty());
    RefPtr<CSSRuleSourceData> data = m_supportsRuleDataStack->last();
    m_supportsRuleDataStack->removeLast();
    return data.release();
}

#endif

CSSParser::RuleList* CSSParser::createRuleList()
{
    OwnPtr<RuleList> list = adoptPtr(new RuleList);
    RuleList* listPtr = list.get();

    m_parsedRuleLists.append(list.release());
    return listPtr;
}

void CSSParser::processAndAddNewRuleToSourceTreeIfNeeded()
{
    if (!isExtractingSourceData())
        return;
    markRuleBodyEnd();
    RefPtr<CSSRuleSourceData> rule = popRuleData();
    fixUnparsedPropertyRanges(rule.get());
    addNewRuleToSourceTree(rule.release());
}

void CSSParser::addNewRuleToSourceTree(PassRefPtr<CSSRuleSourceData> rule)
{
    // Precondition: (isExtractingSourceData()).
    if (!m_ruleSourceDataResult)
        return;
    if (m_currentRuleDataStack->isEmpty())
        m_ruleSourceDataResult->append(rule);
    else
        m_currentRuleDataStack->last()->childRules.append(rule);
}

PassRefPtr<CSSRuleSourceData> CSSParser::popRuleData()
{
    if (!m_ruleSourceDataResult)
        return 0;

    ASSERT(!m_currentRuleDataStack->isEmpty());
    m_currentRuleData.clear();
    RefPtr<CSSRuleSourceData> data = m_currentRuleDataStack->last();
    m_currentRuleDataStack->removeLast();
    return data.release();
}

void CSSParser::syntaxError(const Location& location, SyntaxErrorType error)
{
    if (!isLoggingErrors())
        return;
    StringBuilder builder;
    switch (error) {
    case PropertyDeclarationError:
        builder.appendLiteral("Invalid CSS property declaration at: ");
        break;

    default:
        builder.appendLiteral("Unexpected CSS token: ");
    }

    if (location.token.is8Bit())
        builder.append(location.token.characters8(), location.token.length());
    else
        builder.append(location.token.characters16(), location.token.length());

    logError(builder.toString(), location.lineNumber);

    m_ignoreErrorsInDeclaration = true;
}

bool CSSParser::isLoggingErrors()
{
    return m_logErrors && !m_ignoreErrorsInDeclaration;
}

void CSSParser::logError(const String& message, int lineNumber)
{
    // FIXME: <http://webkit.org/b/114313> CSS Parser ConsoleMessage errors should include column numbers
    PageConsole* console = m_styleSheet->singleOwnerDocument()->page()->console();
    console->addMessage(CSSMessageSource, WarningMessageLevel, message, m_styleSheet->baseURL().string(), lineNumber + 1, 0);
}

StyleRuleKeyframes* CSSParser::createKeyframesRule(const String& name, PassOwnPtr<Vector<RefPtr<StyleKeyframe> > > popKeyframes)
{
    OwnPtr<Vector<RefPtr<StyleKeyframe> > > keyframes = popKeyframes;
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    RefPtr<StyleRuleKeyframes> rule = StyleRuleKeyframes::create();
    for (size_t i = 0; i < keyframes->size(); ++i)
        rule->parserAppendKeyframe(keyframes->at(i));
    rule->setName(name);
    StyleRuleKeyframes* rulePtr = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return rulePtr;
}

StyleRuleBase* CSSParser::createStyleRule(Vector<OwnPtr<CSSParserSelector> >* selectors)
{
    StyleRule* result = 0;
    if (selectors) {
        m_allowImportRules = m_allowNamespaceDeclarations = false;
        RefPtr<StyleRule> rule = StyleRule::create(m_lastSelectorLineNumber);
        rule->parserAdoptSelectorVector(*selectors);
        if (m_hasFontFaceOnlyValues)
            deleteFontFaceOnlyValues();
        rule->setProperties(createStylePropertySet());
        result = rule.get();
        m_parsedRules.append(rule.release());
        processAndAddNewRuleToSourceTreeIfNeeded();
    } else
        popRuleData();
    clearProperties();
    return result;
}

StyleRuleBase* CSSParser::createFontFaceRule()
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    for (unsigned i = 0; i < m_parsedProperties.size(); ++i) {
        CSSProperty& property = m_parsedProperties[i];
        if (property.id() == CSSPropertyFontVariant && property.value()->isPrimitiveValue())
            property.wrapValueInCommaSeparatedList();
        else if (property.id() == CSSPropertyFontFamily && (!property.value()->isValueList() || static_cast<CSSValueList*>(property.value())->length() != 1)) {
            // Unlike font-family property, font-family descriptor in @font-face rule
            // has to be a value list with exactly one family name. It cannot have a
            // have 'initial' value and cannot 'inherit' from parent.
            // See http://dev.w3.org/csswg/css3-fonts/#font-family-desc
            clearProperties();
            popRuleData();
            return 0;
        }
    }
    RefPtr<StyleRuleFontFace> rule = StyleRuleFontFace::create();
    rule->setProperties(createStylePropertySet());
    clearProperties();
    StyleRuleFontFace* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return result;
}

#if ENABLE(SHADOW_DOM)
StyleRuleBase* CSSParser::createHostRule(RuleList* rules)
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    RefPtr<StyleRuleHost> rule;
    if (rules)
        rule = StyleRuleHost::create(*rules);
    else {
        RuleList emptyRules;
        rule = StyleRuleHost::create(emptyRules);
    }
    StyleRuleHost* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();
    return result;
}
#endif

void CSSParser::addNamespace(const AtomicString& prefix, const AtomicString& uri)
{
    if (!m_styleSheet || !m_allowNamespaceDeclarations)
        return;
    m_allowImportRules = false;
    m_styleSheet->parserAddNamespace(prefix, uri);
    if (prefix.isEmpty() && !uri.isNull())
        m_defaultNamespace = uri;
}

QualifiedName CSSParser::determineNameInNamespace(const AtomicString& prefix, const AtomicString& localName)
{
    if (!m_styleSheet)
        return QualifiedName(prefix, localName, m_defaultNamespace);
    return QualifiedName(prefix, localName, m_styleSheet->determineNamespace(prefix));
}

CSSParserSelector* CSSParser::rewriteSpecifiersWithNamespaceIfNeeded(CSSParserSelector* specifiers)
{
    if (m_defaultNamespace != starAtom || specifiers->isCustomPseudoElement())
        return rewriteSpecifiersWithElementName(nullAtom, starAtom, specifiers, /*tagIsForNamespaceRule*/true);
    return specifiers;
}

CSSParserSelector* CSSParser::rewriteSpecifiersWithElementName(const AtomicString& namespacePrefix, const AtomicString& elementName, CSSParserSelector* specifiers, bool tagIsForNamespaceRule)
{
    AtomicString determinedNamespace = namespacePrefix != nullAtom && m_styleSheet ? m_styleSheet->determineNamespace(namespacePrefix) : m_defaultNamespace;
    QualifiedName tag(namespacePrefix, elementName, determinedNamespace);

    if (!specifiers->isCustomPseudoElement()) {
        if (tag == anyQName())
            return specifiers;
#if ENABLE(VIDEO_TRACK)
        if (!(specifiers->pseudoType() == CSSSelector::PseudoCue))
#endif
            specifiers->prependTagSelector(tag, tagIsForNamespaceRule);
        return specifiers;
    }

    CSSParserSelector* lastShadowDescendant = specifiers;
    CSSParserSelector* history = specifiers;
    while (history->tagHistory()) {
        history = history->tagHistory();
        if (history->isCustomPseudoElement() || history->hasShadowDescendant())
            lastShadowDescendant = history;
    }

    if (lastShadowDescendant->tagHistory()) {
        if (tag != anyQName())
            lastShadowDescendant->tagHistory()->prependTagSelector(tag, tagIsForNamespaceRule);
        return specifiers;
    }

    // For shadow-ID pseudo-elements to be correctly matched, the ShadowDescendant combinator has to be used.
    // We therefore create a new Selector with that combinator here in any case, even if matching any (host) element in any namespace (i.e. '*').
    OwnPtr<CSSParserSelector> elementNameSelector = adoptPtr(new CSSParserSelector(tag));
    lastShadowDescendant->setTagHistory(elementNameSelector.release());
    lastShadowDescendant->setRelation(CSSSelector::ShadowDescendant);
    return specifiers;
}

CSSParserSelector* CSSParser::rewriteSpecifiers(CSSParserSelector* specifiers, CSSParserSelector* newSpecifier)
{
#if ENABLE(VIDEO_TRACK)
    if (newSpecifier->isCustomPseudoElement() || newSpecifier->pseudoType() == CSSSelector::PseudoCue) {
#else
    if (newSpecifier->isCustomPseudoElement()) {
#endif
        // Unknown pseudo element always goes at the top of selector chain.
        newSpecifier->appendTagHistory(CSSSelector::ShadowDescendant, sinkFloatingSelector(specifiers));
        return newSpecifier;
    }
    if (specifiers->isCustomPseudoElement()) {
        // Specifiers for unknown pseudo element go right behind it in the chain.
        specifiers->insertTagHistory(CSSSelector::SubSelector, sinkFloatingSelector(newSpecifier), CSSSelector::ShadowDescendant);
        return specifiers;
    }
    specifiers->appendTagHistory(CSSSelector::SubSelector, sinkFloatingSelector(newSpecifier));
    return specifiers;
}

StyleRuleBase* CSSParser::createPageRule(PassOwnPtr<CSSParserSelector> pageSelector)
{
    // FIXME: Margin at-rules are ignored.
    m_allowImportRules = m_allowNamespaceDeclarations = false;
    StyleRulePage* pageRule = 0;
    if (pageSelector) {
        RefPtr<StyleRulePage> rule = StyleRulePage::create();
        Vector<OwnPtr<CSSParserSelector> > selectorVector;
        selectorVector.append(pageSelector);
        rule->parserAdoptSelectorVector(selectorVector);
        rule->setProperties(createStylePropertySet());
        pageRule = rule.get();
        m_parsedRules.append(rule.release());
        processAndAddNewRuleToSourceTreeIfNeeded();
    } else
        popRuleData();
    clearProperties();
    return pageRule;
}

void CSSParser::setReusableRegionSelectorVector(Vector<OwnPtr<CSSParserSelector> >* selectors)
{
    if (selectors)
        m_reusableRegionSelectorVector.swap(*selectors);
}

StyleRuleBase* CSSParser::createRegionRule(Vector<OwnPtr<CSSParserSelector> >* regionSelector, RuleList* rules)
{
    if (!cssRegionsEnabled() || !regionSelector || !rules) {
        popRuleData();
        return 0;
    }

    m_allowImportRules = m_allowNamespaceDeclarations = false;

    RefPtr<StyleRuleRegion> regionRule = StyleRuleRegion::create(regionSelector, *rules);

    StyleRuleRegion* result = regionRule.get();
    m_parsedRules.append(regionRule.release());
    if (isExtractingSourceData())
        addNewRuleToSourceTree(CSSRuleSourceData::createUnknown());

    return result;
}

StyleRuleBase* CSSParser::createMarginAtRule(CSSSelector::MarginBoxType /* marginBox */)
{
    // FIXME: Implement margin at-rule here, using:
    //        - marginBox: margin box
    //        - m_parsedProperties: properties at [m_numParsedPropertiesBeforeMarginBox, m_parsedProperties.size()] are for this at-rule.
    // Don't forget to also update the action for page symbol in CSSGrammar.y such that margin at-rule data is cleared if page_selector is invalid.

    endDeclarationsForMarginBox();
    return 0; // until this method is implemented.
}

void CSSParser::startDeclarationsForMarginBox()
{
    m_numParsedPropertiesBeforeMarginBox = m_parsedProperties.size();
}

void CSSParser::endDeclarationsForMarginBox()
{
    rollbackLastProperties(m_parsedProperties.size() - m_numParsedPropertiesBeforeMarginBox);
    m_numParsedPropertiesBeforeMarginBox = INVALID_NUM_PARSED_PROPERTIES;
}

void CSSParser::deleteFontFaceOnlyValues()
{
    ASSERT(m_hasFontFaceOnlyValues);
    for (unsigned i = 0; i < m_parsedProperties.size();) {
        CSSProperty& property = m_parsedProperties[i];
        if (property.id() == CSSPropertyFontVariant && property.value()->isValueList()) {
            m_parsedProperties.remove(i);
            continue;
        }
        ++i;
    }
}

StyleKeyframe* CSSParser::createKeyframe(CSSParserValueList* keys)
{
    // Create a key string from the passed keys
    StringBuilder keyString;
    for (unsigned i = 0; i < keys->size(); ++i) {
        // Just as per the comment below, we ignore keyframes with
        // invalid key values (plain numbers or unknown identifiers)
        // marked as CSSPrimitiveValue::CSS_UNKNOWN during parsing.
        if (keys->valueAt(i)->unit == CSSPrimitiveValue::CSS_UNKNOWN) {
            clearProperties();
            return 0;
        }

        ASSERT(keys->valueAt(i)->unit == CSSPrimitiveValue::CSS_NUMBER);
        float key = static_cast<float>(keys->valueAt(i)->fValue);
        if (key < 0 || key > 100) {
            // As per http://www.w3.org/TR/css3-animations/#keyframes,
            // "If a keyframe selector specifies negative percentage values
            // or values higher than 100%, then the keyframe will be ignored."
            clearProperties();
            return 0;
        }
        if (i != 0)
            keyString.append(',');
        keyString.append(String::number(key));
        keyString.append('%');
    }

    RefPtr<StyleKeyframe> keyframe = StyleKeyframe::create();
    keyframe->setKeyText(keyString.toString());
    keyframe->setProperties(createStylePropertySet());

    clearProperties();

    StyleKeyframe* keyframePtr = keyframe.get();
    m_parsedKeyframes.append(keyframe.release());
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
}

void CSSParser::updateLastMediaLine(MediaQuerySet* media)
{
    media->setLastLine(m_lineNumber);
}

template <typename CharacterType>
static inline void fixUnparsedProperties(const CharacterType* characters, CSSRuleSourceData* ruleData)
{
    Vector<CSSPropertySourceData>& propertyData = ruleData->styleSourceData->propertyData;
    unsigned size = propertyData.size();
    if (!size)
        return;

    unsigned styleStart = ruleData->ruleBodyRange.start;
    CSSPropertySourceData* nextData = &(propertyData.at(0));
    for (unsigned i = 0; i < size; ++i) {
        CSSPropertySourceData* currentData = nextData;
        nextData = i < size - 1 ? &(propertyData.at(i + 1)) : 0;

        if (currentData->parsedOk)
            continue;
        if (currentData->range.end > 0 && characters[styleStart + currentData->range.end - 1] == ';')
            continue;

        unsigned propertyEndInStyleSheet;
        if (!nextData)
            propertyEndInStyleSheet = ruleData->ruleBodyRange.end - 1;
        else
            propertyEndInStyleSheet = styleStart + nextData->range.start - 1;

        while (isHTMLSpace(characters[propertyEndInStyleSheet]))
            --propertyEndInStyleSheet;

        // propertyEndInStyleSheet points at the last property text character.
        unsigned newPropertyEnd = propertyEndInStyleSheet - styleStart + 1; // Exclusive of the last property text character.
        if (currentData->range.end != newPropertyEnd) {
            currentData->range.end = newPropertyEnd;
            unsigned valueStartInStyleSheet = styleStart + currentData->range.start + currentData->name.length();
            while (valueStartInStyleSheet < propertyEndInStyleSheet && characters[valueStartInStyleSheet] != ':')
                ++valueStartInStyleSheet;
            if (valueStartInStyleSheet < propertyEndInStyleSheet)
                ++valueStartInStyleSheet; // Shift past the ':'.
            while (valueStartInStyleSheet < propertyEndInStyleSheet && isHTMLSpace(characters[valueStartInStyleSheet]))
                ++valueStartInStyleSheet;
            // Need to exclude the trailing ';' from the property value.
            currentData->value = String(characters + valueStartInStyleSheet, propertyEndInStyleSheet - valueStartInStyleSheet + (characters[propertyEndInStyleSheet] == ';' ? 0 : 1));
        }
    }
}

void CSSParser::fixUnparsedPropertyRanges(CSSRuleSourceData* ruleData)
{
    if (!ruleData->styleSourceData)
        return;

    if (is8BitSource()) {
        fixUnparsedProperties<LChar>(m_dataStart8.get() + m_parsedTextPrefixLength, ruleData);
        return;
    }

    fixUnparsedProperties<UChar>(m_dataStart16.get() + m_parsedTextPrefixLength, ruleData);
}

void CSSParser::markRuleHeaderStart(CSSRuleSourceData::Type ruleType)
{
    if (!isExtractingSourceData())
        return;

    // Pop off data for a previous invalid rule.
    if (m_currentRuleData)
        m_currentRuleDataStack->removeLast();

    RefPtr<CSSRuleSourceData> data = CSSRuleSourceData::create(ruleType);
    data->ruleHeaderRange.start = tokenStartOffset();
    m_currentRuleData = data;
    m_currentRuleDataStack->append(data.release());
}

template <typename CharacterType>
inline void CSSParser::setRuleHeaderEnd(const CharacterType* dataStart)
{
    CharacterType* listEnd = tokenStart<CharacterType>();
    while (listEnd > dataStart + 1) {
        if (isHTMLSpace(*(listEnd - 1)))
            --listEnd;
        else
            break;
    }

    m_currentRuleDataStack->last()->ruleHeaderRange.end = listEnd - dataStart;
}

void CSSParser::markRuleHeaderEnd()
{
    if (!isExtractingSourceData())
        return;
    ASSERT(!m_currentRuleDataStack->isEmpty());

    if (is8BitSource())
        setRuleHeaderEnd<LChar>(m_dataStart8.get());
    else
        setRuleHeaderEnd<UChar>(m_dataStart16.get());
}

void CSSParser::markSelectorStart()
{
    if (!isExtractingSourceData())
        return;
    ASSERT(!m_selectorRange.end);

    m_selectorRange.start = tokenStartOffset();
}

void CSSParser::markSelectorEnd()
{
    if (!isExtractingSourceData())
        return;
    ASSERT(!m_selectorRange.end);
    ASSERT(m_currentRuleDataStack->size());

    m_selectorRange.end = tokenStartOffset();
    m_currentRuleDataStack->last()->selectorRanges.append(m_selectorRange);
    m_selectorRange.start = 0;
    m_selectorRange.end = 0;
}

void CSSParser::markRuleBodyStart()
{
    if (!isExtractingSourceData())
        return;
    m_currentRuleData.clear();
    unsigned offset = tokenStartOffset();
    if (tokenStartChar() == '{')
        ++offset; // Skip the rule body opening brace.
    ASSERT(!m_currentRuleDataStack->isEmpty());
    m_currentRuleDataStack->last()->ruleBodyRange.start = offset;
}

void CSSParser::markRuleBodyEnd()
{
    // Precondition: (!isExtractingSourceData())
    unsigned offset = tokenStartOffset();
    ASSERT(!m_currentRuleDataStack->isEmpty());
    m_currentRuleDataStack->last()->ruleBodyRange.end = offset;
}

void CSSParser::markPropertyStart()
{
    m_ignoreErrorsInDeclaration = false;
    if (!isExtractingSourceData())
        return;
    if (m_currentRuleDataStack->isEmpty() || !m_currentRuleDataStack->last()->styleSourceData)
        return;

    m_propertyRange.start = tokenStartOffset();
}

void CSSParser::markPropertyEnd(bool isImportantFound, bool isPropertyParsed)
{
    if (!isExtractingSourceData())
        return;
    if (m_currentRuleDataStack->isEmpty() || !m_currentRuleDataStack->last()->styleSourceData)
        return;

    unsigned offset = tokenStartOffset();
    if (tokenStartChar() == ';') // Include semicolon into the property text.
        ++offset;
    m_propertyRange.end = offset;
    if (m_propertyRange.start != UINT_MAX && !m_currentRuleDataStack->isEmpty()) {
        // This stuff is only executed when the style data retrieval is requested by client.
        const unsigned start = m_propertyRange.start;
        const unsigned end = m_propertyRange.end;
        ASSERT(start < end);
        String propertyString;
        if (is8BitSource())
            propertyString = String(m_dataStart8.get() + start, end - start).stripWhiteSpace();
        else
            propertyString = String(m_dataStart16.get() + start, end - start).stripWhiteSpace();
        if (propertyString.endsWith(';'))
            propertyString = propertyString.left(propertyString.length() - 1);
        size_t colonIndex = propertyString.find(':');
        ASSERT(colonIndex != notFound);

        String name = propertyString.left(colonIndex).stripWhiteSpace();
        String value = propertyString.substring(colonIndex + 1, propertyString.length()).stripWhiteSpace();
        // The property range is relative to the declaration start offset.
        SourceRange& topRuleBodyRange = m_currentRuleDataStack->last()->ruleBodyRange;
        m_currentRuleDataStack->last()->styleSourceData->propertyData.append(
            CSSPropertySourceData(name, value, isImportantFound, isPropertyParsed, SourceRange(start - topRuleBodyRange.start, end - topRuleBodyRange.start)));
    }
    resetPropertyRange();
}

#if ENABLE(CSS_DEVICE_ADAPTATION)
StyleRuleBase* CSSParser::createViewportRule()
{
    m_allowImportRules = m_allowNamespaceDeclarations = false;

    RefPtr<StyleRuleViewport> rule = StyleRuleViewport::create();

    rule->setProperties(createStylePropertySet());
    clearProperties();

    StyleRuleViewport* result = rule.get();
    m_parsedRules.append(rule.release());
    processAndAddNewRuleToSourceTreeIfNeeded();

    return result;
}

bool CSSParser::parseViewportProperty(CSSPropertyID propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    CSSValueID id = value->id;
    bool validPrimitive = false;

    switch (propId) {
    case CSSPropertyMinWidth: // auto | device-width | device-height | <length> | <percentage>
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMaxHeight:
        if (id == CSSValueAuto || id == CSSValueDeviceWidth || id == CSSValueDeviceHeight)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FLength | FPercent | FNonNeg));
        break;
    case CSSPropertyWidth: // shorthand
        return parseViewportShorthand(propId, CSSPropertyMinWidth, CSSPropertyMaxWidth, important);
    case CSSPropertyHeight:
        return parseViewportShorthand(propId, CSSPropertyMinHeight, CSSPropertyMaxHeight, important);
    case CSSPropertyMinZoom: // auto | <number> | <percentage>
    case CSSPropertyMaxZoom:
    case CSSPropertyZoom:
        if (id == CSSValueAuto)
            validPrimitive = true;
        else
            validPrimitive = (!id && validUnit(value, FNumber | FPercent | FNonNeg));
        break;
    case CSSPropertyUserZoom: // zoom | fixed
        if (id == CSSValueZoom || id == CSSValueFixed)
            validPrimitive = true;
        break;
    case CSSPropertyOrientation: // auto | portrait | landscape
        if (id == CSSValueAuto || id == CSSValuePortrait || id == CSSValueLandscape)
            validPrimitive = true;
    default:
        break;
    }

    RefPtr<CSSValue> parsedValue;
    if (validPrimitive) {
        parsedValue = parseValidPrimitive(id, value);
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

bool CSSParser::parseViewportShorthand(CSSPropertyID propId, CSSPropertyID first, CSSPropertyID second, bool important)
{
    unsigned numValues = m_valueList->size();

    if (numValues > 2)
        return false;

    ShorthandScope scope(this, propId);

    if (!parseViewportProperty(first, important))
        return false;

    // If just one value is supplied, the second value
    // is implicitly initialized with the first value.
    if (numValues == 1)
        m_valueList->previous();

    return parseViewportProperty(second, important);
}
#endif

template <typename CharacterType>
static CSSPropertyID cssPropertyID(const CharacterType* propertyName, unsigned length)
{
    char buffer[maxCSSPropertyNameLength + 1 + 1]; // 1 to turn "apple"/"khtml" into "webkit", 1 for null character

    for (unsigned i = 0; i != length; ++i) {
        CharacterType c = propertyName[i];
        if (c == 0 || c >= 0x7F)
            return CSSPropertyInvalid; // illegal character
        buffer[i] = toASCIILower(c);
    }
    buffer[length] = '\0';

    const char* name = buffer;
    if (buffer[0] == '-') {
#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
        // If the prefix is -apple- or -khtml-, change it to -webkit-.
        // This makes the string one character longer.
        if (RuntimeEnabledFeatures::legacyCSSVendorPrefixesEnabled()
            && (hasPrefix(buffer, length, "-apple-") || hasPrefix(buffer, length, "-khtml-"))) {
            memmove(buffer + 7, buffer + 6, length + 1 - 6);
            memcpy(buffer, "-webkit", 7);
            ++length;
        }
#endif
#if PLATFORM(IOS)
        cssPropertyNameIOSAliasing(buffer, name, length);
#endif
    }

    const Property* hashTableEntry = findProperty(name, length);
    const CSSPropertyID propertyID = hashTableEntry ? static_cast<CSSPropertyID>(hashTableEntry->id) : CSSPropertyInvalid;

    static const int cssPropertyHistogramSize = numCSSProperties;
    if (hasPrefix(buffer, length, "-webkit-") && propertyID != CSSPropertyInvalid) {
        int histogramValue = propertyID - firstCSSProperty;
        ASSERT(0 <= histogramValue && histogramValue < cssPropertyHistogramSize);
        HistogramSupport::histogramEnumeration("CSS.PrefixUsage", histogramValue, cssPropertyHistogramSize);
    }

    return propertyID;
}

CSSPropertyID cssPropertyID(const String& string)
{
    unsigned length = string.length();

    if (!length)
        return CSSPropertyInvalid;
    if (length > maxCSSPropertyNameLength)
        return CSSPropertyInvalid;
    
    return string.is8Bit() ? cssPropertyID(string.characters8(), length) : cssPropertyID(string.characters16(), length);
}

CSSPropertyID cssPropertyID(const CSSParserString& string)
{
    unsigned length = string.length();

    if (!length)
        return CSSPropertyInvalid;
    if (length > maxCSSPropertyNameLength)
        return CSSPropertyInvalid;
    
    return string.is8Bit() ? cssPropertyID(string.characters8(), length) : cssPropertyID(string.characters16(), length);
}

#if PLATFORM(IOS)
void cssPropertyNameIOSAliasing(const char* propertyName, const char*& propertyNameAlias, unsigned& newLength)
{
    if (!strcmp(propertyName, "-webkit-hyphenate-locale")) {
        // Worked in iOS 4.2.
        static const char* const webkitLocale = "-webkit-locale";
        propertyNameAlias = webkitLocale;
        newLength = strlen(webkitLocale);
    }
}
#endif

template <typename CharacterType>
static CSSValueID cssValueKeywordID(const CharacterType* valueKeyword, unsigned length)
{
    char buffer[maxCSSValueKeywordLength + 1 + 1]; // 1 to turn "apple"/"khtml" into "webkit", 1 for null character

    for (unsigned i = 0; i != length; ++i) {
        CharacterType c = valueKeyword[i];
        if (c == 0 || c >= 0x7F)
            return CSSValueInvalid; // illegal keyword.
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
    return hashTableEntry ? static_cast<CSSValueID>(hashTableEntry->id) : CSSValueInvalid;
}

CSSValueID cssValueKeywordID(const CSSParserString& string)
{
    unsigned length = string.length();
    if (!length)
        return CSSValueInvalid;
    if (length > maxCSSValueKeywordLength)
        return CSSValueInvalid;

    return string.is8Bit() ? cssValueKeywordID(string.characters8(), length) : cssValueKeywordID(string.characters16(), length);
}

template <typename CharacterType>
static inline bool isCSSTokenizerIdentifier(const CharacterType* characters, unsigned length)
{
    const CharacterType* end = characters + length;

    // -?
    if (characters != end && characters[0] == '-')
        ++characters;

    // {nmstart}
    if (characters == end || !(characters[0] == '_' || characters[0] >= 128 || isASCIIAlpha(characters[0])))
        return false;
    ++characters;

    // {nmchar}*
    for (; characters != end; ++characters) {
        if (!(characters[0] == '_' || characters[0] == '-' || characters[0] >= 128 || isASCIIAlphanumeric(characters[0])))
            return false;
    }

    return true;
}

// "ident" from the CSS tokenizer, minus backslash-escape sequences
static bool isCSSTokenizerIdentifier(const String& string)
{
    unsigned length = string.length();

    if (!length)
        return false;

    if (string.is8Bit())
        return isCSSTokenizerIdentifier(string.characters8(), length);
    return isCSSTokenizerIdentifier(string.characters16(), length);
}

template <typename CharacterType>
static inline bool isCSSTokenizerURL(const CharacterType* characters, unsigned length)
{
    const CharacterType* end = characters + length;
    
    for (; characters != end; ++characters) {
        CharacterType c = characters[0];
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

// "url" from the CSS tokenizer, minus backslash-escape sequences
static bool isCSSTokenizerURL(const String& string)
{
    unsigned length = string.length();

    if (!length)
        return true;

    if (string.is8Bit())
        return isCSSTokenizerURL(string.characters8(), length);
    return isCSSTokenizerURL(string.characters16(), length);
}


template <typename CharacterType>
static inline String quoteCSSStringInternal(const CharacterType* characters, unsigned length)
{
    // For efficiency, we first pre-calculate the length of the quoted string, then we build the actual one.
    // Please see below for the actual logic.
    unsigned quotedStringSize = 2; // Two quotes surrounding the entire string.
    bool afterEscape = false;
    for (unsigned i = 0; i < length; ++i) {
        CharacterType ch = characters[i];
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

    StringBuffer<CharacterType> buffer(quotedStringSize);
    unsigned index = 0;
    buffer[index++] = '\'';
    afterEscape = false;
    for (unsigned i = 0; i < length; ++i) {
        CharacterType ch = characters[i];
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

// We use single quotes for now because markup.cpp uses double quotes.
String quoteCSSString(const String& string)
{
    // This function expands each character to at most 3 characters ('\u0010' -> '\' '1' '0') as well as adds
    // 2 quote characters (before and after). Make sure the resulting size (3 * length + 2) will not overflow unsigned.

    unsigned length = string.length();

    if (!length)
        return String("\'\'");

    if (length > std::numeric_limits<unsigned>::max() / 3 - 2)
        return emptyString();

    if (string.is8Bit())
        return quoteCSSStringInternal(string.characters8(), length);
    return quoteCSSStringInternal(string.characters16(), length);
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
    // However, since the {ident} rule precedes the {nth} rule, some of those
    // tokens are identified as string literal. Furthermore we need to accept
    // "odd" and "even" which does not match to an+b.
    return equalIgnoringCase(token, "odd") || equalIgnoringCase(token, "even")
        || equalIgnoringCase(token, "n") || equalIgnoringCase(token, "-n");
}

}
