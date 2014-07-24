/*
    Copyright (C) 2010 Rob Buis <rwlbuis@gmail.com>
    Copyright (C) 2011 Cosmin Truta <ctruta@gmail.com>
    Copyright (C) 2012 University of Szeged
    Copyright (C) 2012 Renata Hodovan <reni@webkit.org>

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

#if ENABLE(SVG)
#include "CachedSVGDocument.h"

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "ResourceBuffer.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

CachedSVGDocument::CachedSVGDocument(const ResourceRequest& request)
    : CachedResource(request, SVGDocumentResource)
    , m_decoder(TextResourceDecoder::create("application/xml"))
{
    setAccept("image/svg+xml");
}

CachedSVGDocument::~CachedSVGDocument()
{
}

void CachedSVGDocument::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String CachedSVGDocument::encoding() const
{
    return m_decoder->encoding().name();
}

void CachedSVGDocument::finishLoading(ResourceBuffer* data)
{
    if (data) {
        StringBuilder decodedText;
        decodedText.append(m_decoder->decode(data->data(), data->size()));
        decodedText.append(m_decoder->flush());
        // We don't need to create a new frame because the new document belongs to the parent UseElement.
        m_document = SVGDocument::create(0, response().url());
        m_document->setContent(decodedText.toString());
    }
    CachedResource::finishLoading(data);
}

}

#endif

