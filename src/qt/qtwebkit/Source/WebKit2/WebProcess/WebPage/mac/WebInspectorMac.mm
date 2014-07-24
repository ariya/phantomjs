/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebInspector.h"

#import <WebCore/SoftLinking.h>

SOFT_LINK_STAGED_FRAMEWORK(WebInspectorUI, PrivateFrameworks, A)

namespace WebKit {

static bool inspectorReallyUsesWebKitUserInterface(bool preference)
{
    // This matches a similar check in WebInspectorProxyMac.mm. Keep them in sync.

    // Call the soft link framework function to dlopen it, then [NSBundle bundleWithIdentifier:] will work.
    WebInspectorUILibrary();

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebInspectorUI"] pathForResource:@"Main" ofType:@"html"])
        return true;

    if (![[NSBundle bundleWithIdentifier:@"com.apple.WebCore"] pathForResource:@"inspector" ofType:@"html" inDirectory:@"inspector"])
        return false;

    return preference;
}

void WebInspector::setInspectorUsesWebKitUserInterface(bool flag)
{
    if (m_usesWebKitUserInterface == flag)
        return;

    m_usesWebKitUserInterface = flag;
    m_hasLocalizedStringsURL = false;
}

bool WebInspector::canSave() const
{
    return true;
}

String WebInspector::localizedStringsURL() const
{
    if (!m_hasLocalizedStringsURL) {
        NSString *bundleIdentifier = inspectorReallyUsesWebKitUserInterface(m_usesWebKitUserInterface) ? @"com.apple.WebCore" : @"com.apple.WebInspectorUI";
        NSString *path = [[NSBundle bundleWithIdentifier:bundleIdentifier] pathForResource:@"localizedStrings" ofType:@"js"];
        if ([path length])
            m_localizedStringsURL = [[NSURL fileURLWithPath:path] absoluteString];
        else
            m_localizedStringsURL = String();
        m_hasLocalizedStringsURL = true;
    }
    return m_localizedStringsURL;
}

} // namespace WebKit
