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

#include <iostream>
#include <QDebug>

#include <gifwriter.h>
#include "consts.h"
#include "utils.h"

#include "phantom.h"

#define JS_MOUSEEVENT_CLICK_WEBELEMENT  "(function (target) { " \
                                            "var evt = document.createEvent('MouseEvents');" \
                                            "evt.initMouseEvent(\"click\", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);" \
                                            "target.dispatchEvent(evt);" \
                                        "})(this);"
#define JS_INCLUDE_SCRIPT_TAG           "var el = document.createElement('script');" \
                                        "el.onload = %2 || null;" \
                                        "el.src = '%1';" \
                                        "document.body.appendChild(el);"

#define PHANTOMJS_PDF_DPI 72            // Different defaults. OSX: 72, X11: 75(?), Windows: 96

// public:
Phantom::Phantom(QObject *parent)
    : QObject(parent)
    , m_proxyPort(1080)
    , m_returnValue(0)
    , m_converter(0)
    , m_netAccessMan(0)
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_page.setPalette(palette);

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
        if (arg.startsWith("--upload-file") && argIterator.hasNext()) {
            const QString &fileInfoString = argIterator.next();
            QStringList fileInfo = fileInfoString.split("=");
            const QString &tag = fileInfo.at(0);
            const QString &fileName = fileInfo.at(1);
            m_page.m_allowedFiles[tag] = fileName;
            continue;
        }
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
            m_proxyHost = arg.mid(8).trimmed();
            if (m_proxyHost.lastIndexOf(':') > 0) {
                bool ok = true;
                int port = m_proxyHost.mid(m_proxyHost.lastIndexOf(':') + 1).toInt(&ok);
                if (ok) {
                    m_proxyHost = m_proxyHost.left(m_proxyHost.lastIndexOf(':')).trimmed();
                    m_proxyPort = port;
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

    if (m_proxyHost.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
        QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif
    } else {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, m_proxyHost, m_proxyPort);
        QNetworkProxy::setApplicationProxy(proxy);
    }

    // The remaining arguments are available for the script.
    while (argIterator.hasNext()) {
        const QString &arg = argIterator.next();
        m_args += arg;
    }

    // Provide WebPage with a non-standard Network Access Manager
    m_netAccessMan = new NetworkAccessManager(this, diskCacheEnabled, ignoreSslErrors);
    m_page.setNetworkAccessManager(m_netAccessMan);

    connect(m_page.mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(inject()));
    connect(&m_page, SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));

    m_page.settings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages);
    m_page.settings()->setAttribute(QWebSettings::PluginsEnabled, pluginsEnabled);

    m_page.settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    m_page.settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    m_page.settings()->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    m_page.settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
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

    m_page.mainFrame()->evaluateJavaScript(m_script);
    return true;
}

int Phantom::returnValue() const
{
    return m_returnValue;
}

QString Phantom::loadStatus() const
{
    return m_loadStatus;
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

void Phantom::setClipRect(const QVariantMap &size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    int top = size.value("top").toInt();
    int left = size.value("left").toInt();

    if (w >= 0 && h >= 0)
        m_clipRect = QRect(left, top, w, h);
}

QVariantMap Phantom::clipRect() const
{
    QVariantMap result;
    result["width"] = m_clipRect.width();
    result["height"] = m_clipRect.height();
    result["top"] = m_clipRect.top();
    result["left"] = m_clipRect.left();
    return result;
}

void Phantom::setPaperSize(const QVariantMap &size)
{
    m_paperSize = size;
}

QVariantMap Phantom::paperSize() const
{
    return m_paperSize;
}

// public slots:
void Phantom::exit(int code)
{
    m_returnValue = code;
    disconnect(&m_page, SIGNAL(loadFinished(bool)), this, SLOT(finish(bool)));
    QTimer::singleShot(0, qApp, SLOT(quit()));
}

void Phantom::open(const QString &address)
{
    qDebug() << "Opening address: " << qPrintable(address);
    m_page.triggerAction(QWebPage::Stop);
    m_loadStatus = "loading";
    m_page.mainFrame()->load(address);
}

bool Phantom::render(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QDir dir;
    dir.mkpath(fileInfo.absolutePath());

    if (fileName.endsWith(".pdf", Qt::CaseInsensitive))
        return renderPdf(fileName);

    QSize viewportSize = m_page.viewportSize();

    QSize pageSize = m_page.mainFrame()->contentsSize();

    QSize bufferSize;
    if (!m_clipRect.isEmpty()) {
        bufferSize = m_clipRect.size();
    } else {
        bufferSize = m_page.mainFrame()->contentsSize();
    }

    if (pageSize.isEmpty())
        return false;

    QImage buffer(bufferSize, QImage::Format_ARGB32);
    buffer.fill(qRgba(255, 255, 255, 0));
    QPainter p(&buffer);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    m_page.setViewportSize(pageSize);

    if (!m_clipRect.isEmpty()) {
        p.translate(-m_clipRect.left(), -m_clipRect.top());
        m_page.mainFrame()->render(&p, QRegion(m_clipRect));
    } else {
        m_page.mainFrame()->render(&p);
    }

    p.end();
    m_page.setViewportSize(viewportSize);

    if (fileName.toLower().endsWith(".gif")) {
        return exportGif(buffer, fileName);
    }

    return buffer.save(fileName);
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

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
void Phantom::setFormInputFile(QWebElement el, const QString &fileTag)
{
    m_page.m_nextFileTag = fileTag;
    el.evaluateJavaScript(JS_MOUSEEVENT_CLICK_WEBELEMENT);
}
#endif

// private slots:
void Phantom::finish(bool success)
{
    m_loadStatus = success ? "success" : "fail";
    m_page.mainFrame()->evaluateJavaScript(m_script);
}

void Phantom::inject()
{
    m_page.mainFrame()->addToJavaScriptWindowObject("phantom", this);
}

bool Phantom::renderPdf(const QString &fileName)
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setResolution(PHANTOMJS_PDF_DPI);
    QVariantMap paperSize = m_paperSize;

    if (paperSize.isEmpty()) {
        const QSize pageSize = m_page.mainFrame()->contentsSize();
        paperSize.insert("width", QString::number(pageSize.width()) + "px");
        paperSize.insert("height", QString::number(pageSize.height()) + "px");
        paperSize.insert("border", "0px");
    }

    if (paperSize.contains("width") && paperSize.contains("height")) {
        const QSizeF sizePt(ceil(stringToPointSize(paperSize.value("width").toString())),
                            ceil(stringToPointSize(paperSize.value("height").toString())));
        printer.setPaperSize(sizePt, QPrinter::Point);
    } else if (paperSize.contains("format")) {
        const QPrinter::Orientation orientation = paperSize.contains("orientation")
                && paperSize.value("orientation").toString().compare("landscape", Qt::CaseInsensitive) == 0 ?
                    QPrinter::Landscape : QPrinter::Portrait;
        printer.setOrientation(orientation);
        static const struct {
            QString format;
            QPrinter::PaperSize paperSize;
        } formats[] = {
            { "A3", QPrinter::A3 },
            { "A4", QPrinter::A4 },
            { "A5", QPrinter::A5 },
            { "Legal", QPrinter::Legal },
            { "Letter", QPrinter::Letter },
            { "Tabloid", QPrinter::Tabloid }
        };
        printer.setPaperSize(QPrinter::A4); // Fallback
        for (uint i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
            if (paperSize.value("format").toString().compare(formats[i].format, Qt::CaseInsensitive) == 0) {
                printer.setPaperSize(formats[i].paperSize);
                break;
            }
        }
    } else {
        return false;
    }

    const qreal border = paperSize.contains("border") ?
                floor(stringToPointSize(paperSize.value("border").toString())) : 0;
    printer.setPageMargins(border, border, border, border, QPrinter::Point);

    m_page.mainFrame()->print(&printer);
    return true;
}

// private:
qreal Phantom::stringToPointSize(const QString &string)
{
    static const struct {
        QString unit;
        qreal factor;
    } units[] = {
        { "mm", 72 / 25.4 },
        { "cm", 72 / 2.54 },
        { "in", 72 },
        { "px", 72.0 / PHANTOMJS_PDF_DPI / 2.54 },
        { "", 72.0 / PHANTOMJS_PDF_DPI / 2.54 }
    };
    for (uint i = 0; i < sizeof(units) / sizeof(units[0]); ++i) {
        if (string.endsWith(units[i].unit)) {
            QString value = string;
            value.chop(units[i].unit.length());
            return value.toDouble() * units[i].factor;
        }
    }
    return 0;
}
