/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import <WebKit2/WKView.h>

typedef enum {
    WKContentAnchorTopLeft,
    WKContentAnchorTopRight,
    WKContentAnchorBottomLeft,
    WKContentAnchorBottomRight,
} WKContentAnchor;

@interface WKView (Private)

/* C SPI support. */

@property(readonly) WKPageRef pageRef;
@property WKContentAnchor contentAnchor;

- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef;
- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef relatedToPage:(WKPageRef)relatedPage;

- (NSPrintOperation *)printOperationWithPrintInfo:(NSPrintInfo *)printInfo forFrame:(WKFrameRef)frameRef;
- (BOOL)canChangeFrameLayout:(WKFrameRef)frameRef;

- (void)setFrame:(NSRect)rect andScrollBy:(NSSize)offset;

// Stops updating the size of the page as the WKView frame size updates.
// This should always be followed by enableFrameSizeUpdates. Calls can be nested.
- (void)disableFrameSizeUpdates;
// Immediately updates the size of the page to match WKView's frame size
// and allows subsequent updates as the frame size is set. Calls can be nested.
- (void)enableFrameSizeUpdates;
- (BOOL)frameSizeUpdatesDisabled;

- (void)performDictionaryLookupAtCurrentMouseLocation;
+ (void)hideWordDefinitionWindow;

@property (readwrite) CGFloat minimumLayoutWidth;
@property (readwrite) CGFloat minimumWidthForAutoLayout;
@property (readwrite) NSSize minimumSizeForAutoLayout;
@property (readwrite) BOOL shouldClipToVisibleRect;

@property(copy, nonatomic) NSColor *underlayColor;

- (NSView*)fullScreenPlaceholderView;
- (NSWindow*)createFullScreenWindow;

- (void)beginDeferringViewInWindowChanges;
- (void)endDeferringViewInWindowChanges;
- (void)endDeferringViewInWindowChangesSync;
- (BOOL)isDeferringViewInWindowChanges;

- (BOOL)windowOcclusionDetectionEnabled;
- (void)setWindowOcclusionDetectionEnabled:(BOOL)flag;

- (void)forceAsyncDrawingAreaSizeUpdate:(NSSize)size;
- (void)waitForAsyncDrawingAreaSizeUpdate;

@end
