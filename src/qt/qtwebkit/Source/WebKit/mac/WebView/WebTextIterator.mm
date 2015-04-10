/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "WebTextIterator.h"

#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebTypesInternal.h"
#import <wtf/Vector.h>
#import <WebCore/RunLoop.h>
#import <WebCore/TextIterator.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>

using namespace JSC;
using namespace WebCore;

@interface WebTextIteratorPrivate : NSObject {
@public
    OwnPtr<TextIterator> _textIterator;
}
@end

@implementation WebTextIteratorPrivate

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCore::RunLoop::initializeMainRunLoop();
    WebCoreObjCFinalizeOnMainThread(self);
}

@end

@implementation WebTextIterator

- (void)dealloc
{
    [_private release];
    [super dealloc];
}

- (id)initWithRange:(DOMRange *)range
{
    self = [super init];
    if (!self)
        return self;
    
    _private = [[WebTextIteratorPrivate alloc] init];
    _private->_textIterator = adoptPtr(new TextIterator(core(range)));
    return self;
}

- (void)advance
{
    _private->_textIterator->advance();
}

- (BOOL)atEnd
{
    return _private->_textIterator->atEnd();
}

- (DOMRange *)currentRange
{
    return kit(_private->_textIterator->range().get());
}

- (const unichar *)currentTextPointer
{
    return _private->_textIterator->characters();
}

- (NSUInteger)currentTextLength
{
    return _private->_textIterator->length();
}

@end

@implementation WebTextIterator (WebTextIteratorDeprecated)

- (DOMNode *)currentNode
{
    return kit(_private->_textIterator->node());
}

- (NSString *)currentText
{
    return [NSString stringWithCharacters:_private->_textIterator->characters() length:_private->_textIterator->length()];
}

@end
