/*
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
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
#include "TextCodecMac.h"

#include "CharsetData.h"
#include "PlatformString.h"
#include "ThreadGlobalData.h"
#include <wtf/Assertions.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

// We need to keep this because ICU doesn't support some of the encodings that we need:
// <http://bugs.webkit.org/show_bug.cgi?id=4195>.

const size_t ConversionBufferSize = 16384;

static TECConverterWrapper& cachedConverterTEC()
{
    return threadGlobalData().cachedConverterTEC();
}

void TextCodecMac::registerEncodingNames(EncodingNameRegistrar registrar)
{
    TECTextEncodingID lastEncoding = invalidEncoding;
    const char* lastName = 0;

    for (size_t i = 0; CharsetTable[i].name; ++i) {
        if (CharsetTable[i].encoding != lastEncoding) {
            lastEncoding = CharsetTable[i].encoding;
            lastName = CharsetTable[i].name;
        }
        registrar(CharsetTable[i].name, lastName);
    }
}

static PassOwnPtr<TextCodec> newTextCodecMac(const TextEncoding&, const void* additionalData)
{
    return adoptPtr(new TextCodecMac(*static_cast<const TECTextEncodingID*>(additionalData)));
}

void TextCodecMac::registerCodecs(TextCodecRegistrar registrar)
{
    TECTextEncodingID lastEncoding = invalidEncoding;

    for (size_t i = 0; CharsetTable[i].name; ++i)
        if (CharsetTable[i].encoding != lastEncoding) {
            registrar(CharsetTable[i].name, newTextCodecMac, &CharsetTable[i].encoding);
            lastEncoding = CharsetTable[i].encoding;
        }
}

TextCodecMac::TextCodecMac(TECTextEncodingID encoding)
    : m_encoding(encoding)
    , m_numBufferedBytes(0)
    , m_converterTEC(0)
{
}

TextCodecMac::~TextCodecMac()
{
    releaseTECConverter();
}

void TextCodecMac::releaseTECConverter() const
{
    if (m_converterTEC) {
        TECConverterWrapper& cachedConverter = cachedConverterTEC();
        if (cachedConverter.converter)
            TECDisposeConverter(cachedConverter.converter);
        cachedConverter.converter = m_converterTEC;
        cachedConverter.encoding = m_encoding;
        m_converterTEC = 0;
    }
}

OSStatus TextCodecMac::createTECConverter() const
{
    TECConverterWrapper& cachedConverter = cachedConverterTEC();

    bool cachedEncodingEqual = cachedConverter.encoding == m_encoding;
    cachedConverter.encoding = invalidEncoding;

    if (cachedEncodingEqual && cachedConverter.converter) {
        m_converterTEC = cachedConverter.converter;
        cachedConverter.converter = 0;

        TECClearConverterContextInfo(m_converterTEC);
    } else {
        OSStatus status = TECCreateConverter(&m_converterTEC, m_encoding,
            CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicode16BitFormat));
        if (status)
            return status;

        TECSetBasicOptions(m_converterTEC, kUnicodeForceASCIIRangeMask);
    }

    return noErr;
}

OSStatus TextCodecMac::decode(const unsigned char* inputBuffer, int inputBufferLength, int& inputLength,
    void *outputBuffer, int outputBufferLength, int& outputLength)
{
    OSStatus status;
    unsigned long bytesRead = 0;
    unsigned long bytesWritten = 0;

    if (m_numBufferedBytes != 0) {
        // Finish converting a partial character that's in our buffer.
        
        // First, fill the partial character buffer with as many bytes as are available.
        ASSERT(m_numBufferedBytes < sizeof(m_bufferedBytes));
        const int spaceInBuffer = sizeof(m_bufferedBytes) - m_numBufferedBytes;
        const int bytesToPutInBuffer = min(spaceInBuffer, inputBufferLength);
        ASSERT(bytesToPutInBuffer != 0);
        memcpy(m_bufferedBytes + m_numBufferedBytes, inputBuffer, bytesToPutInBuffer);

        // Now, do a conversion on the buffer.
        status = TECConvertText(m_converterTEC, m_bufferedBytes, m_numBufferedBytes + bytesToPutInBuffer, &bytesRead,
            reinterpret_cast<unsigned char*>(outputBuffer), outputBufferLength, &bytesWritten);
        ASSERT(bytesRead <= m_numBufferedBytes + bytesToPutInBuffer);

        if (status == kTECPartialCharErr && bytesRead == 0) {
            // Handle the case where the partial character was not converted.
            if (bytesToPutInBuffer >= spaceInBuffer) {
                LOG_ERROR("TECConvertText gave a kTECPartialCharErr but read none of the %zu bytes in the buffer", sizeof(m_bufferedBytes));
                m_numBufferedBytes = 0;
                status = kTECUnmappableElementErr; // should never happen, but use this error code
            } else {
                // Tell the caller we read all the source bytes and keep them in the buffer.
                m_numBufferedBytes += bytesToPutInBuffer;
                bytesRead = bytesToPutInBuffer;
                status = noErr;
            }
        } else {
            // We are done with the partial character buffer.
            // Also, we have read some of the bytes from the main buffer.
            if (bytesRead > m_numBufferedBytes) {
                bytesRead -= m_numBufferedBytes;
            } else {
                LOG_ERROR("TECConvertText accepted some bytes it previously rejected with kTECPartialCharErr");
                bytesRead = 0;
            }
            m_numBufferedBytes = 0;
            if (status == kTECPartialCharErr) {
                // While there may be a partial character problem in the small buffer,
                // we have to try again and not get confused and think there is a partial
                // character problem in the large buffer.
                status = noErr;
            }
        }
    } else {
        status = TECConvertText(m_converterTEC, inputBuffer, inputBufferLength, &bytesRead,
            static_cast<unsigned char*>(outputBuffer), outputBufferLength, &bytesWritten);
        ASSERT(static_cast<int>(bytesRead) <= inputBufferLength);
    }

    // Work around bug 3351093, where sometimes we get kTECBufferBelowMinimumSizeErr instead of kTECOutputBufferFullStatus.
    if (status == kTECBufferBelowMinimumSizeErr && bytesWritten != 0)
        status = kTECOutputBufferFullStatus;

    inputLength = bytesRead;
    outputLength = bytesWritten;
    return status;
}

String TextCodecMac::decode(const char* bytes, size_t length, bool flush, bool stopOnError, bool& sawError)
{
    // Get a converter for the passed-in encoding.
    if (!m_converterTEC && createTECConverter() != noErr)
        return String();
    
    Vector<UChar> result;

    const unsigned char* sourcePointer = reinterpret_cast<const unsigned char*>(bytes);
    int sourceLength = length;
    bool bufferWasFull = false;
    UniChar buffer[ConversionBufferSize];

    while ((sourceLength || bufferWasFull) && !sawError) {
        int bytesRead = 0;
        int bytesWritten = 0;
        OSStatus status = decode(sourcePointer, sourceLength, bytesRead, buffer, sizeof(buffer), bytesWritten);
        ASSERT(bytesRead <= sourceLength);
        sourcePointer += bytesRead;
        sourceLength -= bytesRead;
        
        switch (status) {
            case noErr:
            case kTECOutputBufferFullStatus:
                break;
            case kTextMalformedInputErr:
            case kTextUndefinedElementErr:
                // FIXME: Put FFFD character into the output string in this case?
                TECClearConverterContextInfo(m_converterTEC);
                if (stopOnError) {
                    sawError = true;
                    break;
                }
                if (sourceLength) {
                    sourcePointer += 1;
                    sourceLength -= 1;
                }
                break;
            case kTECPartialCharErr: {
                // Put the partial character into the buffer.
                ASSERT(m_numBufferedBytes == 0);
                const int bufferSize = sizeof(m_numBufferedBytes);
                if (sourceLength < bufferSize) {
                    memcpy(m_bufferedBytes, sourcePointer, sourceLength);
                    m_numBufferedBytes = sourceLength;
                } else {
                    LOG_ERROR("TECConvertText gave a kTECPartialCharErr, but left %u bytes in the buffer", sourceLength);
                }
                sourceLength = 0;
                break;
            }
            default:
                sawError = true;
                return String();
        }

        ASSERT(!(bytesWritten % sizeof(UChar)));
        result.append(buffer, bytesWritten / sizeof(UChar));

        bufferWasFull = status == kTECOutputBufferFullStatus;
    }
    
    if (flush) {
        unsigned long bytesWritten = 0;
        TECFlushText(m_converterTEC, reinterpret_cast<unsigned char*>(buffer), sizeof(buffer), &bytesWritten);
        ASSERT(!(bytesWritten % sizeof(UChar)));
        result.append(buffer, bytesWritten / sizeof(UChar));
    }

    String resultString = String::adopt(result);

    // <rdar://problem/3225472>
    // Simplified Chinese pages use the code A3A0 to mean "full-width space".
    // But GB18030 decodes it to U+E5E5, which is correct in theory but not in practice.
    // To work around, just change all occurences of U+E5E5 to U+3000 (ideographic space).
    if (m_encoding == kCFStringEncodingGB_18030_2000)
        resultString.replace(0xE5E5, ideographicSpace);
    
    return resultString;
}

CString TextCodecMac::encode(const UChar* characters, size_t length, UnencodableHandling handling)
{
    // FIXME: We should really use TEC here instead of CFString for consistency with the other direction.

    // FIXME: Since there's no "force ASCII range" mode in CFString, we change the backslash into a yen sign.
    // Encoding will change the yen sign back into a backslash.
    String copy(characters, length);
    copy.replace('\\', m_backslashAsCurrencySymbol);
    RetainPtr<CFStringRef> cfs(AdoptCF, copy.createCFString());

    CFIndex startPos = 0;
    CFIndex charactersLeft = CFStringGetLength(cfs.get());
    Vector<char> result;
    size_t size = 0;
    UInt8 lossByte = handling == QuestionMarksForUnencodables ? '?' : 0;
    while (charactersLeft > 0) {
        CFRange range = CFRangeMake(startPos, charactersLeft);
        CFIndex bufferLength;
        CFStringGetBytes(cfs.get(), range, m_encoding, lossByte, false, NULL, 0x7FFFFFFF, &bufferLength);

        result.grow(size + bufferLength);
        unsigned char* buffer = reinterpret_cast<unsigned char*>(result.data() + size);
        CFIndex charactersConverted = CFStringGetBytes(cfs.get(), range, m_encoding, lossByte, false, buffer, bufferLength, &bufferLength);
        size += bufferLength;

        if (charactersConverted != charactersLeft) {
            unsigned badChar = CFStringGetCharacterAtIndex(cfs.get(), startPos + charactersConverted);
            ++charactersConverted;
            if ((badChar & 0xFC00) == 0xD800 && charactersConverted != charactersLeft) { // is high surrogate
                UniChar low = CFStringGetCharacterAtIndex(cfs.get(), startPos + charactersConverted);
                if ((low & 0xFC00) == 0xDC00) { // is low surrogate
                    badChar <<= 10;
                    badChar += low;
                    badChar += 0x10000 - (0xD800 << 10) - 0xDC00;
                    ++charactersConverted;
                }
            }
            UnencodableReplacementArray entity;
            int entityLength = getUnencodableReplacement(badChar, handling, entity);
            result.grow(size + entityLength);
            memcpy(result.data() + size, entity, entityLength);
            size += entityLength;
        }

        startPos += charactersConverted;
        charactersLeft -= charactersConverted;
    }
    return CString(result.data(), size);
}

} // namespace WebCore
