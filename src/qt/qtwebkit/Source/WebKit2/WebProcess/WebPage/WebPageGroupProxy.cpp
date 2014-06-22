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

#include "config.h"
#include "WebPageGroupProxy.h"

#include "WebProcess.h"
#include "InjectedBundle.h"
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/PageGroup.h>

namespace WebKit {

PassRefPtr<WebPageGroupProxy> WebPageGroupProxy::create(const WebPageGroupData& data)
{
    RefPtr<WebPageGroupProxy> pageGroup = adoptRef(new WebPageGroupProxy(data));
    
    if (pageGroup->isVisibleToInjectedBundle() && WebProcess::shared().injectedBundle())
        WebProcess::shared().injectedBundle()->didInitializePageGroup(pageGroup.get());

    return pageGroup.release();
}

WebPageGroupProxy::~WebPageGroupProxy()
{
}

WebPageGroupProxy::WebPageGroupProxy(const WebPageGroupData& data)
    : m_data(data)
    , m_pageGroup(WebCore::PageGroup::pageGroup(m_data.identifer))
{
    for (size_t i = 0; i < data.userStyleSheets.size(); ++i)
        addUserStyleSheet(data.userStyleSheets[i]);
    for (size_t i = 0; i < data.userScripts.size(); ++i)
        addUserScript(data.userScripts[i]);
}

void WebPageGroupProxy::addUserStyleSheet(const WebCore::UserStyleSheet& userStyleSheet)
{
    m_pageGroup->addUserStyleSheetToWorld(WebCore::mainThreadNormalWorld(), userStyleSheet.source(), userStyleSheet.url(), userStyleSheet.whitelist(), userStyleSheet.blacklist(), userStyleSheet.injectedFrames(), userStyleSheet.level());
}

void WebPageGroupProxy::addUserScript(const WebCore::UserScript& userScript)
{
    m_pageGroup->addUserScriptToWorld(WebCore::mainThreadNormalWorld(), userScript.source(), userScript.url(), userScript.whitelist(), userScript.blacklist(), userScript.injectionTime(), userScript.injectedFrames());
}

void WebPageGroupProxy::removeAllUserStyleSheets()
{
    m_pageGroup->removeUserStyleSheetsFromWorld(WebCore::mainThreadNormalWorld());
}

void WebPageGroupProxy::removeAllUserScripts()
{
    m_pageGroup->removeUserScriptsFromWorld(WebCore::mainThreadNormalWorld());
}

void WebPageGroupProxy::removeAllUserContent()
{
    m_pageGroup->removeAllUserContent();
}

} // namespace WebKit
