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

#include <QtGui>
#include <QtWebKit>
#include <QDir>
#include <QFileInfo>
#include <QFile>

#include "consts.h"
#include "terminal.h"
#include "utils.h"
#include "webpage.h"


// public:
Phantom::Phantom(QObject *parent)
    : QObject(parent)
    , m_terminated(false)
    , m_returnValue(0)
    , m_filesystem(0)
{
    // second argument: script name
    QStringList args = QApplication::arguments();

    // Skip the first argument, i.e. the application executable (phantomjs).
    args.removeFirst();

    m_config.init(&args);

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

    m_page = new WebPage(this, &m_config);
    m_pages.append(m_page);

    if (m_config.scriptFile().isEmpty()) {
        Utils::showUsage();
        return;
    }

    if (m_config.proxyHost().isEmpty()) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, m_config.proxyHost(), m_config.proxyPort());
        QNetworkProxy::setApplicationProxy(proxy);
    }

    // Set output encoding
    Terminal::instance()->setEncoding(m_config.outputEncoding());

    // Set script file encoding
    m_scriptFileEnc.setEncoding(m_config.scriptEncoding());

    connect(m_page, SIGNAL(javaScriptConsoleMessageSent(QString, int, QString)),
            SLOT(printConsoleMessage(QString, int, QString)));

    m_defaultPageSettings[PAGE_SETTINGS_LOAD_IMAGES] = QVariant::fromValue(m_config.autoLoadImages());
    m_defaultPageSettings[PAGE_SETTINGS_LOAD_PLUGINS] = QVariant::fromValue(m_config.pluginsEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_JS_ENABLED] = QVariant::fromValue(true);
    m_defaultPageSettings[PAGE_SETTINGS_XSS_AUDITING] = QVariant::fromValue(false);
    m_defaultPageSettings[PAGE_SETTINGS_USER_AGENT] = QVariant::fromValue(m_page->userAgent());
    m_defaultPageSettings[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE] = QVariant::fromValue(m_config.localToRemoteUrlAccessEnabled());
    m_page->applySettings(m_defaultPageSettings);

    setLibraryPath(QFileInfo(m_config.scriptFile()).dir().absolutePath());

    // Add 'phantom' and 'fs' object to the global scope
    m_page->mainFrame()->addToJavaScriptWindowObject("phantom", this);

    QFile f(":/bootstrap.js");
    f.open(QFile::ReadOnly); //< It's OK to assume this succeed. If it doesn't, we have a bigger problem.
    m_page->mainFrame()->evaluateJavaScript(QString::fromUtf8(f.readAll()));
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

    if (m_config.scriptFile().isEmpty())
        return false;

    if (!Utils::injectJsInFrame(m_config.scriptFile(), m_scriptFileEnc, QDir::currentPath(), m_page->mainFrame(), true)) {
        m_returnValue = -1;
        return false;
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

// public slots:
QObject *Phantom::createWebPage()
{
    WebPage *page = new WebPage(this, &m_config);
    m_pages.append(page);
    page->applySettings(m_defaultPageSettings);
    page->setLibraryPath(QFileInfo(m_config.scriptFile()).dir().absolutePath());
    return page;
}

QObject *Phantom::createFilesystem()
{
    if (!m_filesystem)
        m_filesystem = new FileSystem(this);

    return m_filesystem;
}

QString Phantom::loadModuleSource(const QString &name)
{
    QString moduleSource;
    QString moduleSourceFilePath = ":/modules/" + name + ".js";

    QFile f(moduleSourceFilePath);
    if (f.open(QFile::ReadOnly)) {
        moduleSource = QString::fromUtf8(f.readAll());
        f.close();
    }

    return moduleSource;
}

bool Phantom::injectJs(const QString &jsFilePath)
{
    return Utils::injectJsInFrame(jsFilePath, libraryPath(), m_page->mainFrame());
}

void Phantom::exit(int code)
{
    m_terminated = true;
    m_returnValue = code;
    qDeleteAll(m_pages);
    m_pages.clear();
    m_page = 0;
    QApplication::instance()->exit(code);
}

// private slots:
void Phantom::printConsoleMessage(const QString &message, int lineNumber, const QString &source)
{
    QString msg = message;
    if (!source.isEmpty())
        msg = source + ":" + QString::number(lineNumber) + " " + msg;
    Terminal::instance()->cout(msg);
}
