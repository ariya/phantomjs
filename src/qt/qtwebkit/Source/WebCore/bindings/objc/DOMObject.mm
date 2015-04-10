/*
 * Copyright (C) 2004, 2006, 2006, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 James G. Speth <speth@end.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "DOMObject.h"

#import "DOMHTMLLinkElementInternal.h"
#import "DOMHTMLStyleElementInternal.h"
#import "DOMInternal.h"
#import "DOMProcessingInstructionInternal.h"
#import "DOMStyleSheetInternal.h"
#import "HTMLLinkElement.h"
#import "HTMLStyleElement.h"
#import "ProcessingInstruction.h"
#import "StyleSheet.h"
#import "WebScriptObjectPrivate.h"

@implementation DOMObject

// Prevent creation of DOM objects by clients who just "[[xxx alloc] init]".
- (id)init
{
    [NSException raise:NSGenericException format:@"+[%@ init]: should never be used", NSStringFromClass([self class])];
    [self release];
    return nil;
}

- (void)dealloc
{
    if (_internal)
        removeDOMWrapper(_internal);
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)unusedZone
{
    UNUSED_PARAM(unusedZone);
    return [self retain];
}

@end

@implementation DOMObject (DOMLinkStyle)

- (DOMStyleSheet *)sheet
{
    WebCore::StyleSheet* styleSheet;

    if ([self isKindOfClass:[DOMProcessingInstruction class]])
        styleSheet = core(static_cast<DOMProcessingInstruction *>(self))->sheet();
    else if ([self isKindOfClass:[DOMHTMLLinkElement class]])
        styleSheet = core(static_cast<DOMHTMLLinkElement *>(self))->sheet();
    else if ([self isKindOfClass:[DOMHTMLStyleElement class]])
        styleSheet = core(static_cast<DOMHTMLStyleElement *>(self))->sheet();
    else
        return nil;

    return kit(styleSheet);
}

@end
