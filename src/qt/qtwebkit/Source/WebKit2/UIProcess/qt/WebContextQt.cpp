/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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
#include "WebContext.h"

#include "ApplicationCacheStorage.h"
#include "WKSharedAPICast.h"
#include "WebProcessCreationParameters.h"
#include <QProcess>

#if ENABLE(GEOLOCATION)
#include "WebGeolocationManagerProxy.h"
#include "WebGeolocationProviderQt.h"
#endif

namespace WebKit {

String WebContext::platformDefaultApplicationCacheDirectory() const
{
    const String cacheDirectory = WebCore::cacheStorage().cacheDirectory();

    if (cacheDirectory.isEmpty())
        return diskCacheDirectory();

    return cacheDirectory;
}

void WebContext::platformInitializeWebProcess(WebProcessCreationParameters& parameters)
{
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
#if ENABLE(GEOLOCATION) && HAVE(QTLOCATION)
    static WebGeolocationProviderQt* location = WebGeolocationProviderQt::create(toAPI(supplement<WebGeolocationManagerProxy>()));
    WKGeolocationManagerSetProvider(toAPI(supplement<WebGeolocationManagerProxy>()), WebGeolocationProviderQt::provider(location));
#endif
}

void WebContext::platformInvalidateContext()
{
}

String WebContext::platformDefaultDatabaseDirectory() const
{
    return String();
}

String WebContext::platformDefaultIconDatabasePath() const
{
    return String();
}

String WebContext::platformDefaultLocalStorageDirectory() const
{
    return String();
}

String WebContext::platformDefaultDiskCacheDirectory() const
{
    return String();
}

String WebContext::platformDefaultCookieStorageDirectory() const
{
    return String();
}

} // namespace WebKit
