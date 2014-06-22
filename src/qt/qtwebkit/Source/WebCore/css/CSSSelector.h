/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef CSSSelector_h
#define CSSSelector_h

#include "QualifiedName.h"
#include "RenderStyleConstants.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
    class CSSSelectorList;

    // this class represents a selector for a StyleRule
    class CSSSelector {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        CSSSelector();
        CSSSelector(const CSSSelector&);
        explicit CSSSelector(const QualifiedName&, bool tagIsForNamespaceRule = false);

        ~CSSSelector();

        /**
         * Re-create selector text from selector's data
         */
        String selectorText(const String& = "") const;

        // checks if the 2 selectors (including sub selectors) agree.
        bool operator==(const CSSSelector&) const;

        // tag == -1 means apply to all elements (Selector = *)

        unsigned specificity() const;

        /* how the attribute value has to match.... Default is Exact */
        enum Match {
            Unknown = 0,
            Tag,
            Id,
            Class,
            Exact,
            Set,
            List,
            Hyphen,
            PseudoClass,
            PseudoElement,
            Contain, // css3: E[foo*="bar"]
            Begin, // css3: E[foo^="bar"]
            End, // css3: E[foo$="bar"]
            PagePseudoClass
        };

        enum Relation {
            Descendant = 0,
            Child,
            DirectAdjacent,
            IndirectAdjacent,
            SubSelector,
            ShadowDescendant,
        };

        enum PseudoType {
            PseudoNotParsed = 0,
            PseudoUnknown,
            PseudoEmpty,
            PseudoFirstChild,
            PseudoFirstOfType,
            PseudoLastChild,
            PseudoLastOfType,
            PseudoOnlyChild,
            PseudoOnlyOfType,
            PseudoFirstLine,
            PseudoFirstLetter,
            PseudoNthChild,
            PseudoNthOfType,
            PseudoNthLastChild,
            PseudoNthLastOfType,
            PseudoLink,
            PseudoVisited,
            PseudoAny,
            PseudoAnyLink,
            PseudoAutofill,
            PseudoHover,
            PseudoDrag,
            PseudoFocus,
            PseudoActive,
            PseudoChecked,
            PseudoEnabled,
            PseudoFullPageMedia,
            PseudoDefault,
            PseudoDisabled,
            PseudoOptional,
            PseudoRequired,
            PseudoReadOnly,
            PseudoReadWrite,
            PseudoValid,
            PseudoInvalid,
            PseudoIndeterminate,
            PseudoTarget,
            PseudoBefore,
            PseudoAfter,
            PseudoLang,
            PseudoNot,
            PseudoResizer,
            PseudoRoot,
            PseudoScope,
            PseudoScrollbar,
            PseudoScrollbarBack,
            PseudoScrollbarButton,
            PseudoScrollbarCorner,
            PseudoScrollbarForward,
            PseudoScrollbarThumb,
            PseudoScrollbarTrack,
            PseudoScrollbarTrackPiece,
            PseudoWindowInactive,
            PseudoCornerPresent,
            PseudoDecrement,
            PseudoIncrement,
            PseudoHorizontal,
            PseudoVertical,
            PseudoStart,
            PseudoEnd,
            PseudoDoubleButton,
            PseudoSingleButton,
            PseudoNoButton,
            PseudoSelection,
            PseudoLeftPage,
            PseudoRightPage,
            PseudoFirstPage,
#if ENABLE(FULLSCREEN_API)
            PseudoFullScreen,
            PseudoFullScreenDocument,
            PseudoFullScreenAncestor,
            PseudoAnimatingFullScreenTransition,
#endif
            PseudoInRange,
            PseudoOutOfRange,
            PseudoUserAgentCustomElement,
            PseudoWebKitCustomElement,
#if ENABLE(VIDEO_TRACK)
            PseudoCue,
            PseudoFutureCue,
            PseudoPastCue,
#endif
#if ENABLE(IFRAME_SEAMLESS)
            PseudoSeamlessDocument,
#endif
        };

        enum MarginBoxType {
            TopLeftCornerMarginBox,
            TopLeftMarginBox,
            TopCenterMarginBox,
            TopRightMarginBox,
            TopRightCornerMarginBox,
            BottomLeftCornerMarginBox,
            BottomLeftMarginBox,
            BottomCenterMarginBox,
            BottomRightMarginBox,
            BottomRightCornerMarginBox,
            LeftTopMarginBox,
            LeftMiddleMarginBox,
            LeftBottomMarginBox,
            RightTopMarginBox,
            RightMiddleMarginBox,
            RightBottomMarginBox,
        };

        PseudoType pseudoType() const
        {
            if (m_pseudoType == PseudoNotParsed)
                extractPseudoType();
            return static_cast<PseudoType>(m_pseudoType);
        }

        static PseudoType parsePseudoType(const AtomicString&);
        static PseudoId pseudoId(PseudoType);

        // Selectors are kept in an array by CSSSelectorList. The next component of the selector is
        // the next item in the array.
        const CSSSelector* tagHistory() const { return m_isLastInTagHistory ? 0 : const_cast<CSSSelector*>(this + 1); }

        const QualifiedName& tagQName() const;
        const AtomicString& value() const;
        const QualifiedName& attribute() const;
        const AtomicString& attributeCanonicalLocalName() const;
        const AtomicString& argument() const { return m_hasRareData ? m_data.m_rareData->m_argument : nullAtom; }
        const CSSSelectorList* selectorList() const { return m_hasRareData ? m_data.m_rareData->m_selectorList.get() : 0; }

        void setValue(const AtomicString&);
        void setAttribute(const QualifiedName&, bool isCaseInsensitive);
        void setArgument(const AtomicString&);
        void setSelectorList(PassOwnPtr<CSSSelectorList>);

        bool parseNth() const;
        bool matchNth(int count) const;

        bool matchesPseudoElement() const;
        bool isUnknownPseudoElement() const;
        bool isCustomPseudoElement() const;
        bool isSiblingSelector() const;
        bool isAttributeSelector() const;

        Relation relation() const { return static_cast<Relation>(m_relation); }

        bool isLastInSelectorList() const { return m_isLastInSelectorList; }
        void setLastInSelectorList() { m_isLastInSelectorList = true; }
        bool isLastInTagHistory() const { return m_isLastInTagHistory; }
        void setNotLastInTagHistory() { m_isLastInTagHistory = false; }

        bool isSimple() const;

        bool isForPage() const { return m_isForPage; }
        void setForPage() { m_isForPage = true; }

        unsigned m_relation           : 3; // enum Relation
        mutable unsigned m_match      : 4; // enum Match
        mutable unsigned m_pseudoType : 8; // PseudoType

    private:
        mutable bool m_parsedNth      : 1; // Used for :nth-*
        bool m_isLastInSelectorList   : 1;
        bool m_isLastInTagHistory     : 1;
        bool m_hasRareData            : 1;
        bool m_isForPage              : 1;
        bool m_tagIsForNamespaceRule  : 1;

        unsigned specificityForOneSelector() const;
        unsigned specificityForPage() const;
        void extractPseudoType() const;

        // Hide.
        CSSSelector& operator=(const CSSSelector&);

        struct RareData : public RefCounted<RareData> {
            static PassRefPtr<RareData> create(PassRefPtr<AtomicStringImpl> value) { return adoptRef(new RareData(value)); }
            ~RareData();

            bool parseNth();
            bool matchNth(int count);

            AtomicStringImpl* m_value; // Plain pointer to keep things uniform with the union.
            int m_a; // Used for :nth-*
            int m_b; // Used for :nth-*
            QualifiedName m_attribute; // used for attribute selector
            AtomicString m_attributeCanonicalLocalName;
            AtomicString m_argument; // Used for :contains, :lang and :nth-*
            OwnPtr<CSSSelectorList> m_selectorList; // Used for :-webkit-any and :not
        
        private:
            RareData(PassRefPtr<AtomicStringImpl> value);
        };
        void createRareData();

        union DataUnion {
            DataUnion() : m_value(0) { }
            AtomicStringImpl* m_value;
            QualifiedName::QualifiedNameImpl* m_tagQName;
            RareData* m_rareData;
        } m_data;
    };

inline const QualifiedName& CSSSelector::attribute() const
{
    ASSERT(isAttributeSelector());
    ASSERT(m_hasRareData);
    return m_data.m_rareData->m_attribute;
}

inline const AtomicString& CSSSelector::attributeCanonicalLocalName() const
{
    ASSERT(isAttributeSelector());
    ASSERT(m_hasRareData);
    return m_data.m_rareData->m_attributeCanonicalLocalName;
}

inline bool CSSSelector::matchesPseudoElement() const
{
    if (m_pseudoType == PseudoUnknown)
        extractPseudoType();
    return m_match == PseudoElement;
}

inline bool CSSSelector::isUnknownPseudoElement() const
{
    return m_match == PseudoElement && m_pseudoType == PseudoUnknown;
}

inline bool CSSSelector::isCustomPseudoElement() const
{
    return m_match == PseudoElement && (m_pseudoType == PseudoUserAgentCustomElement || m_pseudoType == PseudoWebKitCustomElement);
}

inline bool CSSSelector::isSiblingSelector() const
{
    PseudoType type = pseudoType();
    return m_relation == DirectAdjacent
        || m_relation == IndirectAdjacent
        || type == PseudoEmpty
        || type == PseudoFirstChild
        || type == PseudoFirstOfType
        || type == PseudoLastChild
        || type == PseudoLastOfType
        || type == PseudoOnlyChild
        || type == PseudoOnlyOfType
        || type == PseudoNthChild
        || type == PseudoNthOfType
        || type == PseudoNthLastChild
        || type == PseudoNthLastOfType;
}

inline bool CSSSelector::isAttributeSelector() const
{
    return m_match == CSSSelector::Exact
        || m_match ==  CSSSelector::Set
        || m_match == CSSSelector::List
        || m_match == CSSSelector::Hyphen
        || m_match == CSSSelector::Contain
        || m_match == CSSSelector::Begin
        || m_match == CSSSelector::End;
}

inline void CSSSelector::setValue(const AtomicString& value)
{
    ASSERT(m_match != Tag);
    ASSERT(m_pseudoType == PseudoNotParsed);
    // Need to do ref counting manually for the union.
    if (m_hasRareData) {
        if (m_data.m_rareData->m_value)
            m_data.m_rareData->m_value->deref();
        m_data.m_rareData->m_value = value.impl();
        m_data.m_rareData->m_value->ref();
        return;
    }
    if (m_data.m_value)
        m_data.m_value->deref();
    m_data.m_value = value.impl();
    m_data.m_value->ref();
}

inline CSSSelector::CSSSelector()
    : m_relation(Descendant)
    , m_match(Unknown)
    , m_pseudoType(PseudoNotParsed)
    , m_parsedNth(false)
    , m_isLastInSelectorList(false)
    , m_isLastInTagHistory(true)
    , m_hasRareData(false)
    , m_isForPage(false)
    , m_tagIsForNamespaceRule(false)
{
}

inline CSSSelector::CSSSelector(const QualifiedName& tagQName, bool tagIsForNamespaceRule)
    : m_relation(Descendant)
    , m_match(Tag)
    , m_pseudoType(PseudoNotParsed)
    , m_parsedNth(false)
    , m_isLastInSelectorList(false)
    , m_isLastInTagHistory(true)
    , m_hasRareData(false)
    , m_isForPage(false)
    , m_tagIsForNamespaceRule(tagIsForNamespaceRule)
{
    m_data.m_tagQName = tagQName.impl();
    m_data.m_tagQName->ref();
}

inline CSSSelector::CSSSelector(const CSSSelector& o)
    : m_relation(o.m_relation)
    , m_match(o.m_match)
    , m_pseudoType(o.m_pseudoType)
    , m_parsedNth(o.m_parsedNth)
    , m_isLastInSelectorList(o.m_isLastInSelectorList)
    , m_isLastInTagHistory(o.m_isLastInTagHistory)
    , m_hasRareData(o.m_hasRareData)
    , m_isForPage(o.m_isForPage)
    , m_tagIsForNamespaceRule(o.m_tagIsForNamespaceRule)
{
    if (o.m_match == Tag) {
        m_data.m_tagQName = o.m_data.m_tagQName;
        m_data.m_tagQName->ref();
    } else if (o.m_hasRareData) {
        m_data.m_rareData = o.m_data.m_rareData;
        m_data.m_rareData->ref();
    } else if (o.m_data.m_value) {
        m_data.m_value = o.m_data.m_value;
        m_data.m_value->ref();
    }
}

inline CSSSelector::~CSSSelector()
{
    if (m_match == Tag)
        m_data.m_tagQName->deref();
    else if (m_hasRareData)
        m_data.m_rareData->deref();
    else if (m_data.m_value)
        m_data.m_value->deref();
}

inline const QualifiedName& CSSSelector::tagQName() const
{
    ASSERT(m_match == Tag);
    return *reinterpret_cast<const QualifiedName*>(&m_data.m_tagQName);
}

inline const AtomicString& CSSSelector::value() const
{
    ASSERT(m_match != Tag);
    // AtomicString is really just an AtomicStringImpl* so the cast below is safe.
    // FIXME: Perhaps call sites could be changed to accept AtomicStringImpl?
    return *reinterpret_cast<const AtomicString*>(m_hasRareData ? &m_data.m_rareData->m_value : &m_data.m_value);
}


} // namespace WebCore

#endif // CSSSelector_h
