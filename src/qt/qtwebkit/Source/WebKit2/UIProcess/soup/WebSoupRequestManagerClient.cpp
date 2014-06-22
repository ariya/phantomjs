/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebSoupRequestManagerClient.h"

#include "WKAPICast.h"

namespace WebKit {

bool WebSoupRequestManagerClient::didReceiveURIRequest(WebSoupRequestManagerProxy* soupRequestManager, WebURL* url, WebPageProxy* initiaingPage, uint64_t requestID)
{
    if (!m_client.didReceiveURIRequest)
        return false;

    m_client.didReceiveURIRequest(toAPI(soupRequestManager), toAPI(url), toAPI(initiaingPage), requestID, m_client.clientInfo);
    return true;
}

void WebSoupRequestManagerClient::didFailToLoadURIRequest(WebSoupRequestManagerProxy* soupRequestManager, uint64_t requestID)
{
    if (m_client.didFailToLoadURIRequest)
        m_client.didFailToLoadURIRequest(toAPI(soupRequestManager), requestID, m_client.clientInfo);
}

} // namespace WebKit
