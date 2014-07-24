/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#import "config.h"
#import "ObjCController.h"

// Avoid compile error in DOMPrivate.h.
@class NSFont;

#import <JavaScriptCore/JavaScriptCore.h>
#import <WebKit/DOMAbstractView.h>
#import <WebKit/DOMPrivate.h>
#import <WebKit/WebScriptObject.h>
#import <WebKit/WebView.h>
#import <pthread.h>
#import <wtf/Assertions.h>

// Remove this once hasWebScriptKey has been made public.
@interface WebScriptObject (StagedForPublic)
- (BOOL)hasWebScriptKey:(NSString *)name;
@end

static void* runJavaScriptThread(void* arg)
{
    JSGlobalContextRef ctx = JSGlobalContextCreate(0);
    JSStringRef scriptRef = JSStringCreateWithUTF8CString("'Hello World!'");

    JSValueRef exception = 0;
    JSEvaluateScript(ctx, scriptRef, 0, 0, 1, &exception);
    ASSERT(!exception);

    JSGlobalContextRelease(ctx);
    JSStringRelease(scriptRef);
    
    return 0;
}

@implementation ObjCController

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    if (0
            || aSelector == @selector(classNameOf:)
            || aSelector == @selector(isObject:instanceOf:)
            || aSelector == @selector(objectOfClass:)
            || aSelector == @selector(arrayOfString)
            || aSelector == @selector(identityIsEqual::)
            || aSelector == @selector(longLongRoundTrip:)
            || aSelector == @selector(unsignedLongLongRoundTrip:)
            || aSelector == @selector(testWrapperRoundTripping:)
            || aSelector == @selector(accessStoredWebScriptObject)
            || aSelector == @selector(storeWebScriptObject:)
            || aSelector == @selector(testValueForKey)
            || aSelector == @selector(testHasWebScriptKey:)
            || aSelector == @selector(testArray)
            || aSelector == @selector(setSelectElement:selectedIndex:allowingMultiple:)
        )
        return NO;
    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(classNameOf:))
        return @"className";
    if (aSelector == @selector(isObject:instanceOf:))
        return @"isObjectInstanceOf";
    if (aSelector == @selector(objectOfClass:))
        return @"objectOfClass";
    if (aSelector == @selector(arrayOfString))
        return @"arrayOfString";
    if (aSelector == @selector(identityIsEqual::))
        return @"identityIsEqual";
    if (aSelector == @selector(longLongRoundTrip:))
        return @"longLongRoundTrip";
    if (aSelector == @selector(unsignedLongLongRoundTrip:))
        return @"unsignedLongLongRoundTrip";
    if (aSelector == @selector(testWrapperRoundTripping:))
        return @"testWrapperRoundTripping";
    if (aSelector == @selector(storeWebScriptObject:))
        return @"storeWebScriptObject";
    if (aSelector == @selector(testValueForKey))
        return @"testValueForKey";
    if (aSelector == @selector(testHasWebScriptKey:))
        return @"testHasWebScriptKey";
    if (aSelector == @selector(testArray))
        return @"testArray";
    if (aSelector == @selector(setSelectElement:selectedIndex:allowingMultiple:))
        return @"setSelectElementSelectedIndexAllowingMultiple";

    return nil;
}

- (BOOL)isObject:(id)object instanceOf:(NSString *)aClass
{
    if (!object)
        return [aClass isEqualToString:@"nil"];

    return [object isKindOfClass:NSClassFromString(aClass)];
}
            
- (NSString *)classNameOf:(id)object
{
    if (!object)
        return @"nil";
    return NSStringFromClass([object class]);
}

- (id)objectOfClass:(NSString *)aClass
{
    if ([aClass isEqualToString:@"NSNull"])
        return [NSNull null];
    if ([aClass isEqualToString:@"WebUndefined"])
        return [WebUndefined undefined];
    if ([aClass isEqualToString:@"NSCFBoolean"])
        return [NSNumber numberWithBool:true];
    if ([aClass isEqualToString:@"NSCFNumber"])
        return [NSNumber numberWithInt:1];
    if ([aClass isEqualToString:@"NSCFString"])
        return @"";
    if ([aClass isEqualToString:@"WebScriptObject"])
        return self;
    if ([aClass isEqualToString:@"NSArray"])
        return [NSArray array];

    return nil;
}

- (NSArray *)arrayOfString
{
    NSString *strings[3];
    strings[0] = @"one";
    strings[1] = @"two";
    strings[2] = @"three";
    NSArray *array = [NSArray arrayWithObjects:strings count:3];
    return array;
}

- (BOOL)identityIsEqual:(WebScriptObject *)a :(WebScriptObject *)b
{
    if ([a isKindOfClass:[NSString class]] && [b isKindOfClass:[NSString class]])
        return [(NSString *)a isEqualToString:(NSString *)b];
    return a == b;
}

- (long long)longLongRoundTrip:(long long)num
{
    return num;
}

- (unsigned long long)unsignedLongLongRoundTrip:(unsigned long long)num
{
    return num;
}

- (void)testValueForKey
{
    ASSERT(storedWebScriptObject);
    
    @try {
        [storedWebScriptObject valueForKey:@"ThisKeyDoesNotExist"];
    } @catch (NSException *e) {
    }

    pthread_t pthread;
    pthread_create(&pthread, 0, &runJavaScriptThread, 0);
    pthread_join(pthread, 0);
}

- (BOOL)testHasWebScriptKey:(NSString *)key
{
    ASSERT(storedWebScriptObject);
    return [storedWebScriptObject hasWebScriptKey:key];
}

- (BOOL)testWrapperRoundTripping:(WebScriptObject *)webScriptObject
{
    JSObjectRef jsObject = [webScriptObject JSObject];

    if (!jsObject)
        return false;

    if (!webScriptObject)
        return false;

    if ([[webScriptObject evaluateWebScript:@"({ })"] class] != [webScriptObject class])
        return false;

    [webScriptObject setValue:[NSNumber numberWithInt:666] forKey:@"key"];
    if (![[webScriptObject valueForKey:@"key"] isKindOfClass:[NSNumber class]] ||
        ![[webScriptObject valueForKey:@"key"] isEqualToNumber:[NSNumber numberWithInt:666]])
        return false;

    [webScriptObject removeWebScriptKey:@"key"];
    @try {
        if ([webScriptObject valueForKey:@"key"])
            return false;
    } @catch(NSException *exception) {
        // NSObject throws an exception if the key doesn't exist.
    }

    [webScriptObject setWebScriptValueAtIndex:0 value:webScriptObject];
    if ([webScriptObject webScriptValueAtIndex:0] != webScriptObject)
        return false;

    if ([[webScriptObject stringRepresentation] isEqualToString:@"[Object object]"])
        return false;

    if ([webScriptObject callWebScriptMethod:@"returnThis" withArguments:nil] != webScriptObject)
        return false;

    return true;
}

- (void)accessStoredWebScriptObject
{
#if !ASSERT_DISABLED
    BOOL isWindowObject = [storedWebScriptObject isKindOfClass:[DOMAbstractView class]];
    JSObjectRef jsObject = [storedWebScriptObject JSObject];
    ASSERT((jsObject && isWindowObject) || (!jsObject && !isWindowObject));
#endif
    [storedWebScriptObject callWebScriptMethod:@"" withArguments:nil];
    [storedWebScriptObject evaluateWebScript:@""];
    [storedWebScriptObject setValue:[WebUndefined undefined] forKey:@"key"];
    [storedWebScriptObject valueForKey:@"key"];
    [storedWebScriptObject removeWebScriptKey:@"key"];
    [storedWebScriptObject stringRepresentation];
    [storedWebScriptObject webScriptValueAtIndex:0];
    [storedWebScriptObject setWebScriptValueAtIndex:0 value:[WebUndefined undefined]];
    [storedWebScriptObject setException:@"exception"];
}

- (void)storeWebScriptObject:(WebScriptObject *)webScriptObject
{
    if (webScriptObject == storedWebScriptObject)
        return;

    [storedWebScriptObject release];
    storedWebScriptObject = [webScriptObject retain];
}

- (NSArray *)testArray
{
    return [NSArray array];
}

- (void)dealloc
{
    [storedWebScriptObject release];
    [super dealloc];
}

- (id)invokeUndefinedMethodFromWebScript:(NSString *)name withArguments:(NSArray *)args
{
    // FIXME: Perhaps we should log that this has been called.
    return nil;
}

// MARK: -
// MARK: Testing Objective-C DOM HTML Bindings

- (void)setSelectElement:(WebScriptObject *)element selectedIndex:(int)index allowingMultiple:(BOOL)allowingMultiple
{
    if (![element isKindOfClass:[DOMHTMLSelectElement class]])
        return;

    DOMHTMLSelectElement *select = (DOMHTMLSelectElement*)element;
    [select _activateItemAtIndex:index allowMultipleSelection:allowingMultiple];
}

@end
