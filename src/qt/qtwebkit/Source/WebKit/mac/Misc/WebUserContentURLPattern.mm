/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "WebUserContentURLPattern.h"

#import <WebCore/KURL.h>
#import <WebCore/UserContentURLPattern.h>

using namespace WebCore;

@interface WebUserContentURLPatternPrivate : NSObject
{
@public
    UserContentURLPattern pattern;
}
@end

@implementation WebUserContentURLPatternPrivate
@end

@implementation WebUserContentURLPattern

- (id)initWithPatternString:(NSString *)patternString
{
    self = [super init];
    if (!self)
        return nil;

    _private = [[WebUserContentURLPatternPrivate alloc] init];
    _private->pattern = UserContentURLPattern(patternString);

    return self;
}

- (void)dealloc
{
    [_private release];
    _private = nil;

    [super dealloc];
}

- (BOOL)isValid
{
    return _private->pattern.isValid();
}

- (NSString *)scheme
{
    return _private->pattern.scheme();
}

- (NSString *)host
{
    return _private->pattern.host();
}

- (BOOL)matchesSubdomains
{
    return _private->pattern.matchSubdomains();
}

- (BOOL)matchesURL:(NSURL *)url
{
    return _private->pattern.matches(url);
}

@end
