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
    Q_PROPERTY(bool appCacheEnabled READ appCacheEnabled WRITE setAppCacheEnabled)
    Q_PROPERTY(QString appCachePath READ appCachePath WRITE setAppCachePath)
    Q_PROPERTY(int appCacheDefaultQuota READ appCacheDefaultQuota WRITE setAppCacheDefaultQuota)
    Q_PROPERTY(QString cookiesFile READ cookiesFile WRITE setCookiesFile)
    Q_PROPERTY(bool diskCacheEnabled READ diskCacheEnabled WRITE setDiskCacheEnabled)
    Q_PROPERTY(int maxDiskCacheSize READ maxDiskCacheSize WRITE setMaxDiskCacheSize)
    Q_PROPERTY(bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors)
    Q_PROPERTY(bool indexedDbEnabled READ indexedDbEnabled WRITE setIndexedDbEnabled)
    Q_PROPERTY(QString indexedDbPath READ indexedDbPath WRITE setIndexedDbPath)
    Q_PROPERTY(int indexedDbDefaultQuota READ indexedDbDefaultQuota WRITE setIndexedDbDefaultQuota)
    Q_PROPERTY(bool localToRemoteUrlAccessEnabled READ localToRemoteUrlAccessEnabled WRITE setLocalToRemoteUrlAccessEnabled)
    Q_PROPERTY(QString outputEncoding READ outputEncoding WRITE setOutputEncoding)
    Q_PROPERTY(QString proxyType READ proxyType WRITE setProxyType)
    Q_PROPERTY(QString proxy READ proxy WRITE setProxy)
    Q_PROPERTY(QString proxyAuth READ proxyAuth WRITE setProxyAuth)
    Q_PROPERTY(QString scriptEncoding READ scriptEncoding WRITE setScriptEncoding)
    Q_PROPERTY(bool webSecurityEnabled READ webSecurityEnabled WRITE setWebSecurityEnabled)
    Q_PROPERTY(bool localStorageEnabled READ localStorageEnabled WRITE setLocalStorageEnabled)
    Q_PROPERTY(QString localStoragePath READ localStoragePath WRITE setLocalStoragePath)
    Q_PROPERTY(bool printDebugMessages READ printDebugMessages WRITE setPrintDebugMessages)
    Q_PROPERTY(bool javascriptCanOpenWindows READ javascriptCanOpenWindows WRITE setJavascriptCanOpenWindows)
    Q_PROPERTY(bool javascriptCanCloseWindows READ javascriptCanCloseWindows WRITE setJavascriptCanCloseWindows)
    Q_PROPERTY(QString sslProtocol READ sslProtocol WRITE setSslProtocol)
    Q_PROPERTY(QString webdriver READ webdriver WRITE setWebdriver)
    Q_PROPERTY(QString webdriverSeleniumGridHub READ webdriverSeleniumGridHub WRITE setWebdriverSeleniumGridHub)

public:
    Config(QObject *parent = 0);

    void init(const QStringList *const args);
    void processArgs(const QStringList &args);
    void loadJsonFile(const QString &filePath);

    QString helpText() const;

    bool autoLoadImages() const;
    void setAutoLoadImages(const bool value);

    bool appCacheEnabled() const;
    void setAppCacheEnabled(const bool value);

    QString appCachePath() const;
    void setAppCachePath(const QString &value);

    int appCacheDefaultQuota() const;
    void setAppCacheDefaultQuota(int appCacheDefaultQuota);

    QString cookiesFile() const;
    void setCookiesFile(const QString &cookiesFile);

    bool indexedDbEnabled() const;
    void setIndexedDbEnabled(const bool value);

    QString indexedDbPath() const;
    void setIndexedDbPath(const QString &value);

    int indexedDbDefaultQuota() const;
    void setIndexedDbDefaultQuota(int indexedDbDefaultQuota);

    bool localStorageEnabled() const;
    void setLocalStorageEnabled(const bool value);

    QString localStoragePath() const;
    void setLocalStoragePath(const QString &value);

    bool diskCacheEnabled() const;
    void setDiskCacheEnabled(const bool value);

    int maxDiskCacheSize() const;
    void setMaxDiskCacheSize(int maxDiskCacheSize);

    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(const bool value);

    bool localToRemoteUrlAccessEnabled() const;
    void setLocalToRemoteUrlAccessEnabled(const bool value);

    QString outputEncoding() const;
    void setOutputEncoding(const QString &value);

    QString proxyType() const;
    void setProxyType(const QString value);

    QString proxy() const;
    void setProxy(const QString &value);
    QString proxyHost() const;
    int proxyPort() const;

    QString proxyAuth() const;
    void setProxyAuth(const QString &value);
    QString proxyAuthUser() const;
    QString proxyAuthPass() const;
    void setProxyAuthUser(const QString &value);
    void setProxyAuthPass(const QString &value);

    QStringList scriptArgs() const;
    void setScriptArgs(const QStringList &value);

    QString scriptEncoding() const;
    void setScriptEncoding(const QString &value);

    QString scriptFile() const;
    void setScriptFile(const QString &value);

    QString unknownOption() const;
    void setUnknownOption(const QString &value);

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

    void setWebdriver(const QString& webdriverConfig);
    QString webdriver() const;
    bool isWebdriverMode() const;

    void setWebdriverSeleniumGridHub(const QString& hubUrl);
    QString webdriverSeleniumGridHub() const;

public slots:
    void handleSwitch(const QString &sw);
    void handleOption(const QString &option, const QVariant &value);
    void handleParam(const QString& param, const QVariant &value);
    void handleError(const QString &error);

private:
    void resetToDefaults();
    void setProxyHost(const QString &value);
    void setProxyPort(const int value);
    void setAuthUser(const QString &value);
    void setAuthPass(const QString &value);

    QCommandLine *m_cmdLine;
    bool m_autoLoadImages;
    QString m_cookiesFile;
    bool m_appCacheEnabled;
    QString m_appCachePath;
    int m_appCacheDefaultQuota;
    bool m_indexedDbEnabled;
    QString m_indexedDbPath;
    int m_indexedDbDefaultQuota;
    bool m_localStorageEnabled;
    QString m_localStoragePath;
    bool m_diskCacheEnabled;
    int m_maxDiskCacheSize;
    bool m_ignoreSslErrors;
    bool m_localToRemoteUrlAccessEnabled;
    QString m_outputEncoding;
    QString m_proxyType;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyAuthUser;
    QString m_proxyAuthPass;
    QStringList m_scriptArgs;
    QString m_scriptEncoding;
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
    QString m_webdriver;
    QString m_webdriverSeleniumGridHub;
};

#endif // CONFIG_H
