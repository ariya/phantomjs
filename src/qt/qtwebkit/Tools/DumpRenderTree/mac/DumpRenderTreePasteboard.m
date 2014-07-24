/*
 * Copyright (C) 2005, 2006, 2007 Apple, Inc.  All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2007 Eric Seidel <eric@webkit.org>
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
#import "DumpRenderTreeMac.h"
#import "DumpRenderTreePasteboard.h"

#import <WebKit/WebTypesInternal.h>

@interface LocalPasteboard : NSPasteboard
{
    NSMutableArray *typesArray;
    NSMutableSet *typesSet;
    NSMutableDictionary *dataByType;
    NSInteger changeCount;
    NSString *pasteboardName;
}

-(id)initWithName:(NSString *)name;
@end

static NSMutableDictionary *localPasteboards;

@implementation DumpRenderTreePasteboard

// Return a local pasteboard so we don't disturb the real pasteboards when running tests.
+ (NSPasteboard *)_pasteboardWithName:(NSString *)name
{
    static int number = 0;
    if (!name)
        name = [NSString stringWithFormat:@"LocalPasteboard%d", ++number];
    if (!localPasteboards)
        localPasteboards = [[NSMutableDictionary alloc] init];
    LocalPasteboard *pasteboard = [localPasteboards objectForKey:name];
    if (pasteboard)
        return pasteboard;
    pasteboard = [[LocalPasteboard alloc] initWithName:name];
    [localPasteboards setObject:pasteboard forKey:name];
    [pasteboard release];
    return pasteboard;
}

+ (void)releaseLocalPasteboards
{
    [localPasteboards release];
    localPasteboards = nil;
}

// Convenience method for JS so that it doesn't have to try and create a NSArray on the objc side instead
// of the usual WebScriptObject that is passed around
- (NSInteger)declareType:(NSString *)type owner:(id)newOwner
{
    return [self declareTypes:[NSArray arrayWithObject:type] owner:newOwner];
}

@end

@implementation LocalPasteboard

+ (id)alloc
{
    return NSAllocateObject(self, 0, 0);
}

- (id)initWithName:(NSString *)name
{
    typesArray = [[NSMutableArray alloc] init];
    typesSet = [[NSMutableSet alloc] init];
    dataByType = [[NSMutableDictionary alloc] init];
    pasteboardName = [name copy];
    return self;
}

- (void)dealloc
{
    [typesArray release];
    [typesSet release];
    [dataByType release];
    [pasteboardName release];
    [super dealloc];
}

- (NSString *)name
{
    return pasteboardName;
}

- (void)releaseGlobally
{
}

- (NSInteger)declareTypes:(NSArray *)newTypes owner:(id)newOwner
{
    [typesArray removeAllObjects];
    [typesSet removeAllObjects];
    [dataByType removeAllObjects];
    return [self addTypes:newTypes owner:newOwner];
}

- (NSInteger)addTypes:(NSArray *)newTypes owner:(id)newOwner
{
    unsigned count = [newTypes count];
    unsigned i;
    for (i = 0; i < count; ++i) {
        NSString *type = [newTypes objectAtIndex:i];
        NSString *setType = [typesSet member:type];
        if (!setType) {
            setType = [type copy];
            [typesArray addObject:setType];
            [typesSet addObject:setType];
            [setType release];
        }
        if (newOwner && [newOwner respondsToSelector:@selector(pasteboard:provideDataForType:)])
            [newOwner pasteboard:self provideDataForType:setType];
    }
    return ++changeCount;
}

- (NSInteger)changeCount
{
    return changeCount;
}

- (NSArray *)types
{
    return typesArray;
}

- (NSString *)availableTypeFromArray:(NSArray *)types
{
    unsigned count = [types count];
    unsigned i;
    for (i = 0; i < count; ++i) {
        NSString *type = [types objectAtIndex:i];
        NSString *setType = [typesSet member:type];
        if (setType)
            return setType;
    }
    return nil;
}

- (BOOL)setData:(NSData *)data forType:(NSString *)dataType
{
    if (data == nil)
        data = [NSData data];
    if (![typesSet containsObject:dataType])
        return NO;
    [dataByType setObject:data forKey:dataType];
    ++changeCount;
    return YES;
}

- (NSData *)dataForType:(NSString *)dataType
{
    return [dataByType objectForKey:dataType];
}

- (BOOL)setPropertyList:(id)propertyList forType:(NSString *)dataType
{
    CFDataRef data = NULL;
    if (propertyList)
        data = CFPropertyListCreateXMLData(NULL, propertyList);
    BOOL result = [self setData:(NSData *)data forType:dataType];
    if (data)
        CFRelease(data);
    return result;
}

- (BOOL)setString:(NSString *)string forType:(NSString *)dataType
{
    CFDataRef data = NULL;
    if (string) {
        if ([string length] == 0)
            data = CFDataCreate(NULL, NULL, 0);
        else
            data = CFStringCreateExternalRepresentation(NULL, (CFStringRef)string, kCFStringEncodingUTF8, 0);
    }
    BOOL result = [self setData:(NSData *)data forType:dataType];
    if (data)
        CFRelease(data);
    return result;
}

@end
