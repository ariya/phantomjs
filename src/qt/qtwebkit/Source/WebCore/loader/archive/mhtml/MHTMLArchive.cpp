/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MHTML)
#include "MHTMLArchive.h"

#include "Document.h"
#include "Frame.h"
#include "MHTMLParser.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "PageSerializer.h"
#include "QuotedPrintable.h"
#include "SchemeRegistry.h"
#include "SharedBuffer.h"

#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/DateMath.h>
#include <wtf/GregorianDateTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/Base64.h>
#include <wtf/text/StringBuilder.h>

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif
#include <time.h>


namespace WebCore {

const char* const quotedPrintable = "quoted-printable";
const char* const base64 = "base64";
const char* const binary = "binary";

static String generateRandomBoundary()
{
    // Trying to generate random boundaries similar to IE/UnMHT (ex: ----=_NextPart_000_001B_01CC157B.96F808A0).
    const size_t randomValuesLength = 10;
    char randomValues[randomValuesLength];
    cryptographicallyRandomValues(&randomValues, randomValuesLength);
    StringBuilder stringBuilder;
    stringBuilder.append("----=_NextPart_000_");
    for (size_t i = 0; i < randomValuesLength; ++i) {
        if (i == 2)
            stringBuilder.append('_');
        else if (i == 6)
            stringBuilder.append('.');
        stringBuilder.append(lowerNibbleToASCIIHexDigit(randomValues[i]));
        stringBuilder.append(upperNibbleToASCIIHexDigit(randomValues[i]));
    }
    return stringBuilder.toString();
}

static String replaceNonPrintableCharacters(const String& text)
{
    StringBuilder stringBuilder;
    for (size_t i = 0; i < text.length(); ++i) {
        if (isASCIIPrintable(text[i]))
            stringBuilder.append(text[i]);
        else
            stringBuilder.append('?');
    }
    return stringBuilder.toString();
}

MHTMLArchive::MHTMLArchive()
{
}

MHTMLArchive::~MHTMLArchive()
{
    // Because all frames know about each other we need to perform a deep clearing of the archives graph.
    clearAllSubframeArchives();
}

PassRefPtr<MHTMLArchive> MHTMLArchive::create()
{
    return adoptRef(new MHTMLArchive);
}

PassRefPtr<MHTMLArchive> MHTMLArchive::create(const KURL& url, SharedBuffer* data)
{
    // For security reasons we only load MHTML pages from local URLs.
    if (!SchemeRegistry::shouldTreatURLSchemeAsLocal(url.protocol()))
        return 0;

    MHTMLParser parser(data);
    RefPtr<MHTMLArchive> mainArchive = parser.parseArchive();
    if (!mainArchive)
        return 0; // Invalid MHTML file.

    // Since MHTML is a flat format, we need to make all frames aware of all resources.
    for (size_t i = 0; i < parser.frameCount(); ++i) {
        RefPtr<MHTMLArchive> archive = parser.frameAt(i);
        for (size_t j = 1; j < parser.frameCount(); ++j) {
            if (i != j)
                archive->addSubframeArchive(parser.frameAt(j));
        }
        for (size_t j = 0; j < parser.subResourceCount(); ++j)
            archive->addSubresource(parser.subResourceAt(j));
    }
    return mainArchive.release();
}

PassRefPtr<SharedBuffer> MHTMLArchive::generateMHTMLData(Page* page)
{
    return generateMHTMLData(page, false);
}

PassRefPtr<SharedBuffer> MHTMLArchive::generateMHTMLDataUsingBinaryEncoding(Page* page)
{
    return generateMHTMLData(page, true);
}

PassRefPtr<SharedBuffer> MHTMLArchive::generateMHTMLData(Page* page, bool useBinaryEncoding)
{
    Vector<PageSerializer::Resource> resources;
    PageSerializer pageSerializer(&resources);
    pageSerializer.serialize(page);

    String boundary = generateRandomBoundary();
    String endOfResourceBoundary = makeString("--", boundary, "\r\n");

    GregorianDateTime now;
    now.setToCurrentLocalTime();
    String dateString = makeRFC2822DateString(now.weekDay(), now.monthDay(), now.month(), now.year(), now.hour(), now.minute(), now.second(), now.utcOffset() / 60);

    StringBuilder stringBuilder;
    stringBuilder.append("From: <Saved by WebKit>\r\n");
    stringBuilder.append("Subject: ");
    // We replace non ASCII characters with '?' characters to match IE's behavior.
    stringBuilder.append(replaceNonPrintableCharacters(page->mainFrame()->document()->title()));
    stringBuilder.append("\r\nDate: ");
    stringBuilder.append(dateString);
    stringBuilder.append("\r\nMIME-Version: 1.0\r\n");
    stringBuilder.append("Content-Type: multipart/related;\r\n");
    stringBuilder.append("\ttype=\"");
    stringBuilder.append(page->mainFrame()->document()->suggestedMIMEType());
    stringBuilder.append("\";\r\n");
    stringBuilder.append("\tboundary=\"");
    stringBuilder.append(boundary);
    stringBuilder.append("\"\r\n\r\n");

    // We use utf8() below instead of ascii() as ascii() replaces CRLFs with ?? (we still only have put ASCII characters in it).
    ASSERT(stringBuilder.toString().containsOnlyASCII());
    CString asciiString = stringBuilder.toString().utf8();
    RefPtr<SharedBuffer> mhtmlData = SharedBuffer::create();
    mhtmlData->append(asciiString.data(), asciiString.length());

    for (size_t i = 0; i < resources.size(); ++i) {
        const PageSerializer::Resource& resource = resources[i];

        stringBuilder.clear();
        stringBuilder.append(endOfResourceBoundary);
        stringBuilder.append("Content-Type: ");
        stringBuilder.append(resource.mimeType);

        const char* contentEncoding = 0;
        if (useBinaryEncoding)
            contentEncoding = binary;
        else if (MIMETypeRegistry::isSupportedJavaScriptMIMEType(resource.mimeType) || MIMETypeRegistry::isSupportedNonImageMIMEType(resource.mimeType))
            contentEncoding = quotedPrintable;
        else
            contentEncoding = base64;

        stringBuilder.append("\r\nContent-Transfer-Encoding: ");
        stringBuilder.append(contentEncoding);
        stringBuilder.append("\r\nContent-Location: ");
        stringBuilder.append(resource.url);
        stringBuilder.append("\r\n\r\n");

        asciiString = stringBuilder.toString().utf8();
        mhtmlData->append(asciiString.data(), asciiString.length());

        if (!strcmp(contentEncoding, binary)) {
            const char* data;
            size_t position = 0;
            while (size_t length = resource.data->getSomeData(data, position)) {
                mhtmlData->append(data, length);
                position += length;
            }
        } else {
            // FIXME: ideally we would encode the content as a stream without having to fetch it all.
            const char* data = resource.data->data();
            size_t dataLength = resource.data->size();
            Vector<char> encodedData;
            if (!strcmp(contentEncoding, quotedPrintable)) {
                quotedPrintableEncode(data, dataLength, encodedData);
                mhtmlData->append(encodedData.data(), encodedData.size());
                mhtmlData->append("\r\n", 2);
            } else {
                ASSERT(!strcmp(contentEncoding, base64));
                // We are not specifying insertLFs = true below as it would cut the lines with LFs and MHTML requires CRLFs.
                base64Encode(data, dataLength, encodedData);
                const size_t maximumLineLength = 76;
                size_t index = 0;
                size_t encodedDataLength = encodedData.size();
                do {
                    size_t lineLength = std::min(encodedDataLength - index, maximumLineLength);
                    mhtmlData->append(encodedData.data() + index, lineLength);
                    mhtmlData->append("\r\n", 2);
                    index += maximumLineLength;
                } while (index < encodedDataLength);
            }
        }
    }

    asciiString = makeString("--", boundary, "--\r\n").utf8();
    mhtmlData->append(asciiString.data(), asciiString.length());

    return mhtmlData.release();
}

}
#endif
