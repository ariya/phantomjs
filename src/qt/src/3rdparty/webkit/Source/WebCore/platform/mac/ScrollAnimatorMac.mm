/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SMOOTH_SCROLLING)

#include "ScrollAnimatorMac.h"

#include "FloatPoint.h"
#include "PlatformGestureEvent.h"
#include "PlatformWheelEvent.h"
#include "ScrollView.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include "ScrollbarThemeMac.h"
#include "WebCoreSystemInterface.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/UnusedParam.h>

using namespace WebCore;
using namespace std;

@interface NSObject (ScrollAnimationHelperDetails)
- (id)initWithDelegate:(id)delegate;
- (void)_stopRun;
- (BOOL)_isAnimating;
- (NSPoint)targetOrigin;
@end

@interface ScrollAnimationHelperDelegate : NSObject
{
    WebCore::ScrollAnimatorMac* _animator;
}
- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator;
@end

static NSSize abs(NSSize size)
{
    NSSize finalSize = size;
    if (finalSize.width < 0)
        finalSize.width = -finalSize.width;
    if (finalSize.height < 0)
        finalSize.height = -finalSize.height;
    return finalSize;    
}

@implementation ScrollAnimationHelperDelegate

- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator
{
    self = [super init];
    if (!self)
        return nil;

    _animator = scrollAnimator;
    return self;
}

- (void)scrollAnimatorDestroyed
{
    _animator = 0;
}

- (NSRect)bounds
{
    if (!_animator)
        return NSZeroRect;

    WebCore::FloatPoint currentPosition = _animator->currentPosition();
    return NSMakeRect(currentPosition.x(), currentPosition.y(), 0, 0);
}

- (void)_immediateScrollToPoint:(NSPoint)newPosition
{
    if (!_animator)
        return;
    _animator->immediateScrollToPoint(newPosition);
}

- (NSPoint)_pixelAlignProposedScrollPosition:(NSPoint)newOrigin
{
    return newOrigin;
}

- (NSSize)convertSizeToBase:(NSSize)size
{
    return abs(size);
}

- (NSSize)convertSizeFromBase:(NSSize)size
{
    return abs(size);
}

- (NSSize)convertSizeToBacking:(NSSize)size
{
    return abs(size);
}

- (NSSize)convertSizeFromBacking:(NSSize)size
{
    return abs(size);
}

- (id)superview
{
    return nil;
}

- (id)documentView
{
    return nil;
}

- (id)window
{
    return nil;
}

- (void)_recursiveRecomputeToolTips
{
}

@end

#if USE(WK_SCROLLBAR_PAINTER)

@interface ScrollbarPainterControllerDelegate : NSObject
{
    WebCore::ScrollAnimatorMac* _animator;
}
- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator;
@end

@implementation ScrollbarPainterControllerDelegate

- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator
{
    self = [super init];
    if (!self)
        return nil;
    
    _animator = scrollAnimator;
    return self;
}

- (void)scrollAnimatorDestroyed
{
    _animator = 0;
}

- (NSRect)contentAreaRectForScrollerImpPair:(id)scrollerImpPair
{
    UNUSED_PARAM(scrollerImpPair);
    if (!_animator)
        return NSZeroRect;

    WebCore::IntSize contentsSize = _animator->scrollableArea()->contentsSize();
    return NSMakeRect(0, 0, contentsSize.width(), contentsSize.height());
}

- (BOOL)inLiveResizeForScrollerImpPair:(id)scrollerImpPair
{
    UNUSED_PARAM(scrollerImpPair);
    if (!_animator)
        return NO;

    return _animator->scrollableArea()->inLiveResize();
}

- (NSPoint)mouseLocationInContentAreaForScrollerImpPair:(id)scrollerImpPair
{
    UNUSED_PARAM(scrollerImpPair);
    if (!_animator)
        return NSZeroPoint;

    return _animator->scrollableArea()->currentMousePosition();
}

- (NSPoint)scrollerImpPair:(id)scrollerImpPair convertContentPoint:(NSPoint)pointInContentArea toScrollerImp:(id)scrollerImp
{
    UNUSED_PARAM(scrollerImpPair);
    if (!_animator)
        return NSZeroPoint;

    WebCore::Scrollbar* scrollbar = 0;
    if (wkScrollbarPainterIsHorizontal((WKScrollbarPainterRef)scrollerImp))
        scrollbar = _animator->scrollableArea()->horizontalScrollbar();
    else 
        scrollbar = _animator->scrollableArea()->verticalScrollbar();

    // It is possible to have a null scrollbar here since it is possible for this delegate
    // method to be called between the moment when a scrollbar has been set to 0 and the
    // moment when its destructor has been called. We should probably de-couple some
    // of the clean-up work in ScrollbarThemeMac::unregisterScrollbar() to avoid this
    // issue.
    if (!scrollbar)
        return WebCore::IntPoint();
    
    return scrollbar->convertFromContainingView(WebCore::IntPoint(pointInContentArea));
}

- (void)scrollerImpPair:(id)scrollerImpPair setContentAreaNeedsDisplayInRect:(NSRect)rect
{
    UNUSED_PARAM(scrollerImpPair);
    UNUSED_PARAM(rect);
}

- (void)scrollerImpPair:(id)scrollerImpPair updateScrollerStyleForNewRecommendedScrollerStyle:(NSScrollerStyle)newRecommendedScrollerStyle
{
    if (!_animator)
        return;

    WKScrollbarPainterControllerRef painterController = (WKScrollbarPainterControllerRef)scrollerImpPair;
    WebCore::ScrollbarThemeMac* macTheme = (WebCore::ScrollbarThemeMac*)WebCore::ScrollbarTheme::nativeTheme();

    WKScrollbarPainterRef oldVerticalPainter = wkVerticalScrollbarPainterForController(painterController);
    if (oldVerticalPainter) {
        WebCore::Scrollbar* verticalScrollbar = _animator->scrollableArea()->verticalScrollbar();
        WKScrollbarPainterRef newVerticalPainter = wkMakeScrollbarReplacementPainter(oldVerticalPainter,
                                                                                     newRecommendedScrollerStyle,
                                                                                     verticalScrollbar->controlSize(),
                                                                                     false);
        macTheme->setNewPainterForScrollbar(verticalScrollbar, newVerticalPainter);
        wkSetPainterForPainterController(painterController, newVerticalPainter, false);

        // The different scrollbar styles have different thicknesses, so we must re-set the 
        // frameRect to the new thickness, and the re-layout below will ensure the position
        // and length are properly updated.
        int thickness = macTheme->scrollbarThickness(verticalScrollbar->controlSize());
        verticalScrollbar->setFrameRect(WebCore::IntRect(0, 0, thickness, thickness));
    }

    WKScrollbarPainterRef oldHorizontalPainter = wkHorizontalScrollbarPainterForController(painterController);
    if (oldHorizontalPainter) {
        WebCore::Scrollbar* horizontalScrollbar = _animator->scrollableArea()->horizontalScrollbar();
        WKScrollbarPainterRef newHorizontalPainter = wkMakeScrollbarReplacementPainter(oldHorizontalPainter,
                                                                                       newRecommendedScrollerStyle,
                                                                                       horizontalScrollbar->controlSize(),
                                                                                       true);
        macTheme->setNewPainterForScrollbar(horizontalScrollbar, newHorizontalPainter);
        wkSetPainterForPainterController(painterController, newHorizontalPainter, true);

        // The different scrollbar styles have different thicknesses, so we must re-set the 
        // frameRect to the new thickness, and the re-layout below will ensure the position
        // and length are properly updated.
        int thickness = macTheme->scrollbarThickness(horizontalScrollbar->controlSize());
        horizontalScrollbar->setFrameRect(WebCore::IntRect(0, 0, thickness, thickness));
    }

    wkSetScrollbarPainterControllerStyle(painterController, newRecommendedScrollerStyle);

    // The different scrollbar styles affect layout, so we must re-layout everything.
    _animator->scrollableArea()->scrollbarStyleChanged();
}

@end

@interface ScrollbarPartAnimation : NSAnimation
{
    RetainPtr<WKScrollbarPainterRef> _scrollerPainter;
    WebCore::ScrollbarPart _part;
    WebCore::ScrollAnimatorMac* _animator;
    CGFloat _initialAlpha;
    CGFloat _newAlpha;
}
- (id)initWithScrollbarPainter:(WKScrollbarPainterRef)scrollerPainter part:(WebCore::ScrollbarPart)part scrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator animateAlphaTo:(CGFloat)newAlpha duration:(NSTimeInterval)duration;
@end

@implementation ScrollbarPartAnimation

- (id)initWithScrollbarPainter:(WKScrollbarPainterRef)scrollerPainter part:(WebCore::ScrollbarPart)part scrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator animateAlphaTo:(CGFloat)newAlpha duration:(NSTimeInterval)duration
{
    self = [super initWithDuration:duration animationCurve:NSAnimationEaseInOut];
    if (!self)
        return nil;
    
    _scrollerPainter = scrollerPainter;
    _part = part;
    _animator = scrollAnimator;
    _initialAlpha = _part == WebCore::ThumbPart ? wkScrollbarPainterKnobAlpha(_scrollerPainter.get()) : wkScrollbarPainterTrackAlpha(_scrollerPainter.get());
    _newAlpha = newAlpha;
    
    return self;    
}

- (void)setCurrentProgress:(NSAnimationProgress)progress
{
    [super setCurrentProgress:progress];

    if (!_animator)
        return;

    CGFloat currentAlpha;
    if (_initialAlpha > _newAlpha)
        currentAlpha = 1 - progress;
    else
        currentAlpha = progress;
    
    if (_part == WebCore::ThumbPart)
        wkSetScrollbarPainterKnobAlpha(_scrollerPainter.get(), currentAlpha);
    else
        wkSetScrollbarPainterTrackAlpha(_scrollerPainter.get(), currentAlpha);

    // Invalidate the scrollbars so that they paint the animation
    if (WebCore::Scrollbar* verticalScrollbar = _animator->scrollableArea()->verticalScrollbar())
        verticalScrollbar->invalidateRect(WebCore::IntRect(0, 0, verticalScrollbar->width(), verticalScrollbar->height()));
    if (WebCore::Scrollbar* horizontalScrollbar = _animator->scrollableArea()->horizontalScrollbar())
        horizontalScrollbar->invalidateRect(WebCore::IntRect(0, 0, horizontalScrollbar->width(), horizontalScrollbar->height()));
}

- (void)scrollAnimatorDestroyed
{
    [self stopAnimation];
    _animator = 0;
}

@end

@interface ScrollbarPainterDelegate : NSObject<NSAnimationDelegate>
{
    WebCore::ScrollAnimatorMac* _animator;

    RetainPtr<ScrollbarPartAnimation> _verticalKnobAnimation;
    RetainPtr<ScrollbarPartAnimation> _horizontalKnobAnimation;

    RetainPtr<ScrollbarPartAnimation> _verticalTrackAnimation;
    RetainPtr<ScrollbarPartAnimation> _horizontalTrackAnimation;
}
- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator;
- (void)cancelAnimations;
@end

@implementation ScrollbarPainterDelegate

- (id)initWithScrollAnimator:(WebCore::ScrollAnimatorMac*)scrollAnimator
{
    self = [super init];
    if (!self)
        return nil;
    
    _animator = scrollAnimator;
    return self;
}

- (void)cancelAnimations
{
    [_verticalKnobAnimation.get() stopAnimation];
    [_horizontalKnobAnimation.get() stopAnimation];
    [_verticalTrackAnimation.get() stopAnimation];
    [_horizontalTrackAnimation.get() stopAnimation];
}

- (NSRect)convertRectToBacking:(NSRect)aRect
{
    return aRect;
}

- (NSRect)convertRectFromBacking:(NSRect)aRect
{
    return aRect;
}

- (CALayer *)layer
{
    if (!_animator)
        return nil;
    if (!_animator->isDrawingIntoLayer())
        return nil;

    // FIXME: This should attempt to return an actual layer.
    static CALayer *dummyLayer = [[CALayer alloc] init];
    return dummyLayer;
}

- (void)setUpAnimation:(RetainPtr<ScrollbarPartAnimation>&)scrollbarPartAnimation scrollerPainter:(WKScrollbarPainterRef)scrollerPainter part:(WebCore::ScrollbarPart)part animateAlphaTo:(CGFloat)newAlpha duration:(NSTimeInterval)duration
{
    // If the user has scrolled the page, then the scrollbars must be animated here. 
    // This overrides the early returns.
    bool mustAnimate = _animator->haveScrolledSincePageLoad();

    if (_animator->scrollbarPaintTimerIsActive() && !mustAnimate)
        return;

    if (_animator->scrollableArea()->shouldSuspendScrollAnimations() && !mustAnimate) {
        _animator->startScrollbarPaintTimer();
        return;
    }

    // At this point, we are definitely going to animate now, so stop the timer.
    _animator->stopScrollbarPaintTimer();

    // If we are currently animating, stop
    if (scrollbarPartAnimation) {
        [scrollbarPartAnimation.get() stopAnimation];
        scrollbarPartAnimation = nil;
    }

    if (part == WebCore::ThumbPart && !wkScrollbarPainterIsHorizontal(scrollerPainter)) {
        if (newAlpha == 1) {
            IntRect thumbRect = IntRect(wkScrollbarPainterKnobRect(scrollerPainter));
            _animator->setVisibleScrollerThumbRect(thumbRect);
        } else
            _animator->setVisibleScrollerThumbRect(IntRect());
    }

    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:duration];
    scrollbarPartAnimation.adoptNS([[ScrollbarPartAnimation alloc] initWithScrollbarPainter:scrollerPainter 
                                                                    part:part
                                                                    scrollAnimator:_animator 
                                                                    animateAlphaTo:newAlpha 
                                                                    duration:duration]);
    [scrollbarPartAnimation.get() setAnimationBlockingMode:NSAnimationNonblocking];
    [scrollbarPartAnimation.get() startAnimation];
    [NSAnimationContext endGrouping];
}

- (void)scrollerImp:(id)scrollerImp animateKnobAlphaTo:(CGFloat)newKnobAlpha duration:(NSTimeInterval)duration
{
    if (!_animator)
        return;

    WKScrollbarPainterRef scrollerPainter = (WKScrollbarPainterRef)scrollerImp;
    if (wkScrollbarPainterIsHorizontal(scrollerPainter))
        [self setUpAnimation:_horizontalKnobAnimation scrollerPainter:scrollerPainter part:WebCore::ThumbPart animateAlphaTo:newKnobAlpha duration:duration];
    else
        [self setUpAnimation:_verticalKnobAnimation scrollerPainter:scrollerPainter part:WebCore::ThumbPart animateAlphaTo:newKnobAlpha duration:duration];
}

- (void)scrollerImp:(id)scrollerImp animateTrackAlphaTo:(CGFloat)newTrackAlpha duration:(NSTimeInterval)duration
{
    if (!_animator)
        return;

    WKScrollbarPainterRef scrollerPainter = (WKScrollbarPainterRef)scrollerImp;
    if (wkScrollbarPainterIsHorizontal(scrollerPainter))
        [self setUpAnimation:_horizontalTrackAnimation scrollerPainter:scrollerPainter part:WebCore::BackTrackPart animateAlphaTo:newTrackAlpha duration:duration];
    else
        [self setUpAnimation:_verticalTrackAnimation scrollerPainter:scrollerPainter part:WebCore::BackTrackPart animateAlphaTo:newTrackAlpha duration:duration];
}

- (void)scrollerImp:(id)scrollerImp overlayScrollerStateChangedTo:(NSUInteger)newOverlayScrollerState
{
    UNUSED_PARAM(scrollerImp);
    UNUSED_PARAM(newOverlayScrollerState);
}

- (void)scrollAnimatorDestroyed
{
    _animator = 0;
    [_verticalKnobAnimation.get() scrollAnimatorDestroyed];
    [_horizontalKnobAnimation.get() scrollAnimatorDestroyed];
    [_verticalTrackAnimation.get() scrollAnimatorDestroyed];
    [_horizontalTrackAnimation.get() scrollAnimatorDestroyed];
}

@end

#endif // USE(WK_SCROLLBAR_PAINTER)

namespace WebCore {

PassOwnPtr<ScrollAnimator> ScrollAnimator::create(ScrollableArea* scrollableArea)
{
    return adoptPtr(new ScrollAnimatorMac(scrollableArea));
}

ScrollAnimatorMac::ScrollAnimatorMac(ScrollableArea* scrollableArea)
    : ScrollAnimator(scrollableArea)
#if USE(WK_SCROLLBAR_PAINTER)
    , m_initialScrollbarPaintTimer(this, &ScrollAnimatorMac::initialScrollbarPaintTimerFired)
#endif
#if ENABLE(RUBBER_BANDING)
    , m_inScrollGesture(false)
    , m_momentumScrollInProgress(false)
    , m_ignoreMomentumScrolls(false)
    , m_lastMomemtumScrollTimestamp(0)
    , m_startTime(0)
    , m_snapRubberBandTimer(this, &ScrollAnimatorMac::snapRubberBandTimerFired)
#endif
    , m_drawingIntoLayer(false)
    , m_haveScrolledSincePageLoad(false)
{
    m_scrollAnimationHelperDelegate.adoptNS([[ScrollAnimationHelperDelegate alloc] initWithScrollAnimator:this]);
    m_scrollAnimationHelper.adoptNS([[NSClassFromString(@"NSScrollAnimationHelper") alloc] initWithDelegate:m_scrollAnimationHelperDelegate.get()]);

#if USE(WK_SCROLLBAR_PAINTER)
    m_scrollbarPainterControllerDelegate.adoptNS([[ScrollbarPainterControllerDelegate alloc] initWithScrollAnimator:this]);
    m_scrollbarPainterController = wkMakeScrollbarPainterController(m_scrollbarPainterControllerDelegate.get());
    m_scrollbarPainterDelegate.adoptNS([[ScrollbarPainterDelegate alloc] initWithScrollAnimator:this]);
#endif
}

ScrollAnimatorMac::~ScrollAnimatorMac()
{
#if USE(WK_SCROLLBAR_PAINTER)
    [m_scrollbarPainterControllerDelegate.get() scrollAnimatorDestroyed];
    [(id)m_scrollbarPainterController.get() setDelegate:nil];
    [m_scrollbarPainterDelegate.get() scrollAnimatorDestroyed];
    [m_scrollAnimationHelperDelegate.get() scrollAnimatorDestroyed];
#endif
}

bool ScrollAnimatorMac::scroll(ScrollbarOrientation orientation, ScrollGranularity granularity, float step, float multiplier)
{
    m_haveScrolledSincePageLoad = true;

    if (![[NSUserDefaults standardUserDefaults] boolForKey:@"AppleScrollAnimationEnabled"])
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    if (granularity == ScrollByPixel)
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    float currentPos = orientation == HorizontalScrollbar ? m_currentPosX : m_currentPosY;
    float newPos = std::max<float>(std::min<float>(currentPos + (step * multiplier), static_cast<float>(m_scrollableArea->scrollSize(orientation))), 0);
    if (currentPos == newPos)
        return false;

    NSPoint newPoint;
    if ([m_scrollAnimationHelper.get() _isAnimating]) {
        NSPoint targetOrigin = [m_scrollAnimationHelper.get() targetOrigin];
        newPoint = orientation == HorizontalScrollbar ? NSMakePoint(newPos, targetOrigin.y) : NSMakePoint(targetOrigin.x, newPos);
    } else
        newPoint = orientation == HorizontalScrollbar ? NSMakePoint(newPos, m_currentPosY) : NSMakePoint(m_currentPosX, newPos);

    [m_scrollAnimationHelper.get() scrollToPoint:newPoint];
    return true;
}

void ScrollAnimatorMac::scrollToOffsetWithoutAnimation(const FloatPoint& offset)
{
    [m_scrollAnimationHelper.get() _stopRun];
    immediateScrollToPoint(offset);
}

float ScrollAnimatorMac::adjustScrollXPositionIfNecessary(float position) const
{
    if (!m_scrollableArea->constrainsScrollingToContentEdge())
        return position;

    return max<float>(min<float>(position, m_scrollableArea->contentsSize().width() - m_scrollableArea->visibleWidth()), 0);
}

float ScrollAnimatorMac::adjustScrollYPositionIfNecessary(float position) const
{
    if (!m_scrollableArea->constrainsScrollingToContentEdge())
        return position;

    return max<float>(min<float>(position, m_scrollableArea->contentsSize().height() - m_scrollableArea->visibleHeight()), 0);
}

FloatPoint ScrollAnimatorMac::adjustScrollPositionIfNecessary(const FloatPoint& position) const
{
    if (!m_scrollableArea->constrainsScrollingToContentEdge())
        return position;

    float newX = max<float>(min<float>(position.x(), m_scrollableArea->contentsSize().width() - m_scrollableArea->visibleWidth()), 0);
    float newY = max<float>(min<float>(position.y(), m_scrollableArea->contentsSize().height() - m_scrollableArea->visibleHeight()), 0);

    return FloatPoint(newX, newY);
}

void ScrollAnimatorMac::immediateScrollToPoint(const FloatPoint& newPosition)
{
    FloatPoint adjustedPosition = adjustScrollPositionIfNecessary(newPosition);
 
    if (adjustedPosition.x() == m_currentPosX && adjustedPosition.y() == m_currentPosY)
        return;
    
    m_currentPosX = adjustedPosition.x();
    m_currentPosY = adjustedPosition.y();
    notityPositionChanged();
}

void ScrollAnimatorMac::immediateScrollByDeltaX(float deltaX)
{
    float newPosX = adjustScrollXPositionIfNecessary(m_currentPosX + deltaX);
    
    if (newPosX == m_currentPosX)
        return;
    
    m_currentPosX = newPosX;
    notityPositionChanged();
}

void ScrollAnimatorMac::immediateScrollByDeltaY(float deltaY)
{
    float newPosY = adjustScrollYPositionIfNecessary(m_currentPosY + deltaY);
    
    if (newPosY == m_currentPosY)
        return;
    
    m_currentPosY = newPosY;
    notityPositionChanged();
}

void ScrollAnimatorMac::notityPositionChanged()
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkContentAreaScrolled(m_scrollbarPainterController.get());
#endif
    ScrollAnimator::notityPositionChanged();
}

void ScrollAnimatorMac::contentAreaWillPaint() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkContentAreaWillPaint(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::mouseEnteredContentArea() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkMouseEnteredContentArea(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::mouseExitedContentArea() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkMouseExitedContentArea(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::mouseMovedInContentArea() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkMouseMovedInContentArea(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::willStartLiveResize()
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkWillStartLiveResize(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::contentsResized() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkContentAreaResized(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::willEndLiveResize()
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkWillEndLiveResize(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::contentAreaDidShow() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkContentAreaDidShow(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::contentAreaDidHide() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkContentAreaDidHide(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::didBeginScrollGesture() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkDidBeginScrollGesture(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::didEndScrollGesture() const
{
#if USE(WK_SCROLLBAR_PAINTER)
    wkDidEndScrollGesture(m_scrollbarPainterController.get());
#endif
}

void ScrollAnimatorMac::didAddVerticalScrollbar(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    WKScrollbarPainterRef painter = static_cast<WebCore::ScrollbarThemeMac*>(WebCore::ScrollbarTheme::nativeTheme())->painterForScrollbar(scrollbar);
    wkScrollbarPainterSetDelegate(painter, m_scrollbarPainterDelegate.get());
    wkSetPainterForPainterController(m_scrollbarPainterController.get(), painter, false);
    if (scrollableArea()->inLiveResize())
        wkSetScrollbarPainterKnobAlpha(painter, 1);
#else
    UNUSED_PARAM(scrollbar);
#endif
}

void ScrollAnimatorMac::willRemoveVerticalScrollbar(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    WKScrollbarPainterRef painter = static_cast<WebCore::ScrollbarThemeMac*>(WebCore::ScrollbarTheme::nativeTheme())->painterForScrollbar(scrollbar);
    wkScrollbarPainterSetDelegate(painter, nil);
    wkSetPainterForPainterController(m_scrollbarPainterController.get(), nil, false);
#else
    UNUSED_PARAM(scrollbar);
#endif
}

void ScrollAnimatorMac::didAddHorizontalScrollbar(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    WKScrollbarPainterRef painter = static_cast<WebCore::ScrollbarThemeMac*>(WebCore::ScrollbarTheme::nativeTheme())->painterForScrollbar(scrollbar);
    wkScrollbarPainterSetDelegate(painter, m_scrollbarPainterDelegate.get());
    wkSetPainterForPainterController(m_scrollbarPainterController.get(), painter, true);
    if (scrollableArea()->inLiveResize())
        wkSetScrollbarPainterKnobAlpha(painter, 1);
#else
    UNUSED_PARAM(scrollbar);
#endif
}

void ScrollAnimatorMac::willRemoveHorizontalScrollbar(Scrollbar* scrollbar)
{
#if USE(WK_SCROLLBAR_PAINTER)
    WKScrollbarPainterRef painter = static_cast<WebCore::ScrollbarThemeMac*>(WebCore::ScrollbarTheme::nativeTheme())->painterForScrollbar(scrollbar);
    wkScrollbarPainterSetDelegate(painter, nil);
    wkSetPainterForPainterController(m_scrollbarPainterController.get(), nil, true);
#else
    UNUSED_PARAM(scrollbar);
#endif
}

void ScrollAnimatorMac::cancelAnimations()
{
    m_haveScrolledSincePageLoad = false;

#if USE(WK_SCROLLBAR_PAINTER)
    if (scrollbarPaintTimerIsActive())
        stopScrollbarPaintTimer();
    [m_scrollbarPainterDelegate.get() cancelAnimations];
#endif
}

#if ENABLE(RUBBER_BANDING)

static const float scrollVelocityZeroingTimeout = 0.10f;
static const float rubberbandStiffness = 20;
static const float rubberbandDirectionLockStretchRatio = 1;
static const float rubberbandMinimumRequiredDeltaBeforeStretch = 10;
static const float rubberbandAmplitude = 0.31f;
static const float rubberbandPeriod = 1.6f;

static float elasticDeltaForTimeDelta(float initialPosition, float initialVelocity, float elapsedTime)
{
    float amplitude = rubberbandAmplitude;
    float period = rubberbandPeriod;
    float criticalDampeningFactor = expf((-elapsedTime * rubberbandStiffness) / period);
             
    return (initialPosition + (-initialVelocity * elapsedTime * amplitude)) * criticalDampeningFactor;
}

static float elasticDeltaForReboundDelta(float delta)
{
    float stiffness = std::max(rubberbandStiffness, 1.0f);
    return delta / stiffness;
}

static float reboundDeltaForElasticDelta(float delta)
{
    return delta * rubberbandStiffness;
}

static float scrollWheelMultiplier()
{
    static float multiplier = -1;
    if (multiplier < 0) {
        multiplier = [[NSUserDefaults standardUserDefaults] floatForKey:@"NSScrollWheelMultiplier"];
        if (multiplier <= 0)
            multiplier = 1;
    }
    return multiplier;
}

void ScrollAnimatorMac::handleWheelEvent(PlatformWheelEvent& wheelEvent)
{
    m_haveScrolledSincePageLoad = true;

    if (!wheelEvent.hasPreciseScrollingDeltas()) {
        ScrollAnimator::handleWheelEvent(wheelEvent);
        return;
    }

    // FIXME: This is somewhat roundabout hack to allow forwarding wheel events
    // up to the parent scrollable area. It takes advantage of the fact that
    // the base class implemenatation of handleWheelEvent will not accept the
    // wheel event if there is nowhere to scroll.
    if (fabsf(wheelEvent.deltaY()) >= fabsf(wheelEvent.deltaX())) {
        if (!allowsVerticalStretching()) {
            ScrollAnimator::handleWheelEvent(wheelEvent);
            return;
        }
    } else {
        if (!allowsHorizontalStretching()) {
            ScrollAnimator::handleWheelEvent(wheelEvent);
            return;
        }
    }

    wheelEvent.accept();

    bool isMometumScrollEvent = (wheelEvent.momentumPhase() != PlatformWheelEventPhaseNone);
    if (m_ignoreMomentumScrolls && (isMometumScrollEvent || m_snapRubberBandTimer.isActive())) {
        if (wheelEvent.momentumPhase() == PlatformWheelEventPhaseEnded)
            m_ignoreMomentumScrolls = false;
        return;
    }

    smoothScrollWithEvent(wheelEvent);
}

void ScrollAnimatorMac::handleGestureEvent(const PlatformGestureEvent& gestureEvent)
{
    if (gestureEvent.type() == PlatformGestureEvent::ScrollBeginType)
        beginScrollGesture();
    else
        endScrollGesture();
}

bool ScrollAnimatorMac::pinnedInDirection(float deltaX, float deltaY)
{
    FloatSize limitDelta;
    if (fabsf(deltaY) >= fabsf(deltaX)) {
        if (deltaY < 0) {
            // We are trying to scroll up.  Make sure we are not pinned to the top
            limitDelta.setHeight(m_scrollableArea->visibleContentRect().y() + + m_scrollableArea->scrollOrigin().y());
        } else {
            // We are trying to scroll down.  Make sure we are not pinned to the bottom
            limitDelta.setHeight(m_scrollableArea->contentsSize().height() - (m_scrollableArea->visibleContentRect().maxY() + m_scrollableArea->scrollOrigin().y()));
        }
    } else if (deltaX != 0) {
        if (deltaX < 0) {
            // We are trying to scroll left.  Make sure we are not pinned to the left
            limitDelta.setWidth(m_scrollableArea->visibleContentRect().x() + m_scrollableArea->scrollOrigin().x());
        } else {
            // We are trying to scroll right.  Make sure we are not pinned to the right
            limitDelta.setWidth(m_scrollableArea->contentsSize().width() - (m_scrollableArea->visibleContentRect().maxX() + m_scrollableArea->scrollOrigin().x()));
        }
    }
    
    if ((deltaX != 0 || deltaY != 0) && (limitDelta.width() < 1 && limitDelta.height() < 1))
        return true;
    return false;
}

bool ScrollAnimatorMac::allowsVerticalStretching() const
{
    switch (m_scrollableArea->verticalScrollElasticity()) {
    case ScrollElasticityAutomatic: {
        Scrollbar* hScroller = m_scrollableArea->horizontalScrollbar();
        Scrollbar* vScroller = m_scrollableArea->verticalScrollbar();
        return (((vScroller && vScroller->enabled()) || (!hScroller || !hScroller->enabled())));
    }
    case ScrollElasticityNone:
        return false;
    case ScrollElasticityAllowed:
        return true;
    }

    ASSERT_NOT_REACHED();
    return false;
}

bool ScrollAnimatorMac::allowsHorizontalStretching() const
{
    switch (m_scrollableArea->horizontalScrollElasticity()) {
    case ScrollElasticityAutomatic: {
        Scrollbar* hScroller = m_scrollableArea->horizontalScrollbar();
        Scrollbar* vScroller = m_scrollableArea->verticalScrollbar();
        return (((hScroller && hScroller->enabled()) || (!vScroller || !vScroller->enabled())));
    }
    case ScrollElasticityNone:
        return false;
    case ScrollElasticityAllowed:
        return true;
    }

    ASSERT_NOT_REACHED();
    return false;
}

void ScrollAnimatorMac::smoothScrollWithEvent(PlatformWheelEvent& wheelEvent)
{
    m_haveScrolledSincePageLoad = true;

    float deltaX = m_overflowScrollDelta.width();
    float deltaY = m_overflowScrollDelta.height();

    // Reset overflow values because we may decide to remove delta at various points and put it into overflow.
    m_overflowScrollDelta = FloatSize();

    float eventCoallescedDeltaX = -wheelEvent.deltaX();
    float eventCoallescedDeltaY = -wheelEvent.deltaY();

    deltaX += eventCoallescedDeltaX;
    deltaY += eventCoallescedDeltaY;

    // Slightly prefer scrolling vertically by applying the = case to deltaY
    if (fabsf(deltaY) >= fabsf(deltaX))
        deltaX = 0;
    else
        deltaY = 0;

    bool isVerticallyStretched = false;
    bool isHorizontallyStretched = false;
    bool shouldStretch = false;
    
    IntSize stretchAmount = m_scrollableArea->overhangAmount();

    isHorizontallyStretched = stretchAmount.width();
    isVerticallyStretched = stretchAmount.height();

    PlatformWheelEventPhase phase = wheelEvent.momentumPhase();

    // If we are starting momentum scrolling then do some setup.
    if (!m_momentumScrollInProgress && (phase == PlatformWheelEventPhaseBegan || phase == PlatformWheelEventPhaseChanged))
        m_momentumScrollInProgress = true;

    CFTimeInterval timeDelta = wheelEvent.timestamp() - m_lastMomemtumScrollTimestamp;
    if (m_inScrollGesture || m_momentumScrollInProgress) {
        if (m_lastMomemtumScrollTimestamp && timeDelta > 0 && timeDelta < scrollVelocityZeroingTimeout) {
            m_momentumVelocity.setWidth(eventCoallescedDeltaX / (float)timeDelta);
            m_momentumVelocity.setHeight(eventCoallescedDeltaY / (float)timeDelta);
            m_lastMomemtumScrollTimestamp = wheelEvent.timestamp();
        } else {
            m_lastMomemtumScrollTimestamp = wheelEvent.timestamp();
            m_momentumVelocity = FloatSize();
        }

        if (isVerticallyStretched) {
            if (!isHorizontallyStretched && pinnedInDirection(deltaX, 0)) {                
                // Stretching only in the vertical.
                if (deltaY != 0 && (fabsf(deltaX / deltaY) < rubberbandDirectionLockStretchRatio))
                    deltaX = 0;
                else if (fabsf(deltaX) < rubberbandMinimumRequiredDeltaBeforeStretch) {
                    m_overflowScrollDelta.setWidth(m_overflowScrollDelta.width() + deltaX);
                    deltaX = 0;
                } else
                    m_overflowScrollDelta.setWidth(m_overflowScrollDelta.width() + deltaX);
            }
        } else if (isHorizontallyStretched) {
            // Stretching only in the horizontal.
            if (pinnedInDirection(0, deltaY)) {
                if (deltaX != 0 && (fabsf(deltaY / deltaX) < rubberbandDirectionLockStretchRatio))
                    deltaY = 0;
                else if (fabsf(deltaY) < rubberbandMinimumRequiredDeltaBeforeStretch) {
                    m_overflowScrollDelta.setHeight(m_overflowScrollDelta.height() + deltaY);
                    deltaY = 0;
                } else
                    m_overflowScrollDelta.setHeight(m_overflowScrollDelta.height() + deltaY);
            }
        } else {
            // Not stretching at all yet.
            if (pinnedInDirection(deltaX, deltaY)) {
                if (fabsf(deltaY) >= fabsf(deltaX)) {
                    if (fabsf(deltaX) < rubberbandMinimumRequiredDeltaBeforeStretch) {
                        m_overflowScrollDelta.setWidth(m_overflowScrollDelta.width() + deltaX);
                        deltaX = 0;
                    } else
                        m_overflowScrollDelta.setWidth(m_overflowScrollDelta.width() + deltaX);
                }
                shouldStretch = true;
            }
        }
    }

    if (deltaX != 0 || deltaY != 0) {
        if (!(shouldStretch || isVerticallyStretched || isHorizontallyStretched)) {
            if (deltaY != 0) {
                deltaY *= scrollWheelMultiplier();
                immediateScrollByDeltaY(deltaY);
            }
            if (deltaX != 0) {
                deltaX *= scrollWheelMultiplier();
                immediateScrollByDeltaX(deltaX);
            }
        } else {
            if (!allowsHorizontalStretching()) {
                deltaX = 0;
                eventCoallescedDeltaX = 0;
            } else if ((deltaX != 0) && !isHorizontallyStretched && !pinnedInDirection(deltaX, 0)) {
                deltaX *= scrollWheelMultiplier();

                m_scrollableArea->setConstrainsScrollingToContentEdge(false);
                immediateScrollByDeltaX(deltaX);
                m_scrollableArea->setConstrainsScrollingToContentEdge(true);

                deltaX = 0;
            }
            
            if (!allowsVerticalStretching()) {
                deltaY = 0;
                eventCoallescedDeltaY = 0;
            } else if ((deltaY != 0) && !isVerticallyStretched && !pinnedInDirection(0, deltaY)) {
                deltaY *= scrollWheelMultiplier();

                m_scrollableArea->setConstrainsScrollingToContentEdge(false);
                immediateScrollByDeltaY(deltaY);
                m_scrollableArea->setConstrainsScrollingToContentEdge(true);

                deltaY = 0;
            }
            
            IntSize stretchAmount = m_scrollableArea->overhangAmount();
        
            if (m_momentumScrollInProgress) {
                if ((pinnedInDirection(eventCoallescedDeltaX, eventCoallescedDeltaY) || (fabsf(eventCoallescedDeltaX) + fabsf(eventCoallescedDeltaY) <= 0)) && m_lastMomemtumScrollTimestamp) {
                    m_ignoreMomentumScrolls = true;
                    m_momentumScrollInProgress = false;
                    snapRubberBand();
                }
            }

            m_stretchScrollForce.setWidth(m_stretchScrollForce.width() + deltaX);
            m_stretchScrollForce.setHeight(m_stretchScrollForce.height() + deltaY);

            FloatSize dampedDelta(ceilf(elasticDeltaForReboundDelta(m_stretchScrollForce.width())), ceilf(elasticDeltaForReboundDelta(m_stretchScrollForce.height())));
            FloatPoint origOrigin = (m_scrollableArea->visibleContentRect().location() + m_scrollableArea->scrollOrigin()) - stretchAmount;
            FloatPoint newOrigin = origOrigin + dampedDelta;

            if (origOrigin != newOrigin) {
                m_scrollableArea->setConstrainsScrollingToContentEdge(false);
                immediateScrollToPoint(newOrigin);
                m_scrollableArea->setConstrainsScrollingToContentEdge(true);
            }
        }
    }

    if (m_momentumScrollInProgress && phase == PlatformWheelEventPhaseEnded) {
        m_momentumScrollInProgress = false;
        m_ignoreMomentumScrolls = false;
        m_lastMomemtumScrollTimestamp = 0;
    }
}

void ScrollAnimatorMac::beginScrollGesture()
{
    didBeginScrollGesture();

    m_haveScrolledSincePageLoad = true;
    m_inScrollGesture = true;
    m_momentumScrollInProgress = false;
    m_ignoreMomentumScrolls = false;
    m_lastMomemtumScrollTimestamp = 0;
    m_momentumVelocity = FloatSize();

    IntSize stretchAmount = m_scrollableArea->overhangAmount();
    m_stretchScrollForce.setWidth(reboundDeltaForElasticDelta(stretchAmount.width()));
    m_stretchScrollForce.setHeight(reboundDeltaForElasticDelta(stretchAmount.height()));

    m_overflowScrollDelta = FloatSize();
    
    if (m_snapRubberBandTimer.isActive())
        m_snapRubberBandTimer.stop();
}

void ScrollAnimatorMac::endScrollGesture()
{
    didEndScrollGesture();

    snapRubberBand();
}

void ScrollAnimatorMac::snapRubberBand()
{
    CFTimeInterval timeDelta = [[NSProcessInfo processInfo] systemUptime] - m_lastMomemtumScrollTimestamp;
    if (m_lastMomemtumScrollTimestamp && timeDelta >= scrollVelocityZeroingTimeout)
        m_momentumVelocity = FloatSize();

    m_inScrollGesture = false;

    if (m_snapRubberBandTimer.isActive())
        return;

    m_startTime = [NSDate timeIntervalSinceReferenceDate];
    m_startStretch = FloatSize();
    m_origOrigin = FloatPoint();
    m_origVelocity = FloatSize();

    m_snapRubberBandTimer.startRepeating(1.0/60.0);
}

static inline float roundTowardZero(float num)
{
    return num > 0 ? ceilf(num - 0.5f) : floorf(num + 0.5f);
}

static inline float roundToDevicePixelTowardZero(float num)
{
    float roundedNum = roundf(num);
    if (fabs(num - roundedNum) < 0.125)
        num = roundedNum;

    return roundTowardZero(num);
}

void ScrollAnimatorMac::snapRubberBandTimerFired(Timer<ScrollAnimatorMac>*)
{
    if (!m_momentumScrollInProgress || m_ignoreMomentumScrolls) {
        CFTimeInterval timeDelta = [NSDate timeIntervalSinceReferenceDate] - m_startTime;

        if (m_startStretch == FloatSize()) {
            m_startStretch = m_scrollableArea->overhangAmount();
            if (m_startStretch == FloatSize()) {    
                m_snapRubberBandTimer.stop();
                m_stretchScrollForce = FloatSize();
                m_startTime = 0;
                m_startStretch = FloatSize();
                m_origOrigin = FloatPoint();
                m_origVelocity = FloatSize();

                return;
            }

            m_origOrigin = (m_scrollableArea->visibleContentRect().location() + m_scrollableArea->scrollOrigin()) - m_startStretch;
            m_origVelocity = m_momentumVelocity;

            // Just like normal scrolling, prefer vertical rubberbanding
            if (fabsf(m_origVelocity.height()) >= fabsf(m_origVelocity.width()))
                m_origVelocity.setWidth(0);
            
            // Don't rubber-band horizontally if it's not possible to scroll horizontally
            Scrollbar* hScroller = m_scrollableArea->horizontalScrollbar();
            if (!hScroller || !hScroller->enabled())
                m_origVelocity.setWidth(0);
            
            // Don't rubber-band vertically if it's not possible to scroll horizontally
            Scrollbar* vScroller = m_scrollableArea->verticalScrollbar();
            if (!vScroller || !vScroller->enabled())
                m_origVelocity.setHeight(0);
        }

        FloatPoint delta(roundToDevicePixelTowardZero(elasticDeltaForTimeDelta(m_startStretch.width(), -m_origVelocity.width(), (float)timeDelta)),
                         roundToDevicePixelTowardZero(elasticDeltaForTimeDelta(m_startStretch.height(), -m_origVelocity.height(), (float)timeDelta)));

        if (fabs(delta.x()) >= 1 || fabs(delta.y()) >= 1) {
            FloatPoint newOrigin = m_origOrigin + delta;

            m_scrollableArea->setConstrainsScrollingToContentEdge(false);
            immediateScrollToPoint(newOrigin);
            m_scrollableArea->setConstrainsScrollingToContentEdge(true);

            FloatSize newStretch = m_scrollableArea->overhangAmount();
            
            m_stretchScrollForce.setWidth(reboundDeltaForElasticDelta(newStretch.width()));
            m_stretchScrollForce.setHeight(reboundDeltaForElasticDelta(newStretch.height()));
        } else {
            immediateScrollToPoint(m_origOrigin);

            m_scrollableArea->didCompleteRubberBand(roundedIntSize(m_startStretch));

            m_snapRubberBandTimer.stop();
            m_stretchScrollForce = FloatSize();
            
            m_startTime = 0;
            m_startStretch = FloatSize();
            m_origOrigin = FloatPoint();
            m_origVelocity = FloatSize();
        }
    } else {
        m_startTime = [NSDate timeIntervalSinceReferenceDate];
        m_startStretch = FloatSize();
    }
}
#endif

#if USE(WK_SCROLLBAR_PAINTER)
void ScrollAnimatorMac::startScrollbarPaintTimer()
{
    m_initialScrollbarPaintTimer.startOneShot(0.1);
}

bool ScrollAnimatorMac::scrollbarPaintTimerIsActive() const
{
    return m_initialScrollbarPaintTimer.isActive();
}

void ScrollAnimatorMac::stopScrollbarPaintTimer()
{
    m_initialScrollbarPaintTimer.stop();
}

void ScrollAnimatorMac::initialScrollbarPaintTimerFired(Timer<ScrollAnimatorMac>*)
{
    wkScrollbarPainterForceFlashScrollers(m_scrollbarPainterController.get());
}
#endif

void ScrollAnimatorMac::setVisibleScrollerThumbRect(const IntRect& scrollerThumb)
{
    IntRect rectInViewCoordinates = scrollerThumb;
    if (Scrollbar* verticalScrollbar = m_scrollableArea->verticalScrollbar())
        rectInViewCoordinates = verticalScrollbar->convertToContainingView(scrollerThumb);

    if (rectInViewCoordinates == m_visibleScrollerThumbRect)
        return;

    m_scrollableArea->setVisibleScrollerThumbRect(rectInViewCoordinates);
    m_visibleScrollerThumbRect = rectInViewCoordinates;
}

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)
