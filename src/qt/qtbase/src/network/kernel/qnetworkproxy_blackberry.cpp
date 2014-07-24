/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/**
 * Some notes about the code:
 *
 * ** It is assumed that the system proxies are for url based requests
 *  ie. HTTP/HTTPS based.
 */

#include <QtNetwork/qnetworkproxy.h>

#ifndef QT_NO_NETWORKPROXY


#include <QtCore/qflags.h>
#include <QtCore/qurl.h>
#include <QtNetwork/qnetworkconfiguration.h>

#include <bps/netstatus.h>
#include <errno.h>


QT_BEGIN_NAMESPACE

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    if (query.url().scheme() == QLatin1String("file")
            || query.url().scheme() == QLatin1String("qrc"))
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);

    if (query.queryType() != QNetworkProxyQuery::UrlRequest
            && query.queryType() != QNetworkProxyQuery::TcpSocket) {
        qWarning("Unsupported query type: %d", query.queryType());
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    }

    QUrl url;
    if (query.queryType() == QNetworkProxyQuery::UrlRequest) {
        url = query.url();
    } else if (query.queryType() == QNetworkProxyQuery::TcpSocket
               && !query.peerHostName().isEmpty()) {
        url.setHost(query.peerHostName());
        switch (query.peerPort()) {
        case 443:
            url.setScheme(QStringLiteral("https"));
            break;
        case 21:
            url.setScheme(QStringLiteral("ftp"));
            break;
        default:
            // for unknown ports, we just pretend we are dealing
            // with a HTTP URL, otherwise we will not get a proxy
            // from the netstatus API
            url.setScheme(QStringLiteral("http"));
        }
    }

    if (!url.isValid()) {
        qWarning("Invalid URL: %s", qPrintable(url.toString()));
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    }

    netstatus_proxy_details_t details;
    memset(&details, 0, sizeof(netstatus_proxy_details_t));

#if BPS_VERSION >= 3001001

    QByteArray bUrl(url.toEncoded());
    QString sInterface(query.networkConfiguration().name());
    QByteArray bInterface;
    if (!sInterface.isEmpty()) {
        if (query.networkConfiguration().type() != QNetworkConfiguration::InternetAccessPoint) {
            qWarning("Unsupported configuration type: %d", query.networkConfiguration().type());
            return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
        }
        bInterface = sInterface.toUtf8();
    }

    if (netstatus_get_proxy_details_for_url(bUrl.constData(), (bInterface.isEmpty() ? NULL : bInterface.constData()), &details) != BPS_SUCCESS) {
        qWarning("netstatus_get_proxy_details_for_url failed! errno: %d", errno);
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    }

#else

    if (netstatus_get_proxy_details(&details) != BPS_SUCCESS) {
        qWarning("netstatus_get_proxy_details failed! errno: %d", errno);
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    }

#endif

    if (details.http_proxy_host == NULL) { // No proxy
        netstatus_free_proxy_details(&details);
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    }

    QNetworkProxy proxy;

    QString protocol = query.protocolTag();
    if (protocol.startsWith(QLatin1String("http"), Qt::CaseInsensitive)) { // http, https
        proxy.setType((QNetworkProxy::HttpProxy));
    } else if (protocol == QLatin1String("ftp")) {
        proxy.setType(QNetworkProxy::FtpCachingProxy);
    } else { // assume http proxy
        qDebug("Proxy type: %s assumed to be http proxy", qPrintable(protocol));
        proxy.setType((QNetworkProxy::HttpProxy));
    }

    // Set host
    // Note: ftp and https proxy type fields *are* obsolete.
    // The user interface allows only one host/port which gets duplicated
    // to all proxy type fields.
    proxy.setHostName(QString::fromUtf8(details.http_proxy_host));

    // Set port
    proxy.setPort(details.http_proxy_port);

    // Set username
    if (details.http_proxy_login_user)
        proxy.setUser(QString::fromUtf8(details.http_proxy_login_user));

    // Set password
    if (details.http_proxy_login_password)
        proxy.setPassword(QString::fromUtf8(details.http_proxy_login_password));

    netstatus_free_proxy_details(&details);

    return QList<QNetworkProxy>() << proxy;
}

QT_END_NAMESPACE

#endif
