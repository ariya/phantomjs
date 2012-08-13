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

#include "webpage.h"

#include <math.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkRequest>
#include <QPainter>
#include <QPrinter>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>
#include <QWebInspector>
#include <QMapIterator>
#include <QBuffer>
#include <QDebug>
#include <QImageWriter>

#include <gifwriter.h>

#include "phantom.h"
#include "networkaccessmanager.h"
#include "utils.h"
#include "config.h"
#include "consts.h"
#include "callback.h"

// Ensure we have at least head and body.
#define BLANK_HTML                      "<html><head></head><body></body></html>"
#define CALLBACKS_OBJECT_NAME           "_phantom"
#define INPAGE_CALL_NAME                "window.callPhantom"
#define CALLBACKS_OBJECT_INJECTION      INPAGE_CALL_NAME" = function() { return window."CALLBACKS_OBJECT_NAME".call.call(_phantom, Array.prototype.splice.call(arguments, 0)); };"


/**
  * @class CustomPage
  */
class CustomPage: public QWebPage
{
    Q_OBJECT

public:
    CustomPage(WebPage *parent = 0)
        : QWebPage(parent)
        , m_webPage(parent)
    {
        m_userAgent = QWebPage::userAgentForUrl(QUrl());
        setForwardUnsupportedContent(true);
    }

    bool extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output) {
        Q_UNUSED(option);

        if (extension == ChooseMultipleFilesExtension) {
            static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = QStringList(m_uploadFile);
            return true;
        } else {
            return false;
        }
    }

public slots:
    bool shouldInterruptJavaScript() {
        QApplication::processEvents(QEventLoop::AllEvents, 42);
        return false;
    }

protected:
    bool supportsExtension(Extension extension) const {
        return extension == ChooseMultipleFilesExtension;
    }

    QString chooseFile(QWebFrame *originatingFrame, const QString &oldFile) {
        Q_UNUSED(originatingFrame);
        Q_UNUSED(oldFile);
        return m_uploadFile;
    }

    void javaScriptAlert(QWebFrame *originatingFrame, const QString &msg) {
        Q_UNUSED(originatingFrame);
        emit m_webPage->javaScriptAlertSent(msg);
    }

    bool javaScriptConfirm(QWebFrame *originatingFrame, const QString &msg) {
        Q_UNUSED(originatingFrame);
        return m_webPage->javaScriptConfirm(msg);
    }

    bool javaScriptPrompt(QWebFrame *originatingFrame, const QString &msg, const QString &defaultValue, QString *result) {
        Q_UNUSED(originatingFrame);
        return m_webPage->javaScriptPrompt(msg, defaultValue, result);
    }

    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID) {
        Q_UNUSED(lineNumber);
        Q_UNUSED(sourceID);
        emit m_webPage->javaScriptConsoleMessageSent(message);
    }

    void javaScriptError(const QString &message, int lineNumber, const QString &sourceID, const QString &stack) {
        Q_UNUSED(lineNumber);
        Q_UNUSED(sourceID);
        emit m_webPage->javaScriptErrorSent(message, stack);
    }

    QString userAgentForUrl(const QUrl &url) const {
        Q_UNUSED(url);
        return m_userAgent;
    }

    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type) {
        bool isMainFrame = (frame == m_webPage->m_mainFrame);

        QString navigationType = "Undefined";
        switch (type) {
        case NavigationTypeLinkClicked:
            navigationType = "LinkClicked";
            break;
        case NavigationTypeFormSubmitted:
            navigationType = "FormSubmitted";
            break;
        case NavigationTypeBackOrForward:
            navigationType = "BackOrForward";
            break;
        case NavigationTypeReload:
            navigationType = "Reload";
            break;
        case NavigationTypeFormResubmitted:
            navigationType = "FormResubmitted";
            break;
        case NavigationTypeOther:
            navigationType = "Other";
            break;
        }

        emit m_webPage->navigationRequested(
                    request.url(),                   //< Requested URL
                    navigationType,                  //< Navigation Type
                    !m_webPage->navigationLocked(),  //< Is navigation locked?
                    isMainFrame);                    //< Is main frame?

        return !m_webPage->navigationLocked();
    }

    QWebPage *createWindow (WebWindowType type) {
        Q_UNUSED(type);
        WebPage *newPage;

        // Create a new "raw" WebPage object
        if (m_webPage->ownsPages()) {
            newPage = new WebPage(m_webPage);
        } else {
            newPage = new WebPage(Phantom::instance());
            Phantom::instance()->m_pages.append(newPage);
        }

        // Apply default settings
        newPage->applySettings(Phantom::instance()->defaultPageSettings());

        // Signal JS shim to catch, decorate and store this new child page
        emit m_webPage->rawPageCreated(newPage);

        // Return the new QWebPage to the QWebKit backend
        return newPage->m_customWebPage;
    }

private:
    WebPage *m_webPage;
    QString m_userAgent;
    QString m_uploadFile;
    friend class WebPage;
};


/**
  * Contains the Callback Objects used to regulate callback-traffic from the webpage internal context.
  * It's directly exposed within the webpage JS context,
  * and indirectly in the phantom JS context.
  *
  * @class WebPageCallbacks
  */
class WebpageCallbacks : public QObject
{
    Q_OBJECT

public:
    WebpageCallbacks(QObject *parent = 0)
        : QObject(parent)
        , m_genericCallback(NULL)
        , m_jsConfirmCallback(NULL)
        , m_jsPromptCallback(NULL)
    {
    }

    QObject *getGenericCallback() {
        if (!m_genericCallback) {
            m_genericCallback = new Callback(this);
        }
        return m_genericCallback;
    }

    QObject *getJsConfirmCallback() {
        if (!m_jsConfirmCallback) {
            m_jsConfirmCallback = new Callback(this);
        }
        return m_jsConfirmCallback;
    }

    QObject *getJsPromptCallback() {
        if (!m_jsPromptCallback) {
            m_jsPromptCallback = new Callback(this);
        }
        return m_jsPromptCallback;
    }

public slots:
    QVariant call(const QVariantList &arguments) {
        if (m_genericCallback) {
            return m_genericCallback->call(arguments);
        }
        return QVariant();
    }

private:
    Callback *m_genericCallback;
    Callback *m_jsConfirmCallback;
    Callback *m_jsPromptCallback;

    friend class WebPage;
};


WebPage::WebPage(QObject *parent, const QUrl &baseUrl)
    : REPLCompletable(parent)
    , m_callbacks(NULL)
    , m_navigationLocked(false)
    , m_mousePos(QPoint(0, 0))
    , m_ownsPages(true)
{
    setObjectName("WebPage");
    m_customWebPage = new CustomPage(this);
    m_mainFrame = m_customWebPage->mainFrame();
    m_mainFrame->setHtml(BLANK_HTML, baseUrl);

    Config *phantomCfg = Phantom::instance()->config();

    connect(m_mainFrame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(handleJavaScriptWindowObjectCleared()));
    connect(m_mainFrame, SIGNAL(javaScriptWindowObjectCleared()), SIGNAL(initialized()));
    connect(m_mainFrame, SIGNAL(urlChanged(QUrl)), SIGNAL(urlChanged(QUrl)));
    connect(m_customWebPage, SIGNAL(loadStarted()), SIGNAL(loadStarted()), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(loadFinished(bool)), SLOT(finish(bool)), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(windowCloseRequested()), this, SLOT(close()));

    // Start with transparent background.
    QPalette palette = m_customWebPage->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_customWebPage->setPalette(palette);

    // Set the page Library path
    setLibraryPath(QFileInfo(phantomCfg->scriptFile()).dir().absolutePath());

    // Page size does not need to take scrollbars into account.
    m_mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    m_customWebPage->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    if (phantomCfg->offlineStoragePath().isEmpty()) {
        m_customWebPage->settings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    } else {
        m_customWebPage->settings()->setOfflineStoragePath(phantomCfg->offlineStoragePath());
    }
    if (phantomCfg->offlineStorageDefaultQuota() > 0) {
        m_customWebPage->settings()->setOfflineStorageDefaultQuota(phantomCfg->offlineStorageDefaultQuota());
    }

    m_customWebPage->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    m_customWebPage->settings()->setOfflineWebApplicationCachePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    m_customWebPage->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, true);

    m_customWebPage->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_customWebPage->settings()->setLocalStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

    // Custom network access manager to allow traffic monitoring.
    m_networkAccessManager = new NetworkAccessManager(this, phantomCfg);
    m_customWebPage->setNetworkAccessManager(m_networkAccessManager);
    connect(m_networkAccessManager, SIGNAL(resourceRequested(QVariant)),
            SIGNAL(resourceRequested(QVariant)));
    connect(m_networkAccessManager, SIGNAL(resourceReceived(QVariant)),
            SIGNAL(resourceReceived(QVariant)));

    m_customWebPage->setViewportSize(QSize(400, 300));
}

WebPage::~WebPage()
{
    emit closing(this);
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

QString WebPage::plainText() const
{
    return m_mainFrame->toPlainText();
}

QString WebPage::libraryPath() const
{
   return m_libraryPath;
}

void WebPage::setLibraryPath(const QString &libraryPath)
{
   m_libraryPath = libraryPath;
}

QString WebPage::offlineStoragePath() const
{
    return m_customWebPage->settings()->offlineStoragePath();
}

int WebPage::offlineStorageQuota() const
{
    return m_customWebPage->settings()->offlineStorageDefaultQuota();
}

void WebPage::showInspector(const int port)
{
    m_customWebPage->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    m_inspector = new QWebInspector;
    m_inspector->setPage(m_customWebPage);

    if (port == -1) {
        m_inspector->setVisible(true);
    } else {
        m_customWebPage->setProperty("_q_webInspectorServerPort", port);
    }
}

void WebPage::applySettings(const QVariantMap &def)
{
    QWebSettings *opt = m_customWebPage->settings();

    opt->setAttribute(QWebSettings::AutoLoadImages, def[PAGE_SETTINGS_LOAD_IMAGES].toBool());
    opt->setAttribute(QWebSettings::JavascriptEnabled, def[PAGE_SETTINGS_JS_ENABLED].toBool());
    opt->setAttribute(QWebSettings::XSSAuditingEnabled, def[PAGE_SETTINGS_XSS_AUDITING].toBool());
    opt->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, def[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE].toBool());
    opt->setAttribute(QWebSettings::WebSecurityEnabled, def[PAGE_SETTINGS_WEB_SECURITY_ENABLED].toBool());
    opt->setAttribute(QWebSettings::JavascriptCanOpenWindows, def[PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS].toBool());
    opt->setAttribute(QWebSettings::JavascriptCanCloseWindows, def[PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS].toBool());

    if (def.contains(PAGE_SETTINGS_USER_AGENT))
        m_customWebPage->m_userAgent = def[PAGE_SETTINGS_USER_AGENT].toString();

    if (def.contains(PAGE_SETTINGS_USERNAME))
        m_networkAccessManager->setUserName(def[PAGE_SETTINGS_USERNAME].toString());

    if (def.contains(PAGE_SETTINGS_PASSWORD))
        m_networkAccessManager->setPassword(def[PAGE_SETTINGS_PASSWORD].toString());
}

QString WebPage::userAgent() const
{
    return m_customWebPage->m_userAgent;
}

void WebPage::setNavigationLocked(bool lock)
{
    m_navigationLocked = lock;;
}

bool WebPage::navigationLocked()
{
    return m_navigationLocked;
}

void WebPage::setViewportSize(const QVariantMap &size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    if (w > 0 && h > 0)
        m_customWebPage->setViewportSize(QSize(w, h));
}

QVariantMap WebPage::viewportSize() const
{
    QVariantMap result;
    QSize size = m_customWebPage->viewportSize();
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


void WebPage::setScrollPosition(const QVariantMap &size)
{
    int top = size.value("top").toInt();
    int left = size.value("left").toInt();
    m_scrollPosition = QPoint(left,top);
    m_mainFrame->setScrollPosition(m_scrollPosition);
}

QVariantMap WebPage::scrollPosition() const
{
    QVariantMap result;
    result["top"] = m_scrollPosition.y();
    result["left"] = m_scrollPosition.x();
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

QVariant WebPage::evaluateJavaScript(const QString &code)
{
    QString function = "(" + code + ")()";
    return m_customWebPage->currentFrame()->evaluateJavaScript(
                function,
                QString("phantomjs://webpage.evaluate()"));
}

bool WebPage::javaScriptConfirm(const QString &msg)
{
    if (m_callbacks->m_jsConfirmCallback) {
        QVariant res = m_callbacks->m_jsConfirmCallback->call(QVariantList() << msg);
        if (res.canConvert<bool>()) {
            return res.toBool();
        }
    }
    return false;
}

bool WebPage::javaScriptPrompt(const QString &msg, const QString &defaultValue, QString *result)
{
    if (m_callbacks->m_jsPromptCallback) {
        QVariant res = m_callbacks->m_jsPromptCallback->call(QVariantList() << msg << defaultValue);
        if (!res.isNull() && res.canConvert<QString>()) {
            result->append(res.toString());
            return true;
        }
    }
    return false;
}

void WebPage::finish(bool ok)
{
    QString status = ok ? "success" : "fail";
    emit loadFinished(status);
}

void WebPage::setCustomHeaders(const QVariantMap &headers)
{
    m_networkAccessManager->setCustomHeaders(headers);
}

QVariantMap WebPage::customHeaders() const
{
    return m_networkAccessManager->customHeaders();
}

void WebPage::setCookies(const QVariantList &cookies)
{
    m_networkAccessManager->setCookies(cookies);
}

QVariantList WebPage::cookies() const
{
    return m_networkAccessManager->cookies();
}

void WebPage::openUrl(const QString &address, const QVariant &op, const QVariantMap &settings)
{
    QString operation;
    QByteArray body;
    QNetworkRequest request;

    applySettings(settings);
    m_customWebPage->triggerAction(QWebPage::Stop);

    if (op.type() == QVariant::String)
        operation = op.toString();

    if (op.type() == QVariant::Map) {
        operation = op.toMap().value("operation").toString();
        body = op.toMap().value("data").toByteArray();
        if (op.toMap().contains("headers")) {
            QMapIterator<QString, QVariant> i(op.toMap().value("headers").toMap());
            while (i.hasNext()) {
                i.next();
                request.setRawHeader(i.key().toUtf8(), i.value().toString().toUtf8());
            }
        }
    }

    if (operation.isEmpty())
        operation = "get";

    QNetworkAccessManager::Operation networkOp = QNetworkAccessManager::UnknownOperation;
    operation = operation.toLower();
    if (operation == "get")
        networkOp = QNetworkAccessManager::GetOperation;
    else if (operation == "head")
        networkOp = QNetworkAccessManager::HeadOperation;
    else if (operation == "put")
        networkOp = QNetworkAccessManager::PutOperation;
    else if (operation == "post")
        networkOp = QNetworkAccessManager::PostOperation;
    else if (operation == "delete")
        networkOp = QNetworkAccessManager::DeleteOperation;

    if (networkOp == QNetworkAccessManager::UnknownOperation) {
        m_mainFrame->evaluateJavaScript("console.error('Unknown network operation: " + operation + "');", QString());
        return;
    }

    if (address == "about:blank") {
        m_mainFrame->setHtml(BLANK_HTML);
    } else {
        QUrl url = QUrl::fromEncoded(QByteArray(address.toAscii()));

#if QT_VERSION == QT_VERSION_CHECK(4, 8, 0)
        // Assume local file if scheme is empty
        if (url.scheme().isEmpty()) {
            url.setPath(QFileInfo(url.toString()).absoluteFilePath().prepend("/"));
            url.setScheme("file");
        }
#endif
        request.setUrl(url);
        m_mainFrame->load(request, networkOp, body);
    }
}

void WebPage::release()
{
    close();
}

void WebPage::close() {
    deleteLater();
}

bool WebPage::render(const QString &fileName)
{
    if (m_mainFrame->contentsSize().isEmpty())
        return false;

    QFileInfo fileInfo(fileName);
    QDir dir;
    dir.mkpath(fileInfo.absolutePath());

    if (fileName.endsWith(".pdf", Qt::CaseInsensitive))
        return renderPdf(fileName);

    QImage buffer = renderImage();
    if (fileName.toLower().endsWith(".gif")) {
        return exportGif(buffer, fileName);
    }

    return buffer.save(fileName);
}

QString WebPage::renderBase64(const QByteArray &format)
{
    QByteArray nformat = format.toLower();

    // Check if the given format is supported
    if (QImageWriter::supportedImageFormats().contains(nformat)) {
        QImage rawPageRendering = renderImage();

        // Prepare buffer for writing
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);

        // Writing image to the buffer, using PNG encoding
        rawPageRendering.save(&buffer, nformat);

        return bytes.toBase64();
    }

    // Return an empty string in case an unsupported format was provided
    return "";
}

QImage WebPage::renderImage()
{
    QSize contentsSize = m_mainFrame->contentsSize();
    contentsSize -= QSize(m_scrollPosition.x(), m_scrollPosition.y());
    QRect frameRect = QRect(QPoint(0, 0), contentsSize);
    if (!m_clipRect.isNull())
        frameRect = m_clipRect;

    QSize viewportSize = m_customWebPage->viewportSize();
    m_customWebPage->setViewportSize(contentsSize);

    QImage buffer(frameRect.size(), QImage::Format_ARGB32);
    buffer.fill(qRgba(255, 255, 255, 0));

    QPainter painter;

    // We use tiling approach to work-around Qt software rasterizer bug
    // when dealing with very large paint device.
    // See http://code.google.com/p/phantomjs/issues/detail?id=54.
    const int tileSize = 4096;
    int htiles = (buffer.width() + tileSize - 1) / tileSize;
    int vtiles = (buffer.height() + tileSize - 1) / tileSize;
    for (int x = 0; x < htiles; ++x) {
        for (int y = 0; y < vtiles; ++y) {

            QImage tileBuffer(tileSize, tileSize, QImage::Format_ARGB32);
            tileBuffer.fill(qRgba(255, 255, 255, 0));

            // Render the web page onto the small tile first
            painter.begin(&tileBuffer);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::TextAntialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.translate(-frameRect.left(), -frameRect.top());
            painter.translate(-x * tileSize, -y * tileSize);
            m_mainFrame->render(&painter, QRegion(frameRect));
            painter.end();

            // Copy the tile to the main buffer
            painter.begin(&buffer);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.drawImage(x * tileSize, y * tileSize, tileBuffer);
            painter.end();
        }
    }

    m_customWebPage->setViewportSize(viewportSize);
    return buffer;
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

qreal printMargin(const QVariantMap &map, const QString &key)
{
    const QVariant margin = map.value(key);
    if (margin.isValid() && margin.canConvert(QVariant::String)) {
        return stringToPointSize(margin.toString());
    } else {
        return 0;
    }
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
        paperSize.insert("margin", "0px");
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
            { "A0", QPrinter::A0 },
            { "A1", QPrinter::A1 },
            { "A2", QPrinter::A2 },
            { "A3", QPrinter::A3 },
            { "A4", QPrinter::A4 },
            { "A5", QPrinter::A5 },
            { "A6", QPrinter::A6 },
            { "A7", QPrinter::A7 },
            { "A8", QPrinter::A8 },
            { "A9", QPrinter::A9 },
            { "B0", QPrinter::B0 },
            { "B1", QPrinter::B1 },
            { "B2", QPrinter::B2 },
            { "B3", QPrinter::B3 },
            { "B4", QPrinter::B4 },
            { "B5", QPrinter::B5 },
            { "B6", QPrinter::B6 },
            { "B7", QPrinter::B7 },
            { "B8", QPrinter::B8 },
            { "B9", QPrinter::B9 },
            { "B10", QPrinter::B10 },
            { "C5E", QPrinter::C5E },
            { "Comm10E", QPrinter::Comm10E },
            { "DLE", QPrinter::DLE },
            { "Executive", QPrinter::Executive },
            { "Folio", QPrinter::Folio },
            { "Ledger", QPrinter::Ledger },
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

    printer.setPageMargins(marginLeft, marginTop, marginRight, marginBottom, QPrinter::Point);

    m_mainFrame->print(&printer, this);
    return true;
}

void WebPage::setZoomFactor(qreal zoom)
{
    m_mainFrame->setZoomFactor(zoom);
}

qreal WebPage::zoomFactor() const
{
    return m_mainFrame->zoomFactor();
}

QString WebPage::windowName() const
{
    return m_mainFrame->evaluateJavaScript("window.name;").toString();
}

qreal getHeight(const QVariantMap &map, const QString &key)
{
    QVariant footer = map.value(key);
    if (!footer.canConvert(QVariant::Map)) {
        return 0;
    }
    QVariant height = footer.toMap().value("height");
    if (!height.canConvert(QVariant::String)) {
        return 0;
    }
    return stringToPointSize(height.toString());
}

qreal WebPage::footerHeight() const
{
    return getHeight(m_paperSize, "footer");
}

qreal WebPage::headerHeight() const
{
    return getHeight(m_paperSize, "header");
}

QString getHeaderFooter(const QVariantMap &map, const QString &key, QWebFrame *frame, int page, int numPages)
{
    QVariant header = map.value(key);
    if (!header.canConvert(QVariant::Map)) {
        return QString();
    }
    QVariant callback = header.toMap().value("contents");
    if (callback.canConvert<QObject*>()) {
        Callback* caller = qobject_cast<Callback*>(callback.value<QObject*>());
        if (caller) {
            QVariant ret = caller->call(QVariantList() << page << numPages);
            if (ret.canConvert(QVariant::String)) {
                return ret.toString();
            }
        }
    }
    frame->evaluateJavaScript("console.error('Bad header callback given, use phantom.callback);", QString());
    return QString();
}

QString WebPage::header(int page, int numPages)
{
    return getHeaderFooter(m_paperSize, "header", m_mainFrame, page, numPages);
}

QString WebPage::footer(int page, int numPages)
{
    return getHeaderFooter(m_paperSize, "footer", m_mainFrame, page, numPages);
}

void WebPage::uploadFile(const QString &selector, const QString &fileName)
{
    QWebElement el = m_customWebPage->currentFrame()->findFirstElement(selector);
    if (el.isNull())
        return;

    m_customWebPage->m_uploadFile = fileName;
    el.evaluateJavaScript(JS_ELEMENT_CLICK);
}

bool WebPage::injectJs(const QString &jsFilePath) {
    return Utils::injectJsInFrame(jsFilePath, m_libraryPath, m_customWebPage->currentFrame());
}

void WebPage::_appendScriptElement(const QString &scriptUrl) {
    m_customWebPage->currentFrame()->evaluateJavaScript(QString(JS_APPEND_SCRIPT_ELEMENT).arg(scriptUrl), scriptUrl);
}

QObject *WebPage::_getGenericCallback() {
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getGenericCallback();
}

QObject *WebPage::_getJsConfirmCallback() {
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getJsConfirmCallback();
}

QObject *WebPage::_getJsPromptCallback() {
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getJsPromptCallback();
}

void WebPage::sendEvent(const QString &type, const QVariant &arg1, const QVariant &arg2, const QString &mouseButton)
{
    // Normalize the event "type" to lowercase
    const QString eventType = type.toLower();

    // single keyboard events
    if (eventType == "keydown" || eventType == "keyup") {
        QKeyEvent::Type keyEventType = QEvent::None;
        if (eventType == "keydown")
            keyEventType = QKeyEvent::KeyPress;
        if (eventType == "keyup")
            keyEventType = QKeyEvent::KeyRelease;
        Q_ASSERT(keyEventType != QEvent::None);

        int key = 0;
        QString text;
        if (arg1.type() == QVariant::Char) {
            // a single char was given
            text = arg1.toChar();
            key = text.at(0).toAscii();
        } else if (arg1.type() == QVariant::String) {
            // javascript invokation of a single char
            text = arg1.toString();
            if (!text.isEmpty()) {
                key = text.at(0).toAscii();
            }
        } else {
            // assume a raw integer char code was given
            key = arg1.toInt();
        }
        QKeyEvent *keyEvent = new QKeyEvent(keyEventType, key, Qt::NoModifier, text);
        QApplication::postEvent(m_customWebPage, keyEvent);
        QApplication::processEvents();
        return;
    }

    // sequence of key events: will generate all the single keydown/keyup events
    if (eventType == "keypress") {
        if (arg1.type() == QVariant::String) {
            // this is the case for e.g. sendEvent("...", 'A')
            // but also works with sendEvent("...", "ABCD")
            foreach(QChar typeChar, arg1.toString()) {
                sendEvent("keydown", typeChar);
                sendEvent("keyup", typeChar);
            }
        } else {
            // otherwise we assume a raw integer char-code was given
            sendEvent("keydown", arg1.toInt());
            sendEvent("keyup", arg1.toInt());
        }
        return;
    }

    // mouse events
    if (eventType == "mousedown" ||
            eventType == "mouseup" ||
            eventType == "mousemove" ||
            eventType == "doubleclick") {
        QMouseEvent::Type mouseEventType = QEvent::None;

        // Which mouse button (if it's a click)
        Qt::MouseButton button = Qt::LeftButton;
        Qt::MouseButton buttons = Qt::LeftButton;
        if (mouseButton.toLower() == "middle") {
            button = Qt::MiddleButton;
            buttons = Qt::MiddleButton;
        } else if (mouseButton.toLower() == "right") {
            button = Qt::RightButton;
            buttons = Qt::RightButton;
        }

        // Which mouse event
        if (eventType == "mousedown") {
            mouseEventType = QEvent::MouseButtonPress;
        } else if (eventType == "mouseup") {
            mouseEventType = QEvent::MouseButtonRelease;
        } else if (eventType == "doubleclick") {
            mouseEventType = QEvent::MouseButtonDblClick;
        } else if (eventType == "mousemove") {
            mouseEventType = QEvent::MouseMove;
            button = Qt::NoButton;
            buttons = Qt::NoButton;
        }
        Q_ASSERT(mouseEventType != QEvent::None);

        // Gather coordinates
        if (arg1.isValid() && arg2.isValid()) {
            m_mousePos.setX(arg1.toInt());
            m_mousePos.setY(arg2.toInt());
        }

        // Prepare the Mouse event (no modifiers or other buttons are supported for now)
        qDebug() << "Mouse Event:" << eventType << "(" << mouseEventType << ")" << m_mousePos << ")" << button << buttons;
        QMouseEvent *event = new QMouseEvent(mouseEventType, m_mousePos, button, buttons, Qt::NoModifier);

        // Post and process events
        QApplication::postEvent(m_customWebPage, event);
        QApplication::processEvents();
        return;
    }

    // mouse click events: Qt doesn't provide this as a separate events,
    // so we compose it with a mousedown/mouseup sequence
    if (type == "click") {
        sendEvent("mousedown", arg1, arg2, mouseButton);
        sendEvent("mouseup", arg1, arg2, mouseButton);
        return;
    }
}

QObjectList WebPage::pages() const
{
    QObjectList pages;

    foreach(QObject *p, this->findChildren<WebPage *>()) {
        pages << p;
    }

    return pages;
}

QStringList WebPage::pagesWindowName() const
{
    QStringList pagesWindowName;

    foreach (WebPage *p, this->findChildren<WebPage *>()) {
        pagesWindowName << p->windowName();
    }

    return pagesWindowName;
}

QObject *WebPage::getPage(const QString &windowName) const
{
    foreach (WebPage *p, this->findChildren<WebPage *>()) {
        if (p->windowName() == windowName) {
            return p;
        }
    }
    return NULL;
}

bool WebPage::ownsPages() const
{
    return m_ownsPages;
}

void WebPage::setOwnsPages(const bool owns)
{
    m_ownsPages = owns;
}

int WebPage::framesCount() const
{
    return m_customWebPage->currentFrame()->childFrames().count();
}

int WebPage::childFramesCount() const //< deprecated
{
    return this->framesCount();
}

QStringList WebPage::framesName() const
{
    QStringList framesName;

    foreach(QWebFrame *f, m_customWebPage->currentFrame()->childFrames()) {
        framesName << f->frameName();
    }
    return framesName;
}

QStringList WebPage::childFramesName() const //< deprecated
{
    return this->framesName();
}

bool WebPage::switchToFrame(const QString &frameName)
{
    foreach(QWebFrame * f, m_customWebPage->currentFrame()->childFrames()) {
        if (f->frameName() == frameName) {
            f->setFocus();
            return true;
        }
    }
    return false;
}

bool WebPage::switchToChildFrame(const QString &frameName) //< deprecated
{
    return this->switchToFrame(frameName);
}

bool WebPage::switchToFrame(const int framePosition)
{
    QList<QWebFrame *> childFrames = m_customWebPage->currentFrame()->childFrames();
    if (framePosition >= 0 && framePosition < childFrames.size()) {
        childFrames.at(framePosition)->setFocus();
        return true;
    }
    return false;
}

bool WebPage::switchToChildFrame(const int framePosition) //< deprecated
{
    return this->switchToFrame(framePosition);
}

void WebPage::switchToMainFrame()
{
    m_mainFrame->setFocus();
}

bool WebPage::switchToParentFrame()
{
    if (m_customWebPage->currentFrame()->parentFrame() != NULL) {
        m_customWebPage->currentFrame()->parentFrame()->setFocus();
        return true;
    }
    return false;
}

QString WebPage::frameName() const
{
    return m_customWebPage->currentFrame()->frameName();
}

QString WebPage::currentFrameName() const //< deprecated
{
    return this->frameName();
}

void WebPage::handleJavaScriptWindowObjectCleared()
{
    // Create Callbacks Holder object, if not already present for this page
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    // Reset focus on the Main Frame
    m_mainFrame->setFocus();

    // Decorate the window object in the Main Frame
    m_mainFrame->addToJavaScriptWindowObject(CALLBACKS_OBJECT_NAME, m_callbacks, QScriptEngine::QtOwnership);
    m_mainFrame->evaluateJavaScript(CALLBACKS_OBJECT_INJECTION);

    // Decorate the window object in the Main Frame's Child Frames
    foreach (QWebFrame *childFrame, m_mainFrame->childFrames()) {
        childFrame->addToJavaScriptWindowObject(CALLBACKS_OBJECT_NAME, m_callbacks, QScriptEngine::QtOwnership);
        childFrame->evaluateJavaScript(CALLBACKS_OBJECT_INJECTION);
    }
}

void WebPage::initCompletions()
{
    // Add completion for the Dynamic Properties of the 'webpage' object
    // properties
    addCompletion("clipRect");
    addCompletion("content");
    addCompletion("libraryPath");
    addCompletion("settings");
    addCompletion("viewportSize");
    addCompletion("ownsPages");
    addCompletion("windowName");
    addCompletion("pages");
    addCompletion("pagesWindowName");
    addCompletion("frameName");
    addCompletion("framesName");
    addCompletion("framesCount");
    // functions
    addCompletion("evaluate");
    addCompletion("includeJs");
    addCompletion("injectJs");
    addCompletion("open");
    addCompletion("release");
    addCompletion("render");
    addCompletion("renderBase64");
    addCompletion("sendEvent");
    addCompletion("uploadFile");
    addCompletion("getPage");
    addCompletion("switchToFrame");
    addCompletion("switchToMainFrame");
    addCompletion("switchToParentFrame");
    // callbacks
    addCompletion("onAlert");
    addCompletion("onCallback");
    addCompletion("onPrompt");
    addCompletion("onConfirm");
    addCompletion("onConsoleMessage");
    addCompletion("onInitialized");
    addCompletion("onLoadStarted");
    addCompletion("onLoadFinished");
    addCompletion("onResourceRequested");
    addCompletion("onResourceReceived");
    addCompletion("onUrlChanged");
    addCompletion("onNavigationRequested");
    addCompletion("onError");
    addCompletion("onPageCreated");
    addCompletion("onClosing");
}

#include "webpage.moc"
