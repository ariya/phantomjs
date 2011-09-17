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
#include <QWebPage>
#include <QWebFrame>

#include "terminal.h"

// public:
Config::Config(QObject *parent)
    : QObject(parent)
{
    resetToDefaults();
}

void Config::init(const QStringList *const args)
{
    resetToDefaults();

    processArgs(*args);
}

void Config::processArgs(const QStringList &args)
{
    QStringListIterator it(args);
    while (it.hasNext()) {
        const QString &arg = it.next();

        if (arg == "--version") {
            setVersionFlag(true);
            return;
        }
        if (arg == "--load-images=yes") {
            setAutoLoadImages(true);
            continue;
        }
        if (arg == "--load-images=no") {
            setAutoLoadImages(false);
            continue;
        }
        if (arg == "--load-plugins=yes") {
            setPluginsEnabled(true);
            continue;
        }
        if (arg == "--load-plugins=no") {
            setPluginsEnabled(false);
            continue;
        }
        if (arg == "--disk-cache=yes") {
            setDiskCacheEnabled(true);
            continue;
        }
        if (arg == "--disk-cache=no") {
            setDiskCacheEnabled(false);
            continue;
        }
        if (arg.startsWith("--max-disk-cache-size=")) {
            setMaxDiskCacheSize(arg.mid(arg.indexOf("=") + 1).trimmed().toInt());
            continue;
        }
        if (arg == "--ignore-ssl-errors=yes") {
            setIgnoreSslErrors(true);
            continue;
        }
        if (arg == "--ignore-ssl-errors=no") {
            setIgnoreSslErrors(false);
            continue;
        }
        if (arg == "--local-to-remote-url-access=no") {
            setLocalToRemoteUrlAccessEnabled(false);
            continue;
        }
        if (arg == "--local-to-remote-url-access=yes") {
            setLocalToRemoteUrlAccessEnabled(true);
            continue;
        }
        if (arg.startsWith("--proxy=")) {
            setProxy(arg.mid(8).trimmed());
            continue;
        }
        if (arg.startsWith("--cookies-file=")) {
            setCookiesFile(arg.mid(15).trimmed());
            continue;
        }
        if (arg.startsWith("--output-encoding=")) {
            setOutputEncoding(arg.mid(18).trimmed());
            continue;
        }
        if (arg.startsWith("--script-encoding=")) {
            setScriptEncoding(arg.mid(18).trimmed());
            continue;
        }
        if (arg.startsWith("--config=")) {
            QString configPath = arg.mid(9).trimmed();
            loadJsonFile(configPath);
            continue;
        }
        if (arg.startsWith("--")) {
            setUnknownOption(QString("Unknown option '%1'").arg(arg));
            return;
        }

        // There are no more options at this point.
        // The remaining arguments are available for the script.

        m_scriptFile = arg;

        while (it.hasNext()) {
            m_scriptArgs += it.next();
        }
    }
}

static QString normalizePath(const QString &path)
{
    return path.isEmpty() ? path : QDir::fromNativeSeparators(path);
}

// THIS METHOD ASSUMES THAT content IS *NEVER* NULL!
static bool readFile(const QString &path, QString *const content)
{
    // Ensure empty content
    content->clear();

    // Check existence and try to open as text
    QFile file(path);
    if (!file.exists() || !file.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    content->append(QString::fromUtf8(file.readAll()).trimmed());

    file.close();

    return true;
}

void Config::loadJsonFile(const QString &filePath)
{
    QString jsonConfig;
    if (!readFile(normalizePath(filePath), &jsonConfig)) {
        Terminal::instance()->cerr("Unable to open config: \"" + filePath + "\"");
        return;
    } else if (jsonConfig.isEmpty()) {
        return;
    } else if (!jsonConfig.startsWith('{') || !jsonConfig.endsWith('}')) {
        Terminal::instance()->cerr("Config file MUST be in JSON format!");
        return;
    }

    // Load configurator
    QString configurator;
    if (!readFile(":/configurator.js", &configurator)) {
        Terminal::instance()->cerr("Unable to load JSON configurator!");
        return;
    } else if (configurator.isEmpty()) {
        Terminal::instance()->cerr("Unable to set-up JSON configurator!");
        return;
    }

    QWebPage webPage;

    // Add the config object
    webPage.mainFrame()->addToJavaScriptWindowObject("config", this);

    // Apply the settings
    webPage.mainFrame()->evaluateJavaScript(configurator.arg(jsonConfig));
}

bool Config::autoLoadImages() const
{
    return m_autoLoadImages;
}

void Config::setAutoLoadImages(const bool value)
{
    m_autoLoadImages = value;
}

QString Config::cookiesFile() const
{
    return m_cookiesFile;
}

void Config::setCookiesFile(const QString &value)
{
    m_cookiesFile = value;
}

bool Config::diskCacheEnabled() const
{
    return m_diskCacheEnabled;
}

void Config::setDiskCacheEnabled(const bool value)
{
    m_diskCacheEnabled = value;
}

int Config::maxDiskCacheSize() const
{
    return m_maxDiskCacheSize;
}

void Config::setMaxDiskCacheSize(int maxDiskCacheSize)
{
    m_maxDiskCacheSize = maxDiskCacheSize;
}

bool Config::ignoreSslErrors() const
{
    return m_ignoreSslErrors;
}

void Config::setIgnoreSslErrors(const bool value)
{
    m_ignoreSslErrors = value;
}

bool Config::localToRemoteUrlAccessEnabled() const
{
    return m_localToRemoteUrlAccessEnabled;
}

void Config::setLocalToRemoteUrlAccessEnabled(const bool value)
{
    m_localToRemoteUrlAccessEnabled = value;
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

QString Config::proxy() const
{
    return proxyHost() + ":" + proxyPort();
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

QStringList Config::scriptArgs() const
{
    return m_scriptArgs;
}

void Config::setScriptArgs(const QStringList &value)
{
    m_scriptArgs.clear();

    QStringListIterator it(value);
    while (it.hasNext()) {
        m_scriptArgs.append(it.next());
    }
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

QString Config::scriptFile() const
{
    return m_scriptFile;
}

void Config::setScriptFile(const QString &value)
{
    m_scriptFile = value;
}

QString Config::unknownOption() const
{
    return m_unknownOption;
}

void Config::setUnknownOption(const QString &value)
{
    m_unknownOption = value;
}

bool Config::versionFlag() const
{
    return m_versionFlag;
}

void Config::setVersionFlag(const bool value)
{
    m_versionFlag = value;
}

// private:
void Config::resetToDefaults()
{
    m_autoLoadImages = true;
    m_cookiesFile = QString();
    m_diskCacheEnabled = false;
    m_maxDiskCacheSize = -1;
    m_ignoreSslErrors = false;
    m_localToRemoteUrlAccessEnabled = false;
    m_outputEncoding = "UTF-8";
    m_pluginsEnabled = false;
    m_proxyHost.clear();
    m_proxyPort = 1080;
    m_scriptArgs.clear();
    m_scriptEncoding = "UTF-8";
    m_scriptFile.clear();
    m_unknownOption.clear();
    m_versionFlag = false;
}

void Config::setProxyHost(const QString &value)
{
    m_proxyHost = value;
}

void Config::setProxyPort(const int value)
{
    m_proxyPort = value;
}
