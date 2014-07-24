/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#import "WebDynamicScrollBarsView.h"
#import <WebCore/WebCoreFrameView.h>

@interface WebDynamicScrollBarsView (WebInternal) <WebCoreFrameScrollView>

- (BOOL)allowsHorizontalScrolling;
- (BOOL)allowsVerticalScrolling;

- (void)setScrollingModes:(WebCore::ScrollbarMode)hMode vertical:(WebCore::ScrollbarMode)vMode andLock:(BOOL)lock;
- (void)scrollingModes:(WebCore::ScrollbarMode*)hMode vertical:(WebCore::ScrollbarMode*)vMode;

- (WebCore::ScrollbarMode)horizontalScrollingMode;
- (WebCore::ScrollbarMode)verticalScrollingMode;

- (void)setHorizontalScrollingMode:(WebCore::ScrollbarMode)mode andLock:(BOOL)lock;
- (void)setVerticalScrollingMode:(WebCore::ScrollbarMode)mode andLock:(BOOL)lock;

- (void)setHorizontalScrollingModeLocked:(BOOL)locked;
- (void)setVerticalScrollingModeLocked:(BOOL)locked;
- (void)setScrollingModesLocked:(BOOL)mode;

- (BOOL)horizontalScrollingModeLocked;
- (BOOL)verticalScrollingModeLocked;

- (void)updateScrollers;
- (void)setSuppressLayout:(BOOL)flag;

// Calculate the appropriate frame for the contentView based on allowsScrollersToOverlapContent.
- (NSRect)contentViewFrame;

// Returns YES if we're currently in the middle of programmatically moving the
// scrollbar.
// NOTE: As opposed to other places in the code, programmatically moving the
// scrollers from inside this class should not fire JS events.
- (BOOL)inProgrammaticScroll;
@end
