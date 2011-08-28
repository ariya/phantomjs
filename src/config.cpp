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

#include "config.h"

#include <QDir>
#include <QSettings>

// public:
Config::Config()
{
    resetToDefaults();
}

void Config::load()
{
    resetToDefaults();

    // TODO Should there be a system-wide config file?
    //loadConfigFile(getSystemConfigFilePath());

    // Load global config
    loadIniFile(getGlobalConfigFilePath());

    // Load local config (allows for overrides)
    loadIniFile(getLocalConfigFilePath());
}

void Config::loadIniFile(const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    QStringList keys = settings.allKeys();
    QStringListIterator it(keys);
    while (it.hasNext()) {
        QString key = it.next().trimmed();
        QVariant value = settings.value(key);

        if (key == "phantomjs/loadImages") {
            setAutoLoadImages(asBool(value));
            continue;
        }
        if (key == "phantomjs/loadPlugins") {
            setPluginsEnabled(asBool(value));
            continue;
        }
        if (key == "phantomjs/proxy") {
            setProxy(asString(value));
            continue;
        }
        if (key == "phantomjs/diskCache") {
            setDiskCacheEnabled(asBool(value));
            continue;
        }
        if (key == "phantomjs/ignoreSslErrors") {
            setIgnoreSslErrors(asBool(value));
            continue;
        }
        if (key == "phantomjs/localAccessRemote") {
            setLocalAccessRemote(asBool(value));
            continue;
        }
        if (key == "phantomjs/cookies") {
            setCookieFile(asString(value));
            continue;
        }
        if (key == "phantomjs/outputEncoding") {
            setOutputEncoding(asString(value));
            continue;
        }
        if (key == "phantomjs/scriptEncoding") {
            setScriptEncoding(asString(value));
            continue;
        }
    }
}

bool Config::autoLoadImages() const
{
    return m_autoLoadImages;
}

void Config::setAutoLoadImages(const bool value)
{
    m_autoLoadImages = value;
}

QString Config::cookieFile() const
{
    return m_cookieFile;
}

void Config::setCookieFile(const QString &value)
{
    m_cookieFile = value;
}

bool Config::diskCacheEnabled() const
{
    return m_diskCacheEnabled;
}

void Config::setDiskCacheEnabled(const bool value)
{
    m_diskCacheEnabled = value;
}

bool Config::ignoreSslErrors() const
{
    return m_ignoreSslErrors;
}

void Config::setIgnoreSslErrors(const bool value)
{
    m_ignoreSslErrors = value;
}

bool Config::localAccessRemote() const
{
    return m_localAccessRemote;
}

void Config::setLocalAccessRemote(const bool value)
{
    m_localAccessRemote = value;
}

QString Config::outputEncoding() const
{
    return m_outputEncoding;
}

void Config::setOutputEncoding(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    m_outputEncoding = value;
}

bool Config::pluginsEnabled() const
{
    return m_pluginsEnabled;
}

void Config::setPluginsEnabled(const bool value)
{
    m_pluginsEnabled = value;
}

void Config::setProxy(const QString &value)
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

QString Config::proxyHost() const
{
    return m_proxyHost;
}

int Config::proxyPort() const
{
    return m_proxyPort;
}

QString Config::scriptEncoding() const
{
    return m_scriptEncoding;
}

void Config::setScriptEncoding(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    m_scriptEncoding = value;
}

// private:
void Config::resetToDefaults()
{
    m_autoLoadImages = true;
    m_cookieFile = QString();
    m_diskCacheEnabled = false;
    m_ignoreSslErrors = false;
    m_localAccessRemote = false;
    m_outputEncoding = "UTF-8";
    m_pluginsEnabled = false;
    m_proxyHost = QString();
    m_proxyPort = 1080;
    m_scriptEncoding = "UTF-8";
}

void Config::setProxyHost(const QString &value)
{
    m_proxyHost = value;
}

void Config::setProxyPort(const int value)
{
    m_proxyPort = value;
}

QString Config::getGlobalConfigFilePath() const
{
    return joinPaths(QDir::homePath(), CONFIG_FILE_NAME);
}

QString Config::getLocalConfigFilePath() const
{
    return joinPaths(QDir::currentPath(), CONFIG_FILE_NAME);
}

// private: (static)
bool Config::asBool(const QVariant &value)
{
    QString strVal = asString(value).toLower();

    return strVal == "true" || strVal == "1" || strVal == "yes";
}

QString Config::asString(const QVariant &value)
{
    return value.toString().trimmed();
}

QString Config::joinPaths(const QString &path1, const QString &path2)
{
    QString joinedPath;

    if (path1.isEmpty()) {
        joinedPath = path2;
    } else if (path2.isEmpty()) {
        joinedPath = path1;
    } else {
        joinedPath = path1 + QDir::separator() + path2;
    }

    return normalisePath(joinedPath);
}

QString Config::normalisePath(const QString &path)
{
    return path.isEmpty() ? path : QDir::fromNativeSeparators(path);
}

const QString Config::CONFIG_FILE_NAME = ".phantomjsrc";
