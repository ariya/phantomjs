/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtWebPageUIClient.h"

#include "WKStringQt.h"
#include "WKURLQt.h"
#include "qquickwebview_p.h"
#include "qquickwebview_p_p.h"
#include "qwebpermissionrequest_p.h"
#include <WKArray.h>
#include <WKHitTestResult.h>
#include <WKOpenPanelParameters.h>
#include <WKOpenPanelResultListener.h>
#include <WKRetainPtr.h>

namespace WebKit {

QtWebPageUIClient::QtWebPageUIClient(WKPageRef pageRef, QQuickWebView* webView)
    : m_webView(webView)
{
    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(WKPageUIClient));
    uiClient.version = kWKPageUIClientCurrentVersion;
    uiClient.clientInfo = this;
    uiClient.runJavaScriptAlert = runJavaScriptAlert;
    uiClient.runJavaScriptConfirm = runJavaScriptConfirm;
    uiClient.runJavaScriptPrompt = runJavaScriptPrompt;
    uiClient.runOpenPanel = runOpenPanel;
    uiClient.mouseDidMoveOverElement = mouseDidMoveOverElement;
    uiClient.exceededDatabaseQuota = exceededDatabaseQuota;
    uiClient.decidePolicyForGeolocationPermissionRequest = policyForGeolocationPermissionRequest;
    uiClient.decidePolicyForNotificationPermissionRequest = policyForNotificationPermissionRequest;
    WKPageSetPageUIClient(pageRef, &uiClient);
}

quint64 QtWebPageUIClient::exceededDatabaseQuota(const QString& databaseName, const QString& displayName, WKSecurityOriginRef securityOrigin, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage)
{
    return m_webView->d_func()->exceededDatabaseQuota(databaseName, displayName, securityOrigin, currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
}

void QtWebPageUIClient::runJavaScriptAlert(const QString& message)
{
    m_webView->d_func()->runJavaScriptAlert(message);
}

bool QtWebPageUIClient::runJavaScriptConfirm(const QString& message)
{
    return m_webView->d_func()->runJavaScriptConfirm(message);
}

QString QtWebPageUIClient::runJavaScriptPrompt(const QString& message, const QString& defaultValue, bool& ok)
{
    return m_webView->d_func()->runJavaScriptPrompt(message, defaultValue, ok);
}

void QtWebPageUIClient::runOpenPanel(WKOpenPanelResultListenerRef listenerRef, const QStringList& selectedFileNames, FileChooserType type)
{
    m_webView->d_func()->chooseFiles(listenerRef, selectedFileNames, type);
}

void QtWebPageUIClient::mouseDidMoveOverElement(const QUrl& linkURL, const QString& linkTitle)
{
    if (linkURL == m_lastHoveredURL && linkTitle == m_lastHoveredTitle)
        return;
    m_lastHoveredURL = linkURL;
    m_lastHoveredTitle = linkTitle;
    emit m_webView->linkHovered(m_lastHoveredURL, m_lastHoveredTitle);
}

void QtWebPageUIClient::permissionRequest(QWebPermissionRequest* request)
{
    request->setParent(m_webView);
    emit m_webView->experimental()->permissionRequested(request);
}

static QtWebPageUIClient* toQtWebPageUIClient(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<QtWebPageUIClient*>(const_cast<void*>(clientInfo));
}

unsigned long long QtWebPageUIClient::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef securityOrigin, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void *clientInfo)
{
    QString qDisplayName = WKStringCopyQString(displayName);
    QString qDatabaseName = WKStringCopyQString(databaseName);
    return toQtWebPageUIClient(clientInfo)->exceededDatabaseQuota(qDatabaseName, qDisplayName, securityOrigin, currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
}

void QtWebPageUIClient::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    QString qAlertText = WKStringCopyQString(alertText);
    toQtWebPageUIClient(clientInfo)->runJavaScriptAlert(qAlertText);
}

bool QtWebPageUIClient::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    QString qMessage = WKStringCopyQString(message);
    return toQtWebPageUIClient(clientInfo)->runJavaScriptConfirm(qMessage);
}

static inline WKStringRef createNullWKString()
{
    RefPtr<WebString> webString = WebString::createNull();
    return toAPI(webString.release().leakRef());
}

WKStringRef QtWebPageUIClient::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    QString qMessage = WKStringCopyQString(message);
    QString qDefaultValue = WKStringCopyQString(defaultValue);
    bool ok = false;
    QString result = toQtWebPageUIClient(clientInfo)->runJavaScriptPrompt(qMessage, qDefaultValue, ok);
    if (!ok)
        return createNullWKString();
    return WKStringCreateWithQString(result);
}

void QtWebPageUIClient::runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    WKRetainPtr<WKArrayRef> wkSelectedFileNames = adoptWK(WKOpenPanelParametersCopySelectedFileNames(parameters));
    QStringList selectedFileNames;
    size_t selectedFilesSize = WKArrayGetSize(wkSelectedFileNames.get());
    selectedFileNames.reserve(selectedFilesSize);

    for (size_t i = 0; i < selectedFilesSize; ++i)
        selectedFileNames += WKStringCopyQString(static_cast<WKStringRef>(WKArrayGetItemAtIndex(wkSelectedFileNames.get(), i)));

    FileChooserType allowMultipleFiles = WKOpenPanelParametersGetAllowsMultipleFiles(parameters) ? MultipleFilesSelection : SingleFileSelection;
    toQtWebPageUIClient(clientInfo)->runOpenPanel(listener, selectedFileNames, allowMultipleFiles);
}

void QtWebPageUIClient::mouseDidMoveOverElement(WKPageRef page, WKHitTestResultRef hitTestResult, WKEventModifiers modifiers, WKTypeRef userData, const void* clientInfo)
{
    const QUrl absoluteLinkUrl = WKURLCopyQUrl(adoptWK(WKHitTestResultCopyAbsoluteLinkURL(hitTestResult)).get());
    const QString linkTitle = WKStringCopyQString(adoptWK(WKHitTestResultCopyLinkTitle(hitTestResult)).get());
    toQtWebPageUIClient(clientInfo)->mouseDidMoveOverElement(absoluteLinkUrl, linkTitle);
}

void QtWebPageUIClient::policyForGeolocationPermissionRequest(WKPageRef page, WKFrameRef frame, WKSecurityOriginRef origin, WKGeolocationPermissionRequestRef request, const void* clientInfo)
{
    if (!request)
        return;

    QWebPermissionRequest* req = QWebPermissionRequest::create(origin, request);
    toQtWebPageUIClient(clientInfo)->permissionRequest(req);
}

void QtWebPageUIClient::policyForNotificationPermissionRequest(WKPageRef page, WKSecurityOriginRef origin, WKNotificationPermissionRequestRef request, const void *clientInfo)
{
    if (!request)
        return;

    QWebPermissionRequest* req = QWebPermissionRequest::create(origin, request);
    toQtWebPageUIClient(clientInfo)->permissionRequest(req);
}

} // namespace WebKit

