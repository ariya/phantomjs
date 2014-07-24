/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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

#include "config.h"
#include "SimpleFontData.h"

#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include <mlang.h>
#include <wtf/MathExtras.h>

namespace WebCore {

extern HDC g_screenDC;

void SimpleFontData::platformInit()
{
    if (!m_platformData.isValid())
        return;

    const TEXTMETRIC& tm = m_platformData.metrics();
    m_isSystemFont = m_platformData.isSystemFont();

    float ascent = (tm.tmAscent * m_platformData.size() + 36) / 72.0f;
    float descent = (tm.tmDescent * m_platformData.size() + 36) / 72.0f;
    float lineGap = (tm.tmExternalLeading * m_platformData.size() + 36) / 72.0f;
    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));
    m_fontMetrics.setXHeight(ascent * 0.56f);
}

void SimpleFontData::platformDestroy()
{
}

PassRefPtr<SimpleFontData> SimpleFontData::platformCreateScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    FontDescription fontDesc(fontDescription);
    fontDesc.setComputedSize(lroundf(scaleFactor * fontDesc.computedSize()));
    fontDesc.setSpecifiedSize(lroundf(scaleFactor * fontDesc.specifiedSize()));
    fontDesc.setKeywordSize(lroundf(scaleFactor * fontDesc.keywordSize()));
    FontPlatformData* result = fontCache()->getCachedFontPlatformData(fontDesc, m_platformData.family());
    if (!result)
        return 0;
    return SimpleFontData::create(*result);
}

DWORD getKnownFontCodePages(const wchar_t* family);

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    if (m_platformData.isDisabled())
        return true;

    // FIXME: Microsoft documentation seems to imply that characters can be output using a given font and DC
    // merely by testing code page intersection.  This seems suspect though.  Can't a font only partially
    // cover a given code page?

    // FIXME: in the case that we failed to get the interface, still use the font.
    IMLangFontLinkType* langFontLink = fontCache()->getFontLinkInterface();
    if (!langFontLink)
        return true;

    DWORD fontCodePages = m_platformData.codePages();
    if (!fontCodePages)
        return false;

    DWORD acpCodePages = 0;
    langFontLink->CodePageToCodePages(CP_ACP, &acpCodePages);

    DWORD actualCodePages;
    long numCharactersProcessed;
    while (length) {
        langFontLink->GetStrCodePages(characters, length, acpCodePages, &actualCodePages, &numCharactersProcessed);
        if (actualCodePages && !(actualCodePages & fontCodePages))
            return false;

        length -= numCharactersProcessed;
        characters += numCharactersProcessed;
    }

    return true;
}

void SimpleFontData::determinePitch()
{
    if (!m_platformData.isValid())
        return;

    const TEXTMETRIC& tm = m_platformData.metrics();

    // Yes, this looks backwards, but the fixed pitch bit is actually set if the font
    // is *not* fixed pitch.  Unbelievable but true.
    m_treatAsFixedPitch = !(tm.tmPitchAndFamily & TMPF_FIXED_PITCH);
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph) const
{
    return FloatRect();
}
    
float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (m_platformData.isDisabled())
        return 0;

    HGDIOBJ hOldFont = SelectObject(g_screenDC, m_platformData.hfont());

    SIZE fontSize;
    wchar_t c = glyph;
    GetTextExtentPoint32(g_screenDC, &c, 1, &fontSize);

    SelectObject(g_screenDC, hOldFont);

    return (float)fontSize.cx * (float)m_platformData.size() / 72.f;
}


void SimpleFontData::platformCharWidthInit()
{
    if (!m_platformData.isValid())
        return;

    const TEXTMETRIC& tm = m_platformData.metrics();
    m_avgCharWidth = (tm.tmAveCharWidth * m_platformData.size() + 36) / 72;
    m_maxCharWidth = (tm.tmMaxCharWidth * m_platformData.size() + 36) / 72;
}

} // namespace WebCore
