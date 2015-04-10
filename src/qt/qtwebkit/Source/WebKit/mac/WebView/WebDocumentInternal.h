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

#import <WebKit/WebDocumentPrivate.h>
#import <WebKit/WebHTMLView.h>
#import <WebKit/WebViewPrivate.h>

/*!
@protocol _WebDocumentZooming
@discussion Optional protocol for a view that wants to handle its own zoom.
*/
@protocol _WebDocumentZooming <NSObject>

// Methods to perform the actual commands
- (IBAction)_zoomOut:(id)sender;
- (IBAction)_zoomIn:(id)sender;
- (IBAction)_resetZoom:(id)sender;

// Whether or not the commands can be executed.
- (BOOL)_canZoomOut;
- (BOOL)_canZoomIn;
- (BOOL)_canResetZoom;

@end

@protocol WebDocumentElement <NSObject>
- (NSDictionary *)elementAtPoint:(NSPoint)point;
- (NSDictionary *)elementAtPoint:(NSPoint)point allowShadowContent:(BOOL)allow;
@end

@protocol WebMultipleTextMatches <NSObject>
- (void)setMarkedTextMatchesAreHighlighted:(BOOL)newValue;
- (BOOL)markedTextMatchesAreHighlighted;
- (NSUInteger)countMatchesForText:(NSString *)string inDOMRange:(DOMRange *)range options:(WebFindOptions)options limit:(NSUInteger)limit markMatches:(BOOL)markMatches;
- (void)unmarkAllTextMatches;
- (NSArray *)rectsForTextMatches;
@end

@protocol WebDocumentOptionsSearching <NSObject>
// Prefixed with an underscore to avoid conflict with Mail's -[WebHTMLView(MailExtras) findString:options:].
- (BOOL)_findString:(NSString *)string options:(WebFindOptions)options;
@end

/* Used to save and restore state in the view, typically when going back/forward */
@protocol _WebDocumentViewState <NSObject>
- (NSPoint)scrollPoint;
- (void)setScrollPoint:(NSPoint)p;
- (id)viewState;
- (void)setViewState:(id)statePList;
@end

@interface WebHTMLView (WebDocumentInternalProtocols) <WebDocumentElement, WebMultipleTextMatches, WebDocumentOptionsSearching>
@end
