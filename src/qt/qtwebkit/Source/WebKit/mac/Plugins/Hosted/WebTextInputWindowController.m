/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#if USE(PLUGIN_HOST_PROCESS)

#import "WebTextInputWindowController.h"

#import <WebKitSystemInterface.h>

@interface WebTextInputView : NSTextView {
}
@end

@implementation WebTextInputView

- (NSArray *)validAttributesForMarkedText
{
    // Let TSM know that a bottom input window would be created for marked text.
    NSArray *regularAttributes = [super validAttributesForMarkedText];
    NSMutableArray *floatingWindowAttributes = [NSMutableArray arrayWithArray:regularAttributes];
    [floatingWindowAttributes addObject:@"__NSUsesFloatingInputWindow"];
    return floatingWindowAttributes;
}

@end

@interface WebTextInputPanel : NSPanel {
    NSTextView *_inputTextView;
}

- (NSTextInputContext *)_inputContext;
- (BOOL)_interpretKeyEvent:(NSEvent *)event string:(NSString **)string;

@end

#define inputWindowHeight 20

@implementation WebTextInputPanel

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [_inputTextView release];
    
    [super dealloc];
}

- (id)init
{
    self = [super initWithContentRect:NSZeroRect styleMask:WKGetInputPanelWindowStyle() backing:NSBackingStoreBuffered defer:YES];
    if (!self)
        return nil;
    
    // Set the frame size.
    NSRect visibleFrame = [[NSScreen mainScreen] visibleFrame];
    NSRect frame = NSMakeRect(visibleFrame.origin.x, visibleFrame.origin.y, visibleFrame.size.width, inputWindowHeight);
     
    [self setFrame:frame display:NO];
        
    _inputTextView = [[WebTextInputView alloc] initWithFrame:[self.contentView frame]];
    _inputTextView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable | NSViewMaxXMargin | NSViewMinXMargin | NSViewMaxYMargin | NSViewMinYMargin;
        
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:[self.contentView frame]];
    scrollView.documentView = _inputTextView;
    self.contentView = scrollView;
    [scrollView release];
        
    [self setFloatingPanel:YES];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_keyboardInputSourceChanged:)
                                                 name:NSTextInputContextKeyboardSelectionDidChangeNotification
                                               object:nil];

    return self;
}

- (void)_keyboardInputSourceChanged:(NSNotification *)notification
{
    [_inputTextView setString:@""];
    [self orderOut:nil];
}

- (BOOL)_interpretKeyEvent:(NSEvent *)event string:(NSString **)string
{
    BOOL hadMarkedText = [_inputTextView hasMarkedText];
 
    *string = nil;

    // Let TSM know that a bottom input window would be created for marked text.
    // FIXME: Can be removed once we can rely on __NSUsesFloatingInputWindow (or a better API) being available everywhere.
    EventRef carbonEvent = (EventRef)[event eventRef];
    if (carbonEvent) {
        Boolean ignorePAH = true;
        SetEventParameter(carbonEvent, 'iPAH', typeBoolean, sizeof(ignorePAH), &ignorePAH);
    }

    if (![[_inputTextView inputContext] handleEvent:event])
        return NO;
    
    if ([_inputTextView hasMarkedText]) {
        // Don't show the input method window for dead keys
        if ([[event characters] length] > 0)
            [self orderFront:nil];

        return YES;
    }
    
    if (hadMarkedText) {
        [self orderOut:nil];

        NSString *text = [[_inputTextView textStorage] string];
        if ([text length] > 0)
            *string = [[text copy] autorelease];
    }
            
    [_inputTextView setString:@""];
    return hadMarkedText;
}

- (NSTextInputContext *)_inputContext
{
    return [_inputTextView inputContext];
}

@end

@implementation WebTextInputWindowController

+ (WebTextInputWindowController *)sharedTextInputWindowController
{
    static WebTextInputWindowController *textInputWindowController;
    if (!textInputWindowController)
        textInputWindowController = [[WebTextInputWindowController alloc] init];
    
    return textInputWindowController;
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    
    _panel = [[WebTextInputPanel alloc] init];
    
    return self;
}
        
- (NSTextInputContext *)inputContext
{
    return [_panel _inputContext];
}

- (BOOL)interpretKeyEvent:(NSEvent *)event string:(NSString **)string
{
    return [_panel _interpretKeyEvent:event string:string];
}

@end

#endif // USE(PLUGIN_HOST_PROCESS)

