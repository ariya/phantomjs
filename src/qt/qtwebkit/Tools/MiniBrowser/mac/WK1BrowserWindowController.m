/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WK1BrowserWindowController.h"

#import <WebKit/WebKit.h>
#import <WebKit/WebViewPrivate.h>
#import "AppDelegate.h"

@interface WK1BrowserWindowController ()
@end

@implementation WK1BrowserWindowController

- (void)awakeFromNib
{
    _webView = [[WebView alloc] initWithFrame:[containerView bounds]];
    [_webView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

    // Set the WebView delegates
    [_webView setFrameLoadDelegate:self];
    [_webView setUIDelegate:self];
    [_webView setResourceLoadDelegate:self];

    [containerView addSubview:_webView];
}

- (void)dealloc
{
    [_webView setFrameLoadDelegate:nil];
    [_webView setUIDelegate:nil];
    [_webView setResourceLoadDelegate:nil];
    [_webView release];

    [super dealloc];
}

- (void)loadURLString:(NSString *)urlString
{
    // FIXME: We shouldn't have to set the url text here.
    [urlText setStringValue:urlString];
    [self fetch:nil];
}

- (IBAction)fetch:(id)sender
{
    [urlText setStringValue:[self addProtocolIfNecessary:[urlText stringValue]]];
    NSURL *url = [NSURL URLWithString:[urlText stringValue]];
    [[_webView mainFrame] loadRequest:[NSURLRequest requestWithURL:url]];
}

- (IBAction)showHideWebView:(id)sender
{
    BOOL hidden = ![_webView isHidden];
    
    [_webView setHidden:hidden];
}

- (IBAction)removeReinsertWebView:(id)sender
{
    if ([_webView window]) {
        [_webView retain];
        [_webView removeFromSuperview]; 
    } else {
        [containerView addSubview:_webView];
        [_webView release];
    }
}

- (IBAction)reload:(id)sender
{
    [_webView reload:sender];
}

- (IBAction)forceRepaint:(id)sender
{
    [_webView setNeedsDisplay:YES];
}

- (IBAction)goBack:(id)sender
{
    [_webView goBack:sender];
}

- (IBAction)goForward:(id)sender
{
    [_webView goForward:sender];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = [menuItem action];

    if (action == @selector(zoomIn:))
        return [self canZoomIn];
    if (action == @selector(zoomOut:))
        return [self canZoomOut];
    if (action == @selector(resetZoom:))
        return [self canResetZoom];

    if (action == @selector(showHideWebView:))
        [menuItem setTitle:[_webView isHidden] ? @"Show Web View" : @"Hide Web View"];
    else if (action == @selector(removeReinsertWebView:))
        [menuItem setTitle:[_webView window] ? @"Remove Web View" : @"Insert Web View"];
    else if (action == @selector(toggleZoomMode:))
        [menuItem setState:_zoomTextOnly ? NSOnState : NSOffState];
    else if ([menuItem action] == @selector(togglePaginationMode:))
        [menuItem setState:[self isPaginated] ? NSOnState : NSOffState];
    else if ([menuItem action] == @selector(toggleTransparentWindow:))
        [menuItem setState:[[self window] isOpaque] ? NSOffState : NSOnState];

    return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];

    if (action == @selector(goBack:))
        return [_webView canGoBack];
    
    if (action == @selector(goForward:))
        return [_webView canGoForward];
    
    return YES;
}

- (void)validateToolbar
{
    [toolbar validateVisibleItems];
}

- (BOOL)windowShouldClose:(id)sender
{
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification
{
    [(BrowserAppDelegate *)[NSApp delegate] browserWindowWillClose:[self window]];
    [self autorelease];
}

- (void)applicationTerminating
{
}

- (double)currentZoomFactor
{
    return 1;
}

- (BOOL)canZoomIn
{
    return [_webView canMakeTextLarger];
}

- (void)zoomIn:(id)sender
{
    if (![self canZoomIn])
        return;

    [_webView makeTextLarger:sender];
}

- (BOOL)canZoomOut
{
    return [_webView canMakeTextSmaller];
}

- (void)zoomOut:(id)sender
{
    if (![self canZoomIn])
        return;

    [_webView makeTextSmaller:sender];
}

- (BOOL)canResetZoom
{
    return [_webView canMakeTextStandardSize];
}

- (void)resetZoom:(id)sender
{
    if (![self canResetZoom])
        return;

    [_webView makeTextStandardSize:sender];
}

- (IBAction)toggleZoomMode:(id)sender
{
    // FIXME: non-text zoom not implemented.
    _zoomTextOnly = !_zoomTextOnly;
}

- (BOOL)isPaginated
{
    return [_webView _paginationMode] != WebPaginationModeUnpaginated;
}

- (IBAction)togglePaginationMode:(id)sender
{
    if ([self isPaginated]) {
        [_webView _setPaginationMode:WebPaginationModeUnpaginated];
    } else {
        [_webView _setPaginationMode:WebPaginationModeRightToLeft];
        [_webView _setPageLength:_webView.bounds.size.width / 2];
        [_webView _setGapBetweenPages:10];
    }
}

- (IBAction)toggleTransparentWindow:(id)sender
{
    BOOL isTransparent = ![[self window] isOpaque];
    isTransparent = !isTransparent;
    
    [[self window] setOpaque:!isTransparent];
    [[self window] setHasShadow:!isTransparent];

    if (isTransparent)
        [_webView setBackgroundColor:[NSColor clearColor]];
    else
        [_webView setBackgroundColor:[NSColor whiteColor]];

    [[self window] display];
}

- (IBAction)find:(id)sender
{
}

- (IBAction)dumpSourceToConsole:(id)sender
{
}

// WebFrameLoadDelegate Methods
- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame
{
}

- (void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame
{
    if (frame != [sender mainFrame])
        return;

    NSURL *committedURL = [[[frame dataSource] request] URL];
    [urlText setStringValue:[committedURL absoluteString]];
}

- (void)webView:(WebView *)sender didReceiveTitle:(NSString *)title forFrame:(WebFrame *)frame
{
    if (frame != [sender mainFrame])
        return;

    [[self window] setTitle:[title stringByAppendingString:@" [WK1]"]];
}

- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WebFrame *)frame
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];

    alert.messageText = [NSString stringWithFormat:@"JavaScript alert dialog from %@.", frame.dataSource.request.URL.absoluteString];
    alert.informativeText = message;

    [alert runModal];
    [alert release];
}

@end
