/*
 * Copyright (C) 2005, 2008, 2010 Apple Inc. All rights reserved.
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

#import "WebDynamicScrollBarsViewInternal.h"

#import "WebDocument.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebHTMLViewInternal.h"
#import <WebCore/FrameView.h>
#import <WebKitSystemInterface.h>

using namespace WebCore;

// FIXME: <rdar://problem/5898985> Mail expects a constant of this name to exist.
const int WebCoreScrollbarAlwaysOn = ScrollbarAlwaysOn;

#ifndef __OBJC2__
// In <rdar://problem/7814899> we saw crashes because WebDynamicScrollBarsView increased in size, breaking ABI compatiblity.
COMPILE_ASSERT(sizeof(WebDynamicScrollBarsView) == 0x8c, WebDynamicScrollBarsView_is_expected_size);
#endif

@interface NSScrollView(WebNSScrollViewDetails)
+ (Class)_horizontalScrollerClass;
+ (Class)_verticalScrollerClass;
@end

struct WebDynamicScrollBarsViewPrivate {
    unsigned inUpdateScrollersLayoutPass;

    WebCore::ScrollbarMode hScroll;
    WebCore::ScrollbarMode vScroll;

    bool hScrollModeLocked;
    bool vScrollModeLocked;
    bool suppressLayout;
    bool suppressScrollers;
    bool inUpdateScrollers;
    bool verticallyPinnedByPreviousWheelEvent;
    bool horizontallyPinnedByPreviousWheelEvent;

    bool allowsScrollersToOverlapContent;
    bool alwaysHideHorizontalScroller;
    bool alwaysHideVerticalScroller;
    bool horizontalScrollingAllowedButScrollerHidden;
    bool verticalScrollingAllowedButScrollerHidden;

    // scrollOrigin is set for various combinations of writing mode and direction.
    // See the comment next to the corresponding member in ScrollView.h.
    NSPoint scrollOrigin;

    // Flag to indicate that the scrollbar thumb's initial position needs to
    // be manually set.
    bool scrollOriginChanged;
    NSPoint scrollPositionExcludingOrigin;

    bool inProgrammaticScroll;
};

@implementation WebDynamicScrollBarsView

static Class customScrollerClass;

+ (Class)_horizontalScrollerClass
{
    if (Settings::mockScrollbarsEnabled() && customScrollerClass)
        return customScrollerClass;

    return [super _horizontalScrollerClass];
}

+ (Class)_verticalScrollerClass
{
    if (Settings::mockScrollbarsEnabled() && customScrollerClass)
        return customScrollerClass;

    return [super _horizontalScrollerClass];
}

+ (void)setCustomScrollerClass:(Class)scrollerClass
{
    customScrollerClass = scrollerClass;
}

- (id)initWithFrame:(NSRect)frame
{
    if (!(self = [super initWithFrame:frame]))
        return nil;

    _private = new WebDynamicScrollBarsViewPrivate;
    memset(_private, 0, sizeof(WebDynamicScrollBarsViewPrivate));
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (!(self = [super initWithCoder:aDecoder]))
        return nil;

    _private = new WebDynamicScrollBarsViewPrivate;
    memset(_private, 0, sizeof(WebDynamicScrollBarsViewPrivate));
    return self;
}

- (void)dealloc
{
    delete _private;
    [super dealloc];
}

- (void)finalize
{
    delete _private;
    [super finalize];
}

- (void)setAllowsHorizontalScrolling:(BOOL)flag
{
    if (_private->hScrollModeLocked)
        return;
    if (flag && _private->hScroll == ScrollbarAlwaysOff)
        _private->hScroll = ScrollbarAuto;
    else if (!flag && _private->hScroll != ScrollbarAlwaysOff)
        _private->hScroll = ScrollbarAlwaysOff;
    [self updateScrollers];
}

- (void)setAllowsScrollersToOverlapContent:(BOOL)flag
{
    if (_private->allowsScrollersToOverlapContent == flag)
        return;

    _private->allowsScrollersToOverlapContent = flag;

    [[self contentView] setFrame:[self contentViewFrame]];
    [[self documentView] setNeedsLayout:YES];
    [[self documentView] layout];
}

- (void)setAlwaysHideHorizontalScroller:(BOOL)shouldBeHidden
{
    if (_private->alwaysHideHorizontalScroller == shouldBeHidden)
        return;

    _private->alwaysHideHorizontalScroller = shouldBeHidden;
    [self updateScrollers];
}

- (void)setAlwaysHideVerticalScroller:(BOOL)shouldBeHidden
{
    if (_private->alwaysHideVerticalScroller == shouldBeHidden)
        return;

    _private->alwaysHideVerticalScroller = shouldBeHidden;
    [self updateScrollers];
}

- (BOOL)horizontalScrollingAllowed
{
    return _private->horizontalScrollingAllowedButScrollerHidden || [self hasHorizontalScroller];
}

- (BOOL)verticalScrollingAllowed
{
    return _private->verticalScrollingAllowedButScrollerHidden || [self hasVerticalScroller];
}

static BOOL shouldRoundScrollOrigin(WebDynamicScrollBarsView *view)
{
    NSView *documentView = [view documentView];
    if (![documentView isKindOfClass:[WebHTMLView class]])
        return NO;

    Frame* frame = core([(WebHTMLView *)documentView _frame]);
    if (!frame)
        return NO;
    
    FrameView *frameView = frame->view();
    if (!frameView)
        return NO;

    return frameView->hasViewportConstrainedObjects();
}

- (void)scrollClipView:(NSClipView *)clipView toPoint:(NSPoint)point
{
    if (shouldRoundScrollOrigin(self)) {
        // WebCore isn't yet able to handle subpixel scrolling, as can happen on Retina displays. For
        // now we'll round to the nearest pixel. Once subpixel layout is enabled in WebCore we may be
        // able to remove this method entirely.
        point.x = round(point.x);
        point.y = round(point.y);
    }

    [super scrollClipView:clipView toPoint:point];
}

@end

@implementation WebDynamicScrollBarsView (WebInternal)

- (NSRect)contentViewFrame
{
    NSRect frame = [[self contentView] frame];

    if ([self hasHorizontalScroller])
        frame.size.height = (_private->allowsScrollersToOverlapContent ? NSMaxY([[self horizontalScroller] frame]) : NSMinY([[self horizontalScroller] frame]));
    if ([self hasVerticalScroller])
        frame.size.width = (_private->allowsScrollersToOverlapContent ? NSMaxX([[self verticalScroller] frame]) : NSMinX([[self verticalScroller] frame]));
    return frame;
}

- (void)tile
{
    [super tile];

    // [super tile] sets the contentView size so that it does not overlap with the scrollers,
    // we want to re-set the contentView to overlap scrollers before displaying.
    if (_private->allowsScrollersToOverlapContent)
        [[self contentView] setFrame:[self contentViewFrame]];
}

- (void)setSuppressLayout:(BOOL)flag
{
    _private->suppressLayout = flag;
}

- (void)setScrollBarsSuppressed:(BOOL)suppressed repaintOnUnsuppress:(BOOL)repaint
{
    _private->suppressScrollers = suppressed;

    if (suppressed) {
        [[self verticalScroller] setNeedsDisplay:NO];
        [[self horizontalScroller] setNeedsDisplay:NO];
    }

    if (!suppressed && repaint)
        [super reflectScrolledClipView:[self contentView]];
}

- (void)adjustForScrollOriginChange
{
    if (!_private->scrollOriginChanged)
        return;

    _private->scrollOriginChanged = false;

    NSView *documentView = [self documentView];
    NSRect documentRect = [documentView bounds];

    // The call to [NSView scrollPoint:] fires off notification the handler for which needs to know that
    // we're setting the initial scroll position so it doesn't interpret this as a user action and
    // fire off a JS event.
    _private->inProgrammaticScroll = true;
    [documentView scrollPoint:NSMakePoint(_private->scrollPositionExcludingOrigin.x + documentRect.origin.x, _private->scrollPositionExcludingOrigin.y + documentRect.origin.y)];
    _private->inProgrammaticScroll = false;
}

static const unsigned cMaxUpdateScrollbarsPass = 2;

- (void)updateScrollers
{
    NSView *documentView = [self documentView];

    // If we came in here with the view already needing a layout, then go ahead and do that
    // first.  (This will be the common case, e.g., when the page changes due to window resizing for example).
    // This layout will not re-enter updateScrollers and does not count towards our max layout pass total.
    if (!_private->suppressLayout && !_private->suppressScrollers && [documentView isKindOfClass:[WebHTMLView class]]) {
        WebHTMLView* htmlView = (WebHTMLView*)documentView;
        if ([htmlView _needsLayout]) {
            _private->inUpdateScrollers = YES;
            [(id <WebDocumentView>)documentView layout];
            _private->inUpdateScrollers = NO;
        }
    }

    BOOL hasHorizontalScroller = [self hasHorizontalScroller];
    BOOL hasVerticalScroller = [self hasVerticalScroller];

    BOOL newHasHorizontalScroller = hasHorizontalScroller;
    BOOL newHasVerticalScroller = hasVerticalScroller;

    if (!documentView) {
        newHasHorizontalScroller = NO;
        newHasVerticalScroller = NO;
    }

    if (_private->hScroll != ScrollbarAuto)
        newHasHorizontalScroller = (_private->hScroll == ScrollbarAlwaysOn);
    if (_private->vScroll != ScrollbarAuto)
        newHasVerticalScroller = (_private->vScroll == ScrollbarAlwaysOn);

    if (!documentView || _private->suppressLayout || _private->suppressScrollers || (_private->hScroll != ScrollbarAuto && _private->vScroll != ScrollbarAuto)) {
        _private->horizontalScrollingAllowedButScrollerHidden = newHasHorizontalScroller && _private->alwaysHideHorizontalScroller;
        if (_private->horizontalScrollingAllowedButScrollerHidden)
            newHasHorizontalScroller = NO;

        _private->verticalScrollingAllowedButScrollerHidden = newHasVerticalScroller && _private->alwaysHideVerticalScroller;
        if (_private->verticalScrollingAllowedButScrollerHidden)
            newHasVerticalScroller = NO;

        _private->inUpdateScrollers = YES;
        if (hasHorizontalScroller != newHasHorizontalScroller)
            [self setHasHorizontalScroller:newHasHorizontalScroller];
        if (hasVerticalScroller != newHasVerticalScroller)
            [self setHasVerticalScroller:newHasVerticalScroller];
        if (_private->suppressScrollers) {
            [[self verticalScroller] setNeedsDisplay:NO];
            [[self horizontalScroller] setNeedsDisplay:NO];
        }
        _private->inUpdateScrollers = NO;
        return;
    }

    BOOL needsLayout = NO;

    NSSize documentSize = [documentView frame].size;
    NSSize visibleSize = [self documentVisibleRect].size;
    NSSize frameSize = [self frame].size;
    
    // When in HiDPI with a scale factor > 1, the visibleSize and frameSize may be non-integral values,
    // while the documentSize (set by WebCore) will be integral.  Round up the non-integral sizes so that
    // the mismatch won't cause unwanted scrollbars to appear.  This can result in slightly cut off content,
    // but it will always be less than one pixel, which should not be noticeable.
    visibleSize.width = ceilf(visibleSize.width);
    visibleSize.height = ceilf(visibleSize.height);
    frameSize.width = ceilf(frameSize.width);
    frameSize.height = ceilf(frameSize.height);

    if (_private->hScroll == ScrollbarAuto) {
        newHasHorizontalScroller = documentSize.width > visibleSize.width;
        if (newHasHorizontalScroller && !_private->inUpdateScrollersLayoutPass && documentSize.height <= frameSize.height && documentSize.width <= frameSize.width)
            newHasHorizontalScroller = NO;
    }

    if (_private->vScroll == ScrollbarAuto) {
        newHasVerticalScroller = documentSize.height > visibleSize.height;
        if (newHasVerticalScroller && !_private->inUpdateScrollersLayoutPass && documentSize.height <= frameSize.height && documentSize.width <= frameSize.width)
            newHasVerticalScroller = NO;
    }

    // Unless in ScrollbarsAlwaysOn mode, if we ever turn one scrollbar off, always turn the other one off too.
    // Never ever try to both gain/lose a scrollbar in the same pass.
    if (!newHasHorizontalScroller && hasHorizontalScroller && _private->vScroll != ScrollbarAlwaysOn)
        newHasVerticalScroller = NO;
    if (!newHasVerticalScroller && hasVerticalScroller && _private->hScroll != ScrollbarAlwaysOn)
        newHasHorizontalScroller = NO;

    _private->horizontalScrollingAllowedButScrollerHidden = newHasHorizontalScroller && _private->alwaysHideHorizontalScroller;
    if (_private->horizontalScrollingAllowedButScrollerHidden)
        newHasHorizontalScroller = NO;

    _private->verticalScrollingAllowedButScrollerHidden = newHasVerticalScroller && _private->alwaysHideVerticalScroller;
    if (_private->verticalScrollingAllowedButScrollerHidden)
        newHasVerticalScroller = NO;

    if (hasHorizontalScroller != newHasHorizontalScroller) {
        _private->inUpdateScrollers = YES;
        [self setHasHorizontalScroller:newHasHorizontalScroller];
        _private->inUpdateScrollers = NO;
        needsLayout = YES;
        NSView *documentView = [self documentView];
        NSRect documentRect = [documentView bounds];
        if (documentRect.origin.y < 0 && !newHasHorizontalScroller)
            [documentView setBoundsOrigin:NSMakePoint(documentRect.origin.x, documentRect.origin.y + 15)];
    }

    if (hasVerticalScroller != newHasVerticalScroller) {
        _private->inUpdateScrollers = YES;
        [self setHasVerticalScroller:newHasVerticalScroller];
        _private->inUpdateScrollers = NO;
        needsLayout = YES;
        NSView *documentView = [self documentView];
        NSRect documentRect = [documentView bounds];
        if (documentRect.origin.x < 0 && !newHasVerticalScroller)
            [documentView setBoundsOrigin:NSMakePoint(documentRect.origin.x + 15, documentRect.origin.y)];
    }

    if (needsLayout && _private->inUpdateScrollersLayoutPass < cMaxUpdateScrollbarsPass &&
        [documentView conformsToProtocol:@protocol(WebDocumentView)]) {
        _private->inUpdateScrollersLayoutPass++;
        [(id <WebDocumentView>)documentView setNeedsLayout:YES];
        [(id <WebDocumentView>)documentView layout];
        NSSize newDocumentSize = [documentView frame].size;
        if (NSEqualSizes(documentSize, newDocumentSize)) {
            // The layout with the new scroll state had no impact on
            // the document's overall size, so updateScrollers didn't get called.
            // Recur manually.
            [self updateScrollers];
        }
        _private->inUpdateScrollersLayoutPass--;
    }
}

// Make the horizontal and vertical scroll bars come and go as needed.
- (void)reflectScrolledClipView:(NSClipView *)clipView
{
    if (clipView == [self contentView]) {
        // Prevent appearance of trails because of overlapping views
        if (_private->allowsScrollersToOverlapContent)
            [self setDrawsBackground:NO];

        // FIXME: This hack here prevents infinite recursion that takes place when we
        // gyrate between having a vertical scroller and not having one. A reproducible
        // case is clicking on the "the Policy Routing text" link at
        // http://www.linuxpowered.com/archive/howto/Net-HOWTO-8.html.
        // The underlying cause is some problem in the NSText machinery, but I was not
        // able to pin it down.
        NSGraphicsContext *currentContext = [NSGraphicsContext currentContext];
        if (!_private->inUpdateScrollers && (!currentContext || [currentContext isDrawingToScreen]))
            [self updateScrollers];
    }

    // Update the scrollers if they're not being suppressed.
    if (!_private->suppressScrollers)
        [super reflectScrolledClipView:clipView];

    // The call to [NSView reflectScrolledClipView] sets the scrollbar thumb
    // position to 0 (the left) when the view is initially displayed.
    // This call updates the initial position correctly.
    [self adjustForScrollOriginChange];
}

- (BOOL)allowsHorizontalScrolling
{
    return _private->hScroll != ScrollbarAlwaysOff;
}

- (BOOL)allowsVerticalScrolling
{
    return _private->vScroll != ScrollbarAlwaysOff;
}

- (void)scrollingModes:(WebCore::ScrollbarMode*)hMode vertical:(WebCore::ScrollbarMode*)vMode
{
    *hMode = _private->hScroll;
    *vMode = _private->vScroll;
}

- (ScrollbarMode)horizontalScrollingMode
{
    return _private->hScroll;
}

- (ScrollbarMode)verticalScrollingMode
{
    return _private->vScroll;
}

- (void)setHorizontalScrollingMode:(ScrollbarMode)horizontalMode andLock:(BOOL)lock
{
    [self setScrollingModes:horizontalMode vertical:[self verticalScrollingMode] andLock:lock];
}

- (void)setVerticalScrollingMode:(ScrollbarMode)verticalMode andLock:(BOOL)lock
{
    [self setScrollingModes:[self horizontalScrollingMode] vertical:verticalMode andLock:lock];
}

// Mail uses this method, so we cannot remove it. 
- (void)setVerticalScrollingMode:(ScrollbarMode)verticalMode 
{ 
    [self setScrollingModes:[self horizontalScrollingMode] vertical:verticalMode andLock:NO]; 
} 

- (void)setScrollingModes:(ScrollbarMode)horizontalMode vertical:(ScrollbarMode)verticalMode andLock:(BOOL)lock
{
    BOOL update = NO;
    if (verticalMode != _private->vScroll && !_private->vScrollModeLocked) {
        _private->vScroll = verticalMode;
        update = YES;
    }

    if (horizontalMode != _private->hScroll && !_private->hScrollModeLocked) {
        _private->hScroll = horizontalMode;
        update = YES;
    }

    if (lock)
        [self setScrollingModesLocked:YES];

    if (update)
        [self updateScrollers];
}

- (void)setHorizontalScrollingModeLocked:(BOOL)locked
{
    _private->hScrollModeLocked = locked;
}

- (void)setVerticalScrollingModeLocked:(BOOL)locked
{
    _private->vScrollModeLocked = locked;
}

- (void)setScrollingModesLocked:(BOOL)locked
{
    _private->hScrollModeLocked = _private->vScrollModeLocked = locked;
}

- (BOOL)horizontalScrollingModeLocked
{
    return _private->hScrollModeLocked;
}

- (BOOL)verticalScrollingModeLocked
{
    return _private->vScrollModeLocked;
}

- (BOOL)autoforwardsScrollWheelEvents
{
    return YES;
}

- (void)scrollWheel:(NSEvent *)event
{
    float deltaX;
    float deltaY;
    BOOL isContinuous;
    WKGetWheelEventDeltas(event, &deltaX, &deltaY, &isContinuous);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    NSEventPhase momentumPhase = [event momentumPhase];
    BOOL isLatchingEvent = momentumPhase & NSEventPhaseBegan || momentumPhase & NSEventPhaseStationary;
#else
    int momentumPhase = WKGetNSEventMomentumPhase(event);
    BOOL isLatchingEvent = momentumPhase == WKEventPhaseBegan || momentumPhase == WKEventPhaseChanged;
#endif

    if (fabsf(deltaY) > fabsf(deltaX)) {
        if (![self allowsVerticalScrolling]) {
            [[self nextResponder] scrollWheel:event];
            return;
        }

        if (isLatchingEvent && !_private->verticallyPinnedByPreviousWheelEvent) {
            double verticalPosition = [[self verticalScroller] doubleValue];
            if ((deltaY >= 0.0 && verticalPosition == 0.0) || (deltaY <= 0.0 && verticalPosition == 1.0))
                return;
        }
    } else {
        if (![self allowsHorizontalScrolling]) {
            [[self nextResponder] scrollWheel:event];
            return;
        }

        if (isLatchingEvent && !_private->horizontallyPinnedByPreviousWheelEvent) {
            double horizontalPosition = [[self horizontalScroller] doubleValue];
            if ((deltaX >= 0.0 && horizontalPosition == 0.0) || (deltaX <= 0.0 && horizontalPosition == 1.0))
                return;
        }
    }

    // Calling super can release the last reference. <rdar://problem/7400263>
    // Hold a reference so the code following the super call will not crash.
    [self retain];

    [super scrollWheel:event];

    if (!isLatchingEvent) {
        double verticalPosition = [[self verticalScroller] doubleValue];
        double horizontalPosition = [[self horizontalScroller] doubleValue];

        _private->verticallyPinnedByPreviousWheelEvent = (verticalPosition == 0.0 || verticalPosition == 1.0);
        _private->horizontallyPinnedByPreviousWheelEvent = (horizontalPosition == 0.0 || horizontalPosition == 1.0);
    }

    [self release];
}

// This object will be the parent of the web area in WK1, so it should not be ignored.
- (BOOL)accessibilityIsIgnored 
{
    return NO;
}

- (void)setScrollOrigin:(NSPoint)scrollOrigin updatePositionAtAll:(BOOL)updatePositionAtAll immediately:(BOOL)updatePositionSynchronously
{
    // The cross-platform ScrollView call already checked to see if the old/new scroll origins were the same or not
    // so we don't have to check for equivalence here.
    _private->scrollOrigin = scrollOrigin;
    id docView = [self documentView];

    NSRect visibleRect = [self documentVisibleRect];

    [docView setBoundsOrigin:NSMakePoint(-scrollOrigin.x, -scrollOrigin.y)];

    if (updatePositionAtAll)
        _private->scrollOriginChanged = true;

    // Maintain our original position in the presence of the new scroll origin.
    _private->scrollPositionExcludingOrigin = NSMakePoint(visibleRect.origin.x + scrollOrigin.x, visibleRect.origin.y + scrollOrigin.y);

    if (updatePositionAtAll && updatePositionSynchronously) // Otherwise we'll just let the snap happen when we update for the resize.
        [self adjustForScrollOriginChange];
}

- (NSPoint)scrollOrigin
{
    return _private->scrollOrigin;
}

- (BOOL)inProgrammaticScroll
{
    return _private->inProgrammaticScroll;
}

@end
