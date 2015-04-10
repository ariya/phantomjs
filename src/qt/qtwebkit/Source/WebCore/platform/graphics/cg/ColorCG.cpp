/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Color.h"

#if USE(CG)

#include "GraphicsContextCG.h"
#include <wtf/Assertions.h>
#include <wtf/RetainPtr.h>
#include <ApplicationServices/ApplicationServices.h>

namespace WebCore {

Color::Color(CGColorRef color)
{
    if (!color) {
        m_color = 0;
        m_valid = false;
        return;
    }

    size_t numComponents = CGColorGetNumberOfComponents(color);
    const CGFloat* components = CGColorGetComponents(color);

    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    switch (numComponents) {
    case 2:
        r = g = b = components[0];
        a = components[1];
        break;
    case 4:
        r = components[0];
        g = components[1];
        b = components[2];
        a = components[3];
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    m_color = makeRGBA(r * 255, g * 255, b * 255, a * 255);
    m_valid = true;
}

static inline CGColorSpaceRef cachedCGColorSpace(ColorSpace colorSpace)
{
    switch (colorSpace) {
    case ColorSpaceDeviceRGB:
        return deviceRGBColorSpaceRef();
    case ColorSpaceSRGB:
        return sRGBColorSpaceRef();
    case ColorSpaceLinearRGB:
        return linearRGBColorSpaceRef();
    }
    ASSERT_NOT_REACHED();
    return deviceRGBColorSpaceRef();
}

static CGColorRef leakCGColor(const Color& color, ColorSpace colorSpace)
{
    CGFloat components[4];
    color.getRGBA(components[0], components[1], components[2], components[3]);
    return CGColorCreate(cachedCGColorSpace(colorSpace), components);
}

template<ColorSpace colorSpace> static CGColorRef cachedCGColor(const Color& color)
{
    switch (color.rgb()) {
    case Color::transparent: {
        static CGColorRef transparentCGColor = leakCGColor(color, colorSpace);
        return transparentCGColor;
    }
    case Color::black: {
        static CGColorRef blackCGColor = leakCGColor(color, colorSpace);
        return blackCGColor;
    }
    case Color::white: {
        static CGColorRef whiteCGColor = leakCGColor(color, colorSpace);
        return whiteCGColor;
    }
    }

    ASSERT(color.rgb());

    const size_t cacheSize = 32;
    static RGBA32 cachedRGBAValues[cacheSize];
    static RetainPtr<CGColorRef>* cachedCGColors = new RetainPtr<CGColorRef>[cacheSize];

    for (size_t i = 0; i < cacheSize; ++i) {
        if (cachedRGBAValues[i] == color.rgb())
            return cachedCGColors[i].get();
    }

    CGColorRef newCGColor = leakCGColor(color, colorSpace);

    static size_t cursor;
    cachedRGBAValues[cursor] = color.rgb();
    cachedCGColors[cursor] = adoptCF(newCGColor);
    if (++cursor == cacheSize)
        cursor = 0;

    return newCGColor;
}

CGColorRef cachedCGColor(const Color& color, ColorSpace colorSpace)
{
    switch (colorSpace) {
    case ColorSpaceDeviceRGB:
        return cachedCGColor<ColorSpaceDeviceRGB>(color);
    case ColorSpaceSRGB:
        return cachedCGColor<ColorSpaceSRGB>(color);
    case ColorSpaceLinearRGB:
        return cachedCGColor<ColorSpaceLinearRGB>(color);
    }
    ASSERT_NOT_REACHED();
    return cachedCGColor(color, ColorSpaceDeviceRGB);
}

}

#endif // USE(CG)
