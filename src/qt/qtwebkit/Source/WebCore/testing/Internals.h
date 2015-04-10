/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Internals_h
#define Internals_h

#include "CSSComputedStyleDeclaration.h"
#include "ContextDestructionObserver.h"
#include "ExceptionCodePlaceholder.h"
#include "NodeList.h"
#include <wtf/ArrayBuffer.h>
#include <wtf/Float32Array.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ClientRect;
class ClientRectList;
class DOMStringList;
class DOMWindow;
class Document;
class DocumentMarker;
class Element;
class Frame;
class InspectorFrontendChannelDummy;
class InternalSettings;
class MemoryInfo;
class Node;
class Page;
class Range;
class ScriptExecutionContext;
class ShadowRoot;
class WebKitPoint;
class MallocStatistics;
class SerializedScriptValue;
class TimeRanges;
class TypeConversions;

typedef int ExceptionCode;

class Internals : public RefCounted<Internals>
                , public ContextDestructionObserver {
public:
    static PassRefPtr<Internals> create(Document*);
    virtual ~Internals();

    static void resetToConsistentState(Page*);

    String elementRenderTreeAsText(Element*, ExceptionCode&);

    String address(Node*);

    bool isPreloaded(const String& url);
    bool isLoadingFromMemoryCache(const String& url);

    size_t numberOfScopedHTMLStyleChildren(const Node*, ExceptionCode&) const;
    PassRefPtr<CSSComputedStyleDeclaration> computedStyleIncludingVisitedInfo(Node*, ExceptionCode&) const;

#if ENABLE(SHADOW_DOM)
    typedef ShadowRoot ShadowRootIfShadowDOMEnabledOrNode;
#else
    typedef Node ShadowRootIfShadowDOMEnabledOrNode;
#endif
    ShadowRootIfShadowDOMEnabledOrNode* ensureShadowRoot(Element* host, ExceptionCode&);
    ShadowRootIfShadowDOMEnabledOrNode* createShadowRoot(Element* host, ExceptionCode&);
    ShadowRootIfShadowDOMEnabledOrNode* shadowRoot(Element* host, ExceptionCode&);
    String shadowRootType(const Node*, ExceptionCode&) const;
    Element* includerFor(Node*, ExceptionCode&);
    String shadowPseudoId(Element*, ExceptionCode&);
    void setShadowPseudoId(Element*, const String&, ExceptionCode&);

    // CSS Animation testing.
    unsigned numberOfActiveAnimations() const;
    bool animationsAreSuspended(Document*, ExceptionCode&) const;
    void suspendAnimations(Document*, ExceptionCode&) const;
    void resumeAnimations(Document*, ExceptionCode&) const;
    bool pauseAnimationAtTimeOnElement(const String& animationName, double pauseTime, Element*, ExceptionCode&);
    bool pauseAnimationAtTimeOnPseudoElement(const String& animationName, double pauseTime, Element*, const String& pseudoId, ExceptionCode&);

    // CSS Transition testing.
    bool pauseTransitionAtTimeOnElement(const String& propertyName, double pauseTime, Element*, ExceptionCode&);
    bool pauseTransitionAtTimeOnPseudoElement(const String& property, double pauseTime, Element*, const String& pseudoId, ExceptionCode&);

    PassRefPtr<Element> createContentElement(ExceptionCode&);
    bool isValidContentSelect(Element* insertionPoint, ExceptionCode&);
    Node* treeScopeRootNode(Node*, ExceptionCode&);
    Node* parentTreeScope(Node*, ExceptionCode&);
    bool hasSelectorForIdInShadow(Element* host, const String& idValue, ExceptionCode&);
    bool hasSelectorForClassInShadow(Element* host, const String& className, ExceptionCode&);
    bool hasSelectorForAttributeInShadow(Element* host, const String& attributeName, ExceptionCode&);
    bool hasSelectorForPseudoClassInShadow(Element* host, const String& pseudoClass, ExceptionCode&);

    bool attached(Node*, ExceptionCode&);

    // FIXME: Rename these functions if walker is prefered.
    Node* nextSiblingByWalker(Node*, ExceptionCode&);
    Node* firstChildByWalker(Node*, ExceptionCode&);
    Node* lastChildByWalker(Node*, ExceptionCode&);
    Node* nextNodeByWalker(Node*, ExceptionCode&);
    Node* previousNodeByWalker(Node*, ExceptionCode&);

    String visiblePlaceholder(Element*);
#if ENABLE(INPUT_TYPE_COLOR)
    void selectColorInColorChooser(Element*, const String& colorValue);
#endif
    Vector<String> formControlStateOfPreviousHistoryItem(ExceptionCode&);
    void setFormControlStateOfPreviousHistoryItem(const Vector<String>&, ExceptionCode&);

    PassRefPtr<ClientRect> absoluteCaretBounds(ExceptionCode&);

    PassRefPtr<ClientRect> boundingBox(Element*, ExceptionCode&);

    PassRefPtr<ClientRectList> inspectorHighlightRects(Document*, ExceptionCode&);

    unsigned markerCountForNode(Node*, const String&, ExceptionCode&);
    PassRefPtr<Range> markerRangeForNode(Node*, const String& markerType, unsigned index, ExceptionCode&);
    String markerDescriptionForNode(Node*, const String& markerType, unsigned index, ExceptionCode&);
    void addTextMatchMarker(const Range*, bool isActive);

    void setScrollViewPosition(Document*, long x, long y, ExceptionCode&);
    void setPagination(Document* document, const String& mode, int gap, ExceptionCode& ec) { setPagination(document, mode, gap, 0, ec); }
    void setPagination(Document*, const String& mode, int gap, int pageLength, ExceptionCode&);
    String configurationForViewport(Document*, float devicePixelRatio, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight, ExceptionCode&);

    bool wasLastChangeUserEdit(Element* textField, ExceptionCode&);
    bool elementShouldAutoComplete(Element* inputElement, ExceptionCode&);
    String suggestedValue(Element* inputElement, ExceptionCode&);
    void setSuggestedValue(Element* inputElement, const String&, ExceptionCode&);
    void setEditingValue(Element* inputElement, const String&, ExceptionCode&);
    void setAutofilled(Element*, bool enabled, ExceptionCode&);
    void scrollElementToRect(Element*, long x, long y, long w, long h, ExceptionCode&);

    void paintControlTints(Document*, ExceptionCode&);

    PassRefPtr<Range> rangeFromLocationAndLength(Element* scope, int rangeLocation, int rangeLength, ExceptionCode&);
    unsigned locationFromRange(Element* scope, const Range*, ExceptionCode&);
    unsigned lengthFromRange(Element* scope, const Range*, ExceptionCode&);
    String rangeAsText(const Range*, ExceptionCode&);

    void setDelegatesScrolling(bool enabled, Document*, ExceptionCode&);
#if ENABLE(TOUCH_ADJUSTMENT)
    PassRefPtr<WebKitPoint> touchPositionAdjustedToBestClickableNode(long x, long y, long width, long height, Document*, ExceptionCode&);
    Node* touchNodeAdjustedToBestClickableNode(long x, long y, long width, long height, Document*, ExceptionCode&);
    PassRefPtr<WebKitPoint> touchPositionAdjustedToBestContextMenuNode(long x, long y, long width, long height, Document*, ExceptionCode&);
    Node* touchNodeAdjustedToBestContextMenuNode(long x, long y, long width, long height, Document*, ExceptionCode&);
    PassRefPtr<ClientRect> bestZoomableAreaForTouchPoint(long x, long y, long width, long height, Document*, ExceptionCode&);
#endif

    int lastSpellCheckRequestSequence(Document*, ExceptionCode&);
    int lastSpellCheckProcessedSequence(Document*, ExceptionCode&);

    Vector<String> userPreferredLanguages() const;
    void setUserPreferredLanguages(const Vector<String>&);

    unsigned wheelEventHandlerCount(Document*, ExceptionCode&);
    unsigned touchEventHandlerCount(Document*, ExceptionCode&);
#if ENABLE(TOUCH_EVENT_TRACKING)
    PassRefPtr<ClientRectList> touchEventTargetClientRects(Document*, ExceptionCode&);
#endif

    PassRefPtr<NodeList> nodesFromRect(Document*, int x, int y, unsigned topPadding, unsigned rightPadding,
        unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping, bool allowShadowContent, bool allowChildFrameContent, ExceptionCode&) const;

    void emitInspectorDidBeginFrame();
    void emitInspectorDidCancelFrame();

    bool hasSpellingMarker(Document*, int from, int length, ExceptionCode&);
    bool hasGrammarMarker(Document*, int from, int length, ExceptionCode&);
    bool hasAutocorrectedMarker(Document*, int from, int length, ExceptionCode&);
    void setContinuousSpellCheckingEnabled(bool enabled, ExceptionCode&);
    void setAutomaticQuoteSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticLinkDetectionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticDashSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticTextReplacementEnabled(bool enabled, ExceptionCode&);
    void setAutomaticSpellingCorrectionEnabled(bool enabled, ExceptionCode&);

    bool isOverwriteModeEnabled(Document*, ExceptionCode&);
    void toggleOverwriteModeEnabled(Document*, ExceptionCode&);

    unsigned numberOfScrollableAreas(Document*, ExceptionCode&);

    bool isPageBoxVisible(Document*, int pageNumber, ExceptionCode&);

    static const char* internalsId;

    InternalSettings* settings() const;
    unsigned workerThreadCount() const;

    void setBatteryStatus(Document*, const String& eventType, bool charging, double chargingTime, double dischargingTime, double level, ExceptionCode&);

    void setNetworkInformation(Document*, const String& eventType, double bandwidth, bool metered, ExceptionCode&);

    void setDeviceProximity(Document*, const String& eventType, double value, double min, double max, ExceptionCode&);

    enum {
        // Values need to be kept in sync with Internals.idl.
        LAYER_TREE_INCLUDES_VISIBLE_RECTS = 1,
        LAYER_TREE_INCLUDES_TILE_CACHES = 2,
        LAYER_TREE_INCLUDES_REPAINT_RECTS = 4,
        LAYER_TREE_INCLUDES_PAINTING_PHASES = 8
    };
    String layerTreeAsText(Document*, unsigned flags, ExceptionCode&) const;
    String layerTreeAsText(Document*, ExceptionCode&) const;
    String repaintRectsAsText(Document*, ExceptionCode&) const;
    String scrollingStateTreeAsText(Document*, ExceptionCode&) const;
    String mainThreadScrollingReasons(Document*, ExceptionCode&) const;
    PassRefPtr<ClientRectList> nonFastScrollableRects(Document*, ExceptionCode&) const;

    void garbageCollectDocumentResources(Document*, ExceptionCode&) const;

    void allowRoundingHacks() const;

    void insertAuthorCSS(Document*, const String&) const;
    void insertUserCSS(Document*, const String&) const;

#if ENABLE(INSPECTOR)
    unsigned numberOfLiveNodes() const;
    unsigned numberOfLiveDocuments() const;
    Vector<String> consoleMessageArgumentCounts(Document*) const;
    PassRefPtr<DOMWindow> openDummyInspectorFrontend(const String& url);
    void closeDummyInspectorFrontend();
    void setInspectorResourcesDataSizeLimits(int maximumResourcesContentSize, int maximumSingleResourceContentSize, ExceptionCode&);
    void setJavaScriptProfilingEnabled(bool enabled, ExceptionCode&);
#endif

    String counterValue(Element*);

    int pageNumber(Element*, float pageWidth = 800, float pageHeight = 600);
    Vector<String> shortcutIconURLs(Document*) const;
    Vector<String> allIconURLs(Document*) const;

    int numberOfPages(float pageWidthInPixels = 800, float pageHeightInPixels = 600);
    String pageProperty(String, int, ExceptionCode& = ASSERT_NO_EXCEPTION) const;
    String pageSizeAndMarginsInPixels(int, int, int, int, int, int, int, ExceptionCode& = ASSERT_NO_EXCEPTION) const;

    void setPageScaleFactor(float scaleFactor, int x, int y, ExceptionCode&);

    void setHeaderHeight(Document*, float);
    void setFooterHeight(Document*, float);

#if ENABLE(FULLSCREEN_API)
    void webkitWillEnterFullScreenForElement(Document*, Element*);
    void webkitDidEnterFullScreenForElement(Document*, Element*);
    void webkitWillExitFullScreenForElement(Document*, Element*);
    void webkitDidExitFullScreenForElement(Document*, Element*);
#endif

    void registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme);
    void removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme);

    PassRefPtr<MallocStatistics> mallocStatistics() const;
    PassRefPtr<TypeConversions> typeConversions() const;
    PassRefPtr<MemoryInfo> memoryInfo() const;

    Vector<String> getReferencedFilePaths() const;

    void startTrackingRepaints(Document*, ExceptionCode&);
    void stopTrackingRepaints(Document*, ExceptionCode&);

    PassRefPtr<ArrayBuffer> serializeObject(PassRefPtr<SerializedScriptValue>) const;
    PassRefPtr<SerializedScriptValue> deserializeBuffer(PassRefPtr<ArrayBuffer>) const;

    void setUsesOverlayScrollbars(bool enabled);

    String getCurrentCursorInfo(Document*, ExceptionCode&);

    String markerTextForListItem(Element*, ExceptionCode&);

    void forceReload(bool endToEnd);

#if ENABLE(ENCRYPTED_MEDIA_V2)
    void initializeMockCDM();
#endif

#if ENABLE(SPEECH_SYNTHESIS)
    void enableMockSpeechSynthesizer();
#endif

    String getImageSourceURL(Element*, ExceptionCode&);

#if ENABLE(VIDEO)
    void simulateAudioInterruption(Node*);
#endif

    bool isSelectPopupVisible(Node*);

    String captionsStyleSheetOverride(ExceptionCode&);
    void setCaptionsStyleSheetOverride(const String&, ExceptionCode&);
    void setPrimaryAudioTrackLanguageOverride(const String&, ExceptionCode&);
    void setCaptionDisplayMode(const String&, ExceptionCode&);

#if ENABLE(VIDEO)
    PassRefPtr<TimeRanges> createTimeRanges(Float32Array* startTimes, Float32Array* endTimes);
    double closestTimeToTimeRanges(double time, TimeRanges*);
#endif

    PassRefPtr<ClientRect> selectionBounds(ExceptionCode&);

#if ENABLE(VIBRATION)
    bool isVibrating();
#endif

    bool isPluginUnavailabilityIndicatorObscured(Element*, ExceptionCode&);

private:
    explicit Internals(Document*);
    Document* contextDocument() const;
    Frame* frame() const;
    Vector<String> iconURLs(Document*, int iconTypesMask) const;

    DocumentMarker* markerAt(Node*, const String& markerType, unsigned index, ExceptionCode&);
#if ENABLE(INSPECTOR)
    RefPtr<DOMWindow> m_frontendWindow;
    OwnPtr<InspectorFrontendChannelDummy> m_frontendChannel;
#endif
};

} // namespace WebCore

#endif
