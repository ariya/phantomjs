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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QNetworkProxy>
#include <QVariant>

class QCommandLine;

class Config: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString cookiesFile READ cookiesFile WRITE setCookiesFile)
    Q_PROPERTY(bool diskCacheEnabled READ diskCacheEnabled WRITE setDiskCacheEnabled)
    Q_PROPERTY(int maxDiskCacheSize READ maxDiskCacheSize WRITE setMaxDiskCacheSize)
    Q_PROPERTY(QString diskCachePath READ diskCachePath WRITE setDiskCachePath)
    Q_PROPERTY(bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors)
    Q_PROPERTY(bool localUrlAccessEnabled READ localUrlAccessEnabled WRITE setLocalUrlAccessEnabled)
    Q_PROPERTY(bool localToRemoteUrlAccessEnabled READ localToRemoteUrlAccessEnabled WRITE setLocalToRemoteUrlAccessEnabled)
    Q_PROPERTY(QString outputEncoding READ outputEncoding WRITE setOutputEncoding)
    Q_PROPERTY(QString proxyType READ proxyType WRITE setProxyType)
    Q_PROPERTY(QString proxy READ proxy WRITE setProxy)
    Q_PROPERTY(QString proxyAuth READ proxyAuth WRITE setProxyAuth)
    Q_PROPERTY(QString scriptEncoding READ scriptEncoding WRITE setScriptEncoding)
    Q_PROPERTY(bool webSecurityEnabled READ webSecurityEnabled WRITE setWebSecurityEnabled)
    Q_PROPERTY(QString offlineStoragePath READ offlineStoragePath WRITE setOfflineStoragePath)
    Q_PROPERTY(QString localStoragePath READ localStoragePath WRITE setLocalStoragePath)
    Q_PROPERTY(int localStorageDefaultQuota READ localStorageDefaultQuota WRITE setLocalStorageDefaultQuota)
    Q_PROPERTY(int offlineStorageDefaultQuota READ offlineStorageDefaultQuota WRITE setOfflineStorageDefaultQuota)
    Q_PROPERTY(bool printDebugMessages READ printDebugMessages WRITE setPrintDebugMessages)
    Q_PROPERTY(bool javascriptCanOpenWindows READ javascriptCanOpenWindows WRITE setJavascriptCanOpenWindows)
    Q_PROPERTY(bool javascriptCanCloseWindows READ javascriptCanCloseWindows WRITE setJavascriptCanCloseWindows)
    Q_PROPERTY(QString sslProtocol READ sslProtocol WRITE setSslProtocol)
    Q_PROPERTY(QString sslCiphers READ sslCiphers WRITE setSslCiphers)
    Q_PROPERTY(QString sslCertificatesPath READ sslCertificatesPath WRITE setSslCertificatesPath)
    Q_PROPERTY(QString sslClientCertificateFile READ sslClientCertificateFile WRITE setSslClientCertificateFile)
    Q_PROPERTY(QString sslClientKeyFile READ sslClientKeyFile WRITE setSslClientKeyFile)
    Q_PROPERTY(QByteArray sslClientKeyPassphrase READ sslClientKeyPassphrase WRITE setSslClientKeyPassphrase)
    Q_PROPERTY(QString webdriver READ webdriver WRITE setWebdriver)
    Q_PROPERTY(QString webdriverLogFile READ webdriverLogFile WRITE setWebdriverLogFile)
    Q_PROPERTY(QString webdriverLogLevel READ webdriverLogLevel WRITE setWebdriverLogLevel)
    Q_PROPERTY(QString webdriverSeleniumGridHub READ webdriverSeleniumGridHub WRITE setWebdriverSeleniumGridHub)

public:
    Config(QObject* parent = 0);

    void init(const QStringList* const args);
    void processArgs(const QStringList& args);
    void loadJsonFile(const QString& filePath);

    QString helpText() const;

    bool autoLoadImages() const;
    void setAutoLoadImages(const bool value);

    QString cookiesFile() const;
    void setCookiesFile(const QString& cookiesFile);

    QString offlineStoragePath() const;
    void setOfflineStoragePath(const QString& value);

    int offlineStorageDefaultQuota() const;
    void setOfflineStorageDefaultQuota(int offlineStorageDefaultQuota);

    QString localStoragePath() const;
    void setLocalStoragePath(const QString& value);

    int localStorageDefaultQuota() const;
    void setLocalStorageDefaultQuota(int localStorageDefaultQuota);

    bool diskCacheEnabled() const;
    void setDiskCacheEnabled(const bool value);

    int maxDiskCacheSize() const;
    void setMaxDiskCacheSize(int maxDiskCacheSize);

    QString diskCachePath() const;
    void setDiskCachePath(const QString& value);

    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(const bool value);

    bool localUrlAccessEnabled() const;
    void setLocalUrlAccessEnabled(const bool value);

    bool localToRemoteUrlAccessEnabled() const;
    void setLocalToRemoteUrlAccessEnabled(const bool value);

    QString outputEncoding() const;
    void setOutputEncoding(const QString& value);

    QString proxyType() const;
    void setProxyType(const QString& value);

    QString proxy() const;
    void setProxy(const QString& value);
    QString proxyHost() const;
    int proxyPort() const;

    QString proxyAuth() const;
    void setProxyAuth(const QString& value);
    QString proxyAuthUser() const;
    QString proxyAuthPass() const;
    void setProxyAuthUser(const QString& value);
    void setProxyAuthPass(const QString& value);

    QStringList scriptArgs() const;
    void setScriptArgs(const QStringList& value);

    QString scriptEncoding() const;
    void setScriptEncoding(const QString& value);

    QString scriptLanguage() const;
    void setScriptLanguage(const QString& value);

    QString scriptFile() const;
    void setScriptFile(const QString& value);

    QString unknownOption() const;
    void setUnknownOption(const QString& value);

    bool versionFlag() const;
    void setVersionFlag(const bool value);

    void setDebug(const bool value);
    bool debug() const;

    void setRemoteDebugPort(const int port);
    int remoteDebugPort() const;

    void setRemoteDebugAutorun(const bool value);
    bool remoteDebugAutorun() const;

    bool webSecurityEnabled() const;
    void setWebSecurityEnabled(const bool value);

    bool helpFlag() const;
    void setHelpFlag(const bool value);

    void setPrintDebugMessages(const bool value);
    bool printDebugMessages() const;

    void setJavascriptCanOpenWindows(const bool value);
    bool javascriptCanOpenWindows() const;

    void setJavascriptCanCloseWindows(const bool value);
    bool javascriptCanCloseWindows() const;

    void setSslProtocol(const QString& sslProtocolName);
    QString sslProtocol() const;

    void setSslCiphers(const QString& sslCiphersName);
    QString sslCiphers() const;

    void setSslCertificatesPath(const QString& sslCertificatesPath);
    QString sslCertificatesPath() const;

    void setSslClientCertificateFile(const QString& sslClientCertificateFile);
    QString sslClientCertificateFile() const;

    void setSslClientKeyFile(const QString& sslClientKeyFile);
    QString sslClientKeyFile() const;

    void setSslClientKeyPassphrase(const QByteArray& sslClientKeyPassphrase);
    QByteArray sslClientKeyPassphrase() const;

    void setWebdriver(const QString& webdriverConfig);
    QString webdriver() const;
    bool isWebdriverMode() const;

    void setWebdriverLogFile(const QString& webdriverLogFile);
    QString webdriverLogFile() const;

    void setWebdriverLogLevel(const QString& webdriverLogLevel);
    QString webdriverLogLevel() const;

    void setWebdriverSeleniumGridHub(const QString& hubUrl);
    QString webdriverSeleniumGridHub() const;

public slots:
    void handleSwitch(const QString& sw);
    void handleOption(const QString& option, const QVariant& value);
    void handleParam(const QString& param, const QVariant& value);
    void handleError(const QString& error);

private:
    void resetToDefaults();
    void setProxyHost(const QString& value);
    void setProxyPort(const int value);
    void setAuthUser(const QString& value);
    void setAuthPass(const QString& value);

    QCommandLine* m_cmdLine;
    bool m_autoLoadImages;
    QString m_cookiesFile;
    QString m_offlineStoragePath;
    int m_offlineStorageDefaultQuota;
    QString m_localStoragePath;
    int m_localStorageDefaultQuota;
    bool m_diskCacheEnabled;
    int m_maxDiskCacheSize;
    QString m_diskCachePath;
    bool m_ignoreSslErrors;
    bool m_localUrlAccessEnabled;
    bool m_localToRemoteUrlAccessEnabled;
    QString m_outputEncoding;
    QString m_proxyType;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyAuthUser;
    QString m_proxyAuthPass;
    QStringList m_scriptArgs;
    QString m_scriptEncoding;
    QString m_scriptLanguage;
    QString m_scriptFile;
    QString m_unknownOption;
    bool m_versionFlag;
    QString m_authUser;
    QString m_authPass;
    bool m_debug;
    int m_remoteDebugPort;
    bool m_remoteDebugAutorun;
    bool m_webSecurityEnabled;
    bool m_helpFlag;
    bool m_printDebugMessages;
    bool m_javascriptCanOpenWindows;
    bool m_javascriptCanCloseWindows;
    QString m_sslProtocol;
    QString m_sslCiphers;
    QString m_sslCertificatesPath;
    QString m_sslClientCertificateFile;
    QString m_sslClientKeyFile;
    QByteArray m_sslClientKeyPassphrase;
    QString m_webdriverIp;
    QString m_webdriverPort;
    QString m_webdriverLogFile;
    QString m_webdriverLogLevel;
    QString m_webdriverSeleniumGridHub;
};

#endif // CONFIG_H
