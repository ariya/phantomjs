/*
 * Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "ColorMac.h"

#import <wtf/RetainPtr.h>
#import <wtf/StdLibExtras.h>

namespace WebCore {

// NSColor calls don't throw, so no need to block Cocoa exceptions in this file

static bool useOldAquaFocusRingColor;

RGBA32 oldAquaFocusRingColor()
{
    return 0xFF7DADD9;
}

void setUsesTestModeFocusRingColor(bool newValue)
{
    useOldAquaFocusRingColor = newValue;
}

bool usesTestModeFocusRingColor()
{
    return useOldAquaFocusRingColor;
}

static RGBA32 makeRGBAFromNSColor(NSColor *c)
{
    CGFloat redComponent;
    CGFloat greenComponent;
    CGFloat blueComponent;
    CGFloat alpha;
    [c getRed:&redComponent green:&greenComponent blue:&blueComponent alpha:&alpha];

    return makeRGBA(255 * redComponent, 255 * greenComponent, 255 * blueComponent, 255 * alpha);
}

Color colorFromNSColor(NSColor *c)
{
    return Color(makeRGBAFromNSColor(c));
}

NSColor *nsColor(const Color& color)
{
    RGBA32 c = color.rgb();
    switch (c) {
        case 0: {
            // Need this to avoid returning nil because cachedRGBAValues will default to 0.
            DEFINE_STATIC_LOCAL(RetainPtr<NSColor>, clearColor, ([NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:0]));
            return clearColor.get();
        }
        case Color::black: {
            DEFINE_STATIC_LOCAL(RetainPtr<NSColor>, blackColor, ([NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:1]));
            return blackColor.get();
        }
        case Color::white: {
            DEFINE_STATIC_LOCAL(RetainPtr<NSColor>, whiteColor, ([NSColor colorWithDeviceRed:1 green:1 blue:1 alpha:1]));
            return whiteColor.get();
        }
        default: {
            const int cacheSize = 32;
            static unsigned cachedRGBAValues[cacheSize];
            static RetainPtr<NSColor>* cachedColors = new RetainPtr<NSColor>[cacheSize];

            for (int i = 0; i != cacheSize; ++i) {
                if (cachedRGBAValues[i] == c)
                    return cachedColors[i].get();
            }

            NSColor *result = [NSColor colorWithDeviceRed:static_cast<CGFloat>(color.red()) / 255
                                                    green:static_cast<CGFloat>(color.green()) / 255
                                                     blue:static_cast<CGFloat>(color.blue()) / 255
                                                    alpha:static_cast<CGFloat>(color.alpha()) / 255];

            static int cursor;
            cachedRGBAValues[cursor] = c;
            cachedColors[cursor] = result;
            if (++cursor == cacheSize)
                cursor = 0;
            return result;
        }
    }
}


} // namespace WebCore
