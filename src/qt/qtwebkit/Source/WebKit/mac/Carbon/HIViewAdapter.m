/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
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

#ifndef __LP64__

#import "HIViewAdapter.h"

#import "QuickDrawCompatibility.h"
#import "WebNSObjectExtras.h"
#import <wtf/Assertions.h>

static void SetViewNeedsDisplay(HIViewRef inView, RgnHandle inRegion, Boolean inNeedsDisplay);

#define WATCH_INVALIDATION 0

@interface NSView(ShhhhDontTell)
- (NSRect)_dirtyRect;
@end

@implementation HIViewAdapter

static CFMutableDictionaryRef sViewMap;

static IMP oldNSViewSetNeedsDisplayIMP;
static IMP oldNSViewSetNeedsDisplayInRectIMP;
static IMP oldNSViewNextValidKeyViewIMP;

static void _webkit_NSView_setNeedsDisplay(id self, SEL _cmd, BOOL flag);
static void _webkit_NSView_setNeedsDisplayInRect(id self, SEL _cmd, NSRect invalidRect);
static NSView *_webkit_NSView_nextValidKeyView(id self, SEL _cmd);

+ (void)bindHIViewToNSView:(HIViewRef)hiView nsView:(NSView*)nsView
{
    if (sViewMap == NULL) {
        sViewMap = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);

        // Override -[NSView setNeedsDisplay:]
        Method setNeedsDisplayMethod = class_getInstanceMethod(objc_getClass("NSView"), @selector(setNeedsDisplay:));
        ASSERT(setNeedsDisplayMethod);
        ASSERT(!oldNSViewSetNeedsDisplayIMP);
        oldNSViewSetNeedsDisplayIMP = method_setImplementation(setNeedsDisplayMethod, (IMP)_webkit_NSView_setNeedsDisplay);

        // Override -[NSView setNeedsDisplayInRect:]
        Method setNeedsDisplayInRectMethod = class_getInstanceMethod(objc_getClass("NSView"), @selector(setNeedsDisplayInRect:));
        ASSERT(setNeedsDisplayInRectMethod);
        ASSERT(!oldNSViewSetNeedsDisplayInRectIMP);
        oldNSViewSetNeedsDisplayInRectIMP = method_setImplementation(setNeedsDisplayInRectMethod, (IMP)_webkit_NSView_setNeedsDisplayInRect);

        // Override -[NSView nextValidKeyView]
        Method nextValidKeyViewMethod = class_getInstanceMethod(objc_getClass("NSView"), @selector(nextValidKeyView));
        ASSERT(nextValidKeyViewMethod);
        ASSERT(!oldNSViewNextValidKeyViewIMP);
        oldNSViewNextValidKeyViewIMP = method_setImplementation(nextValidKeyViewMethod, (IMP)_webkit_NSView_nextValidKeyView);
    }

    CFDictionaryAddValue(sViewMap, nsView, hiView);
}

+ (HIViewRef)getHIViewForNSView:(NSView*)inView
{
    return sViewMap ? (HIViewRef)CFDictionaryGetValue(sViewMap, inView) : NULL;
}

+ (void)unbindNSView:(NSView*)inView
{
    CFDictionaryRemoveValue(sViewMap, inView);
}

static void _webkit_NSView_setNeedsDisplay(id self, SEL _cmd, BOOL flag)
{
    oldNSViewSetNeedsDisplayIMP(self, _cmd, flag);

    if (!flag) {
        HIViewRef hiView = NULL;
        NSRect targetBounds = [self visibleRect];
        NSRect validRect = targetBounds;
        NSView *view = self;
        
        while (view) {
            targetBounds = [view visibleRect];
            if ((hiView = [HIViewAdapter getHIViewForNSView:view]) != NULL)
                break;
            validRect = [view convertRect:validRect toView:[view superview]];
            view = [view superview];
        }
        
        if (hiView) {            
            // Flip rect here and convert to region
            HIRect rect;
            rect.origin.x = validRect.origin.x;
            rect.origin.y = targetBounds.size.height - NSMaxY(validRect);
            rect.size.height = validRect.size.height;
            rect.size.width = validRect.size.width;
            
            // For now, call the region-based API.
            RgnHandle rgn = NewRgn();
            if (rgn) {
                Rect qdRect;
                qdRect.top = (SInt16)rect.origin.y;
                qdRect.left = (SInt16)rect.origin.x;
                qdRect.bottom = CGRectGetMaxY(rect);
                qdRect.right = CGRectGetMaxX(rect);
                
                RectRgn(rgn, &qdRect);
                SetViewNeedsDisplay(hiView, rgn, false);
                DisposeRgn(rgn);
            }
        }
    }
}

static void _webkit_NSView_setNeedsDisplayInRect(id self, SEL _cmd, NSRect invalidRect)
{
    invalidRect = NSUnionRect(invalidRect, [self _dirtyRect]);
    oldNSViewSetNeedsDisplayInRectIMP(self, _cmd, invalidRect);
    
    if (!NSIsEmptyRect(invalidRect)) {
        HIViewRef hiView = NULL;
        NSRect targetBounds = [(NSView *)self bounds];
        NSView *view = self;
        
        while (view) {
            targetBounds = [view bounds];
            if ((hiView = [HIViewAdapter getHIViewForNSView:view]) != NULL)
                break;
            invalidRect = [view convertRect:invalidRect toView:[view superview]];
            view = [view superview];
        }
        
        if (hiView) {
            if (NSWidth(invalidRect) > 0 && NSHeight(invalidRect)) {                
                // Flip rect here and convert to region
                HIRect rect;
                rect.origin.x = invalidRect.origin.x;
                rect.origin.y = targetBounds.size.height - NSMaxY(invalidRect);
                rect.size.height = invalidRect.size.height;
                rect.size.width = invalidRect.size.width;
                
                // For now, call the region-based API.
                RgnHandle rgn = NewRgn();
                if (rgn) {
                    Rect qdRect;
                    qdRect.top = (SInt16)rect.origin.y;
                    qdRect.left = (SInt16)rect.origin.x;
                    qdRect.bottom = CGRectGetMaxY(rect);
                    qdRect.right = CGRectGetMaxX(rect);
                    
                    RectRgn(rgn, &qdRect);
                    SetViewNeedsDisplay(hiView, rgn, true);
                    DisposeRgn(rgn);
                }
            }
        } else
            [[self window] setViewsNeedDisplay:YES];
    }
}

static NSView *_webkit_NSView_nextValidKeyView(id self, SEL _cmd)
{
    if ([HIViewAdapter getHIViewForNSView:self])
        return [[self window] contentView];
    else
        return oldNSViewNextValidKeyViewIMP(self, _cmd);
}

@end

static void SetViewNeedsDisplay(HIViewRef inHIView, RgnHandle inRegion, Boolean inNeedsDisplay)
{
    WindowAttributes attrs;
    
    GetWindowAttributes(GetControlOwner(inHIView), &attrs);
    
    if (attrs & kWindowCompositingAttribute) {
#if WATCH_INVALIDATION
        Rect bounds;
        GetRegionBounds(inRegion, &bounds);
        printf("%s: rect on input %d %d %d %d\n", inNeedsDisplay ? "INVALIDATE" : "VALIDATE",
                bounds.top, bounds.left, bounds.bottom, bounds.right);
#endif
        HIViewSetNeedsDisplayInRegion(inHIView, inRegion, inNeedsDisplay);
    } else {
        Rect bounds, cntlBounds;
        GrafPtr port, savePort;
        Rect portBounds;
        
#if WATCH_INVALIDATION
        printf("%s: rect on input %d %d %d %d\n", inNeedsDisplay ? "INVALIDATE" : "VALIDATE",
                bounds.top, bounds.left, bounds.bottom, bounds.right);
#endif
        GetControlBounds(inHIView, &cntlBounds);
        
#if WATCH_INVALIDATION
        printf("%s: control bounds are %d %d %d %d\n", inNeedsDisplay ? "INVALIDATE" : "VALIDATE",
                cntlBounds.top, cntlBounds.left, cntlBounds.bottom, cntlBounds.right);
#endif
        
        port = GetWindowPort(GetControlOwner(inHIView));
        
        GetPort(&savePort);
        SetPort(port);
        GetPortBounds(port, &portBounds);
        SetOrigin(0, 0);
        
        OffsetRgn(inRegion, cntlBounds.left, cntlBounds.top);
        
        GetRegionBounds(inRegion, &bounds);
        
#if WATCH_INVALIDATION
        printf("%s: rect in port coords %d %d %d %d\n", inNeedsDisplay ? "INVALIDATE" : "VALIDATE",
                bounds.top, bounds.left, bounds.bottom, bounds.right);
#endif
        
        if (inNeedsDisplay)
            InvalWindowRgn(GetControlOwner(inHIView), inRegion);
        else
            ValidWindowRgn(GetControlOwner(inHIView), inRegion);
        
        SetOrigin(portBounds.left, portBounds.top);
        SetPort(savePort);
    }
}

#endif
