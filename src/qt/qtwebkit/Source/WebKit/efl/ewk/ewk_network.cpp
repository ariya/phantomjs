/*
    Copyright (C) 2011 Samsung Electronics

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
#include "ewk_network.h"

#include "NetworkStateNotifier.h"
#include "ProxyResolverSoup.h"
#include "ResourceHandle.h"
#include "ewk_private.h"
#include <Eina.h>
#include <libsoup/soup.h>
#include <wtf/text/CString.h>

void ewk_network_proxy_uri_set(const char* proxy)
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();

    if (!proxy) {
        ERR("no proxy uri. remove proxy feature in soup.");
        soup_session_remove_feature_by_type(session, SOUP_TYPE_PROXY_URI_RESOLVER);
        return;
    }

    SoupProxyURIResolver* resolverEfl = soupProxyResolverWkNew(proxy, 0);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(resolverEfl));
    g_object_unref(resolverEfl);
}

const char* ewk_network_proxy_uri_get(void)
{
    SoupURI* uri;
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupProxyURIResolver* resolver = SOUP_PROXY_URI_RESOLVER(soup_session_get_feature(session, SOUP_TYPE_PROXY_URI_RESOLVER));
    if (!resolver)
        return 0;

    g_object_get(resolver, SOUP_PROXY_RESOLVER_WK_PROXY_URI, &uri, NULL);

    if (!uri) {
        ERR("no proxy uri");
        return 0;
    }

    WTF::String proxy = soup_uri_to_string(uri, false);
    return eina_stringshare_add(proxy.utf8().data());
}

Eina_Bool ewk_network_tls_certificate_check_get()
{
    bool checkCertificates = false;

    SoupSession* defaultSession = WebCore::ResourceHandle::defaultSession();
    g_object_get(defaultSession, "ssl-strict", &checkCertificates, NULL);

    return checkCertificates;
}

void ewk_network_tls_certificate_check_set(Eina_Bool checkCertificates)
{
    SoupSession* defaultSession = WebCore::ResourceHandle::defaultSession();
    g_object_set(defaultSession, "ssl-strict", checkCertificates, NULL);
}

const char* ewk_network_tls_ca_certificates_path_get()
{
    const char* bundlePath = 0;

    SoupSession* defaultSession = WebCore::ResourceHandle::defaultSession();
    g_object_get(defaultSession, "ssl-ca-file", &bundlePath, NULL);

    return bundlePath;
}

void ewk_network_tls_ca_certificates_path_set(const char* bundlePath)
{
    SoupSession* defaultSession = WebCore::ResourceHandle::defaultSession();
    g_object_set(defaultSession, "ssl-ca-file", bundlePath, NULL);
}

SoupSession* ewk_network_default_soup_session_get()
{
    return WebCore::ResourceHandle::defaultSession();
}
