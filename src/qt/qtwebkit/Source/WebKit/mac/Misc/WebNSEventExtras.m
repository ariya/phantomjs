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

#import <WebKit/WebNSEventExtras.h>

@implementation NSEvent (WebExtras)

-(BOOL)_web_isKeyEvent:(unichar)key
{
    int type = [self type];
    if (type != NSKeyDown && type != NSKeyUp)
        return NO;
    
    NSString *chars = [self charactersIgnoringModifiers];
    if ([chars length] != 1)
        return NO;
    
    unichar c = [chars characterAtIndex:0];
    if (c != key)
        return NO;
    
    return YES;
}

- (BOOL)_web_isDeleteKeyEvent
{
    const unichar deleteKey = NSDeleteCharacter;
    const unichar deleteForwardKey = NSDeleteFunctionKey;
    return [self _web_isKeyEvent:deleteKey] || [self _web_isKeyEvent:deleteForwardKey];
}

- (BOOL)_web_isEscapeKeyEvent
{
    const unichar escapeKey = 0x001b;
    return [self _web_isKeyEvent:escapeKey];
}

- (BOOL)_web_isOptionTabKeyEvent
{
    return ([self modifierFlags] & NSAlternateKeyMask) && [self _web_isTabKeyEvent];
}

- (BOOL)_web_isReturnOrEnterKeyEvent
{
    const unichar enterKey = NSEnterCharacter;
    const unichar returnKey = NSCarriageReturnCharacter;
    return [self _web_isKeyEvent:enterKey] || [self _web_isKeyEvent:returnKey];
}

- (BOOL)_web_isTabKeyEvent
{
    const unichar tabKey = 0x0009;
    const unichar shiftTabKey = 0x0019;
    return [self _web_isKeyEvent:tabKey] || [self _web_isKeyEvent:shiftTabKey];
}

@end
