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

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QMetaProperty>
#include <QScreen>
#include <QStandardPaths>
#include <QWebPage>
#include "phantom.h"
#include "callback.h"
#include "childprocess.h"
#include "consts.h"
#include "repl.h"
#include "system.h"
#include "terminal.h"
#include "webpage.h"
#include "webserver.h"
#include "network/cookiejar.h"

static Phantom* phantomInstance = Q_NULLPTR;

// private members:
Phantom::Phantom(QObject* parent)
    : QObject(parent)
    , m_terminated(false)
    , m_returnValue(0)
    , m_filesystem(Q_NULLPTR)
    , m_system(Q_NULLPTR)
    , m_childprocess(Q_NULLPTR)
{
    QStringList args = QApplication::arguments();

    // Prepare the configuration object based on the command line arguments.
    // Because this object will be used by other classes, it needs to be ready ASAP.
    m_settings = new Settings(this);
    m_settings->init(&args);
}

void Phantom::init()
{
    if (m_settings->helpFlag()) {
        Terminal::instance()->cout(QString("%1").arg(m_settings->helpText()));
        Terminal::instance()->cout("Any of the options that accept boolean values ('true'/'false') can also accept 'yes'/'no'.");
        Terminal::instance()->cout("");
        Terminal::instance()->cout("Without any argument, PhantomJS will launch in interactive mode (REPL).");
        Terminal::instance()->cout("");
        Terminal::instance()->cout("Documentation can be found at the web site, http://phantomjs.org.");
        Terminal::instance()->cout("");
        m_terminated = true;
        return;
    }

    if (m_settings->versionFlag()) {
        m_terminated = true;
        Terminal::instance()->cout(QString("%1").arg(PHANTOMJS_VERSION_STRING));
        return;
    }

    if (!m_settings->unknownOption().isEmpty()) {
        Terminal::instance()->cerr(m_settings->unknownOption());
        m_terminated = true;
        return;
    }

    // Initialize the CookieJar
    m_defaultCookieJar = new CookieJar(m_settings->cookiesFile());

    // set the default DPI
    m_defaultDpi = qRound(QApplication::primaryScreen()->logicalDotsPerInch());

    QWebSettings::setOfflineWebApplicationCachePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (m_settings->offlineStoragePath().isEmpty()) {
        QWebSettings::setOfflineStoragePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    } else {
        QWebSettings::setOfflineStoragePath(m_settings->offlineStoragePath());
    }
    if (m_settings->offlineStorageDefaultQuota() > 0) {
        QWebSettings::setOfflineStorageDefaultQuota(m_settings->offlineStorageDefaultQuota());
    }

    m_hostPage = new PhantomPage(this);
    m_hostPage->bootstrap();

    // Set up proxy if required
    QString proxyType = m_settings->proxyType();
    if (proxyType != "none") {
        setProxy(m_settings->proxyHost(), m_settings->proxyPort(), proxyType, m_settings->proxyAuthUser(), m_settings->proxyAuthPass());
    }

    // Set output encoding
    Terminal::instance()->setEncoding(m_settings->outputEncoding());
    
    connect(m_hostPage, &PhantomPage::consoleMessageReceived, this, &Phantom::printConsoleMessage);
    connect(m_hostPage, &PhantomPage::initialized, this, &Phantom::onInitialized);

    m_defaultPageSettings[PAGE_SETTINGS_LOAD_IMAGES] = QVariant::fromValue(m_settings->autoLoadImages());
    m_defaultPageSettings[PAGE_SETTINGS_JS_ENABLED] = QVariant::fromValue(true);
    m_defaultPageSettings[PAGE_SETTINGS_XSS_AUDITING] = QVariant::fromValue(false);
    
// TODO m_defaultPageSettings[PAGE_SETTINGS_USER_AGENT] = QVariant::fromValue(m_page->userAgent());
    m_defaultPageSettings[PAGE_SETTINGS_USER_AGENT] = QVariant::fromValue(QString("PhantomJS"));
    m_defaultPageSettings[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE] = QVariant::fromValue(m_settings->localToRemoteUrlAccessEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_WEB_SECURITY_ENABLED] = QVariant::fromValue(m_settings->webSecurityEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS] = QVariant::fromValue(m_settings->javascriptCanOpenWindows());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS] = QVariant::fromValue(m_settings->javascriptCanCloseWindows());
    m_defaultPageSettings[PAGE_SETTINGS_DPI] = QVariant::fromValue(m_defaultDpi);
    setLibraryPath(QFileInfo(m_settings->scriptFile()).dir().absolutePath());
}

Phantom::~Phantom() = default;

// public:
Phantom* Phantom::instance()
{
    if (!phantomInstance) {
        phantomInstance = new Phantom();
        phantomInstance->init();
    }
    return phantomInstance;
}

QVariantMap Phantom::defaultPageSettings() const
{
    return m_defaultPageSettings;
}

QString Phantom::outputEncoding() const
{
    return Terminal::instance()->getEncoding();
}

void Phantom::setOutputEncoding(const QString& encoding)
{
    Terminal::instance()->setEncoding(encoding);
}

bool Phantom::execute()
{
    if (m_terminated) {
        return false;
    }

#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "Phantom - execute: Configuration";
    const QMetaObject* configMetaObj = m_settings->metaObject();
    for (int i = 0, ilen = configMetaObj->propertyCount(); i < ilen; ++i) {
        qDebug() << "    " << i << configMetaObj->property(i).name() << ":" << m_settings->property(configMetaObj->property(i).name()).toString();
    }

    qDebug() << "Phantom - execute: Script & Arguments";
    qDebug() << "    " << "script:" << m_settings->scriptFile();
    QStringList args = m_settings->scriptArgs();
    for (int i = 0, ilen = args.length(); i < ilen; ++i) {
        qDebug() << "    " << i << "arg:" << args.at(i);
    }
#endif

    if (m_settings->isWebdriverMode()) {
        // Remote WebDriver mode requested
        qDebug() << "Phantom - execute: Starting Remote WebDriver mode";

        if (m_hostPage->executeFromFile(":/ghostdriver/main.js") == QVariant::Invalid) {
            m_returnValue = -1;
            return false;
        }
    }
    else if (m_settings->scriptFile().isEmpty()) {
        // REPL mode requested
        qDebug() << "Phantom - execute: Starting REPL mode";

        // Create the REPL: it will launch itself, no need to store this variable.
        REPL::getInstance(m_hostPage->mainFrame(), this);
    }
    else {
        // Load the User Script
        qDebug() << "Phantom - execute: Starting normal mode";

        if (!QFileInfo::exists(m_settings->scriptFile())) {
            qDebug() << "Phantom - unable to open file" << m_settings->scriptFile();
            m_returnValue = -1;
            return false;
        }

        if (m_settings->debug()) {
            // Debug enabled
            // TODO: Remote debugging

            m_hostPage->executeFromFile(m_settings->scriptFile());
        }
        else {
            m_hostPage->executeFromFile(m_settings->scriptFile());
        }
    }

    return !m_terminated;
}

int Phantom::returnValue() const
{
    return m_returnValue;
}

QString Phantom::libraryPath() const
{
    QString path = m_libraryPath.dir().absolutePath();
    qDebug() << __PRETTY_FUNCTION__ << path;

    return path;
}

void Phantom::setLibraryPath(const QString& libraryPath)
{
    qDebug() << __PRETTY_FUNCTION__ << libraryPath;
    m_libraryPath = QFileInfo(libraryPath);
}

QVariantMap Phantom::version() const
{
    QVariantMap result;
    result["major"] = PHANTOMJS_VERSION_MAJOR;
    result["minor"] = PHANTOMJS_VERSION_MINOR;
    result["patch"] = PHANTOMJS_VERSION_PATCH;
    return result;
}

Settings* Phantom::settings() const
{
    return m_settings;
}

bool Phantom::printDebugMessages() const
{
    return m_settings->printDebugMessages();
}

bool Phantom::areCookiesEnabled() const
{
    return m_defaultCookieJar->isEnabled();
}

void Phantom::setCookiesEnabled(const bool value)
{
    if (value) {
        m_defaultCookieJar->enable();
    } else {
        m_defaultCookieJar->disable();
    }
}

bool Phantom::webdriverMode() const
{
    return m_settings->isWebdriverMode();
}

// public slots:
QObject* Phantom::createCookieJar(const QString& filePath)
{
    return new CookieJar(filePath, this);
}

QObject* Phantom::createWebPage()
{
    WebPage* page = new WebPage(this);
    page->setCookieJar(m_defaultCookieJar);

    // Store pointer to the page for later cleanup
    m_pages.append(page);
    // Apply default settings to the page
    page->applySettings(m_defaultPageSettings);

    // Show web-inspector if in debug mode
    if (m_settings->debug()) {
        page->showInspector(m_settings->remoteDebugPort());
    }

    return page;
}

QObject* Phantom::createWebServer()
{
    WebServer* server = new WebServer(this);
    m_servers.append(server);
    return server;
}

QObject* Phantom::createFilesystem()
{
    if (!m_filesystem) {
        m_filesystem = new FileSystem(this);
    }

    return m_filesystem;
}

QObject* Phantom::createSystem()
{
    if (!m_system) {
        m_system = new System(this);

        QStringList systemArgs;
        systemArgs += m_settings->scriptFile();
        systemArgs += m_settings->scriptArgs();
        m_system->setArgs(systemArgs);
    }

    return m_system;
}

QObject* Phantom::_createChildProcess()
{
    if (!m_childprocess) {
        m_childprocess = new ChildProcess(this);
    }

    return m_childprocess;
}

QObject* Phantom::createCallback()
{
    return new Callback(this);
}

void Phantom::loadModule(const QString& moduleSource, const QString& filename)
{
    if (m_terminated) {
        return;
    }

    QString scriptSource =
        "(function(require, exports, module) {\n" +
        moduleSource +
        "\n}.call({}," +
        "require.cache['" + filename + "']._getRequire()," +
        "require.cache['" + filename + "'].exports," +
        "require.cache['" + filename + "']" +
        "));";
    m_hostPage->execute(scriptSource);
}

bool Phantom::injectJs(const QString& jsFilePath)
{
    QString pre = "";
    qDebug() << "Phantom - injectJs:" << jsFilePath;

    // If in Remote Webdriver Mode, we need to manipulate the PATH, to point it to a resource in `ghostdriver.qrc`
    if (webdriverMode()) {
        pre = ":/ghostdriver/";
        qDebug() << "Phantom - injectJs: prepending" << pre;
    }

    if (m_terminated) {
        return false;
    }

    return m_hostPage->executeFromFile(pre + jsFilePath) != QVariant::Invalid;
}

void Phantom::setProxy(const QString& ip, const qint16& port, const QString& proxyType, const QString& user, const QString& password)
{
    qDebug() << "Set " << proxyType << " proxy to: " << ip << ":" << port;
    if (ip.isEmpty()) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else {
        QNetworkProxy::ProxyType networkProxyType = QNetworkProxy::HttpProxy;

        if (proxyType == "socks5") {
            networkProxyType = QNetworkProxy::Socks5Proxy;
        }
           QNetworkProxy::setApplicationProxy(proxy);
        } else {
          QNetworkProxy proxy(networkProxyType, ip, port, QString(), QString(), m_config.useProxyForLocalhost());
            QNetworkProxy::setApplicationProxy(proxy);
        }
    }
}

QString Phantom::proxy()
{
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    if (proxy.hostName().isEmpty()) {
        return QString();
    }
    return proxy.hostName() + ":" + QString::number(proxy.port());
}

int Phantom::remoteDebugPort() const
{
    return m_settings->remoteDebugPort();
}

void Phantom::exit(int code)
{
    if (m_settings->debug()) {
        Terminal::instance()->cout("Phantom::exit() called but not quitting in debug mode.");
    } else {
        doExit(code);
    }
}

void Phantom::debugExit(int code)
{
    doExit(code);
}

QString Phantom::resolveRelativeUrl(QString url, QString base)
{
    QUrl u = QUrl::fromEncoded(url.toLatin1());
    QUrl b = QUrl::fromEncoded(base.toLatin1());

    return b.resolved(u).toEncoded();
}

QString Phantom::fullyDecodeUrl(QString url)
{
    return QUrl::fromEncoded(url.toLatin1()).toDisplayString();
}

// private slots:
void Phantom::printConsoleMessage(const QString& message)
{
    Terminal::instance()->cout(message);
}

void Phantom::onInitialized()
{
    // And load the script
    m_hostPage->executeFromFile(m_settings->scriptFile());
}

bool Phantom::setCookies(const QVariantList& cookies)
{
    // Delete all the cookies from the CookieJar
    m_defaultCookieJar->clearCookies();
    // Add a new set of cookies
    return m_defaultCookieJar->addCookiesFromMap(cookies);
}

QVariantList Phantom::cookies() const
{
    // Return all the Cookies in the CookieJar, as a list of Maps (aka JSON in JS space)
    return m_defaultCookieJar->cookiesToMap();
}

bool Phantom::addCookie(const QVariantMap& cookie)
{
    return m_defaultCookieJar->addCookieFromMap(cookie);
}

bool Phantom::deleteCookie(const QString& cookieName)
{
    if (!cookieName.isEmpty()) {
        return m_defaultCookieJar->deleteCookie(cookieName);
    }
    return false;
}

void Phantom::clearCookies()
{
    m_defaultCookieJar->clearCookies();
}

void Phantom::appendPage(WebPage* webPage)
{
    QPointer<WebPage> p(webPage);
    m_pages.append(p);
}

PhantomPage* Phantom::hostPage() const
{
    return m_hostPage;
}

// private:
void Phantom::doExit(int code)
{
    emit aboutToExit(code);
    m_terminated = true;
    m_returnValue = code;

    // Iterate in reverse order so the first page is the last one scheduled for deletion.
    // The first page is the root object, which will be invalidated when it is deleted.
    // This causes an assertion to go off in BridgeJSC.cpp Instance::createRuntimeObject.
    QListIterator<QPointer<WebPage> > i(m_pages);
    i.toBack();
    while (i.hasPrevious()) {
        const QPointer<WebPage> page = i.previous();

        if (!page) {
            continue;
        }

        // stop processing of JavaScript code by loading a blank page
        page->mainFrame()->setUrl(QUrl(QStringLiteral("about:blank")));
        // delay deletion into the event loop, direct deletion can trigger crashes
        page->deleteLater();
    }
    m_pages.clear();

    m_hostPage->deleteLater();
    QApplication::instance()->exit(code);
}
