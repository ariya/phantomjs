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

#import <WebKit/WebNSViewExtras.h>

#import <WebKit/DOMExtensions.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebFrameViewInternal.h>
#import <WebKit/WebNSImageExtras.h>
#import <WebKit/WebNSPasteboardExtras.h>
#import <WebKit/WebNSURLExtras.h>
#import <WebKit/WebView.h>

#define WebDragStartHysteresisX                 5.0f
#define WebDragStartHysteresisY                 5.0f
#define WebMaxDragImageSize                     NSMakeSize(400.0f, 400.0f)
#define WebMaxOriginalImageArea                 (1500.0f * 1500.0f)
#define WebDragIconRightInset                   7.0f
#define WebDragIconBottomInset                  3.0f

@implementation NSView (WebExtras)

- (NSView *)_web_superviewOfClass:(Class)class
{
    NSView *view = [self superview];
    while (view  && ![view isKindOfClass:class])
        view = [view superview];
    return view;
}

- (WebFrameView *)_web_parentWebFrameView
{
    return (WebFrameView *)[self _web_superviewOfClass:[WebFrameView class]];
}

// FIXME: Mail is the only client of _webView, remove this method once no versions of Mail need it.
- (WebView *)_webView
{
    return (WebView *)[self _web_superviewOfClass:[WebView class]];
}

/* Determine whether a mouse down should turn into a drag; started as copy of NSTableView code */
- (BOOL)_web_dragShouldBeginFromMouseDown:(NSEvent *)mouseDownEvent
                           withExpiration:(NSDate *)expiration
                              xHysteresis:(float)xHysteresis
                              yHysteresis:(float)yHysteresis
{
    NSEvent *nextEvent, *firstEvent, *dragEvent, *mouseUp;
    BOOL dragIt;

    if ([mouseDownEvent type] != NSLeftMouseDown) {
        return NO;
    }

    nextEvent = nil;
    firstEvent = nil;
    dragEvent = nil;
    mouseUp = nil;
    dragIt = NO;

    while ((nextEvent = [[self window] nextEventMatchingMask:(NSLeftMouseUpMask | NSLeftMouseDraggedMask)
                                                   untilDate:expiration
                                                      inMode:NSEventTrackingRunLoopMode
                                                     dequeue:YES]) != nil) {
        if (firstEvent == nil) {
            firstEvent = nextEvent;
        }

        if ([nextEvent type] == NSLeftMouseDragged) {
            float deltax = ABS([nextEvent locationInWindow].x - [mouseDownEvent locationInWindow].x);
            float deltay = ABS([nextEvent locationInWindow].y - [mouseDownEvent locationInWindow].y);
            dragEvent = nextEvent;

            if (deltax >= xHysteresis) {
                dragIt = YES;
                break;
            }

            if (deltay >= yHysteresis) {
                dragIt = YES;
                break;
            }
        } else if ([nextEvent type] == NSLeftMouseUp) {
            mouseUp = nextEvent;
            break;
        }
    }

    // Since we've been dequeuing the events (If we don't, we'll never see the mouse up...),
    // we need to push some of the events back on.  It makes sense to put the first and last
    // drag events and the mouse up if there was one.
    if (mouseUp != nil) {
        [NSApp postEvent:mouseUp atStart:YES];
    }
    if (dragEvent != nil) {
        [NSApp postEvent:dragEvent atStart:YES];
    }
    if (firstEvent != mouseUp && firstEvent != dragEvent) {
        [NSApp postEvent:firstEvent atStart:YES];
    }

    return dragIt;
}

- (BOOL)_web_dragShouldBeginFromMouseDown:(NSEvent *)mouseDownEvent
                           withExpiration:(NSDate *)expiration
{
    return [self _web_dragShouldBeginFromMouseDown:mouseDownEvent
                                    withExpiration:expiration
                                       xHysteresis:WebDragStartHysteresisX
                                       yHysteresis:WebDragStartHysteresisY];
}


- (NSDragOperation)_web_dragOperationForDraggingInfo:(id <NSDraggingInfo>)sender
{
    if (![NSApp modalWindow] && 
        ![[self window] attachedSheet] &&
        [sender draggingSource] != self &&
        [[sender draggingPasteboard] _web_bestURL]) {

        return NSDragOperationCopy;
    }
    
    return NSDragOperationNone;
}

- (void)_web_DragImageForElement:(DOMElement *)element
                         rect:(NSRect)rect
                        event:(NSEvent *)event
                   pasteboard:(NSPasteboard *)pasteboard 
                       source:(id)source
                       offset:(NSPoint *)dragImageOffset
{
    NSPoint mouseDownPoint = [self convertPoint:[event locationInWindow] fromView:nil];
    NSImage *dragImage;
    NSPoint origin;

    NSImage *image = [element image];
    if (image != nil && [image size].height * [image size].width <= WebMaxOriginalImageArea) {
        NSSize originalSize = rect.size;
        origin = rect.origin;
        
        dragImage = [[image copy] autorelease];
        [dragImage setScalesWhenResized:YES];
        [dragImage setSize:originalSize];
        
        [dragImage _web_scaleToMaxSize:WebMaxDragImageSize];
        NSSize newSize = [dragImage size];
        
        [dragImage _web_dissolveToFraction:WebDragImageAlpha];
        
        // Properly orient the drag image and orient it differently if it's smaller than the original
        origin.x = mouseDownPoint.x - (((mouseDownPoint.x - origin.x) / originalSize.width) * newSize.width);
        origin.y = origin.y + originalSize.height;
        origin.y = mouseDownPoint.y - (((mouseDownPoint.y - origin.y) / originalSize.height) * newSize.height);
    } else {
        // FIXME: This has been broken for a while.
        // There's no way to get the MIME type for the image from a DOM element.
        // The old code used WKGetPreferredExtensionForMIMEType([image MIMEType]);
        NSString *extension = @"";
        dragImage = [[NSWorkspace sharedWorkspace] iconForFileType:extension];
        NSSize offset = NSMakeSize([dragImage size].width - WebDragIconRightInset, -WebDragIconBottomInset);
        origin = NSMakePoint(mouseDownPoint.x - offset.width, mouseDownPoint.y - offset.height);
    }

    // This is the offset from the lower left corner of the image to the mouse location.  Because we
    // are a flipped view the calculation of Y is inverted.
    if (dragImageOffset) {
        dragImageOffset->x = mouseDownPoint.x - origin.x;
        dragImageOffset->y = origin.y - mouseDownPoint.y;
    }
    
    // Per kwebster, offset arg is ignored
    [self dragImage:dragImage at:origin offset:NSZeroSize event:event pasteboard:pasteboard source:source slideBack:YES];
}

- (BOOL)_web_firstResponderIsSelfOrDescendantView
{
    NSResponder *responder = [[self window] firstResponder];
    return (responder && 
           (responder == self || 
           ([responder isKindOfClass:[NSView class]] && [(NSView *)responder isDescendantOf:self])));
}

- (NSRect)_web_convertRect:(NSRect)aRect toView:(NSView *)aView
{
    // Converting to this view's window; let -convertRect:toView: handle it
    if (aView == nil)
        return [self convertRect:aRect toView:nil];
        
    // This view must be in a window.  Do whatever weird thing -convertRect:toView: does in this situation.
    NSWindow *thisWindow = [self window];
    if (!thisWindow)
        return [self convertRect:aRect toView:aView];
    
    // The other view must be in a window, too.
    NSWindow *otherWindow = [aView window];
    if (!otherWindow)
        return [self convertRect:aRect toView:aView];

    // Convert to this window's coordinates
    NSRect convertedRect = [self convertRect:aRect toView:nil];
    
    // Convert to screen coordinates
    convertedRect.origin = [thisWindow convertBaseToScreen:convertedRect.origin];
    
    // Convert to other window's coordinates
    convertedRect.origin = [otherWindow convertScreenToBase:convertedRect.origin];
    
    // Convert to other view's coordinates
    convertedRect = [aView convertRect:convertedRect fromView:nil];
    
    return convertedRect;
}

@end
