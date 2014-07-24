/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "HWndDC.h"
#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <mlang.h>
#include <unicode/uchar.h>
#include <unicode/unorm.h>
#include <winsock2.h>
#include <wtf/MathExtras.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

using namespace std;

void SimpleFontData::platformInit()
{
    m_syntheticBoldOffset = m_platformData.syntheticBold() ? 1.0f : 0.f;
    m_scriptCache = 0;
    m_scriptFontProperties = 0;
    m_isSystemFont = false;

    if (m_platformData.useGDI())
        return initGDIFont();

    CGFontRef font = m_platformData.cgFont();
    int iAscent = CGFontGetAscent(font);
    int iDescent = CGFontGetDescent(font);
    int iLineGap = CGFontGetLeading(font);
    unsigned unitsPerEm = CGFontGetUnitsPerEm(font);
    float pointSize = m_platformData.size();
    float fAscent = scaleEmToUnits(iAscent, unitsPerEm) * pointSize;
    float fDescent = -scaleEmToUnits(iDescent, unitsPerEm) * pointSize;
    float fLineGap = scaleEmToUnits(iLineGap, unitsPerEm) * pointSize;

    if (!isCustomFont()) {
        HWndDC dc(0);
        HGDIOBJ oldFont = SelectObject(dc, m_platformData.hfont());
        int faceLength = GetTextFace(dc, 0, 0);
        Vector<WCHAR> faceName(faceLength);
        GetTextFace(dc, faceLength, faceName.data());
        m_isSystemFont = !wcscmp(faceName.data(), L"Lucida Grande");
        SelectObject(dc, oldFont);

        fAscent = ascentConsideringMacAscentHack(faceName.data(), fAscent, fDescent);
    }

    m_fontMetrics.setAscent(fAscent);
    m_fontMetrics.setDescent(fDescent);
    m_fontMetrics.setLineGap(fLineGap);
    m_fontMetrics.setLineSpacing(lroundf(fAscent) + lroundf(fDescent) + lroundf(fLineGap));

    GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();
    Glyph xGlyph = glyphPageZero ? glyphPageZero->glyphDataForCharacter('x').glyph : 0;
    if (xGlyph) {
        // Measure the actual character "x", since it's possible for it to extend below the baseline, and we need the
        // reported x-height to only include the portion of the glyph that is above the baseline.
        CGRect xBox;
        CGFontGetGlyphBBoxes(font, &xGlyph, 1, &xBox);
        m_fontMetrics.setXHeight(scaleEmToUnits(CGRectGetMaxY(xBox), unitsPerEm) * pointSize);
    } else {
        int iXHeight = CGFontGetXHeight(font);
        m_fontMetrics.setXHeight(scaleEmToUnits(iXHeight, unitsPerEm) * pointSize);
    }

    m_fontMetrics.setUnitsPerEm(unitsPerEm);
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    if (!platformData().size())
        return FloatRect();

    if (m_platformData.useGDI())
        return boundsForGDIGlyph(glyph);

    CGRect box;
    CGFontGetGlyphBBoxes(m_platformData.cgFont(), &glyph, 1, &box);
    float pointSize = m_platformData.size();
    CGFloat scale = pointSize / fontMetrics().unitsPerEm();
    FloatRect boundingBox = CGRectApplyAffineTransform(box, CGAffineTransformMakeScale(scale, -scale));
    if (m_syntheticBoldOffset)
        boundingBox.setWidth(boundingBox.width() + m_syntheticBoldOffset);

    return boundingBox;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (!platformData().size())
        return 0;

    if (m_platformData.useGDI())
        return widthForGDIGlyph(glyph);

    CGFontRef font = m_platformData.cgFont();
    float pointSize = m_platformData.size();
    CGSize advance;
    CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
 
    // FIXME: Need to add real support for printer fonts.
    bool isPrinterFont = false;
    wkGetGlyphAdvances(font, m, m_isSystemFont, isPrinterFont, glyph, advance);

    return advance.width + m_syntheticBoldOffset;
}

}
