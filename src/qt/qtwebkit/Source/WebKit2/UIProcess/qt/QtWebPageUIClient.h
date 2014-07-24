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

#ifndef QtWebPageUIClient_h
#define QtWebPageUIClient_h

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <WKFrame.h>
#include <WKGeolocationPermissionRequest.h>
#include <WKPage.h>
#include <WKSecurityOrigin.h>

class QQuickWebView;
class QWebPermissionRequest;

namespace WebKit {

class QtWebPageUIClient {
public:
    enum FileChooserType {
        SingleFileSelection,
        MultipleFilesSelection
    };

    QtWebPageUIClient(WKPageRef, QQuickWebView*);

private:
    quint64 exceededDatabaseQuota(const QString& databaseName, const QString& displayName, WKSecurityOriginRef, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage);
    void runJavaScriptAlert(const QString& message);
    bool runJavaScriptConfirm(const QString& message);
    QString runJavaScriptPrompt(const QString& message, const QString& defaultValue, bool& ok);
    void runOpenPanel(WKOpenPanelResultListenerRef, const QStringList& selectedFileNames, FileChooserType);
    void mouseDidMoveOverElement(const QUrl& linkURL, const QString& linkTitle);
    void permissionRequest(QWebPermissionRequest*);

    // WKPageUIClient callbacks.
    static void runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo);
    static bool runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo);
    static WKStringRef runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo);
    static void runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef, WKOpenPanelResultListenerRef, const void* clientInfo);
    static void mouseDidMoveOverElement(WKPageRef, WKHitTestResultRef, WKEventModifiers, WKTypeRef userData, const void* clientInfo);
    static unsigned long long exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void *clientInfo);
    static void policyForGeolocationPermissionRequest(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKGeolocationPermissionRequestRef, const void*);
    static void policyForNotificationPermissionRequest(WKPageRef, WKSecurityOriginRef, WKNotificationPermissionRequestRef, const void*);

    QQuickWebView* m_webView;
    QUrl m_lastHoveredURL;
    QString m_lastHoveredTitle;
};

} // namespace WebKit

#endif // QtWebPageUIClient_h
