/*
 * Copyright (C) 2005, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#import "WebNSURLExtras.h"

#import "WebKitNSStringExtras.h"
#import "WebLocalizableStrings.h"
#import "WebNSDataExtras.h"
#import "WebNSObjectExtras.h"
#import "WebSystemInterface.h"
#import <Foundation/NSURLRequest.h>
#import <WebCore/KURL.h>
#import <WebCore/LoaderNSURLExtras.h>
#import <WebCore/WebCoreNSURLExtras.h>
#import <WebKitSystemInterface.h>
#import <wtf/Assertions.h>
#import <unicode/uchar.h>
#import <unicode/uscript.h>

using namespace WebCore;
using namespace WTF;

#define URL_BYTES_BUFFER_LENGTH 2048

@implementation NSURL (WebNSURLExtras)

+ (NSURL *)_web_URLWithUserTypedString:(NSString *)string relativeToURL:(NSURL *)URL
{
    return URLWithUserTypedString(string, URL);
}

+ (NSURL *)_web_URLWithUserTypedString:(NSString *)string
{
    return URLWithUserTypedString(string, nil);
}

+ (NSURL *)_web_URLWithDataAsString:(NSString *)string
{
    if (string == nil) {
        return nil;
    }
    return [self _web_URLWithDataAsString:string relativeToURL:nil];
}

+ (NSURL *)_web_URLWithDataAsString:(NSString *)string relativeToURL:(NSURL *)baseURL
{
    if (string == nil) {
        return nil;
    }
    string = [string _webkit_stringByTrimmingWhitespace];
    NSData *data = [string dataUsingEncoding:NSISOLatin1StringEncoding];
    return URLWithData(data, baseURL);
}

+ (NSURL *)_web_URLWithData:(NSData *)data
{
    return URLWithData(data, nil);
}      

+ (NSURL *)_web_URLWithData:(NSData *)data relativeToURL:(NSURL *)baseURL
{
    return URLWithData(data, baseURL);
}

- (NSData *)_web_originalData
{
    return originalURLData(self);
}

- (NSString *)_web_originalDataAsString
{
    return [[[NSString alloc] initWithData:originalURLData(self) encoding:NSISOLatin1StringEncoding] autorelease];
}

- (NSString *)_web_userVisibleString
{
    return userVisibleString(self);
}

- (BOOL)_web_isEmpty
{
    if (!CFURLGetBaseURL((CFURLRef)self))
        return CFURLGetBytes((CFURLRef)self, NULL, 0) == 0;
    return [originalURLData(self) length] == 0;
}

- (const char *)_web_URLCString
{
    NSMutableData *data = [NSMutableData data];
    [data appendData:originalURLData(self)];
    [data appendBytes:"\0" length:1];
    return (const char *)[data bytes];
 }

- (NSURL *)_webkit_canonicalize
{    
    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:self];
    Class concreteClass = WKNSURLProtocolClassForRequest(request);
    if (!concreteClass) {
        [request release];
        return self;
    }
    
    // This applies NSURL's concept of canonicalization, but not KURL's concept. It would
    // make sense to apply both, but when we tried that it caused a performance degradation
    // (see 5315926). It might make sense to apply only the KURL concept and not the NSURL
    // concept, but it's too risky to make that change for WebKit 3.0.
    NSURLRequest *newRequest = [concreteClass canonicalRequestForRequest:request];
    NSURL *newURL = [newRequest URL]; 
    NSURL *result = [[newURL retain] autorelease]; 
    [request release];
    
    return result;
}

- (NSURL *)_web_URLByTruncatingOneCharacterBeforeComponent:(CFURLComponentType)component
{
    return URLByTruncatingOneCharacterBeforeComponent(self, component);
}

- (NSURL *)_webkit_URLByRemovingFragment
{
    return URLByTruncatingOneCharacterBeforeComponent(self, kCFURLComponentFragment);
}

- (NSURL *)_webkit_URLByRemovingResourceSpecifier
{
    return URLByTruncatingOneCharacterBeforeComponent(self, kCFURLComponentResourceSpecifier);
}

- (NSURL *)_web_URLByRemovingUserInfo
{
    return URLByRemovingUserInfo(self);
}

- (BOOL)_webkit_isJavaScriptURL
{
    return [[self _web_originalDataAsString] _webkit_isJavaScriptURL];
}

- (NSString *)_webkit_scriptIfJavaScriptURL
{
    return [[self absoluteString] _webkit_scriptIfJavaScriptURL];
}

- (BOOL)_webkit_isFileURL
{    
    return [[self _web_originalDataAsString] _webkit_isFileURL];
}

- (BOOL)_webkit_isFTPDirectoryURL
{
    return [[self _web_originalDataAsString] _webkit_isFTPDirectoryURL];
}

- (BOOL)_webkit_shouldLoadAsEmptyDocument
{
    return [[self _web_originalDataAsString] _webkit_hasCaseInsensitivePrefix:@"about:"] || [self _web_isEmpty];
}

- (NSURL *)_web_URLWithLowercasedScheme
{
    CFRange range;
    CFURLGetByteRangeForComponent((CFURLRef)self, kCFURLComponentScheme, &range);
    if (range.location == kCFNotFound) {
        return self;
    }
    
    UInt8 static_buffer[URL_BYTES_BUFFER_LENGTH];
    UInt8 *buffer = static_buffer;
    CFIndex bytesFilled = CFURLGetBytes((CFURLRef)self, buffer, URL_BYTES_BUFFER_LENGTH);
    if (bytesFilled == -1) {
        CFIndex bytesToAllocate = CFURLGetBytes((CFURLRef)self, NULL, 0);
        buffer = static_cast<UInt8 *>(malloc(bytesToAllocate));
        bytesFilled = CFURLGetBytes((CFURLRef)self, buffer, bytesToAllocate);
        ASSERT(bytesFilled == bytesToAllocate);
    }
    
    int i;
    BOOL changed = NO;
    for (i = 0; i < range.length; ++i) {
        char c = buffer[range.location + i];
        char lower = toASCIILower(c);
        if (c != lower) {
            buffer[range.location + i] = lower;
            changed = YES;
        }
    }
    
    NSURL *result = changed
        ? (NSURL *)WebCFAutorelease(CFURLCreateAbsoluteURLWithBytes(NULL, buffer, bytesFilled, kCFStringEncodingUTF8, nil, YES))
        : (NSURL *)self;

    if (buffer != static_buffer) {
        free(buffer);
    }
    
    return result;
}


-(NSData *)_web_schemeSeparatorWithoutColon
{
    NSData *result = nil;
    CFRange rangeWithSeparators;
    CFRange range = CFURLGetByteRangeForComponent((CFURLRef)self, kCFURLComponentScheme, &rangeWithSeparators);
    if (rangeWithSeparators.location != kCFNotFound) {
        NSString *absoluteString = [self absoluteString];
        NSRange separatorsRange = NSMakeRange(range.location + range.length + 1, rangeWithSeparators.length - range.length - 1);
        if (separatorsRange.location + separatorsRange.length <= [absoluteString length]) {
            NSString *slashes = [absoluteString substringWithRange:separatorsRange];
            result = [slashes dataUsingEncoding:NSISOLatin1StringEncoding];
        }
    }
    return result;
}

-(NSData *)_web_dataForURLComponentType:(CFURLComponentType)componentType
{
    return dataForURLComponentType(self, componentType);
}

-(NSData *)_web_schemeData
{
    return dataForURLComponentType(self, kCFURLComponentScheme);
}

-(NSData *)_web_hostData
{
    NSData *result = dataForURLComponentType(self, kCFURLComponentHost);
    NSData *scheme = [self _web_schemeData];
    // Take off localhost for file
    if ([scheme _web_isCaseInsensitiveEqualToCString:"file"]) {
        return ([result _web_isCaseInsensitiveEqualToCString:"localhost"]) ? nil : result;
    }
    return result;
}

- (NSString *)_web_hostString
{
    return [[[NSString alloc] initWithData:[self _web_hostData] encoding:NSUTF8StringEncoding] autorelease];
}

- (NSString *)_webkit_suggestedFilenameWithMIMEType:(NSString *)MIMEType
{
    return suggestedFilenameWithMIMEType(self, MIMEType);
}

- (NSURL *)_webkit_URLFromURLOrSchemelessFileURL
{
    if ([self scheme])
        return self;

    return [NSURL URLWithString:[@"file:" stringByAppendingString:[self absoluteString]]];
}

@end

@implementation NSString (WebNSURLExtras)

- (BOOL)_web_isUserVisibleURL
{
    return isUserVisibleURL(self);
}

- (BOOL)_webkit_isJavaScriptURL
{
    return [self _webkit_hasCaseInsensitivePrefix:@"javascript:"];
}

- (BOOL)_webkit_isFileURL
{
    return [self rangeOfString:@"file:" options:(NSCaseInsensitiveSearch | NSAnchoredSearch)].location != NSNotFound;
}

- (NSString *)_webkit_stringByReplacingValidPercentEscapes
{
    return decodeURLEscapeSequences(self);
}

- (NSString *)_webkit_scriptIfJavaScriptURL
{
    if (![self _webkit_isJavaScriptURL]) {
        return nil;
    }
    return [[self substringFromIndex:11] _webkit_stringByReplacingValidPercentEscapes];
}

- (BOOL)_webkit_isFTPDirectoryURL
{
    int length = [self length];
    if (length < 5) {  // 5 is length of "ftp:/"
        return NO;
    }
    unichar lastChar = [self characterAtIndex:length - 1];
    return lastChar == '/' && [self _webkit_hasCaseInsensitivePrefix:@"ftp:"];
}

- (BOOL)_web_hostNameNeedsDecodingWithRange:(NSRange)range
{
    return hostNameNeedsDecodingWithRange(self, range);
}

- (BOOL)_web_hostNameNeedsEncodingWithRange:(NSRange)range
{
    return hostNameNeedsEncodingWithRange(self, range);
}

- (NSString *)_web_decodeHostNameWithRange:(NSRange)range
{
    return decodeHostNameWithRange(self, range);
}

- (NSString *)_web_encodeHostNameWithRange:(NSRange)range
{
    return encodeHostNameWithRange(self, range);
}

- (NSString *)_web_decodeHostName
{
    return decodeHostName(self);
}

- (NSString *)_web_encodeHostName
{
    return encodeHostName(self);
}

-(NSRange)_webkit_rangeOfURLScheme
{
    NSRange colon = [self rangeOfString:@":"];
    if (colon.location != NSNotFound && colon.location > 0) {
        NSRange scheme = {0, colon.location};
        static NSCharacterSet *InverseSchemeCharacterSet = nil;
        if (!InverseSchemeCharacterSet) {
            /*
             This stuff is very expensive.  10-15 msec on a 2x1.2GHz.  If not cached it swamps
             everything else when adding items to the autocomplete DB.  Makes me wonder if we
             even need to enforce the character set here.
            */
            NSString *acceptableCharacters = @"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+.-";
            InverseSchemeCharacterSet = [[[NSCharacterSet characterSetWithCharactersInString:acceptableCharacters] invertedSet] retain];
        }
        NSRange illegals = [self rangeOfCharacterFromSet:InverseSchemeCharacterSet options:0 range:scheme];
        if (illegals.location == NSNotFound)
            return scheme;
    }
    return NSMakeRange(NSNotFound, 0);
}

-(BOOL)_webkit_looksLikeAbsoluteURL
{
    // Trim whitespace because _web_URLWithString allows whitespace.
    return [[self _webkit_stringByTrimmingWhitespace] _webkit_rangeOfURLScheme].location != NSNotFound;
}

- (NSString *)_webkit_URLFragment
{
    NSRange fragmentRange;
    
    fragmentRange = [self rangeOfString:@"#" options:NSLiteralSearch];
    if (fragmentRange.location == NSNotFound)
        return nil;
    return [self substringFromIndex:fragmentRange.location + 1];
}

@end
