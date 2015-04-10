/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProxyResolverSoup.h"

#include <libsoup/soup.h>
#include <string.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

static const char defaultNoProxyValue[] = "localhost,127.0.0.1";

typedef struct {
    SoupURI* proxyURI;
    CString noProxy;
    Vector<String> proxyExceptions;
} SoupProxyResolverWkPrivate;

#define SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), SOUP_TYPE_PROXY_RESOLVER_WK, SoupProxyResolverWkPrivate))

static void soup_proxy_resolver_wk_interface_init(SoupProxyURIResolverInterface* proxyResolverInterface);

G_DEFINE_TYPE_EXTENDED(SoupProxyResolverWk, soup_proxy_resolver_wk, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(SOUP_TYPE_SESSION_FEATURE, 0)
                       G_IMPLEMENT_INTERFACE(SOUP_TYPE_PROXY_URI_RESOLVER, soup_proxy_resolver_wk_interface_init))

enum {
    PROP_0,
    PROP_PROXY_URI,
    PROP_NO_PROXY,
    LAST_PROP
};

static void soup_proxy_resolver_wk_init(SoupProxyResolverWk*)
{
}

static void soupProxyResolverWkFinalize(GObject* object)
{
    SoupProxyResolverWkPrivate* priv = SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(object);

    g_clear_pointer(&priv->proxyURI, soup_uri_free);

    G_OBJECT_CLASS(soup_proxy_resolver_wk_parent_class)->finalize(object);
}

static void soupProxyResolverWkSetProperty(GObject* object, uint propID, const GValue* value, GParamSpec* pspec)
{
    SoupProxyResolverWkPrivate* priv = SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(object);

    switch (propID) {
    case PROP_PROXY_URI: {
        SoupURI* uri = static_cast<SoupURI*>(g_value_get_boxed(value));
        if (priv->proxyURI)
            soup_uri_free(priv->proxyURI);

        priv->proxyURI = uri ? soup_uri_copy(uri) : 0;
        break;
    }
    case PROP_NO_PROXY:
        priv->noProxy = g_value_get_string(value);
        priv->proxyExceptions.clear();
        String::fromUTF8(priv->noProxy.data()).replaceWithLiteral(' ', "").split(',', priv->proxyExceptions);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}

static void soupProxyResolverWkGetProperty(GObject* object, uint propID, GValue* value, GParamSpec* pspec)
{
    SoupProxyResolverWkPrivate* priv = SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(object);

    switch (propID) {
    case PROP_PROXY_URI:
        g_value_set_boxed(value, priv->proxyURI);
        break;
    case PROP_NO_PROXY:
        g_value_set_string(value, priv->noProxy.data());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}

static bool shouldBypassProxy(SoupProxyResolverWkPrivate* priv, SoupURI* uri)
{
    const size_t exceptionCount = priv->proxyExceptions.size();
    for (size_t i = 0; i < exceptionCount; ++i) {
        if (String::fromUTF8(uri->host).endsWith(priv->proxyExceptions[i], false))
            return true;
    }

    return false;
}

typedef struct {
    SoupProxyURIResolver* proxyResolver;
    SoupURI* uri;
    SoupProxyURIResolverCallback callback;
    void* userData;
} SoupWkAsyncData;

static gboolean idle_return_proxy_uri(void* data)
{
    SoupWkAsyncData* ssad = static_cast<SoupWkAsyncData*>(data);
    SoupProxyResolverWkPrivate* priv = SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(ssad->proxyResolver);

    SoupURI* proxyURI = 0;
    if (!shouldBypassProxy(priv, ssad->uri))
        proxyURI = priv->proxyURI;

    ssad->callback(ssad->proxyResolver, SOUP_STATUS_OK, proxyURI, ssad->userData);
    g_object_unref(ssad->proxyResolver);
    soup_uri_free(ssad->uri);
    g_slice_free(SoupWkAsyncData, ssad);

    return false;
}

static void soupProxyResolverWkGetProxyURIAsync(SoupProxyURIResolver* proxyResolver, SoupURI* uri, GMainContext* asyncContext, GCancellable*, SoupProxyURIResolverCallback callback, void* userData)
{
    SoupWkAsyncData* ssad;

    ssad = g_slice_new0(SoupWkAsyncData);
    ssad->proxyResolver = SOUP_PROXY_URI_RESOLVER(g_object_ref(proxyResolver));
    ssad->uri = soup_uri_copy(uri);
    ssad->callback = callback;
    ssad->userData = userData;
    soup_add_completion(asyncContext, idle_return_proxy_uri, ssad);
}

static uint soupProxyResolverWkGetProxyURISync(SoupProxyURIResolver* proxyResolver, SoupURI* uri, GCancellable*, SoupURI** proxyURI)
{
    SoupProxyResolverWkPrivate* priv = SOUP_PROXY_RESOLVER_WK_GET_PRIVATE(proxyResolver);

    if (!shouldBypassProxy(priv, uri))
        *proxyURI = soup_uri_copy(priv->proxyURI);

    return SOUP_STATUS_OK;
}

static void soup_proxy_resolver_wk_class_init(SoupProxyResolverWkClass* wkClass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(wkClass);

    g_type_class_add_private(wkClass, sizeof(SoupProxyResolverWkPrivate));

    object_class->set_property = soupProxyResolverWkSetProperty;
    object_class->get_property = soupProxyResolverWkGetProperty;
    object_class->finalize = soupProxyResolverWkFinalize;

    g_object_class_install_property(object_class, PROP_PROXY_URI,
                                    g_param_spec_boxed(SOUP_PROXY_RESOLVER_WK_PROXY_URI,
                                                       "Proxy URI",
                                                       "The HTTP Proxy to use",
                                                       SOUP_TYPE_URI,
                                                       static_cast<GParamFlags>(G_PARAM_READWRITE)));

    g_object_class_install_property(object_class, PROP_NO_PROXY,
                                    g_param_spec_string(SOUP_PROXY_RESOLVER_WK_NO_PROXY,
                                                       "Proxy exceptions",
                                                       "Comma-separated proxy exceptions",
                                                       defaultNoProxyValue,
                                                       static_cast<GParamFlags>(G_PARAM_READWRITE)));
}

static void soup_proxy_resolver_wk_interface_init(SoupProxyURIResolverInterface* proxy_uri_resolver_interface)
{
    proxy_uri_resolver_interface->get_proxy_uri_async = soupProxyResolverWkGetProxyURIAsync;
    proxy_uri_resolver_interface->get_proxy_uri_sync = soupProxyResolverWkGetProxyURISync;
}

SoupProxyURIResolver* soupProxyResolverWkNew(const char* httpProxy, const char* noProxy)
{
    SoupURI* proxyURI = soup_uri_new(httpProxy);
    SoupProxyURIResolver* resolver = SOUP_PROXY_URI_RESOLVER(g_object_new(SOUP_TYPE_PROXY_RESOLVER_WK,
                                                                          SOUP_PROXY_RESOLVER_WK_PROXY_URI, proxyURI,
                                                                          SOUP_PROXY_RESOLVER_WK_NO_PROXY, noProxy ? noProxy : defaultNoProxyValue,
                                                                          0));
    soup_uri_free(proxyURI);

    return resolver;
}
