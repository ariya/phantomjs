/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "Font.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "FontCache.h"
#include "FontData.h"
#include "FontGlyphs.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "TextRun.h"
#include "WidthIterator.h"
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/unicode/Unicode.h>

#include <windows.h>

using namespace WTF::Unicode;

namespace WebCore {

HDC g_screenDC = GetDC(0);

class ScreenDcReleaser {
public:
    ~ScreenDcReleaser()
    {
        ReleaseDC(0, g_screenDC);
    }
};

ScreenDcReleaser releaseScreenDc;

void Font::drawGlyphs(GraphicsContext* graphicsContext, const SimpleFontData* fontData, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point) const
{
    graphicsContext->drawText(fontData, glyphBuffer, from, numGlyphs, point);
}

class TextRunComponent {
public:
    TextRunComponent() : m_textRun(0, 0) {}
    TextRunComponent(const UChar *start, int length, const TextRun& parentTextRun, const Font &font, int offset);
    TextRunComponent(int spaces, const Font &font, int offset);
    ~TextRunComponent() { m_textRun; }

    bool isSpace() const { return m_spaces; }
    int textLength() const { return m_spaces ? m_spaces : m_textRun.length(); }

    TextRun m_textRun;
    float m_width;
    int m_offset;
    int m_spaces;
};

TextRunComponent::TextRunComponent(const UChar *start, int length, const TextRun& parentTextRun, const Font &font, int o)
    : m_textRun(start, length, 0, 0
        , parentTextRun.allowsTrailingExpansion() ? TextRun::AllowTrailingExpansion : TextRun::ForbidTrailingExpansion
        , parentTextRun.direction()
        , parentTextRun.directionalOverride())
    , m_offset(o)
    , m_spaces(0)
{
    m_textRun.setTabSize(parentTextRun.allowTabs(), parentTextRun.tabSize());

    WidthIterator it(&font, m_textRun);
    it.advance(m_textRun.length(), 0);
    m_width = it.m_runWidthSoFar;
}

TextRunComponent::TextRunComponent(int s, const Font &font, int o)
    : m_textRun(0, 0)
    , m_offset(o)
    , m_spaces(s)
{
    m_width = s * font.primaryFont()->widthForGlyph(' ');
}

typedef Vector<TextRunComponent, 128> TextRunComponents;

static int generateComponents(TextRunComponents* components, const Font &font, const TextRun &run)
{
    int letterSpacing = font.letterSpacing();
    int wordSpacing = font.wordSpacing();
    int padding = run.expansion();
    int numSpaces = 0;
    if (padding) {
        for (int i = 0; i < run.length(); i++)
            if (Font::treatAsSpace(run[i]))
                ++numSpaces;
    }

    int offset = 0;
    if (letterSpacing) {
        // need to draw every letter on it's own
        int start = 0;
        if (Font::treatAsSpace(run[0])) {
            int add = 0;
            if (numSpaces) {
                add = padding/numSpaces;
                padding -= add;
                --numSpaces;
            }
            components->append(TextRunComponent(1, font, offset));
            offset += add + letterSpacing + components->last().m_width;
            start = 1;
        }
        for (int i = 1; i < run.length(); ++i) {
            UChar ch = run[i];
            if (U16_IS_LEAD(ch) && U16_IS_TRAIL(run[i-1]))
                ch = U16_GET_SUPPLEMENTARY(ch, run[i-1]);
            if (U16_IS_TRAIL(ch) || category(ch) == Mark_NonSpacing)
                continue;
            if (Font::treatAsSpace(run[i])) {
                int add = 0;
                if (i - start > 0) {
                    components->append(TextRunComponent(run.characters16() + start, i - start,
                                                        run, font, offset));
                    offset += components->last().m_width + letterSpacing;
                }
                if (numSpaces) {
                    add = padding/numSpaces;
                    padding -= add;
                    --numSpaces;
                }
                components->append(TextRunComponent(1, font, offset));
                offset += wordSpacing + add + components->last().m_width + letterSpacing;
                start = i + 1;
                continue;
            }
            if (i - start > 0) {
                components->append(TextRunComponent(run.characters16() + start, i - start,
                                                    run,
                                                    font, offset));
                offset += components->last().m_width + letterSpacing;
            }
            start = i;
        }
        if (run.length() - start > 0) {
            components->append(TextRunComponent(run.characters16() + start, run.length() - start,
                                                run,
                                                font, offset));
            offset += components->last().m_width;
        }
        offset += letterSpacing;
    } else {
        int start = 0;
        for (int i = 0; i < run.length(); ++i) {
            if (Font::treatAsSpace(run[i])) {
                if (i - start > 0) {
                    components->append(TextRunComponent(run.characters16() + start, i - start,
                                                        run,
                                                        font, offset));
                    offset += components->last().m_width;
                }
                int add = 0;
                if (numSpaces) {
                    add = padding/numSpaces;
                    padding -= add;
                    --numSpaces;
                }
                components->append(TextRunComponent(1, font, offset));
                offset += add + components->last().m_width;
                if (i)
                    offset += wordSpacing;
                start = i + 1;
            }
        }
        if (run.length() - start > 0) {
            components->append(TextRunComponent(run.characters16() + start, run.length() - start,
                                                run,
                                                font, offset));
            offset += components->last().m_width;
        }
    }
    return offset;
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point,
                           int from, int to) const
{
    if (to < 0)
        to = run.length();
    if (from < 0)
        from = 0;

    TextRunComponents components;
    int w = generateComponents(&components, *this, run);

    int curPos = 0;
    for (int i = 0; i < (int)components.size(); ++i) {
        const TextRunComponent& comp = components.at(i);
        int len = comp.textLength();
        int curEnd = curPos + len;
        if (curPos < to && from < curEnd && !comp.isSpace()) {
            FloatPoint pt = point;
            if (run.rtl())
                pt.setX(point.x() + w - comp.m_offset - comp.m_width);
            else
                pt.setX(point.x() + comp.m_offset);
            drawSimpleText(context, comp.m_textRun, pt, from - curPos, std::min(to, curEnd) - curPos);
        }
        curPos += len;
        if (from < curPos)
            from = curPos;
    }
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    TextRunComponents components;
    int w = generateComponents(&components, *this, run);
    return w;
}

int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat, bool includePartialGlyphs) const
{
    // FIXME: This truncation is not a problem for HTML, but only affects SVG, which passes floating-point numbers
    // to Font::offsetForPosition(). Bug http://webkit.org/b/40673 tracks fixing this problem.
    int position = static_cast<int>(xFloat);

    TextRunComponents components;
    int w = generateComponents(&components, *this, run);

    if (position >= w)
        return run.length();

    int offset = 0;
    if (run.rtl()) {
        for (size_t i = 0; i < components.size(); ++i) {
            const TextRunComponent& comp = components.at(i);
            int xe = w - comp.m_offset;
            int xs = xe - comp.m_width;
            if (position >= xs)
                return offset + (comp.isSpace()
                    ? static_cast<int>((position - xe) * comp.m_spaces / std::max(1.f, comp.m_width) + 0.5)
                    : offsetForPositionForSimpleText(comp.m_textRun, position - xs, includePartialGlyphs));

            offset += comp.textLength();
        }
    } else {
        for (size_t i = 0; i < components.size(); ++i) {
            const TextRunComponent& comp = components.at(i);
            int xs = comp.m_offset;
            int xe = xs + comp.m_width;
            if (position <= xe) {
                if (position - xs >= xe)
                    return offset + comp.textLength();
                return offset + (comp.isSpace()
                    ? static_cast<int>((position - xs) * comp.m_spaces / std::max(1.f, comp.m_width) + 0.5)
                    : offsetForPositionForSimpleText(comp.m_textRun, position - xs, includePartialGlyphs));
            }
            offset += comp.textLength();
        }
    }
    return run.length();
}


static float cursorToX(const Font* font, const TextRunComponents& components, int width, bool rtl, int cursor)
{
    int start = 0;
    for (size_t i = 0; i < components.size(); ++i) {
        const TextRunComponent& comp = components.at(i);
        if (start + comp.textLength() <= cursor) {
            start += comp.textLength();
            continue;
        }
        int xs = comp.m_offset;
        if (rtl)
            xs = width - xs - comp.m_width;

        int pos = cursor - start;
        if (comp.isSpace()) {
            if (rtl)
                pos = comp.textLength() - pos;
            return xs + pos * comp.m_width / comp.m_spaces;
        }
        WidthIterator it(font, comp.m_textRun);
        GlyphBuffer glyphBuffer;
        it.advance(pos, &glyphBuffer);
        return xs + it.m_runWidthSoFar;
    }
    return width;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& pt,
                                     int h, int from, int to) const
{
    TextRunComponents components;
    int w = generateComponents(&components, *this, run);

    if (!from && to == run.length())
        return FloatRect(pt.x(), pt.y(), w, h);

    float x1 = cursorToX(this, components, w, run.rtl(), from);
    float x2 = cursorToX(this, components, w, run.rtl(), to);
    if (x2 < x1)
        std::swap(x1, x2);

    return FloatRect(pt.x() + x1, pt.y(), x2 - x1, h);
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

} // namespace WebCore
