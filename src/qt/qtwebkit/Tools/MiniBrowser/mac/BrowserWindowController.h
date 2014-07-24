/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import <Cocoa/Cocoa.h>

@protocol BrowserController

- (IBAction)fetch:(id)sender;
- (IBAction)reload:(id)sender;
- (IBAction)forceRepaint:(id)sender;
- (IBAction)goBack:(id)sender;
- (IBAction)goForward:(id)sender;

- (IBAction)showHideWebView:(id)sender;
- (IBAction)removeReinsertWebView:(id)sender;

- (IBAction)zoomIn:(id)sender;
- (IBAction)zoomOut:(id)sender;
- (IBAction)resetZoom:(id)sender;
- (BOOL)canZoomIn;
- (BOOL)canZoomOut;
- (BOOL)canResetZoom;

- (IBAction)toggleZoomMode:(id)sender;
- (IBAction)togglePaginationMode:(id)sender;

- (IBAction)toggleTransparentWindow:(id)sender;

- (IBAction)dumpSourceToConsole:(id)sender;

- (IBAction)find:(id)sender;

@end

@interface BrowserWindowController : NSWindowController {
    IBOutlet NSProgressIndicator *progressIndicator;
    IBOutlet NSButton *reloadButton;
    IBOutlet NSButton *backButton;
    IBOutlet NSButton *forwardButton;
    IBOutlet NSToolbar *toolbar;
    IBOutlet NSTextField *urlText;
    IBOutlet NSView *containerView;
    
    IBOutlet NSWindow *findPanelWindow;
    
    BOOL _zoomTextOnly;
}

- (void)loadURLString:(NSString *)urlString;
- (NSString *)addProtocolIfNecessary:(NSString *)address;

- (void)applicationTerminating;

- (IBAction)openLocation:(id)sender;

@end

