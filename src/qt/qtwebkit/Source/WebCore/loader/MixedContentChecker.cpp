/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MixedContentChecker.h"

#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

MixedContentChecker::MixedContentChecker(Frame* frame)
    : m_frame(frame)
{
}

FrameLoaderClient* MixedContentChecker::client() const
{
    return m_frame->loader()->client();
}

// static
bool MixedContentChecker::isMixedContent(SecurityOrigin* securityOrigin, const KURL& url)
{
    if (securityOrigin->protocol() != "https")
        return false; // We only care about HTTPS security origins.

    // We're in a secure context, so |url| is mixed content if it's insecure.
    return !SecurityOrigin::isSecure(url);
}

bool MixedContentChecker::canDisplayInsecureContent(SecurityOrigin* securityOrigin, const KURL& url) const
{
    if (!isMixedContent(securityOrigin, url))
        return true;

    Settings* settings = m_frame->settings();
    bool allowed = client()->allowDisplayingInsecureContent(settings && settings->allowDisplayOfInsecureContent(), securityOrigin, url);
    logWarning(allowed, "displayed", url);

    if (allowed)
        client()->didDisplayInsecureContent();

    return allowed;
}

bool MixedContentChecker::canRunInsecureContent(SecurityOrigin* securityOrigin, const KURL& url) const
{
    if (!isMixedContent(securityOrigin, url))
        return true;

    Settings* settings = m_frame->settings();
    bool allowed = client()->allowRunningInsecureContent(settings && settings->allowRunningOfInsecureContent(), securityOrigin, url);
    logWarning(allowed, "ran", url);

    if (allowed)
        client()->didRunInsecureContent(securityOrigin, url);

    return allowed;
}

void MixedContentChecker::logWarning(bool allowed, const String& action, const KURL& target) const
{
    String message = makeString((allowed ? "" : "[blocked] "), "The page at ", m_frame->document()->url().stringCenterEllipsizedToLength(), " ", action, " insecure content from ", target.stringCenterEllipsizedToLength(), ".\n");
    m_frame->document()->addConsoleMessage(SecurityMessageSource, WarningMessageLevel, message);
}

} // namespace WebCore
