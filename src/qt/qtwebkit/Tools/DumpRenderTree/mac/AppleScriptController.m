/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#import "AppleScriptController.h"

#import <WebKit/WebView.h>
#import <WebKit/WebViewPrivate.h>   // for aeDescByEvaluatingJavaScriptFromString, which is pending API review

@implementation AppleScriptController

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    if (aSelector == @selector(doJavaScript:))
        return NO;
    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)aSelector
{
    if (aSelector == @selector(doJavaScript:))
        return @"doJavaScript";

    return nil;
}

- (id)initWithWebView:(WebView *)wv
{
    self = [super init];
    webView = wv;
    return self;
}

static id convertAEDescToObject(NSAppleEventDescriptor *aeDesc)
{
    id value = nil;

    DescType descType = [aeDesc descriptorType];
    switch (descType) {
        case typeUnicodeText:
            value = [NSString stringWithFormat:@"\"%@\"", [aeDesc stringValue]];
            break;
        case typeLongDateTime:
            if ([[aeDesc data] length] == sizeof(LongDateTime)) {
                LongDateTime d;
                [[aeDesc data] getBytes:&d];
                value = [NSString stringWithFormat:@"%016llX", (unsigned long long)d];
            }
            break;
        case typeAEList:
            value = [NSMutableString stringWithString:@"("];
            int numItems = [aeDesc numberOfItems];
            for (int i = 0; i < numItems; ++i) {
                if (i != 0)
                    [(NSMutableString*)value appendString:@", "];
                id obj = convertAEDescToObject([aeDesc descriptorAtIndex:(i + 1)]);
                [(NSMutableString*)value appendString:[obj description]];
            }
            [(NSMutableString*)value appendString:@")"];
            break;
        case typeType: {
            OSType type = [aeDesc typeCodeValue];

            char typeStr[5];
            typeStr[0] = type >> 24;
            typeStr[1] = type >> 16;
            typeStr[2] = type >> 8;
            typeStr[3] = type;
            typeStr[4] = 0;

            value = [NSString stringWithFormat:@"'%s'", typeStr];
            break;
        }
    }

    if (!value)
        value = [aeDesc stringValue];
    if (!value)
        value = [aeDesc data];

    return value;
}

- (NSString *)doJavaScript:(NSString *)aString
{
    NSAppleEventDescriptor *aeDesc = [webView aeDescByEvaluatingJavaScriptFromString:aString];
    if (!aeDesc)
        return @"(null)";
    
    DescType descType = [aeDesc descriptorType];
    char descTypeStr[5];
    descTypeStr[0] = descType >> 24;
    descTypeStr[1] = descType >> 16;
    descTypeStr[2] = descType >> 8;
    descTypeStr[3] = descType;
    descTypeStr[4] = 0;

    return [NSString stringWithFormat:@"%@ ('%s')", convertAEDescToObject(aeDesc), descTypeStr];
}

@end
