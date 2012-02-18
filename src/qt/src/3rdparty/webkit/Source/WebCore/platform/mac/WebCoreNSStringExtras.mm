/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
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
#import "WebCoreNSStringExtras.h"

#import <wtf/RetainPtr.h>

BOOL stringIsCaseInsensitiveEqualToString(NSString *first, NSString *second)
{
    return [first compare:second options:(NSCaseInsensitiveSearch|NSLiteralSearch)] == NSOrderedSame;
}

BOOL hasCaseInsensitiveSuffix(NSString *string, NSString *suffix)
{
    return [string rangeOfString:suffix options:(NSCaseInsensitiveSearch | NSBackwardsSearch | NSAnchoredSearch)].location != NSNotFound;
}

BOOL hasCaseInsensitiveSubstring(NSString *string, NSString *substring)
{
    return [string rangeOfString:substring options:NSCaseInsensitiveSearch].location != NSNotFound;
}

NSString *filenameByFixingIllegalCharacters(NSString *string)
{
    NSMutableString *filename = [[string mutableCopy] autorelease];

    // Strip null characters.
    unichar nullChar = 0;
    [filename replaceOccurrencesOfString:[NSString stringWithCharacters:&nullChar length:0] withString:@"" options:0 range:NSMakeRange(0, [filename length])];

    // Replace "/" with "-".
    [filename replaceOccurrencesOfString:@"/" withString:@"-" options:0 range:NSMakeRange(0, [filename length])];

    // Replace ":" with "-".
    [filename replaceOccurrencesOfString:@":" withString:@"-" options:0 range:NSMakeRange(0, [filename length])];
    
    // Strip leading dots.
    while ([filename hasPrefix:@"."]) {
        [filename deleteCharactersInRange:NSMakeRange(0,1)];
    }
    
    return filename;
}

CFStringEncoding stringEncodingForResource(Handle resource)
{
    short resRef = HomeResFile(resource);
    if (ResError() != noErr)
        return NSMacOSRomanStringEncoding;
    
    // Get the FSRef for the current resource file
    FSRef fref;
    OSStatus error = FSGetForkCBInfo(resRef, 0, NULL, NULL, NULL, &fref, NULL);
    if (error != noErr)
        return NSMacOSRomanStringEncoding;
    
    RetainPtr<CFURLRef> url(AdoptCF, CFURLCreateFromFSRef(NULL, &fref));
    if (!url)
        return NSMacOSRomanStringEncoding;

    NSString *path = [(NSURL *)url.get() path];

    // Get the lproj directory name
    path = [path stringByDeletingLastPathComponent];
    if (!stringIsCaseInsensitiveEqualToString([path pathExtension], @"lproj"))
        return NSMacOSRomanStringEncoding;
    
    NSString *directoryName = [[path stringByDeletingPathExtension] lastPathComponent];
    RetainPtr<CFStringRef> locale(AdoptCF, CFLocaleCreateCanonicalLocaleIdentifierFromString(NULL, (CFStringRef)directoryName));
    if (!locale)
        return NSMacOSRomanStringEncoding;

    LangCode lang;
    RegionCode region;
    error = LocaleStringToLangAndRegionCodes([(NSString *)locale.get() UTF8String], &lang, &region);
    if (error != noErr)
        return NSMacOSRomanStringEncoding;

    TextEncoding encoding;
    error = UpgradeScriptInfoToTextEncoding(kTextScriptDontCare, lang, region, NULL, &encoding);
    if (error != noErr)
        return NSMacOSRomanStringEncoding;
    
    return encoding;
}

