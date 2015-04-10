/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "CSSDefaultStyleSheets.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "HTMLAnchorElement.h"
#include "HTMLAudioElement.h"
#include "MediaQueryEvaluator.h"
#include "Page.h"
#include "RenderTheme.h"
#include "RuleSet.h"
#include "StyleSheetContents.h"
#include "UserAgentStyleSheets.h"

namespace WebCore {

using namespace HTMLNames;

RuleSet* CSSDefaultStyleSheets::defaultStyle;
RuleSet* CSSDefaultStyleSheets::defaultQuirksStyle;
RuleSet* CSSDefaultStyleSheets::defaultPrintStyle;
RuleSet* CSSDefaultStyleSheets::defaultViewSourceStyle;

StyleSheetContents* CSSDefaultStyleSheets::simpleDefaultStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::defaultStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::quirksStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::svgStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::mathMLStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::mediaControlsStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::fullscreenStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::plugInsStyleSheet;

// FIXME: It would be nice to use some mechanism that guarantees this is in sync with the real UA stylesheet.
static const char* simpleUserAgentStyleSheet = "html,body,div{display:block}head{display:none}body{margin:8px}div:focus,span:focus{outline:auto 5px -webkit-focus-ring-color}a:-webkit-any-link{color:-webkit-link;text-decoration:underline}a:-webkit-any-link:active{color:-webkit-activelink}";

static inline bool elementCanUseSimpleDefaultStyle(Element* e)
{
    return e->hasTagName(htmlTag) || e->hasTagName(headTag) || e->hasTagName(bodyTag) || e->hasTagName(divTag) || e->hasTagName(spanTag) || e->hasTagName(brTag) || isHTMLAnchorElement(e);
}

static const MediaQueryEvaluator& screenEval()
{
    DEFINE_STATIC_LOCAL(const MediaQueryEvaluator, staticScreenEval, ("screen"));
    return staticScreenEval;
}

static const MediaQueryEvaluator& printEval()
{
    DEFINE_STATIC_LOCAL(const MediaQueryEvaluator, staticPrintEval, ("print"));
    return staticPrintEval;
}

static StyleSheetContents* parseUASheet(const String& str)
{
    StyleSheetContents* sheet = StyleSheetContents::create().leakRef(); // leak the sheet on purpose
    sheet->parseString(str);
    return sheet;
}

static StyleSheetContents* parseUASheet(const char* characters, unsigned size)
{
    return parseUASheet(String(characters, size));
}

void CSSDefaultStyleSheets::initDefaultStyle(Element* root)
{
    if (!defaultStyle) {
        if (!root || elementCanUseSimpleDefaultStyle(root))
            loadSimpleDefaultStyle();
        else
            loadFullDefaultStyle();
    }
}

void CSSDefaultStyleSheets::loadFullDefaultStyle()
{
    if (simpleDefaultStyleSheet) {
        ASSERT(defaultStyle);
        ASSERT(defaultPrintStyle == defaultStyle);
        delete defaultStyle;
        simpleDefaultStyleSheet->deref();
        defaultStyle = RuleSet::create().leakPtr();
        defaultPrintStyle = RuleSet::create().leakPtr();
        simpleDefaultStyleSheet = 0;
    } else {
        ASSERT(!defaultStyle);
        defaultStyle = RuleSet::create().leakPtr();
        defaultPrintStyle = RuleSet::create().leakPtr();
        defaultQuirksStyle = RuleSet::create().leakPtr();
    }

    // Strict-mode rules.
    String defaultRules = String(htmlUserAgentStyleSheet, sizeof(htmlUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraDefaultStyleSheet();
    defaultStyleSheet = parseUASheet(defaultRules);
    defaultStyle->addRulesFromSheet(defaultStyleSheet, screenEval());
    defaultPrintStyle->addRulesFromSheet(defaultStyleSheet, printEval());

    // Quirks-mode rules.
    String quirksRules = String(quirksUserAgentStyleSheet, sizeof(quirksUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraQuirksStyleSheet();
    quirksStyleSheet = parseUASheet(quirksRules);
    defaultQuirksStyle->addRulesFromSheet(quirksStyleSheet, screenEval());
}

void CSSDefaultStyleSheets::loadSimpleDefaultStyle()
{
    ASSERT(!defaultStyle);
    ASSERT(!simpleDefaultStyleSheet);

    defaultStyle = RuleSet::create().leakPtr();
    // There are no media-specific rules in the simple default style.
    defaultPrintStyle = defaultStyle;
    defaultQuirksStyle = RuleSet::create().leakPtr();

    simpleDefaultStyleSheet = parseUASheet(simpleUserAgentStyleSheet, strlen(simpleUserAgentStyleSheet));
    defaultStyle->addRulesFromSheet(simpleDefaultStyleSheet, screenEval());

    // No need to initialize quirks sheet yet as there are no quirk rules for elements allowed in simple default style.
}

RuleSet* CSSDefaultStyleSheets::viewSourceStyle()
{
    if (!defaultViewSourceStyle) {
        defaultViewSourceStyle = RuleSet::create().leakPtr();
        defaultViewSourceStyle->addRulesFromSheet(parseUASheet(sourceUserAgentStyleSheet, sizeof(sourceUserAgentStyleSheet)), screenEval());
    }
    return defaultViewSourceStyle;
}


void CSSDefaultStyleSheets::ensureDefaultStyleSheetsForElement(Element* element, bool& changedDefaultStyle)
{
    if (simpleDefaultStyleSheet && !elementCanUseSimpleDefaultStyle(element)) {
        loadFullDefaultStyle();
        changedDefaultStyle = true;
    }

#if ENABLE(SVG)
    if (element->isSVGElement() && !svgStyleSheet) {
        // SVG rules.
        svgStyleSheet = parseUASheet(svgUserAgentStyleSheet, sizeof(svgUserAgentStyleSheet));
        defaultStyle->addRulesFromSheet(svgStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(svgStyleSheet, printEval());
        changedDefaultStyle = true;
    }
#endif

#if ENABLE(MATHML)
    if (element->isMathMLElement() && !mathMLStyleSheet) {
        // MathML rules.
        mathMLStyleSheet = parseUASheet(mathmlUserAgentStyleSheet, sizeof(mathmlUserAgentStyleSheet));
        defaultStyle->addRulesFromSheet(mathMLStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(mathMLStyleSheet, printEval());
        changedDefaultStyle = true;
    }
#endif

#if ENABLE(VIDEO)
    if (!mediaControlsStyleSheet && (element->hasTagName(videoTag) || isHTMLAudioElement(element))) {
        String mediaRules = String(mediaControlsUserAgentStyleSheet, sizeof(mediaControlsUserAgentStyleSheet)) + RenderTheme::themeForPage(element->document()->page())->extraMediaControlsStyleSheet();
        mediaControlsStyleSheet = parseUASheet(mediaRules);
        defaultStyle->addRulesFromSheet(mediaControlsStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(mediaControlsStyleSheet, printEval());
        changedDefaultStyle = true;
    }
#endif

#if ENABLE(FULLSCREEN_API)
    if (!fullscreenStyleSheet && element->document()->webkitIsFullScreen()) {
        String fullscreenRules = String(fullscreenUserAgentStyleSheet, sizeof(fullscreenUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraFullScreenStyleSheet();
        fullscreenStyleSheet = parseUASheet(fullscreenRules);
        defaultStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
        defaultQuirksStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
        changedDefaultStyle = true;
    }
#endif

    if (!plugInsStyleSheet && (element->hasTagName(objectTag) || element->hasTagName(embedTag))) {
        String plugInsRules = RenderTheme::themeForPage(element->document()->page())->extraPlugInsStyleSheet() + element->document()->page()->chrome().client()->plugInExtraStyleSheet();
        if (plugInsRules.isEmpty())
            plugInsRules = String(plugInsUserAgentStyleSheet, sizeof(plugInsUserAgentStyleSheet));
        plugInsStyleSheet = parseUASheet(plugInsRules);
        defaultStyle->addRulesFromSheet(plugInsStyleSheet, screenEval());
        changedDefaultStyle = true;
    }

    ASSERT(defaultStyle->features().idsInRules.isEmpty());
    ASSERT(mathMLStyleSheet || defaultStyle->features().siblingRules.isEmpty());
}

} // namespace WebCore
