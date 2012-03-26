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
#include <winsock2.h>
#include "FontCache.h"
#include "Font.h"
#include "SimpleFontData.h"
#include "UnicodeRange.h"
#include <mlang.h>
#include <windows.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>
#if USE(CG)
#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

using std::min;

namespace WebCore
{

void FontCache::platformInit()
{
#if USE(CG)
    wkSetUpFontCache(1536 * 1024 * 4); // This size matches Mac.
#endif
}

IMLangFontLink2* FontCache::getFontLinkInterface()
{
    static IMultiLanguage *multiLanguage;
    if (!multiLanguage) {
        if (CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL, IID_IMultiLanguage, (void**)&multiLanguage) != S_OK)
            return 0;
    }

    static IMLangFontLink2* langFontLink;
    if (!langFontLink) {
        if (multiLanguage->QueryInterface(&langFontLink) != S_OK)
            return 0;
    }

    return langFontLink;
}

static int CALLBACK metaFileEnumProc(HDC hdc, HANDLETABLE* table, CONST ENHMETARECORD* record, int tableEntries, LPARAM logFont)
{
    if (record->iType == EMR_EXTCREATEFONTINDIRECTW) {
        const EMREXTCREATEFONTINDIRECTW* createFontRecord = reinterpret_cast<const EMREXTCREATEFONTINDIRECTW*>(record);
        *reinterpret_cast<LOGFONT*>(logFont) = createFontRecord->elfw.elfLogFont;
    }
    return true;
}

static int CALLBACK linkedFontEnumProc(CONST LOGFONT* logFont, CONST TEXTMETRIC* metrics, DWORD fontType, LPARAM hfont)
{
    *reinterpret_cast<HFONT*>(hfont) = CreateFontIndirect(logFont);
    return false;
}

static const Vector<String>* getLinkedFonts(String& family)
{
    static HashMap<String, Vector<String>*> systemLinkMap;
    Vector<String>* result = systemLinkMap.get(family);
    if (result)
        return result;

    result = new Vector<String>;
    systemLinkMap.set(family, result);
    HKEY fontLinkKey;
    if (FAILED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", 0, KEY_READ, &fontLinkKey)))
        return result;

    DWORD linkedFontsBufferSize = 0;
    RegQueryValueEx(fontLinkKey, family.charactersWithNullTermination(), 0, NULL, NULL, &linkedFontsBufferSize);
    WCHAR* linkedFonts = reinterpret_cast<WCHAR*>(malloc(linkedFontsBufferSize));
    if (SUCCEEDED(RegQueryValueEx(fontLinkKey, family.charactersWithNullTermination(), 0, NULL, reinterpret_cast<BYTE*>(linkedFonts), &linkedFontsBufferSize))) {
        unsigned i = 0;
        unsigned length = linkedFontsBufferSize / sizeof(*linkedFonts);
        while (i < length) {
            while (i < length && linkedFonts[i] != ',')
                i++;
            i++;
            unsigned j = i;
            while (j < length && linkedFonts[j])
                j++;
            result->append(String(linkedFonts + i, j - i));
            i = j + 1;
        }
    }
    free(linkedFonts);
    RegCloseKey(fontLinkKey);
    return result;
}

static const Vector<DWORD, 4>& getCJKCodePageMasks()
{
    // The default order in which we look for a font for a CJK character. If the user's default code page is
    // one of these, we will use it first.
    static const UINT CJKCodePages[] = { 
        932, /* Japanese */
        936, /* Simplified Chinese */
        950, /* Traditional Chinese */
        949  /* Korean */
    };

    static Vector<DWORD, 4> codePageMasks;
    static bool initialized;
    if (!initialized) {
        initialized = true;
        IMLangFontLink2* langFontLink = fontCache()->getFontLinkInterface();
        if (!langFontLink)
            return codePageMasks;

        UINT defaultCodePage;
        DWORD defaultCodePageMask = 0;
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE, reinterpret_cast<LPWSTR>(&defaultCodePage), sizeof(defaultCodePage)))
            langFontLink->CodePageToCodePages(defaultCodePage, &defaultCodePageMask);

        if (defaultCodePage == CJKCodePages[0] || defaultCodePage == CJKCodePages[1] || defaultCodePage == CJKCodePages[2] || defaultCodePage == CJKCodePages[3])
            codePageMasks.append(defaultCodePageMask);
        for (unsigned i = 0; i < 4; ++i) {
            if (defaultCodePage != CJKCodePages[i]) {
                DWORD codePageMask;
                langFontLink->CodePageToCodePages(CJKCodePages[i], &codePageMask);
                codePageMasks.append(codePageMask);
            }
        }
    }
    return codePageMasks;
}

static bool currentFontContainsCharacter(HDC hdc, UChar character)
{
    static Vector<char, 512> glyphsetBuffer;
    glyphsetBuffer.resize(GetFontUnicodeRanges(hdc, 0));
    GLYPHSET* glyphset = reinterpret_cast<GLYPHSET*>(glyphsetBuffer.data());
    GetFontUnicodeRanges(hdc, glyphset);

    // FIXME: Change this to a binary search.
    unsigned i = 0;
    while (i < glyphset->cRanges && glyphset->ranges[i].wcLow <= character)
        i++;

    return i && glyphset->ranges[i - 1].wcLow + glyphset->ranges[i - 1].cGlyphs > character;
}

static HFONT createMLangFont(IMLangFontLink2* langFontLink, HDC hdc, DWORD codePageMask, UChar character = 0)
{
    HFONT MLangFont;
    HFONT hfont = 0;
    if (SUCCEEDED(langFontLink->MapFont(hdc, codePageMask, character, &MLangFont)) && MLangFont) {
        LOGFONT lf;
        GetObject(MLangFont, sizeof(LOGFONT), &lf);
        langFontLink->ReleaseFont(MLangFont);
        hfont = CreateFontIndirect(&lf);
    }
    return hfont;
}

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
    UChar character = characters[0];
    SimpleFontData* fontData = 0;
    HDC hdc = GetDC(0);
    HFONT primaryFont = font.primaryFont()->fontDataForCharacter(character)->platformData().hfont();
    HGDIOBJ oldFont = SelectObject(hdc, primaryFont);
    HFONT hfont = 0;

    if (IMLangFontLink2* langFontLink = getFontLinkInterface()) {
        // Try MLang font linking first.
        DWORD codePages = 0;
        langFontLink->GetCharCodePages(character, &codePages);

        if (codePages && findCharUnicodeRange(character) == cRangeSetCJK) {
            // The CJK character may belong to multiple code pages. We want to
            // do font linking against a single one of them, preferring the default
            // code page for the user's locale.
            const Vector<DWORD, 4>& CJKCodePageMasks = getCJKCodePageMasks();
            unsigned numCodePages = CJKCodePageMasks.size();
            for (unsigned i = 0; i < numCodePages && !hfont; ++i) {
                hfont = createMLangFont(langFontLink, hdc, CJKCodePageMasks[i]);
                if (hfont && !(codePages & CJKCodePageMasks[i])) {
                    // We asked about a code page that is not one of the code pages
                    // returned by MLang, so the font might not contain the character.
                    SelectObject(hdc, hfont);
                    if (!currentFontContainsCharacter(hdc, character)) {
                        DeleteObject(hfont);
                        hfont = 0;
                    }
                    SelectObject(hdc, primaryFont);
                }
            }
        } else
            hfont = createMLangFont(langFontLink, hdc, codePages, character);
    }

    // A font returned from MLang is trusted to contain the character.
    bool containsCharacter = hfont;

    if (!hfont) {
        // To find out what font Uniscribe would use, we make it draw into a metafile and intercept
        // calls to CreateFontIndirect().
        HDC metaFileDc = CreateEnhMetaFile(hdc, NULL, NULL, NULL);
        SelectObject(metaFileDc, primaryFont);

        bool scriptStringOutSucceeded = false;
        SCRIPT_STRING_ANALYSIS ssa;

        // FIXME: If length is greater than 1, we actually return the font for the last character.
        // This function should be renamed getFontDataForCharacter and take a single 32-bit character.
        if (SUCCEEDED(ScriptStringAnalyse(metaFileDc, characters, length, 0, -1, SSA_METAFILE | SSA_FALLBACK | SSA_GLYPHS | SSA_LINK,
            0, NULL, NULL, NULL, NULL, NULL, &ssa))) {
            scriptStringOutSucceeded = SUCCEEDED(ScriptStringOut(ssa, 0, 0, 0, NULL, 0, 0, FALSE));
            ScriptStringFree(&ssa);
        }
        HENHMETAFILE metaFile = CloseEnhMetaFile(metaFileDc);
        if (scriptStringOutSucceeded) {
            LOGFONT logFont;
            logFont.lfFaceName[0] = 0;
            EnumEnhMetaFile(0, metaFile, metaFileEnumProc, &logFont, NULL);
            if (logFont.lfFaceName[0])
                hfont = CreateFontIndirect(&logFont);
        }
        DeleteEnhMetaFile(metaFile);
    }

    String familyName;
    const Vector<String>* linkedFonts = 0;
    unsigned linkedFontIndex = 0;
    while (hfont) {
        SelectObject(hdc, hfont);
        WCHAR name[LF_FACESIZE];
        GetTextFace(hdc, LF_FACESIZE, name);
        familyName = name;

        if (containsCharacter || currentFontContainsCharacter(hdc, character))
            break;

        if (!linkedFonts)
            linkedFonts = getLinkedFonts(familyName);
        SelectObject(hdc, oldFont);
        DeleteObject(hfont);
        hfont = 0;

        if (linkedFonts->size() <= linkedFontIndex)
            break;

        LOGFONT logFont;
        logFont.lfCharSet = DEFAULT_CHARSET;
        memcpy(logFont.lfFaceName, linkedFonts->at(linkedFontIndex).characters(), linkedFonts->at(linkedFontIndex).length() * sizeof(WCHAR));
        logFont.lfFaceName[linkedFonts->at(linkedFontIndex).length()] = 0;
        EnumFontFamiliesEx(hdc, &logFont, linkedFontEnumProc, reinterpret_cast<LPARAM>(&hfont), 0);
        linkedFontIndex++;
    }

    if (hfont) {
        if (!familyName.isEmpty()) {
            FontPlatformData* result = getCachedFontPlatformData(font.fontDescription(), familyName);
            if (result)
                fontData = getCachedFontData(result);
        }

        SelectObject(hdc, oldFont);
        DeleteObject(hfont);
    }

    ReleaseDC(0, hdc);
    return fontData;
}

SimpleFontData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    return 0;
}

static SimpleFontData* fontDataFromDescriptionAndLogFont(FontCache* fontCache, const FontDescription& fontDescription, const LOGFONT& font, AtomicString& outFontFamilyName)
{
    AtomicString familyName = String(font.lfFaceName, wcsnlen(font.lfFaceName, LF_FACESIZE));
    SimpleFontData* fontData = fontCache->getCachedFontData(fontDescription, familyName);
    if (fontData)
        outFontFamilyName = familyName;
    return fontData;
}

SimpleFontData* FontCache::getLastResortFallbackFont(const FontDescription& fontDescription)
{
    DEFINE_STATIC_LOCAL(AtomicString, fallbackFontName, ());
    if (!fallbackFontName.isEmpty())
        return getCachedFontData(fontDescription, fallbackFontName);

    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.

    // Search all typical Windows-installed full Unicode fonts.
    // Sorted by most to least glyphs according to http://en.wikipedia.org/wiki/Unicode_typefaces
    // Start with Times New Roman also since it is the default if the user doesn't change prefs.
    static AtomicString fallbackFonts[] = {
        AtomicString("Times New Roman"),
        AtomicString("Microsoft Sans Serif"),
        AtomicString("Tahoma"),
        AtomicString("Lucida Sans Unicode"),
        AtomicString("Arial")
    };
    SimpleFontData* simpleFont;
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(fallbackFonts); ++i) {
        if (simpleFont = getCachedFontData(fontDescription, fallbackFonts[i])) {
            fallbackFontName = fallbackFonts[i];
            return simpleFont;
        }
    }

    // Fall back to the DEFAULT_GUI_FONT if no known Unicode fonts are available.
    if (HFONT defaultGUIFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))) {
        LOGFONT defaultGUILogFont;
        GetObject(defaultGUIFont, sizeof(defaultGUILogFont), &defaultGUILogFont);
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, defaultGUILogFont, fallbackFontName))
            return simpleFont;
    }

    // Fall back to Non-client metrics fonts.
    NONCLIENTMETRICS nonClientMetrics = {0};
    nonClientMetrics.cbSize = sizeof(nonClientMetrics);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(nonClientMetrics), &nonClientMetrics, 0)) {
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, nonClientMetrics.lfMessageFont, fallbackFontName))
            return simpleFont;
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, nonClientMetrics.lfMenuFont, fallbackFontName))
            return simpleFont;
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, nonClientMetrics.lfStatusFont, fallbackFontName))
            return simpleFont;
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, nonClientMetrics.lfCaptionFont, fallbackFontName))
            return simpleFont;
        if (simpleFont = fontDataFromDescriptionAndLogFont(this, fontDescription, nonClientMetrics.lfSmCaptionFont, fallbackFontName))
            return simpleFont;
    }
    
    ASSERT_NOT_REACHED();
    return 0;
}

static LONG toGDIFontWeight(FontWeight fontWeight)
{
    static LONG gdiFontWeights[] = {
        FW_THIN,        // FontWeight100
        FW_EXTRALIGHT,  // FontWeight200
        FW_LIGHT,       // FontWeight300
        FW_NORMAL,      // FontWeight400
        FW_MEDIUM,      // FontWeight500
        FW_SEMIBOLD,    // FontWeight600
        FW_BOLD,        // FontWeight700
        FW_EXTRABOLD,   // FontWeight800
        FW_HEAVY        // FontWeight900
    };
    return gdiFontWeights[fontWeight];
}

static inline bool isGDIFontWeightBold(LONG gdiFontWeight)
{
    return gdiFontWeight >= FW_SEMIBOLD;
}

static LONG adjustedGDIFontWeight(LONG gdiFontWeight, const String& family)
{
    static AtomicString lucidaStr("Lucida Grande");
    if (equalIgnoringCase(family, lucidaStr)) {
        if (gdiFontWeight == FW_NORMAL)
            return FW_MEDIUM;
        if (gdiFontWeight == FW_BOLD)
            return FW_SEMIBOLD;
    }
    return gdiFontWeight;
}

struct MatchImprovingProcData {
    MatchImprovingProcData(LONG desiredWeight, bool desiredItalic)
        : m_desiredWeight(desiredWeight)
        , m_desiredItalic(desiredItalic)
        , m_hasMatched(false)
    {
    }

    LONG m_desiredWeight;
    bool m_desiredItalic;
    bool m_hasMatched;
    LOGFONT m_chosen;
};

static int CALLBACK matchImprovingEnumProc(CONST LOGFONT* candidate, CONST TEXTMETRIC* metrics, DWORD fontType, LPARAM lParam)
{
    MatchImprovingProcData* matchData = reinterpret_cast<MatchImprovingProcData*>(lParam);

    if (!matchData->m_hasMatched) {
        matchData->m_hasMatched = true;
        matchData->m_chosen = *candidate;
        return 1;
    }

    if (!candidate->lfItalic != !matchData->m_chosen.lfItalic) {
        if (!candidate->lfItalic == !matchData->m_desiredItalic)
            matchData->m_chosen = *candidate;

        return 1;
    }

    unsigned chosenWeightDeltaMagnitude = abs(matchData->m_chosen.lfWeight - matchData->m_desiredWeight);
    unsigned candidateWeightDeltaMagnitude = abs(candidate->lfWeight - matchData->m_desiredWeight);

    // If both are the same distance from the desired weight, prefer the candidate if it is further from regular.
    if (chosenWeightDeltaMagnitude == candidateWeightDeltaMagnitude && abs(candidate->lfWeight - FW_NORMAL) > abs(matchData->m_chosen.lfWeight - FW_NORMAL)) {
        matchData->m_chosen = *candidate;
        return 1;
    }

    // Otherwise, prefer the one closer to the desired weight.
    if (candidateWeightDeltaMagnitude < chosenWeightDeltaMagnitude)
        matchData->m_chosen = *candidate;

    return 1;
}

static HFONT createGDIFont(const AtomicString& family, LONG desiredWeight, bool desiredItalic, int size, bool synthesizeItalic)
{
    HDC hdc = GetDC(0);

    LOGFONT logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    unsigned familyLength = min(family.length(), static_cast<unsigned>(LF_FACESIZE - 1));
    memcpy(logFont.lfFaceName, family.characters(), familyLength * sizeof(UChar));
    logFont.lfFaceName[familyLength] = 0;
    logFont.lfPitchAndFamily = 0;

    MatchImprovingProcData matchData(desiredWeight, desiredItalic);
    EnumFontFamiliesEx(hdc, &logFont, matchImprovingEnumProc, reinterpret_cast<LPARAM>(&matchData), 0);

    ReleaseDC(0, hdc);

    if (!matchData.m_hasMatched)
        return 0;

    matchData.m_chosen.lfHeight = -size;
    matchData.m_chosen.lfWidth = 0;
    matchData.m_chosen.lfEscapement = 0;
    matchData.m_chosen.lfOrientation = 0;
    matchData.m_chosen.lfUnderline = false;
    matchData.m_chosen.lfStrikeOut = false;
    matchData.m_chosen.lfCharSet = DEFAULT_CHARSET;
#if USE(CG) || USE(CAIRO)
    matchData.m_chosen.lfOutPrecision = OUT_TT_ONLY_PRECIS;
#else
    matchData.m_chosen.lfOutPrecision = OUT_TT_PRECIS;
#endif
    matchData.m_chosen.lfQuality = DEFAULT_QUALITY;
    matchData.m_chosen.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

   if (desiredItalic && !matchData.m_chosen.lfItalic && synthesizeItalic)
       matchData.m_chosen.lfItalic = 1;

    HFONT result = CreateFontIndirect(&matchData.m_chosen);
    if (!result)
        return 0;

    HDC dc = GetDC(0);
    SaveDC(dc);
    SelectObject(dc, result);
    WCHAR actualName[LF_FACESIZE];
    GetTextFace(dc, LF_FACESIZE, actualName);
    RestoreDC(dc, -1);
    ReleaseDC(0, dc);

    if (wcsicmp(matchData.m_chosen.lfFaceName, actualName)) {
        DeleteObject(result);
        result = 0;
    }

    return result;
}

struct TraitsInFamilyProcData {
    TraitsInFamilyProcData(const AtomicString& familyName)
        : m_familyName(familyName)
    {
    }

    const AtomicString& m_familyName;
    HashSet<unsigned> m_traitsMasks;
};

static int CALLBACK traitsInFamilyEnumProc(CONST LOGFONT* logFont, CONST TEXTMETRIC* metrics, DWORD fontType, LPARAM lParam)
{
    TraitsInFamilyProcData* procData = reinterpret_cast<TraitsInFamilyProcData*>(lParam);

    unsigned traitsMask = 0;
    traitsMask |= logFont->lfItalic ? FontStyleItalicMask : FontStyleNormalMask;
    traitsMask |= FontVariantNormalMask;
    LONG weight = adjustedGDIFontWeight(logFont->lfWeight, procData->m_familyName);
    traitsMask |= weight == FW_THIN ? FontWeight100Mask :
        weight == FW_EXTRALIGHT ? FontWeight200Mask :
        weight == FW_LIGHT ? FontWeight300Mask :
        weight == FW_NORMAL ? FontWeight400Mask :
        weight == FW_MEDIUM ? FontWeight500Mask :
        weight == FW_SEMIBOLD ? FontWeight600Mask :
        weight == FW_BOLD ? FontWeight700Mask :
        weight == FW_EXTRABOLD ? FontWeight800Mask :
                                 FontWeight900Mask;
    procData->m_traitsMasks.add(traitsMask);
    return 1;
}
void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
    HDC hdc = GetDC(0);

    LOGFONT logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    unsigned familyLength = min(familyName.length(), static_cast<unsigned>(LF_FACESIZE - 1));
    memcpy(logFont.lfFaceName, familyName.characters(), familyLength * sizeof(UChar));
    logFont.lfFaceName[familyLength] = 0;
    logFont.lfPitchAndFamily = 0;

    TraitsInFamilyProcData procData(familyName);
    EnumFontFamiliesEx(hdc, &logFont, traitsInFamilyEnumProc, reinterpret_cast<LPARAM>(&procData), 0);
    copyToVector(procData.m_traitsMasks, traitsMasks);

    ReleaseDC(0, hdc);
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    bool isLucidaGrande = false;
    static AtomicString lucidaStr("Lucida Grande");
    if (equalIgnoringCase(family, lucidaStr))
        isLucidaGrande = true;

    bool useGDI = fontDescription.renderingMode() == AlternateRenderingMode && !isLucidaGrande;

    // The logical size constant is 32. We do this for subpixel precision when rendering using Uniscribe.
    // This masks rounding errors related to the HFONT metrics being  different from the CGFont metrics.
    // FIXME: We will eventually want subpixel precision for GDI mode, but the scaled rendering doesn't
    // look as nice. That may be solvable though.
    LONG weight = adjustedGDIFontWeight(toGDIFontWeight(fontDescription.weight()), family);
    HFONT hfont = createGDIFont(family, weight, fontDescription.italic(),
                                fontDescription.computedPixelSize() * (useGDI ? 1 : 32), useGDI);

    if (!hfont)
        return 0;

    if (isLucidaGrande)
        useGDI = false; // Never use GDI for Lucida Grande.

    LOGFONT logFont;
    GetObject(hfont, sizeof(LOGFONT), &logFont);

    bool synthesizeBold = isGDIFontWeightBold(weight) && !isGDIFontWeightBold(logFont.lfWeight);
    bool synthesizeItalic = fontDescription.italic() && !logFont.lfItalic;

    FontPlatformData* result = new FontPlatformData(hfont, fontDescription.computedPixelSize(), synthesizeBold, synthesizeItalic, useGDI);

#if USE(CG)
    bool fontCreationFailed = !result->cgFont();
#elif USE(CAIRO)
    bool fontCreationFailed = !result->scaledFont();
#endif

    if (fontCreationFailed) {
        // The creation of the CGFontRef failed for some reason.  We already asserted in debug builds, but to make
        // absolutely sure that we don't use this font, go ahead and return 0 so that we can fall back to the next
        // font.
        delete result;
        DeleteObject(hfont);
        return 0;
    }        

    return result;
}

}

