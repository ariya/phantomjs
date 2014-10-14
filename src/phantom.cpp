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

#include "phantom.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QWebPage>
#include <QDebug>
#include <QMetaObject>
#include <QMetaProperty>

#include "consts.h"
#include "terminal.h"
#include "utils.h"
#include "webpage.h"
#include "webserver.h"
#include "repl.h"
#include "system.h"
#include "callback.h"
#include "cookiejar.h"
#include "childprocess.h"

static Phantom *phantomInstance = NULL;

// private:
Phantom::Phantom(QObject *parent)
    : QObject(parent)
    , m_terminated(false)
    , m_returnValue(0)
    , m_filesystem(0)
    , m_system(0)
    , m_childprocess(0)
{
    QStringList args = QApplication::arguments();

    // Prepare the configuration object based on the command line arguments.
    // Because this object will be used by other classes, it needs to be ready ASAP.
    m_config.init(&args);
    // Apply debug configuration as early as possible
    Utils::printDebugMessages = m_config.printDebugMessages();
}

void Phantom::init()
{
    if (m_config.helpFlag()) {
        Terminal::instance()->cout(QString("%1").arg(m_config.helpText()));
        Terminal::instance()->cout("Any of the options that accept boolean values ('true'/'false') can also accept 'yes'/'no'.");
        Terminal::instance()->cout("");
        Terminal::instance()->cout("Without any argument, PhantomJS will launch in interactive mode (REPL).");
        Terminal::instance()->cout("");
        Terminal::instance()->cout("Documentation can be found at the web site, http://phantomjs.org.");
        Terminal::instance()->cout("");
        m_terminated = true;
        return;
    }

    if (m_config.versionFlag()) {
        m_terminated = true;
        Terminal::instance()->cout(QString("%1").arg(PHANTOMJS_VERSION_STRING));
        return;
    }

    if (!m_config.unknownOption().isEmpty()) {
        Terminal::instance()->cerr(m_config.unknownOption());
        m_terminated = true;
        return;
    }

    // Initialize the CookieJar
    CookieJar::instance(m_config.cookiesFile());

    m_page = new WebPage(this, QUrl::fromLocalFile(m_config.scriptFile()));
    m_pages.append(m_page);

    QString proxyType = m_config.proxyType();
    if (proxyType != "none") {
        if (m_config.proxyHost().isEmpty()) {
            QNetworkProxyFactory::setUseSystemConfiguration(true);
        } else {
            QNetworkProxy::ProxyType networkProxyType = QNetworkProxy::HttpProxy;

            if (proxyType == "socks5") {
                networkProxyType = QNetworkProxy::Socks5Proxy;
            }

            if(!m_config.proxyAuthUser().isEmpty() && !m_config.proxyAuthPass().isEmpty()) {
                QNetworkProxy proxy(networkProxyType, m_config.proxyHost(), m_config.proxyPort(), m_config.proxyAuthUser(), m_config.proxyAuthPass());
                QNetworkProxy::setApplicationProxy(proxy);
            } else {
                QNetworkProxy proxy(networkProxyType, m_config.proxyHost(), m_config.proxyPort());
                QNetworkProxy::setApplicationProxy(proxy);
            }
        }
    }

    // Set output encoding
    Terminal::instance()->setEncoding(m_config.outputEncoding());

    // Set script file encoding
    m_scriptFileEnc.setEncoding(m_config.scriptEncoding());

    connect(m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),
            SLOT(printConsoleMessage(QString)));
    connect(m_page, SIGNAL(initialized()),
            SLOT(onInitialized()));

    m_defaultPageSettings[PAGE_SETTINGS_LOAD_IMAGES] = QVariant::fromValue(m_config.autoLoadImages());
    m_defaultPageSettings[PAGE_SETTINGS_JS_ENABLED] = QVariant::fromValue(true);
    m_defaultPageSettings[PAGE_SETTINGS_XSS_AUDITING] = QVariant::fromValue(false);
    m_defaultPageSettings[PAGE_SETTINGS_USER_AGENT] = QVariant::fromValue(m_page->userAgent());
    m_defaultPageSettings[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE] = QVariant::fromValue(m_config.localToRemoteUrlAccessEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_WEB_SECURITY_ENABLED] = QVariant::fromValue(m_config.webSecurityEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS] = QVariant::fromValue(m_config.javascriptCanOpenWindows());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS] = QVariant::fromValue(m_config.javascriptCanCloseWindows());
    m_page->applySettings(m_defaultPageSettings);

    setLibraryPath(QFileInfo(m_config.scriptFile()).dir().absolutePath());
}

// public:
Phantom *Phantom::instance() {
    if (NULL == phantomInstance) {
        phantomInstance = new Phantom();
        phantomInstance->init();
    }
    return phantomInstance;
}

Phantom::~Phantom()
{
    // Nothing to do: cleanup is handled by QObject relationships
}

QStringList Phantom::args() const
{
    return m_config.scriptArgs();
}

QVariantMap Phantom::defaultPageSettings() const
{
    return m_defaultPageSettings;
}

QString Phantom::outputEncoding() const
{
    return Terminal::instance()->getEncoding();
}

void Phantom::setOutputEncoding(const QString &encoding)
{
    Terminal::instance()->setEncoding(encoding);
}

bool Phantom::execute()
{
    if (m_terminated)
        return false;

#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "Phantom - execute: Configuration";
    const QMetaObject* configMetaObj = m_config.metaObject();
    for (int i = 0, ilen = configMetaObj->propertyCount(); i < ilen; ++i) {
        qDebug() << "    " << i << configMetaObj->property(i).name() << ":" << m_config.property(configMetaObj->property(i).name()).toString();
    }

    qDebug() << "Phantom - execute: Script & Arguments";
    qDebug() << "    " << "script:" << m_config.scriptFile();
    QStringList args = m_config.scriptArgs();
    for (int i = 0, ilen = args.length(); i < ilen; ++i) {
        qDebug() << "    " << i << "arg:" << args.at(i);
    }
#endif

    if (m_config.isWebdriverMode()) {                                   // Remote WebDriver mode requested
        qDebug() << "Phantom - execute: Starting Remote WebDriver mode";

        Terminal::instance()->cout("PhantomJS is launching GhostDriver...");
        if (!Utils::injectJsInFrame(":/ghostdriver/main.js", m_scriptFileEnc, QDir::currentPath(), m_page->mainFrame(), true)) {
            m_returnValue = -1;
            return false;
        }
    } else if (m_config.scriptFile().isEmpty()) {                       // REPL mode requested
        qDebug() << "Phantom - execute: Starting REPL mode";

        // Create the REPL: it will launch itself, no need to store this variable.
        REPL::getInstance(m_page->mainFrame(), this);
    } else {                                                            // Load the User Script
        qDebug() << "Phantom - execute: Starting normal mode";

        if (m_config.debug()) {
            // Debug enabled
            if (!Utils::loadJSForDebug(m_config.scriptFile(), m_scriptFileEnc, QDir::currentPath(), m_page->mainFrame(), m_config.remoteDebugAutorun())) {
                m_returnValue = -1;
                return false;
            }
            m_page->showInspector(m_config.remoteDebugPort());
        } else {
            if (!Utils::injectJsInFrame(m_config.scriptFile(), m_scriptFileEnc, QDir::currentPath(), m_page->mainFrame(), true)) {
                m_returnValue = -1;
                return false;
            }
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
    return m_page->libraryPath();
}

void Phantom::setLibraryPath(const QString &libraryPath)
{
    m_page->setLibraryPath(libraryPath);
}

QString Phantom::scriptName() const
{
    return QFileInfo(m_config.scriptFile()).fileName();
}

QVariantMap Phantom::version() const
{
    QVariantMap result;
    result["major"] = PHANTOMJS_VERSION_MAJOR;
    result["minor"] = PHANTOMJS_VERSION_MINOR;
    result["patch"] = PHANTOMJS_VERSION_PATCH;
    return result;
}

QObject *Phantom::page() const
{
    return m_page;
}

Config *Phantom::config()
{
    return &m_config;
}

bool Phantom::printDebugMessages() const
{
    return m_config.printDebugMessages();
}

bool Phantom::areCookiesEnabled() const
{
    return CookieJar::instance()->isEnabled();
}

void Phantom::setCookiesEnabled(const bool value)
{
    if (value) {
        CookieJar::instance()->enable();
    } else {
        CookieJar::instance()->disable();
    }
}

bool Phantom::webdriverMode() const
{
    return m_config.isWebdriverMode();
}

// public slots:
QObject *Phantom::createWebPage()
{
    WebPage *page = new WebPage(this);

    // Store pointer to the page for later cleanup
    m_pages.append(page);
    // Apply default settings to the page
    page->applySettings(m_defaultPageSettings);

    // Show web-inspector if in debug mode
    if (m_config.debug()) {
        page->showInspector(m_config.remoteDebugPort());
    }

    return page;
}

QObject* Phantom::createWebServer()
{
    WebServer *server = new WebServer(this);
    m_servers.append(server);
    return server;
}

QObject *Phantom::createFilesystem()
{
    if (!m_filesystem)
        m_filesystem = new FileSystem(this);

    return m_filesystem;
}

QObject *Phantom::createSystem()
{
    if (!m_system) {
        m_system = new System(this);

        QStringList systemArgs;
        systemArgs += m_config.scriptFile();
        systemArgs += m_config.scriptArgs();
        m_system->setArgs(systemArgs);
    }

    return m_system;
}

QObject *Phantom::_createChildProcess()
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

void Phantom::loadModule(const QString &moduleSource, const QString &filename)
{
    if (m_terminated)
        return;

   QString scriptSource =
      "(function(require, exports, module) {" +
      moduleSource +
      "}.call({}," +
      "require.cache['" + filename + "']._getRequire()," +
      "require.cache['" + filename + "'].exports," +
      "require.cache['" + filename + "']" +
      "));";
   m_page->mainFrame()->evaluateJavaScript(scriptSource, filename);
}

bool Phantom::injectJs(const QString &jsFilePath)
{
    QString pre = "";
    qDebug() << "Phantom - injectJs:" << jsFilePath;

    // If in Remote Webdriver Mode, we need to manipulate the PATH, to point it to a resource in `ghostdriver.qrc`
    if (webdriverMode()) {
        pre = ":/ghostdriver/";
        qDebug() << "Phantom - injectJs: prepending" << pre;
    }

    if (m_terminated)
        return false;

    return Utils::injectJsInFrame(pre + jsFilePath, libraryPath(), m_page->mainFrame());
}

void Phantom::exit(int code)
{
    if (m_config.debug()) {
        Terminal::instance()->cout("Phantom::exit() called but not quitting in debug mode.");
    } else {
        doExit(code);
    }
}

void Phantom::debugExit(int code)
{
    doExit(code);
}

// private slots:
void Phantom::printConsoleMessage(const QString &message)
{
    Terminal::instance()->cout(message);
}

void Phantom::onInitialized()
{
    // Add 'phantom' object to the global scope
    m_page->mainFrame()->addToJavaScriptWindowObject("phantom", this);

    // Bootstrap the PhantomJS scope
    m_page->mainFrame()->evaluateJavaScript(
                Utils::readResourceFileUtf8(":/bootstrap.js"),
                QString("phantomjs://bootstrap.js")
                );
}

bool Phantom::setCookies(const QVariantList &cookies)
{
    // Delete all the cookies from the CookieJar
    CookieJar::instance()->clearCookies();
    // Add a new set of cookies
    return CookieJar::instance()->addCookiesFromMap(cookies);
}

QVariantList Phantom::cookies() const
{
    // Return all the Cookies in the CookieJar, as a list of Maps (aka JSON in JS space)
    return CookieJar::instance()->cookiesToMap();
}

bool Phantom::addCookie(const QVariantMap &cookie)
{
    return CookieJar::instance()->addCookieFromMap(cookie);
}

bool Phantom::deleteCookie(const QString &cookieName)
{
    if (!cookieName.isEmpty()) {
        return CookieJar::instance()->deleteCookie(cookieName);
    }
    return false;
}

void Phantom::clearCookies()
{
    CookieJar::instance()->clearCookies();
}


// private:
void Phantom::doExit(int code)
{
    if (m_config.debug())
    {
        Utils::cleanupFromDebug();
    }

    emit aboutToExit(code);
    m_terminated = true;
    m_returnValue = code;
    foreach (QPointer<WebPage> page, m_pages) {
        if (!page)
            continue;

        // stop processing of JavaScript code by loading a blank page
        page->mainFrame()->setUrl(QUrl(QString("about:blank")));
        // delay deletion into the event loop, direct deletion can trigger crashes
        page->deleteLater();
    }
    m_pages.clear();
    m_page = 0;
    QApplication::instance()->exit(code);
}
