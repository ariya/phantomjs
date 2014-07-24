/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "CSSSelector.h"

#include "CSSOMUtils.h"
#include "CSSSelectorList.h"
#include "HTMLNames.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

void CSSSelector::createRareData()
{
    ASSERT(m_match != Tag);
    if (m_hasRareData)
        return;
    // Move the value to the rare data stucture.
    m_data.m_rareData = RareData::create(adoptRef(m_data.m_value)).leakRef();
    m_hasRareData = true;
}

unsigned CSSSelector::specificity() const
{
    // make sure the result doesn't overflow
    static const unsigned maxValueMask = 0xffffff;
    static const unsigned idMask = 0xff0000;
    static const unsigned classMask = 0xff00;
    static const unsigned elementMask = 0xff;

    if (isForPage())
        return specificityForPage() & maxValueMask;

    unsigned total = 0;
    unsigned temp = 0;

    for (const CSSSelector* selector = this; selector; selector = selector->tagHistory()) {
        temp = total + selector->specificityForOneSelector();
        // Clamp each component to its max in the case of overflow.
        if ((temp & idMask) < (total & idMask))
            total |= idMask;
        else if ((temp & classMask) < (total & classMask))
            total |= classMask;
        else if ((temp & elementMask) < (total & elementMask))
            total |= elementMask;
        else
            total = temp;
    }
    return total;
}

inline unsigned CSSSelector::specificityForOneSelector() const
{
    // FIXME: Pseudo-elements and pseudo-classes do not have the same specificity. This function
    // isn't quite correct.
    switch (m_match) {
    case Id:
        return 0x10000;
    case Exact:
    case Class:
    case Set:
    case List:
    case Hyphen:
    case PseudoClass:
    case PseudoElement:
    case Contain:
    case Begin:
    case End:
        // FIXME: PsuedoAny should base the specificity on the sub-selectors.
        // See http://lists.w3.org/Archives/Public/www-style/2010Sep/0530.html
        if (pseudoType() == PseudoNot && selectorList())
            return selectorList()->first()->specificityForOneSelector();
        return 0x100;
    case Tag:
        return (tagQName().localName() != starAtom) ? 1 : 0;
    case Unknown:
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

unsigned CSSSelector::specificityForPage() const
{
    // See http://dev.w3.org/csswg/css3-page/#cascading-and-page-context
    unsigned s = 0;

    for (const CSSSelector* component = this; component; component = component->tagHistory()) {
        switch (component->m_match) {
        case Tag:
            s += tagQName().localName() == starAtom ? 0 : 4;
            break;
        case PseudoClass:
            switch (component->pseudoType()) {
            case PseudoFirstPage:
                s += 2;
                break;
            case PseudoLeftPage:
            case PseudoRightPage:
                s += 1;
                break;
            case PseudoNotParsed:
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        default:
            break;
        }
    }
    return s;
}

PseudoId CSSSelector::pseudoId(PseudoType type)
{
    switch (type) {
    case PseudoFirstLine:
        return FIRST_LINE;
    case PseudoFirstLetter:
        return FIRST_LETTER;
    case PseudoSelection:
        return SELECTION;
    case PseudoBefore:
        return BEFORE;
    case PseudoAfter:
        return AFTER;
    case PseudoScrollbar:
        return SCROLLBAR;
    case PseudoScrollbarButton:
        return SCROLLBAR_BUTTON;
    case PseudoScrollbarCorner:
        return SCROLLBAR_CORNER;
    case PseudoScrollbarThumb:
        return SCROLLBAR_THUMB;
    case PseudoScrollbarTrack:
        return SCROLLBAR_TRACK;
    case PseudoScrollbarTrackPiece:
        return SCROLLBAR_TRACK_PIECE;
    case PseudoResizer:
        return RESIZER;
#if ENABLE(FULLSCREEN_API)
    case PseudoFullScreen:
        return FULL_SCREEN;
    case PseudoFullScreenDocument:
        return FULL_SCREEN_DOCUMENT;
    case PseudoFullScreenAncestor:
        return FULL_SCREEN_ANCESTOR;
    case PseudoAnimatingFullScreenTransition:
        return ANIMATING_FULL_SCREEN_TRANSITION;
#endif
    case PseudoUnknown:
    case PseudoEmpty:
    case PseudoFirstChild:
    case PseudoFirstOfType:
    case PseudoLastChild:
    case PseudoLastOfType:
    case PseudoOnlyChild:
    case PseudoOnlyOfType:
    case PseudoNthChild:
    case PseudoNthOfType:
    case PseudoNthLastChild:
    case PseudoNthLastOfType:
    case PseudoLink:
    case PseudoVisited:
    case PseudoAny:
    case PseudoAnyLink:
    case PseudoAutofill:
    case PseudoHover:
    case PseudoDrag:
    case PseudoFocus:
    case PseudoActive:
    case PseudoChecked:
    case PseudoEnabled:
    case PseudoFullPageMedia:
    case PseudoDefault:
    case PseudoDisabled:
    case PseudoOptional:
    case PseudoRequired:
    case PseudoReadOnly:
    case PseudoReadWrite:
    case PseudoValid:
    case PseudoInvalid:
    case PseudoIndeterminate:
    case PseudoTarget:
    case PseudoLang:
    case PseudoNot:
    case PseudoRoot:
    case PseudoScope:
    case PseudoScrollbarBack:
    case PseudoScrollbarForward:
    case PseudoWindowInactive:
    case PseudoCornerPresent:
    case PseudoDecrement:
    case PseudoIncrement:
    case PseudoHorizontal:
    case PseudoVertical:
    case PseudoStart:
    case PseudoEnd:
    case PseudoDoubleButton:
    case PseudoSingleButton:
    case PseudoNoButton:
    case PseudoFirstPage:
    case PseudoLeftPage:
    case PseudoRightPage:
    case PseudoInRange:
    case PseudoOutOfRange:
    case PseudoUserAgentCustomElement:
    case PseudoWebKitCustomElement:
#if ENABLE(VIDEO_TRACK)
    case PseudoCue:
    case PseudoFutureCue:
    case PseudoPastCue:
#endif
#if ENABLE(IFRAME_SEAMLESS)
    case PseudoSeamlessDocument:
#endif
        return NOPSEUDO;
    case PseudoNotParsed:
        ASSERT_NOT_REACHED();
        return NOPSEUDO;
    }

    ASSERT_NOT_REACHED();
    return NOPSEUDO;
}

static HashMap<AtomicStringImpl*, CSSSelector::PseudoType>* nameToPseudoTypeMap()
{
    DEFINE_STATIC_LOCAL(AtomicString, active, ("active", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, after, ("after", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, any, ("-webkit-any(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, anyLink, ("-webkit-any-link", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, autofill, ("-webkit-autofill", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, before, ("before", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, checked, ("checked", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, defaultString, ("default", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, disabled, ("disabled", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, readOnly, ("read-only", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, readWrite, ("read-write", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, valid, ("valid", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, invalid, ("invalid", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, drag, ("-webkit-drag", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, dragAlias, ("-khtml-drag", AtomicString::ConstructFromLiteral)); // was documented with this name in Apple documentation, so keep an alia
    DEFINE_STATIC_LOCAL(AtomicString, empty, ("empty", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, enabled, ("enabled", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, firstChild, ("first-child", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, firstLetter, ("first-letter", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, firstLine, ("first-line", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, firstOfType, ("first-of-type", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, fullPageMedia, ("-webkit-full-page-media", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, nthChild, ("nth-child(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, nthOfType, ("nth-of-type(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, nthLastChild, ("nth-last-child(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, nthLastOfType, ("nth-last-of-type(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, focus, ("focus", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, hover, ("hover", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, indeterminate, ("indeterminate", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, lastChild, ("last-child", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, lastOfType, ("last-of-type", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, link, ("link", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, lang, ("lang(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, notStr, ("not(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, onlyChild, ("only-child", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, onlyOfType, ("only-of-type", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, optional, ("optional", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, required, ("required", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, resizer, ("-webkit-resizer", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, root, ("root", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbar, ("-webkit-scrollbar", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarButton, ("-webkit-scrollbar-button", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarCorner, ("-webkit-scrollbar-corner", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarThumb, ("-webkit-scrollbar-thumb", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarTrack, ("-webkit-scrollbar-track", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarTrackPiece, ("-webkit-scrollbar-track-piece", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, selection, ("selection", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, target, ("target", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, visited, ("visited", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, windowInactive, ("window-inactive", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, decrement, ("decrement", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, increment, ("increment", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, start, ("start", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, end, ("end", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, horizontal, ("horizontal", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, vertical, ("vertical", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, doubleButton, ("double-button", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, singleButton, ("single-button", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, noButton, ("no-button", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, cornerPresent, ("corner-present", AtomicString::ConstructFromLiteral));
    // Paged Media pseudo-classes
    DEFINE_STATIC_LOCAL(AtomicString, firstPage, ("first", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, leftPage, ("left", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, rightPage, ("right", AtomicString::ConstructFromLiteral));
#if ENABLE(FULLSCREEN_API)
    DEFINE_STATIC_LOCAL(AtomicString, fullScreen, ("-webkit-full-screen", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, fullScreenDocument, ("-webkit-full-screen-document", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, fullScreenAncestor, ("-webkit-full-screen-ancestor", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, animatingFullScreenTransition, ("-webkit-animating-full-screen-transition", AtomicString::ConstructFromLiteral));
#endif
#if ENABLE(VIDEO_TRACK)
    DEFINE_STATIC_LOCAL(AtomicString, cue, ("cue(", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, futureCue, ("future", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, pastCue, ("past", AtomicString::ConstructFromLiteral));
#endif
#if ENABLE(IFRAME_SEAMLESS)
    DEFINE_STATIC_LOCAL(AtomicString, seamlessDocument, ("-webkit-seamless-document", AtomicString::ConstructFromLiteral));
#endif
    DEFINE_STATIC_LOCAL(AtomicString, inRange, ("in-range", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, outOfRange, ("out-of-range", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, scope, ("scope", AtomicString::ConstructFromLiteral));

    static HashMap<AtomicStringImpl*, CSSSelector::PseudoType>* nameToPseudoType = 0;
    if (!nameToPseudoType) {
        nameToPseudoType = new HashMap<AtomicStringImpl*, CSSSelector::PseudoType>;
        nameToPseudoType->set(active.impl(), CSSSelector::PseudoActive);
        nameToPseudoType->set(after.impl(), CSSSelector::PseudoAfter);
        nameToPseudoType->set(anyLink.impl(), CSSSelector::PseudoAnyLink);
        nameToPseudoType->set(any.impl(), CSSSelector::PseudoAny);
        nameToPseudoType->set(autofill.impl(), CSSSelector::PseudoAutofill);
        nameToPseudoType->set(before.impl(), CSSSelector::PseudoBefore);
        nameToPseudoType->set(checked.impl(), CSSSelector::PseudoChecked);
        nameToPseudoType->set(defaultString.impl(), CSSSelector::PseudoDefault);
        nameToPseudoType->set(disabled.impl(), CSSSelector::PseudoDisabled);
        nameToPseudoType->set(readOnly.impl(), CSSSelector::PseudoReadOnly);
        nameToPseudoType->set(readWrite.impl(), CSSSelector::PseudoReadWrite);
        nameToPseudoType->set(valid.impl(), CSSSelector::PseudoValid);
        nameToPseudoType->set(invalid.impl(), CSSSelector::PseudoInvalid);
        nameToPseudoType->set(drag.impl(), CSSSelector::PseudoDrag);
        nameToPseudoType->set(dragAlias.impl(), CSSSelector::PseudoDrag);
        nameToPseudoType->set(enabled.impl(), CSSSelector::PseudoEnabled);
        nameToPseudoType->set(empty.impl(), CSSSelector::PseudoEmpty);
        nameToPseudoType->set(firstChild.impl(), CSSSelector::PseudoFirstChild);
        nameToPseudoType->set(fullPageMedia.impl(), CSSSelector::PseudoFullPageMedia);
        nameToPseudoType->set(lastChild.impl(), CSSSelector::PseudoLastChild);
        nameToPseudoType->set(lastOfType.impl(), CSSSelector::PseudoLastOfType);
        nameToPseudoType->set(onlyChild.impl(), CSSSelector::PseudoOnlyChild);
        nameToPseudoType->set(onlyOfType.impl(), CSSSelector::PseudoOnlyOfType);
        nameToPseudoType->set(firstLetter.impl(), CSSSelector::PseudoFirstLetter);
        nameToPseudoType->set(firstLine.impl(), CSSSelector::PseudoFirstLine);
        nameToPseudoType->set(firstOfType.impl(), CSSSelector::PseudoFirstOfType);
        nameToPseudoType->set(focus.impl(), CSSSelector::PseudoFocus);
        nameToPseudoType->set(hover.impl(), CSSSelector::PseudoHover);
        nameToPseudoType->set(indeterminate.impl(), CSSSelector::PseudoIndeterminate);
        nameToPseudoType->set(link.impl(), CSSSelector::PseudoLink);
        nameToPseudoType->set(lang.impl(), CSSSelector::PseudoLang);
        nameToPseudoType->set(notStr.impl(), CSSSelector::PseudoNot);
        nameToPseudoType->set(nthChild.impl(), CSSSelector::PseudoNthChild);
        nameToPseudoType->set(nthOfType.impl(), CSSSelector::PseudoNthOfType);
        nameToPseudoType->set(nthLastChild.impl(), CSSSelector::PseudoNthLastChild);
        nameToPseudoType->set(nthLastOfType.impl(), CSSSelector::PseudoNthLastOfType);
        nameToPseudoType->set(root.impl(), CSSSelector::PseudoRoot);
        nameToPseudoType->set(windowInactive.impl(), CSSSelector::PseudoWindowInactive);
        nameToPseudoType->set(decrement.impl(), CSSSelector::PseudoDecrement);
        nameToPseudoType->set(increment.impl(), CSSSelector::PseudoIncrement);
        nameToPseudoType->set(start.impl(), CSSSelector::PseudoStart);
        nameToPseudoType->set(end.impl(), CSSSelector::PseudoEnd);
        nameToPseudoType->set(horizontal.impl(), CSSSelector::PseudoHorizontal);
        nameToPseudoType->set(vertical.impl(), CSSSelector::PseudoVertical);
        nameToPseudoType->set(doubleButton.impl(), CSSSelector::PseudoDoubleButton);
        nameToPseudoType->set(singleButton.impl(), CSSSelector::PseudoSingleButton);
        nameToPseudoType->set(noButton.impl(), CSSSelector::PseudoNoButton);
        nameToPseudoType->set(optional.impl(), CSSSelector::PseudoOptional);
        nameToPseudoType->set(required.impl(), CSSSelector::PseudoRequired);
        nameToPseudoType->set(resizer.impl(), CSSSelector::PseudoResizer);
        nameToPseudoType->set(scope.impl(), CSSSelector::PseudoScope);
        nameToPseudoType->set(scrollbar.impl(), CSSSelector::PseudoScrollbar);
        nameToPseudoType->set(scrollbarButton.impl(), CSSSelector::PseudoScrollbarButton);
        nameToPseudoType->set(scrollbarCorner.impl(), CSSSelector::PseudoScrollbarCorner);
        nameToPseudoType->set(scrollbarThumb.impl(), CSSSelector::PseudoScrollbarThumb);
        nameToPseudoType->set(scrollbarTrack.impl(), CSSSelector::PseudoScrollbarTrack);
        nameToPseudoType->set(scrollbarTrackPiece.impl(), CSSSelector::PseudoScrollbarTrackPiece);
        nameToPseudoType->set(cornerPresent.impl(), CSSSelector::PseudoCornerPresent);
        nameToPseudoType->set(selection.impl(), CSSSelector::PseudoSelection);
        nameToPseudoType->set(target.impl(), CSSSelector::PseudoTarget);
        nameToPseudoType->set(visited.impl(), CSSSelector::PseudoVisited);
        nameToPseudoType->set(firstPage.impl(), CSSSelector::PseudoFirstPage);
        nameToPseudoType->set(leftPage.impl(), CSSSelector::PseudoLeftPage);
        nameToPseudoType->set(rightPage.impl(), CSSSelector::PseudoRightPage);
#if ENABLE(FULLSCREEN_API)
        nameToPseudoType->set(fullScreen.impl(), CSSSelector::PseudoFullScreen);
        nameToPseudoType->set(fullScreenDocument.impl(), CSSSelector::PseudoFullScreenDocument);
        nameToPseudoType->set(fullScreenAncestor.impl(), CSSSelector::PseudoFullScreenAncestor);
        nameToPseudoType->set(animatingFullScreenTransition.impl(), CSSSelector::PseudoAnimatingFullScreenTransition);
#endif
#if ENABLE(VIDEO_TRACK)
        nameToPseudoType->set(cue.impl(), CSSSelector::PseudoCue);
        nameToPseudoType->set(futureCue.impl(), CSSSelector::PseudoFutureCue);
        nameToPseudoType->set(pastCue.impl(), CSSSelector::PseudoPastCue);
#endif
#if ENABLE(IFRAME_SEAMLESS)
        nameToPseudoType->set(seamlessDocument.impl(), CSSSelector::PseudoSeamlessDocument);
#endif
        nameToPseudoType->set(inRange.impl(), CSSSelector::PseudoInRange);
        nameToPseudoType->set(outOfRange.impl(), CSSSelector::PseudoOutOfRange);
    }
    return nameToPseudoType;
}

CSSSelector::PseudoType CSSSelector::parsePseudoType(const AtomicString& name)
{
    if (name.isNull())
        return PseudoUnknown;
    HashMap<AtomicStringImpl*, CSSSelector::PseudoType>* nameToPseudoType = nameToPseudoTypeMap();
    HashMap<AtomicStringImpl*, CSSSelector::PseudoType>::iterator slot = nameToPseudoType->find(name.impl());

    if (slot != nameToPseudoType->end())
        return slot->value;

    if (name.startsWith("-webkit-"))
        return PseudoWebKitCustomElement;
    if (name.startsWith("x-") || name.startsWith("cue"))
        return PseudoUserAgentCustomElement;

    return PseudoUnknown;
}

void CSSSelector::extractPseudoType() const
{
    if (m_match != PseudoClass && m_match != PseudoElement && m_match != PagePseudoClass)
        return;

    m_pseudoType = parsePseudoType(value());

    bool element = false; // pseudo-element
    bool compat = false; // single colon compatbility mode
    bool isPagePseudoClass = false; // Page pseudo-class

    switch (m_pseudoType) {
    case PseudoAfter:
    case PseudoBefore:
#if ENABLE(VIDEO_TRACK)
    case PseudoCue:
#endif
    case PseudoFirstLetter:
    case PseudoFirstLine:
        compat = true;
#if ENABLE(SHADOW_DOM)
    case PseudoDistributed:
#endif
    case PseudoResizer:
    case PseudoScrollbar:
    case PseudoScrollbarCorner:
    case PseudoScrollbarButton:
    case PseudoScrollbarThumb:
    case PseudoScrollbarTrack:
    case PseudoScrollbarTrackPiece:
    case PseudoSelection:
    case PseudoUserAgentCustomElement:
    case PseudoWebKitCustomElement:
        element = true;
        break;
    case PseudoUnknown:
    case PseudoEmpty:
    case PseudoFirstChild:
    case PseudoFirstOfType:
    case PseudoLastChild:
    case PseudoLastOfType:
    case PseudoOnlyChild:
    case PseudoOnlyOfType:
    case PseudoNthChild:
    case PseudoNthOfType:
    case PseudoNthLastChild:
    case PseudoNthLastOfType:
    case PseudoLink:
    case PseudoVisited:
    case PseudoAny:
    case PseudoAnyLink:
    case PseudoAutofill:
    case PseudoHover:
    case PseudoDrag:
    case PseudoFocus:
    case PseudoActive:
    case PseudoChecked:
    case PseudoEnabled:
    case PseudoFullPageMedia:
    case PseudoDefault:
    case PseudoDisabled:
    case PseudoOptional:
    case PseudoRequired:
    case PseudoReadOnly:
    case PseudoReadWrite:
    case PseudoScope:
    case PseudoValid:
    case PseudoInvalid:
    case PseudoIndeterminate:
    case PseudoTarget:
    case PseudoLang:
    case PseudoNot:
    case PseudoRoot:
    case PseudoScrollbarBack:
    case PseudoScrollbarForward:
    case PseudoWindowInactive:
    case PseudoCornerPresent:
    case PseudoDecrement:
    case PseudoIncrement:
    case PseudoHorizontal:
    case PseudoVertical:
    case PseudoStart:
    case PseudoEnd:
    case PseudoDoubleButton:
    case PseudoSingleButton:
    case PseudoNoButton:
    case PseudoNotParsed:
#if ENABLE(FULLSCREEN_API)
    case PseudoFullScreen:
    case PseudoFullScreenDocument:
    case PseudoFullScreenAncestor:
    case PseudoAnimatingFullScreenTransition:
#endif
#if ENABLE(IFRAME_SEAMLESS)
    case PseudoSeamlessDocument:
#endif
    case PseudoInRange:
    case PseudoOutOfRange:
#if ENABLE(VIDEO_TRACK)
    case PseudoFutureCue:
    case PseudoPastCue:
#endif
        break;
    case PseudoFirstPage:
    case PseudoLeftPage:
    case PseudoRightPage:
        isPagePseudoClass = true;
        break;
    }

    bool matchPagePseudoClass = (m_match == PagePseudoClass);
    if (matchPagePseudoClass != isPagePseudoClass)
        m_pseudoType = PseudoUnknown;
    else if (m_match == PseudoClass && element) {
        if (!compat)
            m_pseudoType = PseudoUnknown;
        else
           m_match = PseudoElement;
    } else if (m_match == PseudoElement && !element)
        m_pseudoType = PseudoUnknown;
}

bool CSSSelector::operator==(const CSSSelector& other) const
{
    const CSSSelector* sel1 = this;
    const CSSSelector* sel2 = &other;

    while (sel1 && sel2) {
        if (sel1->attribute() != sel2->attribute()
            || sel1->relation() != sel2->relation()
            || sel1->m_match != sel2->m_match
            || sel1->value() != sel2->value()
            || sel1->pseudoType() != sel2->pseudoType()
            || sel1->argument() != sel2->argument()) {
            return false;
        }
        if (sel1->m_match == Tag) {
            if (sel1->tagQName() != sel2->tagQName())
                return false;
        }
        sel1 = sel1->tagHistory();
        sel2 = sel2->tagHistory();
    }

    if (sel1 || sel2)
        return false;

    return true;
}

String CSSSelector::selectorText(const String& rightSide) const
{
    StringBuilder str;

    if (m_match == CSSSelector::Tag && !m_tagIsForNamespaceRule) {
        if (tagQName().prefix().isNull())
            str.append(tagQName().localName());
        else {
            str.append(tagQName().prefix().string());
            str.append('|');
            str.append(tagQName().localName());
        }
    }

    const CSSSelector* cs = this;
    while (true) {
        if (cs->m_match == CSSSelector::Id) {
            str.append('#');
            serializeIdentifier(cs->value(), str);
        } else if (cs->m_match == CSSSelector::Class) {
            str.append('.');
            serializeIdentifier(cs->value(), str);
        } else if (cs->m_match == CSSSelector::PseudoClass || cs->m_match == CSSSelector::PagePseudoClass) {
            str.append(':');
            str.append(cs->value());

            switch (cs->pseudoType()) {
            case PseudoNot:
                if (const CSSSelectorList* selectorList = cs->selectorList())
                    str.append(selectorList->first()->selectorText());
                str.append(')');
                break;
            case PseudoLang:
            case PseudoNthChild:
            case PseudoNthLastChild:
            case PseudoNthOfType:
            case PseudoNthLastOfType:
                str.append(cs->argument());
                str.append(')');
                break;
            case PseudoAny: {
                const CSSSelector* firstSubSelector = cs->selectorList()->first();
                for (const CSSSelector* subSelector = firstSubSelector; subSelector; subSelector = CSSSelectorList::next(subSelector)) {
                    if (subSelector != firstSubSelector)
                        str.append(',');
                    str.append(subSelector->selectorText());
                }
                str.append(')');
                break;
            }
            default:
                break;
            }
        } else if (cs->m_match == CSSSelector::PseudoElement) {
            str.appendLiteral("::");
            str.append(cs->value());
        } else if (cs->isAttributeSelector()) {
            str.append('[');
            const AtomicString& prefix = cs->attribute().prefix();
            if (!prefix.isNull()) {
                str.append(prefix);
                str.append('|');
            }
            str.append(cs->attribute().localName());
            switch (cs->m_match) {
                case CSSSelector::Exact:
                    str.append('=');
                    break;
                case CSSSelector::Set:
                    // set has no operator or value, just the attrName
                    str.append(']');
                    break;
                case CSSSelector::List:
                    str.appendLiteral("~=");
                    break;
                case CSSSelector::Hyphen:
                    str.appendLiteral("|=");
                    break;
                case CSSSelector::Begin:
                    str.appendLiteral("^=");
                    break;
                case CSSSelector::End:
                    str.appendLiteral("$=");
                    break;
                case CSSSelector::Contain:
                    str.appendLiteral("*=");
                    break;
                default:
                    break;
            }
            if (cs->m_match != CSSSelector::Set) {
                serializeString(cs->value(), str);
                str.append(']');
            }
        }
        if (cs->relation() != CSSSelector::SubSelector || !cs->tagHistory())
            break;
        cs = cs->tagHistory();
    }

    if (const CSSSelector* tagHistory = cs->tagHistory()) {
        switch (cs->relation()) {
        case CSSSelector::Descendant:
            return tagHistory->selectorText(" " + str.toString() + rightSide);
        case CSSSelector::Child:
            return tagHistory->selectorText(" > " + str.toString() + rightSide);
        case CSSSelector::DirectAdjacent:
            return tagHistory->selectorText(" + " + str.toString() + rightSide);
        case CSSSelector::IndirectAdjacent:
            return tagHistory->selectorText(" ~ " + str.toString() + rightSide);
        case CSSSelector::SubSelector:
            ASSERT_NOT_REACHED();
        case CSSSelector::ShadowDescendant:
            return tagHistory->selectorText(str.toString() + rightSide);
        }
    }
    return str.toString() + rightSide;
}

void CSSSelector::setAttribute(const QualifiedName& value, bool isCaseInsensitive)
{
    createRareData();
    m_data.m_rareData->m_attribute = value;
    m_data.m_rareData->m_attributeCanonicalLocalName = isCaseInsensitive ? value.localName().lower() : value.localName();
}

void CSSSelector::setArgument(const AtomicString& value)
{
    createRareData();
    m_data.m_rareData->m_argument = value;
}

void CSSSelector::setSelectorList(PassOwnPtr<CSSSelectorList> selectorList)
{
    createRareData();
    m_data.m_rareData->m_selectorList = selectorList;
}

bool CSSSelector::parseNth() const
{
    if (!m_hasRareData)
        return false;
    if (m_parsedNth)
        return true;
    m_parsedNth = m_data.m_rareData->parseNth();
    return m_parsedNth;
}

bool CSSSelector::matchNth(int count) const
{
    ASSERT(m_hasRareData);
    return m_data.m_rareData->matchNth(count);
}

CSSSelector::RareData::RareData(PassRefPtr<AtomicStringImpl> value)
    : m_value(value.leakRef())
    , m_a(0)
    , m_b(0)
    , m_attribute(anyQName())
    , m_argument(nullAtom)
{
}

CSSSelector::RareData::~RareData()
{
    if (m_value)
        m_value->deref();
}

// a helper function for parsing nth-arguments
bool CSSSelector::RareData::parseNth()
{
    String argument = m_argument.lower();

    if (argument.isEmpty())
        return false;

    m_a = 0;
    m_b = 0;
    if (argument == "odd") {
        m_a = 2;
        m_b = 1;
    } else if (argument == "even") {
        m_a = 2;
        m_b = 0;
    } else {
        size_t n = argument.find('n');
        if (n != notFound) {
            if (argument[0] == '-') {
                if (n == 1)
                    m_a = -1; // -n == -1n
                else
                    m_a = argument.substring(0, n).toInt();
            } else if (!n)
                m_a = 1; // n == 1n
            else
                m_a = argument.substring(0, n).toInt();

            size_t p = argument.find('+', n);
            if (p != notFound)
                m_b = argument.substring(p + 1, argument.length() - p - 1).toInt();
            else {
                p = argument.find('-', n);
                if (p != notFound)
                    m_b = -argument.substring(p + 1, argument.length() - p - 1).toInt();
            }
        } else
            m_b = argument.toInt();
    }
    return true;
}

// a helper function for checking nth-arguments
bool CSSSelector::RareData::matchNth(int count)
{
    if (!m_a)
        return count == m_b;
    else if (m_a > 0) {
        if (count < m_b)
            return false;
        return (count - m_b) % m_a == 0;
    } else {
        if (count > m_b)
            return false;
        return (m_b - count) % (-m_a) == 0;
    }
}

} // namespace WebCore
