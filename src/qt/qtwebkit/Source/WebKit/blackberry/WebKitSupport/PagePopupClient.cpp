/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PagePopupClient.h"

#include "DocumentWriter.h"
#include "IntRect.h"
#include "WebPage_p.h"
#include <BlackBerryPlatformString.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

PagePopupClient::PagePopupClient(WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
}

void PagePopupClient::closePopup()
{
    ASSERT(m_webPagePrivate);
    m_webPagePrivate->closePagePopup();
}

void PagePopupClient::didClosePopup()
{
    m_webPagePrivate = 0;
}

IntSize PagePopupClient::contentSize()
{
    // FIXME: will generate content size dynamically.
    return IntSize(320, 256);
}

void PagePopupClient::writeDocument(DocumentWriter& writer)
{
    writer.addData(m_source.utf8().data(), m_source.utf8().length());
}

}
}

