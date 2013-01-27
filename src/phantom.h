/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

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

#ifndef PHANTOM_H
#define PHANTOM_H

#include <QPointer>

#include "csconverter.h"
#include "filesystem.h"
#include "encoding.h"
#include "config.h"
#include "system.h"
#include "childprocess.h"

class WebPage;
class CustomPage;
class WebServer;

class Phantom : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QVariantMap defaultPageSettings READ defaultPageSettings)
    Q_PROPERTY(QString libraryPath READ libraryPath WRITE setLibraryPath)
    Q_PROPERTY(QString outputEncoding READ outputEncoding WRITE setOutputEncoding)
    Q_PROPERTY(QString scriptName READ scriptName)
    Q_PROPERTY(QVariantMap version READ version)
    Q_PROPERTY(QObject *page READ page)
    Q_PROPERTY(bool cookiesEnabled READ areCookiesEnabled WRITE setCookiesEnabled)
    Q_PROPERTY(QVariantList cookies READ cookies WRITE setCookies)
    Q_PROPERTY(bool webdriverMode READ webdriverMode)

private:
    // Private constructor: the Phantom class is a singleton
    Phantom(QObject *parent = 0);
    void init();

public:
    static Phantom *instance();
    virtual ~Phantom();

    QStringList args() const;

    QVariantMap defaultPageSettings() const;

    QString outputEncoding() const;
    void setOutputEncoding(const QString &encoding);

    bool execute();
    int returnValue() const;

    QString libraryPath() const;
    void setLibraryPath(const QString &libraryPath);

    QString scriptName() const;

    QVariantMap version() const;

    QObject *page() const;

    /**
     * Pointer to the Config loaded at startup.
     * The configuration is determined by the commandline parameters.
     *
     * @brief config
     * @return Pointer to the current Config(uration)
     */
    Config *config();

    bool printDebugMessages() const;

    bool areCookiesEnabled() const;
    void setCookiesEnabled(const bool value);

    bool webdriverMode() const;

    /**
     * Create `child_process` module instance
     */
    Q_INVOKABLE QObject *_createChildProcess();

public slots:
    QObject *createWebPage();
    QObject *createWebServer();
    QObject *createFilesystem();
    QObject *createSystem();
    QObject *createCallback();
    void loadModule(const QString &moduleSource, const QString &filename);
    bool injectJs(const QString &jsFilePath);

    /**
     * Allows to set cookies into the CookieJar.
     * Pages will be able to access only the cookies they are supposed to see given their URL.
     *
     * Cookies are expected in the format:
     * <pre>
     * {
     *   "name"     : "cookie name (string)",
     *   "value"    : "cookie value (string)",
     *   "domain"   : "cookie domain (string)",
     *   "path"     : "cookie path (string, optional)",
     *   "httponly" : "http only cookie (boolean, optional)",
     *   "secure"   : "secure cookie (boolean, optional)",
     *   "expires"  : "expiration date (string, GMT format, optional)"
     * }
     * </pre>
     * @brief setCookies
     * @param cookies Expects a QList of QVariantMaps
     * @return Boolean "true" if at least 1 cookie was set
     */
    bool setCookies(const QVariantList &cookies);
    /**
     * All the Cookies in the CookieJar
     *
     * @see WebPage::setCookies for details on the format
     * @brief cookies
     * @return QList of QVariantMap cookies visible to this Page, at the current URL.
     */
    QVariantList cookies() const;
    /**
     * Add a Cookie (in QVariantMap format) into the CookieJar
     * @see WebPage::setCookies for details on the format
     * @brief addCookie
     * @param cookie Cookie in QVariantMap format
     * @return Boolean "true" if cookie was added
     */
    bool addCookie(const QVariantMap &cookie);
    /**
     * Delete cookie by name from the CookieJar
     * @brief deleteCookie
     * @param cookieName Name of the Cookie to delete
     * @return Boolean "true" if cookie was deleted
     */
    bool deleteCookie(const QString &cookieName);
    /**
     * Delete All Cookies from the CookieJar
     * @brief clearCookies
     */
    void clearCookies();

    // exit() will not exit in debug mode. debugExit() will always exit.
    void exit(int code = 0);
    void debugExit(int code = 0);

signals:
    void aboutToExit(int code);

private slots:
    void printConsoleMessage(const QString &msg);

    void onInitialized();

private:
    void doExit(int code);

    Encoding m_scriptFileEnc;
    WebPage *m_page;
    bool m_terminated;
    int m_returnValue;
    QString m_script;
    QVariantMap m_defaultPageSettings;
    FileSystem *m_filesystem;
    System *m_system;
    ChildProcess *m_childprocess;
    QList<QPointer<WebPage> > m_pages;
    QList<QPointer<WebServer> > m_servers;
    Config m_config;

    friend class CustomPage;
};

#endif // PHANTOM_H
