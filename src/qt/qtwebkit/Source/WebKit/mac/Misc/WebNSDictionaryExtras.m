/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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

#import <WebKit/WebNSDataExtras.h>

#import <wtf/Assertions.h>

@implementation NSDictionary (WebNSDictionaryExtras)
-(NSNumber *)_webkit_numberForKey:(id)key
{
    id object = [self objectForKey:key];
    return [object isKindOfClass:[NSNumber class]] ? object : nil;
}

-(int)_webkit_intForKey:(NSString *)key
{
    NSNumber *number = [self _webkit_numberForKey:key];
    return number == nil ? 0 : [number intValue];
}

-(NSString *)_webkit_stringForKey:(id)key
{
    id object = [self objectForKey:key];
    return [object isKindOfClass:[NSString class]] ? object : nil;
}

-(NSArray *)_webkit_arrayForKey:(id)key
{
    id object = [self objectForKey:key];
    return [object isKindOfClass:[NSArray class]] ? object : nil;
}

-(id)_webkit_objectForMIMEType:(NSString *)MIMEType
{
    id result;
    NSRange slashRange;
    
    result = [self objectForKey:MIMEType];
    if (result) {
        return result;
    }
    
    slashRange = [MIMEType rangeOfString:@"/"];
    if (slashRange.location == NSNotFound) {
        return nil;
    }
    
    return [self objectForKey:[MIMEType substringToIndex:slashRange.location + 1]];
}

- (BOOL)_webkit_boolForKey:(id)key
{
    NSNumber *number = [self _webkit_numberForKey:key];
    return number && [number boolValue];
}

@end

@implementation NSMutableDictionary (WebNSDictionaryExtras)

-(void)_webkit_setInt:(int)value forKey:(id)key
{
    NSNumber *object = [[NSNumber alloc] initWithInt:value];
    [self setObject:object forKey:key];
    [object release];
}

-(void)_webkit_setFloat:(float)value forKey:(id)key
{
    NSNumber *object = [[NSNumber alloc] initWithFloat:value];
    [self setObject:object forKey:key];
    [object release];
}

-(void)_webkit_setBool:(BOOL)value forKey:(id)key
{
    NSNumber *object = [[NSNumber alloc] initWithBool:value];
    [self setObject:object forKey:key];
    [object release];
}

- (void)_webkit_setLongLong:(long long)value forKey:(id)key
{
    NSNumber *object = [[NSNumber alloc] initWithLongLong:value];
    [self setObject:object forKey:key];
    [object release];
}

- (void)_webkit_setUnsignedLongLong:(unsigned long long)value forKey:(id)key
{
    NSNumber *object = [[NSNumber alloc] initWithUnsignedLongLong:value];
    [self setObject:object forKey:key];
    [object release];
}

@end

