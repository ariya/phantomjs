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

#if defined(__LP64__) && defined(__clang__)

#import <Foundation/Foundation.h>
#import <WebKit2/WKBase.h>

@class WKDOMNode, WKDOMDocument;

WK_EXPORT
@interface WKDOMRange : NSObject

- (id)initWithDocument:(WKDOMDocument *)document;

- (void)setStart:(WKDOMNode *)node offset:(int)offset;
- (void)setEnd:(WKDOMNode *)node offset:(int)offset;
- (void)collapse:(BOOL)toStart;
- (void)selectNode:(WKDOMNode *)node;
- (void)selectNodeContents:(WKDOMNode *)node;

@property(readonly, retain) WKDOMNode *startContainer;
@property(readonly) NSInteger startOffset;
@property(readonly, retain) WKDOMNode *endContainer;
@property(readonly) NSInteger endOffset;
@property(readonly, copy) NSString *text;
@property(readonly) BOOL isCollapsed;
@property(readonly) NSArray *textRects;

@end

#endif // defined(__LP64__) && defined(__clang__)
