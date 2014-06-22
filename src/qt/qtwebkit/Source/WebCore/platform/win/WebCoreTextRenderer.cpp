/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#include "WebCoreTextRenderer.h"

#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "GraphicsContext.h"
#include "StringTruncator.h"
#include "TextRun.h"
#include <wtf/unicode/Unicode.h>

namespace WebCore {

static bool shouldUseFontSmoothing = true;

static bool isOneLeftToRightRun(const TextRun& run)
{
    for (int i = 0; i < run.length(); i++) {
        WTF::Unicode::Direction direction = WTF::Unicode::direction(run[i]);
        if (direction == WTF::Unicode::RightToLeft || direction > WTF::Unicode::OtherNeutral)
            return false;
    }
    return true;
}

static void doDrawTextAtPoint(GraphicsContext& context, const String& text, const IntPoint& point, const Font& font, const Color& color, int underlinedIndex)
{
    FontCachePurgePreventer fontCachePurgePreventer;

    TextRun run(text.characters(), text.length());

    context.setFillColor(color, ColorSpaceDeviceRGB);
    if (isOneLeftToRightRun(run))
        font.drawText(&context, run, point);
    else
        context.drawBidiText(font, run, point);

    if (underlinedIndex >= 0) {
        ASSERT_WITH_SECURITY_IMPLICATION(underlinedIndex < static_cast<int>(text.length()));

        int beforeWidth;
        if (underlinedIndex > 0) {
            TextRun beforeRun(text.characters(), underlinedIndex);
            beforeWidth = font.width(beforeRun);
        } else
            beforeWidth = 0;

        TextRun underlinedRun(text.characters() + underlinedIndex, 1);
        int underlinedWidth = font.width(underlinedRun);

        IntPoint underlinePoint(point);
        underlinePoint.move(beforeWidth, 1);

        context.setStrokeColor(color, ColorSpaceDeviceRGB);
        context.drawLineForText(underlinePoint, underlinedWidth, false);
    }
}

void WebCoreDrawTextAtPoint(GraphicsContext& context, const String& text, const IntPoint& point, const Font& font, const Color& color, int underlinedIndex)
{
    context.save();

    doDrawTextAtPoint(context, text, point, font, color, underlinedIndex);

    context.restore();
}

void WebCoreDrawDoubledTextAtPoint(GraphicsContext& context, const String& text, const IntPoint& point, const Font& font, const Color& topColor, const Color& bottomColor, int underlinedIndex)
{
    context.save();

    IntPoint textPos = point;

    doDrawTextAtPoint(context, text, textPos, font, bottomColor, underlinedIndex);
    textPos.move(0, -1);
    doDrawTextAtPoint(context, text, textPos, font, topColor, underlinedIndex);

    context.restore();
}

float WebCoreTextFloatWidth(const String& text, const Font& font)
{
    FontCachePurgePreventer fontCachePurgePreventer;

    return StringTruncator::width(text, font, StringTruncator::EnableRoundingHacks);
}

void WebCoreSetShouldUseFontSmoothing(bool smooth)
{
    shouldUseFontSmoothing = smooth;
}

bool WebCoreShouldUseFontSmoothing()
{
    return shouldUseFontSmoothing;
}

void WebCoreSetAlwaysUsesComplexTextCodePath(bool complex)
{
    Font::setCodePath(complex ? Font::Complex : Font::Auto);
}

bool WebCoreAlwaysUsesComplexTextCodePath()
{
    return Font::codePath() == Font::Complex;
}

} // namespace WebCore
