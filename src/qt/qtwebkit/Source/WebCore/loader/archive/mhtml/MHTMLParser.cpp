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
#include "MHTMLParser.h"

#include "MHTMLArchive.h"
#include "MIMEHeader.h"
#include "MIMETypeRegistry.h"
#include "QuotedPrintable.h"
#include <wtf/HashMap.h>
#include <wtf/NotFound.h>
#include <wtf/text/Base64.h>

namespace WebCore {

static bool skipLinesUntilBoundaryFound(SharedBufferChunkReader& lineReader, const String& boundary)
{
    String line;
    while (!(line = lineReader.nextChunkAsUTF8StringWithLatin1Fallback()).isNull()) {
        if (line == boundary)
            return true;
    }
    return false;
}

MHTMLParser::MHTMLParser(SharedBuffer* data)
    : m_lineReader(data, "\r\n")
{
}

PassRefPtr<MHTMLArchive> MHTMLParser::parseArchive()
{
    RefPtr<MIMEHeader> header = MIMEHeader::parseHeader(&m_lineReader);
    return parseArchiveWithHeader(header.get());
}

PassRefPtr<MHTMLArchive> MHTMLParser::parseArchiveWithHeader(MIMEHeader* header)
{
    if (!header) {
        LOG_ERROR("Failed to parse MHTML part: no header.");
        return 0;
    }

    RefPtr<MHTMLArchive> archive = MHTMLArchive::create();
    if (!header->isMultipart()) {
        // With IE a page with no resource is not multi-part.
        bool endOfArchiveReached = false;
        RefPtr<ArchiveResource> resource = parseNextPart(*header, String(), String(), endOfArchiveReached);
        if (!resource)
            return 0;
        archive->setMainResource(resource);
        return archive;
    }

    // Skip the message content (it's a generic browser specific message).
    skipLinesUntilBoundaryFound(m_lineReader, header->endOfPartBoundary());

    bool endOfArchive = false;
    while (!endOfArchive) {
        RefPtr<MIMEHeader> resourceHeader = MIMEHeader::parseHeader(&m_lineReader);
        if (!resourceHeader) {
            LOG_ERROR("Failed to parse MHTML, invalid MIME header.");
            return 0;
        }
        if (resourceHeader->contentType() == "multipart/alternative") {
            // Ignore IE nesting which makes little sense (IE seems to nest only some of the frames).
            RefPtr<MHTMLArchive> subframeArchive = parseArchiveWithHeader(resourceHeader.get());
            if (!subframeArchive) {
                LOG_ERROR("Failed to parse MHTML subframe.");
                return 0;
            }
            bool endOfPartReached = skipLinesUntilBoundaryFound(m_lineReader, header->endOfPartBoundary());
            ASSERT_UNUSED(endOfPartReached, endOfPartReached);
            // The top-frame is the first frame found, regardless of the nesting level.
            if (subframeArchive->mainResource())
                addResourceToArchive(subframeArchive->mainResource(), archive.get());
            archive->addSubframeArchive(subframeArchive);
            continue;
        }

        RefPtr<ArchiveResource> resource = parseNextPart(*resourceHeader, header->endOfPartBoundary(), header->endOfDocumentBoundary(), endOfArchive);
        if (!resource) {
            LOG_ERROR("Failed to parse MHTML part.");
            return 0;
        }
        addResourceToArchive(resource.get(), archive.get());
    }

    return archive.release();
}

void MHTMLParser::addResourceToArchive(ArchiveResource* resource, MHTMLArchive* archive)
{
    const String& mimeType = resource->mimeType();
    if (!MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType) || MIMETypeRegistry::isSupportedJavaScriptMIMEType(mimeType) || mimeType == "text/css") {
        m_resources.append(resource);
        return;
    }

    // The first document suitable resource is the main frame.
    if (!archive->mainResource()) {
        archive->setMainResource(resource);
        m_frames.append(archive);
        return;
    }

    RefPtr<MHTMLArchive> subframe = MHTMLArchive::create();
    subframe->setMainResource(resource);
    m_frames.append(subframe);
}

PassRefPtr<ArchiveResource> MHTMLParser::parseNextPart(const MIMEHeader& mimeHeader, const String& endOfPartBoundary, const String& endOfDocumentBoundary, bool& endOfArchiveReached)
{
    ASSERT(endOfPartBoundary.isEmpty() == endOfDocumentBoundary.isEmpty());

    RefPtr<SharedBuffer> content = SharedBuffer::create();
    const bool checkBoundary = !endOfPartBoundary.isEmpty();
    bool endOfPartReached = false;
    if (mimeHeader.contentTransferEncoding() == MIMEHeader::Binary) {
        if (!checkBoundary) {
            LOG_ERROR("Binary contents requires end of part");
            return 0;
        }
        m_lineReader.setSeparator(endOfPartBoundary.utf8().data());
        Vector<char> part;
        if (!m_lineReader.nextChunk(part)) {
            LOG_ERROR("Binary contents requires end of part");
            return 0;
         }
         content->append(part);
         m_lineReader.setSeparator("\r\n");
         Vector<char> nextChars;
         if (m_lineReader.peek(nextChars, 2) != 2) {
             LOG_ERROR("Invalid seperator.");
             return 0;
         }
         endOfPartReached = true;
         ASSERT(nextChars.size() == 2);
         endOfArchiveReached = (nextChars[0] == '-' && nextChars[1] == '-');
         if (!endOfArchiveReached) {
             String line = m_lineReader.nextChunkAsUTF8StringWithLatin1Fallback();
             if (!line.isEmpty()) {
                 LOG_ERROR("No CRLF at end of binary section.");
                 return 0;
             }
         }
    } else {
        String line;
        while (!(line = m_lineReader.nextChunkAsUTF8StringWithLatin1Fallback()).isNull()) {
            endOfArchiveReached = (line == endOfDocumentBoundary);
            if (checkBoundary && (line == endOfPartBoundary || endOfArchiveReached)) {
                endOfPartReached = true;
                break;
            }
            // Note that we use line.utf8() and not line.ascii() as ascii turns special characters (such as tab, line-feed...) into '?'.
            content->append(line.utf8().data(), line.length());
            if (mimeHeader.contentTransferEncoding() == MIMEHeader::QuotedPrintable) {
                // The line reader removes the \r\n, but we need them for the content in this case as the QuotedPrintable decoder expects CR-LF terminated lines.
                content->append("\r\n", 2);
            }
        }
    }
    if (!endOfPartReached && checkBoundary) {
        LOG_ERROR("No bounday found for MHTML part.");
        return 0;
    }

    Vector<char> data;
    switch (mimeHeader.contentTransferEncoding()) {
    case MIMEHeader::Base64:
        if (!base64Decode(content->data(), content->size(), data)) {
            LOG_ERROR("Invalid base64 content for MHTML part.");
            return 0;
        }
        break;
    case MIMEHeader::QuotedPrintable:
        quotedPrintableDecode(content->data(), content->size(), data);
        break;
    case MIMEHeader::SevenBit:
    case MIMEHeader::Binary:
        data.append(content->data(), content->size());
        break;
    default:
        LOG_ERROR("Invalid encoding for MHTML part.");
        return 0;
    }
    RefPtr<SharedBuffer> contentBuffer = SharedBuffer::adoptVector(data);
    // FIXME: the URL in the MIME header could be relative, we should resolve it if it is.
    // The specs mentions 5 ways to resolve a URL: http://tools.ietf.org/html/rfc2557#section-5
    // IE and Firefox (UNMht) seem to generate only absolute URLs.
    KURL location = KURL(KURL(), mimeHeader.contentLocation());
    return ArchiveResource::create(contentBuffer, location, mimeHeader.contentType(), mimeHeader.charset(), String());
}

size_t MHTMLParser::frameCount() const
{
    return m_frames.size();
}

MHTMLArchive* MHTMLParser::frameAt(size_t index) const
{
    return m_frames[index].get();
}

size_t MHTMLParser::subResourceCount() const
{
    return m_resources.size();
}

ArchiveResource* MHTMLParser::subResourceAt(size_t index) const
{
    return m_resources[index].get();
}

}
#endif
