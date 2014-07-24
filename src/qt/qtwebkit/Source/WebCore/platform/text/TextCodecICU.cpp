/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
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
#if USE(ICU_UNICODE)
#include "TextCodecICU.h"

#include "TextEncoding.h"
#include "TextEncodingRegistry.h"
#include "ThreadGlobalData.h"
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <wtf/Assertions.h>
#include <wtf/StringExtras.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

using std::min;

namespace WebCore {

const size_t ConversionBufferSize = 16384;

ICUConverterWrapper::~ICUConverterWrapper()
{
    if (converter)
        ucnv_close(converter);
}

static UConverter*& cachedConverterICU()
{
    return threadGlobalData().cachedConverterICU().converter;
}

PassOwnPtr<TextCodec> TextCodecICU::create(const TextEncoding& encoding, const void* additionalData)
{
    // Name strings are persistently kept in TextEncodingRegistry maps, so they are never deleted.
    return adoptPtr(new TextCodecICU(encoding.name(), static_cast<const char*>(additionalData)));
}

void TextCodecICU::registerEncodingNames(EncodingNameRegistrar registrar)
{
    // We register Hebrew with logical ordering using a separate name.
    // Otherwise, this would share the same canonical name as the
    // visual ordering case, and then TextEncoding could not tell them
    // apart; ICU treats these names as synonyms.
    registrar("ISO-8859-8-I", "ISO-8859-8-I");

    int32_t numConverters = ucnv_countAvailable();
    for (int32_t i = 0; i < numConverters; ++i) {
        const char* canonicalConverterName = ucnv_getAvailableName(i);
        UErrorCode error = U_ZERO_ERROR;
        // Try MIME before trying IANA to pick up commonly used names like
        // 'EUC-JP' instead of horrendously long names like 
        // 'Extended_UNIX_Code_Packed_Format_for_Japanese'. 
        const char* webStandardName = ucnv_getStandardName(canonicalConverterName, "MIME", &error);
        if (!U_SUCCESS(error) || !webStandardName) {
            error = U_ZERO_ERROR;
            // Try IANA to pick up 'windows-12xx' and other names
            // which are not preferred MIME names but are widely used. 
            webStandardName = ucnv_getStandardName(canonicalConverterName, "IANA", &error);
            if (!U_SUCCESS(error) || !webStandardName)
                continue;
        }

        // Any standard encoding overrides should match checks in registerCodecs() below.

        // 1. Treat GB2312 encoding as GBK (its more modern superset), to match other browsers.
        // 2. On the Web, GB2312 is encoded as EUC-CN or HZ, while ICU provides a native encoding
        //    for encoding GB_2312-80 and several others. So, we need to override this behavior, too.
        if (strcmp(webStandardName, "GB2312") == 0 || strcmp(webStandardName, "GB_2312-80") == 0)
            webStandardName = "GBK";
        // Similarly, EUC-KR encodings all map to an extended version.
        else if (strcmp(webStandardName, "KSC_5601") == 0 || strcmp(webStandardName, "EUC-KR") == 0 || strcmp(webStandardName, "cp1363") == 0)
            webStandardName = "windows-949";
        // And so on.
        // FIXME: strcasecmp is locale sensitive, we should not be using it.
        else if (strcasecmp(webStandardName, "iso-8859-9") == 0) // This name is returned in different case by ICU 3.2 and 3.6.
            webStandardName = "windows-1254";
        else if (strcmp(webStandardName, "TIS-620") == 0)
            webStandardName = "windows-874";

        registrar(webStandardName, webStandardName);

        uint16_t numAliases = ucnv_countAliases(canonicalConverterName, &error);
        ASSERT(U_SUCCESS(error));
        if (U_SUCCESS(error))
            for (uint16_t j = 0; j < numAliases; ++j) {
                error = U_ZERO_ERROR;
                const char* alias = ucnv_getAlias(canonicalConverterName, j, &error);
                ASSERT(U_SUCCESS(error));
                if (U_SUCCESS(error) && alias != webStandardName)
                    registrar(alias, webStandardName);
            }
    }

    // Additional aliases.
    // macroman is present in modern versions of ICU, but not in ICU 3.2 (shipped with Mac OS X 10.4).
    // FIXME: Do any ports still use such old versions?
    registrar("macroman", "macintosh");

    // Additional aliases that historically were present in the encoding
    // table in WebKit on Macintosh that don't seem to be present in ICU.
    // Perhaps we can prove these are not used on the web and remove them.
    // Or perhaps we can get them added to ICU.
    registrar("x-mac-roman", "macintosh");
    registrar("maccyrillic", "x-mac-cyrillic");
    registrar("x-mac-ukrainian", "x-mac-cyrillic");
    registrar("cn-big5", "Big5");
    registrar("x-x-big5", "Big5");
    registrar("cn-gb", "GBK");
    registrar("csgb231280", "GBK");
    registrar("x-euc-cn", "GBK");
    registrar("x-gbk", "GBK");
    registrar("csISO88598I", "ISO-8859-8-I");
    registrar("koi", "KOI8-R");
    registrar("logical", "ISO-8859-8-I");
    registrar("visual", "ISO-8859-8");
    registrar("winarabic", "windows-1256");
    registrar("winbaltic", "windows-1257");
    registrar("wincyrillic", "windows-1251");
    registrar("iso-8859-11", "windows-874");
    registrar("iso8859-11", "windows-874");
    registrar("dos-874", "windows-874");
    registrar("wingreek", "windows-1253");
    registrar("winhebrew", "windows-1255");
    registrar("winlatin2", "windows-1250");
    registrar("winturkish", "windows-1254");
    registrar("winvietnamese", "windows-1258");
    registrar("x-cp1250", "windows-1250");
    registrar("x-cp1251", "windows-1251");
    registrar("x-euc", "EUC-JP");
    registrar("x-windows-949", "windows-949");
    registrar("KSC5601", "windows-949");
    registrar("x-uhc", "windows-949");
    registrar("shift-jis", "Shift_JIS");

    // These aliases are present in modern versions of ICU, but use different codecs, and have no standard names.
    // They are not present in ICU 3.2.
    registrar("dos-720", "cp864");
    registrar("jis7", "ISO-2022-JP");

    // Alternative spelling of ISO encoding names.
    registrar("ISO8859-1", "ISO-8859-1");
    registrar("ISO8859-2", "ISO-8859-2");
    registrar("ISO8859-3", "ISO-8859-3");
    registrar("ISO8859-4", "ISO-8859-4");
    registrar("ISO8859-5", "ISO-8859-5");
    registrar("ISO8859-6", "ISO-8859-6");
    registrar("ISO8859-7", "ISO-8859-7");
    registrar("ISO8859-8", "ISO-8859-8");
    registrar("ISO8859-8-I", "ISO-8859-8-I");
    registrar("ISO8859-9", "windows-1254");
    registrar("ISO8859-10", "ISO-8859-10");
    registrar("ISO8859-13", "ISO-8859-13");
    registrar("ISO8859-14", "ISO-8859-14");
    registrar("ISO8859-15", "ISO-8859-15");
    // Not registering ISO8859-16, because Firefox (as of version 3.6.6) doesn't know this particular alias,
    // and because older versions of ICU don't support ISO-8859-16 encoding at all.
}

void TextCodecICU::registerCodecs(TextCodecRegistrar registrar)
{
    // See comment above in registerEncodingNames.
    UErrorCode error = U_ZERO_ERROR;
    const char* canonicalConverterName = ucnv_getCanonicalName("ISO-8859-8-I", "IANA", &error);
    ASSERT(U_SUCCESS(error));
    registrar("ISO-8859-8-I", create, canonicalConverterName);

    int32_t numConverters = ucnv_countAvailable();
    for (int32_t i = 0; i < numConverters; ++i) {
        canonicalConverterName = ucnv_getAvailableName(i);
        error = U_ZERO_ERROR;
        const char* webStandardName = ucnv_getStandardName(canonicalConverterName, "MIME", &error);
        if (!U_SUCCESS(error) || !webStandardName) {
            error = U_ZERO_ERROR;
            webStandardName = ucnv_getStandardName(canonicalConverterName, "IANA", &error);
            if (!U_SUCCESS(error) || !webStandardName)
                continue;
        }

        // Don't register codecs for overridden encodings.
        if (strcmp(webStandardName, "GB2312") == 0 || strcmp(webStandardName, "GB_2312-80") == 0
            || strcmp(webStandardName, "KSC_5601") == 0 || strcmp(webStandardName, "EUC-KR") == 0
            || strcmp(webStandardName, "cp1363") == 0
            || strcasecmp(webStandardName, "iso-8859-9") == 0
            || strcmp(webStandardName, "TIS-620") == 0)
            continue;

        registrar(webStandardName, create, fastStrDup(canonicalConverterName));
    }

    // These encodings currently don't have standard names, so we need to register encoders manually.
    // FIXME: Is there a good way to determine the most up to date variant programmatically?
    registrar("windows-874", create, "windows-874-2000");
    registrar("windows-949", create, "windows-949-2000");
}

TextCodecICU::TextCodecICU(const char* encoding, const char* canonicalConverterName)
    : m_encodingName(encoding)
    , m_canonicalConverterName(canonicalConverterName)
    , m_converterICU(0)
    , m_needsGBKFallbacks(false)
{
}

TextCodecICU::~TextCodecICU()
{
    releaseICUConverter();
}

void TextCodecICU::releaseICUConverter() const
{
    if (m_converterICU) {
        UConverter*& cachedConverter = cachedConverterICU();
        if (cachedConverter)
            ucnv_close(cachedConverter);
        ucnv_reset(m_converterICU);
        cachedConverter = m_converterICU;
        m_converterICU = 0;
    }
}

void TextCodecICU::createICUConverter() const
{
    ASSERT(!m_converterICU);

    UErrorCode err;

    m_needsGBKFallbacks = !strcmp(m_encodingName, "GBK");

    UConverter*& cachedConverter = cachedConverterICU();
    if (cachedConverter) {
        err = U_ZERO_ERROR;
        const char* cachedConverterName = ucnv_getName(cachedConverter, &err);
        if (U_SUCCESS(err) && !strcmp(m_canonicalConverterName, cachedConverterName)) {
            m_converterICU = cachedConverter;
            cachedConverter = 0;
            return;
        }
    }

    err = U_ZERO_ERROR;
    m_converterICU = ucnv_open(m_canonicalConverterName, &err);
    ASSERT(U_SUCCESS(err));
    if (m_converterICU)
        ucnv_setFallback(m_converterICU, TRUE);
}

int TextCodecICU::decodeToBuffer(UChar* target, UChar* targetLimit, const char*& source, const char* sourceLimit, int32_t* offsets, bool flush, UErrorCode& err)
{
    UChar* targetStart = target;
    err = U_ZERO_ERROR;
    ucnv_toUnicode(m_converterICU, &target, targetLimit, &source, sourceLimit, offsets, flush, &err);
    return target - targetStart;
}

class ErrorCallbackSetter {
public:
    ErrorCallbackSetter(UConverter* converter, bool stopOnError)
        : m_converter(converter)
        , m_shouldStopOnEncodingErrors(stopOnError)
    {
        if (m_shouldStopOnEncodingErrors) {
            UErrorCode err = U_ZERO_ERROR;
            ucnv_setToUCallBack(m_converter, UCNV_TO_U_CALLBACK_SUBSTITUTE,
                           UCNV_SUB_STOP_ON_ILLEGAL, &m_savedAction,
                           &m_savedContext, &err);
            ASSERT(err == U_ZERO_ERROR);
        }
    }
    ~ErrorCallbackSetter()
    {
        if (m_shouldStopOnEncodingErrors) {
            UErrorCode err = U_ZERO_ERROR;
            const void* oldContext;
            UConverterToUCallback oldAction;
            ucnv_setToUCallBack(m_converter, m_savedAction,
                   m_savedContext, &oldAction,
                   &oldContext, &err);
            ASSERT(oldAction == UCNV_TO_U_CALLBACK_SUBSTITUTE);
            ASSERT(!strcmp(static_cast<const char*>(oldContext), UCNV_SUB_STOP_ON_ILLEGAL));
            ASSERT(err == U_ZERO_ERROR);
        }
    }

private:
    UConverter* m_converter;
    bool m_shouldStopOnEncodingErrors;
    const void* m_savedContext;
    UConverterToUCallback m_savedAction;
};

String TextCodecICU::decode(const char* bytes, size_t length, bool flush, bool stopOnError, bool& sawError)
{
    // Get a converter for the passed-in encoding.
    if (!m_converterICU) {
        createICUConverter();
        ASSERT(m_converterICU);
        if (!m_converterICU) {
            LOG_ERROR("error creating ICU encoder even though encoding was in table");
            return String();
        }
    }
    
    ErrorCallbackSetter callbackSetter(m_converterICU, stopOnError);

    StringBuilder result;

    UChar buffer[ConversionBufferSize];
    UChar* bufferLimit = buffer + ConversionBufferSize;
    const char* source = reinterpret_cast<const char*>(bytes);
    const char* sourceLimit = source + length;
    int32_t* offsets = NULL;
    UErrorCode err = U_ZERO_ERROR;

    do {
        int ucharsDecoded = decodeToBuffer(buffer, bufferLimit, source, sourceLimit, offsets, flush, err);
        result.append(buffer, ucharsDecoded);
    } while (err == U_BUFFER_OVERFLOW_ERROR);

    if (U_FAILURE(err)) {
        // flush the converter so it can be reused, and not be bothered by this error.
        do {
            decodeToBuffer(buffer, bufferLimit, source, sourceLimit, offsets, true, err);
        } while (source < sourceLimit);
        sawError = true;
    }

    String resultString = result.toString();

    // <http://bugs.webkit.org/show_bug.cgi?id=17014>
    // Simplified Chinese pages use the code A3A0 to mean "full-width space", but ICU decodes it as U+E5E5.
    // FIXME: strcasecmp is locale sensitive, we should not be using it.
    if (strcmp(m_encodingName, "GBK") == 0 || strcasecmp(m_encodingName, "gb18030") == 0)
        resultString.replace(0xE5E5, ideographicSpace);

    return resultString;
}

// We need to apply these fallbacks ourselves as they are not currently supported by ICU and
// they were provided by the old TEC encoding path. Needed to fix <rdar://problem/4708689>.
static UChar fallbackForGBK(UChar32 character)
{
    switch (character) {
    case 0x01F9:
        return 0xE7C8;
    case 0x1E3F:
        return 0xE7C7;
    case 0x22EF:
        return 0x2026;
    case 0x301C:
        return 0xFF5E;
    }
    return 0;
}

// Invalid character handler when writing escaped entities for unrepresentable
// characters. See the declaration of TextCodec::encode for more.
static void urlEscapedEntityCallback(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
    UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err)
{
    if (reason == UCNV_UNASSIGNED) {
        *err = U_ZERO_ERROR;

        UnencodableReplacementArray entity;
        int entityLen = TextCodec::getUnencodableReplacement(codePoint, URLEncodedEntitiesForUnencodables, entity);
        ucnv_cbFromUWriteBytes(fromUArgs, entity, entityLen, 0, err);
    } else
        UCNV_FROM_U_CALLBACK_ESCAPE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

// Substitutes special GBK characters, escaping all other unassigned entities.
static void gbkCallbackEscape(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
    UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) 
{
    UChar outChar;
    if (reason == UCNV_UNASSIGNED && (outChar = fallbackForGBK(codePoint))) {
        const UChar* source = &outChar;
        *err = U_ZERO_ERROR;
        ucnv_cbFromUWriteUChars(fromUArgs, &source, source + 1, 0, err);
        return;
    }
    UCNV_FROM_U_CALLBACK_ESCAPE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

// Combines both gbkUrlEscapedEntityCallback and GBK character substitution.
static void gbkUrlEscapedEntityCallack(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
    UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) 
{
    if (reason == UCNV_UNASSIGNED) {
        if (UChar outChar = fallbackForGBK(codePoint)) {
            const UChar* source = &outChar;
            *err = U_ZERO_ERROR;
            ucnv_cbFromUWriteUChars(fromUArgs, &source, source + 1, 0, err);
            return;
        }
        urlEscapedEntityCallback(context, fromUArgs, codeUnits, length, codePoint, reason, err);
        return;
    }
    UCNV_FROM_U_CALLBACK_ESCAPE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

static void gbkCallbackSubstitute(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
    UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) 
{
    UChar outChar;
    if (reason == UCNV_UNASSIGNED && (outChar = fallbackForGBK(codePoint))) {
        const UChar* source = &outChar;
        *err = U_ZERO_ERROR;
        ucnv_cbFromUWriteUChars(fromUArgs, &source, source + 1, 0, err);
        return;
    }
    UCNV_FROM_U_CALLBACK_SUBSTITUTE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

CString TextCodecICU::encode(const UChar* characters, size_t length, UnencodableHandling handling)
{
    if (!length)
        return "";

    if (!m_converterICU)
        createICUConverter();
    if (!m_converterICU)
        return CString();

    // FIXME: We should see if there is "force ASCII range" mode in ICU;
    // until then, we change the backslash into a yen sign.
    // Encoding will change the yen sign back into a backslash.
    String copy;
    const UChar* source;
    const UChar* sourceLimit;
    if (shouldShowBackslashAsCurrencySymbolIn(m_encodingName)) {
        copy.append(characters, length);
        copy.replace('\\', 0xA5);
        source = copy.characters();
        sourceLimit = source + copy.length();
    } else {
        source = characters;
        sourceLimit = source + length;
    }

    UErrorCode err = U_ZERO_ERROR;

    switch (handling) {
        case QuestionMarksForUnencodables:
            ucnv_setSubstChars(m_converterICU, "?", 1, &err);
            ucnv_setFromUCallBack(m_converterICU, m_needsGBKFallbacks ? gbkCallbackSubstitute : UCNV_FROM_U_CALLBACK_SUBSTITUTE, 0, 0, 0, &err);
            break;
        case EntitiesForUnencodables:
            ucnv_setFromUCallBack(m_converterICU, m_needsGBKFallbacks ? gbkCallbackEscape : UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC, 0, 0, &err);
            break;
        case URLEncodedEntitiesForUnencodables:
            ucnv_setFromUCallBack(m_converterICU, m_needsGBKFallbacks ? gbkUrlEscapedEntityCallack : urlEscapedEntityCallback, 0, 0, 0, &err);
            break;
    }

    ASSERT(U_SUCCESS(err));
    if (U_FAILURE(err))
        return CString();

    Vector<char> result;
    size_t size = 0;
    do {
        char buffer[ConversionBufferSize];
        char* target = buffer;
        char* targetLimit = target + ConversionBufferSize;
        err = U_ZERO_ERROR;
        ucnv_fromUnicode(m_converterICU, &target, targetLimit, &source, sourceLimit, 0, true, &err);
        size_t count = target - buffer;
        result.grow(size + count);
        memcpy(result.data() + size, buffer, count);
        size += count;
    } while (err == U_BUFFER_OVERFLOW_ERROR);

    return CString(result.data(), size);
}

} // namespace WebCore
#endif

