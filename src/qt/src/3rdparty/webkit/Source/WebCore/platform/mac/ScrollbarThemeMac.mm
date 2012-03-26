/*
 * Copyright (C) 2008, 2011 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScrollbarThemeMac.h"

#include "ImageBuffer.h"
#include "LocalCurrentGraphicsContext.h"
#include "PlatformMouseEvent.h"
#include "ScrollAnimatorMac.h"
#include "ScrollView.h"
#include "WebCoreSystemInterface.h"
#include <Carbon/Carbon.h>
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UnusedParam.h>

// FIXME: There are repainting problems due to Aqua scroll bar buttons' visual overflow.

using namespace std;
using namespace WebCore;

namespace WebCore {

#if USE(WK_SCROLLBAR_PAINTER)
typedef HashMap<Scrollbar*, RetainPtr<WKScrollbarPainterRef> > ScrollbarPainterMap;
#else
typedef HashSet<Scrollbar*> ScrollbarPainterMap;
#endif

static ScrollbarPainterMap* scrollbarMap()
{
    static ScrollbarPainterMap* map = new ScrollbarPainterMap;
    return map;
}

}

@interface ScrollbarPrefsObserver : NSObject
{
}

+ (void)registerAsObserver;
+ (void)appearancePrefsChanged:(NSNotification*)theNotification;
+ (void)behaviorPrefsChanged:(NSNotification*)theNotification;

@end

@implementation ScrollbarPrefsObserver

+ (void)appearancePrefsChanged:(NSNotification*)unusedNotification
{
    UNUSED_PARAM(unusedNotification);

    static_cast<ScrollbarThemeMac*>(ScrollbarTheme::nativeTheme())->preferencesChanged();
    if (scrollbarMap()->isEmpty())
        return;
    ScrollbarPainterMap::iterator end = scrollbarMap()->end();
    for (ScrollbarPainterMap::iterator it = scrollbarMap()->begin(); it != end; ++it) {
#if USE(WK_SCROLLBAR_PAINTER)
        it->first->styleChanged();
        it->first->invalidate();
#else
        (*it)->styleChanged();
        (*it)->invalidate();
#endif
    }
}

+ (void)behaviorPrefsChanged:(NSNotification*)unusedNotification
{
    UNUSED_PARAM(unusedNotification);

    static_cast<ScrollbarThemeMac*>(ScrollbarTheme::nativeTheme())->preferencesChanged();
}

+ (void)registerAsObserver
{
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(appearancePrefsChanged:) name:@"AppleAquaScrollBarVariantChanged" object:nil suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(behaviorPrefsChanged:) name:@"AppleNoRedisplayAppearancePreferenceChanged" object:nil suspensionBehavior:NSNotificationSuspensionBehaviorCoalesce];
}

@end

namespace WebCore {

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    DEFINE_STATIC_LOCAL(ScrollbarThemeMac, theme, ());
    return &theme;
}

// FIXME: Get these numbers from CoreUI.
static int cRealButtonLength[] = { 28, 21 };
static int cButtonHitInset[] = { 3, 2 };
// cRealButtonLength - cButtonInset
static int cButtonLength[] = { 14, 10 };
#if !USE(WK_SCROLLBAR_PAINTER)
static int cScrollbarThickness[] = { 15, 11 };
static int cButtonInset[] = { 14, 11 };
static int cThumbMinLength[] = { 26, 20 };
#endif

static int cOuterButtonLength[] = { 16, 14 }; // The outer button in a double button pair is a bit bigger.
static int cOuterButtonOverlap = 2;

static float gInitialButtonDelay = 0.5f;
static float gAutoscrollButtonDelay = 0.05f;
static bool gJumpOnTrackClick = false;

#if USE(WK_SCROLLBAR_PAINTER)
static ScrollbarButtonsPlacement gButtonPlacement = ScrollbarButtonsNone;
#else
static ScrollbarButtonsPlacement gButtonPlacement = ScrollbarButtonsDoubleEnd;
#endif

static void updateArrowPlacement()
{
#if USE(WK_SCROLLBAR_PAINTER)
    return;
#endif
    NSString *buttonPlacement = [[NSUserDefaults standardUserDefaults] objectForKey:@"AppleScrollBarVariant"];
    if ([buttonPlacement isEqualToString:@"Single"])
        gButtonPlacement = ScrollbarButtonsSingle;
    else if ([buttonPlacement isEqualToString:@"DoubleMin"])
        gButtonPlacement = ScrollbarButtonsDoubleStart;
    else if ([buttonPlacement isEqualToString:@"DoubleBoth"])
        gButtonPlacement = ScrollbarButtonsDoubleBoth;
    else {

        gButtonPlacement = ScrollbarButtonsDoubleEnd;
    }
}

void ScrollbarThemeMac::registerScrollbar(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    bool isHorizontal = scrollbar->orientation() == HorizontalScrollbar;
    WKScrollbarPainterRef scrollbarPainter = wkMakeScrollbarPainter(scrollbar->controlSize(), isHorizontal);
    scrollbarMap()->add(scrollbar, scrollbarPainter);
#else
    scrollbarMap()->add(scrollbar);
#endif
}

void ScrollbarThemeMac::unregisterScrollbar(Scrollbar* scrollbar)
{
    scrollbarMap()->remove(scrollbar);
}

#if USE(WK_SCROLLBAR_PAINTER)
void ScrollbarThemeMac::setNewPainterForScrollbar(Scrollbar* scrollbar, WKScrollbarPainterRef newPainter)
{
    scrollbarMap()->set(scrollbar, newPainter);
}

WKScrollbarPainterRef ScrollbarThemeMac::painterForScrollbar(Scrollbar* scrollbar)
{
    return scrollbarMap()->get(scrollbar).get();
}
#endif

ScrollbarThemeMac::ScrollbarThemeMac()
{
    static bool initialized;
    if (!initialized) {
        initialized = true;
        [ScrollbarPrefsObserver registerAsObserver];
        preferencesChanged();
    }
}

ScrollbarThemeMac::~ScrollbarThemeMac()
{
}

void ScrollbarThemeMac::preferencesChanged()
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults synchronize];
    updateArrowPlacement();
    gInitialButtonDelay = [defaults floatForKey:@"NSScrollerButtonDelay"];
    gAutoscrollButtonDelay = [defaults floatForKey:@"NSScrollerButtonPeriod"];
    gJumpOnTrackClick = [defaults boolForKey:@"AppleScrollerPagingBehavior"];
}

int ScrollbarThemeMac::scrollbarThickness(ScrollbarControlSize controlSize)
{
#if USE(WK_SCROLLBAR_PAINTER)
    return wkScrollbarThickness(controlSize);
#else
    return cScrollbarThickness[controlSize];
#endif
}

bool ScrollbarThemeMac::usesOverlayScrollbars() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    return wkScrollbarPainterUsesOverlayScrollers();
#else
    return false;
#endif
}

double ScrollbarThemeMac::initialAutoscrollTimerDelay()
{
    return gInitialButtonDelay;
}

double ScrollbarThemeMac::autoscrollTimerDelay()
{
    return gAutoscrollButtonDelay;
}
    
ScrollbarButtonsPlacement ScrollbarThemeMac::buttonsPlacement() const
{
    return gButtonPlacement;
}

bool ScrollbarThemeMac::hasButtons(Scrollbar* scrollbar)
{
    return scrollbar->enabled() && gButtonPlacement != ScrollbarButtonsNone
             && (scrollbar->orientation() == HorizontalScrollbar
             ? scrollbar->width()
             : scrollbar->height()) >= 2 * (cRealButtonLength[scrollbar->controlSize()] - cButtonHitInset[scrollbar->controlSize()]);
}

bool ScrollbarThemeMac::hasThumb(Scrollbar* scrollbar)
{
    int minLengthForThumb;
#if USE(WK_SCROLLBAR_PAINTER)
    minLengthForThumb = wkScrollbarMinimumTotalLengthNeededForThumb(scrollbarMap()->get(scrollbar).get());
#else
    minLengthForThumb = 2 * cButtonInset[scrollbar->controlSize()] + cThumbMinLength[scrollbar->controlSize()] + 1;
#endif
    return scrollbar->enabled() && (scrollbar->orientation() == HorizontalScrollbar ? 
             scrollbar->width() : 
             scrollbar->height()) >= minLengthForThumb;
}

static IntRect buttonRepaintRect(const IntRect& buttonRect, ScrollbarOrientation orientation, ScrollbarControlSize controlSize, bool start)
{
    ASSERT(gButtonPlacement != ScrollbarButtonsNone);

    IntRect paintRect(buttonRect);
    if (orientation == HorizontalScrollbar) {
        paintRect.setWidth(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setX(buttonRect.x() - (cRealButtonLength[controlSize] - buttonRect.width()));
    } else {
        paintRect.setHeight(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setY(buttonRect.y() - (cRealButtonLength[controlSize] - buttonRect.height()));
    }

    return paintRect;
}

IntRect ScrollbarThemeMac::backButtonRect(Scrollbar* scrollbar, ScrollbarPart part, bool painting)
{
    IntRect result;
    
    if (part == BackButtonStartPart && (buttonsPlacement() == ScrollbarButtonsNone || buttonsPlacement() == ScrollbarButtonsDoubleEnd))
        return result;
    
    if (part == BackButtonEndPart && (buttonsPlacement() == ScrollbarButtonsNone || buttonsPlacement() == ScrollbarButtonsDoubleStart || buttonsPlacement() == ScrollbarButtonsSingle))
        return result;
        
    int thickness = scrollbarThickness(scrollbar->controlSize());
    bool outerButton = part == BackButtonStartPart && (buttonsPlacement() == ScrollbarButtonsDoubleStart || buttonsPlacement() == ScrollbarButtonsDoubleBoth);
    if (outerButton) {
        if (scrollbar->orientation() == HorizontalScrollbar)
            result = IntRect(scrollbar->x(), scrollbar->y(), cOuterButtonLength[scrollbar->controlSize()] + painting ? cOuterButtonOverlap : 0, thickness);
        else
            result = IntRect(scrollbar->x(), scrollbar->y(), thickness, cOuterButtonLength[scrollbar->controlSize()] + painting ? cOuterButtonOverlap : 0);
        return result;
    }
    
    // Our repaint rect is slightly larger, since we are a button that is adjacent to the track.
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int start = part == BackButtonStartPart ? scrollbar->x() : scrollbar->x() + scrollbar->width() - cOuterButtonLength[scrollbar->controlSize()] - cButtonLength[scrollbar->controlSize()];
        result = IntRect(start, scrollbar->y(), cButtonLength[scrollbar->controlSize()], thickness);
    } else {
        int start = part == BackButtonStartPart ? scrollbar->y() : scrollbar->y() + scrollbar->height() - cOuterButtonLength[scrollbar->controlSize()] - cButtonLength[scrollbar->controlSize()];
        result = IntRect(scrollbar->x(), start, thickness, cButtonLength[scrollbar->controlSize()]);
    }
    
    if (painting)
        return buttonRepaintRect(result, scrollbar->orientation(), scrollbar->controlSize(), part == BackButtonStartPart);
    return result;
}

IntRect ScrollbarThemeMac::forwardButtonRect(Scrollbar* scrollbar, ScrollbarPart part, bool painting)
{
    IntRect result;
    
    if (part == ForwardButtonEndPart && (buttonsPlacement() == ScrollbarButtonsNone || buttonsPlacement() == ScrollbarButtonsDoubleStart))
        return result;
    
    if (part == ForwardButtonStartPart && (buttonsPlacement() == ScrollbarButtonsNone || buttonsPlacement() == ScrollbarButtonsDoubleEnd || buttonsPlacement() == ScrollbarButtonsSingle))
        return result;
        
    int thickness = scrollbarThickness(scrollbar->controlSize());
    int outerButtonLength = cOuterButtonLength[scrollbar->controlSize()];
    int buttonLength = cButtonLength[scrollbar->controlSize()];
    
    bool outerButton = part == ForwardButtonEndPart && (buttonsPlacement() == ScrollbarButtonsDoubleEnd || buttonsPlacement() == ScrollbarButtonsDoubleBoth);
    if (outerButton) {
        if (scrollbar->orientation() == HorizontalScrollbar) {
            result = IntRect(scrollbar->x() + scrollbar->width() - outerButtonLength, scrollbar->y(), outerButtonLength, thickness);
            if (painting)
                result.inflateX(cOuterButtonOverlap);
        } else {
            result = IntRect(scrollbar->x(), scrollbar->y() + scrollbar->height() - outerButtonLength, thickness, outerButtonLength);
            if (painting)
                result.inflateY(cOuterButtonOverlap);
        }
        return result;
    }
    
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int start = part == ForwardButtonEndPart ? scrollbar->x() + scrollbar->width() - buttonLength : scrollbar->x() + outerButtonLength;
        result = IntRect(start, scrollbar->y(), buttonLength, thickness);
    } else {
        int start = part == ForwardButtonEndPart ? scrollbar->y() + scrollbar->height() - buttonLength : scrollbar->y() + outerButtonLength;
        result = IntRect(scrollbar->x(), start, thickness, buttonLength);
    }
    if (painting)
        return buttonRepaintRect(result, scrollbar->orientation(), scrollbar->controlSize(), part == ForwardButtonStartPart);
    return result;
}

IntRect ScrollbarThemeMac::trackRect(Scrollbar* scrollbar, bool painting)
{
    if (painting || !hasButtons(scrollbar))
        return scrollbar->frameRect();
    
    IntRect result;
    int thickness = scrollbarThickness(scrollbar->controlSize());
    int startWidth = 0;
    int endWidth = 0;
    int outerButtonLength = cOuterButtonLength[scrollbar->controlSize()];
    int buttonLength = cButtonLength[scrollbar->controlSize()];
    int doubleButtonLength = outerButtonLength + buttonLength;
    switch (buttonsPlacement()) {
        case ScrollbarButtonsSingle:
            startWidth = buttonLength;
            endWidth = buttonLength;
            break;
        case ScrollbarButtonsDoubleStart:
            startWidth = doubleButtonLength;
            break;
        case ScrollbarButtonsDoubleEnd:
            endWidth = doubleButtonLength;
            break;
        case ScrollbarButtonsDoubleBoth:
            startWidth = doubleButtonLength;
            endWidth = doubleButtonLength;
            break;
        default:
            break;
    }
    
    int totalWidth = startWidth + endWidth;
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(scrollbar->x() + startWidth, scrollbar->y(), scrollbar->width() - totalWidth, thickness);
    return IntRect(scrollbar->x(), scrollbar->y() + startWidth, thickness, scrollbar->height() - totalWidth);
}

int ScrollbarThemeMac::minimumThumbLength(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    return wkScrollbarMinimumThumbLength(scrollbarMap()->get(scrollbar).get());
#else
    return cThumbMinLength[scrollbar->controlSize()];
#endif
}

bool ScrollbarThemeMac::shouldCenterOnThumb(Scrollbar*, const PlatformMouseEvent& evt)
{
    if (evt.button() != LeftButton)
        return false;
    if (gJumpOnTrackClick)
        return !evt.altKey();
    return evt.altKey();
}

bool ScrollbarThemeMac::shouldDragDocumentInsteadOfThumb(Scrollbar*, const PlatformMouseEvent& event)
{
    return event.altKey();
}

static int scrollbarPartToHIPressedState(ScrollbarPart part)
{
    switch (part) {
        case BackButtonStartPart:
            return kThemeTopOutsideArrowPressed;
        case BackButtonEndPart:
            return kThemeTopOutsideArrowPressed; // This does not make much sense.  For some reason the outside constant is required.
        case ForwardButtonStartPart:
            return kThemeTopInsideArrowPressed;
        case ForwardButtonEndPart:
            return kThemeBottomOutsideArrowPressed;
        case ThumbPart:
            return kThemeThumbPressed;
        default:
            return 0;
    }
}

#if USE(WK_SCROLLBAR_PAINTER)
static inline wkScrollerKnobStyle toScrollbarPainterKnobStyle(ScrollbarOverlayStyle style)
{
    switch (style) {
    case ScrollbarOverlayStyleDark:
        return wkScrollerKnobStyleDark;
    case ScrollbarOverlayStyleLight:
        return wkScrollerKnobStyleLight;
    default:
        return wkScrollerKnobStyleDefault;
    }
}
#endif

bool ScrollbarThemeMac::paint(Scrollbar* scrollbar, GraphicsContext* context, const IntRect& damageRect)
{
#if USE(WK_SCROLLBAR_PAINTER)
    float value = 0;
    float overhang = 0;

    if (scrollbar->currentPos() < 0) {
        // Scrolled past the top.
        value = 0;
        overhang = -scrollbar->currentPos();
    } else if (scrollbar->visibleSize() + scrollbar->currentPos() > scrollbar->totalSize()) {
        // Scrolled past the bottom.
        value = 1;
        overhang = scrollbar->currentPos() + scrollbar->visibleSize() - scrollbar->totalSize();
    } else {
        // Within the bounds of the scrollable area.
        int maximum = scrollbar->maximum();
        if (maximum > 0)
            value = scrollbar->currentPos() / maximum;
        else
            value = 0;
    }
    
    ScrollAnimatorMac* scrollAnimator = static_cast<ScrollAnimatorMac*>(scrollbar->scrollableArea()->scrollAnimator());
    scrollAnimator->setIsDrawingIntoLayer(context->isCALayerContext());

#if USE(WK_SCROLLBAR_PAINTER)
    wkSetScrollbarPainterKnobStyle(painterForScrollbar(scrollbar), toScrollbarPainterKnobStyle(scrollbar->scrollableArea()->recommendedScrollbarOverlayStyle()));
#endif
    
    GraphicsContextStateSaver stateSaver(*context);
    context->clip(damageRect);
    context->translate(scrollbar->frameRect().x(), scrollbar->frameRect().y());
    LocalCurrentGraphicsContext localContext(context);
    wkScrollbarPainterPaint(scrollbarMap()->get(scrollbar).get(),
                            scrollbar->enabled(),
                            value,
                            (static_cast<CGFloat>(scrollbar->visibleSize()) - overhang) / scrollbar->totalSize(),
                            scrollbar->frameRect());

    scrollAnimator->setIsDrawingIntoLayer(false);
    return true;
#endif

    HIThemeTrackDrawInfo trackInfo;
    trackInfo.version = 0;
    trackInfo.kind = scrollbar->controlSize() == RegularScrollbar ? kThemeMediumScrollBar : kThemeSmallScrollBar;
    trackInfo.bounds = scrollbar->frameRect();

    float maximum = 0.0f;
    float position = 0.0f;
    if (scrollbar->currentPos() < 0) {
        // Scrolled past the top.
        maximum = (scrollbar->totalSize() - scrollbar->currentPos()) - scrollbar->visibleSize();
        position = 0;
    } else if (scrollbar->visibleSize() + scrollbar->currentPos() > scrollbar->totalSize()) {
        // Scrolled past the bottom.
        maximum = scrollbar->currentPos();
        position = maximum;
    } else {
        // Within the bounds of the scrollable area.
        maximum = scrollbar->maximum();
        position = scrollbar->currentPos();
    }

    trackInfo.min = 0;
    trackInfo.max = static_cast<int>(maximum);
    trackInfo.value = static_cast<int>(position);

    trackInfo.trackInfo.scrollbar.viewsize = scrollbar->visibleSize();
    trackInfo.attributes = 0;
    if (scrollbar->orientation() == HorizontalScrollbar)
        trackInfo.attributes |= kThemeTrackHorizontal;

    if (!scrollbar->enabled())
        trackInfo.enableState = kThemeTrackDisabled;
    else
        trackInfo.enableState = scrollbar->scrollableArea()->isActive() ? kThemeTrackActive : kThemeTrackInactive;

    if (hasThumb(scrollbar))
        trackInfo.attributes |= kThemeTrackShowThumb;
    else if (!hasButtons(scrollbar))
        trackInfo.enableState = kThemeTrackNothingToScroll;
    trackInfo.trackInfo.scrollbar.pressState = scrollbarPartToHIPressedState(scrollbar->pressedPart());
    
    // The Aqua scrollbar is buggy when rotated and scaled.  We will just draw into a bitmap if we detect a scale or rotation.
    const AffineTransform& currentCTM = context->getCTM();
    bool canDrawDirectly = currentCTM.isIdentityOrTranslationOrFlipped();
    if (canDrawDirectly)
        HIThemeDrawTrack(&trackInfo, 0, context->platformContext(), kHIThemeOrientationNormal);
    else {
        trackInfo.bounds = IntRect(IntPoint(), scrollbar->frameRect().size());
        
        IntRect bufferRect(scrollbar->frameRect());
        bufferRect.intersect(damageRect);
        
        OwnPtr<ImageBuffer> imageBuffer = ImageBuffer::create(bufferRect.size());
        if (!imageBuffer)
            return true;

        imageBuffer->context()->translate(scrollbar->frameRect().x() - bufferRect.x(), scrollbar->frameRect().y() - bufferRect.y());
        HIThemeDrawTrack(&trackInfo, 0, imageBuffer->context()->platformContext(), kHIThemeOrientationNormal);
        context->drawImageBuffer(imageBuffer.get(), ColorSpaceDeviceRGB, bufferRect.location());
    }

    return true;
}

}

