/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebSerializedJSValuePrivate.h"

#import <WebCore/SerializedScriptValue.h>
#import <wtf/RefPtr.h>

using namespace WebCore;

@interface WebSerializedJSValuePrivate : NSObject {
@public
    RefPtr<SerializedScriptValue> value;
}
@end

@implementation WebSerializedJSValuePrivate
@end

@implementation WebSerializedJSValue

- (id)initWithValue:(JSValueRef)value context:(JSContextRef)sourceContext exception:(JSValueRef*)exception
{
    ASSERT_ARG(value, value);
    ASSERT_ARG(sourceContext, sourceContext);

    if (!value || !sourceContext) {
        [self release];
        return nil;
    }

    self = [super init];
    if (!self)
        return nil;

    _private = [[WebSerializedJSValuePrivate alloc] init];
    
    _private->value = SerializedScriptValue::create(sourceContext, value, exception);
    if (!_private->value) {
        [self release];
        return nil;
    }

    return self;
}

- (id)initWithInternalRepresentation:(void *)internalRepresenatation
{
    ASSERT_ARG(internalRepresenatation, internalRepresenatation);

    if (!internalRepresenatation) {
        [self release];
        return nil;
    }

    self = [super init];
    if (!self)
        return nil;
    
    _private = [[WebSerializedJSValuePrivate alloc] init];

    _private->value = ((SerializedScriptValue*)internalRepresenatation);
    if (!_private->value) {
        [self release];
        return nil;
    }
    
    return self;
}

- (JSValueRef)deserialize:(JSContextRef)destinationContext
{
    if (!_private || !_private->value)
        return 0;    
    return _private->value->deserialize(destinationContext, 0);
}

- (void)dealloc
{
    [_private release];
    _private = nil;
    [super dealloc];
}

- (void*)internalRepresentation
{
    if (!_private)
        return 0;
    return _private->value.get();
}

@end

