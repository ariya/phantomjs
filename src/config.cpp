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
#include <QNetworkProxy>

#include "terminal.h"
#include "utils.h"

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

        if (arg == "--help" || arg == "-h") {
            setHelpFlag(true);
            return;
        }
        if (arg == "--version" || arg == "-v") {
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
        if (arg.startsWith("--proxy-type=")) {
            setProxyType(arg.mid(13).trimmed());
            continue;
        }
        if (arg.startsWith("--proxy-auth=")){
            setProxyAuth(arg.mid(13).trimmed());
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
        if (arg.startsWith("--local-storage-path=")) {
            setOfflineStoragePath(arg.mid(21).trimmed());
            continue;
        }
        if (arg.startsWith("--local-storage-quota=")) {
            setOfflineStorageDefaultQuota(arg.mid(arg.indexOf("=") + 1).trimmed().toInt());
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
        if (arg.startsWith("--remote-debugger-port=")) {
            setDebug(true);
            setRemoteDebugPort(arg.mid(23).trimmed().toInt());
            continue;
        }
        if (arg == "--remote-debugger-autorun=yes") {
            setRemoteDebugAutorun(true);
            continue;
        }
        if (arg == "--remote-debugger-autorun=no") {
            setRemoteDebugAutorun(false);
            continue;
        }
        if (arg == "--web-security=yes") {
            setWebSecurityEnabled(true);
            continue;
        }
        if (arg == "--web-security=no") {
            setWebSecurityEnabled(false);
            continue;
        }
        if (arg == "--debug=yes") {
            setPrintDebugMessages(true);
            continue;
        }
        if (arg == "--debug=no") {
            setPrintDebugMessages(false);
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

void Config::loadJsonFile(const QString &filePath)
{
    QString jsonConfig;
    QFile f(filePath);

    // Check file exists and is readable
    if (!f.exists() || !f.open(QFile::ReadOnly | QFile::Text)) {
        Terminal::instance()->cerr("Unable to open config: \"" + filePath + "\"");
        return;
    }

    // Read content
    jsonConfig = QString::fromUtf8(f.readAll().trimmed());
    f.close();

    // Check it's a valid JSON format
    if (jsonConfig.isEmpty() || !jsonConfig.startsWith('{') || !jsonConfig.endsWith('}')) {
        Terminal::instance()->cerr("Config file MUST be in JSON format!");
        return;
    }

    // Load configurator
    QString configurator = Utils::readResourceFileUtf8(":/configurator.js");

    // Use a temporary QWebPage to load the JSON configuration in this Object using the 'configurator' above
    QWebPage webPage;
    // Add this object to the global scope
    webPage.mainFrame()->addToJavaScriptWindowObject("config", this);
    // Apply the JSON config settings to this very object
    webPage.mainFrame()->evaluateJavaScript(configurator.arg(jsonConfig), QString());
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

QString Config::offlineStoragePath() const
{
    return m_offlineStoragePath;
}

void Config::setOfflineStoragePath(const QString &value)
{
    QDir dir(value);
    m_offlineStoragePath = dir.absolutePath();
}

int Config::offlineStorageDefaultQuota() const
{
    return m_offlineStorageDefaultQuota;
}

void Config::setOfflineStorageDefaultQuota(int offlineStorageDefaultQuota)
{
    m_offlineStorageDefaultQuota = offlineStorageDefaultQuota * 1024;
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

QString Config::proxyType() const
{
    return m_proxyType;
}

void Config::setProxyType(const QString value)
{
    m_proxyType = value;
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

void Config::setProxyAuth(const QString &value)
{
    QString proxyUser = value;
    QString proxyPass = "";

    if (proxyUser.lastIndexOf(':') > 0) {
        proxyPass = proxyUser.mid(proxyUser.lastIndexOf(':') + 1).trimmed();
        proxyUser = proxyUser.left(proxyUser.lastIndexOf(':')).trimmed();

        setProxyAuthUser(proxyUser);
        setProxyAuthPass(proxyPass);
    }
}

QString Config::proxyAuth() const
{
    return proxyAuthUser() + ":" + proxyAuthPass();
}

QString Config::proxyAuthUser() const
{
    return m_proxyAuthUser;
}

QString Config::proxyAuthPass() const
{
    return m_proxyAuthPass;
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

bool Config::debug() const
{
    return m_debug;
}

void Config::setDebug(const bool value)
{
    m_debug = value;
}

int Config::remoteDebugPort() const
{
    return m_remoteDebugPort;
}

void Config::setRemoteDebugPort(const int port)
{
    m_remoteDebugPort = port;
}

bool Config::remoteDebugAutorun() const
{
    return m_remoteDebugAutorun;
}

void Config::setRemoteDebugAutorun(const bool value)
{
    m_remoteDebugAutorun = value;
}

bool Config::webSecurityEnabled() const
{
    return m_webSecurityEnabled;
}

void Config::setWebSecurityEnabled(const bool value)
{
    m_webSecurityEnabled = value;
}

void Config::setJavascriptCanOpenWindows(const bool value)
{
    m_javascriptCanOpenWindows = value;
}

bool Config::javascriptCanOpenWindows() const
{
    return m_javascriptCanOpenWindows;
}

void Config::setJavascriptCanCloseWindows(const bool value)
{
    m_javascriptCanCloseWindows = value;
}

bool Config::javascriptCanCloseWindows() const
{
    return m_javascriptCanCloseWindows;
}

// private:
void Config::resetToDefaults()
{
    m_autoLoadImages = true;
    m_cookiesFile = QString();
    m_offlineStoragePath = QString();
    m_offlineStorageDefaultQuota = -1;
    m_diskCacheEnabled = false;
    m_maxDiskCacheSize = -1;
    m_ignoreSslErrors = false;
    m_localToRemoteUrlAccessEnabled = false;
    m_outputEncoding = "UTF-8";
    m_proxyType = "http";
    m_proxyHost.clear();
    m_proxyPort = 1080;
    m_proxyAuthUser.clear();
    m_proxyAuthPass.clear();
    m_scriptArgs.clear();
    m_scriptEncoding = "UTF-8";
    m_scriptFile.clear();
    m_unknownOption.clear();
    m_versionFlag = false;
    m_debug = false;
    m_remoteDebugPort = -1;
    m_remoteDebugAutorun = false;
    m_webSecurityEnabled = true;
    m_javascriptCanOpenWindows = true;
    m_javascriptCanCloseWindows = true;
    m_helpFlag = false;
    m_printDebugMessages = false;
}

void Config::setProxyAuthPass(const QString &value)
{
    m_proxyAuthPass = value;
}

void Config::setProxyAuthUser(const QString &value)
{
    m_proxyAuthUser = value;
}

void Config::setProxyHost(const QString &value)
{
    m_proxyHost = value;
}

void Config::setProxyPort(const int value)
{
    m_proxyPort = value;
}

bool Config::helpFlag() const
{
    return m_helpFlag;
}

void Config::setHelpFlag(const bool value)
{
    m_helpFlag = value;
}

bool Config::printDebugMessages() const
{
    return m_printDebugMessages;
}

void Config::setPrintDebugMessages(const bool value)
{
    m_printDebugMessages = value;
}
