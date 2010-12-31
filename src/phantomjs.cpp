/*
  This file is part of the PhantomJS project from Ofi Labs.

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

#include <QtGui>
#include <QtWebKit>
#include <iostream>

#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
#error Use Qt 4.7 or later version
#endif

class WebPage: public QWebPage
{
    Q_OBJECT
public:
    WebPage(QObject *parent = 0);

public slots:
    bool shouldInterruptJavaScript();

protected:
    QString userAgentForUrl(const QUrl &url) const;

private:
    QString m_userAgent;
    friend class Phantom;
};

WebPage::WebPage(QObject *parent)
    : QWebPage(parent)
{
    m_userAgent = QWebPage::userAgentForUrl(QUrl());
}

bool WebPage::shouldInterruptJavaScript()
{
    QApplication::processEvents(QEventLoop::AllEvents, 42);
    return false;
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    Q_UNUSED(url);
    return m_userAgent;
}

class Phantom: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList arguments READ arguments)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(QString loadStatus READ loadStatus)
    Q_PROPERTY(QString storage READ storage WRITE setStorage)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent)
    Q_PROPERTY(QVariantMap viewportSize READ viewportSize WRITE setViewportSize)

public:
    Phantom(QObject *parent = 0);

    QStringList arguments() const;

    QString content() const;
    void setContent(const QString &content);

    void execute(const QString &fileName);
    int returnValue() const;

    QString loadStatus() const;

    void setStorage(const QString &value);
    QString storage() const;

    void setUserAgent(const QString &ua);
    QString userAgent() const;

    void setViewportSize(const QVariantMap &size);
    QVariantMap viewportSize() const;

public slots:
    void exit(int code = 0);
    void log(const QString &msg);
    void open(const QString &address);
    void sleep(int ms);

private slots:
    void inject();
    void finish(bool);

private:
    QStringList m_arguments;
    QString m_loadStatus;
    WebPage m_page;
    int m_returnValue;
    QString m_script;
    QString m_storage;
};

Phantom::Phantom(QObject *parent)
    : QObject(parent)
    , m_returnValue(0)
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_page.setPalette(palette);

    // first argument: program name (phantomjs)
    // second argument: script name
    m_arguments = QApplication::arguments();
    m_arguments.removeFirst();
    m_arguments.removeFirst();

    connect(m_page.mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(inject()));
    connect(&m_page, SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));

    m_page.settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    m_page.settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_page.settings()->setLocalStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    m_page.settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    // Ensure we have document.body.
    m_page.mainFrame()->setHtml("<html><body></body></html>");
}

QStringList Phantom::arguments() const
{
     return m_arguments;
}

QString Phantom::content() const
{
    return m_page.mainFrame()->toHtml();
}

void Phantom::setContent(const QString &content)
{
    m_page.mainFrame()->setHtml(content);
}

void Phantom::execute(const QString &fileName)
{
    QFile file;
    file.setFileName(fileName);
    if (!file.open(QFile::ReadOnly)) {
        std::cerr << "Can't open " << qPrintable(fileName) << std::endl << std::endl;
        exit(1);
        return;
    }
    m_script = file.readAll();
    file.close();

    m_page.mainFrame()->evaluateJavaScript(m_script);
}

void Phantom::exit(int code)
{
    m_returnValue = code;
    QTimer::singleShot(0, qApp, SLOT(quit()));
}

void Phantom::finish(bool success)
{
    m_loadStatus = success ? "success" : "fail";
    m_page.mainFrame()->evaluateJavaScript(m_script);
}

void Phantom::inject()
{
    m_page.mainFrame()->addToJavaScriptWindowObject("phantom", this);
}

QString Phantom::loadStatus() const
{
    return m_loadStatus;
}

void Phantom::log(const QString &msg)
{
    std::cout << qPrintable(msg) << std::endl;
}

void Phantom::open(const QString &address)
{
    m_page.triggerAction(QWebPage::Stop);
    m_loadStatus = "loading";
    m_page.mainFrame()->setUrl(address);
}

int Phantom::returnValue() const
{
    return m_returnValue;
}

void Phantom::sleep(int ms)
{
    QTime startTime = QTime::currentTime();
    while (true) {
        QApplication::processEvents(QEventLoop::AllEvents, 25);
        if (startTime.msecsTo(QTime::currentTime()) > ms)
            break;
    }
}

void Phantom::setStorage(const QString &value)
{
    m_storage = value;
}

QString Phantom::storage() const
{
    return m_storage;
}

void Phantom::setUserAgent(const QString &ua)
{
    m_page.m_userAgent = ua;
}

QString Phantom::userAgent() const
{
    return m_page.m_userAgent;
}

void Phantom::setViewportSize(const QVariantMap &size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    if (w > 0 && h > 0)
        m_page.setViewportSize(QSize(w, h));
}

QVariantMap Phantom::viewportSize() const
{
    QVariantMap result;
    QSize size = m_page.viewportSize();
    result["width"] = size.width();
    result["height"] = size.height();
    return result;
}

#include "phantomjs.moc"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "phantomjs script.js" << std::endl << std::endl;
        return 1;
    }

    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/phantomjs-icon.png"));
    app.setApplicationName("PhantomJS");
    app.setOrganizationName("Ofi Labs");
    app.setOrganizationDomain("www.ofilabs.com");
    app.setApplicationVersion("1.0");

    Phantom phantom;
    phantom.execute(QString::fromLocal8Bit(argv[1]));
    app.exec();
    return phantom.returnValue();
}
