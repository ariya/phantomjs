/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include "private/qquicknetworkreply_p.h"
#include "private/qquicknetworkrequest_p.h"
#include "private/qquickwebpage_p.h"
#include "private/qquickwebview_p.h"
#include "private/qtwebsecurityorigin_p.h"
#include "private/qwebdownloaditem_p.h"
#include "private/qwebkittest_p.h"
#include "private/qwebnavigationhistory_p.h"
#include "private/qwebpermissionrequest_p.h"
#include "private/qwebpreferences_p.h"

#include <QtQml/qqml.h>
#include <QtQml/qqmlextensionplugin.h>

QT_BEGIN_NAMESPACE

class QQuickWebViewExperimentalExtension : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebViewExperimental* experimental READ experimental CONSTANT FINAL)
public:
    QQuickWebViewExperimentalExtension(QObject *parent = 0) : QObject(parent) { }
    QQuickWebViewExperimental* experimental() { return static_cast<QQuickWebView*>(parent())->experimental(); }
};

class WebKitQmlExperimentalExtensionPlugin: public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface" FILE "plugin.json")
public:
    virtual void registerTypes(const char* uri)
    {
        qWarning("\nWARNING: This project is using the experimental QML API extensions for QtWebKit and is therefore tied to a specific QtWebKit release.\n"
                 "WARNING: The experimental API will change from version to version, or even be removed. You have been warned!\n");

        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtWebKit.experimental"));

        qmlRegisterUncreatableType<QWebDownloadItem>(uri, 1, 0, "DownloadItem", QObject::tr("Cannot create separate instance of DownloadItem"));
        qmlRegisterUncreatableType<QWebNavigationListModel>(uri, 1, 0, "NavigationListModel", QObject::tr("Cannot create separate instance of NavigationListModel"));
        qmlRegisterUncreatableType<QWebNavigationHistory>(uri, 1, 0, "NavigationHistory", QObject::tr("Cannot create separate instance of NavigationHistory"));
        qmlRegisterUncreatableType<QWebPreferences>(uri, 1, 0, "WebPreferences", QObject::tr("Cannot create separate instance of WebPreferences"));
        qmlRegisterUncreatableType<QWebPermissionRequest>(uri, 1, 0, "PermissionRequest", QObject::tr("Cannot create separate instance of PermissionRequest"));
        qmlRegisterUncreatableType<QtWebSecurityOrigin>(uri, 1, 0, "SecurityOrigin", QObject::tr("Cannot create separate instance of SecurityOrigin"));

        qmlRegisterExtendedType<QQuickWebView, QQuickWebViewExperimentalExtension>(uri, 1, 0, "WebView");
        qmlRegisterUncreatableType<QQuickWebViewExperimental>(uri, 1, 0, "WebViewExperimental",
            QObject::tr("Cannot create separate instance of WebViewExperimental"));
        qmlRegisterUncreatableType<QWebKitTest>(uri, 1, 0, "QWebKitTest",
            QObject::tr("Cannot create separate instance of QWebKitTest"));
        qmlRegisterType<QQuickUrlSchemeDelegate>(uri, 1, 0, "UrlSchemeDelegate");
        qmlRegisterUncreatableType<QQuickNetworkRequest>(uri, 1, 0, "NetworkRequest",
            QObject::tr("NetworkRequest should not be created from QML"));
        qmlRegisterUncreatableType<QQuickNetworkReply>(uri, 1, 0, "NetworkReply",
            QObject::tr("NetworkReply should not be created from QML"));
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
