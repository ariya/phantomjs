/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#import "WebNodeHighlighter.h"

#import "DOMNodeInternal.h"
#import "WebNodeHighlight.h"
#import "WebViewInternal.h"
#import <WebCore/Page.h>

@implementation WebNodeHighlighter

- (id)initWithInspectedWebView:(WebView *)webView
{
    if (!(self = [super init]))
        return nil;

    // Don't retain to avoid a circular reference.
    _inspectedWebView = webView;

    return self;
}

- (void)dealloc
{
    ASSERT(!_currentHighlight);
    [super dealloc];
}

// MARK: -

- (void)highlight
{
    // The scrollview's content view stays around between page navigations, so target it.
    NSView *view = [[[[[_inspectedWebView mainFrame] frameView] documentView] enclosingScrollView] contentView];
    if (![view window])
        return; // Skip the highlight if we have no window (e.g. hidden tab).
    
    if (!_currentHighlight) {
        _currentHighlight = [[WebNodeHighlight alloc] initWithTargetView:view inspectorController:[_inspectedWebView page]->inspectorController()];
        [_currentHighlight setDelegate:self];
        [_currentHighlight attach];
    } else
        [[_currentHighlight highlightView] setNeedsDisplay:YES];
}

- (void)hideHighlight
{
    [_currentHighlight detach];
    [_currentHighlight setDelegate:nil];
    [_currentHighlight release];
    _currentHighlight = nil;
}

// MARK: -
// MARK: WebNodeHighlight delegate

- (void)didAttachWebNodeHighlight:(WebNodeHighlight *)highlight
{
    [_inspectedWebView setCurrentNodeHighlight:highlight];
}

- (void)willDetachWebNodeHighlight:(WebNodeHighlight *)highlight
{
    [_inspectedWebView setCurrentNodeHighlight:nil];
}
    
@end
