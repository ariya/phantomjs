/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef WebCore_NSScrollerImpDetails_h
#define WebCore_NSScrollerImpDetails_h

#include "config.h"

// Public APIs not available on versions of Mac on which we build
#if __MAC_OS_X_VERSION_MAX_ALLOWED == 1060
enum {
    NSScrollerStyleLegacy       = 0,
    NSScrollerStyleOverlay      = 1
};
typedef NSInteger NSScrollerStyle;

enum {
    NSScrollerKnobStyleDefault = 0,
    NSScrollerKnobStyleDark = 1,
    NSScrollerKnobStyleLight = 2
};
typedef NSInteger NSScrollerKnobStyle;
#endif

#if __MAC_OS_X_VERSION_MAX_ALLOWED == 1060
@interface NSScroller(NSObject)
+ (NSScrollerStyle)preferredScrollerStyle;
@end
#endif

@interface NSObject (ScrollbarPainter)
+ (id)scrollerImpWithStyle:(NSScrollerStyle)newScrollerStyle controlSize:(NSControlSize)newControlSize horizontal:(BOOL)horizontal replacingScrollerImp:(id)previous;
- (CGFloat)knobAlpha;
- (void)setKnobAlpha:(CGFloat)knobAlpha;
- (CGFloat)trackAlpha;
- (void)setTrackAlpha:(CGFloat)trackAlpha;
- (void)setEnabled:(BOOL)enabled;
- (void)setBoundsSize:(NSSize)boundsSize;
- (void)setDoubleValue:(double)doubleValue;
- (void)setKnobProportion:(CGFloat)proportion;
- (void)setKnobStyle:(NSScrollerKnobStyle)knobStyle;
- (void)setExpanded:(BOOL)expanded;
- (BOOL)isExpanded;
- (void)setDelegate:(id)delegate;
- (void)setUiStateTransitionProgress:(CGFloat)uiStateTransitionProgress;
- (void)setExpansionTransitionProgress:(CGFloat)expansionTransitionProgress;
- (BOOL)isHorizontal;
- (CGFloat)trackWidth;
- (CGFloat)trackBoxWidth;
- (CGFloat)knobMinLength;
- (CGFloat)trackOverlapEndInset;
- (CGFloat)knobOverlapEndInset;
- (CGFloat)trackEndInset;
- (CGFloat)knobEndInset;
- (CGFloat)uiStateTransitionProgress;
- (CGFloat)expansionTransitionProgress;
- (NSRect)rectForPart:(NSScrollerPart)partCode;
- (void)drawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag alpha:(CGFloat)alpha;
- (void)drawKnob;
- (void)mouseEnteredScroller;
- (void)mouseExitedScroller;
@end

@interface NSObject (ScrollbarPainterController)
- (void)setDelegate:(id)delegate;
- (void)hideOverlayScrollers;
- (void)flashScrollers;
- (id)horizontalScrollerImp;
- (void)setHorizontalScrollerImp:(id)horizontal;
- (id)verticalScrollerImp;
- (void)setVerticalScrollerImp:(id)vertical;
- (NSScrollerStyle)scrollerStyle;
- (void)setScrollerStyle:(NSScrollerStyle)scrollerStyle;
- (void)contentAreaScrolled;
- (void)contentAreaScrolledInDirection:(NSPoint)direction;
- (void)contentAreaWillDraw;
- (void)mouseEnteredContentArea;
- (void)mouseExitedContentArea;
- (void)mouseMovedInContentArea;
- (void)startLiveResize;
- (void)contentAreaDidResize;
- (void)endLiveResize;
- (void)windowOrderedIn;
- (void)windowOrderedOut;
- (void)beginScrollGesture;
- (void)endScrollGesture;
@end

namespace WebCore {

static inline bool isScrollbarOverlayAPIAvailable()
{
#if USE(SCROLLBAR_PAINTER)
    return true;
#else
    return false;
#endif
}

NSScrollerStyle recommendedScrollerStyle();

}

#endif
