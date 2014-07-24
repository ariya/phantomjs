/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#import "WebNSArrayExtras.h"

#import <wtf/Assertions.h>

@implementation NSArray (WebNSArrayExtras)

-(NSNumber *)_webkit_numberAtIndex:(NSUInteger)index
{
    id object = [self objectAtIndex:index];
    return [object isKindOfClass:[NSNumber class]] ? object : nil;
}

-(NSString *)_webkit_stringAtIndex:(NSUInteger)index
{
    id object = [self objectAtIndex:index];
    return [object isKindOfClass:[NSString class]] ? object : nil;
}

@end

@implementation NSMutableArray (WebNSArrayExtras)

- (void)_webkit_removeUselessMenuItemSeparators
{
    // Starting with a mutable array of NSMenuItems, removes any separators at the start,
    // removes any separators at the end, and collapses any other adjacent separators to
    // a single separator.

    int index;
    // Start this with YES so very last item will be removed if it's a separator.
    BOOL removePreviousItemIfSeparator = YES;
    for (index = [self count] - 1; index >= 0; --index) {
        NSMenuItem *item = [self objectAtIndex:index];
        ASSERT([item isKindOfClass:[NSMenuItem class]]);
        BOOL itemIsSeparator = [item isSeparatorItem];
        if (itemIsSeparator && (removePreviousItemIfSeparator || index == 0))
            [self removeObjectAtIndex:index];
        removePreviousItemIfSeparator = itemIsSeparator;
    }
    
    // This could leave us with one initial separator; kill it off too
    if ([self count] && [[self objectAtIndex:0] isSeparatorItem])
        [self removeObjectAtIndex:0];
}

@end
