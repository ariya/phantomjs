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

#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <QUrl>
#include <QList>
#include <QString>
#include <QStringList>

#include "networkproxyfactory.h"
#include "phantom.h"
#include "config.h"
#include "cookiejar.h"
#include "terminal.h"
NetworkProxyFactory::NetworkProxyFactory()
{
}

NetworkProxyFactory::~NetworkProxyFactory()
{
}

bool NetworkProxyFactory::initializeFromEnvironment(const Config *config)
{
	QString proxyType = config->proxyType();

    if (proxyType == "none") 
		return false;

    if (config->proxyHost().isEmpty()) 
		return false;

	if (config->proxyExceptions().isEmpty()) 
		return false;

	QNetworkProxy::ProxyType networkProxyType;
	if (proxyType == "socks5") 
		networkProxyType = QNetworkProxy::Socks5Proxy;
	else
		networkProxyType = QNetworkProxy::HttpProxy;

	if(!config->proxyAuthUser().isEmpty() && !config->proxyAuthPass().isEmpty()) 
		m_httpProxy << QNetworkProxy(networkProxyType, config->proxyHost(), config->proxyPort(), config->proxyAuthUser(), config->proxyAuthPass());
	else
		m_httpProxy << QNetworkProxy(networkProxyType, config->proxyHost(), config->proxyPort());

    QByteArray exceptions = config->proxyExceptions().toLocal8Bit(); 
    noProxyTokens = exceptions.split(',');
    
    return true;
}


bool NetworkProxyFactory::ignoreProxyFor(const QNetworkProxyQuery &query)
{
	foreach (const QByteArray rawToken, noProxyTokens) {
		QByteArray token = rawToken.trimmed();
		QString peerHostName = query.peerHostName();

		// Since we use suffix matching, "*" is our 'default' behaviour
		if (token.startsWith("*"))
			token = token.mid(1);

		// Harmonize trailing dot notation
		if (token.endsWith('.') && !peerHostName.endsWith('.'))
			token = token.left(token.length()-1);

		// We prepend a dot to both values, so that when we do a suffix match,
		// we don't match "donotmatch.com" with "match.com"
		if (!token.startsWith('.'))
			token.prepend('.');

		if (!peerHostName.startsWith('.'))
			peerHostName.prepend('.');

		if (peerHostName.endsWith(QString::fromLatin1(token)))
		{
	 		return true;
		}
	}
	return false;
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery& query)
{
    QString protocol = query.protocolTag().toLower();
    /*bool localHost = false;*/
    QList<QNetworkProxy> noproxy;
    noproxy << QNetworkProxy::NoProxy;

	//Terminal::instance()->cout(query.peerHostName());
	
    if (!query.peerHostName().compare(QLatin1String("localhost"), Qt::CaseInsensitive) || !query.peerHostName().compare(QLatin1String("127.0.0.1"), Qt::CaseInsensitive))
        return noproxy;
    if (ignoreProxyFor(query))
        return noproxy;

    return m_httpProxy;
/*
    if (protocol == QLatin1String("https") && !localHost)
        return m_httpsProxy;
*/
}
