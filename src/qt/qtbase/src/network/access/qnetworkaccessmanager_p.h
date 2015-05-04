/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKACCESSMANAGER_P_H
#define QNETWORKACCESSMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qnetworkaccessmanager.h"
#include "qnetworkaccesscache_p.h"
#include "qnetworkaccessbackend_p.h"
#include "private/qobject_p.h"
#include "QtNetwork/qnetworkproxy.h"
#include "QtNetwork/qnetworksession.h"
#include "qnetworkaccessauthenticationmanager_p.h"
#ifndef QT_NO_BEARERMANAGEMENT
#include "QtNetwork/qnetworkconfigmanager.h"
#endif

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QAbstractNetworkCache;
class QNetworkAuthenticationCredential;
class QNetworkCookieJar;

class QNetworkAccessManagerPrivate: public QObjectPrivate
{
public:
    QNetworkAccessManagerPrivate()
        : networkCache(0), cookieJar(0),
          httpThread(0),
#ifndef QT_NO_NETWORKPROXY
          proxyFactory(0),
#endif
#ifndef QT_NO_BEARERMANAGEMENT
          lastSessionState(QNetworkSession::Invalid),
          networkConfiguration(networkConfigurationManager.defaultConfiguration()),
          customNetworkConfiguration(false),
          networkSessionRequired(networkConfigurationManager.capabilities()
                                 & QNetworkConfigurationManager::NetworkSessionRequired),
          networkAccessible(QNetworkAccessManager::Accessible),
          activeReplyCount(0),
          online(false),
          initializeSession(true),
#endif
          cookieJarCreated(false),
          authenticationManager(QSharedPointer<QNetworkAccessAuthenticationManager>::create())
    { }
    ~QNetworkAccessManagerPrivate();

    void _q_replyFinished();
    void _q_replyEncrypted();
    void _q_replySslErrors(const QList<QSslError> &errors);
    QNetworkReply *postProcess(QNetworkReply *reply);
    void createCookieJar() const;

    void authenticationRequired(QAuthenticator *authenticator,
                                QNetworkReply *reply,
                                bool synchronous,
                                QUrl &url,
                                QUrl *urlForLastAuthentication,
                                bool allowAuthenticationReuse = true);
    void cacheCredentials(const QUrl &url, const QAuthenticator *auth);
    QNetworkAuthenticationCredential *fetchCachedCredentials(const QUrl &url,
                                                             const QAuthenticator *auth = 0);

#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QUrl &url,
                                const QNetworkProxy &proxy,
                                bool synchronous,
                                QAuthenticator *authenticator,
                                QNetworkProxy *lastProxyAuthentication);
    void cacheProxyCredentials(const QNetworkProxy &proxy, const QAuthenticator *auth);
    QNetworkAuthenticationCredential *fetchCachedProxyCredentials(const QNetworkProxy &proxy,
                                                             const QAuthenticator *auth = 0);
    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query);
#endif

    QNetworkAccessBackend *findBackend(QNetworkAccessManager::Operation op, const QNetworkRequest &request);
    QStringList backendSupportedSchemes() const;

#ifndef QT_NO_BEARERMANAGEMENT
    void createSession(const QNetworkConfiguration &config);
    QSharedPointer<QNetworkSession> getNetworkSession() const;

    void _q_networkSessionClosed();
    void _q_networkSessionNewConfigurationActivated();
    void _q_networkSessionPreferredConfigurationChanged(const QNetworkConfiguration &config,
                                                        bool isSeamless);
    void _q_networkSessionStateChanged(QNetworkSession::State state);
    void _q_onlineStateChanged(bool isOnline);
#endif

    QNetworkRequest prepareMultipart(const QNetworkRequest &request, QHttpMultiPart *multiPart);

    // this is the cache for storing downloaded files
    QAbstractNetworkCache *networkCache;

    QNetworkCookieJar *cookieJar;

    QThread *httpThread;


#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    QNetworkProxyFactory *proxyFactory;
#endif

#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSessionStrongRef;
    QWeakPointer<QNetworkSession> networkSessionWeakRef;
    QNetworkSession::State lastSessionState;
    QNetworkConfigurationManager networkConfigurationManager;
    QNetworkConfiguration networkConfiguration;
    // we need to track whether the user set a config or not,
    // because the default config might change
    bool customNetworkConfiguration;
    bool networkSessionRequired;
    QNetworkAccessManager::NetworkAccessibility networkAccessible;
    int activeReplyCount;
    bool online;
    bool initializeSession;
#endif

    bool cookieJarCreated;

    // The cache with authorization data:
    QSharedPointer<QNetworkAccessAuthenticationManager> authenticationManager;

    // this cache can be used by individual backends to cache e.g. their TCP connections to a server
    // and use the connections for multiple requests.
    QNetworkAccessCache objectCache;
    static inline QNetworkAccessCache *getObjectCache(QNetworkAccessBackend *backend)
    { return &backend->manager->objectCache; }
    Q_AUTOTEST_EXPORT static void clearCache(QNetworkAccessManager *manager);
#ifndef QT_NO_BEARERMANAGEMENT
    Q_AUTOTEST_EXPORT static const QWeakPointer<const QNetworkSession> getNetworkSession(const QNetworkAccessManager *manager);
#endif
    Q_DECLARE_PUBLIC(QNetworkAccessManager)
};

QT_END_NAMESPACE

#endif
