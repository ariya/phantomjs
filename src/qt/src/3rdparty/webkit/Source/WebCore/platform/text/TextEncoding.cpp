/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "TextEncoding.h"

#include "PlatformString.h"
#include "TextCodec.h"
#include "TextEncodingRegistry.h"
#if USE(ICU_UNICODE)
#include <unicode/unorm.h>
#elif USE(QT4_UNICODE)
#include <QString>
#elif USE(GLIB_UNICODE)
#include <glib.h>
#include "GOwnPtr.h"
#endif
#include <wtf/text/CString.h>
#include <wtf/OwnPtr.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

static const TextEncoding& UTF7Encoding()
{
    static TextEncoding globalUTF7Encoding("UTF-7");
    return globalUTF7Encoding;
}

TextEncoding::TextEncoding(const char* name)
    : m_name(atomicCanonicalTextEncodingName(name))
    , m_backslashAsCurrencySymbol(backslashAsCurrencySymbol())
{
}

TextEncoding::TextEncoding(const String& name)
    : m_name(atomicCanonicalTextEncodingName(name.characters(), name.length()))
    , m_backslashAsCurrencySymbol(backslashAsCurrencySymbol())
{
}

String TextEncoding::decode(const char* data, size_t length, bool stopOnError, bool& sawError) const
{
    if (!m_name)
        return String();

    return newTextCodec(*this)->decode(data, length, true, stopOnError, sawError);
}

CString TextEncoding::encode(const UChar* characters, size_t length, UnencodableHandling handling) const
{
    if (!m_name)
        return CString();

    if (!length)
        return "";

#if USE(ICU_UNICODE)
    // FIXME: What's the right place to do normalization?
    // It's a little strange to do it inside the encode function.
    // Perhaps normalization should be an explicit step done before calling encode.

    const UChar* source = characters;
    size_t sourceLength = length;

    Vector<UChar> normalizedCharacters;

    UErrorCode err = U_ZERO_ERROR;
    if (unorm_quickCheck(source, sourceLength, UNORM_NFC, &err) != UNORM_YES) {
        // First try using the length of the original string, since normalization to NFC rarely increases length.
        normalizedCharacters.grow(sourceLength);
        int32_t normalizedLength = unorm_normalize(source, length, UNORM_NFC, 0, normalizedCharacters.data(), length, &err);
        if (err == U_BUFFER_OVERFLOW_ERROR) {
            err = U_ZERO_ERROR;
            normalizedCharacters.resize(normalizedLength);
            normalizedLength = unorm_normalize(source, length, UNORM_NFC, 0, normalizedCharacters.data(), normalizedLength, &err);
        }
        ASSERT(U_SUCCESS(err));

        source = normalizedCharacters.data();
        sourceLength = normalizedLength;
    }
    return newTextCodec(*this)->encode(source, sourceLength, handling);
#elif USE(QT4_UNICODE)
    QString str(reinterpret_cast<const QChar*>(characters), length);
    str = str.normalized(QString::NormalizationForm_C);
    return newTextCodec(*this)->encode(reinterpret_cast<const UChar *>(str.utf16()), str.length(), handling);
#elif USE(GLIB_UNICODE)
    GOwnPtr<char> UTF8Source;
    UTF8Source.set(g_utf16_to_utf8(characters, length, 0, 0, 0));
    if (!UTF8Source) {
        // If conversion to UTF-8 failed, try with the string without normalization
        return newTextCodec(*this)->encode(characters, length, handling);
    }

    GOwnPtr<char> UTF8Normalized;
    UTF8Normalized.set(g_utf8_normalize(UTF8Source.get(), -1, G_NORMALIZE_NFC));

    long UTF16Length;
    GOwnPtr<UChar> UTF16Normalized;
    UTF16Normalized.set(g_utf8_to_utf16(UTF8Normalized.get(), -1, 0, &UTF16Length, 0));

    return newTextCodec(*this)->encode(UTF16Normalized.get(), UTF16Length, handling);
#elif OS(WINCE)
    // normalization will be done by Windows CE API
    OwnPtr<TextCodec> textCodec = newTextCodec(*this);
    return textCodec.get() ? textCodec->encode(characters, length, handling) : CString();
#elif USE(BREWMP_UNICODE)
    // FIXME: not sure if Brew MP normalizes the input string automatically
    OwnPtr<TextCodec> textCodec = newTextCodec(*this);
    return textCodec.get() ? textCodec->encode(characters, length, handling) : CString();
#endif
}

const char* TextEncoding::domName() const
{
    if (noExtendedTextEncodingNameUsed())
        return m_name;

    // We treat EUC-KR as windows-949 (its superset), but need to expose 
    // the name 'EUC-KR' because the name 'windows-949' is not recognized by
    // most Korean web servers even though they do use the encoding
    // 'windows-949' with the name 'EUC-KR'. 
    // FIXME: This is not thread-safe. At the moment, this function is
    // only accessed in a single thread, but eventually has to be made
    // thread-safe along with usesVisualOrdering().
    static const char* const a = atomicCanonicalTextEncodingName("windows-949");
    if (m_name == a)
        return "EUC-KR";
    return m_name;
}

bool TextEncoding::usesVisualOrdering() const
{
    if (noExtendedTextEncodingNameUsed())
        return false;

    static const char* const a = atomicCanonicalTextEncodingName("ISO-8859-8");
    return m_name == a;
}

bool TextEncoding::isJapanese() const
{
    return isJapaneseEncoding(m_name);
}

UChar TextEncoding::backslashAsCurrencySymbol() const
{
    return shouldShowBackslashAsCurrencySymbolIn(m_name) ? 0x00A5 : '\\';
}

bool TextEncoding::isNonByteBasedEncoding() const
{
    if (noExtendedTextEncodingNameUsed()) {
        return *this == UTF16LittleEndianEncoding()
            || *this == UTF16BigEndianEncoding();
    }

    return *this == UTF16LittleEndianEncoding()
        || *this == UTF16BigEndianEncoding()
        || *this == UTF32BigEndianEncoding()
        || *this == UTF32LittleEndianEncoding();
}

bool TextEncoding::isUTF7Encoding() const
{
    if (noExtendedTextEncodingNameUsed())
        return false;

    return *this == UTF7Encoding();
}

const TextEncoding& TextEncoding::closestByteBasedEquivalent() const
{
    if (isNonByteBasedEncoding())
        return UTF8Encoding();
    return *this; 
}

// HTML5 specifies that UTF-8 be used in form submission when a form is 
// is a part of a document in UTF-16 probably because UTF-16 is not a 
// byte-based encoding and can contain 0x00. By extension, the same
// should be done for UTF-32. In case of UTF-7, it is a byte-based encoding,
// but it's fraught with problems and we'd rather steer clear of it.
const TextEncoding& TextEncoding::encodingForFormSubmission() const
{
    if (isNonByteBasedEncoding() || isUTF7Encoding())
        return UTF8Encoding();
    return *this;
}

const TextEncoding& ASCIIEncoding()
{
    static TextEncoding globalASCIIEncoding("ASCII");
    return globalASCIIEncoding;
}

const TextEncoding& Latin1Encoding()
{
    static TextEncoding globalLatin1Encoding("latin1");
    return globalLatin1Encoding;
}

const TextEncoding& UTF16BigEndianEncoding()
{
    static TextEncoding globalUTF16BigEndianEncoding("UTF-16BE");
    return globalUTF16BigEndianEncoding;
}

const TextEncoding& UTF16LittleEndianEncoding()
{
    static TextEncoding globalUTF16LittleEndianEncoding("UTF-16LE");
    return globalUTF16LittleEndianEncoding;
}

const TextEncoding& UTF32BigEndianEncoding()
{
    static TextEncoding globalUTF32BigEndianEncoding("UTF-32BE");
    return globalUTF32BigEndianEncoding;
}

const TextEncoding& UTF32LittleEndianEncoding()
{
    static TextEncoding globalUTF32LittleEndianEncoding("UTF-32LE");
    return globalUTF32LittleEndianEncoding;
}

const TextEncoding& UTF8Encoding()
{
    static TextEncoding globalUTF8Encoding("UTF-8");
    ASSERT(globalUTF8Encoding.isValid());
    return globalUTF8Encoding;
}

const TextEncoding& WindowsLatin1Encoding()
{
    static TextEncoding globalWindowsLatin1Encoding("WinLatin-1");
    return globalWindowsLatin1Encoding;
}

} // namespace WebCore
