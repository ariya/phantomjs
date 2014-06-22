/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
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

#ifndef Font_h
#define Font_h

#include "FontDescription.h"
#include "FontGlyphs.h"
#include "SimpleFontData.h"
#include "TextDirection.h"
#include "TypesettingFeatures.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MathExtras.h>
#include <wtf/unicode/CharacterNames.h>

#if PLATFORM(QT)
#include <QRawFont>
QT_BEGIN_NAMESPACE
class QTextLayout;
QT_END_NAMESPACE
#endif

// "X11/X.h" defines Complex to 0 and conflicts
// with Complex value in CodePath enum.
#ifdef Complex
#undef Complex
#endif

namespace WebCore {

class FloatPoint;
class FloatRect;
class FontData;
class FontMetrics;
class FontPlatformData;
class FontSelector;
class GlyphBuffer;
class GraphicsContext;
class RenderText;
class TextLayout;
class TextRun;

struct GlyphData;

struct GlyphOverflow {
    GlyphOverflow()
        : left(0)
        , right(0)
        , top(0)
        , bottom(0)
        , computeBounds(false)
    {
    }

    int left;
    int right;
    int top;
    int bottom;
    bool computeBounds;
};


class Font {
public:
    Font();
    Font(const FontDescription&, short letterSpacing, short wordSpacing);
    // This constructor is only used if the platform wants to start with a native font.
    Font(const FontPlatformData&, bool isPrinting, FontSmoothingMode = AutoSmoothing);
    ~Font();

    Font(const Font&);
    Font& operator=(const Font&);

    bool operator==(const Font& other) const;
    bool operator!=(const Font& other) const { return !(*this == other); }

    const FontDescription& fontDescription() const { return m_fontDescription; }

    int pixelSize() const { return fontDescription().computedPixelSize(); }
    float size() const { return fontDescription().computedSize(); }

    void update(PassRefPtr<FontSelector>) const;

    enum CustomFontNotReadyAction { DoNotPaintIfFontNotReady, UseFallbackIfFontNotReady };
    void drawText(GraphicsContext*, const TextRun&, const FloatPoint&, int from = 0, int to = -1, CustomFontNotReadyAction = DoNotPaintIfFontNotReady) const;
    void drawEmphasisMarks(GraphicsContext*, const TextRun&, const AtomicString& mark, const FloatPoint&, int from = 0, int to = -1) const;

    float width(const TextRun&, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    float width(const TextRun&, int& charsConsumed, String& glyphName) const;

    PassOwnPtr<TextLayout> createLayout(RenderText*, float xPos, bool collapseWhiteSpace) const;
    static void deleteLayout(TextLayout*);
    static float width(TextLayout&, unsigned from, unsigned len, HashSet<const SimpleFontData*>* fallbackFonts = 0);

    int offsetForPosition(const TextRun&, float position, bool includePartialGlyphs) const;
    FloatRect selectionRectForText(const TextRun&, const FloatPoint&, int h, int from = 0, int to = -1) const;

    bool isSmallCaps() const { return m_fontDescription.smallCaps(); }

    short wordSpacing() const { return m_wordSpacing; }
    short letterSpacing() const { return m_letterSpacing; }
    void setWordSpacing(short s) { m_wordSpacing = s; }
    void setLetterSpacing(short s) { m_letterSpacing = s; }
    bool isFixedPitch() const;
    bool isPrinterFont() const { return m_fontDescription.usePrinterFont(); }
    
    FontRenderingMode renderingMode() const { return m_fontDescription.renderingMode(); }

    TypesettingFeatures typesettingFeatures() const { return static_cast<TypesettingFeatures>(m_typesettingFeatures); }

    const AtomicString& firstFamily() const { return m_fontDescription.firstFamily(); }
    unsigned familyCount() const { return m_fontDescription.familyCount(); }
    const AtomicString& familyAt(unsigned i) const { return m_fontDescription.familyAt(i); }

    FontItalic italic() const { return m_fontDescription.italic(); }
    FontWeight weight() const { return m_fontDescription.weight(); }
    FontWidthVariant widthVariant() const { return m_fontDescription.widthVariant(); }

    bool isPlatformFont() const { return m_glyphs->isForPlatformFont(); }

    const FontMetrics& fontMetrics() const { return primaryFont()->fontMetrics(); }
    float spaceWidth() const { return primaryFont()->spaceWidth() + m_letterSpacing; }
    float tabWidth(const SimpleFontData&, unsigned tabSize, float position) const;
    float tabWidth(unsigned tabSize, float position) const { return tabWidth(*primaryFont(), tabSize, position); }

    int emphasisMarkAscent(const AtomicString&) const;
    int emphasisMarkDescent(const AtomicString&) const;
    int emphasisMarkHeight(const AtomicString&) const;

    const SimpleFontData* primaryFont() const;
    const FontData* fontDataAt(unsigned) const;
    GlyphData glyphDataForCharacter(UChar32 c, bool mirror, FontDataVariant variant = AutoVariant) const
    {
        return glyphDataAndPageForCharacter(c, mirror, variant).first;
    }
#if PLATFORM(MAC)
    const SimpleFontData* fontDataForCombiningCharacterSequence(const UChar*, size_t length, FontDataVariant) const;
#endif
    std::pair<GlyphData, GlyphPage*> glyphDataAndPageForCharacter(UChar32 c, bool mirror, FontDataVariant variant) const
    {
        return m_glyphs->glyphDataAndPageForCharacter(m_fontDescription, c, mirror, variant);
    }
    bool primaryFontHasGlyphForCharacter(UChar32) const;

    static bool isCJKIdeograph(UChar32);
    static bool isCJKIdeographOrSymbol(UChar32);

    static unsigned expansionOpportunityCount(const LChar*, size_t length, TextDirection, bool& isAfterExpansion);
    static unsigned expansionOpportunityCount(const UChar*, size_t length, TextDirection, bool& isAfterExpansion);

#if PLATFORM(QT)
    QRawFont rawFont() const;
    QFont syntheticFont() const;
#endif

    static void setShouldUseSmoothing(bool);
    static bool shouldUseSmoothing();

    enum CodePath { Auto, Simple, Complex, SimpleWithGlyphOverflow };
    CodePath codePath(const TextRun&) const;
    static CodePath characterRangeCodePath(const LChar*, unsigned) { return Simple; }
    static CodePath characterRangeCodePath(const UChar*, unsigned len);

private:
    enum ForTextEmphasisOrNot { NotForTextEmphasis, ForTextEmphasis };

    // Returns the initial in-stream advance.
    float getGlyphsAndAdvancesForSimpleText(const TextRun&, int from, int to, GlyphBuffer&, ForTextEmphasisOrNot = NotForTextEmphasis) const;
    void drawSimpleText(GraphicsContext*, const TextRun&, const FloatPoint&, int from, int to) const;
    void drawEmphasisMarksForSimpleText(GraphicsContext*, const TextRun&, const AtomicString& mark, const FloatPoint&, int from, int to) const;
    void drawGlyphs(GraphicsContext*, const SimpleFontData*, const GlyphBuffer&, int from, int to, const FloatPoint&) const;
    void drawGlyphBuffer(GraphicsContext*, const TextRun&, const GlyphBuffer&, const FloatPoint&) const;
    void drawEmphasisMarks(GraphicsContext*, const TextRun&, const GlyphBuffer&, const AtomicString&, const FloatPoint&) const;
    float floatWidthForSimpleText(const TextRun&, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    int offsetForPositionForSimpleText(const TextRun&, float position, bool includePartialGlyphs) const;
    FloatRect selectionRectForSimpleText(const TextRun&, const FloatPoint&, int h, int from, int to) const;

    bool getEmphasisMarkGlyphData(const AtomicString&, GlyphData&) const;

    static bool canReturnFallbackFontsForComplexText();
    static bool canExpandAroundIdeographsInComplexText();

    // Returns the initial in-stream advance.
    float getGlyphsAndAdvancesForComplexText(const TextRun&, int from, int to, GlyphBuffer&, ForTextEmphasisOrNot = NotForTextEmphasis) const;
    void drawComplexText(GraphicsContext*, const TextRun&, const FloatPoint&, int from, int to) const;
    void drawEmphasisMarksForComplexText(GraphicsContext*, const TextRun&, const AtomicString& mark, const FloatPoint&, int from, int to) const;
    float floatWidthForComplexText(const TextRun&, HashSet<const SimpleFontData*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    int offsetForPositionForComplexText(const TextRun&, float position, bool includePartialGlyphs) const;
    FloatRect selectionRectForComplexText(const TextRun&, const FloatPoint&, int h, int from, int to) const;

    friend struct WidthIterator;
    friend class SVGTextRunRenderingContext;

public:
    // Useful for debugging the different font rendering code paths.
    static void setCodePath(CodePath);
    static CodePath codePath();
    static CodePath s_codePath;

    static void setDefaultTypesettingFeatures(TypesettingFeatures);
    static TypesettingFeatures defaultTypesettingFeatures();

    static const uint8_t s_roundingHackCharacterTable[256];
    static bool isRoundingHackCharacter(UChar32 c)
    {
        return !(c & ~0xFF) && s_roundingHackCharacterTable[c];
    }

    FontSelector* fontSelector() const;
    static bool treatAsSpace(UChar c) { return c == ' ' || c == '\t' || c == '\n' || c == noBreakSpace; }
    static bool treatAsZeroWidthSpace(UChar c) { return treatAsZeroWidthSpaceInComplexScript(c) || c == 0x200c || c == 0x200d; }
    static bool treatAsZeroWidthSpaceInComplexScript(UChar c) { return c < 0x20 || (c >= 0x7F && c < 0xA0) || c == softHyphen || c == zeroWidthSpace || (c >= 0x200e && c <= 0x200f) || (c >= 0x202a && c <= 0x202e) || c == zeroWidthNoBreakSpace || c == objectReplacementCharacter; }
    static bool canReceiveTextEmphasis(UChar32 c);

    static inline UChar normalizeSpaces(UChar character)
    {
        if (treatAsSpace(character))
            return space;

        if (treatAsZeroWidthSpace(character))
            return zeroWidthSpace;

        return character;
    }

    static String normalizeSpaces(const LChar*, unsigned length);
    static String normalizeSpaces(const UChar*, unsigned length);

    bool needsTranscoding() const { return m_needsTranscoding; }
    FontGlyphs* glyphs() const { return m_glyphs.get(); }

private:
    bool loadingCustomFonts() const
    {
        return m_glyphs && m_glyphs->loadingCustomFonts();
    }

    TypesettingFeatures computeTypesettingFeatures() const
    {
        TextRenderingMode textRenderingMode = m_fontDescription.textRenderingMode();
        TypesettingFeatures features = s_defaultTypesettingFeatures;

        switch (textRenderingMode) {
        case AutoTextRendering:
            break;
        case OptimizeSpeed:
            features &= ~(Kerning | Ligatures);
            break;
        case GeometricPrecision:
        case OptimizeLegibility:
            features |= Kerning | Ligatures;
            break;
        }

        switch (m_fontDescription.kerning()) {
        case FontDescription::NoneKerning:
            features &= ~Kerning;
            break;
        case FontDescription::NormalKerning:
            features |= Kerning;
            break;
        case FontDescription::AutoKerning:
            break;
        }

        switch (m_fontDescription.commonLigaturesState()) {
        case FontDescription::DisabledLigaturesState:
            features &= ~Ligatures;
            break;
        case FontDescription::EnabledLigaturesState:
            features |= Ligatures;
            break;
        case FontDescription::NormalLigaturesState:
            break;
        }

        return features;
    }

#if PLATFORM(QT)
    void initFormatForTextLayout(QTextLayout*, const TextRun&) const;
#endif

    static TypesettingFeatures s_defaultTypesettingFeatures;

    FontDescription m_fontDescription;
    mutable RefPtr<FontGlyphs> m_glyphs;
    short m_letterSpacing;
    short m_wordSpacing;
    bool m_needsTranscoding;
    mutable unsigned m_typesettingFeatures : 2; // (TypesettingFeatures) Caches values computed from m_fontDescription.
};

void invalidateFontGlyphsCache();
void pruneUnreferencedEntriesFromFontGlyphsCache();

inline Font::~Font()
{
}

inline const SimpleFontData* Font::primaryFont() const
{
    ASSERT(m_glyphs);
    return m_glyphs->primarySimpleFontData(m_fontDescription);
}

inline const FontData* Font::fontDataAt(unsigned index) const
{
    ASSERT(m_glyphs);
    return m_glyphs->realizeFontDataAt(m_fontDescription, index);
}

inline bool Font::isFixedPitch() const
{
    ASSERT(m_glyphs);
    return m_glyphs->isFixedPitch(m_fontDescription);
}

inline FontSelector* Font::fontSelector() const
{
    return m_glyphs ? m_glyphs->fontSelector() : 0;
}

inline float Font::tabWidth(const SimpleFontData& fontData, unsigned tabSize, float position) const
{
    if (!tabSize)
        return letterSpacing();
    float tabWidth = tabSize * fontData.spaceWidth() + letterSpacing();
    return tabWidth - fmodf(position, tabWidth);
}

}

namespace WTF {

template <> void deleteOwnedPtr<WebCore::TextLayout>(WebCore::TextLayout*);

}

#endif
