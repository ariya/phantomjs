/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "WebStringTruncator.h"

#import "WebSystemInterface.h"
#import <WebCore/Font.h>
#import <WebCore/FontCache.h>
#import <WebCore/FontPlatformData.h>
#import <WebCore/StringTruncator.h>
#import <wtf/StdLibExtras.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

static NSFont *defaultMenuFont()
{
    static NSFont *defaultMenuFont = nil;
    if (!defaultMenuFont) {
        defaultMenuFont = [NSFont menuFontOfSize:0];
        CFRetain(defaultMenuFont);
    }
    return defaultMenuFont;
}

static Font& fontFromNSFont(NSFont *font)
{
    static NSFont *currentFont;
    DEFINE_STATIC_LOCAL(Font, currentRenderer, ());

    if ([font isEqual:currentFont])
        return currentRenderer;
    if (currentFont)
        CFRelease(currentFont);
    currentFont = font;
    CFRetain(currentFont);
    FontPlatformData f(font, [font pointSize]);
    currentRenderer = Font(f, ![[NSGraphicsContext currentContext] isDrawingToScreen]);
    return currentRenderer;
}

@implementation WebStringTruncator

+ (void)initialize
{
    InitWebCoreSystemInterface();
}

+ (NSString *)centerTruncateString:(NSString *)string toWidth:(float)maxWidth
{
    FontCachePurgePreventer fontCachePurgePreventer;
    return StringTruncator::centerTruncate(string, maxWidth, fontFromNSFont(defaultMenuFont()));
}

+ (NSString *)centerTruncateString:(NSString *)string toWidth:(float)maxWidth withFont:(NSFont *)font
{
    FontCachePurgePreventer fontCachePurgePreventer;
    return StringTruncator::centerTruncate(string, maxWidth, fontFromNSFont(font));
}

+ (NSString *)rightTruncateString:(NSString *)string toWidth:(float)maxWidth withFont:(NSFont *)font
{
    FontCachePurgePreventer fontCachePurgePreventer;
    return StringTruncator::rightTruncate(string, maxWidth, fontFromNSFont(font));
}

+ (float)widthOfString:(NSString *)string font:(NSFont *)font
{
    FontCachePurgePreventer fontCachePurgePreventer;
    return StringTruncator::width(string, fontFromNSFont(font));
}

@end
