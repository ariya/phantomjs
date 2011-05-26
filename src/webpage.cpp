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

#include "webpage.h"

#include <iostream>
#include <math.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPrinter>
#include <QWebFrame>
#include <QWebPage>

#include <gifwriter.h>

class CustomPage: public QWebPage
{
public:
    CustomPage(QObject *parent = 0)
        : QWebPage(parent)
    {
        m_userAgent = QWebPage::userAgentForUrl(QUrl());
    }

public slots:
    bool shouldInterruptJavaScript() {
        QApplication::processEvents(QEventLoop::AllEvents, 42);
        return false;
    }

protected:
    void javaScriptAlert(QWebFrame *originatingFrame, const QString &msg) {
        Q_UNUSED(originatingFrame);
        std::cout << "JavaScript alert: " << qPrintable(msg) << std::endl;
    }

    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID) {
        if (!sourceID.isEmpty())
            std::cout << qPrintable(sourceID) << ":" << lineNumber << " ";
        std::cout << qPrintable(message) << std::endl;
    }

    QString userAgentForUrl(const QUrl &url) const {
        Q_UNUSED(url);
        return m_userAgent;
    }

private:
    QString m_userAgent;
    friend class WebPage;
};

WebPage::WebPage(QObject *parent)
    : QObject(parent)
{
    setObjectName("WebPage");
    m_webPage = new CustomPage(this);
    m_mainFrame = m_webPage->mainFrame();

    connect(m_webPage, SIGNAL(loadFinished(bool)), SLOT(finish(bool)));

    // Start with transparent background.
    QPalette palette = m_webPage->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_webPage->setPalette(palette);

    // Page size does not need to take scrollbars into account.
    m_mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    m_webPage->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    m_webPage->settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    m_webPage->settings()->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    m_webPage->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    m_webPage->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_webPage->settings()->setLocalStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
#endif

    // Ensure we have at least document.body.
    m_mainFrame->setHtml("<html><body></body></html>");

    m_webPage->setViewportSize(QSize(400, 300));
}

QWebFrame *WebPage::mainFrame()
{
    return m_mainFrame;
}

QString WebPage::content() const
{
    return m_mainFrame->toHtml();
}

void WebPage::setContent(const QString &content)
{
    m_mainFrame->setHtml(content);
}

void WebPage::setUserAgent(const QString &ua)
{
    m_webPage->m_userAgent = ua;
}

QString WebPage::userAgent() const
{
    return m_webPage->m_userAgent;
}

void WebPage::setViewportSize(const QVariantMap &size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    if (w > 0 && h > 0)
        m_webPage->setViewportSize(QSize(w, h));
}

QVariantMap WebPage::viewportSize() const
{
    QVariantMap result;
    QSize size = m_webPage->viewportSize();
    result["width"] = size.width();
    result["height"] = size.height();
    return result;
}

void WebPage::setClipRect(const QVariantMap &size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    int top = size.value("top").toInt();
    int left = size.value("left").toInt();

    if (w >= 0 && h >= 0)
        m_clipRect = QRect(left, top, w, h);
}

QVariantMap WebPage::clipRect() const
{
    QVariantMap result;
    result["width"] = m_clipRect.width();
    result["height"] = m_clipRect.height();
    result["top"] = m_clipRect.top();
    result["left"] = m_clipRect.left();
    return result;
}

void WebPage::setPaperSize(const QVariantMap &size)
{
    m_paperSize = size;
}

QVariantMap WebPage::paperSize() const
{
    return m_paperSize;
}

QVariant WebPage::evaluate(const QString &code)
{
    QString function = "(" + code + ")()";
    return m_mainFrame->evaluateJavaScript(function);
}

void WebPage::finish(bool ok)
{
    QString status = ok ? "success" : "fail";
    emit loadStatusChanged(status);
}

void WebPage::openUrl(const QString &address)
{
    m_webPage->triggerAction(QWebPage::Stop);
    m_mainFrame->load(address);
}

bool WebPage::render(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QDir dir;
    dir.mkpath(fileInfo.absolutePath());

    if (fileName.endsWith(".pdf", Qt::CaseInsensitive))
        return renderPdf(fileName);

    QSize viewportSize = m_webPage->viewportSize();

    QSize pageSize = m_mainFrame->contentsSize();

    QSize bufferSize;
    if (!m_clipRect.isNull()) {
        bufferSize = m_clipRect.size();
    } else {
        bufferSize = m_mainFrame->contentsSize();
    }

    if (pageSize.isEmpty())
        return false;

    QImage buffer(bufferSize, QImage::Format_ARGB32);
    buffer.fill(qRgba(255, 255, 255, 0));
    QPainter p(&buffer);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    m_webPage->setViewportSize(pageSize);

    if (!m_clipRect.isEmpty()) {
        p.translate(-m_clipRect.left(), -m_clipRect.top());
        m_mainFrame->render(&p, QRegion(m_clipRect));
    } else {
        m_mainFrame->render(&p);
    }

    p.end();
    m_webPage->setViewportSize(viewportSize);

    if (fileName.toLower().endsWith(".gif")) {
        return exportGif(buffer, fileName);
    }

    return buffer.save(fileName);
}

#define PHANTOMJS_PDF_DPI 72            // Different defaults. OSX: 72, X11: 75(?), Windows: 96

qreal stringToPointSize(const QString &string)
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

bool WebPage::renderPdf(const QString &fileName)
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setResolution(PHANTOMJS_PDF_DPI);
    QVariantMap paperSize = m_paperSize;

    if (paperSize.isEmpty()) {
        const QSize pageSize = m_mainFrame->contentsSize();
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

    m_mainFrame->print(&printer);
    return true;
}
