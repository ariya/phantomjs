/*
* Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "FontCache.h"

#include "Font.h"
#include "FontData.h"
#include "SimpleFontData.h"
#include "UnicodeRange.h"
#include "wtf/OwnPtr.h"

#include <windows.h>
#include <mlang.h>

namespace WebCore {

extern HDC g_screenDC;

static IMultiLanguage *multiLanguage = 0;
static IMLangFontLinkType* langFontLink = 0;

IMultiLanguage* FontCache::getMultiLanguageInterface()
{
    if (!multiLanguage)
        CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&multiLanguage);

    return multiLanguage;
}

IMLangFontLinkType* FontCache::getFontLinkInterface()
{
    if (!langFontLink) {
        if (IMultiLanguage* mli = getMultiLanguageInterface())
            mli->QueryInterface(&langFontLink);
    }

    return langFontLink;
}

static bool currentFontContainsCharacter(IMLangFontLinkType* langFontLink, HDC hdc, HFONT hfont, UChar character, const wchar_t* faceName)
{
#if USE(IMLANG_FONT_LINK2)
    UINT unicodeRanges;
    if (S_OK != langFontLink->GetFontUnicodeRanges(hdc, &unicodeRanges, 0))
        return false;

    static Vector<UNICODERANGE, 64> glyphsetBuffer;
    glyphsetBuffer.resize(unicodeRanges);

    if (S_OK != langFontLink->GetFontUnicodeRanges(hdc, &unicodeRanges, glyphsetBuffer.data()))
        return false;

    // FIXME: Change this to a binary search. (Yong Li: That's easy. But, is it guaranteed that the ranges are sorted?)
    for (Vector<UNICODERANGE, 64>::const_iterator i = glyphsetBuffer.begin(); i != glyphsetBuffer.end(); ++i) {
        if (i->wcTo >= character)
            return i->wcFrom <= character;
    }
#else
    DWORD fontCodePages = 0, charCodePages = 0;
    HRESULT result = langFontLink->GetFontCodePages(hdc, hfont, &fontCodePages);
    if (result != S_OK)
        return false;
    result = langFontLink->GetCharCodePages(character, &charCodePages);
    if (result != S_OK)
        return false;

    fontCodePages |= FontPlatformData::getKnownFontCodePages(faceName);
    if (fontCodePages & charCodePages)
        return true;
#endif

    return false;
}

static HFONT createMLangFont(IMLangFontLinkType* langFontLink, HDC hdc, const FontPlatformData& refFont, DWORD codePageMask, UChar character = 0)
{
    HFONT mlangFont;

#if USE(IMLANG_FONT_LINK2)
    HRESULT result = langFontLink->MapFont(hdc, codePageMask, character, &mlangFont);
#else
    HRESULT result = langFontLink->MapFont(hdc, codePageMask, refFont.hfont(), &mlangFont);
#endif

    if (SUCCEEDED(result))
        return mlangFont;
    return 0;
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
        IMLangFontLinkType* langFontLink = fontCache()->getFontLinkInterface();
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
    LONG weight = FontPlatformData::adjustedGDIFontWeight(logFont->lfWeight, procData->m_familyName);
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

void FontCache::platformInit()
{
}

void FontCache::comInitialize()
{
}

void FontCache::comUninitialize()
{
    if (langFontLink) {
        langFontLink->Release();
        langFontLink = 0;
    }
    if (multiLanguage) {
        multiLanguage->Release();
        multiLanguage = 0;
    }
}

PassRefPtr<SimpleFontData> FontCache::systemFallbackForCharacters(const FontDescription& description, const SimpleFontData* originalFontData, bool, const UChar* characters, int length)
{
    String familyName;
    WCHAR name[LF_FACESIZE];

    UChar character = characters[0];
    const FontPlatformData& origFont = originalFontData->platformData();
    unsigned unicodeRange = findCharUnicodeRange(character);

    if (IMLangFontLinkType* langFontLink = getFontLinkInterface()) {
        HGDIOBJ oldFont = GetCurrentObject(g_screenDC, OBJ_FONT);
        HFONT hfont = 0;
        DWORD codePages = 0;
        UINT codePage = 0;
        // Try MLang font linking first.
        langFontLink->GetCharCodePages(character, &codePages);
        if (codePages && unicodeRange == cRangeSetCJK) {
            // The CJK character may belong to multiple code pages. We want to
            // do font linking against a single one of them, preferring the default
            // code page for the user's locale.
            const Vector<DWORD, 4>& CJKCodePageMasks = getCJKCodePageMasks();
            unsigned numCodePages = CJKCodePageMasks.size();
            for (unsigned i = 0; i < numCodePages; ++i) {
                hfont = createMLangFont(langFontLink, g_screenDC, origFont, CJKCodePageMasks[i]);
                if (!hfont)
                    continue;

                SelectObject(g_screenDC, hfont);
                GetTextFace(g_screenDC, LF_FACESIZE, name);

                if (hfont && !(codePages & CJKCodePageMasks[i])) {
                    // We asked about a code page that is not one of the code pages
                    // returned by MLang, so the font might not contain the character.
                    if (!currentFontContainsCharacter(langFontLink, g_screenDC, hfont, character, name))
                    {
                        SelectObject(g_screenDC, oldFont);
                        langFontLink->ReleaseFont(hfont);
                        hfont = 0;
                        continue;
                    }
                }
                break;
            }
        } else {
            hfont = createMLangFont(langFontLink, g_screenDC, origFont, codePages, character);
            SelectObject(g_screenDC, hfont);
            GetTextFace(g_screenDC, LF_FACESIZE, name);
        }
        SelectObject(g_screenDC, oldFont);

        if (hfont) {
            familyName = name;
            langFontLink->ReleaseFont(hfont);
        } else
            FontPlatformData::mapKnownFont(codePages, familyName);
    }

    if (familyName.isEmpty())
        familyName = FontPlatformData::defaultFontFamily();

    if (!familyName.isEmpty()) {
        // FIXME: temporary workaround for Thai font problem
        FontDescription fontDescription(description);
        if (unicodeRange == cRangeThai && fontDescription.weight() > FontWeightNormal)
            fontDescription.setWeight(FontWeightNormal);

        FontPlatformData* result = getCachedFontPlatformData(fontDescription, familyName);
        if (result && result->hash() != origFont.hash()) {
            if (RefPtr<SimpleFontData> fontData = getCachedFontData(result, DoNotRetain))
                return fontData.release();
        }
    }

    return 0;
}

PassRefPtr<SimpleFontData> FontCache::getLastResortFallbackFont(const FontDescription& fontDesc, ShouldRetain shouldRetain)
{
    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.
    return getCachedFontData(fontDesc, FontPlatformData::defaultFontFamily(), false, shouldRetain);
}

PassOwnPtr<FontPlatformData> FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    return adoptPtr(new FontPlatformData(fontDescription, family));
}

void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
    LOGFONT logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    unsigned familyLength = std::min(familyName.length(), static_cast<unsigned>(LF_FACESIZE - 1));
    memcpy(logFont.lfFaceName, familyName.characters(), familyLength * sizeof(UChar));
    logFont.lfFaceName[familyLength] = 0;
    logFont.lfPitchAndFamily = 0;

    TraitsInFamilyProcData procData(familyName);
    EnumFontFamiliesEx(g_screenDC, &logFont, traitsInFamilyEnumProc, reinterpret_cast<LPARAM>(&procData), 0);
    copyToVector(procData.m_traitsMasks, traitsMasks);
}

} // namespace WebCore
