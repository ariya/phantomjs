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

#import "WebFormDelegatePrivate.h"

// FIXME: This could become an informal protocol; we switched all the API
// delegates to be informal.

@implementation WebFormDelegate

static WebFormDelegate *sharedDelegate = nil;

// Return a object with NOP implementations of the protocol's methods
// Note this feature relies on our default delegate being stateless
+ (WebFormDelegate *)_sharedWebFormDelegate
{
    if (!sharedDelegate)
        sharedDelegate = [[WebFormDelegate alloc] init];
    return sharedDelegate;
}
    
- (void)textFieldDidBeginEditing:(DOMHTMLInputElement *)element inFrame:(WebFrame *)frame
{
}

- (void)textFieldDidEndEditing:(DOMHTMLInputElement *)element inFrame:(WebFrame *)frame
{
}

- (void)textDidChangeInTextField:(DOMHTMLInputElement *)element inFrame:(WebFrame *)frame
{
}

- (void)textDidChangeInTextArea:(DOMHTMLTextAreaElement *)element inFrame:(WebFrame *)frame
{
}

- (void)didFocusTextField:(DOMHTMLInputElement *)element inFrame:(WebFrame *)frame
{
}

- (BOOL)textField:(DOMHTMLInputElement *)element doCommandBySelector:(SEL)commandSelector inFrame:(WebFrame *)frame
{
    return NO;
}

- (BOOL)textField:(DOMHTMLInputElement *)element shouldHandleEvent:(NSEvent *)event inFrame:(WebFrame *)frame
{
    return NO;
}

- (void)frame:(WebFrame *)frame sourceFrame:(WebFrame *)sourceFrame willSubmitForm:(DOMElement *)form
    withValues:(NSDictionary *)values submissionListener:(id <WebFormSubmissionListener>)listener
{
    [listener continue];
}

- (void)willSendSubmitEventToForm:(DOMHTMLFormElement *)element inFrame:(WebFrame *)sourceFrame withValues:(NSDictionary *)values
{
}

@end
