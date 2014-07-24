/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Jonas Witt <jonas.witt@gmail.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
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
#import "EventSendingController.h"

#import "DumpRenderTree.h"
#import "DumpRenderTreeDraggingInfo.h"
#import "DumpRenderTreeFileDraggingSource.h"

#import <Carbon/Carbon.h>                           // for GetCurrentEventTime()
#import <WebKit/DOMPrivate.h>
#import <WebKit/WebKit.h>
#import <WebKit/WebViewPrivate.h>

extern "C" void _NSNewKillRingSequence();

enum MouseAction {
    MouseDown,
    MouseUp,
    MouseDragged
};

// Match the DOM spec (sadly the DOM spec does not provide an enum)
enum MouseButton {
    LeftMouseButton = 0,
    MiddleMouseButton = 1,
    RightMouseButton = 2,
    NoMouseButton = -1
};

struct KeyMappingEntry {
    int macKeyCode;
    int macNumpadKeyCode;
    unichar character;
    NSString* characterName;
};

NSPoint lastMousePosition;
NSPoint lastClickPosition;
int lastClickButton = NoMouseButton;
NSArray *webkitDomEventNames;
NSMutableArray *savedMouseEvents; // mouse events sent between mouseDown and mouseUp are stored here, and then executed at once.
BOOL replayingSavedEvents;

@implementation EventSendingController

+ (void)initialize
{
    webkitDomEventNames = [[NSArray alloc] initWithObjects:
        @"abort",
        @"beforecopy",
        @"beforecut",
        @"beforepaste",
        @"blur",
        @"change",
        @"click",
        @"contextmenu",
        @"copy",
        @"cut",
        @"dblclick",
        @"drag",
        @"dragend",
        @"dragenter",
        @"dragleave",
        @"dragover",
        @"dragstart",
        @"drop",
        @"error",
        @"focus",
        @"input",
        @"keydown",
        @"keypress",
        @"keyup",
        @"load",
        @"mousedown",
        @"mousemove",
        @"mouseout",
        @"mouseover",
        @"mouseup",
        @"mousewheel",
        @"beforeunload",
        @"paste",
        @"readystatechange",
        @"reset",
        @"resize", 
        @"scroll", 
        @"search",
        @"select",
        @"selectstart",
        @"submit", 
        @"textInput", 
        @"textzoomin",
        @"textzoomout",
        @"unload",
        @"zoom",
        nil];
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    if (aSelector == @selector(beginDragWithFiles:)
            || aSelector == @selector(clearKillRing)
            || aSelector == @selector(contextClick)
            || aSelector == @selector(enableDOMUIEventLogging:)
            || aSelector == @selector(fireKeyboardEventsToElement:)
            || aSelector == @selector(keyDown:withModifiers:withLocation:)
            || aSelector == @selector(leapForward:)
            || aSelector == @selector(mouseDown:withModifiers:)
            || aSelector == @selector(mouseMoveToX:Y:)
            || aSelector == @selector(mouseUp:withModifiers:)
            || aSelector == @selector(scheduleAsynchronousClick)
            || aSelector == @selector(scheduleAsynchronousKeyDown:withModifiers:withLocation:)
            || aSelector == @selector(textZoomIn)
            || aSelector == @selector(textZoomOut)
            || aSelector == @selector(zoomPageIn)
            || aSelector == @selector(zoomPageOut)
            || aSelector == @selector(scalePageBy:atX:andY:)
            || aSelector == @selector(mouseScrollByX:andY:)
            || aSelector == @selector(continuousMouseScrollByX:andY:))
        return NO;
    return YES;
}

+ (BOOL)isKeyExcludedFromWebScript:(const char*)name
{
    if (strcmp(name, "dragMode") == 0)
        return NO;
    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(beginDragWithFiles:))
        return @"beginDragWithFiles";
    if (aSelector == @selector(contextClick))
        return @"contextClick";
    if (aSelector == @selector(enableDOMUIEventLogging:))
        return @"enableDOMUIEventLogging";
    if (aSelector == @selector(fireKeyboardEventsToElement:))
        return @"fireKeyboardEventsToElement";
    if (aSelector == @selector(keyDown:withModifiers:withLocation:))
        return @"keyDown";
    if (aSelector == @selector(scheduleAsynchronousKeyDown:withModifiers:withLocation:))
        return @"scheduleAsynchronousKeyDown";
    if (aSelector == @selector(leapForward:))
        return @"leapForward";
    if (aSelector == @selector(mouseDown:withModifiers:))
        return @"mouseDown";
    if (aSelector == @selector(mouseUp:withModifiers:))
        return @"mouseUp";
    if (aSelector == @selector(mouseMoveToX:Y:))
        return @"mouseMoveTo";
    if (aSelector == @selector(setDragMode:))
        return @"setDragMode";
    if (aSelector == @selector(mouseScrollByX:andY:))
        return @"mouseScrollBy";
    if (aSelector == @selector(continuousMouseScrollByX:andY:))
        return @"continuousMouseScrollBy";
    if (aSelector == @selector(scalePageBy:atX:andY:))
        return @"scalePageBy";
    return nil;
}

- (id)init
{
    self = [super init];
    if (self)
        dragMode = YES;
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (double)currentEventTime
{
    return GetCurrentEventTime() + timeOffset;
}

- (void)leapForward:(int)milliseconds
{
    if (dragMode && leftMouseButtonDown && !replayingSavedEvents) {
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[EventSendingController instanceMethodSignatureForSelector:@selector(leapForward:)]];
        [invocation setTarget:self];
        [invocation setSelector:@selector(leapForward:)];
        [invocation setArgument:&milliseconds atIndex:2];
        
        [EventSendingController saveEvent:invocation];
        
        return;
    }

    timeOffset += milliseconds / 1000.0;
}

- (void)clearKillRing
{
    _NSNewKillRingSequence();
}

static NSEventType eventTypeForMouseButtonAndAction(int button, MouseAction action)
{
    switch (button) {
        case LeftMouseButton:
            switch (action) {
                case MouseDown:
                    return NSLeftMouseDown;
                case MouseUp:
                    return NSLeftMouseUp;
                case MouseDragged:
                    return NSLeftMouseDragged;
            }
        case RightMouseButton:
            switch (action) {
                case MouseDown:
                    return NSRightMouseDown;
                case MouseUp:
                    return NSRightMouseUp;
                case MouseDragged:
                    return NSRightMouseDragged;
            }
        default:
            switch (action) {
                case MouseDown:
                    return NSOtherMouseDown;
                case MouseUp:
                    return NSOtherMouseUp;
                case MouseDragged:
                    return NSOtherMouseDragged;
            }
    }
    assert(0);
    return static_cast<NSEventType>(0);
}

- (void)beginDragWithFiles:(WebScriptObject*)jsFilePaths
{
    assert(!draggingInfo);
    assert([jsFilePaths isKindOfClass:[WebScriptObject class]]);

    NSPasteboard *pboard = [NSPasteboard pasteboardWithUniqueName];
    [pboard declareTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:nil];

    NSURL *currentTestURL = [NSURL URLWithString:[[mainFrame webView] mainFrameURL]];

    NSMutableArray *filePaths = [NSMutableArray array];
    for (unsigned i = 0; [[jsFilePaths webScriptValueAtIndex:i] isKindOfClass:[NSString class]]; i++) {
        NSString *filePath = (NSString *)[jsFilePaths webScriptValueAtIndex:i];
        // Have NSURL encode the name so that we handle '?' in file names correctly.
        NSURL *fileURL = [NSURL fileURLWithPath:filePath];
        NSURL *absoluteFileURL = [NSURL URLWithString:[fileURL relativeString]  relativeToURL:currentTestURL];
        [filePaths addObject:[absoluteFileURL path]];
    }

    [pboard setPropertyList:filePaths forType:NSFilenamesPboardType];
    assert([pboard propertyListForType:NSFilenamesPboardType]); // setPropertyList will silently fail on error, assert that it didn't fail

    // Provide a source, otherwise [DumpRenderTreeDraggingInfo draggingSourceOperationMask] defaults to NSDragOperationNone
    DumpRenderTreeFileDraggingSource *source = [[[DumpRenderTreeFileDraggingSource alloc] init] autorelease];
    draggingInfo = [[DumpRenderTreeDraggingInfo alloc] initWithImage:nil offset:NSZeroSize pasteboard:pboard source:source];
    [[mainFrame webView] draggingEntered:draggingInfo];

    dragMode = NO; // dragMode saves events and then replays them later.  We don't need/want that.
    leftMouseButtonDown = YES; // Make the rest of eventSender think a drag is in progress
}

- (void)updateClickCountForButton:(int)buttonNumber
{
    if (([self currentEventTime] - lastClick >= 1) ||
        !NSEqualPoints(lastMousePosition, lastClickPosition) ||
        lastClickButton != buttonNumber) {
        clickCount = 1;
        lastClickButton = buttonNumber;
    } else
        clickCount++;
}

static int modifierFlags(const NSString* modifierName)
{
    int flags = 0;
    if ([modifierName isEqual:@"ctrlKey"])
        flags |= NSControlKeyMask;
    else if ([modifierName isEqual:@"shiftKey"] || [modifierName isEqual:@"rangeSelectionKey"])
        flags |= NSShiftKeyMask;
    else if ([modifierName isEqual:@"altKey"])
        flags |= NSAlternateKeyMask;
    else if ([modifierName isEqual:@"metaKey"] || [modifierName isEqual:@"addSelectionKey"])
        flags |= NSCommandKeyMask;

    return flags;
}

static int buildModifierFlags(const WebScriptObject* modifiers)
{
    int flags = 0;
    if ([modifiers isKindOfClass:[NSString class]])
        return modifierFlags((NSString*)modifiers);
    else if (![modifiers isKindOfClass:[WebScriptObject class]])
        return flags;
    for (unsigned i = 0; [[modifiers webScriptValueAtIndex:i] isKindOfClass:[NSString class]]; i++) {
        NSString* modifierName = (NSString*)[modifiers webScriptValueAtIndex:i];
        flags |= modifierFlags(modifierName);
    }
    return flags;
}

- (void)mouseDown:(int)buttonNumber withModifiers:(WebScriptObject*)modifiers
{
    [[[mainFrame frameView] documentView] layout];
    [self updateClickCountForButton:buttonNumber];
    
    NSEventType eventType = eventTypeForMouseButtonAndAction(buttonNumber, MouseDown);
    NSEvent *event = [NSEvent mouseEventWithType:eventType
                                        location:lastMousePosition 
                                   modifierFlags:buildModifierFlags(modifiers)
                                       timestamp:[self currentEventTime]
                                    windowNumber:[[[mainFrame webView] window] windowNumber] 
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:clickCount 
                                        pressure:0.0];

    NSView *subView = [[mainFrame webView] hitTest:[event locationInWindow]];
    if (subView) {
        [subView mouseDown:event];
        if (buttonNumber == LeftMouseButton)
            leftMouseButtonDown = YES;
    }
}

- (void)mouseDown:(int)buttonNumber
{
    [self mouseDown:buttonNumber withModifiers:nil];
}

- (void)textZoomIn
{
    [[mainFrame webView] makeTextLarger:self];
}

- (void)textZoomOut
{
    [[mainFrame webView] makeTextSmaller:self];
}

- (void)zoomPageIn
{
    [[mainFrame webView] zoomPageIn:self];
}

- (void)zoomPageOut
{
    [[mainFrame webView] zoomPageOut:self];
}

- (void)scalePageBy:(float)scale atX:(float)x andY:(float)y
{
    [[mainFrame webView] _scaleWebView:scale atOrigin:NSMakePoint(x, y)];
}

- (void)mouseUp:(int)buttonNumber withModifiers:(WebScriptObject*)modifiers
{
    if (dragMode && !replayingSavedEvents) {
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[EventSendingController instanceMethodSignatureForSelector:@selector(mouseUp:withModifiers:)]];
        [invocation setTarget:self];
        [invocation setSelector:@selector(mouseUp:withModifiers:)];
        [invocation setArgument:&buttonNumber atIndex:2];
        [invocation setArgument:&modifiers atIndex:3];
        
        [EventSendingController saveEvent:invocation];
        [EventSendingController replaySavedEvents];

        return;
    }

    [[[mainFrame frameView] documentView] layout];
    NSEventType eventType = eventTypeForMouseButtonAndAction(buttonNumber, MouseUp);
    NSEvent *event = [NSEvent mouseEventWithType:eventType
                                        location:lastMousePosition 
                                   modifierFlags:buildModifierFlags(modifiers)
                                       timestamp:[self currentEventTime]
                                    windowNumber:[[[mainFrame webView] window] windowNumber] 
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:clickCount 
                                        pressure:0.0];

    NSView *targetView = [[mainFrame webView] hitTest:[event locationInWindow]];
    // FIXME: Silly hack to teach DRT to respect capturing mouse events outside the WebView.
    // The right solution is just to use NSApplication's built-in event sending methods, 
    // instead of rolling our own algorithm for selecting an event target.
    targetView = targetView ? targetView : [[mainFrame frameView] documentView];
    assert(targetView);
    [targetView mouseUp:event];
    if (buttonNumber == LeftMouseButton)
        leftMouseButtonDown = NO;
    lastClick = [event timestamp];
    lastClickPosition = lastMousePosition;
    if (draggingInfo) {
        WebView *webView = [mainFrame webView];
        
        NSDragOperation dragOperation = [webView draggingUpdated:draggingInfo];
        
        if (dragOperation != NSDragOperationNone)
            [webView performDragOperation:draggingInfo];
        else
            [webView draggingExited:draggingInfo];
        // Per NSDragging.h: draggingSources may not implement draggedImage:endedAt:operation:
        if ([[draggingInfo draggingSource] respondsToSelector:@selector(draggedImage:endedAt:operation:)])
            [[draggingInfo draggingSource] draggedImage:[draggingInfo draggedImage] endedAt:lastMousePosition operation:dragOperation];
        [draggingInfo release];
        draggingInfo = nil;
    }
}

- (void)mouseUp:(int)buttonNumber
{
    [self mouseUp:buttonNumber withModifiers:nil];
}

- (void)mouseMoveToX:(int)x Y:(int)y
{
    if (dragMode && leftMouseButtonDown && !replayingSavedEvents) {
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[EventSendingController instanceMethodSignatureForSelector:@selector(mouseMoveToX:Y:)]];
        [invocation setTarget:self];
        [invocation setSelector:@selector(mouseMoveToX:Y:)];
        [invocation setArgument:&x atIndex:2];
        [invocation setArgument:&y atIndex:3];
        
        [EventSendingController saveEvent:invocation];
        return;
    }

    NSView *view = [mainFrame webView];
    lastMousePosition = [view convertPoint:NSMakePoint(x, [view frame].size.height - y) toView:nil];
    NSEvent *event = [NSEvent mouseEventWithType:(leftMouseButtonDown ? NSLeftMouseDragged : NSMouseMoved)
                                        location:lastMousePosition 
                                   modifierFlags:0 
                                       timestamp:[self currentEventTime]
                                    windowNumber:[[view window] windowNumber] 
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:(leftMouseButtonDown ? clickCount : 0) 
                                        pressure:0.0];

    NSView *subView = [[mainFrame webView] hitTest:[event locationInWindow]];
    if (subView) {
        if (leftMouseButtonDown) {
            if (draggingInfo) {
                // Per NSDragging.h: draggingSources may not implement draggedImage:movedTo:
                if ([[draggingInfo draggingSource] respondsToSelector:@selector(draggedImage:movedTo:)])
                    [[draggingInfo draggingSource] draggedImage:[draggingInfo draggedImage] movedTo:lastMousePosition];
                [[mainFrame webView] draggingUpdated:draggingInfo];
            } else
                [subView mouseDragged:event];
        } else
            [subView mouseMoved:event];
    }
}

- (void)mouseScrollByX:(int)x andY:(int)y continuously:(BOOL)c
{
    CGScrollEventUnit unit = c?kCGScrollEventUnitPixel:kCGScrollEventUnitLine;
    CGEventRef cgScrollEvent = CGEventCreateScrollWheelEvent(NULL, unit, 2, y, x);
    
    // CGEvent locations are in global display coordinates.
    CGPoint lastGlobalMousePosition = {
        lastMousePosition.x,
        [[NSScreen mainScreen] frame].size.height - lastMousePosition.y
    };
    CGEventSetLocation(cgScrollEvent, lastGlobalMousePosition);

    NSEvent *scrollEvent = [NSEvent eventWithCGEvent:cgScrollEvent];
    CFRelease(cgScrollEvent);

    NSView *subView = [[mainFrame webView] hitTest:[scrollEvent locationInWindow]];
    if (subView)
        [subView scrollWheel:scrollEvent];
}

- (void)continuousMouseScrollByX:(int)x andY:(int)y
{
    [self mouseScrollByX:x andY:y continuously:YES];
}

- (void)mouseScrollByX:(int)x andY:(int)y
{
    [self mouseScrollByX:x andY:y continuously:NO];
}

- (NSArray *)contextClick
{
    [[[mainFrame frameView] documentView] layout];
    [self updateClickCountForButton:RightMouseButton];

    NSEvent *event = [NSEvent mouseEventWithType:NSRightMouseDown
                                        location:lastMousePosition 
                                   modifierFlags:0 
                                       timestamp:[self currentEventTime]
                                    windowNumber:[[[mainFrame webView] window] windowNumber] 
                                         context:[NSGraphicsContext currentContext] 
                                     eventNumber:++eventNumber 
                                      clickCount:clickCount 
                                        pressure:0.0];

    NSView *subView = [[mainFrame webView] hitTest:[event locationInWindow]];
    NSMutableArray *menuItemStrings = [NSMutableArray array];
    
    if (subView) {
        NSMenu* menu = [subView menuForEvent:event];

        for (int i = 0; i < [menu numberOfItems]; ++i) {
            NSMenuItem* menuItem = [menu itemAtIndex:i];
            if (!strcmp("Inspect Element", [[menuItem title] UTF8String]))
                continue;

            if ([menuItem isSeparatorItem])
                [menuItemStrings addObject:@"<separator>"];
            else
                [menuItemStrings addObject:[menuItem title]];
        }
    }
    
    return menuItemStrings;
}

- (void)scheduleAsynchronousClick
{
    [self performSelector:@selector(mouseDown:) withObject:nil afterDelay:0];
    [self performSelector:@selector(mouseUp:) withObject:nil afterDelay:0];
}

+ (void)saveEvent:(NSInvocation *)event
{
    if (!savedMouseEvents)
        savedMouseEvents = [[NSMutableArray alloc] init];
    [savedMouseEvents addObject:event];
}

+ (void)replaySavedEvents
{
    replayingSavedEvents = YES;
    while ([savedMouseEvents count]) {
        // if a drag is initiated, the remaining saved events will be dispatched from our dragging delegate
        NSInvocation *invocation = [[[savedMouseEvents objectAtIndex:0] retain] autorelease];
        [savedMouseEvents removeObjectAtIndex:0];
        [invocation invoke];
    }
    replayingSavedEvents = NO;
}

+ (void)clearSavedEvents
{
    [savedMouseEvents release];
    savedMouseEvents = nil;
}

- (void)keyDown:(NSString *)character withModifiers:(WebScriptObject *)modifiers withLocation:(unsigned long)location
{
    NSString *eventCharacter = character;
    unsigned short keyCode = 0;
    if ([character isEqualToString:@"leftArrow"]) {
        const unichar ch = NSLeftArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7B;
    } else if ([character isEqualToString:@"rightArrow"]) {
        const unichar ch = NSRightArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7C;
    } else if ([character isEqualToString:@"upArrow"]) {
        const unichar ch = NSUpArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7E;
    } else if ([character isEqualToString:@"downArrow"]) {
        const unichar ch = NSDownArrowFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x7D;
    } else if ([character isEqualToString:@"pageUp"]) {
        const unichar ch = NSPageUpFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x74;
    } else if ([character isEqualToString:@"pageDown"]) {
        const unichar ch = NSPageDownFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x79;
    } else if ([character isEqualToString:@"home"]) {
        const unichar ch = NSHomeFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x73;
    } else if ([character isEqualToString:@"end"]) {
        const unichar ch = NSEndFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x77;
    } else if ([character isEqualToString:@"insert"]) {
        const unichar ch = NSInsertFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x72;
    } else if ([character isEqualToString:@"delete"]) {
        const unichar ch = NSDeleteFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x75;
    } else if ([character isEqualToString:@"printScreen"]) {
        const unichar ch = NSPrintScreenFunctionKey;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x0; // There is no known virtual key code for PrintScreen.
    } else if ([character isEqualToString:@"cyrillicSmallLetterA"]) {
        const unichar ch = 0x0430;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3; // Shares key with "F" on Russian layout.
    } else if ([character isEqualToString:@"leftControl"]) {
        const unichar ch = 0xFFE3;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3B;
    } else if ([character isEqualToString:@"leftShift"]) {
        const unichar ch = 0xFFE1;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x38;
    } else if ([character isEqualToString:@"leftAlt"]) {
        const unichar ch = 0xFFE7;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3A;
    } else if ([character isEqualToString:@"rightControl"]) {
        const unichar ch = 0xFFE4;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3E;
    } else if ([character isEqualToString:@"rightShift"]) {
        const unichar ch = 0xFFE2;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3C;
    } else if ([character isEqualToString:@"rightAlt"]) {
        const unichar ch = 0xFFE8;
        eventCharacter = [NSString stringWithCharacters:&ch length:1];
        keyCode = 0x3D;
    }

    // Compare the input string with the function-key names defined by the DOM spec (i.e. "F1",...,"F24").
    // If the input string is a function-key name, set its key code.
    for (unsigned i = 1; i <= 24; i++) {
        if ([character isEqualToString:[NSString stringWithFormat:@"F%u", i]]) {
            const unichar ch = NSF1FunctionKey + (i - 1);
            eventCharacter = [NSString stringWithCharacters:&ch length:1];
            switch (i) {
                case 1: keyCode = 0x7A; break;
                case 2: keyCode = 0x78; break;
                case 3: keyCode = 0x63; break;
                case 4: keyCode = 0x76; break;
                case 5: keyCode = 0x60; break;
                case 6: keyCode = 0x61; break;
                case 7: keyCode = 0x62; break;
                case 8: keyCode = 0x64; break;
                case 9: keyCode = 0x65; break;
                case 10: keyCode = 0x6D; break;
                case 11: keyCode = 0x67; break;
                case 12: keyCode = 0x6F; break;
                case 13: keyCode = 0x69; break;
                case 14: keyCode = 0x6B; break;
                case 15: keyCode = 0x71; break;
                case 16: keyCode = 0x6A; break;
                case 17: keyCode = 0x40; break;
                case 18: keyCode = 0x4F; break;
                case 19: keyCode = 0x50; break;
                case 20: keyCode = 0x5A; break;
            }
        }
    }

    // FIXME: No keyCode is set for most keys.
    if ([character isEqualToString:@"\t"])
        keyCode = 0x30;
    else if ([character isEqualToString:@" "])
        keyCode = 0x31;
    else if ([character isEqualToString:@"\r"])
        keyCode = 0x24;
    else if ([character isEqualToString:@"\n"])
        keyCode = 0x4C;
    else if ([character isEqualToString:@"\x8"])
        keyCode = 0x33;
    else if ([character isEqualToString:@"a"])
        keyCode = 0x00;
    else if ([character isEqualToString:@"b"])
        keyCode = 0x0B;
    else if ([character isEqualToString:@"d"])
        keyCode = 0x02;
    else if ([character isEqualToString:@"e"])
        keyCode = 0x0E;

    KeyMappingEntry table[] = {
        {0x2F, 0x41, '.', nil},
        {0,    0x43, '*', nil},
        {0,    0x45, '+', nil},
        {0,    0x47, NSClearLineFunctionKey, @"clear"},
        {0x2C, 0x4B, '/', nil},
        {0,    0x4C, 3, @"enter" },
        {0x1B, 0x4E, '-', nil},
        {0x18, 0x51, '=', nil},
        {0x1D, 0x52, '0', nil},
        {0x12, 0x53, '1', nil},
        {0x13, 0x54, '2', nil},
        {0x14, 0x55, '3', nil},
        {0x15, 0x56, '4', nil},
        {0x17, 0x57, '5', nil},
        {0x16, 0x58, '6', nil},
        {0x1A, 0x59, '7', nil},
        {0x1C, 0x5B, '8', nil},
        {0x19, 0x5C, '9', nil},
    };
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(table); ++i) {
        NSString* currentCharacterString = [NSString stringWithCharacters:&table[i].character length:1];
        if ([character isEqualToString:currentCharacterString] || [character isEqualToString:table[i].characterName]) {
            if (location == DOM_KEY_LOCATION_NUMPAD)
                keyCode = table[i].macNumpadKeyCode;
            else
                keyCode = table[i].macKeyCode;
            eventCharacter = currentCharacterString;
            break;
        }
    }

    NSString *charactersIgnoringModifiers = eventCharacter;

    int modifierFlags = 0;

    if ([character length] == 1 && [character characterAtIndex:0] >= 'A' && [character characterAtIndex:0] <= 'Z') {
        modifierFlags |= NSShiftKeyMask;
        charactersIgnoringModifiers = [character lowercaseString];
    }

    modifierFlags |= buildModifierFlags(modifiers);

    if (location == DOM_KEY_LOCATION_NUMPAD)
        modifierFlags |= NSNumericPadKeyMask;

    [[[mainFrame frameView] documentView] layout];

    NSEvent *event = [NSEvent keyEventWithType:NSKeyDown
                        location:NSMakePoint(5, 5)
                        modifierFlags:modifierFlags
                        timestamp:[self currentEventTime]
                        windowNumber:[[[mainFrame webView] window] windowNumber]
                        context:[NSGraphicsContext currentContext]
                        characters:eventCharacter
                        charactersIgnoringModifiers:charactersIgnoringModifiers
                        isARepeat:NO
                        keyCode:keyCode];

    [[[[mainFrame webView] window] firstResponder] keyDown:event];

    event = [NSEvent keyEventWithType:NSKeyUp
                        location:NSMakePoint(5, 5)
                        modifierFlags:modifierFlags
                        timestamp:[self currentEventTime]
                        windowNumber:[[[mainFrame webView] window] windowNumber]
                        context:[NSGraphicsContext currentContext]
                        characters:eventCharacter
                        charactersIgnoringModifiers:charactersIgnoringModifiers
                        isARepeat:NO
                        keyCode:keyCode];

    [[[[mainFrame webView] window] firstResponder] keyUp:event];
}

- (void)keyDownWrapper:(NSString *)character withModifiers:(WebScriptObject *)modifiers withLocation:(unsigned long)location
{
    [self keyDown:character withModifiers:modifiers withLocation:location];
}

- (void)scheduleAsynchronousKeyDown:(NSString *)character withModifiers:(WebScriptObject *)modifiers withLocation:(unsigned long)location
{
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[EventSendingController instanceMethodSignatureForSelector:@selector(keyDownWrapper:withModifiers:withLocation:)]];
    [invocation retainArguments];
    [invocation setTarget:self];
    [invocation setSelector:@selector(keyDownWrapper:withModifiers:withLocation:)];
    [invocation setArgument:&character atIndex:2];
    [invocation setArgument:&modifiers atIndex:3];
    [invocation setArgument:&location atIndex:4];
    [invocation performSelector:@selector(invoke) withObject:nil afterDelay:0];
}

- (void)enableDOMUIEventLogging:(WebScriptObject *)node
{
    NSEnumerator *eventEnumerator = [webkitDomEventNames objectEnumerator];
    id eventName;
    while ((eventName = [eventEnumerator nextObject])) {
        [(id<DOMEventTarget>)node addEventListener:eventName listener:self useCapture:NO];
    }
}

- (void)handleEvent:(DOMEvent *)event
{
    DOMNode *target = [event target];

    printf("event type:      %s\n", [[event type] UTF8String]);
    printf("  target:        <%s>\n", [[[target nodeName] lowercaseString] UTF8String]);
    
    if ([event isKindOfClass:[DOMEvent class]]) {
        printf("  eventPhase:    %d\n", [event eventPhase]);
        printf("  bubbles:       %d\n", [event bubbles] ? 1 : 0);
        printf("  cancelable:    %d\n", [event cancelable] ? 1 : 0);
    }
    
    if ([event isKindOfClass:[DOMUIEvent class]]) {
        printf("  detail:        %d\n", [(DOMUIEvent*)event detail]);
        
        DOMAbstractView *view = [(DOMUIEvent*)event view];
        if (view) {
            printf("  view:          OK");            
            if ([view document])
                printf(" (document: OK)");
            printf("\n");
        }
    }
    
    if ([event isKindOfClass:[DOMKeyboardEvent class]]) {
        printf("  keyIdentifier: %s\n", [[(DOMKeyboardEvent*)event keyIdentifier] UTF8String]);
        printf("  keyLocation:   %d\n", [(DOMKeyboardEvent*)event location]);
        printf("  modifier keys: c:%d s:%d a:%d m:%d\n", 
               [(DOMKeyboardEvent*)event ctrlKey] ? 1 : 0, 
               [(DOMKeyboardEvent*)event shiftKey] ? 1 : 0, 
               [(DOMKeyboardEvent*)event altKey] ? 1 : 0, 
               [(DOMKeyboardEvent*)event metaKey] ? 1 : 0);
        printf("  keyCode:       %d\n", [(DOMKeyboardEvent*)event keyCode]);
        printf("  charCode:      %d\n", [(DOMKeyboardEvent*)event charCode]);
    }
    
    if ([event isKindOfClass:[DOMMouseEvent class]]) {
        printf("  button:        %d\n", [(DOMMouseEvent*)event button]);
        printf("  clientX:       %d\n", [(DOMMouseEvent*)event clientX]);
        printf("  clientY:       %d\n", [(DOMMouseEvent*)event clientY]);
        printf("  screenX:       %d\n", [(DOMMouseEvent*)event screenX]);
        printf("  screenY:       %d\n", [(DOMMouseEvent*)event screenY]);
        printf("  modifier keys: c:%d s:%d a:%d m:%d\n", 
               [(DOMMouseEvent*)event ctrlKey] ? 1 : 0, 
               [(DOMMouseEvent*)event shiftKey] ? 1 : 0, 
               [(DOMMouseEvent*)event altKey] ? 1 : 0, 
               [(DOMMouseEvent*)event metaKey] ? 1 : 0);
        id relatedTarget = [(DOMMouseEvent*)event relatedTarget];
        if (relatedTarget) {
            printf("  relatedTarget: %s", [[[relatedTarget class] description] UTF8String]);
            if ([relatedTarget isKindOfClass:[DOMNode class]])
                printf(" (nodeName: %s)", [[(DOMNode*)relatedTarget nodeName] UTF8String]);
            printf("\n");
        }
    }
    
    if ([event isKindOfClass:[DOMMutationEvent class]]) {
        printf("  prevValue:     %s\n", [[(DOMMutationEvent*)event prevValue] UTF8String]);
        printf("  newValue:      %s\n", [[(DOMMutationEvent*)event newValue] UTF8String]);
        printf("  attrName:      %s\n", [[(DOMMutationEvent*)event attrName] UTF8String]);
        printf("  attrChange:    %d\n", [(DOMMutationEvent*)event attrChange]);
        DOMNode *relatedNode = [(DOMMutationEvent*)event relatedNode];
        if (relatedNode) {
            printf("  relatedNode:   %s (nodeName: %s)\n", 
                   [[[relatedNode class] description] UTF8String],
                   [[relatedNode nodeName] UTF8String]);
        }
    }
    
    if ([event isKindOfClass:[DOMWheelEvent class]]) {
        printf("  clientX:       %d\n", [(DOMWheelEvent*)event clientX]);
        printf("  clientY:       %d\n", [(DOMWheelEvent*)event clientY]);
        printf("  screenX:       %d\n", [(DOMWheelEvent*)event screenX]);
        printf("  screenY:       %d\n", [(DOMWheelEvent*)event screenY]);
        printf("  modifier keys: c:%d s:%d a:%d m:%d\n", 
               [(DOMWheelEvent*)event ctrlKey] ? 1 : 0, 
               [(DOMWheelEvent*)event shiftKey] ? 1 : 0, 
               [(DOMWheelEvent*)event altKey] ? 1 : 0, 
               [(DOMWheelEvent*)event metaKey] ? 1 : 0);
        printf("  isHorizontal:  %d\n", [(DOMWheelEvent*)event isHorizontal] ? 1 : 0);
        printf("  wheelDelta:    %d\n", [(DOMWheelEvent*)event wheelDelta]);
    }
}

// FIXME: It's not good to have a test hard-wired into this controller like this.
// Instead we need to get testing framework based on the Objective-C bindings
// to work well enough that we can test that way instead.
- (void)fireKeyboardEventsToElement:(WebScriptObject *)element {
    
    if (![element isKindOfClass:[DOMHTMLElement class]])
        return;
    
    DOMHTMLElement *target = (DOMHTMLElement*)element;
    DOMDocument *document = [target ownerDocument];
    
    // Keyboard Event 1
    
    DOMEvent *domEvent = [document createEvent:@"KeyboardEvent"];
    [(DOMKeyboardEvent*)domEvent initKeyboardEvent:@"keydown" 
                                         canBubble:YES
                                        cancelable:YES
                                              view:[document defaultView]
                                     keyIdentifier:@"U+000041" 
                                       location:0
                                           ctrlKey:YES
                                            altKey:NO
                                          shiftKey:NO
                                           metaKey:NO];
    [target dispatchEvent:domEvent];  
        
    // Keyboard Event 2
    
    domEvent = [document createEvent:@"KeyboardEvent"];
    [(DOMKeyboardEvent*)domEvent initKeyboardEvent:@"keypress" 
                                         canBubble:YES
                                        cancelable:YES
                                              view:[document defaultView]
                                     keyIdentifier:@"U+000045" 
                                       location:1
                                           ctrlKey:NO
                                            altKey:YES
                                          shiftKey:NO
                                           metaKey:NO];
    [target dispatchEvent:domEvent];    
    
    // Keyboard Event 3
    
    domEvent = [document createEvent:@"KeyboardEvent"];
    [(DOMKeyboardEvent*)domEvent initKeyboardEvent:@"keyup" 
                                         canBubble:YES
                                        cancelable:YES
                                              view:[document defaultView]
                                     keyIdentifier:@"U+000056" 
                                       location:0
                                           ctrlKey:NO
                                            altKey:NO
                                          shiftKey:NO
                                           metaKey:NO];
    [target dispatchEvent:domEvent];   
    
}

@end
