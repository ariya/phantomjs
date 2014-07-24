/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"

#import "WebWindowAnimation.h"

#import "FloatConversion.h"
#import "WebCoreSystemInterface.h"
#import <wtf/Assertions.h>
#import <wtf/MathExtras.h>

using namespace WebCore;

static const CGFloat slowMotionFactor = 10;

static NSTimeInterval WebWindowAnimationDurationFromDuration(NSTimeInterval duration)
{
    return ([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask) ? duration * slowMotionFactor : duration;        
}

static NSRect scaledRect(NSRect _initialFrame, NSRect _finalFrame, CGFloat factor)
{
    NSRect currentRect = _initialFrame;
    currentRect.origin.x += (NSMinX(_finalFrame) - NSMinX(_initialFrame)) * factor;
    currentRect.origin.y += (NSMinY(_finalFrame) - NSMinY(_initialFrame)) * factor;
    currentRect.size.width += (NSWidth(_finalFrame) - NSWidth(_initialFrame)) * factor;
    currentRect.size.height += (NSHeight(_finalFrame) - NSHeight(_initialFrame)) * factor;
    return currentRect;
}

static CGFloat squaredDistance(NSPoint point1, NSPoint point2)
{
    CGFloat deltaX = point1.x - point2.x;
    CGFloat deltaY = point1.y - point2.y;
    return deltaX * deltaX + deltaY * deltaY;
}

@implementation WebWindowScaleAnimation

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    [self setAnimationBlockingMode:NSAnimationNonblockingThreaded];
    [self setFrameRate:60];
    return self;
}

- (id)initWithHintedDuration:(NSTimeInterval)duration window:(NSWindow *)window initalFrame:(NSRect)initialFrame finalFrame:(NSRect)finalFrame
{
    self = [self init];
    if (!self)
        return nil;
    _hintedDuration = duration;
    _window = window;
    _initialFrame = initialFrame;
    _finalFrame = finalFrame;
    _realFrame = [window frame];
    return self;
}

- (void) dealloc
{
    [_subAnimation release];
    [super dealloc];
}

- (void)setDuration:(NSTimeInterval)duration
{
    [super setDuration:WebWindowAnimationDurationFromDuration(duration)];
}

- (void)setWindow:(NSWindow *)window
{
    _window = window;
}

- (float)currentValue
{
    return narrowPrecisionToFloat(0.5 - 0.5 * cos(piDouble * (1 - [self currentProgress])));
}

- (NSRect)currentFrame
{
    return scaledRect(_finalFrame, _initialFrame, [self currentValue]);
}

- (void)setCurrentProgress:(NSAnimationProgress)progress
{
    if (!_window)
        return;

    [super setCurrentProgress:progress];

    NSRect currentRect = [self currentFrame];
    wkWindowSetScaledFrame(_window, currentRect, _realFrame);
    [_subAnimation setCurrentProgress:progress];
}

- (void)setSubAnimation:(NSAnimation *)animation
{
    id oldAnimation = _subAnimation;
    _subAnimation = [animation retain];
    [oldAnimation release];
}

- (NSTimeInterval)additionalDurationNeededToReachFinalFrame
{
    static const CGFloat maxAdditionalDuration = 1;
    static const CGFloat speedFactor = 0.0001f;
    
    CGFloat maxDist = squaredDistance(_initialFrame.origin, _finalFrame.origin);
    CGFloat dist;
    
    dist = squaredDistance(NSMakePoint(NSMaxX(_initialFrame), NSMinY(_initialFrame)), NSMakePoint(NSMaxX(_finalFrame), NSMinY(_finalFrame)));
    if (dist > maxDist)
        maxDist = dist;
    
    dist = squaredDistance(NSMakePoint(NSMaxX(_initialFrame), NSMaxY(_initialFrame)), NSMakePoint(NSMaxX(_finalFrame), NSMaxY(_finalFrame)));
    if (dist > maxDist)
        maxDist = dist;
    
    dist = squaredDistance(NSMakePoint(NSMinX(_initialFrame), NSMinY(_initialFrame)), NSMakePoint(NSMinX(_finalFrame), NSMinY(_finalFrame)));
    if (dist > maxDist)
        maxDist = dist;
    
    return MIN(sqrt(maxDist) * speedFactor, maxAdditionalDuration);    
}

- (void)startAnimation
{
    // Compute extra time
    if (_hintedDuration)
        [self setDuration:_hintedDuration + [self additionalDurationNeededToReachFinalFrame]];
    [super startAnimation];
}

- (void)stopAnimation
{
    _window = nil;
    [super stopAnimation];
    [_subAnimation stopAnimation];
}

@end

@implementation WebWindowFadeAnimation

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    [self setAnimationBlockingMode:NSAnimationNonblockingThreaded];
    [self setFrameRate:60];
    [self setAnimationCurve:NSAnimationEaseInOut];
    return self;
}

- (id)initWithDuration:(NSTimeInterval)duration window:(NSWindow *)window initialAlpha:(CGFloat)initialAlpha finalAlpha:(CGFloat)finalAlpha
{
    self = [self init];
    if (!self)
        return nil;    
    _window = window;
    _initialAlpha = initialAlpha;
    _finalAlpha = finalAlpha;
    [self setDuration:duration];
    return self;
}

- (void)setDuration:(NSTimeInterval)duration
{
    [super setDuration:WebWindowAnimationDurationFromDuration(duration)];
}

- (CGFloat)currentAlpha
{
    return MAX(0, MIN(1, _initialAlpha + [self currentValue] * (_finalAlpha - _initialAlpha)));
}

- (void)setCurrentProgress:(NSAnimationProgress)progress
{
    if (_isStopped)
        return;

    ASSERT(_window);
    [super setCurrentProgress:progress];

    wkWindowSetAlpha(_window, [self currentAlpha]);
}

- (void)setWindow:(NSWindow*)window
{
    _window = window;
}

- (void)stopAnimation
{
    // This is relevant when we are a sub animation of a scale animation.
    // In this case we are hosted in the animated thread of the parent
    // and even after [super stopAnimation], the parent might call
    // setCurrrentProgress.
    _isStopped = YES;

    [super stopAnimation];
}

@end

