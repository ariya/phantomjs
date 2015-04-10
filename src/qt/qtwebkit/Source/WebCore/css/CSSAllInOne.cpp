/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.
 
#include "CSSAspectRatioValue.cpp"
#include "CSSBasicShapes.cpp"
#include "CSSBorderImage.cpp"
#include "CSSBorderImageSliceValue.cpp"
#include "CSSCanvasValue.cpp"
#include "CSSCharsetRule.cpp"
#include "CSSComputedStyleDeclaration.cpp"
#include "CSSCrossfadeValue.cpp"
#include "CSSCursorImageValue.cpp"
#include "CSSDefaultStyleSheets.cpp"
#include "CSSFontFace.cpp"
#include "CSSFontFaceRule.cpp"
#include "CSSFontFaceSource.cpp"
#include "CSSFontFaceSrcValue.cpp"
#include "CSSFontSelector.cpp"
#include "CSSFunctionValue.cpp"
#include "CSSGradientValue.cpp"
#include "CSSGroupingRule.cpp"
#include "CSSHostRule.cpp"
#include "CSSImageGeneratorValue.cpp"
#include "CSSImageValue.cpp"
#include "CSSImportRule.cpp"
#include "CSSInheritedValue.cpp"
#include "CSSInitialValue.cpp"
#include "CSSLineBoxContainValue.cpp"
#include "CSSMediaRule.cpp"
#include "CSSOMUtils.cpp"
#include "CSSPageRule.cpp"
#include "CSSParser.cpp"
#include "CSSParserValues.cpp"
#include "CSSPropertySourceData.cpp"
#include "CSSReflectValue.cpp"
#include "CSSRule.cpp"
#include "CSSRuleList.cpp"
#include "CSSSegmentedFontFace.cpp"
#include "CSSSelector.cpp"
#include "CSSSelectorList.cpp"
#include "CSSStyleRule.cpp"
#include "CSSStyleSheet.cpp"
#include "CSSTimingFunctionValue.cpp"
#include "CSSUnicodeRangeValue.cpp"
#include "CSSValue.cpp"
#include "CSSValueList.cpp"
#include "CSSValuePool.cpp"
#include "DOMWindowCSS.cpp"
#include "DeprecatedStyleBuilder.cpp"
#include "DocumentRuleSets.cpp"
#include "ElementRuleCollector.cpp"
#include "InspectorCSSOMWrappers.cpp"
#include "PageRuleCollector.cpp"
#include "RuleFeature.cpp"
#include "RuleSet.cpp"
#include "SelectorCheckerFastPath.cpp"
#include "SelectorFilter.cpp"
#include "StylePropertySet.cpp"
#include "StylePropertyShorthand.cpp"
#include "StyleResolver.cpp"
#include "StyleScopeResolver.cpp"
#include "ViewportStyleResolver.cpp"
