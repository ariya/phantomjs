/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import "WebJavaScriptTextInputPanel.h"

#import <wtf/Assertions.h>

#import <WebKit/WebNSControlExtras.h>
#import <WebKit/WebNSWindowExtras.h>

@implementation WebJavaScriptTextInputPanel

- (id)initWithPrompt:(NSString *)p text:(NSString *)t
{
    self = [self initWithWindowNibName:@"WebJavaScriptTextInputPanel"];
    if (!self)
        return nil;
    NSWindow *window = [self window];
    
    // This must be done after the call to [self window], because
    // until then, prompt and textInput will be nil.
    ASSERT(prompt);
    ASSERT(textInput);
    [prompt setStringValue:p];
    [textInput setStringValue:t];

    [prompt sizeToFitAndAdjustWindowHeight];
    [window centerOverMainWindow];
    
    return self;
}

- (NSString *)text
{
    return [textInput stringValue];
}

- (IBAction)pressedCancel:(id)sender
{
    [NSApp stopModalWithCode:NO];
}

- (IBAction)pressedOK:(id)sender
{
    [NSApp stopModalWithCode:YES];
}

@end
