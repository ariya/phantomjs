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
#include "InjectedBundlePageUIClient.h"

#include "InjectedBundleHitTestResult.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WebGraphicsContext.h"
#include "WebSecurityOrigin.h"
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

void InjectedBundlePageUIClient::willAddMessageToConsole(WebPage* page, const String& message, int32_t lineNumber)
{
    if (m_client.willAddMessageToConsole)
        m_client.willAddMessageToConsole(toAPI(page), toAPI(message.impl()), lineNumber, m_client.clientInfo);
}

void InjectedBundlePageUIClient::willSetStatusbarText(WebPage* page, const String& statusbarText)
{
    if (m_client.willSetStatusbarText)
        m_client.willSetStatusbarText(toAPI(page), toAPI(statusbarText.impl()), m_client.clientInfo);
}

void InjectedBundlePageUIClient::willRunJavaScriptAlert(WebPage* page, const String& alertText, WebFrame* frame)
{
    if (m_client.willRunJavaScriptAlert)
        m_client.willRunJavaScriptAlert(toAPI(page), toAPI(alertText.impl()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageUIClient::willRunJavaScriptConfirm(WebPage* page, const String& message, WebFrame* frame)
{
    if (m_client.willRunJavaScriptConfirm)
        m_client.willRunJavaScriptConfirm(toAPI(page), toAPI(message.impl()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageUIClient::willRunJavaScriptPrompt(WebPage* page, const String& message, const String& defaultValue, WebFrame* frame)
{
    if (m_client.willRunJavaScriptPrompt)
        m_client.willRunJavaScriptPrompt(toAPI(page), toAPI(message.impl()), toAPI(defaultValue.impl()), toAPI(frame), m_client.clientInfo);
}

void InjectedBundlePageUIClient::mouseDidMoveOverElement(WebPage* page, const HitTestResult& coreHitTestResult, WebEvent::Modifiers modifiers, RefPtr<APIObject>& userData)
{
    if (!m_client.mouseDidMoveOverElement)
        return;

    RefPtr<InjectedBundleHitTestResult> hitTestResult = InjectedBundleHitTestResult::create(coreHitTestResult);

    WKTypeRef userDataToPass = 0;
    m_client.mouseDidMoveOverElement(toAPI(page), toAPI(hitTestResult.get()), toAPI(modifiers), &userDataToPass, m_client.clientInfo);
    userData = adoptRef(toImpl(userDataToPass));
}

void InjectedBundlePageUIClient::pageDidScroll(WebPage* page)
{
    if (!m_client.pageDidScroll)
        return;

    m_client.pageDidScroll(toAPI(page), m_client.clientInfo);
}

bool InjectedBundlePageUIClient::shouldPaintCustomOverhangArea()
{
    return m_client.paintCustomOverhangArea;
}

void InjectedBundlePageUIClient::paintCustomOverhangArea(WebPage* page, GraphicsContext* graphicsContext, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect)
{
    ASSERT(shouldPaintCustomOverhangArea());

    RefPtr<WebGraphicsContext> context = WebGraphicsContext::create(graphicsContext);
    m_client.paintCustomOverhangArea(toAPI(page), toAPI(context.get()), toAPI(horizontalOverhangArea), toAPI(verticalOverhangArea), toAPI(dirtyRect), m_client.clientInfo);
}

String InjectedBundlePageUIClient::shouldGenerateFileForUpload(WebPage* page, const String& originalFilePath)
{
    if (!m_client.shouldGenerateFileForUpload)
        return String();
    RefPtr<WebString> generatedFilePath = adoptRef(toImpl(m_client.shouldGenerateFileForUpload(toAPI(page), toAPI(originalFilePath.impl()), m_client.clientInfo)));
    return generatedFilePath ? generatedFilePath->string() : String();
}

String InjectedBundlePageUIClient::generateFileForUpload(WebPage* page, const String& originalFilePath)
{
    if (!m_client.shouldGenerateFileForUpload)
        return String();
    RefPtr<WebString> generatedFilePath = adoptRef(toImpl(m_client.generateFileForUpload(toAPI(page), toAPI(originalFilePath.impl()), m_client.clientInfo)));
    return generatedFilePath ? generatedFilePath->string() : String();
}

bool InjectedBundlePageUIClient::shouldRubberBandInDirection(WebPage* page, WKScrollDirection direction) const
{
    if (!m_client.shouldRubberBandInDirection)
        return true;
    return m_client.shouldRubberBandInDirection(toAPI(page), direction, m_client.clientInfo);
}
    
WKBundlePageUIElementVisibility InjectedBundlePageUIClient::statusBarIsVisible(WebPage* page)
{
    if (!m_client.statusBarIsVisible)
        return WKBundlePageUIElementVisibilityUnknown;
    
    return m_client.statusBarIsVisible(toAPI(page), m_client.clientInfo);
}

WKBundlePageUIElementVisibility InjectedBundlePageUIClient::menuBarIsVisible(WebPage* page)
{
    if (!m_client.menuBarIsVisible)
        return WKBundlePageUIElementVisibilityUnknown;
    
    return m_client.menuBarIsVisible(toAPI(page), m_client.clientInfo);
}

WKBundlePageUIElementVisibility InjectedBundlePageUIClient::toolbarsAreVisible(WebPage* page)
{
    if (!m_client.toolbarsAreVisible)
        return WKBundlePageUIElementVisibilityUnknown;
    
    return m_client.toolbarsAreVisible(toAPI(page), m_client.clientInfo);
}

void InjectedBundlePageUIClient::didReachApplicationCacheOriginQuota(WebPage* page, WebSecurityOrigin* origin, int64_t totalBytesNeeded)
{
    if (!m_client.didReachApplicationCacheOriginQuota)
        return;

    m_client.didReachApplicationCacheOriginQuota(toAPI(page), toAPI(origin), totalBytesNeeded, m_client.clientInfo);
}

uint64_t InjectedBundlePageUIClient::didExceedDatabaseQuota(WebPage* page, WebSecurityOrigin* origin, const String& databaseName, const String& databaseDisplayName, uint64_t currentQuotaBytes, uint64_t currentOriginUsageBytes, uint64_t currentDatabaseUsageBytes, uint64_t expectedUsageBytes)
{
    if (!m_client.didExceedDatabaseQuota)
        return 0;

    return m_client.didExceedDatabaseQuota(toAPI(page), toAPI(origin), toAPI(databaseName.impl()), toAPI(databaseDisplayName.impl()), currentQuotaBytes, currentOriginUsageBytes, currentDatabaseUsageBytes, expectedUsageBytes, m_client.clientInfo);
}

String InjectedBundlePageUIClient::plugInStartLabelTitle(const String& mimeType) const
{
    if (!m_client.createPlugInStartLabelTitle)
        return String();

    RefPtr<WebString> title = adoptRef(toImpl(m_client.createPlugInStartLabelTitle(toAPI(mimeType.impl()), m_client.clientInfo)));
    return title ? title->string() : String();
}

String InjectedBundlePageUIClient::plugInStartLabelSubtitle(const String& mimeType) const
{
    if (!m_client.createPlugInStartLabelSubtitle)
        return String();

    RefPtr<WebString> subtitle = adoptRef(toImpl(m_client.createPlugInStartLabelSubtitle(toAPI(mimeType.impl()), m_client.clientInfo)));
    return subtitle ? subtitle->string() : String();
}

String InjectedBundlePageUIClient::plugInExtraStyleSheet() const
{
    if (!m_client.createPlugInExtraStyleSheet)
        return String();

    RefPtr<WebString> styleSheet = adoptRef(toImpl(m_client.createPlugInExtraStyleSheet(m_client.clientInfo)));
    return styleSheet ? styleSheet->string() : String();
}

String InjectedBundlePageUIClient::plugInExtraScript() const
{
    if (!m_client.createPlugInExtraScript)
        return String();

    RefPtr<WebString> script = adoptRef(toImpl(m_client.createPlugInExtraScript(m_client.clientInfo)));
    return script ? script->string() : String();
}

} // namespace WebKit
