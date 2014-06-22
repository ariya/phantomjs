/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

// Things internal to the WebKit framework; not SPI.

#import "WebHTMLViewPrivate.h"

#if USE(ACCELERATED_COMPOSITING)
@class CALayer;
#endif
@class WebFrame;

namespace WebCore {
    class CachedImage;
    class KeyboardEvent;
}

@interface WebHTMLView (WebInternal)
- (void)_selectionChanged;
- (void)_updateFontPanel;
- (BOOL)_canSmartCopyOrDelete;

- (id <WebHTMLHighlighter>)_highlighterForType:(NSString*)type;
- (WebFrame *)_frame;
- (void)_lookUpInDictionaryFromMenu:(id)sender;
- (BOOL)_interpretKeyEvent:(WebCore::KeyboardEvent *)event savingCommands:(BOOL)savingCommands;
- (DOMDocumentFragment *)_documentFragmentFromPasteboard:(NSPasteboard *)pasteboard;
- (NSEvent *)_mouseDownEvent;
- (BOOL)isGrammarCheckingEnabled;
- (void)setGrammarCheckingEnabled:(BOOL)flag;
- (void)toggleGrammarChecking:(id)sender;
- (WebCore::CachedImage*)promisedDragTIFFDataSource;
- (void)setPromisedDragTIFFDataSource:(WebCore::CachedImage*)source;
- (void)_web_updateLayoutAndStyleIfNeededRecursive;
- (void)_destroyAllWebPlugins;
- (BOOL)_needsLayout;

#if USE(ACCELERATED_COMPOSITING)
- (void)attachRootLayer:(CALayer*)layer;
- (void)detachRootLayer;
- (BOOL)_web_isDrawingIntoLayer;
#endif

@end
