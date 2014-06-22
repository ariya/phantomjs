/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpRenderTree.h"

#include "DumpRenderTreeQt.h"
#include "DumpRenderTreeSupportQt.h"
#include "EventSenderQt.h"
#include "GCController.h"
#include "InitWebCoreQt.h"
#include "InitWebKitQt.h"
#include "JSStringRefQt.h"
#include "QtTestSupport.h"
#include "TestRunner.h"
#include "TestRunnerQt.h"
#include "TextInputControllerQt.h"
#include "testplugin.h"
#include "WorkQueue.h"

#include <QApplication>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFocusEvent>
#include <QLabel>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPaintDevice>
#include <QPaintEngine>
#if !defined(QT_NO_PRINTER) && HAVE(QTPRINTSUPPORT)
#include <QPrinter>
#endif
#include <QProgressBar>
#include <QUndoStack>
#include <QUrl>
#include <limits.h>
#include <locale.h>
#include <qwebsecurityorigin.h>
#include <qwebsettings.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

using namespace WebCore;

const int databaseDefaultQuota = 5 * 1024 * 1024;

NetworkAccessManager::NetworkAccessManager(QObject* parent)
    : QNetworkAccessManager(parent)
{
#ifndef QT_NO_OPENSSL
    connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            this, SLOT(sslErrorsEncountered(QNetworkReply*, const QList<QSslError>&)));
#endif
}

#ifndef QT_NO_OPENSSL
void NetworkAccessManager::sslErrorsEncountered(QNetworkReply* reply, const QList<QSslError>& errors)
{
    if (reply->url().host() == "127.0.0.1" || reply->url().host() == "localhost") {
        bool ignore = true;

        // Accept any HTTPS certificate.
        foreach (const QSslError& error, errors) {
            if (error.error() < QSslError::UnableToGetIssuerCertificate || error.error() > QSslError::HostNameMismatch) {
                ignore = false;
                break;
            }
        }

        if (ignore)
            reply->ignoreSslErrors();
    }
}
#endif


#if !defined(QT_NO_PRINTER) && HAVE(QTPRINTSUPPORT)
class NullPrinter : public QPrinter {
public:
    class NullPaintEngine : public QPaintEngine {
    public:
        virtual bool begin(QPaintDevice*) { return true; }
        virtual bool end() { return true; }
        virtual QPaintEngine::Type type() const { return QPaintEngine::User; }
        virtual void drawPixmap(const QRectF& r, const QPixmap& pm, const QRectF& sr) { }
        virtual void updateState(const QPaintEngineState& state) { }
    };

    virtual QPaintEngine* paintEngine() const { return const_cast<NullPaintEngine*>(&m_engine); }

    NullPaintEngine m_engine;
};
#endif

WebPage::WebPage(QObject* parent, DumpRenderTree* drt)
    : QWebPage(parent)
    , m_webInspector(0)
    , m_drt(drt)
{
    QWebSettings* globalSettings = QWebSettings::globalSettings();

    globalSettings->setFontSize(QWebSettings::MinimumFontSize, 0);
    globalSettings->setFontSize(QWebSettings::MinimumLogicalFontSize, 5);
    globalSettings->setFontSize(QWebSettings::DefaultFontSize, 16);
    globalSettings->setFontSize(QWebSettings::DefaultFixedFontSize, 13);

    globalSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    globalSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    globalSettings->setAttribute(QWebSettings::LinksIncludedInFocusChain, false);
    globalSettings->setAttribute(QWebSettings::PluginsEnabled, true);
    globalSettings->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
    globalSettings->setAttribute(QWebSettings::JavascriptEnabled, true);
    globalSettings->setAttribute(QWebSettings::PrivateBrowsingEnabled, false);
    globalSettings->setAttribute(QWebSettings::SpatialNavigationEnabled, false);

    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(setViewGeometry(const QRect & )));

    setNetworkAccessManager(m_drt->networkAccessManager());
    setPluginFactory(new TestPlugin(this));

    connect(this, SIGNAL(featurePermissionRequested(QWebFrame*, QWebPage::Feature)), this, SLOT(requestPermission(QWebFrame*, QWebPage::Feature)));
    connect(this, SIGNAL(featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)), this, SLOT(cancelPermission(QWebFrame*, QWebPage::Feature)));
}

WebPage::~WebPage()
{
    delete m_webInspector;
}

QWebInspector* WebPage::webInspector()
{
    if (!m_webInspector) {
        m_webInspector = new QWebInspector;
        m_webInspector->setPage(this);
    }
    return m_webInspector;
}

void WebPage::resetSettings()
{
    // After each layout test, reset the settings that may have been changed by
    // testRunner.overridePreference() or similar.
    settings()->resetFontSize(QWebSettings::DefaultFontSize);
    settings()->resetAttribute(QWebSettings::JavascriptCanOpenWindows);
    settings()->resetAttribute(QWebSettings::JavascriptEnabled);
    settings()->resetAttribute(QWebSettings::PrivateBrowsingEnabled);
    settings()->resetAttribute(QWebSettings::SpatialNavigationEnabled);
    settings()->resetAttribute(QWebSettings::LinksIncludedInFocusChain);
    settings()->resetAttribute(QWebSettings::OfflineWebApplicationCacheEnabled);
    settings()->resetAttribute(QWebSettings::LocalContentCanAccessRemoteUrls);
    settings()->resetAttribute(QWebSettings::LocalContentCanAccessFileUrls);
    settings()->resetAttribute(QWebSettings::PluginsEnabled);
    settings()->resetAttribute(QWebSettings::JavascriptCanAccessClipboard);
    settings()->resetAttribute(QWebSettings::AutoLoadImages);
    settings()->resetAttribute(QWebSettings::ZoomTextOnly);
    settings()->resetAttribute(QWebSettings::CSSRegionsEnabled);
    settings()->resetAttribute(QWebSettings::CSSGridLayoutEnabled);
    settings()->resetAttribute(QWebSettings::AcceleratedCompositingEnabled);

    m_drt->testRunner()->setCaretBrowsingEnabled(false);
    m_drt->testRunner()->setAuthorAndUserStylesEnabled(true);
    m_drt->jscTestRunner()->setDefersLoading(false);

    // globalSettings must be reset explicitly.
    m_drt->testRunner()->setXSSAuditorEnabled(false);

    QWebSettings::setMaximumPagesInCache(0); // reset to default
    settings()->setUserStyleSheetUrl(QUrl()); // reset to default

    DumpRenderTreeSupportQt::setSeamlessIFramesEnabled(true);

    DumpRenderTreeSupportQt::resetInternalsObject(mainFrame()->handle());

    m_pendingGeolocationRequests.clear();
}

QWebPage* WebPage::createWindow(QWebPage::WebWindowType)
{
    return m_drt->createWindow();
}

void WebPage::javaScriptAlert(QWebFrame*, const QString& message)
{
    if (!isTextOutputEnabled())
        return;

    fprintf(stdout, "ALERT: %s\n", message.toUtf8().constData());
    fflush(stdout);
}

void WebPage::requestPermission(QWebFrame* frame, QWebPage::Feature feature)
{
    switch (feature) {
    case Notifications:
        if (!m_drt->testRunner()->ignoreReqestForPermission())
            setFeaturePermission(frame, feature, PermissionGrantedByUser);
        break;
    case Geolocation:
        if (m_drt->testRunner()->isGeolocationPermissionSet())
            if (m_drt->testRunner()->geolocationPermission())
                setFeaturePermission(frame, feature, PermissionGrantedByUser);
            else
                setFeaturePermission(frame, feature, PermissionDeniedByUser);
        else
            m_pendingGeolocationRequests.append(frame);
        break;
    default:
        break;
    }
}

void WebPage::cancelPermission(QWebFrame* frame, QWebPage::Feature feature)
{
    switch (feature) {
    case Geolocation:
        m_pendingGeolocationRequests.removeOne(frame);
        break;
    default:
        break;
    }
}

void WebPage::permissionSet(QWebPage::Feature feature)
{
    switch (feature) {
    case Geolocation:
        {
        Q_ASSERT(m_drt->testRunner()->isGeolocationPermissionSet());
        foreach (QWebFrame* frame, m_pendingGeolocationRequests)
            if (m_drt->testRunner()->geolocationPermission())
                setFeaturePermission(frame, feature, PermissionGrantedByUser);
            else
                setFeaturePermission(frame, feature, PermissionDeniedByUser);

        m_pendingGeolocationRequests.clear();
        break;
        }
    default:
        break;
    }
}

static QString urlSuitableForTestResult(const QString& url)
{
    if (url.isEmpty() || !url.startsWith(QLatin1String("file://")))
        return url;

    return QFileInfo(url).fileName();
}

void WebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString&)
{
    if (!isTextOutputEnabled())
        return;

    QString newMessage;
    if (!message.isEmpty()) {
        newMessage = message;

        size_t fileProtocol = newMessage.indexOf(QLatin1String("file://"));
        if (fileProtocol != -1) {
            newMessage = newMessage.left(fileProtocol) + urlSuitableForTestResult(newMessage.mid(fileProtocol));
        }
    }

    fprintf(stdout, "CONSOLE MESSAGE: ");
    if (lineNumber)
        fprintf(stdout, "line %d: ", lineNumber);
    fprintf(stdout, "%s\n", newMessage.toUtf8().constData());
}

bool WebPage::javaScriptConfirm(QWebFrame*, const QString& msg)
{
    if (!isTextOutputEnabled())
        return true;

    fprintf(stdout, "CONFIRM: %s\n", msg.toUtf8().constData());
    return true;
}

bool WebPage::javaScriptPrompt(QWebFrame*, const QString& msg, const QString& defaultValue, QString* result)
{
    if (!isTextOutputEnabled())
        return true;

    fprintf(stdout, "PROMPT: %s, default text: %s\n", msg.toUtf8().constData(), defaultValue.toUtf8().constData());
    *result = defaultValue;
    return true;
}

bool WebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
    if (m_drt->testRunner()->waitForPolicy())
        m_drt->testRunner()->notifyDone();

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

bool WebPage::supportsExtension(QWebPage::Extension extension) const
{
    if (extension == QWebPage::ErrorPageExtension)
        return m_drt->testRunner()->shouldHandleErrorPages();

    return false;
}

bool WebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    const QWebPage::ErrorPageExtensionOption* info = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);

    // Lets handle error pages for the main frame for now.
    if (info->frame != mainFrame())
        return false;

    QWebPage::ErrorPageExtensionReturn* errorPage = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

    errorPage->content = QString("data:text/html,<body/>").toUtf8();

    return true;
}

QObject* WebPage::createPlugin(const QString& classId, const QUrl& url, const QStringList& paramNames, const QStringList& paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);

    if (classId == QLatin1String("QProgressBar"))
        return new QProgressBar(view());
    if (classId == QLatin1String("QLabel"))
        return new QLabel(view());

    return 0;
}

void WebPage::setViewGeometry(const QRect& rect)
{
    if (WebViewGraphicsBased* v = qobject_cast<WebViewGraphicsBased*>(view()))
        v->scene()->setSceneRect(QRectF(rect));
    else if (QWidget *v = view())
        v->setGeometry(rect);
}

WebViewGraphicsBased::WebViewGraphicsBased(QWidget* parent)
    : m_item(new QGraphicsWebView)
{
    setScene(new QGraphicsScene(this));
    scene()->addItem(m_item);
}

static DumpRenderTree *s_instance = 0;

DumpRenderTree::DumpRenderTree()
    : m_dumpPixelsForAllTests(false)
    , m_stdin(0)
    , m_enableTextOutput(false)
    , m_standAloneMode(false)
    , m_graphicsBased(false)
    , m_persistentStoragePath(QString(getenv("DUMPRENDERTREE_TEMP")))
{
    ASSERT(!s_instance);
    s_instance = this;

    QByteArray viewMode = getenv("QT_DRT_WEBVIEW_MODE");
    if (viewMode == "graphics")
        setGraphicsBased(true);

    WebKit::initializeWebKitWidgets();
    WebCore::initializeWebCoreQt();
    DumpRenderTreeSupportQt::initialize();

    // Set running in DRT mode for qwebpage to create testable objects.
    DumpRenderTreeSupportQt::setDumpRenderTreeModeEnabled(true);
    DumpRenderTreeSupportQt::overwritePluginDirectories();
    QWebSettings::enablePersistentStorage(m_persistentStoragePath);

    m_networkAccessManager = new NetworkAccessManager(this);
    // create our primary testing page/view.
    if (isGraphicsBased()) {
        WebViewGraphicsBased* view = new WebViewGraphicsBased(0);
        m_page = new WebPage(view, this);
        view->setPage(m_page);
        m_mainView = view;
    } else {
        QWebView* view = new QWebView(0);
        m_page = new WebPage(view, this);
        view->setPage(m_page);
        m_mainView = view;
    }
    // Use a frame group name for all pages created by DumpRenderTree to allow
    // testing of cross-page frame lookup.
    DumpRenderTreeSupportQt::webPageSetGroupName(pageAdapter(), "org.webkit.qt.DumpRenderTree");

    m_mainView->setContextMenuPolicy(Qt::NoContextMenu);
    m_mainView->resize(QSize(TestRunner::viewWidth, TestRunner::viewHeight));

    // clean up cache by resetting quota.
    qint64 quota = webPage()->settings()->offlineWebApplicationCacheQuota();
    webPage()->settings()->setOfflineWebApplicationCacheQuota(quota);

    // create our controllers. This has to be done before connectFrame,
    // as it exports there to the JavaScript DOM window.
    m_controller = new TestRunnerQt(this);
    connect(m_controller, SIGNAL(showPage()), this, SLOT(showPage()));
    connect(m_controller, SIGNAL(hidePage()), this, SLOT(hidePage()));

    // async geolocation permission set by controller
    connect(m_controller, SIGNAL(geolocationPermissionSet()), this, SLOT(geolocationPermissionSet()));

    connect(m_controller, SIGNAL(done()), this, SLOT(dump()));
    m_eventSender = new EventSender(m_page);
    m_textInputController = new TextInputController(m_page);
    m_gcController.reset(new GCController());

    // now connect our different signals
    connect(m_page, SIGNAL(frameCreated(QWebFrame *)),
            this, SLOT(connectFrame(QWebFrame *)));
    connectFrame(m_page->mainFrame());

    connect(m_page, SIGNAL(loadFinished(bool)),
            m_controller, SLOT(maybeDump(bool)));
    // We need to connect to loadStarted() because notifyDone should only
    // dump results itself when the last page loaded in the test has finished loading.
    connect(m_page, SIGNAL(loadStarted()),
            m_controller, SLOT(resetLoadFinished()));
    connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(windowCloseRequested()));
    connect(m_page, SIGNAL(printRequested(QWebFrame*)), this, SLOT(dryRunPrint(QWebFrame*)));

    connect(m_page->mainFrame(), SIGNAL(titleChanged(const QString&)),
            SLOT(titleChanged(const QString&)));
    connect(m_page, SIGNAL(databaseQuotaExceeded(QWebFrame*,QString)),
            this, SLOT(dumpDatabaseQuota(QWebFrame*,QString)));
    connect(m_page, SIGNAL(applicationCacheQuotaExceeded(QWebSecurityOrigin *, quint64, quint64)),
            this, SLOT(dumpApplicationCacheQuota(QWebSecurityOrigin *, quint64, quint64)));
    connect(m_page, SIGNAL(statusBarMessage(const QString&)),
            this, SLOT(statusBarMessage(const QString&)));

    QObject::connect(this, SIGNAL(quit()), qApp, SLOT(quit()), Qt::QueuedConnection);

    DumpRenderTreeSupportQt::setDumpRenderTreeModeEnabled(true);
    DumpRenderTreeSupportQt::setInteractiveFormValidationEnabled(pageAdapter(), true);
    DumpRenderTreeSupportQt::enableMockScrollbars();

    QFocusEvent event(QEvent::FocusIn, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(m_mainView, &event);
}

DumpRenderTree::~DumpRenderTree()
{
    if (!m_redirectOutputFileName.isEmpty())
        fclose(stdout);
    if (!m_redirectErrorFileName.isEmpty())
        fclose(stderr);
    delete m_mainView;
    delete m_stdin;
    s_instance = 0;
}

DumpRenderTree* DumpRenderTree::instance()
{
    return s_instance;
}

static void clearHistory(QWebPage* page)
{
    // QWebHistory::clear() leaves current page, so remove it as well by setting
    // max item count to 0, and then setting it back to it's original value.

    QWebHistory* history = page->history();
    int itemCount = history->maximumItemCount();

    history->clear();
    history->setMaximumItemCount(0);
    history->setMaximumItemCount(itemCount);
}

void DumpRenderTree::dryRunPrint(QWebFrame* frame)
{
#if !defined(QT_NO_PRINTER) && HAVE(QTPRINTSUPPORT)
    NullPrinter printer;
    frame->print(&printer);
#endif
}

void DumpRenderTree::resetToConsistentStateBeforeTesting(const QUrl& url)
{
    // reset so that any current loads are stopped
    // NOTE: that this has to be done before the testRunner is
    // reset or we get timeouts for some tests.
    m_page->blockSignals(true);
    m_page->triggerAction(QWebPage::Stop);
    m_page->blockSignals(false);

    QList<QWebSecurityOrigin> knownOrigins = QWebSecurityOrigin::allOrigins();
    for (int i = 0; i < knownOrigins.size(); ++i)
        knownOrigins[i].setDatabaseQuota(databaseDefaultQuota);

    // reset the testRunner at this point, so that we under no
    // circumstance dump (stop the waitUntilDone timer) during the reset
    // of the DRT.
    m_controller->reset();

    m_jscController = TestRunner::create(url.toString().toStdString(), m_expectedHash.toStdString());

    // reset mouse clicks counter
    m_eventSender->resetClickCount();

    closeRemainingWindows();
    
    // Call setTextSizeMultiplier(1.0) to reset TextZoomFactor and PageZoomFactor too. 
    // It should be done before resetSettings() to guarantee resetting QWebSettings::ZoomTextOnly correctly.
    m_page->mainFrame()->setTextSizeMultiplier(1.0);

    m_page->resetSettings();
#ifndef QT_NO_UNDOSTACK
    m_page->undoStack()->clear();
#endif

    clearHistory(m_page);
    DumpRenderTreeSupportQt::scalePageBy(mainFrameAdapter(), 1, QPoint(0, 0));
    DumpRenderTreeSupportQt::clearFrameName(mainFrameAdapter());
    DumpRenderTreeSupportQt::removeUserStyleSheets(pageAdapter());

    m_page->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAsNeeded);
    m_page->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAsNeeded);

    if (url.scheme() == "http" || url.scheme() == "https") {
        // credentials may exist from previous tests.
        m_page->setNetworkAccessManager(0);
        delete m_networkAccessManager;
        m_networkAccessManager = new NetworkAccessManager(this);
        m_page->setNetworkAccessManager(m_networkAccessManager);
    }

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    DumpRenderTreeSupportQt::resetOriginAccessWhiteLists();

    DumpRenderTreeSupportQt::setWindowsBehaviorAsEditingBehavior(pageAdapter());

    QLocale::setDefault(QLocale::c());

    testRunner()->setDeveloperExtrasEnabled(true);
#ifndef Q_OS_WINCE
    setlocale(LC_ALL, "");
#endif

    DumpRenderTreeSupportQt::clearOpener(mainFrameAdapter());
}

static bool isGlobalHistoryTest(const QUrl& url)
{
    if (url.path().contains("globalhistory/"))
        return true;
    return false;
}

static bool isWebInspectorTest(const QUrl& url)
{
    if (url.path().contains("inspector/"))
        return true;
    return false;
}

static bool isDumpAsTextTest(const QUrl& url)
{
    if (url.path().contains("dumpAsText/"))
        return true;
    return false;
}


void DumpRenderTree::open(const QUrl& url)
{
    DumpRenderTreeSupportQt::dumpResourceLoadCallbacksPath(QFileInfo(url.toString()).path());
    resetToConsistentStateBeforeTesting(url);

    if (isWebInspectorTest(m_page->mainFrame()->url()))
        testRunner()->closeWebInspector();

    if (isWebInspectorTest(url))
        testRunner()->showWebInspector();

    if (isDumpAsTextTest(url))
        m_jscController->setDumpAsText(true);

    if (isGlobalHistoryTest(url))
        testRunner()->dumpHistoryCallbacks();

    // W3C SVG tests expect to be 480x360
    bool isW3CTest = url.toString().contains("svg/W3C-SVG-1.1");
    int width = isW3CTest ? TestRunner::w3cSVGViewWidth : TestRunner::viewWidth;
    int height = isW3CTest ? TestRunner::w3cSVGViewHeight : TestRunner::viewHeight;
    m_mainView->resize(QSize(width, height));
    m_page->setPreferredContentsSize(QSize());
    m_page->setViewportSize(QSize(width, height));

    QFocusEvent ev(QEvent::FocusIn);
    m_page->event(&ev);

    WebKit::QtTestSupport::clearMemoryCaches();

    WebKit::QtTestSupport::initializeTestFonts();

    DumpRenderTreeSupportQt::dumpFrameLoader(url.toString().contains("loading/"));
    setTextOutputEnabled(true);
    m_page->mainFrame()->load(url);
}

void DumpRenderTree::readLine()
{
    if (!m_stdin) {
        m_stdin = new QFile;
        m_stdin->open(stdin, QFile::ReadOnly);

        if (!m_stdin->isReadable()) {
            emit quit();
            return;
        }
    }

    QByteArray line = m_stdin->readLine().trimmed();

    if (line.isEmpty()) {
        emit quit();
        return;
    }

    processLine(QString::fromLocal8Bit(line.constData(), line.length()));
}

void DumpRenderTree::processArgsLine(const QStringList &args)
{
    setStandAloneMode(true);

    m_standAloneModeTestList = args;

    QFileInfo firstEntry(m_standAloneModeTestList.first());
    if (firstEntry.isDir()) {
        QDir folderEntry(m_standAloneModeTestList.first());
        QStringList supportedExt;
        // Check for all supported extensions (from Scripts/webkitpy/layout_tests/layout_package/test_files.py).
        supportedExt << "*.html" << "*.shtml" << "*.xml" << "*.xhtml" << "*.xhtmlmp" << "*.pl" << "*.php" << "*.svg";
        m_standAloneModeTestList = folderEntry.entryList(supportedExt, QDir::Files);
        for (int i = 0; i < m_standAloneModeTestList.size(); ++i)
            m_standAloneModeTestList[i] = folderEntry.absoluteFilePath(m_standAloneModeTestList[i]);
    }
    connect(this, SIGNAL(ready()), this, SLOT(loadNextTestInStandAloneMode()), Qt::QueuedConnection);

    if (!m_standAloneModeTestList.isEmpty()) {
        QString first = m_standAloneModeTestList.takeFirst();
        processLine(first);
    }
}

void DumpRenderTree::loadNextTestInStandAloneMode()
{
    if (m_standAloneModeTestList.isEmpty()) {
        emit quit();
        return;
    }
    QString first = m_standAloneModeTestList.takeFirst();
    processLine(first);
}

void DumpRenderTree::processLine(const QString &input)
{
    TestCommand command = parseInputLine(std::string(input.toLatin1().constData()));
    QString pathOrURL = QLatin1String(command.pathOrURL.c_str());
    m_dumpPixelsForCurrentTest = command.shouldDumpPixels || m_dumpPixelsForAllTests;
    m_expectedHash = QLatin1String(command.expectedPixelHash.c_str());

    if (pathOrURL.startsWith(QLatin1String("http:"))
            || pathOrURL.startsWith(QLatin1String("https:"))
            || pathOrURL.startsWith(QLatin1String("file:"))
            || pathOrURL == QLatin1String("about:blank")) {
        open(QUrl(pathOrURL));
    } else {
        QFileInfo fi(pathOrURL);

        if (!fi.exists()) {
            QDir currentDir = QDir::currentPath();

            // Try to be smart about where the test is located
            if (currentDir.dirName() == QLatin1String("LayoutTests"))
                fi = QFileInfo(currentDir, pathOrURL.replace(QRegExp(".*?LayoutTests/(.*)"), "\\1"));
            else if (!pathOrURL.contains(QLatin1String("LayoutTests")))
                fi = QFileInfo(currentDir, pathOrURL.prepend(QLatin1String("LayoutTests/")));

            if (!fi.exists()) {
                emit ready();
                return;
            }
        }

        open(QUrl::fromLocalFile(fi.absoluteFilePath()));
    }

    if (command.timeout > 0)
        setTimeout(command.timeout);
    fflush(stdout);
}

void DumpRenderTree::closeRemainingWindows()
{
    foreach (QObject* widget, windows)
        delete widget;
    windows.clear();
}

void DumpRenderTree::initJSObjects()
{
    QWebFrame *frame = qobject_cast<QWebFrame*>(sender());
    Q_ASSERT(frame);

    JSContextRef context = 0;
    JSObjectRef window = 0;

    DumpRenderTreeSupportQt::getJSWindowObject(frame->handle(), &context, &window);

    frame->addToJavaScriptWindowObject(QLatin1String("testRunner"), m_controller);
    frame->addToJavaScriptWindowObject(QLatin1String("eventSender"), m_eventSender);
    frame->addToJavaScriptWindowObject(QLatin1String("textInputController"), m_textInputController);
    m_gcController->makeWindowObject(context, window, 0);

    if (m_jscController) {
        JSObjectRef dummyWindow = JSObjectMake(context, 0, 0);
        m_jscController->makeWindowObject(context, dummyWindow, 0);
        JSRetainPtr<JSStringRef> testRunnerName(Adopt, JSStringCreateWithUTF8CString("testRunner"));
        JSValueRef wrappedTestRunner = JSObjectGetProperty(context, dummyWindow, testRunnerName.get(), 0);
        JSRetainPtr<JSStringRef> helperScript(Adopt, JSStringCreateWithUTF8CString("(function() {\n"
                                                                                   "    function bind(fun, thisArg) {\n"
                                                                                   "        return function() {\n"
                                                                                   "            return fun.apply(thisArg, Array.prototype.slice.call(arguments));\n"
                                                                                   "        }\n"
                                                                                   "    }\n"
                                                                                   "for (var prop in this.jscBasedTestRunner) {\n"
                                                                                   "    var pd = Object.getOwnPropertyDescriptor(this.qtBasedTestRunner, prop);\n"
                                                                                   "    if (pd !== undefined) continue;\n"
                                                                                   "    pd = Object.getOwnPropertyDescriptor(this.jscBasedTestRunner, prop);\n"
                                                                                   "    this.qtBasedTestRunner[prop] = bind(this.jscBasedTestRunner[prop], this.jscBasedTestRunner);\n"
                                                                                   "}\n"
                                                                                   "}).apply(this)\n"));

        JSRetainPtr<JSStringRef> qtBasedTestRunnerName(Adopt, JSStringCreateWithUTF8CString("qtBasedTestRunner"));
        JSRetainPtr<JSStringRef> jscBasedTestRunnerName(Adopt, JSStringCreateWithUTF8CString("jscBasedTestRunner"));

        JSObjectRef args = JSObjectMake(context, 0, 0);
        JSObjectSetProperty(context, args, qtBasedTestRunnerName.get(), JSObjectGetProperty(context, window, testRunnerName.get(), 0), 0, 0);
        JSObjectSetProperty(context, args, jscBasedTestRunnerName.get(), wrappedTestRunner, 0, 0);

        JSValueRef ex = 0;
        JSEvaluateScript(context, helperScript.get(), args, 0, 0, &ex);
        if (ex) {
            JSRetainPtr<JSStringRef> msg(Adopt, JSValueToStringCopy(context, ex, 0));
            fprintf(stderr, "Error evaluating TestRunner setup-script: %s\n", qPrintable(JSStringCopyQString(msg.get())));
        }
    }

    DumpRenderTreeSupportQt::injectInternalsObject(frame->handle());
}

void DumpRenderTree::showPage()
{
    m_mainView->show();
    // we need a paint event but cannot process all the events
    QPixmap pixmap(m_mainView->size());
    m_mainView->render(&pixmap);
}

void DumpRenderTree::hidePage()
{
    m_mainView->hide();
}

QString DumpRenderTree::dumpFrameScrollPosition(QWebFrame* frame)
{
    if (!frame || !DumpRenderTreeSupportQt::hasDocumentElement(frame->handle()))
        return QString();

    QString result;
    QPoint pos = frame->scrollPosition();
    if (pos.x() > 0 || pos.y() > 0) {
        QWebFrame* parent = qobject_cast<QWebFrame *>(frame->parent());
        if (parent)
            result.append(QString("frame '%1' ").arg(frame->title()));
        result.append(QString("scrolled to %1,%2\n").arg(pos.x()).arg(pos.y()));
    }

    if (m_jscController->dumpChildFrameScrollPositions()) {
        QList<QWebFrame*> children = frame->childFrames();
        for (int i = 0; i < children.size(); ++i)
            result += dumpFrameScrollPosition(children.at(i));
    }
    return result;
}

QString DumpRenderTree::dumpFramesAsText(QWebFrame* frame)
{
    if (!frame || !DumpRenderTreeSupportQt::hasDocumentElement(frame->handle()))
        return QString();

    QString result;
    QWebFrame* parent = qobject_cast<QWebFrame*>(frame->parent());
    if (parent) {
        result.append(QLatin1String("\n--------\nFrame: '"));
        result.append(frame->frameName());
        result.append(QLatin1String("'\n--------\n"));
    }

    QString innerText = frame->toPlainText();
    result.append(innerText);
    result.append(QLatin1String("\n"));

    if (m_jscController->dumpChildFramesAsText()) {
        QList<QWebFrame *> children = frame->childFrames();
        for (int i = 0; i < children.size(); ++i)
            result += dumpFramesAsText(children.at(i));
    }

    return result;
}

static QString dumpHistoryItem(const QWebHistoryItem& item, int indent, bool current)
{
    QString result;

    int start = 0;
    if (current) {
        result.append(QLatin1String("curr->"));
        start = 6;
    }
    for (int i = start; i < indent; i++)
        result.append(' ');

    QString url = item.url().toString();
    if (url.contains("file://")) {
        static QString layoutTestsString("/LayoutTests/");
        static QString fileTestString("(file test):");

        QString res = url.mid(url.indexOf(layoutTestsString) + layoutTestsString.length());
        if (res.isEmpty())
            return result;

        result.append(fileTestString);
        result.append(res);
    } else {
        result.append(url);
    }

    QString target = DumpRenderTreeSupportQt::historyItemTarget(item);
    if (!target.isEmpty())
        result.append(QString(QLatin1String(" (in frame \"%1\")")).arg(target));

    if (DumpRenderTreeSupportQt::isTargetItem(item))
        result.append(QLatin1String("  **nav target**"));
    result.append(QLatin1String("\n"));

    QMap<QString, QWebHistoryItem> children = DumpRenderTreeSupportQt::getChildHistoryItems(item);
    foreach (QWebHistoryItem item, children)
        result += dumpHistoryItem(item, 12, false);

    return result;
}

QString DumpRenderTree::dumpBackForwardList(QWebPage* page)
{
    QWebHistory* history = page->history();

    QString result;
    result.append(QLatin1String("\n============== Back Forward List ==============\n"));

    // FORMAT:
    // "        (file test):fast/loader/resources/click-fragment-link.html  **nav target**"
    // "curr->  (file test):fast/loader/resources/click-fragment-link.html#testfragment  **nav target**"

    int maxItems = history->maximumItemCount();

    foreach (const QWebHistoryItem item, history->backItems(maxItems)) {
        if (!item.isValid())
            continue;
        result.append(dumpHistoryItem(item, 8, false));
    }

    QWebHistoryItem item = history->currentItem();
    if (item.isValid())
        result.append(dumpHistoryItem(item, 8, true));

    foreach (const QWebHistoryItem item, history->forwardItems(maxItems)) {
        if (!item.isValid())
            continue;
        result.append(dumpHistoryItem(item, 8, false));
    }

    result.append(QLatin1String("===============================================\n"));
    return result;
}

static const char *methodNameStringForFailedTest(TestRunner *controller)
{
    const char *errorMessage;
    if (controller->dumpAsText())
        errorMessage = "[documentElement innerText]";
    // FIXME: Add when we have support
    //else if (controller->dumpDOMAsWebArchive())
    //    errorMessage = "[[mainFrame DOMDocument] webArchive]";
    //else if (controller->dumpSourceAsWebArchive())
    //    errorMessage = "[[mainFrame dataSource] webArchive]";
    else
        errorMessage = "[mainFrame renderTreeAsExternalRepresentation]";

    return errorMessage;
}

void DumpRenderTree::dump()
{
    // Prevent any further frame load or resource load callbacks from appearing after we dump the result.
    DumpRenderTreeSupportQt::dumpFrameLoader(false);
    DumpRenderTreeSupportQt::dumpResourceLoadCallbacks(false);

    QWebFrame *mainFrame = m_page->mainFrame();

    if (isStandAloneMode()) {
        QString markup = mainFrame->toHtml();
        fprintf(stdout, "Source:\n\n%s\n", markup.toUtf8().constData());
    }

    QString mimeType = DumpRenderTreeSupportQt::responseMimeType(mainFrame->handle());
    if (mimeType == "text/plain")
        m_jscController->setDumpAsText(true);

    // Dump render text...
    QString resultString;
    QString resultContentType = "text/plain";
    QByteArray resultData;
    if (m_controller->shouldDumpAsAudio()) {
        resultContentType = "audio/wav";
        resultData = m_controller->audioData();
    } else if (m_jscController->dumpAsText())
        resultString = dumpFramesAsText(mainFrame);
    else {
        resultString = DumpRenderTreeSupportQt::frameRenderTreeDump(mainFrame->handle());
        resultString += dumpFrameScrollPosition(mainFrame);
    }
    if (!resultString.isEmpty()) {
        fprintf(stdout, "Content-Type: %s\n", resultContentType.toUtf8().constData());
        fprintf(stdout, "%s", resultString.toUtf8().constData());

        if (m_jscController->dumpBackForwardList()) {
            fprintf(stdout, "%s", dumpBackForwardList(webPage()).toUtf8().constData());
            foreach (QObject* widget, windows) {
                QWebPage* page = qobject_cast<QWebPage*>(widget->findChild<QWebPage*>());
                fprintf(stdout, "%s", dumpBackForwardList(page).toUtf8().constData());
            }
        }
    } else if (!resultData.isEmpty()) {
        fprintf(stdout, "Content-Type: %s\n", resultContentType.toUtf8().constData());
        fprintf(stdout, "Content-Transfer-Encoding: base64\n");
        fprintf(stdout, "%s", resultData.toBase64().constData());
    } else
        printf("ERROR: nil result from %s", methodNameStringForFailedTest(m_jscController.get()));

    // signal end of text block
    fputs("#EOF\n", stdout);
    fputs("#EOF\n", stderr);

    if (m_dumpPixelsForCurrentTest && m_jscController->generatePixelResults()) {
        QImage image;
        if (!m_jscController->isPrinting()) {
            image = QImage(m_page->viewportSize(), QImage::Format_ARGB32);
            image.fill(Qt::white);
            QPainter painter(&image);
            mainFrame->render(&painter);
            painter.end();
        } else
            image = DumpRenderTreeSupportQt::paintPagesWithBoundaries(mainFrame->handle());

        if (DumpRenderTreeSupportQt::trackRepaintRects(mainFrameAdapter())) {
            QVector<QRect> repaintRects;
            DumpRenderTreeSupportQt::getTrackedRepaintRects(mainFrameAdapter(), repaintRects);
            QImage mask(image.size(), image.format());
            mask.fill(QColor(0, 0, 0, 0.66 * 255));

            QPainter maskPainter(&mask);
            maskPainter.setCompositionMode(QPainter::CompositionMode_Source);
            for (int i = 0; i < repaintRects.size(); ++i)
                maskPainter.fillRect(repaintRects[i], Qt::transparent);

            QPainter painter(&image);
            painter.drawImage(image.rect(), mask);

            DumpRenderTreeSupportQt::setTrackRepaintRects(mainFrameAdapter(), false);
        }

        QCryptographicHash hash(QCryptographicHash::Md5);
        for (int row = 0; row < image.height(); ++row)
            hash.addData(reinterpret_cast<const char*>(image.scanLine(row)), image.width() * 4);
        QString actualHash = hash.result().toHex();

        fprintf(stdout, "\nActualHash: %s\n", qPrintable(actualHash));

        bool dumpImage = true;

        if (!m_expectedHash.isEmpty()) {
            Q_ASSERT(m_expectedHash.length() == 32);
            fprintf(stdout, "\nExpectedHash: %s\n", qPrintable(m_expectedHash));

            if (m_expectedHash == actualHash)
                dumpImage = false;
        }

        if (dumpImage) {
            image.setText("checksum", actualHash);

            QBuffer buffer;
            buffer.open(QBuffer::WriteOnly);
            image.save(&buffer, "PNG");
            buffer.close();
            const QByteArray &data = buffer.data();

            printf("Content-Type: %s\n", "image/png");
            printf("Content-Length: %lu\n", static_cast<unsigned long>(data.length()));

            const quint32 bytesToWriteInOneChunk = 1 << 15;
            quint32 dataRemainingToWrite = data.length();
            const char *ptr = data.data();
            while (dataRemainingToWrite) {
                quint32 bytesToWriteInThisChunk = qMin(dataRemainingToWrite, bytesToWriteInOneChunk);
                quint32 bytesWritten = fwrite(ptr, 1, bytesToWriteInThisChunk, stdout);
                if (bytesWritten != bytesToWriteInThisChunk)
                    break;
                dataRemainingToWrite -= bytesWritten;
                ptr += bytesWritten;
            }
        }

        fflush(stdout);
    }

    puts("#EOF");   // terminate the (possibly empty) pixels block

    fflush(stdout);
    fflush(stderr);

     emit ready();
}

void DumpRenderTree::titleChanged(const QString &s)
{
    if (m_jscController->dumpTitleChanges())
        printf("TITLE CHANGED: '%s'\n", s.toUtf8().data());
}

void DumpRenderTree::connectFrame(QWebFrame *frame)
{
    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(initJSObjects()));
    connect(frame, SIGNAL(provisionalLoad()),
            testRunner(), SLOT(provisionalLoad()));
}

void DumpRenderTree::dumpDatabaseQuota(QWebFrame* frame, const QString& dbName)
{
    if (!m_jscController->dumpDatabaseCallbacks())
        return;
    QWebSecurityOrigin origin = frame->securityOrigin();
    printf("UI DELEGATE DATABASE CALLBACK: exceededDatabaseQuotaForSecurityOrigin:{%s, %s, %i} database:%s\n",
           origin.scheme().toUtf8().data(),
           origin.host().toUtf8().data(),
           origin.port(),
           dbName.toUtf8().data());
    origin.setDatabaseQuota(databaseDefaultQuota);
}

void DumpRenderTree::dumpApplicationCacheQuota(QWebSecurityOrigin* origin, quint64 defaultOriginQuota, quint64 totalSpaceNeeded)
{
    if (m_jscController->dumpApplicationCacheDelegateCallbacks()) {
        // For example, numbers from 30000 - 39999 will output as 30000.
        // Rounding up or down not really matter for these tests. It's
        // sufficient to just get a range of 10000 to determine if we were
        // above or below a threshold.
        quint64 truncatedSpaceNeeded = (totalSpaceNeeded / 10000) * 10000;
        printf("UI DELEGATE APPLICATION CACHE CALLBACK: exceededApplicationCacheOriginQuotaForSecurityOrigin:{%s, %s, %i} totalSpaceNeeded:~%llu\n",
               origin->scheme().toUtf8().data(),
               origin->host().toUtf8().data(),
               origin->port(),
               truncatedSpaceNeeded
               );
    }

    if (m_jscController->disallowIncreaseForApplicationCacheQuota())
        return;

    origin->setApplicationCacheQuota(defaultOriginQuota);
}

void DumpRenderTree::statusBarMessage(const QString& message)
{
    if (!m_jscController->dumpStatusCallbacks())
        return;

    printf("UI DELEGATE STATUS CALLBACK: setStatusText:%s\n", message.toUtf8().constData());
}

QWebPage *DumpRenderTree::createWindow()
{
    if (!m_jscController->canOpenWindows())
        return 0;

    // Create a dummy container object to track the page in DRT.
    // QObject is used instead of QWidget to prevent DRT from
    // showing the main view when deleting the container.

    QObject* container = new QObject(m_mainView);
    // create a QWebPage we want to return
    QWebPage* page = static_cast<QWebPage*>(new WebPage(container, this));
    // gets cleaned up in closeRemainingWindows()
    windows.append(container);

    // connect the needed signals to the page
    connect(page, SIGNAL(frameCreated(QWebFrame*)), this, SLOT(connectFrame(QWebFrame*)));
    connectFrame(page->mainFrame());
    connect(page, SIGNAL(loadFinished(bool)), m_controller, SLOT(maybeDump(bool)));
    connect(page, SIGNAL(windowCloseRequested()), this, SLOT(windowCloseRequested()));

    // Use a frame group name for all pages created by DumpRenderTree to allow
    // testing of cross-page frame lookup.
    DumpRenderTreeSupportQt::webPageSetGroupName(page->handle(), "org.webkit.qt.DumpRenderTree");

    return page;
}

void DumpRenderTree::windowCloseRequested()
{
    QWebPage* page = qobject_cast<QWebPage*>(sender());
    QObject* container = page->parent();
    windows.removeAll(container);
    container->deleteLater();
}

int DumpRenderTree::windowCount() const
{
// include the main view in the count
    return windows.count() + 1;
}

void DumpRenderTree::geolocationPermissionSet() 
{
    m_page->permissionSet(QWebPage::Geolocation);
}

void DumpRenderTree::switchFocus(bool focused)
{
    QFocusEvent event((focused) ? QEvent::FocusIn : QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    if (!isGraphicsBased())
        QApplication::sendEvent(m_mainView, &event);
    else {
        if (WebViewGraphicsBased* view = qobject_cast<WebViewGraphicsBased*>(m_mainView))
            view->scene()->sendEvent(view->graphicsView(), &event);
    }

}

QWebPageAdapter* DumpRenderTree::pageAdapter() const
{
    return m_page->handle();
}

QWebFrameAdapter* DumpRenderTree::mainFrameAdapter() const
{
    return m_page->mainFrame()->handle();
}

QList<WebPage*> DumpRenderTree::getAllPages() const
{
    QList<WebPage*> pages;
    pages.append(m_page);
    foreach (QObject* widget, windows) {
        if (WebPage* page = widget->findChild<WebPage*>())
            pages.append(page);
    }
    return pages;
}

void DumpRenderTree::setTimeout(int timeout)
{
    m_controller->setTimeout(timeout);
}

void DumpRenderTree::setShouldTimeout(bool flag)
{
    m_controller->setShouldTimeout(flag);
}
