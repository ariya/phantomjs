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

#if QT_VERSION < QT_VERSION_CHECK(4, 5, 0)
#error Use Qt 4.5 or later version
#endif

#define PHANTOMJS_VERSION_MAJOR  1
#define PHANTOMJS_VERSION_MINOR  1
#define PHANTOMJS_VERSION_PATCH  0
#define PHANTOMJS_VERSION_STRING "1.1.0"

class WebPage: public QWebPage
{
    Q_OBJECT
public:
    WebPage(QObject *parent = 0);

public slots:
    bool shouldInterruptJavaScript();

protected:
    void javaScriptAlert(QWebFrame *originatingFrame, const QString &msg);
    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID);
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

void WebPage::javaScriptAlert(QWebFrame *originatingFrame, const QString &msg)
{
    Q_UNUSED(originatingFrame);
    std::cout << "JavaScript alert: " << qPrintable(msg) << std::endl;
}

void WebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    if (!sourceID.isEmpty())
        std::cout << qPrintable(sourceID) << ":" << lineNumber << " ";
    std::cout << qPrintable(message) << std::endl;
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
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(QString loadStatus READ loadStatus)
    Q_PROPERTY(QString state READ state WRITE setState)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent)
    Q_PROPERTY(QVariantMap version READ version)
    Q_PROPERTY(QVariantMap viewportSize READ viewportSize WRITE setViewportSize)

public:
    Phantom(QObject *parent = 0);

    QStringList args() const;

    QString content() const;
    void setContent(const QString &content);

    void execute(const QString &fileName);
    int returnValue() const;

    QString loadStatus() const;

    void setState(const QString &value);
    QString state() const;

    void setUserAgent(const QString &ua);
    QString userAgent() const;

    QVariantMap version() const;

    void setViewportSize(const QVariantMap &size);
    QVariantMap viewportSize() const;

public slots:
    void exit(int code = 0);
    void open(const QString &address);
    bool render(const QString &fileName);
    void sleep(int ms);

private slots:
    void inject();
    void finish(bool);

private:
    QStringList m_args;
    QString m_loadStatus;
    WebPage m_page;
    int m_returnValue;
    QString m_script;
    QString m_state;
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
    m_args = QApplication::arguments();
    m_args.removeFirst();
    m_args.removeFirst();

    connect(m_page.mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(inject()));
    connect(&m_page, SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));

    m_page.settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    m_page.settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    m_page.settings()->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    m_page.settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);
    m_page.settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_page.settings()->setLocalStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#endif

    // Ensure we have document.body.
    m_page.mainFrame()->setHtml("<html><body></body></html>");

    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
}

QStringList Phantom::args() const
{
     return m_args;
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
    m_script =  QString::fromUtf8(file.readAll());
    file.close();

    m_page.mainFrame()->evaluateJavaScript(m_script);
}

void Phantom::exit(int code)
{
    m_returnValue = code;
    disconnect(&m_page, SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));
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

void Phantom::open(const QString &address)
{
    m_page.triggerAction(QWebPage::Stop);
    m_loadStatus = "loading";
    m_page.mainFrame()->setUrl(address);
}

bool Phantom::render(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QDir dir;
    dir.mkpath(fileInfo.absolutePath());

    if (fileName.toLower().endsWith(".pdf")) {
        QPrinter printer;
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        m_page.mainFrame()->print(&printer);
        return true;
    }

    QSize viewportSize = m_page.viewportSize();
    QSize pageSize = m_page.mainFrame()->contentsSize();
    if (pageSize.isEmpty())
        return false;

    QImage buffer(pageSize, QImage::Format_ARGB32);
    buffer.fill(qRgba(255, 255, 255, 0));
    QPainter p(&buffer);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_page.setViewportSize(pageSize);
    m_page.mainFrame()->render(&p);
    p.end();
    m_page.setViewportSize(viewportSize);
    return buffer.save(fileName);
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

void Phantom::setState(const QString &value)
{
    m_state = value;
}

QString Phantom::state() const
{
    return m_state;
}

void Phantom::setUserAgent(const QString &ua)
{
    m_page.m_userAgent = ua;
}

QString Phantom::userAgent() const
{
    return m_page.m_userAgent;
}

QVariantMap Phantom::version() const
{
    QVariantMap result;
    result["major"] = PHANTOMJS_VERSION_MAJOR;
    result["minor"] = PHANTOMJS_VERSION_MINOR;
    result["patch"] = PHANTOMJS_VERSION_PATCH;
    return result;
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

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/phantomjs-icon.png"));
    app.setApplicationName("PhantomJS");
    app.setOrganizationName("Ofi Labs");
    app.setOrganizationDomain("www.ofilabs.com");
    app.setApplicationVersion(PHANTOMJS_VERSION_STRING);

    Phantom phantom;
    phantom.execute(QString::fromLocal8Bit(argv[1]));
    app.exec();
    return phantom.returnValue();
}
