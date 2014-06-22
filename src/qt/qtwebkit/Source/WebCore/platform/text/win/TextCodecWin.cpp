/*
 * Copyright (C) 2007-2009 Torch Mobile, Inc. All rights reserved.
 * Copyright (C) 2010-2012 Patrick Gansterer <paroga@paroga.com>
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
 *  This library is distributed in the hope that i will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextCodecWin.h"

#include "COMPtr.h"
#include <mlang.h>
#include <windows.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

struct CharsetInfo {
    CString m_name;
    String m_friendlyName;
    UINT m_codePage;
    Vector<CString> m_aliases;
};

class LanguageManager {
private:
    LanguageManager();

    friend LanguageManager& languageManager();
};

// Usage: a lookup table used to get CharsetInfo with code page ID.
// Key: code page ID. Value: charset information.
static HashMap<UINT, CString>& codePageCharsets()
{
    static HashMap<UINT, CString> cc;
    return cc;
}

static HashMap<String, CharsetInfo>& knownCharsets()
{
    static HashMap<String, CharsetInfo> kc;
    return kc;
}

// Usage: a map that stores charsets that are supported by system. Sorted by name.
// Key: charset. Value: code page ID.
typedef HashSet<String> CharsetSet;
static CharsetSet& supportedCharsets()
{
    static CharsetSet sl;
    return sl;
}

static LanguageManager& languageManager()
{
    static LanguageManager lm;
    return lm;
}

LanguageManager::LanguageManager()
{
    COMPtr<IMultiLanguage> multiLanguage;
    if (FAILED(::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, reinterpret_cast<LPVOID*>(&multiLanguage))))
        return;

    COMPtr<IEnumCodePage> enumInterface;
    if (FAILED(multiLanguage->EnumCodePages(MIMECONTF_BROWSER, &enumInterface)))
        return;

    MIMECPINFO cpInfo;
    ULONG ccpInfo;
    while (SUCCEEDED(enumInterface->Next(1, &cpInfo, &ccpInfo)) && ccpInfo) {
        if (!IsValidCodePage(cpInfo.uiCodePage))
            continue;

        HashMap<UINT, CString>::iterator i = codePageCharsets().find(cpInfo.uiCodePage);

        CString name(String(cpInfo.wszWebCharset).latin1());
        if (i == codePageCharsets().end()) {
            CharsetInfo info;
            info.m_codePage = cpInfo.uiCodePage;
            knownCharsets().set(name.data(), info);
            i = codePageCharsets().set(cpInfo.uiCodePage, name).iterator;
        }
        if (i != codePageCharsets().end()) {
            HashMap<String, CharsetInfo>::iterator j = knownCharsets().find(String(i->value.data(), i->value.length()));
            ASSERT(j != knownCharsets().end());
            CharsetInfo& info = j->value;
            info.m_name = i->value.data();
            info.m_friendlyName = cpInfo.wszDescription;
            info.m_aliases.append(name);
            info.m_aliases.append(String(cpInfo.wszHeaderCharset).latin1());
            info.m_aliases.append(String(cpInfo.wszBodyCharset).latin1());
            String cpName = "cp" + String::number(cpInfo.uiCodePage);
            info.m_aliases.append(cpName.latin1());
            supportedCharsets().add(i->value.data());
        }
    }
}

static UINT getCodePage(const char* name)
{
    // Explicitly use a "const" reference to fix the silly VS build error
    // saying "==" is not found for const_iterator and iterator
    const HashMap<String, CharsetInfo>& charsets = knownCharsets();
    HashMap<String, CharsetInfo>::const_iterator i = charsets.find(name);
    return i == charsets.end() ? CP_ACP : i->value.m_codePage;
}

static PassOwnPtr<TextCodec> newTextCodecWin(const TextEncoding& encoding, const void*)
{
    return adoptPtr(new TextCodecWin(getCodePage(encoding.name())));
}

TextCodecWin::TextCodecWin(UINT codePage)
    : m_codePage(codePage)
{
}

TextCodecWin::~TextCodecWin()
{
}

void TextCodecWin::registerExtendedEncodingNames(EncodingNameRegistrar registrar)
{
    languageManager();
    for (CharsetSet::iterator i = supportedCharsets().begin(); i != supportedCharsets().end(); ++i) {
        HashMap<String, CharsetInfo>::iterator j = knownCharsets().find(*i);
        if (j != knownCharsets().end()) {
            registrar(j->value.m_name.data(), j->value.m_name.data());
            for (Vector<CString>::const_iterator alias = j->value.m_aliases.begin(); alias != j->value.m_aliases.end(); ++alias)
                registrar(alias->data(), j->value.m_name.data());
        }
    }
}

void TextCodecWin::registerExtendedCodecs(TextCodecRegistrar registrar)
{
    languageManager();
    for (CharsetSet::iterator i = supportedCharsets().begin(); i != supportedCharsets().end(); ++i) {
        HashMap<String, CharsetInfo>::iterator j = knownCharsets().find(*i);
        if (j != knownCharsets().end())
            registrar(j->value.m_name.data(), newTextCodecWin, 0);
    }
}

static DWORD getCodePageFlags(UINT codePage)
{
    if (codePage == 42) // Symbol
        return 0;

    // Microsoft says the flag must be 0 for the following code pages
    if (codePage > 50000) {
        if ((codePage >= 50220 && codePage <= 50222)
            || codePage == 50225
            || codePage == 50227
            || codePage == 50229
            || codePage == 52936
            || codePage == 54936
            || (codePage >= 57002 && codePage <= 57001)
            || codePage == 65000 // UTF-7
            )
            return 0;
    }

    return MB_PRECOMPOSED | MB_ERR_INVALID_CHARS;
}

static inline const char* findFirstNonAsciiCharacter(const char* bytes, size_t length)
{
    for (const char* bytesEnd = bytes + length; bytes < bytesEnd; ++bytes) {
        if (*bytes & 0x80)
            break;
    }
    return bytes;
}

static void decodeInternal(Vector<UChar, 8192>& result, UINT codePage, const char* bytes, size_t length, size_t* left)
{
    *left = length;
    if (!bytes || !length)
        return;

    DWORD flags = getCodePageFlags(codePage);

    int testLength = length;
    int untestedLength = length;
    for (;;) {
        int resultLength = MultiByteToWideChar(codePage, flags, bytes, testLength, 0, 0);

        if (resultLength > 0) {
            int oldSize = result.size();
            result.resize(oldSize + resultLength);

            MultiByteToWideChar(codePage, flags, bytes, testLength, result.data() + oldSize, resultLength);

            if (testLength == untestedLength) {
                *left = length - testLength;
                break;
            }
            untestedLength -= testLength;
            length -= testLength;
            bytes += testLength;
        } else {
            untestedLength = testLength - 1;
            if (!untestedLength) {
                *left = length;
                break;
            }
        }
        testLength = (untestedLength + 1) / 2;
    }
}

String TextCodecWin::decode(const char* bytes, size_t length, bool flush, bool stopOnError, bool& sawError)
{
    if (!m_decodeBuffer.isEmpty()) {
        m_decodeBuffer.append(bytes, length);
        bytes = m_decodeBuffer.data();
        length = m_decodeBuffer.size();
    }

    size_t left;
    Vector<UChar, 8192> result;
    for (;;) {
        decodeInternal(result, m_codePage, bytes, length, &left);
        if (!left)
            break;

        if (!flush && left < 16)
            break;

        result.append(L'?');
        sawError = true;
        if (stopOnError)
            return String::adopt(result);

        if (left == 1)
            break;

        bytes += length - left + 1;
        length = left - 1;
    }
    if (left && !flush) {
        if (m_decodeBuffer.isEmpty())
            m_decodeBuffer.append(bytes + length - left, left);
        else {
            memmove(m_decodeBuffer.data(), bytes + length - left, left);
            m_decodeBuffer.resize(left);
        }
    } else
        m_decodeBuffer.clear();

    return String::adopt(result);
}

CString TextCodecWin::encode(const UChar* characters, size_t length, UnencodableHandling)
{
    if (!characters || !length)
        return CString();

    int resultLength = WideCharToMultiByte(m_codePage, WC_COMPOSITECHECK, characters, length, 0, 0, 0, 0);

    // FIXME: We need to implement UnencodableHandling: QuestionMarksForUnencodables, EntitiesForUnencodables, and URLEncodedEntitiesForUnencodables.

    if (resultLength <= 0)
        return "?";

    char* characterBuffer;
    CString result = CString::newUninitialized(resultLength, characterBuffer);

    WideCharToMultiByte(m_codePage, WC_COMPOSITECHECK, characters, length, characterBuffer, resultLength, 0, 0);

    return result;
}

void TextCodecWin::enumerateSupportedEncodings(EncodingReceiver& receiver)
{
    languageManager();
    for (CharsetSet::iterator i = supportedCharsets().begin(); i != supportedCharsets().end(); ++i) {
        HashMap<String, CharsetInfo>::iterator j = knownCharsets().find(*i);
        if (j != knownCharsets().end() && !receiver.receive(j->value.m_name.data(), j->value.m_friendlyName.charactersWithNullTermination().data(), j->value.m_codePage))
            break;
    }
}

} // namespace WebCore
