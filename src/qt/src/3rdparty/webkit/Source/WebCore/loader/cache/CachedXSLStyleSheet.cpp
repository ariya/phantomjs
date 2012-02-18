/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedXSLStyleSheet.h"

#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/Vector.h>

namespace WebCore {

#if ENABLE(XSLT)

CachedXSLStyleSheet::CachedXSLStyleSheet(const String &url)
    : CachedResource(url, XSLStyleSheet)
    , m_decoder(TextResourceDecoder::create("text/xsl"))
{
    // It's XML we want.
    // FIXME: This should accept more general xml formats */*+xml, image/svg+xml for example.
    setAccept("text/xml, application/xml, application/xhtml+xml, text/xsl, application/rss+xml, application/atom+xml");
}

void CachedXSLStyleSheet::didAddClient(CachedResourceClient* c)
{  
    if (!isLoading())
        c->setXSLStyleSheet(m_url, m_response.url(), m_sheet);
}

void CachedXSLStyleSheet::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String CachedXSLStyleSheet::encoding() const
{
    return m_decoder->encoding().name();
}

void CachedXSLStyleSheet::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    if (!allDataReceived)
        return;

    m_data = data;     
    setEncodedSize(m_data.get() ? m_data->size() : 0);
    if (m_data.get()) {
        m_sheet = String(m_decoder->decode(m_data->data(), encodedSize()));
        m_sheet += m_decoder->flush();
    }
    setLoading(false);
    checkNotify();
}

void CachedXSLStyleSheet::checkNotify()
{
    if (isLoading())
        return;
    
    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient *c = w.next())
        c->setXSLStyleSheet(m_url, m_response.url(), m_sheet);
}

void CachedXSLStyleSheet::error(CachedResource::Status status)
{
    setStatus(status);
    ASSERT(errorOccurred());
    setLoading(false);
    checkNotify();
}

#endif

}
