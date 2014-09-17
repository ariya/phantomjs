/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2010, 2011 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Font.h"

#include "FloatRect.h"
#include "FontCache.h"
#include "FontTranscoder.h"
#if PLATFORM(QT) && HAVE(QRAWFONT)
#include "ContextShadow.h"
#include "GraphicsContext.h"
#endif
#include "IntPoint.h"
#include "GlyphBuffer.h"
#include "TextRun.h"
#include "WidthIterator.h"
#include <wtf/MathExtras.h>
#include <wtf/UnusedParam.h>

using namespace WTF;
using namespace Unicode;

namespace WebCore {

Font::CodePath Font::s_codePath = Auto;

// ============================================================================================
// Font Implementation (Cross-Platform Portion)
// ============================================================================================

Font::Font()
    : m_letterSpacing(0)
    , m_wordSpacing(0)
    , m_isPlatformFont(false)
    , m_needsTranscoding(false)
{
}

Font::Font(const FontDescription& fd, short letterSpacing, short wordSpacing) 
    : m_fontDescription(fd)
    , m_letterSpacing(letterSpacing)
    , m_wordSpacing(wordSpacing)
    , m_isPlatformFont(false)
    , m_needsTranscoding(fontTranscoder().needsTranscoding(fd))
{
}

Font::Font(const FontPlatformData& fontData, bool isPrinterFont, FontSmoothingMode fontSmoothingMode)
    : m_fontList(FontFallbackList::create())
    , m_letterSpacing(0)
    , m_wordSpacing(0)
    , m_isPlatformFont(true)
{
    m_fontDescription.setUsePrinterFont(isPrinterFont);
    m_fontDescription.setFontSmoothing(fontSmoothingMode);
    m_needsTranscoding = fontTranscoder().needsTranscoding(fontDescription());
    m_fontList->setPlatformFont(fontData);
}

Font::Font(const Font& other)
    : m_fontDescription(other.m_fontDescription)
    , m_fontList(other.m_fontList)
    , m_letterSpacing(other.m_letterSpacing)
    , m_wordSpacing(other.m_wordSpacing)
    , m_isPlatformFont(other.m_isPlatformFont)
    , m_needsTranscoding(fontTranscoder().needsTranscoding(other.m_fontDescription))
{
}

Font& Font::operator=(const Font& other)
{
    m_fontDescription = other.m_fontDescription;
    m_fontList = other.m_fontList;
    m_letterSpacing = other.m_letterSpacing;
    m_wordSpacing = other.m_wordSpacing;
    m_isPlatformFont = other.m_isPlatformFont;
    m_needsTranscoding = other.m_needsTranscoding;
    return *this;
}

bool Font::operator==(const Font& other) const
{
    // Our FontData don't have to be checked, since checking the font description will be fine.
    // FIXME: This does not work if the font was made with the FontPlatformData constructor.
    if (loadingCustomFonts() || other.loadingCustomFonts())
        return false;
    
    FontSelector* first = m_fontList ? m_fontList->fontSelector() : 0;
    FontSelector* second = other.m_fontList ? other.m_fontList->fontSelector() : 0;
    
    return first == second
           && m_fontDescription == other.m_fontDescription
           && m_letterSpacing == other.m_letterSpacing
           && m_wordSpacing == other.m_wordSpacing
           && (m_fontList ? m_fontList->generation() : 0) == (other.m_fontList ? other.m_fontList->generation() : 0);
}

void Font::update(PassRefPtr<FontSelector> fontSelector) const
{
    // FIXME: It is pretty crazy that we are willing to just poke into a RefPtr, but it ends up 
    // being reasonably safe (because inherited fonts in the render tree pick up the new
    // style anyway. Other copies are transient, e.g., the state in the GraphicsContext, and
    // won't stick around long enough to get you in trouble). Still, this is pretty disgusting,
    // and could eventually be rectified by using RefPtrs for Fonts themselves.
    if (!m_fontList)
        m_fontList = FontFallbackList::create();
    m_fontList->invalidate(fontSelector);
}

void Font::drawText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    // Don't draw anything while we are using custom fonts that are in the process of loading.
    if (loadingCustomFonts())
        return;
    
    to = (to == -1 ? run.length() : to);

#if ENABLE(SVG_FONTS)
    if (primaryFont()->isSVGFont()) {
        drawTextUsingSVGFont(context, run, point, from, to);
        return;
    }
#endif

    CodePath codePathToUse = codePath(run);

#if PLATFORM(QT) && HAVE(QRAWFONT)
    if (context->textDrawingMode() & TextModeStroke || context->contextShadow()->m_type != ContextShadow::NoShadow)
        codePathToUse = Complex;
#endif

    if (codePathToUse != Complex)
        return drawSimpleText(context, run, point, from, to);

    return drawComplexText(context, run, point, from, to);
}

void Font::drawEmphasisMarks(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    if (loadingCustomFonts())
        return;

    if (to < 0)
        to = run.length();

#if ENABLE(SVG_FONTS)
    // FIXME: Implement for SVG fonts.
    if (primaryFont()->isSVGFont())
        return;
#endif

    if (codePath(run) != Complex)
        drawEmphasisMarksForSimpleText(context, run, mark, point, from, to);
    else
        drawEmphasisMarksForComplexText(context, run, mark, point, from, to);
}

float Font::width(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
#if ENABLE(SVG_FONTS)
    if (primaryFont()->isSVGFont())
        return floatWidthUsingSVGFont(run);
#endif

    CodePath codePathToUse = codePath(run);
    if (codePathToUse != Complex) {
        // If the complex text implementation cannot return fallback fonts, avoid
        // returning them for simple text as well.
        static bool returnFallbackFonts = canReturnFallbackFontsForComplexText();
        return floatWidthForSimpleText(run, 0, returnFallbackFonts ? fallbackFonts : 0, codePathToUse == SimpleWithGlyphOverflow || (glyphOverflow && glyphOverflow->computeBounds) ? glyphOverflow : 0);
    }

    return floatWidthForComplexText(run, fallbackFonts, glyphOverflow);
}

float Font::width(const TextRun& run, int extraCharsAvailable, int& charsConsumed, String& glyphName) const
{
#if !ENABLE(SVG_FONTS)
    UNUSED_PARAM(extraCharsAvailable);
#else
    if (primaryFont()->isSVGFont())
        return floatWidthUsingSVGFont(run, extraCharsAvailable, charsConsumed, glyphName);
#endif

    charsConsumed = run.length();
    glyphName = "";

    if (codePath(run) != Complex)
        return floatWidthForSimpleText(run, 0);

    return floatWidthForComplexText(run);
}

FloatRect Font::selectionRectForText(const TextRun& run, const FloatPoint& point, int h, int from, int to) const
{
#if ENABLE(SVG_FONTS)
    if (primaryFont()->isSVGFont())
        return selectionRectForTextUsingSVGFont(run, point, h, from, to);
#endif

    to = (to == -1 ? run.length() : to);

    if (codePath(run) != Complex)
        return selectionRectForSimpleText(run, point, h, from, to);

    return selectionRectForComplexText(run, point, h, from, to);
}

int Font::offsetForPosition(const TextRun& run, float x, bool includePartialGlyphs) const
{
#if ENABLE(SVG_FONTS)
    if (primaryFont()->isSVGFont())
        return offsetForPositionForTextUsingSVGFont(run, x, includePartialGlyphs);
#endif

    if (codePath(run) != Complex)
        return offsetForPositionForSimpleText(run, x, includePartialGlyphs);

    return offsetForPositionForComplexText(run, x, includePartialGlyphs);
}

#if ENABLE(SVG_FONTS)
bool Font::isSVGFont() const
{ 
    return primaryFont()->isSVGFont(); 
}
#endif

String Font::normalizeSpaces(const UChar* characters, unsigned length)
{
    UChar* buffer;
    String normalized = String::createUninitialized(length, buffer);

    for (unsigned i = 0; i < length; ++i)
        buffer[i] = normalizeSpaces(characters[i]);

    return normalized;
}

static bool shouldUseFontSmoothing = true;

void Font::setShouldUseSmoothing(bool shouldUseSmoothing)
{
    ASSERT(isMainThread());
    shouldUseFontSmoothing = shouldUseSmoothing;
}

bool Font::shouldUseSmoothing()
{
    return shouldUseFontSmoothing;
}

void Font::setCodePath(CodePath p)
{
    s_codePath = p;
}

Font::CodePath Font::codePath()
{
    return s_codePath;
}

Font::CodePath Font::codePath(const TextRun& run) const
{
    if (s_codePath != Auto)
        return s_codePath;

#if PLATFORM(QT) && !HAVE(QRAWFONT)
    if (run.expansion() || run.rtl() || isSmallCaps() || wordSpacing() || letterSpacing())
        return Complex;
#endif

    CodePath result = Simple;

    // Start from 0 since drawing and highlighting also measure the characters before run->from
    // FIXME: Should use a UnicodeSet in ports where ICU is used. Note that we 
    // can't simply use UnicodeCharacter Property/class because some characters
    // are not 'combining', but still need to go to the complex path.
    // Alternatively, we may as well consider binary search over a sorted
    // list of ranges.
    for (int i = 0; i < run.length(); i++) {
        const UChar c = run[i];
        if (c < 0x2E5) // U+02E5 through U+02E9 (Modifier Letters : Tone letters)  
            continue;
        if (c <= 0x2E9) 
            return Complex;

        if (c < 0x300) // U+0300 through U+036F Combining diacritical marks
            continue;
        if (c <= 0x36F)
            return Complex;

        if (c < 0x0591 || c == 0x05BE) // U+0591 through U+05CF excluding U+05BE Hebrew combining marks, Hebrew punctuation Paseq, Sof Pasuq and Nun Hafukha
            continue;
        if (c <= 0x05CF)
            return Complex;

        // U+0600 through U+109F Arabic, Syriac, Thaana, NKo, Samaritan, Mandaic,
        // Devanagari, Bengali, Gurmukhi, Gujarati, Oriya, Tamil, Telugu, Kannada, 
        // Malayalam, Sinhala, Thai, Lao, Tibetan, Myanmar
        if (c < 0x0600) 
            continue;
        if (c <= 0x109F)
            return Complex;

        // U+1100 through U+11FF Hangul Jamo (only Ancient Korean should be left here if you precompose;
        // Modern Korean will be precomposed as a result of step A)
        if (c < 0x1100)
            continue;
        if (c <= 0x11FF)
            return Complex;

        if (c < 0x135D) // U+135D through U+135F Ethiopic combining marks
            continue;
        if (c <= 0x135F)
            return Complex;

        if (c < 0x1700) // U+1780 through U+18AF Tagalog, Hanunoo, Buhid, Taghanwa,Khmer, Mongolian
            continue;
        if (c <= 0x18AF)
            return Complex;

        if (c < 0x1900) // U+1900 through U+194F Limbu (Unicode 4.0)
            continue;
        if (c <= 0x194F)
            return Complex;

        if (c < 0x1980) // U+1980 through U+19DF New Tai Lue
            continue;
        if (c <= 0x19DF)
            return Complex;

        if (c < 0x1A00) // U+1A00 through U+1CFF Buginese, Tai Tham, Balinese, Batak, Lepcha, Vedic
            continue;
        if (c <= 0x1CFF)
            return Complex;

        if (c < 0x1DC0) // U+1DC0 through U+1DFF Comining diacritical mark supplement
            continue;
        if (c <= 0x1DFF)
            return Complex;

        // U+1E00 through U+2000 characters with diacritics and stacked diacritics
        if (c <= 0x2000) {
            result = SimpleWithGlyphOverflow;
            continue;
        }

        if (c < 0x20D0) // U+20D0 through U+20FF Combining marks for symbols
            continue;
        if (c <= 0x20FF)
            return Complex;

        if (c < 0x2CEF) // U+2CEF through U+2CF1 Combining marks for Coptic
            continue;
        if (c <= 0x2CF1)
            return Complex;

        if (c < 0x302A) // U+302A through U+302F Ideographic and Hangul Tone marks
            continue;
        if (c <= 0x302F)
            return Complex;

        if (c < 0xA67C) // U+A67C through U+A67D Combining marks for old Cyrillic
            continue;
        if (c <= 0xA67D)
            return Complex;

        if (c < 0xA6F0) // U+A6F0 through U+A6F1 Combining mark for Bamum
            continue;
        if (c <= 0xA6F1)
            return Complex;

       // U+A800 through U+ABFF Nagri, Phags-pa, Saurashtra, Devanagari Extended,
       // Hangul Jamo Ext. A, Javanese, Myanmar Extended A, Tai Viet, Meetei Mayek,
        if (c < 0xA800) 
            continue;
        if (c <= 0xABFF)
            return Complex;

        if (c < 0xD7B0) // U+D7B0 through U+D7FF Hangul Jamo Ext. B
            continue;
        if (c <= 0xD7FF)
            return Complex;

        if (c < 0xFE20) // U+FE20 through U+FE2F Combining half marks
            continue;
        if (c <= 0xFE2F)
            return Complex;

        // FIXME: Make this loop UTF-16-aware and check for Brahmi (U+11000 block)
        // Kaithi (U+11080 block) and other complex scripts in plane 1 or higher.
    }

    if (typesettingFeatures())
        return Complex;

    return result;
}

bool Font::isCJKIdeograph(UChar32 c)
{
    // The basic CJK Unified Ideographs block.
    if (c >= 0x4E00 && c <= 0x9FFF)
        return true;
    
    // CJK Unified Ideographs Extension A.
    if (c >= 0x3400 && c <= 0x4DBF)
        return true;
    
    // CJK Radicals Supplement.
    if (c >= 0x2E80 && c <= 0x2EFF)
        return true;
    
    // Kangxi Radicals.
    if (c >= 0x2F00 && c <= 0x2FDF)
        return true;
    
    // CJK Strokes.
    if (c >= 0x31C0 && c <= 0x31EF)
        return true;
    
    // CJK Compatibility Ideographs.
    if (c >= 0xF900 && c <= 0xFAFF)
        return true;
    
    // CJK Unified Ideographs Extension B.
    if (c >= 0x20000 && c <= 0x2A6DF)
        return true;
        
    // CJK Unified Ideographs Extension C.
    if (c >= 0x2A700 && c <= 0x2B73F)
        return true;
    
    // CJK Unified Ideographs Extension D.
    if (c >= 0x2B740 && c <= 0x2B81F)
        return true;
    
    // CJK Compatibility Ideographs Supplement.
    if (c >= 0x2F800 && c <= 0x2FA1F)
        return true;

    return false;
}

bool Font::isCJKIdeographOrSymbol(UChar32 c)
{
    // 0x2C7 Caron, Mandarin Chinese 3rd Tone
    // 0x2CA Modifier Letter Acute Accent, Mandarin Chinese 2nd Tone
    // 0x2CB Modifier Letter Grave Access, Mandarin Chinese 4th Tone 
    // 0x2D9 Dot Above, Mandarin Chinese 5th Tone 
    if ((c == 0x2C7) || (c == 0x2CA) || (c == 0x2CB) || (c == 0x2D9))
        return true;

    // Ideographic Description Characters.
    if (c >= 0x2FF0 && c <= 0x2FFF)
        return true;
    
    // CJK Symbols and Punctuation.
    if (c >= 0x3000 && c <= 0x303F)
        return true;
   
    // Hiragana 
    if (c >= 0x3040 && c <= 0x309F)
        return true;

    // Katakana 
    if (c >= 0x30A0 && c <= 0x30FF)
        return true;

    // Bopomofo
    if (c >= 0x3100 && c <= 0x312F)
        return true;
    
    // Bopomofo Extended
    if (c >= 0x31A0 && c <= 0x31BF)
        return true;
 
    // Enclosed CJK Letters and Months.
    if (c >= 0x3200 && c <= 0x32FF)
        return true;
    
    // CJK Compatibility.
    if (c >= 0x3300 && c <= 0x33FF)
        return true;
    
    // CJK Compatibility Forms.
    if (c >= 0xFE30 && c <= 0xFE4F)
        return true;

    // Halfwidth and Fullwidth Forms
    // Usually only used in CJK
    if (c >= 0xFF00 && c <= 0xFFEF)
        return true;

    // Emoji.
    if (c >= 0x1F200 && c <= 0x1F6F)
        return true;

    return isCJKIdeograph(c);
}

unsigned Font::expansionOpportunityCount(const UChar* characters, size_t length, TextDirection direction, bool& isAfterExpansion)
{
    static bool expandAroundIdeographs = canExpandAroundIdeographsInComplexText();
    unsigned count = 0;
    if (direction == LTR) {
        for (size_t i = 0; i < length; ++i) {
            UChar32 character = characters[i];
            if (treatAsSpace(character)) {
                count++;
                isAfterExpansion = true;
                continue;
            }
            if (U16_IS_LEAD(character) && i + 1 < length && U16_IS_TRAIL(characters[i + 1])) {
                character = U16_GET_SUPPLEMENTARY(character, characters[i + 1]);
                i++;
            }
            if (expandAroundIdeographs && isCJKIdeographOrSymbol(character)) {
                if (!isAfterExpansion)
                    count++;
                count++;
                isAfterExpansion = true;
                continue;
            }
            isAfterExpansion = false;
        }
    } else {
        for (size_t i = length; i > 0; --i) {
            UChar32 character = characters[i - 1];
            if (treatAsSpace(character)) {
                count++;
                isAfterExpansion = true;
                continue;
            }
            if (U16_IS_TRAIL(character) && i > 1 && U16_IS_LEAD(characters[i - 2])) {
                character = U16_GET_SUPPLEMENTARY(characters[i - 2], character);
                i--;
            }
            if (expandAroundIdeographs && isCJKIdeographOrSymbol(character)) {
                if (!isAfterExpansion)
                    count++;
                count++;
                isAfterExpansion = true;
                continue;
            }
            isAfterExpansion = false;
        }
    }
    return count;
}

bool Font::canReceiveTextEmphasis(UChar32 c)
{
    CharCategory category = Unicode::category(c);
    if (category & (Separator_Space | Separator_Line | Separator_Paragraph | Other_NotAssigned | Other_Control | Other_Format))
        return false;

    // Additional word-separator characters listed in CSS Text Level 3 Editor's Draft 3 November 2010.
    if (c == ethiopicWordspace || c == aegeanWordSeparatorLine || c == aegeanWordSeparatorDot
        || c == ugariticWordDivider || c == tibetanMarkIntersyllabicTsheg || c == tibetanMarkDelimiterTshegBstar)
        return false;

    return true;
}

}
