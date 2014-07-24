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

#ifndef ProxyResolverSoup_h
#define ProxyResolverSoup_h

#include <libsoup/soup.h>

G_BEGIN_DECLS

#define SOUP_TYPE_PROXY_RESOLVER_WK               (soup_proxy_resolver_wk_get_type ())
#define SOUP_PROXY_RESOLVER_WK(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), SOUP_TYPE_PROXY_RESOLVER_WK, SoupProxyResolverWk))
#define SOUP_PROXY_RESOLVER_WK_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), SOUP_TYPE_PROXY_RESOLVER_WK, SoupProxyResolverWkClass))
#define SOUP_IS_PROXY_RESOLVER_WK(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), SOUP_TYPE_PROXY_RESOLVER_WK))
#define SOUP_IS_PROXY_RESOLVER_WK_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), SOUP_TYPE_PROXY_RESOLVER_WK))
#define SOUP_PROXY_RESOLVER_WK_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUP_TYPE_PROXY_RESOLVER_WK, SoupProxyResolverWkClass))

static const char SOUP_PROXY_RESOLVER_WK_PROXY_URI[] = "proxy-uri";
static const char SOUP_PROXY_RESOLVER_WK_NO_PROXY[] = "no-proxy";

typedef struct {
    GObject parent;
} SoupProxyResolverWk;

typedef struct {
    GObjectClass parent_class;
} SoupProxyResolverWkClass;

GType soup_proxy_resolver_wk_get_type(void);

SoupProxyURIResolver* soupProxyResolverWkNew(const char* httpProxy, const char* noProxy);

G_END_DECLS

#endif // ProxyResolverSoup_h
