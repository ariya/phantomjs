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

#include <QApplication>
#include <QBuffer>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QKeyEvent>
#include <QMapIterator>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QPainter>
#include <QScreen>
#include <QUrl>
#include <QUuid>
#include <QWebElement>
#include <QWebFrame>
#include <QWebHistory>
#include <QWebHistoryItem>
#include <QWebInspector>
#include <QWebPage>
#include <math.h>

#include "callback.h"
#include "config.h"
#include "consts.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "phantom.h"
#include "system.h"
#include "utils.h"

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

// Ensure we have at least head and body.
#define BLANK_HTML                      "<html><head></head><body></body></html>"
#define CALLBACKS_OBJECT_NAME           "_phantom"
#define INPAGE_CALL_NAME                "window.callPhantom"
#define CALLBACKS_OBJECT_INJECTION      INPAGE_CALL_NAME" = function() { return window."CALLBACKS_OBJECT_NAME".call.call(_phantom, Array.prototype.slice.call(arguments, 0)); };"
#define CALLBACKS_OBJECT_PRESENT        "typeof(window."CALLBACKS_OBJECT_NAME") !== \"undefined\";"

#define STDOUT_FILENAME "/dev/stdout"
#define STDERR_FILENAME "/dev/stderr"


/**
  * @class CustomPage
  */
class CustomPage: public QWebPage
{
    Q_OBJECT

public:
    CustomPage(WebPage* parent = 0)
        : QWebPage(parent)
        , m_webPage(parent)
    {
        m_userAgent = QWebPage::userAgentForUrl(QUrl());
        setForwardUnsupportedContent(true);
    }

    bool extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output)
    {
        Q_UNUSED(option);

        if (extension == ChooseMultipleFilesExtension) {
            static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = m_uploadFiles;
            return true;
        } else {
            return false;
        }
    }

    void setCookieJar(CookieJar* cookieJar)
    {
        m_cookieJar = cookieJar;
    }

public slots:
    bool shouldInterruptJavaScript()
    {
        m_webPage->javascriptInterrupt();

        if (m_webPage->m_shouldInterruptJs) {

            // reset our flag
            m_webPage->m_shouldInterruptJs = false;
            return true;
        }
        return false;
    }

protected:
    bool supportsExtension(Extension extension) const
    {
        return extension == ChooseMultipleFilesExtension;
    }

    QString chooseFile(QWebFrame* originatingFrame, const QString& oldFile)
    {
        Q_UNUSED(originatingFrame);

        // Check if User set a file via File Picker
        QString chosenFile = m_webPage->filePicker(oldFile);
        if (chosenFile == QString::null && m_uploadFiles.count() > 0) {
            // Check if instead User set a file via uploadFile API
            chosenFile = m_uploadFiles.first();
        }

        // Return the value coming from the "filePicker" callback, IFF not null.
        qDebug() << "CustomPage - file chosen for upload:" << chosenFile;
        return chosenFile;
    }

    void javaScriptAlert(QWebFrame* originatingFrame, const QString& msg)
    {
        Q_UNUSED(originatingFrame);
        emit m_webPage->javaScriptAlertSent(msg);
    }

    bool javaScriptConfirm(QWebFrame* originatingFrame, const QString& msg)
    {
        Q_UNUSED(originatingFrame);
        return m_webPage->javaScriptConfirm(msg);
    }

    bool javaScriptPrompt(QWebFrame* originatingFrame, const QString& msg, const QString& defaultValue, QString* result)
    {
        Q_UNUSED(originatingFrame);
        return m_webPage->javaScriptPrompt(msg, defaultValue, result);
    }

    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
    {
        Q_UNUSED(lineNumber);
        Q_UNUSED(sourceID);
        emit m_webPage->javaScriptConsoleMessageSent(message);
    }

    void javaScriptError(const QString& message, int lineNumber, const QString& sourceID, const QString& stack)
    {
        emit m_webPage->javaScriptErrorSent(message, lineNumber, sourceID, stack);
    }

    QString userAgentForUrl(const QUrl& url) const
    {
        Q_UNUSED(url);
        return m_userAgent;
    }

    bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, QWebPage::NavigationType type)
    {
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
        bool isNavigationLocked = m_webPage->navigationLocked();

        emit m_webPage->navigationRequested(
            request.url().toEncoded(),       //< Requested URL
            navigationType,                  //< Navigation Type
            !isNavigationLocked,             //< Will navigate (not locked)?
            isMainFrame);                    //< Is main frame?

        return !isNavigationLocked;
    }

    QWebPage* createWindow(WebWindowType type)
    {
        Q_UNUSED(type);
        WebPage* newPage;

        // Create a new "raw" WebPage object
        if (m_webPage->ownsPages()) {
            newPage = new WebPage(m_webPage);
        } else {
            newPage = new WebPage(Phantom::instance());
            Phantom::instance()->m_pages.append(newPage);
        }
        newPage->setCookieJar(m_cookieJar);

        // Apply default settings
        newPage->applySettings(Phantom::instance()->defaultPageSettings());

        // Signal JS shim to catch, decorate and store this new child page
        emit m_webPage->rawPageCreated(newPage);

        // Return the new QWebPage to the QWebKit backend
        return newPage->m_customWebPage;
    }

private:
    WebPage* m_webPage;
    QString m_userAgent;
    QStringList m_uploadFiles;
    friend class WebPage;
    CookieJar* m_cookieJar;
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
    WebpageCallbacks(QObject* parent = 0)
        : QObject(parent)
        , m_genericCallback(NULL)
        , m_filePickerCallback(NULL)
        , m_jsConfirmCallback(NULL)
        , m_jsPromptCallback(NULL)
        , m_jsInterruptCallback(NULL)
    {
    }

    QObject* getGenericCallback()
    {
        qDebug() << "WebpageCallbacks - getGenericCallback";

        if (!m_genericCallback) {
            m_genericCallback = new Callback(this);
        }
        return m_genericCallback;
    }

    QObject* getFilePickerCallback()
    {
        qDebug() << "WebpageCallbacks - getFilePickerCallback";

        if (!m_filePickerCallback) {
            m_filePickerCallback = new Callback(this);
        }
        return m_filePickerCallback;
    }

    QObject* getJsConfirmCallback()
    {
        qDebug() << "WebpageCallbacks - getJsConfirmCallback";

        if (!m_jsConfirmCallback) {
            m_jsConfirmCallback = new Callback(this);
        }
        return m_jsConfirmCallback;
    }

    QObject* getJsPromptCallback()
    {
        qDebug() << "WebpageCallbacks - getJsConfirmCallback";

        if (!m_jsPromptCallback) {
            m_jsPromptCallback = new Callback(this);
        }
        return m_jsPromptCallback;
    }

    QObject* getJsInterruptCallback()
    {
        qDebug() << "WebpageCallbacks - getJsInterruptCallback";

        if (!m_jsInterruptCallback) {
            m_jsInterruptCallback = new Callback(this);
        }
        return m_jsInterruptCallback;
    }

public slots:
    QVariant call(const QVariantList& arguments)
    {
        if (m_genericCallback) {
            return m_genericCallback->call(arguments);
        }
        return QVariant();
    }

private:
    Callback* m_genericCallback;
    Callback* m_filePickerCallback;
    Callback* m_jsConfirmCallback;
    Callback* m_jsPromptCallback;
    Callback* m_jsInterruptCallback;

    friend class WebPage;
};


WebPage::WebPage(QObject* parent, const QUrl& baseUrl)
    : QObject(parent)
    , m_navigationLocked(false)
    , m_mousePos(QPoint(0, 0))
    , m_ownsPages(true)
    , m_loadingProgress(0)
    , m_shouldInterruptJs(false)
{
    setObjectName("WebPage");
    m_callbacks = new WebpageCallbacks(this);
    m_customWebPage = new CustomPage(this);
    Config* phantomCfg = Phantom::instance()->config();

    // To grant universal access to a web page
    // attribute "WebSecurityEnabled" must be applied during the initializing
    // security context for Document instance. Setting up it later will not cause any effect
    // see <qt\src\3rdparty\webkit\Source\WebCore\dom\Document.cpp:4468>
    QWebSettings* settings = m_customWebPage->settings();
    settings->setAttribute(QWebSettings::WebSecurityEnabled, phantomCfg->webSecurityEnabled());

    m_mainFrame = m_customWebPage->mainFrame();
    m_currentFrame = m_mainFrame;
    m_mainFrame->setHtml(BLANK_HTML, baseUrl);

    // NOTE: below you can see that between all the event handlers
    // we listen for, "SLOT(setupFrame())" is connected to 2 signals:
    //   1. page.loadFinished
    //   2. mainFrame.javaScriptWindowObjectCleared
    // We have found out that, despite our understanding, the event #1 above
    // fires BEFORE the event #2 when loading a url.
    // But, if no page load is requested, #2 is the only one to fire.
    //
    // So, we call the slot twice to setup the main frame
    // (no parameter == main frame) but we make sure to do the setup only once.
    //
    // @see WebPage::setupFrame(QWebFrame *) for details.
    connect(m_mainFrame, SIGNAL(loadFinished(bool)), this, SLOT(setupFrame()), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(frameCreated(QWebFrame*)), this, SLOT(setupFrame(QWebFrame*)), Qt::DirectConnection);
    connect(m_mainFrame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(setupFrame()));
    connect(m_mainFrame, SIGNAL(javaScriptWindowObjectCleared()), SIGNAL(initialized()));
    connect(m_mainFrame, SIGNAL(urlChanged(QUrl)), this, SLOT(handleUrlChanged(QUrl)));
    connect(m_customWebPage, SIGNAL(loadStarted()), SIGNAL(loadStarted()), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(loadFinished(bool)), SLOT(finish(bool)), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(windowCloseRequested()), this, SLOT(close()), Qt::QueuedConnection);
    connect(m_customWebPage, SIGNAL(loadProgress(int)), this, SLOT(updateLoadingProgress(int)));
    connect(m_customWebPage, SIGNAL(repaintRequested(QRect)), this, SLOT(handleRepaintRequested(QRect)), Qt::QueuedConnection);


    // Start with transparent background.
    QPalette palette = m_customWebPage->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_customWebPage->setPalette(palette);

    // Set the page Library path
    setLibraryPath(QFileInfo(phantomCfg->scriptFile()).dir().absolutePath());

    // Page size does not need to take scrollbars into account.
    m_mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    QWebSettings* pageSettings = m_customWebPage->settings();
    pageSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    pageSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    pageSettings->setAttribute(QWebSettings::FrameFlatteningEnabled, true);

    bool isLocalStorageEnabled = phantomCfg->localStorageDefaultQuota() >= 0;
    pageSettings->setAttribute(QWebSettings::LocalStorageEnabled, isLocalStorageEnabled);

    if (isLocalStorageEnabled) {
        if (phantomCfg->localStoragePath().isEmpty()) {
            pageSettings->setLocalStoragePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        }
        else {
            pageSettings->setLocalStoragePath(phantomCfg->localStoragePath());
        }
    }

    // Custom network access manager to allow traffic monitoring.
    m_networkAccessManager = new NetworkAccessManager(this, phantomCfg);
    m_customWebPage->setNetworkAccessManager(m_networkAccessManager);
    connect(m_networkAccessManager, SIGNAL(resourceRequested(QVariant, QObject*)),
            SIGNAL(resourceRequested(QVariant, QObject*)));
    connect(m_networkAccessManager, SIGNAL(resourceReceived(QVariant)),
            SIGNAL(resourceReceived(QVariant)));
    connect(m_networkAccessManager, SIGNAL(resourceError(QVariant)),
            SIGNAL(resourceError(QVariant)));
    connect(m_networkAccessManager, SIGNAL(resourceTimeout(QVariant)),
            SIGNAL(resourceTimeout(QVariant)));

    m_dpi = qRound(QApplication::primaryScreen()->logicalDotsPerInch());
    m_customWebPage->setViewportSize(QSize(400, 300));
}

WebPage::~WebPage()
{
    emit closing(this);
}

QWebFrame* WebPage::mainFrame()
{
    return m_mainFrame;
}

QString WebPage::content() const
{
    return m_mainFrame->toHtml();
}

QString WebPage::frameContent() const
{
    return m_currentFrame->toHtml();
}

void WebPage::setContent(const QString& content)
{
    m_mainFrame->setHtml(content);
}

void WebPage::setContent(const QString& content, const QString& baseUrl)
{
    if (baseUrl == "about:blank") {
        m_mainFrame->setHtml(BLANK_HTML);
    } else {
        m_mainFrame->setHtml(content, QUrl(baseUrl));
    }
}

void WebPage::setFrameContent(const QString& content)
{
    m_currentFrame->setHtml(content);
}

void WebPage::setFrameContent(const QString& content, const QString& baseUrl)
{
    if (baseUrl == "about:blank") {
        m_currentFrame->setHtml(BLANK_HTML);
    } else {
        m_currentFrame->setHtml(content, QUrl(baseUrl));
    }
}

QString WebPage::title() const
{
    return m_mainFrame->title();
}

QString WebPage::frameTitle() const
{
    return m_currentFrame->title();
}

QString WebPage::url() const
{
    return m_mainFrame->url().toEncoded();
}

QString WebPage::frameUrl() const
{
    // Issue #11035: QWebFrame::url() is only set by QWebFrame::setUrl();
    // it doesn't reflect a URL specified in a <frame> tag.
    // QWebFrame::baseUrl() will be other than expected in the presence of
    // <base href>, but it's less wrong.
    return m_currentFrame->baseUrl().toEncoded();
}

bool WebPage::loading() const
{
    // Return "true" if Load Progress is "]0, 100["
    return (0 < m_loadingProgress && m_loadingProgress < 100);
}

int WebPage::loadingProgress() const
{
    return m_loadingProgress;
}

bool WebPage::canGoBack()
{
    return m_customWebPage->history()->canGoBack();
}

bool WebPage::goBack()
{
    if (canGoBack()) {
        m_customWebPage->history()->back();
        return true;
    }
    return false;
}

bool WebPage::canGoForward()
{
    return m_customWebPage->history()->canGoForward();
}

bool WebPage::goForward()
{
    if (canGoForward()) {
        m_customWebPage->history()->forward();
        return true;
    }
    return false;
}

bool WebPage::go(int historyItemRelativeIndex)
{
    // Convert the relative index to absolute
    int historyItemIndex = m_customWebPage->history()->currentItemIndex() + historyItemRelativeIndex;

    // Fetch the right item from the history
    QWebHistoryItem historyItem = m_customWebPage->history()->itemAt(historyItemIndex);

    // Go to the history item, if it's valid
    if (historyItem.isValid()) {
        m_customWebPage->history()->goToItem(historyItem);
        return true;
    }

    return false;
}

void WebPage::reload()
{
    m_customWebPage->triggerAction(QWebPage::Reload);
}

void WebPage::stop()
{
    m_customWebPage->triggerAction(QWebPage::Stop);
}


QString WebPage::plainText() const
{
    return m_mainFrame->toPlainText();
}

QString WebPage::framePlainText() const
{
    return m_currentFrame->toPlainText();
}

QString WebPage::libraryPath() const
{
    return m_libraryPath;
}

void WebPage::setLibraryPath(const QString& libraryPath)
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

int WebPage::showInspector(const int port)
{
    m_customWebPage->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    m_inspector = new QWebInspector;
    m_inspector->setPage(m_customWebPage);

    if (port == -1) {
        m_inspector->setVisible(true);
    } else {
        m_customWebPage->setProperty("_q_webInspectorServerPort", port);
        return m_customWebPage->property("_q_webInspectorServerPort").toInt();
    }
    return port;
}

void WebPage::applySettings(const QVariantMap& def)
{
    QWebSettings* opt = m_customWebPage->settings();

    opt->setAttribute(QWebSettings::AutoLoadImages, def[PAGE_SETTINGS_LOAD_IMAGES].toBool());
    opt->setAttribute(QWebSettings::JavascriptEnabled, def[PAGE_SETTINGS_JS_ENABLED].toBool());
    opt->setAttribute(QWebSettings::XSSAuditingEnabled, def[PAGE_SETTINGS_XSS_AUDITING].toBool());
    opt->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, def[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE].toBool());
    opt->setAttribute(QWebSettings::WebSecurityEnabled, def[PAGE_SETTINGS_WEB_SECURITY_ENABLED].toBool());
    opt->setAttribute(QWebSettings::JavascriptCanOpenWindows, def[PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS].toBool());
    opt->setAttribute(QWebSettings::JavascriptCanCloseWindows, def[PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS].toBool());

    if (def.contains(PAGE_SETTINGS_USER_AGENT)) {
        m_customWebPage->m_userAgent = def[PAGE_SETTINGS_USER_AGENT].toString();
    }

    if (def.contains(PAGE_SETTINGS_USERNAME)) {
        m_networkAccessManager->setUserName(def[PAGE_SETTINGS_USERNAME].toString());
    }

    if (def.contains(PAGE_SETTINGS_PASSWORD)) {
        m_networkAccessManager->setPassword(def[PAGE_SETTINGS_PASSWORD].toString());
    }

    if (def.contains(PAGE_SETTINGS_MAX_AUTH_ATTEMPTS)) {
        m_networkAccessManager->setMaxAuthAttempts(def[PAGE_SETTINGS_MAX_AUTH_ATTEMPTS].toInt());
    }

    if (def.contains(PAGE_SETTINGS_RESOURCE_TIMEOUT)) {
        m_networkAccessManager->setResourceTimeout(def[PAGE_SETTINGS_RESOURCE_TIMEOUT].toInt());
    }

    if (def.contains(PAGE_SETTINGS_PROXY)) {
        setProxy(def[PAGE_SETTINGS_PROXY].toString());
    }

    if (def.contains(PAGE_SETTINGS_DPI)) {
        m_dpi = def[PAGE_SETTINGS_DPI].toReal();
    }
}

void WebPage::setProxy(const QString& proxyUrl)
{
    QUrl url(proxyUrl);
    qDebug() << "Setting proxy to: " << url.scheme() << url.host() << url.port();
    QNetworkProxy::ProxyType type = QNetworkProxy::HttpProxy;
    if (url.scheme() == "socks5") {
        type = QNetworkProxy::Socks5Proxy;
    }
    QNetworkProxy proxy(type, url.host(), url.port(), url.userName(), url.password());
    m_networkAccessManager->setProxy(proxy);
}

QString WebPage::userAgent() const
{
    return m_customWebPage->m_userAgent;
}

void WebPage::setNavigationLocked(bool lock)
{
    m_navigationLocked = lock;
}

bool WebPage::navigationLocked()
{
    return m_navigationLocked;
}

void WebPage::setViewportSize(const QVariantMap& size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    if (w > 0 && h > 0) {
        m_customWebPage->setViewportSize(QSize(w, h));
    }
}

QVariantMap WebPage::viewportSize() const
{
    QVariantMap result;
    QSize size = m_customWebPage->viewportSize();
    result["width"] = size.width();
    result["height"] = size.height();
    return result;
}

void WebPage::setClipRect(const QVariantMap& size)
{
    int w = size.value("width").toInt();
    int h = size.value("height").toInt();
    int top = size.value("top").toInt();
    int left = size.value("left").toInt();

    if (w >= 0 && h >= 0) {
        m_clipRect = QRect(left, top, w, h);
    }
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


void WebPage::setScrollPosition(const QVariantMap& size)
{
    int top = size.value("top").toInt();
    int left = size.value("left").toInt();
    m_scrollPosition = QPoint(left, top);
    m_mainFrame->setScrollPosition(m_scrollPosition);
}

QVariantMap WebPage::scrollPosition() const
{
    QVariantMap result;
    result["top"] = m_scrollPosition.y();
    result["left"] = m_scrollPosition.x();
    return result;
}

void WebPage::setPaperSize(const QVariantMap& size)
{
    m_paperSize = size;
}

QVariantMap WebPage::paperSize() const
{
    return m_paperSize;
}

QVariant WebPage::evaluateJavaScript(const QString& code)
{
    QVariant evalResult;
    QString function = "(" + code + ")()";

    qDebug() << "WebPage - evaluateJavaScript" << function;

    evalResult = m_currentFrame->evaluateJavaScript(
                     function,                                   //< function evaluated
                     QString("phantomjs://webpage.evaluate()")); //< reference source file

    qDebug() << "WebPage - evaluateJavaScript result" << evalResult;

    return evalResult;
}

QString WebPage::filePicker(const QString& oldFile)
{
    qDebug() << "WebPage - filePicker" << "- old file:" << oldFile;

    if (m_callbacks->m_filePickerCallback) {
        QVariant res = m_callbacks->m_filePickerCallback->call(QVariantList() << oldFile);

        if (res.canConvert<QString>()) {
            QString filePath = res.toString();
            qDebug() << "WebPage - filePicker" << "- new file:" << filePath;
            // Return this value only if the file actually exists
            if (QFile::exists(filePath)) {
                return filePath;
            }
        }
    }
    return QString::null;
}

bool WebPage::javaScriptConfirm(const QString& msg)
{
    if (m_callbacks->m_jsConfirmCallback) {
        QVariant res = m_callbacks->m_jsConfirmCallback->call(QVariantList() << msg);
        if (res.canConvert<bool>()) {
            return res.toBool();
        }
    }
    return false;
}

bool WebPage::javaScriptPrompt(const QString& msg, const QString& defaultValue, QString* result)
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

void WebPage::javascriptInterrupt()
{
    if (m_callbacks->m_jsInterruptCallback) {
        QVariant res = m_callbacks->m_jsInterruptCallback->call(QVariantList());

        if (res.canConvert<bool>()) {
            m_shouldInterruptJs = res.toBool();
        }
    }
}

void WebPage::finish(bool ok)
{
    QString status = ok ? "success" : "fail";
    emit loadFinished(status);
}

void WebPage::setCustomHeaders(const QVariantMap& headers)
{
    m_networkAccessManager->setCustomHeaders(headers);
}

QVariantMap WebPage::customHeaders() const
{
    return m_networkAccessManager->customHeaders();
}

void WebPage::setCookieJar(CookieJar* cookieJar)
{
    m_cookieJar = cookieJar;
    m_customWebPage->setCookieJar(m_cookieJar);
    m_networkAccessManager->setCookieJar(m_cookieJar);
}

void WebPage::setCookieJarFromQObject(QObject* cookieJar)
{
    setCookieJar(qobject_cast<CookieJar*>(cookieJar));
}

CookieJar* WebPage::cookieJar()
{
    return m_cookieJar;
}

bool WebPage::setCookies(const QVariantList& cookies)
{
    // Delete all the cookies for this URL
    m_cookieJar->deleteCookies(this->url());
    // Add a new set of cookies foor this URL
    return m_cookieJar->addCookiesFromMap(cookies, this->url());
}

QVariantList WebPage::cookies() const
{
    // Return all the Cookies visible to this Page, as a list of Maps (aka JSON in JS space)
    return m_cookieJar->cookiesToMap(this->url());
}

bool WebPage::addCookie(const QVariantMap& cookie)
{
    return m_cookieJar->addCookieFromMap(cookie, this->url());
}

bool WebPage::deleteCookie(const QString& cookieName)
{
    if (!cookieName.isEmpty()) {
        return m_cookieJar->deleteCookie(cookieName, this->url());
    }
    return false;
}

bool WebPage::clearCookies()
{
    return m_cookieJar->deleteCookies(this->url());
}

void WebPage::openUrl(const QString& address, const QVariant& op, const QVariantMap& settings)
{
    QString operation;
    QByteArray body;
    QNetworkRequest request;

    applySettings(settings);
    m_customWebPage->triggerAction(QWebPage::Stop);

    if (op.type() == QVariant::String) {
        operation = op.toString();
    }

    if (op.type() == QVariant::Map) {
        QVariantMap settingsMap = op.toMap();
        operation = settingsMap.value("operation").toString();
        QString bodyString = settingsMap.value("data").toString();
        QString encoding = settingsMap.value("encoding").toString().toLower();
        body = encoding == "utf-8" || encoding == "utf8" ? bodyString.toUtf8() : bodyString.toLatin1();
        if (settingsMap.contains("headers")) {
            QMapIterator<QString, QVariant> i(settingsMap.value("headers").toMap());
            while (i.hasNext()) {
                i.next();
                request.setRawHeader(i.key().toUtf8(), i.value().toString().toUtf8());
            }
        }
    }

    if (operation.isEmpty()) {
        operation = "get";
    }

    QNetworkAccessManager::Operation networkOp = QNetworkAccessManager::UnknownOperation;
    operation = operation.toLower();
    if (operation == "get") {
        networkOp = QNetworkAccessManager::GetOperation;
    } else if (operation == "head") {
        networkOp = QNetworkAccessManager::HeadOperation;
    } else if (operation == "put") {
        networkOp = QNetworkAccessManager::PutOperation;
    } else if (operation == "post") {
        networkOp = QNetworkAccessManager::PostOperation;
    } else if (operation == "delete") {
        networkOp = QNetworkAccessManager::DeleteOperation;
    }

    if (networkOp == QNetworkAccessManager::UnknownOperation) {
        m_mainFrame->evaluateJavaScript("console.error('Unknown network operation: " + operation + "');", QString());
        return;
    }

    if (address == "about:blank") {
        m_mainFrame->setHtml(BLANK_HTML);
    } else {
        QUrl url = QUrl::fromEncoded(QByteArray(address.toLatin1()));

        // Assume local file if scheme is empty
        if (url.scheme().isEmpty()) {
            url.setPath(QFileInfo(url.toString()).absoluteFilePath().prepend("/"));
            url.setScheme("file");
        }

        request.setUrl(url);
        m_mainFrame->load(request, networkOp, body);
    }
}

void WebPage::release()
{
    close();
}

void WebPage::close()
{
    deleteLater();
}

bool WebPage::render(const QString& fileName, const QVariantMap& option)
{
    if (m_mainFrame->contentsSize().isEmpty()) {
        return false;
    }

    QString outFileName = fileName;
    QString tempFileName = "";

    QString format = "";
    int quality = -1; // QImage#save default

    if (fileName == STDOUT_FILENAME || fileName == STDERR_FILENAME) {
        if (!QFile::exists(fileName)) {
            // create temporary file for OS that have no /dev/stdout or /dev/stderr. (ex. windows)
            tempFileName = QDir::tempPath() + "/phantomjstemp" + QUuid::createUuid().toString();
            outFileName = tempFileName;
        }

        format = "png"; // default format for stdout and stderr
    } else {
        QFileInfo fileInfo(outFileName);
        QDir dir;
        dir.mkpath(fileInfo.absolutePath());
    }

    if (option.contains("format")) {
        format = option.value("format").toString();
    } else if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        format = "pdf";
    }

    if (option.contains("quality")) {
        quality = option.value("quality").toInt();
    }

    bool retval = true;
    if (format == "pdf") {
        QPdfWriter pdfWriter(fileName);
        retval = renderPdf(pdfWriter);
    } else {
        RenderMode mode;
        if (option.contains("onlyViewport") && option.value("onlyViewport").toBool()) {
            mode = Viewport;
        } else {
            mode = Content;
        }
        QImage rawPageRendering = renderImage(mode);

        const char* f = 0; // 0 is QImage#save default
        if (format != "") {
            f = format.toLocal8Bit().constData();
        }

        retval = rawPageRendering.save(outFileName, f, quality);
    }

    if (tempFileName != "") {
        // cleanup temporary file and render to stdout or stderr
        QFile i(tempFileName);
        i.open(QIODevice::ReadOnly);

        QByteArray ba = i.readAll();

        System* system = (System*)Phantom::instance()->createSystem();
        if (fileName == STDOUT_FILENAME) {
#ifdef Q_OS_WIN
            _setmode(_fileno(stdout), O_BINARY);
#endif

            ((File*)system->_stdout())->write(QString::fromLatin1(ba.constData(), ba.size()));

#ifdef Q_OS_WIN
            _setmode(_fileno(stdout), O_TEXT);
#endif
        } else if (fileName == STDERR_FILENAME) {
#ifdef Q_OS_WIN
            _setmode(_fileno(stderr), O_BINARY);
#endif

            ((File*)system->_stderr())->write(QString::fromLatin1(ba.constData(), ba.size()));

#ifdef Q_OS_WIN
            _setmode(_fileno(stderr), O_TEXT);
#endif
        }

        i.close();

        QFile::remove(tempFileName);
    }

    return retval;
}

QString WebPage::renderBase64(const QByteArray& format)
{
    QByteArray nformat = format.toLower();

    if (format != "pdf" && !QImageWriter::supportedImageFormats().contains(nformat)) {
        // Return an empty string in case an unsupported format was provided
        return "";
    }

    // Prepare buffer for writing
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    if (format == "pdf") {
        QPdfWriter pdfWriter(&buffer);

        if (!renderPdf(pdfWriter)) {
            // Return an empty string if pdf render fails
            return "";
        }
    } else {
        QImage rawPageRendering = renderImage();

        // Writing image to the buffer, using PNG encoding
        rawPageRendering.save(&buffer, nformat);
    }

    return bytes.toBase64();
}

QImage WebPage::renderImage(const RenderMode mode)
{
    QRect frameRect;
    QSize viewportSize = m_customWebPage->viewportSize();
    if (mode == Viewport) {
        frameRect = QRect(QPoint(0, 0), viewportSize);
    } else {
        QSize contentsSize = m_mainFrame->contentsSize();
        contentsSize -= QSize(m_scrollPosition.x(), m_scrollPosition.y());
        frameRect = QRect(QPoint(0, 0), contentsSize);
        m_customWebPage->setViewportSize(contentsSize);
    }

    if (!m_clipRect.isNull()) {
        frameRect = m_clipRect;
    }

#ifdef Q_OS_WIN
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
#else
    QImage::Format format = QImage::Format_ARGB32;
#endif

    QImage buffer(frameRect.size(), format);
    buffer.fill(Qt::transparent);

    QPainter painter;

    // We use tiling approach to work-around Qt software rasterizer bug
    // when dealing with very large paint device.
    // See http://code.google.com/p/phantomjs/issues/detail?id=54.
    const int tileSize = 4096;
    int htiles = (buffer.width() + tileSize - 1) / tileSize;
    int vtiles = (buffer.height() + tileSize - 1) / tileSize;
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
            m_mainFrame->render(&painter, QRegion(frameRect));
            painter.end();

            // Copy the tile to the main buffer
            painter.begin(&buffer);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.drawImage(x * tileSize, y * tileSize, tileBuffer);
            painter.end();
        }
    }
    if (mode != Viewport) {
        m_customWebPage->setViewportSize(viewportSize);
    }
    return buffer;
}

qreal WebPage::stringToPointSize(const QString& string) const
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
    for (uint i = 0; i < sizeof(units) / sizeof(units[0]); ++i) {
        if (string.endsWith(units[i].unit)) {
            QString value = string;
            value.chop(units[i].unit.length());
            return value.toDouble() * units[i].factor;
        }
    }
    return 0;
}

qreal WebPage::printMargin(const QVariantMap& map, const QString& key)
{
    const QVariant margin = map.value(key);
    if (margin.isValid() && margin.canConvert(QVariant::String)) {
        return stringToPointSize(margin.toString());
    } else {
        return 0;
    }
}

bool WebPage::renderPdf(QPdfWriter& pdfWriter)
{
    pdfWriter.setResolution(m_dpi);
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
        pdfWriter.setPageSize(QPageSize(sizePt, QPageSize::Point));
    } else if (paperSize.contains("format")) {
        const QPageLayout::Orientation orientation = paperSize.contains("orientation")
                && paperSize.value("orientation").toString().compare("landscape", Qt::CaseInsensitive) == 0 ?
                QPageLayout::Portrait : QPageLayout::Landscape;
        pdfWriter.setPageOrientation(orientation);
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
        pdfWriter.setPageSize(QPageSize(QPageSize::A4)); // Fallback
        for (uint i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
            if (paperSize.value("format").toString().compare(formats[i].format, Qt::CaseInsensitive) == 0) {
                pdfWriter.setPageSize(QPageSize(formats[i].paperSize));
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

    pdfWriter.setPageMargins(QMarginsF(marginLeft, marginTop, marginRight, marginBottom), QPageLayout::Point);

    QPainter painter(&pdfWriter);
    m_mainFrame->render(&painter);
    painter.end();

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

qreal WebPage::getHeight(const QVariantMap& map, const QString& key) const
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

QString getHeaderFooter(const QVariantMap& map, const QString& key, QWebFrame* frame, int page, int numPages)
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

void WebPage::_uploadFile(const QString& selector, const QStringList& fileNames)
{
    QWebElement el = m_currentFrame->findFirstElement(selector);
    if (el.isNull()) {
        return;
    }

    // Filter out "fileNames" that don't actually exist
    m_customWebPage->m_uploadFiles.clear();
    for (int i = 0, ilen = fileNames.length(); i < ilen; ++i) {
        if (QFile::exists(fileNames[i])) {
            m_customWebPage->m_uploadFiles.append(fileNames[i]);
        }
    }

    el.evaluateJavaScript(JS_ELEMENT_CLICK);
}

bool WebPage::injectJs(const QString& jsFilePath)
{
    return Utils::injectJsInFrame(jsFilePath, m_libraryPath, m_currentFrame);
}

void WebPage::_appendScriptElement(const QString& scriptUrl)
{
    m_currentFrame->evaluateJavaScript(QString(JS_APPEND_SCRIPT_ELEMENT).arg(scriptUrl), scriptUrl);
}

QObject* WebPage::_getGenericCallback()
{
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getGenericCallback();
}

QObject* WebPage::_getFilePickerCallback()
{
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getFilePickerCallback();
}

QObject* WebPage::_getJsConfirmCallback()
{
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getJsConfirmCallback();
}

QObject* WebPage::_getJsPromptCallback()
{
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getJsPromptCallback();
}

QObject* WebPage::_getJsInterruptCallback()
{
    if (!m_callbacks) {
        m_callbacks = new WebpageCallbacks(this);
    }

    return m_callbacks->getJsInterruptCallback();
}

void WebPage::sendEvent(const QString& type, const QVariant& arg1, const QVariant& arg2, const QString& mouseButton, const QVariant& modifierArg)
{
    Qt::KeyboardModifiers keyboardModifiers(modifierArg.toInt());
    // Normalize the event "type" to lowercase
    const QString eventType = type.toLower();

    // single keyboard events
    if (eventType == "keydown" || eventType == "keyup") {
        QKeyEvent::Type keyEventType = QEvent::None;
        if (eventType == "keydown") {
            keyEventType = QKeyEvent::KeyPress;
        }
        if (eventType == "keyup") {
            keyEventType = QKeyEvent::KeyRelease;
        }
        Q_ASSERT(keyEventType != QEvent::None);

        int key = 0;
        QString text;
        if (arg1.type() == QVariant::Char) {
            // a single char was given
            text = arg1.toChar();
            key = text.at(0).toUpper().unicode();
        } else if (arg1.type() == QVariant::String) {
            // javascript invokation of a single char
            text = arg1.toString();
            if (!text.isEmpty()) {
                key = text.at(0).toUpper().unicode();
            }
        } else {
            // assume a raw integer char code was given
            key = arg1.toInt();
        }
        QKeyEvent* keyEvent = new QKeyEvent(keyEventType, key, keyboardModifiers, text);
        QApplication::postEvent(m_customWebPage, keyEvent);
        QApplication::processEvents();
        return;
    }

    // sequence of key events: will generate all the single keydown/keyup events
    if (eventType == "keypress") {
        if (arg1.type() == QVariant::String) {
            // this is the case for e.g. sendEvent("...", 'A')
            // but also works with sendEvent("...", "ABCD")
            foreach(const QChar typeChar, arg1.toString()) {
                sendEvent("keydown", typeChar, QVariant(), QString(), modifierArg);
                sendEvent("keyup", typeChar, QVariant(), QString(), modifierArg);
            }
        } else {
            // otherwise we assume a raw integer char-code was given
            sendEvent("keydown", arg1.toInt(), QVariant(), QString(), modifierArg);
            sendEvent("keyup", arg1.toInt(), QVariant(), QString(), modifierArg);
        }
        return;
    }

    // mouse events
    if (eventType == "mousedown" ||
            eventType == "mouseup" ||
            eventType == "mousemove" ||
            eventType == "mousedoubleclick") {
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
        } else if (eventType == "mousedoubleclick") {
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

        // Prepare the Mouse event
        qDebug() << "Mouse Event:" << eventType << "(" << mouseEventType << ")" << m_mousePos << ")" << button << buttons;
        QMouseEvent* event = new QMouseEvent(mouseEventType, m_mousePos, button, buttons, keyboardModifiers);

        // Post and process events
        QApplication::postEvent(m_customWebPage, event);
        QApplication::processEvents();
        return;
    }

    // context click
    if (type == "contextmenu") {
        QContextMenuEvent::Reason reason = QContextMenuEvent::Mouse;

        // Gather coordinates
        if (arg1.isValid() && arg2.isValid()) {
            m_mousePos.setX(arg1.toInt());
            m_mousePos.setY(arg2.toInt());
        }

        // Prepare the context menu event
        qDebug() << "Context Menu Event:" << eventType << "(" << reason << "," << m_mousePos << ")";
        QContextMenuEvent* event = new QContextMenuEvent(reason, m_mousePos, QCursor::pos(), keyboardModifiers);

        // Post and process events
        // Send the context menu event directly to QWebPage::swallowContextMenuEvent which forwards it to JS
        // If we fire the event using postEvent to m_customWebPage, it will end up in QWebPagePrivate::contextMenuEvent,
        // which will not forward it to JS at all
        m_customWebPage->swallowContextMenuEvent(event);
        return;
    }


    // mouse click events: Qt doesn't provide this as a separate events,
    // so we compose it with a mousedown/mouseup sequence
    // mouse doubleclick events: It is not enough to simply send a
    // MouseButtonDblClick event by itself; it must be accompanied
    // by a preceding press-release, and a following release.
    if (type == "click" || type == "doubleclick") {
        sendEvent("mousedown", arg1, arg2, mouseButton, modifierArg);
        sendEvent("mouseup", arg1, arg2, mouseButton, modifierArg);
        if (type == "doubleclick") {
            sendEvent("mousedoubleclick", arg1, arg2, mouseButton, modifierArg);
            sendEvent("mouseup", arg1, arg2, mouseButton, modifierArg);
        }
        return;
    }
}

QObjectList WebPage::pages() const
{
    QObjectList pages;

    QList<WebPage*> childPages = this->findChildren<WebPage*>();
    for (int i = childPages.length() - 1; i >= 0; --i) {
        pages << childPages.at(i);
    }

    return pages;
}

QStringList WebPage::pagesWindowName() const
{
    QStringList pagesWindowName;

    foreach(const WebPage * p, this->findChildren<WebPage*>()) {
        pagesWindowName << p->windowName();
    }

    return pagesWindowName;
}

QObject* WebPage::getPage(const QString& windowName) const
{
    QList<WebPage*> childPages = this->findChildren<WebPage*>();
    for (int i = childPages.length() - 1; i >= 0; --i) {
        if (childPages.at(i)->windowName() == windowName) {
            return childPages.at(i);
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
    return m_currentFrame->childFrames().count();
}

int WebPage::childFramesCount() const //< deprecated
{
    return this->framesCount();
}

QStringList WebPage::framesName() const
{
    QStringList framesName;

    foreach(const QWebFrame * f, m_currentFrame->childFrames()) {
        framesName << f->frameName();
    }
    return framesName;
}

QStringList WebPage::childFramesName() const //< deprecated
{
    return this->framesName();
}

// The main frame is never destroyed unexpectedly, but child frames
// can go away for all sorts of reasons.  When this happens to the
// frame that is m_currentFrame, we need to reset m_currentFrame to
// m_mainFrame, *immediately* (hence Qt::DirectConnection), and
// without touching any members of m_currentFrame.  It is sufficient
// to register m_currentFrame for this treatment, because if any of
// its parent frames are destroyed it will be destroyed too.

void WebPage::changeCurrentFrame(QWebFrame* const frame)
{
    if (frame != m_currentFrame) {
        qDebug() << "WebPage - changeCurrentFrame"
                 << "from" << m_currentFrame->frameName()
                 << "to" << frame->frameName();

        if (m_currentFrame != m_mainFrame) {
            disconnect(m_currentFrame, &QWebFrame::destroyed,
                       this, &WebPage::handleCurrentFrameDestroyed);
        }

        m_currentFrame = frame;

        if (m_currentFrame != m_mainFrame) {
            connect(m_currentFrame, &QWebFrame::destroyed,
                    this, &WebPage::handleCurrentFrameDestroyed,
                    Qt::DirectConnection);
        }
    }
}

// This is called from the QObject::destroyed signal and does not need
// to (indeed, must not) do the monkeying with signals that
// changeCurrentFrame does.
void WebPage::handleCurrentFrameDestroyed()
{
    m_currentFrame = m_mainFrame;
}

bool WebPage::switchToFrame(const QString& frameName)
{
    QList<QWebFrame*> childFrames = m_currentFrame->childFrames();
    for (int i = childFrames.length() - 1; i >= 0; --i) {
        if (childFrames.at(i)->frameName() == frameName) {
            this->changeCurrentFrame(childFrames.at(i));
            return true;
        }
    }
    return false;
}

bool WebPage::switchToChildFrame(const QString& frameName) //< deprecated
{
    return this->switchToFrame(frameName);
}

bool WebPage::switchToFrame(const int framePosition)
{
    QList<QWebFrame*> childFrames = m_currentFrame->childFrames();
    if (framePosition >= 0 && framePosition < childFrames.size()) {
        this->changeCurrentFrame(childFrames.at(framePosition));
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
    if (m_currentFrame != m_mainFrame) {
        this->changeCurrentFrame(m_mainFrame);
    }
}

bool WebPage::switchToParentFrame()
{
    if (m_currentFrame->parentFrame() != NULL) {
        this->changeCurrentFrame(m_currentFrame->parentFrame());
        return true;
    }
    return false;
}

void WebPage::switchToFocusedFrame()
{
    this->changeCurrentFrame(m_customWebPage->currentFrame());
}

QString WebPage::frameName() const
{
    return m_currentFrame->frameName();
}

QString WebPage::currentFrameName() const //< deprecated
{
    return this->frameName();
}

QString WebPage::focusedFrameName() const
{
    return m_customWebPage->currentFrame()->frameName();
}

static void injectCallbacksObjIntoFrame(QWebFrame* frame, WebpageCallbacks* callbacksObject)
{
    // Inject object only if it's not already present
    if (frame->evaluateJavaScript(CALLBACKS_OBJECT_PRESENT).toBool() == false) {
        // Decorate the window object in this frame (object ownership left to the creator/parent)
        frame->addToJavaScriptWindowObject(CALLBACKS_OBJECT_NAME, callbacksObject, QWebFrame::QtOwnership);
        frame->evaluateJavaScript(CALLBACKS_OBJECT_INJECTION);
    }
}

void WebPage::setupFrame(QWebFrame* frame)
{
    qDebug() << "WebPage - setupFrame" << (frame == NULL ? "" : frame->frameName());

    // Inject the Callbacks object in the main frame
    injectCallbacksObjIntoFrame(frame == NULL ? m_mainFrame : frame, m_callbacks);
}

void WebPage::updateLoadingProgress(int progress)
{
    qDebug() << "WebPage - updateLoadingProgress:" << progress;
    m_loadingProgress = progress;
}

void WebPage::handleRepaintRequested(const QRect& dirtyRect)
{
    emit repaintRequested(dirtyRect.x(), dirtyRect.y(), dirtyRect.width(), dirtyRect.height());
}

void WebPage::handleUrlChanged(const QUrl& url)
{
    emit urlChanged(url.toEncoded());
}

void WebPage::stopJavaScript()
{
    m_shouldInterruptJs = true;
}

void WebPage::clearMemoryCache()
{
    QWebSettings::clearMemoryCaches();
}

#include "webpage.moc"
