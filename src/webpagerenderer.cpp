/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2017 Vitaly Slobodin <vitaliy.slobodin@gmail.com>

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

#include <QImageWriter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QWebFrame>
#include <QtMath>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

#include "webpagerenderer.h"

static const struct {
    QString format;
    QPageSize::PageSizeId paperSize;
} formats[] = {
    { "A0", QPageSize::A0 },
    { "A1", QPageSize::A1 },
    { "A2", QPageSize::A2 },
    { "A3", QPageSize::A3 },
    { "A4", QPageSize::A4 },
    { "A5", QPageSize::A5 },
    { "A6", QPageSize::A6 },
    { "A7", QPageSize::A7 },
    { "A8", QPageSize::A8 },
    { "A9", QPageSize::A9 },
    { "B0", QPageSize::B0 },
    { "B1", QPageSize::B1 },
    { "B2", QPageSize::B2 },
    { "B3", QPageSize::B3 },
    { "B4", QPageSize::B4 },
    { "B5", QPageSize::B5 },
    { "B6", QPageSize::B6 },
    { "B7", QPageSize::B7 },
    { "B8", QPageSize::B8 },
    { "B9", QPageSize::B9 },
    { "B10", QPageSize::B10 },
    { "C5E", QPageSize::C5E },
    { "Comm10E", QPageSize::Comm10E },
    { "DLE", QPageSize::DLE },
    { "Executive", QPageSize::Executive },
    { "Folio", QPageSize::Folio },
    { "Ledger", QPageSize::Ledger },
    { "Legal", QPageSize::Legal },
    { "Letter", QPageSize::Letter },
    { "Tabloid", QPageSize::Tabloid }
};

WebPageRenderer::WebPageRenderer(CustomPage* customPage, qreal dpi, QObject* parent)
    : m_customPage(customPage)
    , QObject(parent)
    , m_dpi(dpi)
{
}

qreal WebPageRenderer::stringToPointSize(const QString& string) const
{
    static const struct {
        QString unit;
        qreal factor;
    } units[] = {
        { "mm", 72 / 25.4 },
        { "cm", 72 / 2.54 },
        { "in", 72 },
        { "px", 72.0 / m_dpi },
        { "", 72.0 / m_dpi }
    };
    for (const auto & unit : units) {
        if (string.endsWith(unit.unit)) {
            QString value = string;
            value.chop(unit.unit.length());
            return value.toDouble() * unit.factor;
        }
    }
    return 0;
}

QBuffer* WebPageRenderer::renderPdf(QVariantMap paperSize, QRect clipRect) const
{
    QBuffer* buffer = new QBuffer();
    buffer->open(QBuffer::WriteOnly);
    QPdfWriter* pdfWriter = new QPdfWriter(buffer);
    pdfWriter->setResolution(m_dpi);

    if (paperSize.isEmpty()) {
        const QSize pageSize = m_customPage->mainFrame()->contentsSize();
        paperSize.insert("width", QString::number(pageSize.width()) + "px");
        paperSize.insert("height", QString::number(pageSize.height()) + "px");
        paperSize.insert("margin", "0px");
    }

    if (paperSize.contains("width") && paperSize.contains("height")) {
        const QSizeF sizePt(qCeil(stringToPointSize(paperSize.value("width").toString())),
            qCeil(stringToPointSize(paperSize.value("height").toString())));
        pdfWriter->setPageSize(QPageSize(sizePt, QPageSize::Point));
    } else if (paperSize.contains("format")) {
        const QPageLayout::Orientation orientation = paperSize.contains("orientation")
            && paperSize.value("orientation").toString().compare("landscape", Qt::CaseInsensitive) == 0 ?
            QPageLayout::Landscape : QPageLayout::Portrait;
        pdfWriter->setPageOrientation(orientation);
        pdfWriter->setPageSize(QPageSize(QPageSize::A4)); // Fallback
        for (const auto & format : formats) {
            if (paperSize.value("format").toString().compare(format.format, Qt::CaseInsensitive) == 0) {
                pdfWriter->setPageSize(QPageSize(format.paperSize));
                break;
            }
        }
    } else {
        return Q_NULLPTR;
    }

    if (paperSize.contains("border") && !paperSize.contains("margin")) {
        // backwards compatibility
        paperSize["margin"] = paperSize["border"];
    }

    qreal marginLeft = 0;
    qreal marginTop = 0;
    qreal marginRight = 0;
    qreal marginBottom = 0;

    if (paperSize.contains("margin")) {
        const QVariant margins = paperSize["margin"];
        if (margins.canConvert(QVariant::Map)) {
            const QVariantMap map = margins.toMap();
            marginLeft = printMargin(map, "left");
            marginTop = printMargin(map, "top");
            marginRight = printMargin(map, "right");
            marginBottom = printMargin(map, "bottom");
        } else if (margins.canConvert(QVariant::String)) {
            const qreal margin = stringToPointSize(margins.toString());
            marginLeft = margin;
            marginTop = margin;
            marginRight = margin;
            marginBottom = margin;
        }
    }

    pdfWriter->setPageMargins(QMarginsF(marginLeft, marginTop, marginRight, marginBottom), QPageLayout::Point);

    QPainter* painter = new QPainter(pdfWriter);
    if (clipRect.isEmpty()) {
        m_customPage->mainFrame()->render(painter);
    } else {
        m_customPage->mainFrame()->render(painter, QRegion(clipRect));
    }
    painter->end();

    delete pdfWriter;

    return buffer;
}

bool WebPageRenderer::renderToFile(
    const QString& filename,
    const QString& format,
    const QRect clipRect,
    QVariantMap paperSize,
    RenderMode mode,
    int quality)
{
    QString outputFormat = format.toLower();
    QBuffer* buffer = Q_NULLPTR;
    QImage* image = Q_NULLPTR;

    if (outputFormat != "pdf" && !QImageWriter::supportedImageFormats().contains(outputFormat.toLatin1())) {
        qDebug() << "WebPage - render. Unsupported format" << format;
        return false;
    }

    if (outputFormat == "pdf") {
        buffer = renderPdf(paperSize, clipRect);
    } else {
        image = renderImage(mode, clipRect);
    }

    if (!image && outputFormat != "pdf") {
        return false;
    }

#ifdef Q_OS_WIN
    if (filename == "/dev/stdout" || filename == "/dev/stderr") {

        setStdOutputMode(filename, O_BINARY);
        image->save(filename, outputFormat.toLatin1().constData(), quality);
        setStdOutputMode(filename, O_TEXT);
    }
#endif

    bool successful = false;
    if (image) {
        successful = image->save(filename, format.toLatin1().constData(), quality);
        delete image;
    }

    if (buffer) {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            // TODO: Is it ok?
            successful = file.write(buffer->data()) > 0;
            file.close();
        }
        delete buffer;
    }

    return successful;
}

QString WebPageRenderer::renderToBase64(
    const QString& format,
    const QRect clipRect,
    QVariantMap paperSize,
    RenderMode mode,
    int quality) const
{
    QString outputFormat = format.toLower();

    if (outputFormat != "pdf" && !QImageWriter::supportedImageFormats().contains(outputFormat.toLatin1())) {
        qDebug() << "WebPage - render. Unsupported format" << format;
        return "";
    }

    if (outputFormat == "pdf") {
        QBuffer* buffer = renderPdf(paperSize, clipRect);
        return buffer->buffer().toBase64().data();
    }

    QImage* image = renderImage(mode, clipRect);

    if (!image) {
        return "";
    }

    QByteArray ba;
    QBuffer bu(&ba);
    bu.open(QIODevice::ReadOnly);
    image->save(&bu, outputFormat.toLatin1().constData(), quality);
    delete image;

    return ba.toBase64().data();
}

void WebPageRenderer::setDpi(qreal dpi)
{
    m_dpi = dpi;
}

/*
 * PRIVATE
 */

QImage* WebPageRenderer::renderImage(const RenderMode mode, QRect clipRect) const
{
    QRect frameRect;
    QSize viewportSize = m_customPage->viewportSize();
    if (mode == Viewport) {
        frameRect = QRect(QPoint(0, 0), viewportSize);
    }
    else {
        QSize contentsSize = m_customPage->mainFrame()->contentsSize();
        auto scrollPosition = m_customPage->mainFrame()->scrollPosition();
        contentsSize -= QSize(scrollPosition.x(), scrollPosition.y());
        frameRect = QRect(QPoint(0, 0), contentsSize);
        m_customPage->setViewportSize(contentsSize);
    }

    if (!clipRect.isNull()) {
        frameRect = clipRect;
    }

#ifdef Q_OS_WIN
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
#else
    QImage::Format format = QImage::Format_ARGB32;
#endif

    QImage* buffer = new QImage(frameRect.size(), format);
    buffer->fill(Qt::transparent);

    QPainter painter;

    // We use tiling approach to work-around Qt software rasterizer bug
    // when dealing with very large paint device.
    // See http://code.google.com/p/phantomjs/issues/detail?id=54.
    const int tileSize = 4096;
    int htiles = (buffer->width() + tileSize - 1) / tileSize;
    int vtiles = (buffer->height() + tileSize - 1) / tileSize;
    for (int x = 0; x < htiles; ++x) {
        for (int y = 0; y < vtiles; ++y) {

            QImage tileBuffer(tileSize, tileSize, format);
            tileBuffer.fill(Qt::transparent);

            // Render the web page onto the small tile first
            painter.begin(&tileBuffer);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::TextAntialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.translate(-frameRect.left(), -frameRect.top());
            painter.translate(-x * tileSize, -y * tileSize);
            m_customPage->mainFrame()->render(&painter, QRegion(frameRect));
            painter.end();

            // Copy the tile to the main buffer
            painter.begin(buffer);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.drawImage(x * tileSize, y * tileSize, tileBuffer);
            painter.end();
        }
    }
    if (mode != Viewport) {
        m_customPage->setViewportSize(viewportSize);
    }
    return buffer;
}



qreal WebPageRenderer::printMargin(const QVariantMap& map, const QString& key) const
{
    const QVariant margin = map.value(key);
    if (margin.isValid() && margin.canConvert(QVariant::String)) {
        return stringToPointSize(margin.toString());
    }

    return 0;
}

#ifdef Q_OS_WIN
void WebPageRenderer::setStdOutputMode(const QString& filename, int mode)
{
    // If you write data to a file stream, explicitly flush the code
    // by using fflush before you use _setmode to change the mode
    // see: https://msdn.microsoft.com/en-us/library/tw4k6df8.aspx
    FILE* stream = Q_NULLPTR;
    if (filename == "/dev/stdout") {
        stream = stdout;
    } else if (filename == "/dev/stderr") {
        stream = stderr;
    } else {
        return;
    }

    fflush(stream);
    _setmode(_fileno(stream), mode);
    fflush(stream);
}
#endif
