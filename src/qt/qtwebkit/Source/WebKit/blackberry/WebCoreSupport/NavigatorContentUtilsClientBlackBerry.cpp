/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "NavigatorContentUtilsClientBlackBerry.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include "WebPage_p.h"

namespace WebCore {

NavigatorContentUtilsClientBlackBerry::NavigatorContentUtilsClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
}

void NavigatorContentUtilsClientBlackBerry::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    m_webPagePrivate->m_client->registerProtocolHandler(scheme, baseURL, url, title);
}


#if ENABLE(CUSTOM_SCHEME_HANDLER)
NavigatorContentUtilsClient::CustomHandlersState NavigatorContentUtilsClientBlackBerry::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url)
{
    return static_cast<CustomHandlersState>(m_webPagePrivate->m_client->isProtocolHandlerRegistered(scheme, baseURL, url));
}

void NavigatorContentUtilsClientBlackBerry::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    m_webPagePrivate->m_client->unregisterProtocolHandler(scheme, baseURL, url);
}
#endif

}

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
