/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com
  Copyright (C) 2013 James M. Greene <james.m.greene@gmail.com>

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

#include <iostream>
#include <QDir>
#include <QFileInfo>
#include <QWebFrame>
#include <QWebPage>
#include "settings.h"
#include "consts.h"
#include "qcommandline.h"
#include "terminal.h"

static const struct QCommandLineConfigEntry flags[] = {
    { QCommandLine::Option, '\0', "cookies-file", "Sets the file name to store the persistent cookies", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "config", "Specifies JSON-formatted configuration file", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "debug", "Prints additional warning and debug message: 'true' or 'false' (default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "disk-cache", "Enables disk cache: 'true' or 'false' (default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "disk-cache-path", "Specifies the location for the disk cache", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ignore-ssl-errors", "Ignores SSL errors (expired/self-signed certificate errors): 'true' or 'false' (default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "load-images", "Loads all inlined images: 'true' (default) or 'false'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-url-access", "Allows use of 'file:///' URLs: 'true' (default) or 'false'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-storage-path", "Specifies the location for local storage", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-storage-quota", "Sets the maximum size of the local storage (in KB)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "offline-storage-path", "Specifies the location for offline storage", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "offline-storage-quota", "Sets the maximum size of the offline storage (in KB)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-to-remote-url-access", "Allows local content to access remote URL: 'true' or 'false' (default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "max-disk-cache-size", "Limits the size of the disk cache (in KB)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "output-encoding", "Sets the encoding for the terminal output, default is 'utf8'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "remote-debugger-port", "Starts the script in a debug harness and listens on the specified port", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "remote-debugger-autorun", "Runs the script in the debugger immediately: 'true' or 'false' (default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy", "Sets the proxy server, e.g. '--proxy=http://proxy.company.com:8080'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy-auth", "Provides authentication information for the proxy, e.g. '--proxy-auth=username:password'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy-type", "Specifies the proxy type, 'http' (default), 'none' (disable completely), or 'socks5'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "web-security", "Enables web security, 'true' (default) or 'false'", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-protocol", "Selects a specific SSL protocol version to offer. Values (case insensitive): TLSv1.2, TLSv1.1, TLSv1.0, TLSv1 (same as v1.0), SSLv3, or ANY. Default is to offer all that Qt thinks are secure (SSLv3 and up). Not all values may be supported, depending on the system OpenSSL library.", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-ciphers", "Sets supported TLS/SSL ciphers. Argument is a colon-separated list of OpenSSL cipher names (macros like ALL, kRSA, etc. may not be used). Default matches modern browsers.", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-certificates-path", "Sets the location for custom CA certificates (if none set, uses environment variable SSL_CERT_DIR. If none set too, uses system default)", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-certificate-file", "Sets the location of a client certificate", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-key-file", "Sets the location of a clients' private key", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-key-passphrase", "Sets the passphrase for the clients' private key", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "webdriver", "Starts in 'Remote WebDriver mode' (embedded GhostDriver): '[[<IP>:]<PORT>]' (default '127.0.0.1:8910') ", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "webdriver-logfile", "File where to write the WebDriver's Log (default 'none') (NOTE: needs '--webdriver') ", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "webdriver-loglevel", "WebDriver Logging Level: (supported: 'ERROR', 'WARN', 'INFO', 'DEBUG') (default 'INFO') (NOTE: needs '--webdriver') ", QCommandLine::Optional },
    { QCommandLine::Option, '\0', "webdriver-selenium-grid-hub", "URL to the Selenium Grid HUB: 'URL_TO_HUB' (default 'none') (NOTE: needs '--webdriver') ", QCommandLine::Optional },
    { QCommandLine::Param, '\0', "script", "Script", QCommandLine::Flags(QCommandLine::Optional | QCommandLine::ParameterFence)},
    { QCommandLine::Param, '\0', "argument", "Script argument", QCommandLine::OptionalMultiple },
    { QCommandLine::Switch, 'w', "wd", "Equivalent to '--webdriver' option above", QCommandLine::Optional },
    { QCommandLine::Switch, 'h', "help", "Shows this message and quits", QCommandLine::Optional },
    { QCommandLine::Switch, 'v', "version", "Prints out PhantomJS version", QCommandLine::Optional },
    QCOMMANDLINE_CONFIG_ENTRY_END
};

Settings::Settings(QObject* parent)
    : QObject(parent)
{
    m_cmdLine = new QCommandLine(this);

    // We will handle --help and --version ourselves in phantom.cpp
    m_cmdLine->enableHelp(false);
    m_cmdLine->enableVersion(false);
    resetToDefaults();
}

void Settings::init(const QStringList* const args)
{
    resetToDefaults();
    QByteArray envSslCertDir = qgetenv("SSL_CERT_DIR");
    if (!envSslCertDir.isEmpty()) {
        setSslCertificatesPath(envSslCertDir);
    }
    processArgs(*args);
}

void Settings::processArgs(const QStringList& args)
{
    connect(m_cmdLine, SIGNAL(switchFound(const QString&)), this, SLOT(handleSwitch(const QString&)));
    connect(m_cmdLine, SIGNAL(optionFound(const QString&, const QVariant&)), this, SLOT(handleOption(const QString&, const QVariant&)));
    connect(m_cmdLine, SIGNAL(paramFound(const QString&, const QVariant&)), this, SLOT(handleParam(const QString&, const QVariant&)));
    connect(m_cmdLine, SIGNAL(parseError(const QString&)), this, SLOT(handleError(const QString&)));
    m_cmdLine->setArguments(args);
    m_cmdLine->setConfig(flags);
    m_cmdLine->parse();

    // Inject command line parameters to be picked up by GhostDriver
    if (isWebdriverMode()) {
        QStringList argsForGhostDriver;

        m_scriptFile = "main.js";                                           //< launch script

        argsForGhostDriver << QString("--ip=%1").arg(m_webdriverIp);        //< "--ip=IP"
        argsForGhostDriver << QString("--port=%1").arg(m_webdriverPort);    //< "--port=PORT"

        if (!m_webdriverSeleniumGridHub.isEmpty()) {
            argsForGhostDriver << QString("--hub=%1").arg(m_webdriverSeleniumGridHub);  //< "--hub=SELENIUM_GRID_HUB_URL"
        }

        if (!m_webdriverLogFile.isEmpty()) {
            argsForGhostDriver << QString("--logFile=%1").arg(m_webdriverLogFile);          //< "--logFile=LOG_FILE"
            argsForGhostDriver << "--logColor=false";                                   //< Force no-color-output in Log File
        }

        argsForGhostDriver << QString("--logLevel=%1").arg(m_webdriverLogLevel);    //< "--logLevel=LOG_LEVEL"

        // Clear current args and override with those
        setScriptArgs(argsForGhostDriver);
    }
}

void Settings::loadJsonFile(const QString& filePath)
{
    QString jsonConfig;
    QFile f(filePath);

    // Check file exists and is readable
    if (!f.exists() || !f.open(QFile::ReadOnly | QFile::Text)) {
        Terminal::instance()->cerr(QString("Unable to open config: \"%1\"").arg(filePath));
        return;
    }

    // Read content
    jsonConfig = QString::fromUtf8(f.readAll().trimmed());
    f.close();

    // Check it's a valid JSON format
    if (jsonConfig.isEmpty() || !jsonConfig.startsWith('{') || !jsonConfig.endsWith('}')) {
        Terminal::instance()->cerr("File with settings MUST be in JSON format!");
        return;
    }
}

QString Settings::helpText() const
{
    return m_cmdLine->help();
}

bool Settings::autoLoadImages() const
{
    return m_autoLoadImages;
}

void Settings::setAutoLoadImages(const bool value)
{
    m_autoLoadImages = value;
}

QString Settings::cookiesFile() const
{
    return m_cookiesFile;
}

void Settings::setCookiesFile(const QString& value)
{
    m_cookiesFile = value;
}

QString Settings::offlineStoragePath() const
{
    return m_offlineStoragePath;
}

void Settings::setOfflineStoragePath(const QString& value)
{
    QDir dir(value);
    m_offlineStoragePath = dir.absolutePath();
}

int Settings::offlineStorageDefaultQuota() const
{
    return m_offlineStorageDefaultQuota;
}

void Settings::setOfflineStorageDefaultQuota(int offlineStorageDefaultQuota)
{
    m_offlineStorageDefaultQuota = offlineStorageDefaultQuota * 1024;
}


QString Settings::localStoragePath() const
{
    return m_localStoragePath;
}

void Settings::setLocalStoragePath(const QString& value)
{
    QDir dir(value);
    m_localStoragePath = dir.absolutePath();
}

int Settings::localStorageDefaultQuota() const
{
    return m_localStorageDefaultQuota;
}

void Settings::setLocalStorageDefaultQuota(int localStorageDefaultQuota)
{
    m_localStorageDefaultQuota = localStorageDefaultQuota * 1024;
}

bool Settings::diskCacheEnabled() const
{
    return m_diskCacheEnabled;
}

void Settings::setDiskCacheEnabled(const bool value)
{
    m_diskCacheEnabled = value;
}

int Settings::maxDiskCacheSize() const
{
    return m_maxDiskCacheSize;
}

void Settings::setMaxDiskCacheSize(int maxDiskCacheSize)
{
    m_maxDiskCacheSize = maxDiskCacheSize;
}

QString Settings::diskCachePath() const
{
    return m_diskCachePath;
}

void Settings::setDiskCachePath(const QString& value)
{
    QDir dir(value);
    m_diskCachePath = dir.absolutePath();
}

bool Settings::ignoreSslErrors() const
{
    return m_ignoreSslErrors;
}

void Settings::setIgnoreSslErrors(const bool value)
{
    m_ignoreSslErrors = value;
}

bool Settings::localUrlAccessEnabled() const
{
    return m_localUrlAccessEnabled;
}

void Settings::setLocalUrlAccessEnabled(const bool value)
{
    m_localUrlAccessEnabled = value;
}

bool Settings::localToRemoteUrlAccessEnabled() const
{
    return m_localToRemoteUrlAccessEnabled;
}

void Settings::setLocalToRemoteUrlAccessEnabled(const bool value)
{
    m_localToRemoteUrlAccessEnabled = value;
}

QString Settings::outputEncoding() const
{
    return m_outputEncoding;
}

void Settings::setOutputEncoding(const QString& value)
{
    if (value.isEmpty()) {
        return;
    }

    m_outputEncoding = value;
}

QString Settings::proxyType() const
{
    return m_proxyType;
}

void Settings::setProxyType(const QString& value)
{
    m_proxyType = value;
}

QString Settings::proxy() const
{
    return m_proxyHost + ":" + QString::number(m_proxyPort);
}

void Settings::setProxy(const QString& value)
{
    QUrl proxyUrl = QUrl::fromUserInput(value);

    if (proxyUrl.isValid()) {
        setProxyHost(proxyUrl.host());
        setProxyPort(proxyUrl.port(1080));
    }
}

void Settings::setProxyAuth(const QString& value)
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

QString Settings::proxyAuth() const
{
    return proxyAuthUser() + ":" + proxyAuthPass();
}

QString Settings::proxyAuthUser() const
{
    return m_proxyAuthUser;
}

QString Settings::proxyAuthPass() const
{
    return m_proxyAuthPass;
}

QString Settings::proxyHost() const
{
    return m_proxyHost;
}

qint16 Settings::proxyPort() const
{
    return m_proxyPort;
}

QStringList Settings::scriptArgs() const
{
    return m_scriptArgs;
}

void Settings::setScriptArgs(const QStringList& value)
{
    m_scriptArgs.clear();

    QStringListIterator it(value);
    while (it.hasNext()) {
        m_scriptArgs.append(it.next());
    }
}

QString Settings::scriptFile() const
{
    return m_scriptFile;
}

void Settings::setScriptFile(const QString& value)
{
    m_scriptFile = value;
}

QString Settings::unknownOption() const
{
    return m_unknownOption;
}

void Settings::setUnknownOption(const QString& value)
{
    m_unknownOption = value;
}

bool Settings::versionFlag() const
{
    return m_versionFlag;
}

void Settings::setVersionFlag(const bool value)
{
    m_versionFlag = value;
}

bool Settings::debug() const
{
    return m_debug;
}

void Settings::setDebug(const bool value)
{
    m_debug = value;
}

int Settings::remoteDebugPort() const
{
    return m_remoteDebugPort;
}

void Settings::setRemoteDebugPort(const int port)
{
    m_remoteDebugPort = port;
}

bool Settings::remoteDebugAutorun() const
{
    return m_remoteDebugAutorun;
}

void Settings::setRemoteDebugAutorun(const bool value)
{
    m_remoteDebugAutorun = value;
}

bool Settings::webSecurityEnabled() const
{
    return m_webSecurityEnabled;
}

void Settings::setWebSecurityEnabled(const bool value)
{
    m_webSecurityEnabled = value;
}

void Settings::setJavascriptCanOpenWindows(const bool value)
{
    m_javascriptCanOpenWindows = value;
}

bool Settings::javascriptCanOpenWindows() const
{
    return m_javascriptCanOpenWindows;
}

void Settings::setJavascriptCanCloseWindows(const bool value)
{
    m_javascriptCanCloseWindows = value;
}

bool Settings::javascriptCanCloseWindows() const
{
    return m_javascriptCanCloseWindows;
}

void Settings::setWebdriver(const QString& webdriverConfig)
{
    // Parse and validate the configuration
    bool isValidPort;
    QStringList wdCfg = webdriverConfig.split(':');
    if (wdCfg.length() == 1 && wdCfg[0].toInt(&isValidPort) && isValidPort) {
        // Only a PORT was provided
        m_webdriverPort = wdCfg[0];
    } else if (wdCfg.length() == 2 && !wdCfg[0].isEmpty() && wdCfg[1].toInt(&isValidPort) && isValidPort) {
        // Both IP and PORT provided
        m_webdriverIp = wdCfg[0];
        m_webdriverPort = wdCfg[1];
    }
}

QString Settings::webdriver() const
{
    return QString("%1:%2").arg(m_webdriverIp).arg(m_webdriverPort);
}

bool Settings::isWebdriverMode() const
{
    return !m_webdriverPort.isEmpty();
}

void Settings::setWebdriverLogFile(const QString& webdriverLogFile)
{
    m_webdriverLogFile = webdriverLogFile;
}

QString Settings::webdriverLogFile() const
{
    return m_webdriverLogFile;
}

void Settings::setWebdriverLogLevel(const QString& webdriverLogLevel)
{
    m_webdriverLogLevel = webdriverLogLevel;
}

QString Settings::webdriverLogLevel() const
{
    return m_webdriverLogLevel;
}

void Settings::setWebdriverSeleniumGridHub(const QString& hubUrl)
{
    m_webdriverSeleniumGridHub = hubUrl;
}

QString Settings::webdriverSeleniumGridHub() const
{
    return m_webdriverSeleniumGridHub;
}

// private:
void Settings::resetToDefaults()
{
    m_autoLoadImages = true;
    m_cookiesFile = QString();
    m_offlineStoragePath = QString();
    m_offlineStorageDefaultQuota = -1;
    m_localStoragePath = QString();
    m_localStorageDefaultQuota = 5000;
    m_diskCacheEnabled = false;
    m_maxDiskCacheSize = -1;
    m_diskCachePath = QString();
    m_ignoreSslErrors = true;
    m_localUrlAccessEnabled = true;
    m_localToRemoteUrlAccessEnabled = false;
    m_outputEncoding = "UTF-8";
    m_proxyType = "http";
    m_proxyHost.clear();
    m_proxyPort = 1080;
    m_proxyAuthUser.clear();
    m_proxyAuthPass.clear();
    m_scriptArgs.clear();
    m_scriptFile.clear();
    m_unknownOption.clear();
    m_versionFlag = false;
    m_debug = false;
    m_remoteDebugPort = -1;
    m_remoteDebugAutorun = false;
    m_webSecurityEnabled = false;
    m_javascriptCanOpenWindows = true;
    m_javascriptCanCloseWindows = true;
    m_helpFlag = false;
    m_printDebugMessages = false;
    m_sslProtocol = "any";
    // Default taken from Chromium 35.0.1916.153
    m_sslCiphers = ("ECDHE-ECDSA-AES128-GCM-SHA256"
                    ":ECDHE-RSA-AES128-GCM-SHA256"
                    ":DHE-RSA-AES128-GCM-SHA256"
                    ":ECDHE-ECDSA-AES256-SHA"
                    ":ECDHE-ECDSA-AES128-SHA"
                    ":ECDHE-RSA-AES128-SHA"
                    ":ECDHE-RSA-AES256-SHA"
                    ":ECDHE-ECDSA-RC4-SHA"
                    ":ECDHE-RSA-RC4-SHA"
                    ":DHE-RSA-AES128-SHA"
                    ":DHE-DSS-AES128-SHA"
                    ":DHE-RSA-AES256-SHA"
                    ":AES128-GCM-SHA256"
                    ":AES128-SHA"
                    ":AES256-SHA"
                    ":DES-CBC3-SHA"
                    ":RC4-SHA"
                    ":RC4-MD5");
    m_sslCertificatesPath.clear();
    m_sslClientCertificateFile.clear();
    m_sslClientKeyFile.clear();
    m_sslClientKeyPassphrase.clear();
    m_webdriverIp = QString();
    m_webdriverPort = QString();
    m_webdriverLogFile = QString();
    m_webdriverLogLevel = "INFO";
    m_webdriverSeleniumGridHub = QString();
}

void Settings::setProxyAuthPass(const QString& value)
{
    m_proxyAuthPass = value;
}

void Settings::setProxyAuthUser(const QString& value)
{
    m_proxyAuthUser = value;
}

void Settings::setProxyHost(const QString& value)
{
    m_proxyHost = value;
}

void Settings::setProxyPort(const int value)
{
    m_proxyPort = value;
}

bool Settings::helpFlag() const
{
    return m_helpFlag;
}

void Settings::setHelpFlag(const bool value)
{
    m_helpFlag = value;
}

bool Settings::printDebugMessages() const
{
    return m_printDebugMessages;
}

void Settings::setPrintDebugMessages(const bool value)
{
    m_printDebugMessages = value;
}

void Settings::handleSwitch(const QString& sw)
{
    setHelpFlag(sw == "help");
    setVersionFlag(sw == "version");

    if (sw == "wd") {
        setWebdriver(DEFAULT_WEBDRIVER_CONFIG);
    }
}

void Settings::handleOption(const QString& option, const QVariant& value)
{
    bool boolValue = false;
    QStringList booleanFlags;
    booleanFlags << "debug";
    booleanFlags << "disk-cache";
    booleanFlags << "ignore-ssl-errors";
    booleanFlags << "load-images";
    booleanFlags << "local-url-access";
    booleanFlags << "local-to-remote-url-access";
    booleanFlags << "remote-debugger-autorun";
    booleanFlags << "web-security";
    if (booleanFlags.contains(option)) {
        if ((value != "true") && (value != "yes") && (value != "false") && (value != "no")) {
            setUnknownOption(QString("Invalid values for '%1' option.").arg(option));
            return;
        }
        boolValue = (value == "true") || (value == "yes");
    }

    if (option == "cookies-file") {
        setCookiesFile(value.toString());
    }

    if (option == "config") {
        loadJsonFile(value.toString());
    }

    if (option == "debug") {
        setPrintDebugMessages(boolValue);
    }

    if (option == "disk-cache") {
        setDiskCacheEnabled(boolValue);
    }

    if (option == "disk-cache-path") {
        setDiskCachePath(value.toString());
    }

    if (option == "ignore-ssl-errors") {
        setIgnoreSslErrors(boolValue);
    }

    if (option == "load-images") {
        setAutoLoadImages(boolValue);
    }

    if (option == "local-storage-path") {
        setLocalStoragePath(value.toString());
    }

    if (option == "local-storage-quota") {
        setLocalStorageDefaultQuota(value.toInt());
    }

    if (option == "offline-storage-path") {
        setOfflineStoragePath(value.toString());
    }

    if (option == "offline-storage-quota") {
        setOfflineStorageDefaultQuota(value.toInt());
    }

    if (option == "local-url-access") {
        setLocalUrlAccessEnabled(boolValue);
    }

    if (option == "local-to-remote-url-access") {
        setLocalToRemoteUrlAccessEnabled(boolValue);
    }

    if (option == "max-disk-cache-size") {
        setMaxDiskCacheSize(value.toInt());
    }

    if (option == "output-encoding") {
        setOutputEncoding(value.toString());
    }

    if (option == "remote-debugger-autorun") {
        setRemoteDebugAutorun(boolValue);
    }

    if (option == "remote-debugger-port") {
        setDebug(true);
        setRemoteDebugPort(value.toInt());
    }

    if (option == "proxy") {
        setProxy(value.toString());
    }

    if (option == "proxy-type") {
        setProxyType(value.toString());
    }

    if (option == "proxy-auth") {
        setProxyAuth(value.toString());
    }

    if (option == "web-security") {
        setWebSecurityEnabled(boolValue);
    }
    if (option == "ssl-protocol") {
        setSslProtocol(value.toString());
    }
    if (option == "ssl-ciphers") {
        setSslCiphers(value.toString());
    }
    if (option == "ssl-certificates-path") {
        setSslCertificatesPath(value.toString());
    }
    if (option == "ssl-client-certificate-file") {
        setSslClientCertificateFile(value.toString());
    }
    if (option == "ssl-client-key-file") {
        setSslClientKeyFile(value.toString());
    }
    if (option == "ssl-client-key-passphrase") {
        setSslClientKeyPassphrase(value.toByteArray());
    }
    if (option == "webdriver") {
        setWebdriver(value.toString().length() > 0 ? value.toString() : DEFAULT_WEBDRIVER_CONFIG);
    }
    if (option == "webdriver-logfile") {
        setWebdriverLogFile(value.toString());
    }
    if (option == "webdriver-loglevel") {
        setWebdriverLogLevel(value.toString());
    }
    if (option == "webdriver-selenium-grid-hub") {
        setWebdriverSeleniumGridHub(value.toString());
    }
}

void Settings::handleParam(const QString& param, const QVariant& value)
{
    Q_UNUSED(param);

    if (m_scriptFile.isEmpty()) {
        m_scriptFile = value.toString();
    } else {
        m_scriptArgs += value.toString();
    }
}

void Settings::handleError(const QString& error)
{
    setUnknownOption(QString("Error: %1").arg(error));
}

QString Settings::sslProtocol() const
{
    return m_sslProtocol;
}

void Settings::setSslProtocol(const QString& sslProtocolName)
{
    m_sslProtocol = sslProtocolName.toLower();
}

QString Settings::sslCiphers() const
{
    return m_sslCiphers;
}

void Settings::setSslCiphers(const QString& sslCiphersName)
{
    // OpenSSL cipher strings are case sensitive.
    m_sslCiphers = sslCiphersName;
}

QString Settings::sslCertificatesPath() const
{
    return m_sslCertificatesPath;
}

void Settings::setSslCertificatesPath(const QString& sslCertificatesPath)
{
    QFileInfo sslPathInfo = QFileInfo(sslCertificatesPath);
    if (sslPathInfo.isDir()) {
        if (sslCertificatesPath.endsWith('/')) {
            m_sslCertificatesPath = sslCertificatesPath + "*";
        } else {
            m_sslCertificatesPath = sslCertificatesPath + "/*";
        }
    } else {
        m_sslCertificatesPath = sslCertificatesPath;
    }
}

QString Settings::sslClientCertificateFile() const
{
    return m_sslClientCertificateFile;
}

void Settings::setSslClientCertificateFile(const QString& sslClientCertificateFile)
{
    m_sslClientCertificateFile = sslClientCertificateFile;
}

QString Settings::sslClientKeyFile() const
{
    return m_sslClientKeyFile;
}

void Settings::setSslClientKeyFile(const QString& sslClientKeyFile)
{
    m_sslClientKeyFile = sslClientKeyFile;
}

QByteArray Settings::sslClientKeyPassphrase() const
{
    return m_sslClientKeyPassphrase;
}

void Settings::setSslClientKeyPassphrase(const QByteArray& sslClientKeyPassphrase)
{
    m_sslClientKeyPassphrase = sslClientKeyPassphrase;
}
