/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

#import "config.h"
#import "ObjCPlugin.h"

#import <WebKit/WebKit.h>
#import <objc/runtime.h>

// === NSObject category to expose almost everything to JavaScript ===

// Warning: this class introduces huge security weaknesses, and should only be used
// for testing inside of DumpRenderTree, and only with trusted code.  By default, it has
// the same restrictive behavior as the standard WebKit setup.  However, scripts can use the
// plugin's removeBridgeRestrictions: method to open up almost total access to the Cocoa
// frameworks.

static BOOL _allowsScriptsFullAccess = NO;

@interface NSObject (ObjCScriptAccess)

+ (void)setAllowsScriptsFullAccess:(BOOL)value;
+ (BOOL)allowsScriptsFullAccess;

@end

@implementation NSObject (ObjCScriptAccess)

+ (void)setAllowsScriptsFullAccess:(BOOL)value
{
    _allowsScriptsFullAccess = value;
}

+ (BOOL)allowsScriptsFullAccess
{
    return _allowsScriptsFullAccess;
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector
{
    return !_allowsScriptsFullAccess;
}

+ (NSString *)webScriptNameForSelector:(SEL)selector
{
    return nil;
}

@end

@interface JSObjC : NSObject {
}

// expose some useful objc functions to the scripting environment
- (id)lookUpClass:(NSString *)name;
- (void)log:(NSString *)message;
- (id)retainObject:(id)obj;
- (id)classOfObject:(id)obj;
- (NSString *)classNameOfObject:(id)obj;

@end

@implementation JSObjC

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector
{
    return NO;
}

+ (NSString *)webScriptNameForSelector:(SEL)selector
{
    return nil;
}

- (id)invokeDefaultMethodWithArguments:(NSArray *)args
{
    // this is a useful shortcut for accessing objective-c classes from the scripting
    // environment, e.g. 'var myObject = objc("NSObject").alloc().init();'
    if ([args count] == 1)
        return [self lookUpClass:[args objectAtIndex:0]];
    return nil;
}

- (id)lookUpClass:(NSString *)name
{
    return NSClassFromString(name);
}

- (void)log:(NSString *)message
{
    NSLog(@"%@", message);
}

- (id)retainObject:(id)obj
{
    return [obj retain];
}

- (id)classOfObject:(id)obj
{
    return (id)[obj class];
}

- (NSString *)classNameOfObject:(id)obj
{
    return [obj className];
}

@end

@implementation ObjCPlugin

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    if (aSelector == @selector(removeBridgeRestrictions:))
        return NO;

    if (aSelector == @selector(echo:))
        return NO;

    if (aSelector == @selector(throwIfArgumentIsNotHello:))
        return NO;

    if (aSelector == @selector(methodMappedToLongName))
        return NO;

    NSString *selectorName = NSStringFromSelector(aSelector);
    if ([selectorName hasPrefix:@"testConversion"])
        return NO;

    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(echo:))
        return @"echo";

    if (aSelector == @selector(throwIfArgumentIsNotHello:))
        return @"throwIfArgumentIsNotHello";

    if (aSelector == @selector(methodMappedToLongName))
        return [@"" stringByPaddingToLength:4096 withString: @"long" startingAtIndex:0];

    return nil;
}

+ (NSString *)webScriptNameForKey:(const char *)key 
{
    if (strcmp(key, "throwOnDealloc") == 0)
      return @"throwOnDealloc";
    
    return nil;
}

+ (BOOL)isKeyExcludedFromWebScript:(const char *)key 
{
    if (strcmp(key, "throwOnDealloc") == 0)
      return NO;
    
    return YES;
}

- (void)removeBridgeRestrictions:(id)container
{
    // let scripts invoke any selector
    [NSObject setAllowsScriptsFullAccess:YES];
    
    // store a JSObjC instance into the provided container
    JSObjC *objc = [[JSObjC alloc] init];
    [container setValue:objc forKey:@"objc"];
    [objc release];
}

- (id)echo:(id)obj
{
    return obj;
}

- (void)throwIfArgumentIsNotHello:(NSString *)str 
{
    if (![str isEqualToString:@"Hello"]) 
        [WebScriptObject throwException:[NSString stringWithFormat:@"%@ != Hello", str]];
}

- (NSString *)methodMappedToLongName
{
    return @"methodMappedToLongName";
}

- (NSString *)testConversionColon:(int)useless
{
    return @"testConversionColon:(int)useless";
}

- (NSString *)testConversionEscapeChar$a_b$_:(int)useless
{
    return @"testConversionEscapeChar$a_b$_:(int)useless";
}

- (void)dealloc
{
    if (throwOnDealloc)
        [WebScriptObject throwException:@"Throwing exception on dealloc of ObjCPlugin"];
    
    [super dealloc];
}

@end
