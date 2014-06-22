/*
 * Copyright (C) 2006, 2008 Apple Computer, Inc.  All rights reserved.
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

#import "WebNodeHighlightView.h"
#import "WebNodeHighlight.h"

#import <WebCore/GraphicsContext.h>
#import <WebCore/InspectorController.h>
#import <wtf/Assertions.h>

using namespace WebCore;

@implementation WebNodeHighlightView

- (id)initWithWebNodeHighlight:(WebNodeHighlight *)webNodeHighlight
{
    self = [self initWithFrame:NSZeroRect];
    if (!self)
        return nil;

    _webNodeHighlight = [webNodeHighlight retain];

    return self;
}

- (void)dealloc
{
    [self detachFromWebNodeHighlight];
    [super dealloc];
}

- (void)detachFromWebNodeHighlight
{
    [_webNodeHighlight release];
    _webNodeHighlight = nil;
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)drawRect:(NSRect)rect 
{
    if (_webNodeHighlight) {
        [NSGraphicsContext saveGraphicsState];

        ASSERT([[NSGraphicsContext currentContext] isFlipped]);

        GraphicsContext context((PlatformGraphicsContext*)[[NSGraphicsContext currentContext] graphicsPort]);
        [_webNodeHighlight inspectorController]->drawHighlight(context);
        [NSGraphicsContext restoreGraphicsState];
    }
}

- (WebNodeHighlight *)webNodeHighlight
{
    return _webNodeHighlight;
}

@end
