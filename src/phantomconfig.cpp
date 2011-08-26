/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com

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

#include "phantomconfig.h"

// public:
PhantomConfig::PhantomConfig()
{
    resetToDefaults();
}

void PhantomConfig::load()
{
    resetToDefaults();
}

QString PhantomConfig::cookieFile() const
{
    return m_cookieFile;
}

void PhantomConfig::setCookieFile(const QString &value)
{
    m_cookieFile = value;
}

bool PhantomConfig::diskCacheEnabled() const
{
    return m_diskCacheEnabled;
}

void PhantomConfig::setDiskCacheEnabled(const bool value)
{
    m_diskCacheEnabled = value;
}

bool PhantomConfig::ignoreSslErrors() const
{
    return m_ignoreSslErrors;
}

void PhantomConfig::setIgnoreSslErrors(const bool value)
{
    m_ignoreSslErrors = value;
}

QString PhantomConfig::outputEncoding() const
{
    return m_outputEncoding;
}

void PhantomConfig::setOutputEncoding(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    m_outputEncoding = value;
}

void PhantomConfig::setProxy(const QString &value)
{
    QString proxyHost = value;
    int proxyPort = 1080;

    if (proxyHost.lastIndexOf(':') > 0) {
        bool ok = true;
        int port = proxyHost.mid(proxyHost.lastIndexOf(':') + 1).toInt(&ok);
        if (ok) {
            proxyHost = proxyHost.left(proxyHost.lastIndexOf(':')).trimmed();
            proxyPort = port;
        }
    }

    setProxyHost(proxyHost);
    setProxyPort(proxyPort);
}

QString PhantomConfig::proxyHost() const
{
    return m_proxyHost;
}

int PhantomConfig::proxyPort() const
{
    return m_proxyPort;
}

QString PhantomConfig::scriptEncoding() const
{
    return m_scriptEncoding;
}

void PhantomConfig::setScriptEncoding(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    m_scriptEncoding = value;
}

// private:
void PhantomConfig::resetToDefaults()
{
    m_cookieFile = QString();
    m_diskCacheEnabled = false;
    m_ignoreSslErrors = false;
    m_outputEncoding = "UTF-8";
    m_proxyHost = QString();
    m_proxyPort = 1080;
    m_scriptEncoding = "UTF-8";
}

void PhantomConfig::setProxyHost(const QString &value)
{
    m_proxyHost = value;
}

void PhantomConfig::setProxyPort(const int value)
{
    m_proxyPort = value;
}
