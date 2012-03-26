/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 *
 */

#ifndef CSSStyleSelector_h
#define CSSStyleSelector_h

#include "CSSFontSelector.h"
#include "CSSRule.h"
#include "LinkHash.h"
#include "MediaQueryExp.h"
#include "RenderStyle.h"
#include <wtf/BloomFilter.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CSSMutableStyleDeclaration;
class CSSPageRule;
class CSSPrimitiveValue;
class CSSProperty;
class CSSFontFace;
class CSSFontFaceRule;
class CSSImageValue;
class CSSRuleList;
class CSSSelector;
class CSSStyleApplyProperty;
class CSSStyleRule;
class CSSStyleSheet;
class CSSValue;
class ContainerNode;
class Document;
class Element;
class Frame;
class FrameView;
class KURL;
class KeyframeList;
class KeyframeValue;
class MediaQueryEvaluator;
class Node;
class RuleData;
class RuleSet;
class Settings;
class StyleImage;
class StyleSheet;
class StyleSheetList;
class StyledElement;
class WebKitCSSKeyframeRule;
class WebKitCSSKeyframesRule;

class MediaQueryResult {
    WTF_MAKE_NONCOPYABLE(MediaQueryResult); WTF_MAKE_FAST_ALLOCATED;
public:
    MediaQueryResult(const MediaQueryExp& expr, bool result)
        : m_expression(expr)
        , m_result(result)
    {
    }

    MediaQueryExp m_expression;
    bool m_result;
};

    // This class selects a RenderStyle for a given element based on a collection of stylesheets.
    class CSSStyleSelector {
        friend class CSSStyleApplyProperty;
        WTF_MAKE_NONCOPYABLE(CSSStyleSelector); WTF_MAKE_FAST_ALLOCATED;
    public:
        CSSStyleSelector(Document*, StyleSheetList* authorSheets, CSSStyleSheet* mappedElementSheet,
                         CSSStyleSheet* pageUserSheet, const Vector<RefPtr<CSSStyleSheet> >* pageGroupUserSheets,
                         bool strictParsing, bool matchAuthorAndUserStyles);
        ~CSSStyleSelector();
        
        // Using these during tree walk will allow style selector to optimize child and descendant selector lookups.
        void pushParent(Element* parent);
        void popParent(Element* parent);


        PassRefPtr<RenderStyle> styleForElement(Element* e, RenderStyle* parentStyle = 0, bool allowSharing = true, bool resolveForRootDefault = false, bool matchVisitedPseudoClass = false);
        
        void keyframeStylesForAnimation(Element*, const RenderStyle*, KeyframeList& list);

        PassRefPtr<RenderStyle> pseudoStyleForElement(PseudoId pseudo, Element* e, RenderStyle* parentStyle = 0, bool matchVisitedPseudoClass = false);

        PassRefPtr<RenderStyle> styleForPage(int pageIndex);

        static PassRefPtr<RenderStyle> styleForDocument(Document*);

        RenderStyle* style() const { return m_style.get(); }
        RenderStyle* parentStyle() const { return m_parentStyle; }
        RenderStyle* rootElementStyle() const { return m_rootElementStyle; }
        Element* element() const { return m_element; }

    private:
        void initForStyleResolve(Element*, RenderStyle* parentStyle = 0, PseudoId = NOPSEUDO);
        void initElement(Element*);
        RenderStyle* locateSharedStyle();
        bool matchesSiblingRules();
        Node* locateCousinList(Element* parent, unsigned& visitedNodeCount) const;
        Node* findSiblingForStyleSharing(Node*, unsigned& count) const;
        bool canShareStyleWithElement(Node*) const;
        
        void pushParentStackFrame(Element* parent);
        void popParentStackFrame();

        PassRefPtr<RenderStyle> styleForKeyframe(const RenderStyle*, const WebKitCSSKeyframeRule*, KeyframeValue&);

    public:
        // These methods will give back the set of rules that matched for a given element (or a pseudo-element).
        PassRefPtr<CSSRuleList> styleRulesForElement(Element*, bool authorOnly, bool includeEmptyRules = false, CSSRuleFilter filter = AllCSSRules);
        PassRefPtr<CSSRuleList> pseudoStyleRulesForElement(Element*, PseudoId, bool authorOnly, bool includeEmptyRules = false, CSSRuleFilter filter = AllCSSRules);

        // Given a CSS keyword in the range (xx-small to -webkit-xxx-large), this function will return
        // the correct font size scaled relative to the user's default (medium).
        static float fontSizeForKeyword(Document*, int keyword, bool shouldUseFixedDefaultSize);

        // Given a font size in pixel, this function will return legacy font size between 1 and 7.
        static int legacyFontSize(Document*, int pixelFontSize, bool shouldUseFixedDefaultSize);

    private:

        // When the CSS keyword "larger" is used, this function will attempt to match within the keyword
        // table, and failing that, will simply multiply by 1.2.
        float largerFontSize(float size, bool quirksMode) const;

        // Like the previous function, but for the keyword "smaller".
        float smallerFontSize(float size, bool quirksMode) const;

    public:
        void setStyle(PassRefPtr<RenderStyle> s) { m_style = s; } // Used by the document when setting up its root style.

        void applyPropertyToStyle(int id, CSSValue*, RenderStyle*);

    private:
        void setFontSize(FontDescription&, float size);
        static float getComputedSizeFromSpecifiedSize(Document*, RenderStyle*, bool isAbsoluteSize, float specifiedSize, bool useSVGZoomRules);
    public:
        Color getColorFromPrimitiveValue(CSSPrimitiveValue*) const;

        bool hasSelectorForAttribute(const AtomicString&) const;
 
        CSSFontSelector* fontSelector() const { return m_fontSelector.get(); }

        void addViewportDependentMediaQueryResult(const MediaQueryExp*, bool result);

        bool affectedByViewportChange() const;

        void allVisitedStateChanged() { m_checker.allVisitedStateChanged(); }
        void visitedStateChanged(LinkHash visitedHash) { m_checker.visitedStateChanged(visitedHash); }

        void addKeyframeStyle(PassRefPtr<WebKitCSSKeyframesRule> rule);
        void addPageStyle(PassRefPtr<CSSPageRule>);

        bool usesSiblingRules() const { return m_features.siblingRules; }
        bool usesFirstLineRules() const { return m_features.usesFirstLineRules; }
        bool usesBeforeAfterRules() const { return m_features.usesBeforeAfterRules; }
        bool usesLinkRules() const { return m_features.usesLinkRules; }

        static bool createTransformOperations(CSSValue* inValue, RenderStyle* inStyle, RenderStyle* rootStyle, TransformOperations& outOperations);

        struct Features {
            Features();
            ~Features();
            HashSet<AtomicStringImpl*> idsInRules;
            OwnPtr<RuleSet> siblingRules;
            bool usesFirstLineRules;
            bool usesBeforeAfterRules;
            bool usesLinkRules;
        };

    private:
        enum SelectorMatch { SelectorMatches, SelectorFailsLocally, SelectorFailsCompletely };

        // This function fixes up the default font size if it detects that the current generic font family has changed. -dwh
        void checkForGenericFamilyChange(RenderStyle*, RenderStyle* parentStyle);
        void checkForZoomChange(RenderStyle*, RenderStyle* parentStyle);
        void checkForTextSizeAdjust();

        void adjustRenderStyle(RenderStyle* styleToAdjust, RenderStyle* parentStyle, Element*);

        void addMatchedRule(const RuleData* rule) { m_matchedRules.append(rule); }
        void addMatchedDeclaration(CSSMutableStyleDeclaration* decl);

        void matchRules(RuleSet*, int& firstRuleIndex, int& lastRuleIndex, bool includeEmptyRules);
        void matchRulesForList(const Vector<RuleData>*, int& firstRuleIndex, int& lastRuleIndex, bool includeEmptyRules);
        bool fastRejectSelector(const RuleData&) const;
        void sortMatchedRules();
        
        bool checkSelector(const RuleData&);

        template <bool applyFirst>
        void applyDeclarations(bool important, int startIndex, int endIndex);

        void matchPageRules(RuleSet*, bool isLeftPage, bool isFirstPage, const String& pageName);
        void matchPageRulesForList(const Vector<RuleData>*, bool isLeftPage, bool isFirstPage, const String& pageName);
        bool isLeftPage(int pageIndex) const;
        bool isRightPage(int pageIndex) const { return !isLeftPage(pageIndex); }
        bool isFirstPage(int pageIndex) const;
        String pageName(int pageIndex) const;
        
        OwnPtr<RuleSet> m_authorStyle;
        OwnPtr<RuleSet> m_userStyle;

        Features m_features;

        struct ParentStackFrame {
            ParentStackFrame() {}
            ParentStackFrame(Element* element) : element(element) {}
            Element* element;
            Vector<unsigned, 4> identifierHashes;
        };
        Vector<ParentStackFrame> m_parentStack;
        
        // With 100 unique strings in the filter, 2^12 slot table has false positive rate of ~0.2%.
        static const unsigned bloomFilterKeyBits = 12;
        OwnPtr<BloomFilter<bloomFilterKeyBits> > m_ancestorIdentifierFilter;

        bool m_hasUAAppearance;
        BorderData m_borderData;
        FillLayer m_backgroundData;
        Color m_backgroundColor;

        typedef HashMap<AtomicStringImpl*, RefPtr<WebKitCSSKeyframesRule> > KeyframesRuleMap;
        KeyframesRuleMap m_keyframesRuleMap;

    public:
        static RenderStyle* styleNotYetAvailable() { return s_styleNotYetAvailable; }

        class SelectorChecker {
            WTF_MAKE_NONCOPYABLE(SelectorChecker);
        public:
            SelectorChecker(Document*, bool strictParsing);

            bool checkSelector(CSSSelector*, Element*) const;
            SelectorMatch checkSelector(CSSSelector*, Element*, HashSet<AtomicStringImpl*>* selectorAttrs, PseudoId& dynamicPseudo, bool isSubSelector, bool encounteredLink, RenderStyle* = 0, RenderStyle* elementParentStyle = 0) const;
            bool checkOneSelector(CSSSelector*, Element*, HashSet<AtomicStringImpl*>* selectorAttrs, PseudoId& dynamicPseudo, bool isSubSelector, bool encounteredLink, RenderStyle*, RenderStyle* elementParentStyle) const;
            bool checkScrollbarPseudoClass(CSSSelector*, PseudoId& dynamicPseudo) const;
            static bool fastCheckSelector(const CSSSelector*, const Element*);

            EInsideLink determineLinkState(Element* element) const;
            EInsideLink determineLinkStateSlowCase(Element* element) const;
            void allVisitedStateChanged();
            void visitedStateChanged(LinkHash visitedHash);

            Document* m_document;
            bool m_strictParsing;
            bool m_collectRulesOnly;
            bool m_sameOriginOnly;
            PseudoId m_pseudoStyle;
            bool m_documentIsHTML;
            mutable bool m_hasUnknownPseudoElements;
            mutable bool m_matchVisitedPseudoClass;
            mutable HashSet<LinkHash, LinkHashHash> m_linksCheckedForVisitedState;
        };

    private:
        static RenderStyle* s_styleNotYetAvailable;

        void matchUARules(int& firstUARule, int& lastUARule);
        void updateFont();
        void cacheBorderAndBackground();

        void mapFillAttachment(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillClip(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillComposite(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillOrigin(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillImage(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillRepeatX(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillRepeatY(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillSize(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillXPosition(CSSPropertyID, FillLayer*, CSSValue*);
        void mapFillYPosition(CSSPropertyID, FillLayer*, CSSValue*);

        void mapAnimationDelay(Animation*, CSSValue*);
        void mapAnimationDirection(Animation*, CSSValue*);
        void mapAnimationDuration(Animation*, CSSValue*);
        void mapAnimationFillMode(Animation*, CSSValue*);
        void mapAnimationIterationCount(Animation*, CSSValue*);
        void mapAnimationName(Animation*, CSSValue*);
        void mapAnimationPlayState(Animation*, CSSValue*);
        void mapAnimationProperty(Animation*, CSSValue*);
        void mapAnimationTimingFunction(Animation*, CSSValue*);

        void mapNinePieceImage(CSSPropertyID, CSSValue*, NinePieceImage&);

        void applyProperty(int id, CSSValue*);
        void applyPageSizeProperty(CSSValue*);
        bool pageSizeFromName(CSSPrimitiveValue*, CSSPrimitiveValue*, Length& width, Length& height);
        Length mmLength(double mm) const;
        Length inchLength(double inch) const;
#if ENABLE(SVG)
        void applySVGProperty(int id, CSSValue*);
#endif

        void loadPendingImages();
        
        StyleImage* styleImage(CSSPropertyID, CSSValue* value);
        StyleImage* cachedOrPendingFromValue(CSSPropertyID property, CSSImageValue* value);

        // We collect the set of decls that match in |m_matchedDecls|.  We then walk the
        // set of matched decls four times, once for those properties that others depend on (like font-size),
        // and then a second time for all the remaining properties.  We then do the same two passes
        // for any !important rules.
        Vector<CSSMutableStyleDeclaration*, 64> m_matchedDecls;

        // A buffer used to hold the set of matched rules for an element, and a temporary buffer used for
        // merge sorting.
        Vector<const RuleData*, 32> m_matchedRules;

        RefPtr<CSSRuleList> m_ruleList;
        
        HashSet<int> m_pendingImageProperties; // Hash of CSSPropertyIDs

        OwnPtr<MediaQueryEvaluator> m_medium;
        RefPtr<RenderStyle> m_rootDefaultStyle;

        PseudoId m_dynamicPseudo;

        SelectorChecker m_checker;

        RefPtr<RenderStyle> m_style;
        RenderStyle* m_parentStyle;
        RenderStyle* m_rootElementStyle;
        Element* m_element;
        StyledElement* m_styledElement;
        EInsideLink m_elementLinkState;
        ContainerNode* m_parentNode;
        CSSValue* m_lineHeightValue;
        bool m_fontDirty;
        bool m_matchAuthorAndUserStyles;
        
        RefPtr<CSSFontSelector> m_fontSelector;
        HashSet<AtomicStringImpl*> m_selectorAttrs;
        Vector<CSSMutableStyleDeclaration*> m_additionalAttributeStyleDecls;
        Vector<MediaQueryResult*> m_viewportDependentMediaQueryResults;

        const CSSStyleApplyProperty& m_applyProperty;
    };

} // namespace WebCore

#endif // CSSStyleSelector_h
