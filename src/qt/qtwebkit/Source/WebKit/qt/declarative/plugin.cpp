/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#include "qglobal.h"

#include <QtQml/qqml.h>
#include <QtQml/qqmlextensionplugin.h>

#if defined(HAVE_WEBKIT2)
#include "private/qquickwebpage_p.h"
#include "private/qquickwebview_p.h"
#include "private/qwebiconimageprovider_p.h"
#include "private/qwebloadrequest_p.h"
#include "private/qwebnavigationrequest_p.h"

#include <QtNetwork/qnetworkreply.h>
#include <QtQml/qqmlengine.h>
#endif

QT_BEGIN_NAMESPACE

class WebKitQmlPlugin : public QQmlExtensionPlugin {
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface" FILE "plugin.json")
    Q_OBJECT
public:
#if defined(HAVE_WEBKIT2)
    virtual void initializeEngine(QQmlEngine* engine, const char* uri)
    {
        Q_UNUSED(uri);
        engine->addImageProvider(QLatin1String("webicon"), new QWebIconImageProvider);
    }
#endif

    virtual void registerTypes(const char* uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtWebKit"));

#if defined(HAVE_WEBKIT2)
        qmlRegisterType<QQuickWebView>(uri, 3, 0, "WebView");
        qmlRegisterUncreatableType<QQuickWebPage>(uri, 3, 0, "WebPage", QObject::tr("Cannot create separate instance of WebPage, use WebView"));
        qmlRegisterUncreatableType<QNetworkReply>(uri, 3, 0, "NetworkReply", QObject::tr("Cannot create separate instance of NetworkReply"));
        qmlRegisterUncreatableType<QWebNavigationRequest>(uri, 3, 0, "NavigationRequest", QObject::tr("Cannot create separate instance of NavigationRequest"));
        qmlRegisterUncreatableType<QWebLoadRequest>(uri, 3, 0, "WebLoadRequest", QObject::tr("Cannot create separate instance of WebLoadRequest"));
#endif
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
