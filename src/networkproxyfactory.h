/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2014 Jef le Ponot <jef_le_ponot@voila.fr>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NETWORKPROXYFACTORY_H
#define NETWORKPROXYFACTORY_H

#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <QUrl>
#include <QList>
#include <QString>
#include <QStringList>
#include "config.h"

class NetworkProxyFactory : public QObject, public QNetworkProxyFactory
{
    Q_OBJECT
public:
    NetworkProxyFactory();
    ~NetworkProxyFactory();
    /*
    void setHttpProxy(const QString &userName);
    void setHttpsProxy(const QString &password);
    void setFtpProxy(int maxAttempts);
    void setSocksProxy(int resourceTimeout);
    void setNoProxy(const QVariantMap &headers);
    */
    bool initializeFromEnvironment(const Config *config);
    
	QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery& query = QNetworkProxyQuery());
protected:

signals:

private slots:

private:
	bool ignoreProxyFor(const QNetworkProxyQuery &query);

	QList<QNetworkProxy> m_httpProxy;
    QList<QNetworkProxy> m_httpsProxy;
    QList<QNetworkProxy> m_ftpProxy;
    QList<QNetworkProxy> m_socksProxy;

    QList<QByteArray> noProxyTokens;
};


#endif //NETWORKPROXYFACTORY_H
