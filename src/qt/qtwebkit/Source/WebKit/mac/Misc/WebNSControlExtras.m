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

#import "WebNSControlExtras.h"

@implementation NSControl (WebExtras)

- (void)sizeToFitAndAdjustWindowHeight
{
    NSRect frame = [self frame];

    NSSize bestSize = [[self cell] cellSizeForBounds:NSMakeRect(0.0f, 0.0f, frame.size.width, 10000.0f)];
    
    float heightDelta = bestSize.height - frame.size.height;

    frame.size.height += heightDelta;
    frame.origin.y    -= heightDelta;
    [self setFrame:frame];

    NSWindow *window = [self window];
    NSRect windowFrame = [window frame];

    CGFloat backingScaleFactor;
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    backingScaleFactor = [window backingScaleFactor];
#else
    backingScaleFactor = [window userSpaceScaleFactor];
#endif

    windowFrame.size.height += heightDelta * backingScaleFactor;
    [window setFrame:windowFrame display:NO];
}

@end
