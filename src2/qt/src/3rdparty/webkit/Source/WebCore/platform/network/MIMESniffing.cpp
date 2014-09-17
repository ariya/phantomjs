/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "MIMESniffing.h"

#if OS(QNX)
#include <string.h>
#else
#include <cstring>
#endif
#include <stdint.h>

// MIME type sniffing implementation based on http://tools.ietf.org/html/draft-abarth-mime-sniff-06

namespace {

static inline bool isTextInList(const char* text, size_t size, const char** data)
{
    for (size_t i = 0; i < size; ++i) {
        if (!strcmp(text, data[i]))
            return true;
    }
    return false;

}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-6
const char* textTypes[] = {
    "text/plain",
    "text/plain; charset=ISO-8859-1",
    "text/plain; charset=iso-8859-1",
    "text/plain; charset=UTF-8"
};
const size_t textTypesSize = sizeof(textTypes) / sizeof(textTypes[0]);

static inline bool isTextOrBinaryType(const char* type)
{
    return isTextInList(type, textTypesSize, textTypes);
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-6
const char* unknownTypes[] = {
    "",
    "unknown/unknown",
    "application/unknown",
    "*/*"
};
const size_t unknownTypesSize = sizeof(unknownTypes) / sizeof(unknownTypes[0]);

static inline bool isUnknownType(const char* type)
{
    return isTextInList(type, unknownTypesSize, unknownTypes);
}

const char* xmlTypes[] = {
    "text/xml",
    "application/xml"
};
const size_t xmlTypesSize = sizeof(xmlTypes) / sizeof(xmlTypes[0]);

const char xmlSuffix[] = "+xml";

static inline bool isXMLType(const char* type)
{
    const size_t xmlSuffixSize = sizeof(xmlSuffix) - 1;
    size_t typeSize = strlen(type);
    if (typeSize >= xmlSuffixSize && !memcmp(type + typeSize - xmlSuffixSize, xmlSuffix, xmlSuffixSize))
        return true;

    return isTextInList(type, xmlTypesSize, xmlTypes);
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-8
const char binaryFlags[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static inline bool isBinaryChar(unsigned char data)
{
    return binaryFlags[data];
}

static inline bool isBinaryData(const char* data, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (isBinaryChar(data[i]))
            return true;
    }
    return false;
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-11
const char whiteSpaceChars[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static inline bool isWhiteSpace(unsigned char data)
{
    return whiteSpaceChars[data];
}

static inline void skipWhiteSpace(const char* data, size_t& pos, size_t dataSize)
{
    while (pos < dataSize && isWhiteSpace(data[pos]))
        ++pos;
}

enum {
    SkipWhiteSpace = 1,
    TrailingSpaceOrBracket = 2
};

struct MagicNumbers {
    const char* pattern;
    const char* mask;
    const char* mimeType;
    size_t size;
    int flags;
};

#define MAGIC_NUMBERS_MASKED(pattern, mask, mimeType, flags) {(pattern), (mask), (mimeType), sizeof(pattern) - 1, (flags)}
#define MAGIC_NUMBERS_SIMPLE(pattern, mimeType) {(pattern), 0, (mimeType), sizeof(pattern) - 1, 0}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-12
const MagicNumbers securityConstrainedTypes[] = {
    MAGIC_NUMBERS_MASKED("<!DOCTYPE HTML", "\xFF\xFF\xDF\xDF\xDF\xDF\xDF\xDF\xDF\xFF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<HTML", "\xFF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<HEAD", "\xFF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<SCRIPT", "\xFF\xDF\xDF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<IFRAME", "\xFF\xDF\xDF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<H1", "\xFF\xDF\xFF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<DIV", "\xFF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<FONT", "\xFF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<TABLE", "\xFF\xDF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<A", "\xFF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<STYLE", "\xFF\xDF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<TITLE", "\xFF\xDF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<B", "\xFF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<BODY", "\xFF\xDF\xDF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<BR", "\xFF\xDF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<P", "\xFF\xDF", "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<!--", 0, "text/html", SkipWhiteSpace | TrailingSpaceOrBracket),
    MAGIC_NUMBERS_MASKED("<?xml", 0, "text/xml", SkipWhiteSpace),
    MAGIC_NUMBERS_SIMPLE("%PDF-", "application/pdf")
};
const size_t securityConstrainedTypesSize = sizeof(securityConstrainedTypes) / sizeof(securityConstrainedTypes[0]);

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-8
const MagicNumbers bomTypes[] = {
    MAGIC_NUMBERS_SIMPLE("\xFE\xFF", "text/plain"), // UTF-16BE BOM
    MAGIC_NUMBERS_SIMPLE("\xFF\xFE", "text/plain"), // UTF-16LE BOM
    MAGIC_NUMBERS_SIMPLE("\xEF\xBB\xBF", "text/plain") // UTF-8 BOM
};
const size_t bomTypesSize = sizeof(bomTypes) / sizeof(bomTypes[0]);

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-13
const MagicNumbers safeTypes[] = {
    MAGIC_NUMBERS_SIMPLE("%!PS-Adobe-", "application/postscript"),
    MAGIC_NUMBERS_SIMPLE("\x4F\x67\x67\x53\x00", "application/ogg"), // An Ogg Vorbis audio or video signature.
    MAGIC_NUMBERS_MASKED("RIFF\x00\x00\x00\x00WAVE", "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", "audio/x-wave", 0), // "RIFF" followed by four bytes, followed by "WAVE".
    MAGIC_NUMBERS_SIMPLE("\x1A\x45\xDF\xA3", "video/webm"), // The WebM signature.
    MAGIC_NUMBERS_SIMPLE("Rar!\x1A\x07\x00", "application/x-rar-compressed"), // A RAR archive.
    MAGIC_NUMBERS_SIMPLE("\x50\x4B\x03\x04", "application/zip"), // A ZIP archive.
    MAGIC_NUMBERS_SIMPLE("\x1F\x8B\x08", "application/x-gzip") // A GZIP archive.
};
const size_t safeTypesSize = sizeof(safeTypes) / sizeof(safeTypes[0]);

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-16
const MagicNumbers imageTypes[] = {
    MAGIC_NUMBERS_MASKED("RIFF\x00\x00\x00\x00WEBPVP", "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF", "image/webp", 0), // "RIFF" followed by four bytes, followed by "WEBPVP".
    MAGIC_NUMBERS_SIMPLE("GIF87a", "image/gif"),
    MAGIC_NUMBERS_SIMPLE("GIF89a", "image/gif"),
    MAGIC_NUMBERS_SIMPLE("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", "image/png"),
    MAGIC_NUMBERS_SIMPLE("\xFF\xD8\xFF", "image/jpeg"),
    MAGIC_NUMBERS_SIMPLE("BM", "image/bmp"),
    MAGIC_NUMBERS_SIMPLE("\x00\x00\x01\x00", "image/vnd.microsoft.icon") // A Windows Icon signature.
};
const size_t imageTypesSize = sizeof(imageTypes) / sizeof(imageTypes[0]);

static inline size_t dataSizeNeededForImageSniffing()
{
    size_t result = 0;
    for (int i = 0; i < imageTypesSize; ++i) {
        if (imageTypes[i].size > result)
            result = imageTypes[i].size;
    }
    return result;
}

static inline bool maskedCompare(const MagicNumbers& info, const char* data, size_t dataSize)
{
    if (dataSize < info.size)
        return false;

    const uint32_t* pattern32 = reinterpret_cast<const uint32_t*>(info.pattern);
    const uint32_t* mask32 = reinterpret_cast<const uint32_t*>(info.mask);
    const uint32_t* data32 = reinterpret_cast<const uint32_t*>(data);

    size_t count = info.size >> 2;

    for (size_t i = 0; i < count; ++i) {
        if ((*data32++ & *mask32++) != *pattern32++)
            return false;
    }

    const char* p = reinterpret_cast<const char*>(pattern32);
    const char* m = reinterpret_cast<const char*>(mask32);
    const char* d = reinterpret_cast<const char*>(data32);

    count = info.size & 3;

    for (size_t i = 0; i < count; ++i) {
        if ((*d++ & *m++) != *p++)
            return false;
    }

    return true;
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-11
static inline bool checkSpaceOrBracket(const char* data)
{
    return isWhiteSpace(*data) || *data == 0x3E;
}

static inline bool compare(const MagicNumbers& info, const char* data, size_t dataSize)
{
    if (info.flags & SkipWhiteSpace) {
        size_t pos = 0;
        skipWhiteSpace(data, pos, dataSize);
        data += pos;
        dataSize -= pos;
    }

    bool result;
    if (info.mask)
        result = maskedCompare(info, data, info.size);
    else
        result = dataSize >= info.size && !memcmp(data, info.pattern, info.size);

    return result && (!(info.flags & TrailingSpaceOrBracket) || checkSpaceOrBracket(data + info.size));
}

static inline const char* findMIMEType(const char* data, size_t dataSize, const MagicNumbers* types, size_t typesCount)
{
    for (size_t i = 0; i < typesCount; ++i) {
        if (compare(types[i], data, dataSize))
            return types[i].mimeType;
    }
    return 0;
}

static inline const char* findSimpleMIMEType(const char* data, size_t dataSize, const MagicNumbers* types, size_t typesCount)
{
    for (size_t i = 0; i < typesCount; ++i) {
        ASSERT(!types[i].mask);
        ASSERT(!types[i].flags);

        if (dataSize >= types[i].size && !memcmp(data, types[i].pattern, types[i].size))
            return types[i].mimeType;
    }
    return 0;
}

bool isTypeInList(const char* type, const MagicNumbers* types, size_t typesCount)
{
    for (size_t i = 0; i < typesCount; ++i) {
        if (!strcmp(type, types[i].mimeType))
            return true;
    }
    return false;
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-8
static const char* internalTextOrBinaryTypeSniffingProcedure(const char* data, size_t dataSize)
{
    const char* mimeType = 0;

    mimeType = findSimpleMIMEType(data, dataSize, bomTypes, bomTypesSize);
    if (mimeType)
        return mimeType;

    if (!isBinaryData(data, dataSize))
        return "text/plain";

    mimeType = findMIMEType(data, dataSize, safeTypes, safeTypesSize);
    if (mimeType)
        return mimeType;

    mimeType = findMIMEType(data, dataSize, imageTypes, imageTypesSize);
    if (mimeType)
        return mimeType;

    return "application/octet-stream";
}

static const char* textOrBinaryTypeSniffingProcedure(const char* data, size_t dataSize)
{
    const char* result = internalTextOrBinaryTypeSniffingProcedure(data, dataSize);
    ASSERT(!isTypeInList(result, securityConstrainedTypes, securityConstrainedTypesSize));
    return result;
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-10
static const char* unknownTypeSniffingProcedure(const char* data, size_t dataSize)
{
    const char* mimeType = 0;

    mimeType = findMIMEType(data, dataSize, securityConstrainedTypes, securityConstrainedTypesSize);
    if (mimeType)
        return mimeType;

    mimeType = findSimpleMIMEType(data, dataSize, bomTypes, bomTypesSize);
    if (mimeType)
        return mimeType;

    mimeType = findMIMEType(data, dataSize, safeTypes, safeTypesSize);
    if (mimeType)
        return mimeType;

    mimeType = findMIMEType(data, dataSize, imageTypes, imageTypesSize);
    if (mimeType)
        return mimeType;

    if (!isBinaryData(data, dataSize))
        return "text/plain";

    return "application/octet-stream";
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-16
static const char* imageTypeSniffingProcedure(const char* data, size_t dataSize)
{
    return findMIMEType(data, dataSize, imageTypes, imageTypesSize);
}

static inline bool checkText(const char* data, size_t& pos, size_t dataSize, const char* text, size_t textSize)
{
    if (dataSize - pos < textSize || memcmp(data + pos, text, textSize))
        return false;

    pos += textSize;
    return true;
}

const char rssUrl[] = "http://purl.org/rss/1.0";
const char rdfUrl[] = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

static inline const char* checkRDF(const char* data, size_t pos, size_t dataSize)
{
    bool isRDF = false;
    bool isRSS = false;

    while (pos <= dataSize) {
        if (checkText(data, pos, dataSize, rssUrl, sizeof(rssUrl) - 1)) {
            isRSS = true;
            continue;
        }

        if (checkText(data, pos, dataSize, rdfUrl, sizeof(rdfUrl) - 1)) {
            isRDF = true;
            continue;
        }

        ++pos;

        if (isRSS && isRDF)
            return "application/rdf+xml";
    }

    return 0;
}

static inline bool skipTag(const char*& data, size_t& pos, size_t dataSize, const char* tag, size_t tagSize, const char* tagEnd, size_t tagEndSize)
{
    if (!checkText(data, pos, dataSize, tag, tagSize))
        return false;

    while (pos < dataSize && !checkText(data, pos, dataSize, tagEnd, tagEndSize))
        ++pos;

    return true;
}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-17
static const char* feedTypeSniffingProcedure(const char* data, size_t dataSize)
{
    size_t pos = 0;

    if (dataSize >= 3 && !memcmp(data, "\xEF\xBB\xBF", 3))
        pos += 3;

    while (pos < dataSize) {
        skipWhiteSpace(data, pos, dataSize);

        if (!skipTag(data, pos, dataSize, "<!--", 4, "-->", 3) && !skipTag(data, pos, dataSize, "<!", 2, "!>", 2) && !skipTag(data, pos, dataSize, "<?", 2, "?>", 2))
            break;
    }

    if (checkText(data, pos, dataSize, "<rss", 4))
        return "application/rss+xml";

    if (checkText(data, pos, dataSize, "<feed", 5))
        return "application/atom+xml";

    if (checkText(data, pos, dataSize, "<rdf:RDF", 8))
        return checkRDF(data, pos, dataSize);

    return 0;
}

}

// http://tools.ietf.org/html/draft-abarth-mime-sniff-06#page-6
MIMESniffer::MIMESniffer(const char* advertisedMIMEType, bool isSupportedImageType)
    : m_dataSize(0)
    , m_function(0)
{
    if (!advertisedMIMEType) {
        m_dataSize = 512;
        m_function = &unknownTypeSniffingProcedure;
        return;
    }

    if (isTextOrBinaryType(advertisedMIMEType)) {
        m_dataSize = 512;
        m_function = &textOrBinaryTypeSniffingProcedure;
        return;
    }

    if (isUnknownType(advertisedMIMEType)) {
        m_dataSize = 512;
        m_function = &unknownTypeSniffingProcedure;
        return;
    }

    if (isXMLType(advertisedMIMEType))
        return;

    if (isSupportedImageType) {
        static const size_t dataSize = dataSizeNeededForImageSniffing();
        m_dataSize = dataSize;
        m_function = &imageTypeSniffingProcedure;
        return;
    }

    if (!strcmp(advertisedMIMEType, "text/html")) {
        m_dataSize = 512;
        m_function = &feedTypeSniffingProcedure;
        return;
    }
}
