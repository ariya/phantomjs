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

#ifndef StyleResolver_h
#define StyleResolver_h

#include "CSSToStyleMap.h"
#include "CSSValueList.h"
#include "DocumentRuleSets.h"
#include "InspectorCSSOMWrappers.h"
#include "LinkHash.h"
#include "MediaQueryExp.h"
#include "RenderStyle.h"
#include "RuleFeature.h"
#include "RuleSet.h"
#include "RuntimeEnabledFeatures.h"
#include "ScrollTypes.h"
#include "SelectorChecker.h"
#include "SelectorFilter.h"
#include "StyleInheritedData.h"
#include "StyleScopeResolver.h"
#include "ViewportStyleResolver.h"
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
#include "WebKitCSSSVGDocumentValue.h"
#endif
#if ENABLE(CSS_SHADERS)
#include "CustomFilterConstants.h"
#endif
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

enum ESmartMinimumForFontSize { DoNotUseSmartMinimumForFontSize, UseSmartMinimumForFontFize };

class CSSCursorImageValue;
class CSSFontSelector;
class CSSFontFace;
class CSSFontFaceRule;
class CSSImageGeneratorValue;
class CSSImageSetValue;
class CSSImageValue;
class CSSPageRule;
class CSSPrimitiveValue;
class CSSProperty;
class CSSSelector;
class CSSStyleSheet;
class CSSValue;
class ContainerNode;
class CustomFilterOperation;
class CustomFilterParameter;
class CustomFilterParameterList;
class CustomFilterProgram;
struct CustomFilterProgramMixSettings;
class Document;
class DeprecatedStyleBuilder;
class Element;
class Frame;
class FrameView;
class KURL;
class KeyframeList;
class KeyframeValue;
class MediaQueryEvaluator;
class Node;
class RenderRegion;
class RenderScrollbar;
class RuleData;
class RuleSet;
class Settings;
class StyleCustomFilterProgramCache;
class StyleScopeResolver;
class StyleImage;
class StyleKeyframe;
class StylePendingImage;
class StylePropertySet;
class StyleRule;
#if ENABLE(SHADOW_DOM)
class StyleRuleHost;
#endif
class StyleRuleKeyframes;
class StyleRulePage;
class StyleRuleRegion;
class StyleShader;
class StyleSheet;
class StyleSheetList;
class StyledElement;
class ViewportStyleResolver;
class WebKitCSSFilterValue;
class WebKitCSSShaderValue;
class WebKitCSSSVGDocumentValue;

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

enum StyleSharingBehavior {
    AllowStyleSharing,
    DisallowStyleSharing,
};

// MatchOnlyUserAgentRules is used in media queries, where relative units
// are interpreted according to the document root element style, and styled only
// from the User Agent Stylesheet rules.

enum RuleMatchingBehavior {
    MatchAllRules,
    MatchAllRulesExcludingSMIL,
    MatchOnlyUserAgentRules,
};

class PseudoStyleRequest {
public:
    PseudoStyleRequest(PseudoId pseudoId, RenderScrollbar* scrollbar = 0, ScrollbarPart scrollbarPart = NoPart)
        : pseudoId(pseudoId)
        , scrollbarPart(scrollbarPart)
        , scrollbar(scrollbar)
    {
    }

    PseudoId pseudoId;
    ScrollbarPart scrollbarPart;
    RenderScrollbar* scrollbar;
};

class MatchRequest {
public:
    MatchRequest(RuleSet* ruleSet, bool includeEmptyRules = false, const ContainerNode* scope = 0)
        : ruleSet(ruleSet)
        , includeEmptyRules(includeEmptyRules)
        , scope(scope) { }
    const RuleSet* ruleSet;
    const bool includeEmptyRules;
    const ContainerNode* scope;
};

// This class selects a RenderStyle for a given element based on a collection of stylesheets.
class StyleResolver {
    WTF_MAKE_NONCOPYABLE(StyleResolver); WTF_MAKE_FAST_ALLOCATED;
public:
    StyleResolver(Document*, bool matchAuthorAndUserStyles);
    ~StyleResolver();

    // Using these during tree walk will allow style selector to optimize child and descendant selector lookups.
    void pushParentElement(Element*);
    void popParentElement(Element*);
    void pushParentShadowRoot(const ShadowRoot*);
    void popParentShadowRoot(const ShadowRoot*);
#if ENABLE(SHADOW_DOM)
    void addHostRule(StyleRuleHost* rule, bool hasDocumentSecurityOrigin, const ContainerNode* scope) { ensureScopeResolver()->addHostRule(rule, hasDocumentSecurityOrigin, scope); }
#endif

    PassRefPtr<RenderStyle> styleForElement(Element*, RenderStyle* parentStyle = 0, StyleSharingBehavior = AllowStyleSharing,
        RuleMatchingBehavior = MatchAllRules, RenderRegion* regionForStyling = 0);

    void keyframeStylesForAnimation(Element*, const RenderStyle*, KeyframeList&);

    PassRefPtr<RenderStyle> pseudoStyleForElement(Element*, const PseudoStyleRequest&, RenderStyle* parentStyle);

    PassRefPtr<RenderStyle> styleForPage(int pageIndex);
    PassRefPtr<RenderStyle> defaultStyleForElement();
    PassRefPtr<RenderStyle> styleForText(Text*);

    static PassRefPtr<RenderStyle> styleForDocument(Document*, CSSFontSelector* = 0);

    RenderStyle* style() const { return m_state.style(); }
    RenderStyle* parentStyle() const { return m_state.parentStyle(); }
    RenderStyle* rootElementStyle() const { return m_state.rootElementStyle(); }
    Element* element() { return m_state.element(); }
    Document* document() { return m_document; }
    StyleScopeResolver* scopeResolver() const { return m_scopeResolver.get(); }
    bool hasParentNode() const { return m_state.parentNode(); }

    // FIXME: It could be better to call m_ruleSets.appendAuthorStyleSheets() directly after we factor StyleRsolver further.
    // https://bugs.webkit.org/show_bug.cgi?id=108890
    void appendAuthorStyleSheets(unsigned firstNew, const Vector<RefPtr<CSSStyleSheet> >&);

    DocumentRuleSets& ruleSets() { return m_ruleSets; }
    const DocumentRuleSets& ruleSets() const { return m_ruleSets; }
    SelectorFilter& selectorFilter() { return m_selectorFilter; }

#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)
    StyleScopeResolver* ensureScopeResolver()
    {
#if ENABLE(STYLE_SCOPED)
#if ENABLE(SHADOW_DOM)
        ASSERT(RuntimeEnabledFeatures::shadowDOMEnabled() || RuntimeEnabledFeatures::styleScopedEnabled());
#else
        ASSERT(RuntimeEnabledFeatures::styleScopedEnabled());
#endif
#else
        ASSERT(RuntimeEnabledFeatures::shadowDOMEnabled());
#endif
        if (!m_scopeResolver)
            m_scopeResolver = adoptPtr(new StyleScopeResolver());
        return m_scopeResolver.get();
    }
#endif

private:
    void initElement(Element*);
    RenderStyle* locateSharedStyle();
    bool styleSharingCandidateMatchesRuleSet(RuleSet*);
    bool styleSharingCandidateMatchesHostRules();
    Node* locateCousinList(Element* parent, unsigned& visitedNodeCount) const;
    StyledElement* findSiblingForStyleSharing(Node*, unsigned& count) const;
    bool canShareStyleWithElement(StyledElement*) const;

    PassRefPtr<RenderStyle> styleForKeyframe(const RenderStyle*, const StyleKeyframe*, KeyframeValue&);

public:
    // These methods will give back the set of rules that matched for a given element (or a pseudo-element).
    enum CSSRuleFilter {
        UAAndUserCSSRules   = 1 << 1,
        AuthorCSSRules      = 1 << 2,
        EmptyCSSRules       = 1 << 3,
        CrossOriginCSSRules = 1 << 4,
        AllButEmptyCSSRules = UAAndUserCSSRules | AuthorCSSRules | CrossOriginCSSRules,
        AllCSSRules         = AllButEmptyCSSRules | EmptyCSSRules,
    };
    Vector<RefPtr<StyleRuleBase> > styleRulesForElement(Element*, unsigned rulesToInclude = AllButEmptyCSSRules);
    Vector<RefPtr<StyleRuleBase> > pseudoStyleRulesForElement(Element*, PseudoId, unsigned rulesToInclude = AllButEmptyCSSRules);

    // Given a CSS keyword in the range (xx-small to -webkit-xxx-large), this function will return
    // the correct font size scaled relative to the user's default (medium).
    static float fontSizeForKeyword(Document*, int keyword, bool shouldUseFixedDefaultSize);

    // Given a font size in pixel, this function will return legacy font size between 1 and 7.
    static int legacyFontSize(Document*, int pixelFontSize, bool shouldUseFixedDefaultSize);

public:
    void applyPropertyToStyle(CSSPropertyID, CSSValue*, RenderStyle*);

    void applyPropertyToCurrentStyle(CSSPropertyID, CSSValue*);

    void updateFont();
    void initializeFontStyle(Settings*);

    static float getComputedSizeFromSpecifiedSize(Document*, float zoomFactor, bool isAbsoluteSize, float specifiedSize, ESmartMinimumForFontSize = UseSmartMinimumForFontFize);

    void setFontSize(FontDescription&, float size);

private:
    static float getComputedSizeFromSpecifiedSize(Document*, RenderStyle*, bool isAbsoluteSize, float specifiedSize, bool useSVGZoomRules);

public:
    bool useSVGZoomRules();

    static bool colorFromPrimitiveValueIsDerivedFromElement(CSSPrimitiveValue*);
    Color colorFromPrimitiveValue(CSSPrimitiveValue*, bool forVisitedLink = false) const;

    bool hasSelectorForId(const AtomicString&) const;
    bool hasSelectorForClass(const AtomicString&) const;
    bool hasSelectorForAttribute(const AtomicString&) const;

    CSSFontSelector* fontSelector() const { return m_fontSelector.get(); }
#if ENABLE(CSS_DEVICE_ADAPTATION)
    ViewportStyleResolver* viewportStyleResolver() { return m_viewportStyleResolver.get(); }
#endif

    void addViewportDependentMediaQueryResult(const MediaQueryExp*, bool result);
    bool hasViewportDependentMediaQueries() const { return !m_viewportDependentMediaQueryResults.isEmpty(); }
    bool affectedByViewportChange() const;

    void addKeyframeStyle(PassRefPtr<StyleRuleKeyframes>);

    bool checkRegionStyle(Element* regionElement);

    bool usesSiblingRules() const { return !m_ruleSets.features().siblingRules.isEmpty(); }
    bool usesFirstLineRules() const { return m_ruleSets.features().usesFirstLineRules; }
    bool usesBeforeAfterRules() const { return m_ruleSets.features().usesBeforeAfterRules; }
    
    void invalidateMatchedPropertiesCache();

#if ENABLE(CSS_FILTERS)
    bool createFilterOperations(CSSValue* inValue, RenderStyle* inStyle, RenderStyle* rootStyle, FilterOperations& outOperations);
#if ENABLE(CSS_SHADERS)
    StyleShader* styleShader(CSSValue*);
    StyleShader* cachedOrPendingStyleShaderFromValue(WebKitCSSShaderValue*);
    bool parseCustomFilterParameterList(CSSValue*, CustomFilterParameterList&);
    PassRefPtr<CustomFilterParameter> parseCustomFilterParameter(const String& name, CSSValue*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterColorParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterArrayParameter(const String& name, CSSValueList*, bool);
    PassRefPtr<CustomFilterParameter> parseCustomFilterNumberParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterTransformParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterOperation> createCustomFilterOperationWithAtRuleReferenceSyntax(WebKitCSSFilterValue*);
    PassRefPtr<CustomFilterOperation> createCustomFilterOperationWithInlineSyntax(WebKitCSSFilterValue*);
    PassRefPtr<CustomFilterOperation> createCustomFilterOperation(WebKitCSSFilterValue*);
    void loadPendingShaders();
    PassRefPtr<CustomFilterProgram> lookupCustomFilterProgram(WebKitCSSShaderValue* vertexShader, WebKitCSSShaderValue* fragmentShader, 
        CustomFilterProgramType, const CustomFilterProgramMixSettings&, CustomFilterMeshType);
#endif
#if ENABLE(SVG)
    void loadPendingSVGDocuments();
#endif
#endif // ENABLE(CSS_FILTERS)

    void loadPendingResources();

    struct RuleRange {
        RuleRange(int& firstRuleIndex, int& lastRuleIndex): firstRuleIndex(firstRuleIndex), lastRuleIndex(lastRuleIndex) { }
        int& firstRuleIndex;
        int& lastRuleIndex;
    };

    struct MatchRanges {
        MatchRanges() : firstUARule(-1), lastUARule(-1), firstAuthorRule(-1), lastAuthorRule(-1), firstUserRule(-1), lastUserRule(-1) { }
        int firstUARule;
        int lastUARule;
        int firstAuthorRule;
        int lastAuthorRule;
        int firstUserRule;
        int lastUserRule;
        RuleRange UARuleRange() { return RuleRange(firstUARule, lastUARule); }
        RuleRange authorRuleRange() { return RuleRange(firstAuthorRule, lastAuthorRule); }
        RuleRange userRuleRange() { return RuleRange(firstUserRule, lastUserRule); }
    };

    struct MatchedProperties {
        MatchedProperties();
        ~MatchedProperties();
        
        RefPtr<StylePropertySet> properties;
        union {
            struct {
                unsigned linkMatchType : 2;
                unsigned whitelistType : 2;
            };
            // Used to make sure all memory is zero-initialized since we compute the hash over the bytes of this object.
            void* possiblyPaddedMember;
        };
    };

    struct MatchResult {
        MatchResult() : isCacheable(true) { }
        Vector<MatchedProperties, 64> matchedProperties;
        Vector<StyleRule*, 64> matchedRules;
        MatchRanges ranges;
        bool isCacheable;

        void addMatchedProperties(const StylePropertySet* properties, StyleRule* = 0, unsigned linkMatchType = SelectorChecker::MatchAll, PropertyWhitelistType = PropertyWhitelistNone);
    };

private:
    // This function fixes up the default font size if it detects that the current generic font family has changed. -dwh
    void checkForGenericFamilyChange(RenderStyle*, RenderStyle* parentStyle);
    void checkForZoomChange(RenderStyle*, RenderStyle* parentStyle);

    void adjustRenderStyle(RenderStyle* styleToAdjust, RenderStyle* parentStyle, Element*);

    bool fastRejectSelector(const RuleData&) const;

    void applyMatchedProperties(const MatchResult&, const Element*);

    enum StyleApplicationPass {
#if ENABLE(CSS_VARIABLES)
        VariableDefinitions,
#endif
        HighPriorityProperties,
        LowPriorityProperties
    };
    template <StyleApplicationPass pass>
    void applyMatchedProperties(const MatchResult&, bool important, int startIndex, int endIndex, bool inheritedOnly);
    template <StyleApplicationPass pass>
    void applyProperties(const StylePropertySet* properties, StyleRule*, bool isImportant, bool inheritedOnly, PropertyWhitelistType = PropertyWhitelistNone);
#if ENABLE(CSS_VARIABLES)
    void resolveVariables(CSSPropertyID, CSSValue*, Vector<std::pair<CSSPropertyID, String> >& knownExpressions);
#endif
    static bool isValidRegionStyleProperty(CSSPropertyID);
#if ENABLE(VIDEO_TRACK)
    static bool isValidCueStyleProperty(CSSPropertyID);
#endif
    void matchPageRules(MatchResult&, RuleSet*, bool isLeftPage, bool isFirstPage, const String& pageName);
    void matchPageRulesForList(Vector<StyleRulePage*>& matchedRules, const Vector<StyleRulePage*>&, bool isLeftPage, bool isFirstPage, const String& pageName);
    Settings* documentSettings() { return m_document->settings(); }

    bool isLeftPage(int pageIndex) const;
    bool isRightPage(int pageIndex) const { return !isLeftPage(pageIndex); }
    bool isFirstPage(int pageIndex) const;
    String pageName(int pageIndex) const;

    DocumentRuleSets m_ruleSets;

    typedef HashMap<AtomicStringImpl*, RefPtr<StyleRuleKeyframes> > KeyframesRuleMap;
    KeyframesRuleMap m_keyframesRuleMap;

public:
    typedef HashMap<CSSPropertyID, RefPtr<CSSValue> > PendingImagePropertyMap;
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
    typedef HashMap<FilterOperation*, RefPtr<WebKitCSSSVGDocumentValue> > PendingSVGDocumentMap;
#endif

    class State {
        WTF_MAKE_NONCOPYABLE(State);
    public:
        State()
        : m_element(0)
        , m_styledElement(0)
        , m_parentNode(0)
        , m_parentStyle(0)
        , m_rootElementStyle(0)
        , m_regionForStyling(0)
        , m_elementLinkState(NotInsideLink)
        , m_distributedToInsertionPoint(false)
        , m_elementAffectedByClassRules(false)
        , m_applyPropertyToRegularStyle(true)
        , m_applyPropertyToVisitedLinkStyle(false)
#if ENABLE(CSS_SHADERS)
        , m_hasPendingShaders(false)
#endif
        , m_lineHeightValue(0)
        , m_fontDirty(false)
        , m_hasUAAppearance(false)
        , m_backgroundData(BackgroundFillLayer) { }

    public:
        void initElement(Element*);
        void initForStyleResolve(Document*, Element*, RenderStyle* parentStyle = 0, RenderRegion* regionForStyling = 0);
        void clear();

        Document* document() const { return m_element->document(); }
        Element* element() const { return m_element; }
        StyledElement* styledElement() const { return m_styledElement; }
        void setStyle(PassRefPtr<RenderStyle> style) { m_style = style; }
        RenderStyle* style() const { return m_style.get(); }
        PassRefPtr<RenderStyle> takeStyle() { return m_style.release(); }

        const ContainerNode* parentNode() const { return m_parentNode; }
        void setParentStyle(PassRefPtr<RenderStyle> parentStyle) { m_parentStyle = parentStyle; }
        RenderStyle* parentStyle() const { return m_parentStyle.get(); }
        RenderStyle* rootElementStyle() const { return m_rootElementStyle; }

        const RenderRegion* regionForStyling() const { return m_regionForStyling; }
        EInsideLink elementLinkState() const { return m_elementLinkState; }
        bool distributedToInsertionPoint() const { return m_distributedToInsertionPoint; }
        void setElementAffectedByClassRules(bool isAffected) { m_elementAffectedByClassRules = isAffected; }
        bool elementAffectedByClassRules() const { return m_elementAffectedByClassRules; }

        void setApplyPropertyToRegularStyle(bool isApply) { m_applyPropertyToRegularStyle = isApply; }
        void setApplyPropertyToVisitedLinkStyle(bool isApply) { m_applyPropertyToVisitedLinkStyle = isApply; }
        bool applyPropertyToRegularStyle() const { return m_applyPropertyToRegularStyle; }
        bool applyPropertyToVisitedLinkStyle() const { return m_applyPropertyToVisitedLinkStyle; }
        PendingImagePropertyMap& pendingImageProperties() { return m_pendingImageProperties; }
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
        PendingSVGDocumentMap& pendingSVGDocuments() { return m_pendingSVGDocuments; }
#endif
#if ENABLE(CSS_SHADERS)
        void setHasPendingShaders(bool hasPendingShaders) { m_hasPendingShaders = hasPendingShaders; }
        bool hasPendingShaders() const { return m_hasPendingShaders; }
#endif

        void setLineHeightValue(CSSValue* value) { m_lineHeightValue = value; }
        CSSValue* lineHeightValue() { return m_lineHeightValue; }
        void setFontDirty(bool isDirty) { m_fontDirty = isDirty; }
        bool fontDirty() const { return m_fontDirty; }

        void cacheBorderAndBackground();
        bool hasUAAppearance() const { return m_hasUAAppearance; }
        BorderData borderData() const { return m_borderData; }
        FillLayer backgroundData() const { return m_backgroundData; }
        Color backgroundColor() const { return m_backgroundColor; }

        const FontDescription& fontDescription() { return m_style->fontDescription(); }
        const FontDescription& parentFontDescription() { return m_parentStyle->fontDescription(); }
        void setFontDescription(const FontDescription& fontDescription) { m_fontDirty |= m_style->setFontDescription(fontDescription); }
        void setZoom(float f) { m_fontDirty |= m_style->setZoom(f); }
        void setEffectiveZoom(float f) { m_fontDirty |= m_style->setEffectiveZoom(f); }
        void setWritingMode(WritingMode writingMode) { m_fontDirty |= m_style->setWritingMode(writingMode); }
        void setTextOrientation(TextOrientation textOrientation) { m_fontDirty |= m_style->setTextOrientation(textOrientation); }

        bool useSVGZoomRules() const { return m_element && m_element->isSVGElement(); }

    private:
        // FIXME(bug 108563): to make it easier to review, these member
        // variables are public. However we should add methods to access
        // these variables.
        Element* m_element;
        RefPtr<RenderStyle> m_style;
        StyledElement* m_styledElement;
        ContainerNode* m_parentNode;
        RefPtr<RenderStyle> m_parentStyle;
        RenderStyle* m_rootElementStyle;

        // Required to ASSERT in applyProperties.
        RenderRegion* m_regionForStyling;
        
        EInsideLink m_elementLinkState;

        bool m_distributedToInsertionPoint;

        bool m_elementAffectedByClassRules;

        bool m_applyPropertyToRegularStyle;
        bool m_applyPropertyToVisitedLinkStyle;

        PendingImagePropertyMap m_pendingImageProperties;
#if ENABLE(CSS_SHADERS)
        bool m_hasPendingShaders;
#endif
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
        PendingSVGDocumentMap m_pendingSVGDocuments;
#endif
        CSSValue* m_lineHeightValue;
        bool m_fontDirty;

        bool m_hasUAAppearance;
        BorderData m_borderData;
        FillLayer m_backgroundData;
        Color m_backgroundColor;
    };

    static RenderStyle* styleNotYetAvailable() { return s_styleNotYetAvailable; }

    PassRefPtr<StyleImage> styleImage(CSSPropertyID, CSSValue*);
    PassRefPtr<StyleImage> cachedOrPendingFromValue(CSSPropertyID, CSSImageValue*);
    PassRefPtr<StyleImage> generatedOrPendingFromValue(CSSPropertyID, CSSImageGeneratorValue*);
#if ENABLE(CSS_IMAGE_SET)
    PassRefPtr<StyleImage> setOrPendingFromValue(CSSPropertyID, CSSImageSetValue*);
#endif
    PassRefPtr<StyleImage> cursorOrPendingFromValue(CSSPropertyID, CSSCursorImageValue*);

    bool applyPropertyToRegularStyle() const { return m_state.applyPropertyToRegularStyle(); }
    bool applyPropertyToVisitedLinkStyle() const { return m_state.applyPropertyToVisitedLinkStyle(); }

    static Length convertToIntLength(const CSSPrimitiveValue*, const RenderStyle*, const RenderStyle* rootStyle, double multiplier = 1);
    static Length convertToFloatLength(const CSSPrimitiveValue*, const RenderStyle*, const RenderStyle* rootStyle, double multiplier = 1);

    CSSToStyleMap* styleMap() { return &m_styleMap; }
    InspectorCSSOMWrappers& inspectorCSSOMWrappers() { return m_inspectorCSSOMWrappers; }
    const FontDescription& fontDescription() { return m_state.fontDescription(); }
    const FontDescription& parentFontDescription() { return m_state.parentFontDescription(); }
    void setFontDescription(const FontDescription& fontDescription) { m_state.setFontDescription(fontDescription); }
    void setZoom(float f) { m_state.setZoom(f); }
    void setEffectiveZoom(float f) { m_state.setEffectiveZoom(f); }
    void setWritingMode(WritingMode writingMode) { m_state.setWritingMode(writingMode); }
    void setTextOrientation(TextOrientation textOrientation) { m_state.setTextOrientation(textOrientation); }

private:
    static RenderStyle* s_styleNotYetAvailable;

    void cacheBorderAndBackground();

private:
    bool canShareStyleWithControl(StyledElement*) const;

    void applyProperty(CSSPropertyID, CSSValue*);

#if ENABLE(SVG)
    void applySVGProperty(CSSPropertyID, CSSValue*);
#endif

    PassRefPtr<StyleImage> loadPendingImage(StylePendingImage*);
    void loadPendingImages();
#if ENABLE(CSS_SHAPES)
    void loadPendingShapeImage(ShapeValue*);
#endif

    static unsigned computeMatchedPropertiesHash(const MatchedProperties*, unsigned size);
    struct MatchedPropertiesCacheItem {
        Vector<MatchedProperties> matchedProperties;
        MatchRanges ranges;
        RefPtr<RenderStyle> renderStyle;
        RefPtr<RenderStyle> parentRenderStyle;
    };
    const MatchedPropertiesCacheItem* findFromMatchedPropertiesCache(unsigned hash, const MatchResult&);
    void addToMatchedPropertiesCache(const RenderStyle*, const RenderStyle* parentStyle, unsigned hash, const MatchResult&);

    // Every N additions to the matched declaration cache trigger a sweep where entries holding
    // the last reference to a style declaration are garbage collected.
    void sweepMatchedPropertiesCache(Timer<StyleResolver>*);

    bool classNamesAffectedByRules(const SpaceSplitString&) const;
    bool sharingCandidateHasIdenticalStyleAffectingAttributes(StyledElement*) const;

    unsigned m_matchedPropertiesCacheAdditionsSinceLastSweep;

    typedef HashMap<unsigned, MatchedPropertiesCacheItem> MatchedPropertiesCache;
    MatchedPropertiesCache m_matchedPropertiesCache;

    Timer<StyleResolver> m_matchedPropertiesCacheSweepTimer;

    OwnPtr<MediaQueryEvaluator> m_medium;
    RefPtr<RenderStyle> m_rootDefaultStyle;

    Document* m_document;
    SelectorFilter m_selectorFilter;

    bool m_matchAuthorAndUserStyles;

    RefPtr<CSSFontSelector> m_fontSelector;
    Vector<OwnPtr<MediaQueryResult> > m_viewportDependentMediaQueryResults;

#if ENABLE(CSS_DEVICE_ADAPTATION)
    RefPtr<ViewportStyleResolver> m_viewportStyleResolver;
#endif

    const DeprecatedStyleBuilder& m_deprecatedStyleBuilder;

    OwnPtr<StyleScopeResolver> m_scopeResolver;
    CSSToStyleMap m_styleMap;
    InspectorCSSOMWrappers m_inspectorCSSOMWrappers;

    State m_state;

#if ENABLE(CSS_SHADERS)
    OwnPtr<StyleCustomFilterProgramCache> m_customFilterProgramCache;
#endif

    friend class DeprecatedStyleBuilder;
    friend bool operator==(const MatchedProperties&, const MatchedProperties&);
    friend bool operator!=(const MatchedProperties&, const MatchedProperties&);
    friend bool operator==(const MatchRanges&, const MatchRanges&);
    friend bool operator!=(const MatchRanges&, const MatchRanges&);
};

inline bool StyleResolver::hasSelectorForAttribute(const AtomicString &attributeName) const
{
    ASSERT(!attributeName.isEmpty());
    return m_ruleSets.features().attrsInRules.contains(attributeName.impl());
}

inline bool StyleResolver::hasSelectorForClass(const AtomicString& classValue) const
{
    ASSERT(!classValue.isEmpty());
    return m_ruleSets.features().classesInRules.contains(classValue.impl());
}

inline bool StyleResolver::hasSelectorForId(const AtomicString& idValue) const
{
    ASSERT(!idValue.isEmpty());
    return m_ruleSets.features().idsInRules.contains(idValue.impl());
}

inline bool checkRegionSelector(const CSSSelector* regionSelector, Element* regionElement)
{
    if (!regionSelector || !regionElement)
        return false;

    SelectorChecker selectorChecker(regionElement->document(), SelectorChecker::QueryingRules);
    for (const CSSSelector* s = regionSelector; s; s = CSSSelectorList::next(s)) {
        SelectorChecker::SelectorCheckingContext selectorCheckingContext(s, regionElement, SelectorChecker::VisitedMatchDisabled);
        PseudoId ignoreDynamicPseudo = NOPSEUDO;
        if (selectorChecker.match(selectorCheckingContext, ignoreDynamicPseudo) == SelectorChecker::SelectorMatches)
            return true;
    }
    return false;
}

} // namespace WebCore

#endif // StyleResolver_h
