/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2010 Ariya Hidayat <ariya.hidayat@gmail.com>

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

#include <iostream>

#include <QtGui>
#include <QtWebKit>

#include <gifwriter.h>
#include "consts.h"
#include "utils.h"
#include "webpage.h"


// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
#define CONSTRUCT_WEBPAGE "window.WebPage = function(){\n" \
    "var page = phantom.createWebPage();\n" \
    "page.open = function (url, callback) {\n" \
    "    this.loadStatusChanged.connect(callback);\n" \
    "    this.openUrl(url);\n" \
    "};" \
    "return page;" \
    "}"


Phantom::Phantom(QObject *parent)
    : QObject(parent)
    , m_returnValue(0)
    , m_converter(0)
    , m_netAccessMan(0)
{
    m_page = new WebPage(this);

    QString proxyHost;
    int proxyPort = 1080;
    bool autoLoadImages = true;
    bool pluginsEnabled = false;
    bool diskCacheEnabled = false;
    bool ignoreSslErrors = false;

    // second argument: script name
    QStringList args = QApplication::arguments();

    // Skip the first argument, i.e. the application executable (phantomjs).
    args.removeFirst();

    // Handle all command-line options.
    QStringListIterator argIterator(args);
    while (argIterator.hasNext()) {
        const QString &arg = argIterator.next();
        if (arg == "--load-images=yes") {
            autoLoadImages = true;
            continue;
        }
        if (arg == "--load-images=no") {
            autoLoadImages = false;
            continue;
        }
        if (arg == "--load-plugins=yes") {
            pluginsEnabled = true;
            continue;
        }
        if (arg == "--load-plugins=no") {
            pluginsEnabled = false;
            continue;
        }
        if (arg == "--disk-cache=yes") {
            diskCacheEnabled = true;
            continue;
        }
        if (arg == "--disk-cache=no") {
            diskCacheEnabled = false;
            continue;
        }
        if (arg == "--ignore-ssl-errors=yes") {
            ignoreSslErrors = true;
            continue;
        }
        if (arg == "--ignore-ssl-errors=no") {
            ignoreSslErrors = false;
            continue;
        }
        if (arg.startsWith("--proxy=")) {
            proxyHost = arg.mid(8).trimmed();
            if (proxyHost.lastIndexOf(':') > 0) {
                bool ok = true;
                int port = proxyHost.mid(proxyHost.lastIndexOf(':') + 1).toInt(&ok);
                if (ok) {
                    proxyHost = proxyHost.left(proxyHost.lastIndexOf(':')).trimmed();
                    proxyPort = port;
                }
            }
            continue;
        }
        if (arg.startsWith("--")) {
            qFatal("Unknown option '%s'", qPrintable(arg));
            exit(-1);
            return;
        } else {
            m_scriptFile = arg;
            break;
        }
    }

    if (m_scriptFile.isEmpty()) {
        Utils::showUsage();
        return;
    }

    if (proxyHost.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
        QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif
    } else {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, proxyHost, proxyPort);
        QNetworkProxy::setApplicationProxy(proxy);
    }

    // The remaining arguments are available for the script.
    while (argIterator.hasNext()) {
        const QString &arg = argIterator.next();
        m_args += arg;
    }

#if 0
    // Provide WebPage with a non-standard Network Access Manager
    m_netAccessMan = new NetworkAccessManager(this, diskCacheEnabled, ignoreSslErrors);
    m_page->setNetworkAccessManager(m_netAccessMan);

    m_page->settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages);
    m_page->settings()->setAttribute(QWebSettings::PluginsEnabled, pluginsEnabled);

    m_page->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    m_page->settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    m_page->settings()->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    m_page->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    m_page->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_page->settings()->setLocalStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#endif
#endif

    m_page->mainFrame()->addToJavaScriptWindowObject("phantom", this);
    m_page->mainFrame()->evaluateJavaScript(CONSTRUCT_WEBPAGE);
}

QStringList Phantom::args() const
{
    return m_args;
}

bool Phantom::execute()
{
    if (m_scriptFile.isEmpty())
        return false;

    QFile file;
    file.setFileName(m_scriptFile);
    if (!file.open(QFile::ReadOnly)) {
        std::cerr << "Can't open '" << qPrintable(m_scriptFile) << "'" << std::endl << std::endl;
        exit(1);
        return false;
    }
    m_script = QString::fromUtf8(file.readAll());
    file.close();

    if (m_script.startsWith("#!")) {
        m_script.prepend("//");
    }

    if (m_scriptFile.endsWith(".coffee")) {
        if (!m_converter)
            m_converter = new CSConverter(this);
        m_script = m_converter->convert(m_script);
    }

    m_page->mainFrame()->evaluateJavaScript(m_script);
    return true;
}

int Phantom::returnValue() const
{
    return m_returnValue;
}

QVariantMap Phantom::version() const
{
    QVariantMap result;
    result["major"] = PHANTOMJS_VERSION_MAJOR;
    result["minor"] = PHANTOMJS_VERSION_MINOR;
    result["patch"] = PHANTOMJS_VERSION_PATCH;
    return result;
}

QObject *Phantom::createWebPage()
{
    return new WebPage(this);
}

void Phantom::exit(int code)
{
    m_returnValue = code;
    QTimer::singleShot(0, qApp, SLOT(quit()));
}
