/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

namespace WebCore {

using namespace HTMLNames;
    
void CSSSelector::createRareData()
{
    if (m_hasRareData) 
        return;
    // Move the value to the rare data stucture.
    m_data.m_rareData = new RareData(adoptRef(m_data.m_value));
    m_hasRareData = true;
}

unsigned CSSSelector::specificity() const
{
    // make sure the result doesn't overflow
    static const unsigned maxValueMask = 0xffffff;
    unsigned total = 0;
    for (const CSSSelector* selector = this; selector; selector = selector->tagHistory()) {
        if (selector->m_isForPage)
            return (total + selector->specificityForPage()) & maxValueMask;
        total = (total + selector->specificityForOneSelector()) & maxValueMask;
    }
    return total;
}

inline unsigned CSSSelector::specificityForOneSelector() const
{
    // FIXME: Pseudo-elements and pseudo-classes do not have the same specificity. This function
    // isn't quite correct.
    unsigned s = (m_tag.localName() == starAtom ? 0 : 1);
    switch (m_match) {
    case Id:
        s += 0x10000;
        break;
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
        if (pseudoType() == PseudoNot) {
            ASSERT(selectorList());
            s += selectorList()->first()->specificityForOneSelector();
        } else
            s += 0x100;
    case None:
        break;
    }
    return s;
}

unsigned CSSSelector::specificityForPage() const
{
    // See http://dev.w3.org/csswg/css3-page/#cascading-and-page-context
    unsigned s = (m_tag.localName() == starAtom ? 0 : 4);

    switch (pseudoType()) {
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
    case PseudoFileUploadButton:
        return FILE_UPLOAD_BUTTON;
    case PseudoInputPlaceholder:
        return INPUT_PLACEHOLDER;
#if ENABLE(INPUT_SPEECH)
    case PseudoInputSpeechButton:
        return INPUT_SPEECH_BUTTON;
#endif
    case PseudoSearchCancelButton:
        return SEARCH_CANCEL_BUTTON;
    case PseudoSearchDecoration:
        return SEARCH_DECORATION;
    case PseudoSearchResultsDecoration:
        return SEARCH_RESULTS_DECORATION;
    case PseudoSearchResultsButton:
        return SEARCH_RESULTS_BUTTON;
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
    case PseudoInnerSpinButton:
        return INNER_SPIN_BUTTON;
    case PseudoOuterSpinButton:
        return OUTER_SPIN_BUTTON;
#if ENABLE(FULLSCREEN_API)
    case PseudoFullScreen:
        return FULL_SCREEN;
    case PseudoFullScreenDocument:
        return FULL_SCREEN_DOCUMENT;
    case PseudoFullScreenMediaDocument:
        return FULL_SCREEN_MEDIA_DOCUMENT;
#endif
            
    case PseudoInputListButton:
#if ENABLE(DATALIST)
        return INPUT_LIST_BUTTON;
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
    DEFINE_STATIC_LOCAL(AtomicString, active, ("active"));
    DEFINE_STATIC_LOCAL(AtomicString, after, ("after"));
    DEFINE_STATIC_LOCAL(AtomicString, any, ("-webkit-any("));
    DEFINE_STATIC_LOCAL(AtomicString, anyLink, ("-webkit-any-link"));
    DEFINE_STATIC_LOCAL(AtomicString, autofill, ("-webkit-autofill"));
    DEFINE_STATIC_LOCAL(AtomicString, before, ("before"));
    DEFINE_STATIC_LOCAL(AtomicString, checked, ("checked"));
    DEFINE_STATIC_LOCAL(AtomicString, fileUploadButton, ("-webkit-file-upload-button"));
#if ENABLE(INPUT_SPEECH)
    DEFINE_STATIC_LOCAL(AtomicString, inputSpeechButton, ("-webkit-input-speech-button"));
#endif
    DEFINE_STATIC_LOCAL(AtomicString, defaultString, ("default"));
    DEFINE_STATIC_LOCAL(AtomicString, disabled, ("disabled"));
    DEFINE_STATIC_LOCAL(AtomicString, readOnly, ("read-only"));
    DEFINE_STATIC_LOCAL(AtomicString, readWrite, ("read-write"));
    DEFINE_STATIC_LOCAL(AtomicString, valid, ("valid"));
    DEFINE_STATIC_LOCAL(AtomicString, invalid, ("invalid"));
    DEFINE_STATIC_LOCAL(AtomicString, drag, ("-webkit-drag"));
    DEFINE_STATIC_LOCAL(AtomicString, dragAlias, ("-khtml-drag")); // was documented with this name in Apple documentation, so keep an alia
    DEFINE_STATIC_LOCAL(AtomicString, empty, ("empty"));
    DEFINE_STATIC_LOCAL(AtomicString, enabled, ("enabled"));
    DEFINE_STATIC_LOCAL(AtomicString, firstChild, ("first-child"));
    DEFINE_STATIC_LOCAL(AtomicString, firstLetter, ("first-letter"));
    DEFINE_STATIC_LOCAL(AtomicString, firstLine, ("first-line"));
    DEFINE_STATIC_LOCAL(AtomicString, firstOfType, ("first-of-type"));
    DEFINE_STATIC_LOCAL(AtomicString, fullPageMedia, ("-webkit-full-page-media"));
    DEFINE_STATIC_LOCAL(AtomicString, nthChild, ("nth-child("));
    DEFINE_STATIC_LOCAL(AtomicString, nthOfType, ("nth-of-type("));
    DEFINE_STATIC_LOCAL(AtomicString, nthLastChild, ("nth-last-child("));
    DEFINE_STATIC_LOCAL(AtomicString, nthLastOfType, ("nth-last-of-type("));
    DEFINE_STATIC_LOCAL(AtomicString, focus, ("focus"));
    DEFINE_STATIC_LOCAL(AtomicString, hover, ("hover"));
    DEFINE_STATIC_LOCAL(AtomicString, indeterminate, ("indeterminate"));
    DEFINE_STATIC_LOCAL(AtomicString, innerSpinButton, ("-webkit-inner-spin-button"));
#if ENABLE(DATALIST)
    DEFINE_STATIC_LOCAL(AtomicString, inputListButton, ("-webkit-input-list-button"));
#endif
    DEFINE_STATIC_LOCAL(AtomicString, inputPlaceholder, ("-webkit-input-placeholder"));
    DEFINE_STATIC_LOCAL(AtomicString, lastChild, ("last-child"));
    DEFINE_STATIC_LOCAL(AtomicString, lastOfType, ("last-of-type"));
    DEFINE_STATIC_LOCAL(AtomicString, link, ("link"));
    DEFINE_STATIC_LOCAL(AtomicString, lang, ("lang("));
    DEFINE_STATIC_LOCAL(AtomicString, notStr, ("not("));
    DEFINE_STATIC_LOCAL(AtomicString, onlyChild, ("only-child"));
    DEFINE_STATIC_LOCAL(AtomicString, onlyOfType, ("only-of-type"));
    DEFINE_STATIC_LOCAL(AtomicString, optional, ("optional"));
    DEFINE_STATIC_LOCAL(AtomicString, outerSpinButton, ("-webkit-outer-spin-button"));
    DEFINE_STATIC_LOCAL(AtomicString, required, ("required"));
    DEFINE_STATIC_LOCAL(AtomicString, resizer, ("-webkit-resizer"));
    DEFINE_STATIC_LOCAL(AtomicString, root, ("root"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbar, ("-webkit-scrollbar"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarButton, ("-webkit-scrollbar-button"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarCorner, ("-webkit-scrollbar-corner"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarThumb, ("-webkit-scrollbar-thumb"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarTrack, ("-webkit-scrollbar-track"));
    DEFINE_STATIC_LOCAL(AtomicString, scrollbarTrackPiece, ("-webkit-scrollbar-track-piece"));
    DEFINE_STATIC_LOCAL(AtomicString, searchCancelButton, ("-webkit-search-cancel-button"));
    DEFINE_STATIC_LOCAL(AtomicString, searchDecoration, ("-webkit-search-decoration"));
    DEFINE_STATIC_LOCAL(AtomicString, searchResultsDecoration, ("-webkit-search-results-decoration"));
    DEFINE_STATIC_LOCAL(AtomicString, searchResultsButton, ("-webkit-search-results-button"));
    DEFINE_STATIC_LOCAL(AtomicString, selection, ("selection"));
    DEFINE_STATIC_LOCAL(AtomicString, target, ("target"));
    DEFINE_STATIC_LOCAL(AtomicString, visited, ("visited"));
    DEFINE_STATIC_LOCAL(AtomicString, windowInactive, ("window-inactive"));
    DEFINE_STATIC_LOCAL(AtomicString, decrement, ("decrement"));
    DEFINE_STATIC_LOCAL(AtomicString, increment, ("increment"));
    DEFINE_STATIC_LOCAL(AtomicString, start, ("start"));
    DEFINE_STATIC_LOCAL(AtomicString, end, ("end"));
    DEFINE_STATIC_LOCAL(AtomicString, horizontal, ("horizontal"));
    DEFINE_STATIC_LOCAL(AtomicString, vertical, ("vertical"));
    DEFINE_STATIC_LOCAL(AtomicString, doubleButton, ("double-button"));
    DEFINE_STATIC_LOCAL(AtomicString, singleButton, ("single-button"));
    DEFINE_STATIC_LOCAL(AtomicString, noButton, ("no-button"));
    DEFINE_STATIC_LOCAL(AtomicString, cornerPresent, ("corner-present"));
    // Paged Media pseudo-classes
    DEFINE_STATIC_LOCAL(AtomicString, firstPage, ("first"));
    DEFINE_STATIC_LOCAL(AtomicString, leftPage, ("left"));
    DEFINE_STATIC_LOCAL(AtomicString, rightPage, ("right"));
#if ENABLE(FULLSCREEN_API)
    DEFINE_STATIC_LOCAL(AtomicString, fullScreen, ("-webkit-full-screen"));
    DEFINE_STATIC_LOCAL(AtomicString, fullScreenDocument, ("-webkit-full-screen-document"));
    DEFINE_STATIC_LOCAL(AtomicString, fullScreenMediaDocument, ("-webkit-full-screen-media-document"));
#endif
    DEFINE_STATIC_LOCAL(AtomicString, inRange, ("in-range"));
    DEFINE_STATIC_LOCAL(AtomicString, outOfRange, ("out-of-range"));

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
        nameToPseudoType->set(fileUploadButton.impl(), CSSSelector::PseudoFileUploadButton);
#if ENABLE(INPUT_SPEECH)
        nameToPseudoType->set(inputSpeechButton.impl(), CSSSelector::PseudoInputSpeechButton);
#endif
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
#if ENABLE(DATALIST)
        nameToPseudoType->set(inputListButton.impl(), CSSSelector::PseudoInputListButton);
#endif
        nameToPseudoType->set(inputPlaceholder.impl(), CSSSelector::PseudoInputPlaceholder);
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
        nameToPseudoType->set(innerSpinButton.impl(), CSSSelector::PseudoInnerSpinButton);
        nameToPseudoType->set(link.impl(), CSSSelector::PseudoLink);
        nameToPseudoType->set(lang.impl(), CSSSelector::PseudoLang);
        nameToPseudoType->set(notStr.impl(), CSSSelector::PseudoNot);
        nameToPseudoType->set(nthChild.impl(), CSSSelector::PseudoNthChild);
        nameToPseudoType->set(nthOfType.impl(), CSSSelector::PseudoNthOfType);
        nameToPseudoType->set(nthLastChild.impl(), CSSSelector::PseudoNthLastChild);
        nameToPseudoType->set(nthLastOfType.impl(), CSSSelector::PseudoNthLastOfType);
        nameToPseudoType->set(outerSpinButton.impl(), CSSSelector::PseudoOuterSpinButton);
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
        nameToPseudoType->set(scrollbar.impl(), CSSSelector::PseudoScrollbar);
        nameToPseudoType->set(scrollbarButton.impl(), CSSSelector::PseudoScrollbarButton);
        nameToPseudoType->set(scrollbarCorner.impl(), CSSSelector::PseudoScrollbarCorner);
        nameToPseudoType->set(scrollbarThumb.impl(), CSSSelector::PseudoScrollbarThumb);
        nameToPseudoType->set(scrollbarTrack.impl(), CSSSelector::PseudoScrollbarTrack);
        nameToPseudoType->set(scrollbarTrackPiece.impl(), CSSSelector::PseudoScrollbarTrackPiece);
        nameToPseudoType->set(cornerPresent.impl(), CSSSelector::PseudoCornerPresent);
        nameToPseudoType->set(searchCancelButton.impl(), CSSSelector::PseudoSearchCancelButton);
        nameToPseudoType->set(searchDecoration.impl(), CSSSelector::PseudoSearchDecoration);
        nameToPseudoType->set(searchResultsDecoration.impl(), CSSSelector::PseudoSearchResultsDecoration);
        nameToPseudoType->set(searchResultsButton.impl(), CSSSelector::PseudoSearchResultsButton);
        nameToPseudoType->set(selection.impl(), CSSSelector::PseudoSelection);
        nameToPseudoType->set(target.impl(), CSSSelector::PseudoTarget);
        nameToPseudoType->set(visited.impl(), CSSSelector::PseudoVisited);
        nameToPseudoType->set(firstPage.impl(), CSSSelector::PseudoFirstPage);
        nameToPseudoType->set(leftPage.impl(), CSSSelector::PseudoLeftPage);
        nameToPseudoType->set(rightPage.impl(), CSSSelector::PseudoRightPage);
#if ENABLE(FULLSCREEN_API)
        nameToPseudoType->set(fullScreen.impl(), CSSSelector::PseudoFullScreen);
        nameToPseudoType->set(fullScreenDocument.impl(), CSSSelector::PseudoFullScreenDocument);
        nameToPseudoType->set(fullScreenMediaDocument.impl(), CSSSelector::PseudoFullScreenMediaDocument);
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
    return slot == nameToPseudoType->end() ? PseudoUnknown : slot->second;
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
    case PseudoFirstLetter:
    case PseudoFirstLine:
        compat = true;
    case PseudoFileUploadButton:
    case PseudoInputListButton:
    case PseudoInputPlaceholder:
#if ENABLE(INPUT_SPEECH)
    case PseudoInputSpeechButton:
#endif
    case PseudoInnerSpinButton:
    case PseudoOuterSpinButton: 
    case PseudoResizer:
    case PseudoScrollbar:
    case PseudoScrollbarCorner:
    case PseudoScrollbarButton:
    case PseudoScrollbarThumb:
    case PseudoScrollbarTrack:
    case PseudoScrollbarTrackPiece:
    case PseudoSearchCancelButton:
    case PseudoSearchDecoration:
    case PseudoSearchResultsDecoration:
    case PseudoSearchResultsButton:
    case PseudoSelection:
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
    case PseudoFullScreenMediaDocument:
#endif
    case PseudoInRange:
    case PseudoOutOfRange:
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

bool CSSSelector::operator==(const CSSSelector& other)
{
    const CSSSelector* sel1 = this;
    const CSSSelector* sel2 = &other;

    while (sel1 && sel2) {
        if (sel1->m_tag != sel2->m_tag || sel1->attribute() != sel2->attribute() ||
             sel1->relation() != sel2->relation() || sel1->m_match != sel2->m_match ||
             sel1->value() != sel2->value() ||
             sel1->pseudoType() != sel2->pseudoType() ||
             sel1->argument() != sel2->argument())
            return false;
        sel1 = sel1->tagHistory();
        sel2 = sel2->tagHistory();
    }

    if (sel1 || sel2)
        return false;

    return true;
}

String CSSSelector::selectorText() const
{
    String str = "";

    const AtomicString& prefix = m_tag.prefix();
    const AtomicString& localName = m_tag.localName();
    if (m_match == CSSSelector::None || !prefix.isNull() || localName != starAtom) {
        if (prefix.isNull())
            str = localName;
        else {
            str = prefix.string();
            str.append("|");
            str.append(localName);
        }
    }

    const CSSSelector* cs = this;
    while (true) {
        if (cs->m_match == CSSSelector::Id) {
            str += "#";
            serializeIdentifier(cs->value(), str);
        } else if (cs->m_match == CSSSelector::Class) {
            str += ".";
            serializeIdentifier(cs->value(), str);
        } else if (cs->m_match == CSSSelector::PseudoClass || cs->m_match == CSSSelector::PagePseudoClass) {
            str += ":";
            str += cs->value();

            switch (cs->pseudoType()) {
            case PseudoNot:
                ASSERT(cs->selectorList());
                str += cs->selectorList()->first()->selectorText();
                str += ")";
                break;
            case PseudoLang:
            case PseudoNthChild:
            case PseudoNthLastChild:
            case PseudoNthOfType:
            case PseudoNthLastOfType:
                str += cs->argument();
                str += ")";
                break;
            case PseudoAny: {
                CSSSelector* firstSubSelector = cs->selectorList()->first();
                for (CSSSelector* subSelector = firstSubSelector; subSelector; subSelector = CSSSelectorList::next(subSelector)) {
                    if (subSelector != firstSubSelector)
                        str += ",";
                    str += subSelector->selectorText();
                }
                str += ")";
                break;
            }
            default:
                break;
            }
        } else if (cs->m_match == CSSSelector::PseudoElement) {
            str += "::";
            str += cs->value();
        } else if (cs->hasAttribute()) {
            str += "[";
            const AtomicString& prefix = cs->attribute().prefix();
            if (!prefix.isNull()) {
                str.append(prefix);
                str.append("|");
            }
            str += cs->attribute().localName();
            switch (cs->m_match) {
                case CSSSelector::Exact:
                    str += "=";
                    break;
                case CSSSelector::Set:
                    // set has no operator or value, just the attrName
                    str += "]";
                    break;
                case CSSSelector::List:
                    str += "~=";
                    break;
                case CSSSelector::Hyphen:
                    str += "|=";
                    break;
                case CSSSelector::Begin:
                    str += "^=";
                    break;
                case CSSSelector::End:
                    str += "$=";
                    break;
                case CSSSelector::Contain:
                    str += "*=";
                    break;
                default:
                    break;
            }
            if (cs->m_match != CSSSelector::Set) {
                serializeString(cs->value(), str);
                str += "]";
            }
        }
        if (cs->relation() != CSSSelector::SubSelector || !cs->tagHistory())
            break;
        cs = cs->tagHistory();
    }

    if (CSSSelector* tagHistory = cs->tagHistory()) {
        String tagHistoryText = tagHistory->selectorText();
        if (cs->relation() == CSSSelector::DirectAdjacent)
            str = tagHistoryText + " + " + str;
        else if (cs->relation() == CSSSelector::IndirectAdjacent)
            str = tagHistoryText + " ~ " + str;
        else if (cs->relation() == CSSSelector::Child)
            str = tagHistoryText + " > " + str;
        else
            // Descendant
            str = tagHistoryText + " " + str;
    }

    return str;
}

const QualifiedName& CSSSelector::attribute() const
{ 
    switch (m_match) {
    case Id:
        return idAttr;
    case Class:
        return classAttr;
    default:
        return m_hasRareData ? m_data.m_rareData->m_attribute : anyQName();
    }
}

void CSSSelector::setAttribute(const QualifiedName& value) 
{ 
    createRareData(); 
    m_data.m_rareData->m_attribute = value; 
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

bool CSSSelector::parseNth()
{
    if (!m_hasRareData)
        return false;
    if (m_parsedNth)
        return true;
    m_parsedNth = m_data.m_rareData->parseNth();
    return m_parsedNth;
}

bool CSSSelector::matchNth(int count)
{
    ASSERT(m_hasRareData);
    return m_data.m_rareData->matchNth(count);
}

bool CSSSelector::isSimple() const
{
    if (selectorList() || tagHistory() || matchesPseudoElement())
        return false;

    int numConditions = 0;

    // hasTag() cannot be be used here because namespace may not be nullAtom.
    // Example:
    //     @namespace "http://www.w3.org/2000/svg";
    //     svg:not(:root) { ...
    if (m_tag != starAtom)
        numConditions++;

    if (m_match == Id || m_match == Class || m_match == PseudoClass)
        numConditions++;

    if (m_hasRareData && m_data.m_rareData->m_attribute != anyQName())
        numConditions++;

    // numConditions is 0 for a universal selector.
    // numConditions is 1 for other simple selectors.
    return numConditions <= 1;
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
