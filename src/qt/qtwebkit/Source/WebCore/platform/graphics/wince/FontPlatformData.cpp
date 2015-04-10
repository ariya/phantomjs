/*
 *  Copyright (C) 2007-2009 Torch Mobile Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "FontPlatformData.h"

#include "Font.h"
#include "FontCache.h"
#include "FontData.h"
#include "SimpleFontData.h"
#include "UnicodeRange.h"
#include "wtf/OwnPtr.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#include <windows.h>
#include <mlang.h>

namespace WebCore {

extern HDC g_screenDC;

static wchar_t songTiStr[] = { 0x5b8b, 0x4f53, 0 };
static wchar_t heiTiStr[] = { 0x9ed1, 0x4f53, 0 };

class FontFamilyCodePageInfo {
public:
    FontFamilyCodePageInfo()
        : m_codePage(0), m_codePages(0)
    {
    }
    FontFamilyCodePageInfo(const wchar_t* family, UINT codePage)
        : m_family(family), m_codePage(codePage), m_codePages(0)
    {
    }
    DWORD codePages() const
    {
        if (!m_codePages) {
            if (IMLangFontLinkType* langFontLink = fontCache()->getFontLinkInterface())
                langFontLink->CodePageToCodePages(m_codePage, &m_codePages);
        }
        return m_codePages;
    }

    String m_family;
    UINT m_codePage;
private:
    mutable DWORD m_codePages;
};

class FontFamilyChecker {
public:
    FontFamilyChecker(const wchar_t* family)
        : m_exists(false)
    {
        EnumFontFamilies(g_screenDC, family, enumFontFamProc, (LPARAM)this);
    }
    bool isSupported() const { return m_exists; }
private:
    bool m_exists;
    static int CALLBACK enumFontFamProc(const LOGFONT FAR* lpelf, const TEXTMETRIC FAR* lpntm, DWORD FontType, LPARAM lParam);
};

class ValidFontFamilyFinder {
public:
    ValidFontFamilyFinder()
    {
        EnumFontFamilies(g_screenDC, 0, enumFontFamProc, (LPARAM)this);
    }
    const String& family() const { return m_family; }
private:
    String m_family;
    static int CALLBACK enumFontFamProc(const LOGFONT FAR* lpelf, const TEXTMETRIC FAR* lpntm, DWORD FontType, LPARAM lParam);
};

class FixedSizeFontData: public RefCounted<FixedSizeFontData> {
public:
    LOGFONT m_font;
    OwnPtr<HFONT> m_hfont;
    TEXTMETRIC m_metrics;
    DWORD m_codePages;
    unsigned m_weight;
    bool m_italic;

    static PassRefPtr<FixedSizeFontData> create(const AtomicString& family, unsigned weight, bool italic);
private:
    FixedSizeFontData()
        : m_codePages(0)
        , m_weight(0)
        , m_italic(false)
    {
        memset(&m_font, 0, sizeof(m_font));
        memset(&m_metrics, 0, sizeof(m_metrics));
    }
};

struct FixedSizeFontDataKey {
    FixedSizeFontDataKey(const AtomicString& family = AtomicString(), unsigned weight = 0, bool italic = false)
        : m_family(family)
        , m_weight(weight)
        , m_italic(italic)
    {
    }

    FixedSizeFontDataKey(WTF::HashTableDeletedValueType) : m_weight(-2) { }
    bool isHashTableDeletedValue() const { return m_weight == -2; }

    bool operator==(const FixedSizeFontDataKey& other) const
    {
        return equalIgnoringCase(m_family, other.m_family)
            && m_weight == other.m_weight
            && m_italic == other.m_italic;
    }

    AtomicString m_family;
    unsigned m_weight;
    bool m_italic;
};

struct FixedSizeFontDataKeyHash {
    static unsigned hash(const FixedSizeFontDataKey& font)
    {
        unsigned hashCodes[] = {
            CaseFoldingHash::hash(font.m_family),
            font.m_weight,
            // static_cast<unsigned>(font.m_italic);
        };
        return StringHasher::hashMemory<sizeof(hashCodes)>(hashCodes);
    }

    static bool equal(const FixedSizeFontDataKey& a, const FixedSizeFontDataKey& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FixedSizeFontDataKeyTraits : WTF::GenericHashTraits<FixedSizeFontDataKey> {
    static const bool emptyValueIsZero = true;
    static const FixedSizeFontDataKey& emptyValue()
    {
        DEFINE_STATIC_LOCAL(FixedSizeFontDataKey, key, (nullAtom));
        return key;
    }
    static void constructDeletedValue(FixedSizeFontDataKey& slot)
    {
        new (&slot) FixedSizeFontDataKey(WTF::HashTableDeletedValue);
    }
    static bool isDeletedValue(const FixedSizeFontDataKey& value)
    {
        return value.isHashTableDeletedValue();
    }
};

int CALLBACK FontFamilyChecker::enumFontFamProc(const LOGFONT FAR* lpelf, const TEXTMETRIC FAR* lpntm, DWORD FontType, LPARAM lParam)
{
    ((FontFamilyChecker*)lParam)->m_exists = true;
    return 0;
}

int CALLBACK ValidFontFamilyFinder::enumFontFamProc(const LOGFONT FAR* lpelf, const TEXTMETRIC FAR* lpntm, DWORD FontType, LPARAM lParam)
{
    if (lpelf->lfCharSet != SYMBOL_CHARSET) {
        ((ValidFontFamilyFinder*)lParam)->m_family = String(lpelf->lfFaceName);
        return 0;
    }
    return 1;
}

typedef Vector<FontFamilyCodePageInfo> KnownFonts;
static KnownFonts& knownFonts()
{
    static KnownFonts fonts;
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        if (FontPlatformData::isSongTiSupported())
            fonts.append(FontFamilyCodePageInfo(songTiStr, 936));
    }
    return fonts;
}

static String getDefaultFontFamily()
{
    if (FontFamilyChecker(L"Tahoma").isSupported())
        return String(L"Tahoma");

    bool good = false;
    String family;
    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\GDI\\SysFnt", 0, 0, &key) == ERROR_SUCCESS) {
        DWORD maxlen, type;
        if (RegQueryValueEx(key, L"Nm", 0, &type, 0, &maxlen) == ERROR_SUCCESS && type == REG_SZ) {
            ++maxlen;
            if (wchar_t* buffer = new wchar_t[maxlen]) {
                if (RegQueryValueEx(key, L"Nm", 0, &type, (LPBYTE)buffer, &maxlen) == ERROR_SUCCESS) {
                    family = String(buffer, maxlen);
                    good = true;
                }
                delete[] buffer;
            }
        }
        RegCloseKey(key);
    }
    if (good)
        return family;

    return ValidFontFamilyFinder().family();
}

typedef HashMap<FixedSizeFontDataKey, RefPtr<FixedSizeFontData>, FixedSizeFontDataKeyHash, FixedSizeFontDataKeyTraits> FixedSizeFontCache;
FixedSizeFontCache g_fixedSizeFontCache;

PassRefPtr<FixedSizeFontData> FixedSizeFontData::create(const AtomicString& family, unsigned weight, bool italic)
{
    FixedSizeFontData* fontData = new FixedSizeFontData();

    fontData->m_weight = weight;
    fontData->m_italic = italic;

    LOGFONT& winFont = fontData->m_font;
    // The size here looks unusual.  The negative number is intentional.
    winFont.lfHeight = -72;
    winFont.lfWidth = 0;
    winFont.lfEscapement = 0;
    winFont.lfOrientation = 0;
    winFont.lfUnderline = false;
    winFont.lfStrikeOut = false;
    winFont.lfCharSet = DEFAULT_CHARSET;
    winFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    winFont.lfQuality = CLEARTYPE_QUALITY; //DEFAULT_QUALITY;
    winFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    winFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    winFont.lfItalic = italic;
    winFont.lfWeight = FontPlatformData::adjustedGDIFontWeight(weight, family);

    int len = std::min(family.length(), (unsigned int)LF_FACESIZE - 1);
    wmemcpy(winFont.lfFaceName, family.characters(), len);
    winFont.lfFaceName[len] = L'\0';

    fontData->m_hfont = adoptPtr(CreateFontIndirect(&winFont));

    HGDIOBJ oldFont = SelectObject(g_screenDC, fontData->m_hfont.get());

    GetTextMetrics(g_screenDC, &fontData->m_metrics);

    if (IMLangFontLinkType* langFontLink = fontCache()->getFontLinkInterface()) {
        langFontLink->GetFontCodePages(g_screenDC, fontData->m_hfont.get(), &fontData->m_codePages);
        fontData->m_codePages |= FontPlatformData::getKnownFontCodePages(winFont.lfFaceName);
    }

    SelectObject(g_screenDC, oldFont);

    return adoptRef(fontData);
}

static PassRefPtr<FixedSizeFontData> createFixedSizeFontData(const AtomicString& family, unsigned weight, bool italic)
{
    FixedSizeFontDataKey key(family, weight, italic);
    FixedSizeFontCache::AddResult result = g_fixedSizeFontCache.add(key, RefPtr<FixedSizeFontData>());
    if (result.isNewEntry)
        result.iterator->value = FixedSizeFontData::create(family, weight, italic);

    return result.iterator->value;
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

class FontPlatformPrivateData {
public:
    int m_reference;
    RefPtr<FixedSizeFontData> m_rootFontData;
    AtomicString m_family;
    FontDescription m_fontDescription;
    OwnPtr<HFONT> m_hfontScaled;
    int m_size;
    long m_fontScaledWidth;
    long m_fontScaledHeight;
    bool m_disabled;
    FontPlatformPrivateData(int size, unsigned weight)
        : m_reference(1)
        , m_family(FontPlatformData::defaultFontFamily())
        , m_size(size)
        , m_fontScaledWidth(0)
        , m_fontScaledHeight(0)
        , m_disabled(false)
    {
        m_rootFontData = createFixedSizeFontData(m_family, weight, false);
    }
    FontPlatformPrivateData(const FontDescription& fontDescription, const AtomicString& family)
        : m_reference(1)
        , m_size(fontDescription.computedPixelSize())
        , m_fontDescription(fontDescription)
        , m_family(family)
        , m_fontScaledWidth(0)
        , m_fontScaledHeight(0)
        , m_disabled(!fontDescription.specifiedSize())
    {
        m_rootFontData = FixedSizeFontData::create(family, toGDIFontWeight(fontDescription.weight()), fontDescription.italic());
    }
};

FontPlatformData::FontPlatformData(const FontDescription& fontDescription, const AtomicString& desiredFamily, bool useDefaultFontIfNotPresent)
{
    String family(desiredFamily);
    if (!equalIgnoringCase(family, defaultFontFamily()) && !FontFamilyChecker(family.charactersWithNullTermination().data()).isSupported()) {
        if (equalIgnoringCase(family, String(heiTiStr)) && isSongTiSupported())
            family = String(songTiStr);
        else if (useDefaultFontIfNotPresent)
            family = defaultFontFamily();
    }

    m_private = new FontPlatformPrivateData(fontDescription, family);
}

FontPlatformData::FontPlatformData(float size, bool bold, bool oblique)
{
    if (!size)
        m_private = 0;
    else
        m_private = new FontPlatformPrivateData((int)(size + 0.5), bold ? FW_BOLD : FW_NORMAL);
}

FontPlatformData::~FontPlatformData()
{
    if (isValid() && !--m_private->m_reference) {
        if (m_private->m_rootFontData->refCount() == 2) {
            FixedSizeFontDataKey key(m_private->m_family, m_private->m_rootFontData->m_weight, m_private->m_rootFontData->m_italic);
            g_fixedSizeFontCache.remove(key);
        }
        delete m_private;
    }
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& o)
{
    if (isValid() && !--m_private->m_reference)
        delete m_private;

    if (m_private = o.m_private)
        ++m_private->m_reference;

    return *this;
}

HFONT FontPlatformData::hfont() const
{
    if (!isValid())
        return 0;

    if (m_private->m_disabled)
        return 0;

    if (!m_private->m_rootFontData->m_hfont)
        m_private->m_rootFontData->m_hfont = adoptPtr(CreateFontIndirect(&m_private->m_rootFontData->m_font));

    return m_private->m_rootFontData->m_hfont.get();
}

HFONT FontPlatformData::getScaledFontHandle(int height, int width) const
{
    if (!isValid() || m_private->m_disabled)
        return 0;

    if (!m_private->m_hfontScaled || m_private->m_fontScaledHeight != height || m_private->m_fontScaledWidth != width) {
        m_private->m_fontScaledHeight = height;
        m_private->m_fontScaledWidth = width;
        LOGFONT font = m_private->m_rootFontData->m_font;
        font.lfHeight = -height;
        font.lfWidth = width;
        m_private->m_hfontScaled = adoptPtr(CreateFontIndirect(&font));
    }

    return m_private->m_hfontScaled.get();
}

bool FontPlatformData::discardFontHandle()
{
    if (!isValid())
        return false;

    if (m_private->m_rootFontData->m_hfont) {
        m_private->m_rootFontData->m_hfont = nullptr;
        return true;
    }

    if (m_private->m_hfontScaled) {
        m_private->m_hfontScaled = nullptr;
        return true;
    }
    return false;
}

const TEXTMETRIC& FontPlatformData::metrics() const
{
    return m_private->m_rootFontData->m_metrics;
}

bool FontPlatformData::isSystemFont() const
{
    return false;
}

int FontPlatformData::size() const
{
    return m_private->m_size;
}

const FontDescription& FontPlatformData::fontDescription() const
{
    return m_private->m_fontDescription;
}

const AtomicString& FontPlatformData::family() const
{
    return m_private->m_family;
}

const LOGFONT& FontPlatformData::logFont() const
{
    return m_private->m_rootFontData->m_font;
}

bool FontPlatformData::isDisabled() const
{
    return !isValid() || m_private->m_disabled;
}

DWORD FontPlatformData::codePages() const
{
    return m_private->m_rootFontData->m_codePages;
}

bool FontPlatformData::isSongTiSupported()
{
    static bool exists = FontFamilyChecker(songTiStr).isSupported();
    return exists;
}

bool FontPlatformData::mapKnownFont(DWORD codePages, String& family)
{
    KnownFonts& fonts = knownFonts();
    for (KnownFonts::iterator i = fonts.begin(); i != fonts.end(); ++i) {
        if (i->codePages() & codePages) {
            family = i->m_family;
            return true;
        }
    }
    return false;
}

DWORD FontPlatformData::getKnownFontCodePages(const wchar_t* family)
{
    KnownFonts& fonts = knownFonts();
    for (KnownFonts::iterator i = fonts.begin(); i != fonts.end(); ++i) {
        if (equalIgnoringCase(i->m_family, String(family)))
            return i->codePages();
    }
    return 0;
}

const String& FontPlatformData::defaultFontFamily()
{
    static String family(getDefaultFontFamily());
    return family;
}

LONG FontPlatformData::adjustedGDIFontWeight(LONG gdiFontWeight, const String& family)
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

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return String();
}
#endif

}
