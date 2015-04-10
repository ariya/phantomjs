/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
#include "WebProcessMainGtk.h"

#include "WKBase.h"
#include "WebKit2Initialize.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/NetworkingContext.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/RunLoop.h>
#include <WebKit2/WebProcess.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <libsoup/soup.h>
#include <unistd.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

using namespace WebCore;

namespace WebKit {

WK_EXPORT int WebProcessMainGtk(int argc, char* argv[])
{
    ASSERT(argc == 2);

#ifndef NDEBUG
    if (g_getenv("WEBKIT2_PAUSE_WEB_PROCESS_ON_LAUNCH"))
        sleep(30);
#endif

    gtk_init(&argc, &argv);

    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    InitializeWebKit2();

    int socket = atoi(argv[1]);

    ChildProcessInitializationParameters parameters;
    parameters.connectionIdentifier = socket;

    WebProcess::shared().initialize(parameters);

    // Despite using system CAs to validate certificates we're
    // accepting invalid certificates by default. New API will be
    // added later to let client accept/discard invalid certificates.
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    g_object_set(session, SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
                 SOUP_SESSION_SSL_STRICT, FALSE, NULL);

    RunLoop::run();

    if (SoupSessionFeature* soupCache = soup_session_get_feature(session, SOUP_TYPE_CACHE)) {
        soup_cache_flush(SOUP_CACHE(soupCache));
        soup_cache_dump(SOUP_CACHE(soupCache));
    }

    return 0;
}

} // namespace WebKit
