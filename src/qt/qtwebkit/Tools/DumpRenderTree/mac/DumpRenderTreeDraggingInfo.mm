/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "DumpRenderTreeDraggingInfo.h"

#import "DumpRenderTree.h"
#import "EventSendingController.h"
#import <WebKit/WebKit.h>

@implementation DumpRenderTreeDraggingInfo

- (id)initWithImage:(NSImage *)anImage offset:(NSSize)o pasteboard:(NSPasteboard *)pboard source:(id)source
{
    draggedImage = [anImage retain];
    draggingPasteboard = [pboard retain];
    draggingSource = [source retain];
    offset = o;
    
    return [super init];
}

- (void)dealloc
{
    [draggedImage release];
    [draggingPasteboard release];
    [draggingSource release];
    [super dealloc];
}

- (NSWindow *)draggingDestinationWindow 
{
    return [[mainFrame webView] window];
}

- (NSDragOperation)draggingSourceOperationMask 
{
    return [draggingSource draggingSourceOperationMaskForLocal:YES];
}

- (NSPoint)draggingLocation
{ 
    return lastMousePosition; 
}

- (NSPoint)draggedImageLocation 
{
    return NSMakePoint(lastMousePosition.x + offset.width, lastMousePosition.y + offset.height);
}

- (NSImage *)draggedImage
{
    return draggedImage;
}

- (NSPasteboard *)draggingPasteboard
{
    return draggingPasteboard;
}

- (id)draggingSource
{
    return draggingSource;
}

- (int)draggingSequenceNumber
{
    NSLog(@"DumpRenderTree doesn't support draggingSequenceNumber");
    return 0;
}

- (void)slideDraggedImageTo:(NSPoint)screenPoint
{
    NSLog(@"DumpRenderTree doesn't support slideDraggedImageTo:");
}

- (NSArray *)namesOfPromisedFilesDroppedAtDestination:(NSURL *)dropDestination
{
    NSLog(@"DumpRenderTree doesn't support namesOfPromisedFilesDroppedAtDestination:");
    return nil;
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
- (NSDraggingFormation)draggingFormation
{
    return NSDraggingFormationDefault;
}

- (void)setDraggingFormation:(NSDraggingFormation)formation
{
    // Ignored.
}

- (BOOL)animatesToDestination
{
    return NO;
}

- (void)setAnimatesToDestination:(BOOL)flag
{
    // Ignored.
}

- (NSInteger)numberOfValidItemsForDrop
{
    return 1;
}

- (void)setNumberOfValidItemsForDrop:(NSInteger)number
{
    // Ignored.
}

- (void)enumerateDraggingItemsWithOptions:(NSEnumerationOptions)enumOpts forView:(NSView *)view classes:(NSArray *)classArray searchOptions:(NSDictionary *)searchOptions usingBlock:(void (^)(NSDraggingItem *draggingItem, NSInteger idx, BOOL *stop))block
{
    // Ignored.
}
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

@end

