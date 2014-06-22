/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
    Copyright (C) 2010 Holger Hans Peter Freyther

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "../util.h"
#include "../WebCoreSupport/DumpRenderTreeSupportQt.h"
#include <QClipboard>
#include <QDir>
#include <QGraphicsWidget>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMimeDatabase>
#include <QPushButton>
#include <QStateMachine>
#include <QStyle>
#include <QtTest/QtTest>
#include <QTextCharFormat>
#include <private/qinputmethod_p.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qgraphicswebview.h>
#include <qnetworkcookiejar.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qpa/qplatforminputcontext.h>
#include <qwebdatabase.h>
#include <qwebelement.h>
#include <qwebframe.h>
#include <qwebhistory.h>
#include <qwebpage.h>
#include <qwebsecurityorigin.h>
#include <qwebview.h>
#include <qimagewriter.h>

static void removeRecursive(const QString& dirname)
{
    QDir dir(dirname);
    QFileInfoList entries(dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot));
    for (int i = 0; i < entries.count(); ++i)
        if (entries[i].isDir())
            removeRecursive(entries[i].filePath());
        else
            dir.remove(entries[i].fileName());
    QDir().rmdir(dirname);
}

class TestInputContext : public QPlatformInputContext
{
public:
    TestInputContext()
    : m_visible(false)
    {
        QInputMethodPrivate* inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
        inputMethodPrivate->testContext = this;
    }

    ~TestInputContext()
    {
        QInputMethodPrivate* inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
        inputMethodPrivate->testContext = 0;
    }

    virtual void showInputPanel()
    {
        m_visible = true;
    }
    virtual void hideInputPanel()
    {
        m_visible = false;
    }
    virtual bool isInputPanelVisible() const
    {
        return m_visible;
    }

    bool m_visible;
};

class tst_QWebPage : public QObject
{
    Q_OBJECT

public:
    tst_QWebPage();
    virtual ~tst_QWebPage();

public Q_SLOTS:
    void init();
    void cleanup();
    void cleanupFiles();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void thirdPartyCookiePolicy();
    void contextMenuCopy();
    void contextMenuPopulatedOnce();
    void acceptNavigationRequest();
    void domainSpecificKeyEvent();
    void geolocationRequestJS();
    void loadFinished();
    void actionStates();
    void popupFormSubmission();
    void acceptNavigationRequestWithNewWindow();
    void userStyleSheet();
    void userStyleSheetFromLocalFileUrl();
    void userStyleSheetFromQrcUrl();
    void loadHtml5Video();
    void modified();
    void contextMenuCrash();
    void updatePositionDependentActionsCrash();
    void database();
    void createPluginWithPluginsEnabled();
    void createPluginWithPluginsDisabled();
    void destroyPlugin_data();
    void destroyPlugin();
    void createViewlessPlugin_data();
    void createViewlessPlugin();
    void graphicsWidgetPlugin();
    void multiplePageGroupsAndLocalStorage();
    void cursorMovements();
    void textSelection();
    void textEditing();
    void backActionUpdate();
    void frameAt();
    void requestCache();
    void loadCachedPage();
    void protectBindingsRuntimeObjectsFromCollector();
    void localURLSchemes();
    void testOptionalJSObjects();
    void testLocalStorageVisibility();
    void testEnablePersistentStorage();
    void consoleOutput();
    void inputMethods_data();
    void inputMethods();
    void inputMethodsTextFormat_data();
    void inputMethodsTextFormat();
    void defaultTextEncoding();
    void errorPageExtension();
    void errorPageExtensionInIFrames();
    void errorPageExtensionInFrameset();
    void errorPageExtensionLoadFinished();
    void userAgentApplicationName();
    void userAgentNewlineStripping();
    void undoActionHaveCustomText();

    void viewModes();

    void crashTests_LazyInitializationOfMainFrame();

    void screenshot_data();
    void screenshot();

    void changeVisibilityState();

#if defined(ENABLE_WEBGL) && ENABLE_WEBGL
    void acceleratedWebGLScreenshotWithoutView();
    void unacceleratedWebGLScreenshotWithoutView();
#endif

    void originatingObjectInNetworkRequests();
    void networkReplyParentDidntChange();
    void destroyQNAMBeforeAbortDoesntCrash();
    void testJSPrompt();
    void showModalDialog();
    void testStopScheduledPageRefresh();
    void findText();
    void supportedContentType();
    // [Qt] tst_QWebPage::infiniteLoopJS() timeouts with DFG JIT
    // https://bugs.webkit.org/show_bug.cgi?id=79040
    // void infiniteLoopJS();
    void navigatorCookieEnabled();
    void deleteQWebViewTwice();
    void renderOnRepaintRequestedShouldNotRecurse();
    void loadSignalsOrder_data();
    void loadSignalsOrder();
    void openWindowDefaultSize();
    void cssMediaTypeGlobalSetting();
    void cssMediaTypePageSetting();

#ifdef Q_OS_MAC
    void macCopyUnicodeToClipboard();
#endif

private:
    QWebView* m_view;
    QWebPage* m_page;
    QString tmpDirPath() const
    {
        static QString tmpd = QDir::tempPath() + "/tst_qwebpage-"
            + QDateTime::currentDateTime().toString(QLatin1String("yyyyMMddhhmmss"));
        return tmpd;
    }
};

tst_QWebPage::tst_QWebPage()
{
}

tst_QWebPage::~tst_QWebPage()
{
}

void tst_QWebPage::init()
{
    m_view = new QWebView();
    m_page = m_view->page();
}

void tst_QWebPage::cleanup()
{
    delete m_view;
}

void tst_QWebPage::cleanupFiles()
{
    removeRecursive(tmpDirPath());
}

void tst_QWebPage::initTestCase()
{
    cleanupFiles(); // In case there are old files from previous runs
}

void tst_QWebPage::cleanupTestCase()
{
    cleanupFiles(); // Be nice
}

class NavigationRequestOverride : public QWebPage
{
public:
    NavigationRequestOverride(QWebView* parent, bool initialValue) : QWebPage(parent), m_acceptNavigationRequest(initialValue) {}

    bool m_acceptNavigationRequest;
protected:
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest &request, QWebPage::NavigationType type) {
        Q_UNUSED(frame);
        Q_UNUSED(request);
        Q_UNUSED(type);

        return m_acceptNavigationRequest;
    }
};

void tst_QWebPage::acceptNavigationRequest()
{
    QSignalSpy loadSpy(m_view, SIGNAL(loadFinished(bool)));

    NavigationRequestOverride* newPage = new NavigationRequestOverride(m_view, false);
    m_view->setPage(newPage);

    m_view->setHtml(QString("<html><body><form name='tstform' action='data:text/html,foo'method='get'>"
                            "<input type='text'><input type='submit'></form></body></html>"), QUrl());
    QTRY_COMPARE(loadSpy.count(), 1);

    m_view->page()->mainFrame()->evaluateJavaScript("tstform.submit();");

    newPage->m_acceptNavigationRequest = true;
    m_view->page()->mainFrame()->evaluateJavaScript("tstform.submit();");
    QTRY_COMPARE(loadSpy.count(), 2);

    QCOMPARE(m_view->page()->mainFrame()->toPlainText(), QString("foo?"));

    // Restore default page
    m_view->setPage(0);
}

void tst_QWebPage::domainSpecificKeyEvent()
{
    QWebView webView;
    webView.show();
    QTest::qWaitForWindowExposed(&webView);

    webView.setHtml(QLatin1String("<html><head>"
        "<script>"
        "var receivedKeyArray = new Array();"
        "function keyEvent(e) {"
        "receivedKeyArray.push(e.type+':'+e.keyCode+':'+e.charCode);"
        "};"
        "window.onkeyup = keyEvent; window.onkeypress = keyEvent; window.onkeydown = keyEvent;"
        "</script></head><body>test</body></html>"));

    // Enable settings to use nativeVirtualKey as DOM key value.
    webView.page()->setProperty("_q_useNativeVirtualKeyAsDOMKey", true);
    // Simulate domain specific keyevent to WebKit by passing it as nativeVirtualKey in QKeyEvent.
    // Qt::Key_Pause --> 0x51
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Pause, Qt::NoModifier, 0, 81, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    keyEvent = QKeyEvent(QEvent::KeyRelease, Qt::Key_Pause, Qt::NoModifier, 0, 81, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    const QLatin1String expectedReceivedKeyArray1("keydown:81:0,keyup:81:0");
    QString receivedKeyArray = webView.page()->mainFrame()->evaluateJavaScript(QLatin1String("receivedKeyArray")).toStringList().join(",");
    QVERIFY(receivedKeyArray == expectedReceivedKeyArray1);

    // Normal PC keyboard key converstion flow shouldn't be affected when sending nativeVirtual key as 0.
    // Qt::Key_Pause --> VK_PAUSE(0x13)
    webView.page()->mainFrame()->evaluateJavaScript(QLatin1String("receivedKeyArray = new Array()")); // Reset
    keyEvent = QKeyEvent(QEvent::KeyPress, Qt::Key_Pause, Qt::NoModifier, 0, 0, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    keyEvent = QKeyEvent(QEvent::KeyRelease, Qt::Key_Pause, Qt::NoModifier, 0, 0, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    const QLatin1String expectedReceivedKeyArray2("keydown:19:0,keyup:19:0");
    receivedKeyArray = webView.page()->mainFrame()->evaluateJavaScript(QLatin1String("receivedKeyArray")).toStringList().join(",");
    QVERIFY(receivedKeyArray == expectedReceivedKeyArray2);

    // Negative case.
    // Disable settings to use nativeVirtualKey as DOM key value.
    webView.page()->setProperty("_q_useNativeVirtualKeyAsDOMKey", false);
    // Qt::Key_Pause --> VK_PAUSE(0x13)
    webView.page()->mainFrame()->evaluateJavaScript(QLatin1String("receivedKeyArray = new Array()")); // Reset
    keyEvent = QKeyEvent(QEvent::KeyPress, Qt::Key_Pause, Qt::NoModifier, 0, 81, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    keyEvent = QKeyEvent(QEvent::KeyRelease, Qt::Key_Pause, Qt::NoModifier, 0, 81, 0);
    QApplication::sendEvent(&webView, &keyEvent);
    const QLatin1String expectedReceivedKeyArray3("keydown:19:0,keyup:19:0");
    receivedKeyArray = webView.page()->mainFrame()->evaluateJavaScript(QLatin1String("receivedKeyArray")).toStringList().join(",");
    QVERIFY(receivedKeyArray == expectedReceivedKeyArray3);
}

class JSTestPage : public QWebPage
{
Q_OBJECT
public:
    JSTestPage(QObject* parent = 0)
    : QWebPage(parent) {}

    virtual bool shouldInterruptJavaScript()
    {
        return true;
    }
public Q_SLOTS:
    void requestPermission(QWebFrame* frame, QWebPage::Feature feature)
    {
        if (m_allowGeolocation)
            setFeaturePermission(frame, feature, PermissionGrantedByUser);
        else 
            setFeaturePermission(frame, feature, PermissionDeniedByUser);
    }

public:
    void setGeolocationPermission(bool allow) 
    {
        m_allowGeolocation = allow;
    }

private: 
    bool m_allowGeolocation;
};

// [Qt] tst_QWebPage::infiniteLoopJS() timeouts with DFG JIT
// https://bugs.webkit.org/show_bug.cgi?id=79040
/*
void tst_QWebPage::infiniteLoopJS()
{
    JSTestPage* newPage = new JSTestPage(m_view);
    m_view->setPage(newPage);
    m_view->setHtml(QString("<html><body>test</body></html>"), QUrl());
    m_view->page()->mainFrame()->evaluateJavaScript("var run = true;var a = 1;while(run){a++;}");
    delete newPage;
}
*/

void tst_QWebPage::geolocationRequestJS()
{
    JSTestPage* newPage = new JSTestPage(m_view);

    if (newPage->mainFrame()->evaluateJavaScript(QLatin1String("!navigator.geolocation")).toBool()) {
        delete newPage;
        W_QSKIP("Geolocation is not supported.", SkipSingle);
    }

    connect(newPage, SIGNAL(featurePermissionRequested(QWebFrame*, QWebPage::Feature)),
            newPage, SLOT(requestPermission(QWebFrame*, QWebPage::Feature)));

    newPage->setGeolocationPermission(false);
    m_view->setPage(newPage);
    m_view->setHtml(QString("<html><body>test</body></html>"), QUrl());
    m_view->page()->mainFrame()->evaluateJavaScript("var errorCode = 0; function error(err) { errorCode = err.code; } function success(pos) { } navigator.geolocation.getCurrentPosition(success, error)");
    QTest::qWait(2000);
    QVariant empty = m_view->page()->mainFrame()->evaluateJavaScript("errorCode");

    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=102235", Continue);
    QVERIFY(empty.type() == QVariant::Double && empty.toInt() != 0);

    newPage->setGeolocationPermission(true);
    m_view->page()->mainFrame()->evaluateJavaScript("errorCode = 0; navigator.geolocation.getCurrentPosition(success, error);");
    empty = m_view->page()->mainFrame()->evaluateJavaScript("errorCode");

    //http://dev.w3.org/geo/api/spec-source.html#position
    //PositionError: const unsigned short PERMISSION_DENIED = 1;
    QVERIFY(empty.type() == QVariant::Double && empty.toInt() != 1);
    delete newPage;
}

void tst_QWebPage::loadFinished()
{
    qRegisterMetaType<QWebFrame*>("QWebFrame*");
    qRegisterMetaType<QNetworkRequest*>("QNetworkRequest*");
    QSignalSpy spyLoadStarted(m_view, SIGNAL(loadStarted()));
    QSignalSpy spyLoadFinished(m_view, SIGNAL(loadFinished(bool)));

    m_view->page()->mainFrame()->load(QUrl("data:text/html,<frameset cols=\"25%,75%\"><frame src=\"data:text/html,"
                                           "<head><meta http-equiv='refresh' content='1'></head>foo \">"
                                           "<frame src=\"data:text/html,bar\"></frameset>"));
    QTRY_COMPARE(spyLoadFinished.count(), 1);

    QTRY_VERIFY(spyLoadStarted.count() > 1);
    QTRY_VERIFY(spyLoadFinished.count() > 1);

    spyLoadFinished.clear();

    m_view->page()->mainFrame()->load(QUrl("data:text/html,<frameset cols=\"25%,75%\"><frame src=\"data:text/html,"
                                           "foo \"><frame src=\"data:text/html,bar\"></frameset>"));
    QTRY_COMPARE(spyLoadFinished.count(), 1);
    QCOMPARE(spyLoadFinished.count(), 1);
}

void tst_QWebPage::actionStates()
{
    QWebPage* page = m_view->page();

    page->mainFrame()->load(QUrl("qrc:///resources/script.html"));

    QAction* reloadAction = page->action(QWebPage::Reload);
    QAction* stopAction = page->action(QWebPage::Stop);

    QTRY_VERIFY(reloadAction->isEnabled());
    QTRY_VERIFY(!stopAction->isEnabled());
}

class ConsolePage : public QWebPage
{
public:
    ConsolePage(QObject* parent = 0) : QWebPage(parent) {}

    virtual void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
    {
        messages.append(message);
        lineNumbers.append(lineNumber);
        sourceIDs.append(sourceID);
    }

    QStringList messages;
    QList<int> lineNumbers;
    QStringList sourceIDs;
};

void tst_QWebPage::consoleOutput()
{
    ConsolePage page;
    page.mainFrame()->evaluateJavaScript("this is not valid JavaScript");
    QCOMPARE(page.messages.count(), 1);
    QCOMPARE(page.lineNumbers.at(0), 1);
}

class TestPage : public QWebPage {
    Q_OBJECT
public:
    TestPage(QObject* parent = 0) : QWebPage(parent)
    {
        connect(this, SIGNAL(geometryChangeRequested(QRect)), this, SLOT(slotGeometryChangeRequested(QRect)));
    }

    struct Navigation {
        QPointer<QWebFrame> frame;
        QNetworkRequest request;
        NavigationType type;
    };

    QList<Navigation> navigations;
    QList<TestPage*> createdWindows;
    QRect requestedGeometry;

    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest &request, NavigationType type) {
        Navigation n;
        n.frame = frame;
        n.request = request;
        n.type = type;
        navigations.append(n);
        return true;
    }

    virtual QWebPage* createWindow(WebWindowType) {
        TestPage* page = new TestPage(this);
        createdWindows.append(page);
        return page;
    }

private Q_SLOTS:
    void slotGeometryChangeRequested(const QRect& geom) {
        requestedGeometry = geom;
    }
};

void tst_QWebPage::popupFormSubmission()
{
    TestPage page;
    page.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    page.mainFrame()->setHtml("<form name=form1 method=get action='' target=myNewWin>"\
                                "<input type=hidden name=foo value='bar'>"\
                                "</form>");
    page.mainFrame()->evaluateJavaScript("window.open('', 'myNewWin', 'width=500,height=300,toolbar=0')");
    page.mainFrame()->evaluateJavaScript("document.form1.submit();");

    QTest::qWait(500);
    // The number of popup created should be one.
    QVERIFY(page.createdWindows.size() == 1);

    QString url = page.createdWindows.takeFirst()->mainFrame()->url().toString();
    // Check if the form submission was OK.
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=118597", Continue);
    QVERIFY(url.contains("?foo=bar"));
}

void tst_QWebPage::acceptNavigationRequestWithNewWindow()
{
    TestPage* page = new TestPage(m_view);
    page->settings()->setAttribute(QWebSettings::LinksIncludedInFocusChain, true);
    m_page = page;
    m_view->setPage(m_page);

    m_view->setUrl(QString("data:text/html,<a href=\"data:text/html,Reached\" target=\"_blank\">Click me</a>"));
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QFocusEvent fe(QEvent::FocusIn);
    m_page->event(&fe);

    QVERIFY(m_page->focusNextPrevChild(/*next*/ true));

    QKeyEvent keyEnter(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    m_page->event(&keyEnter);

    QCOMPARE(page->navigations.count(), 2);

    TestPage::Navigation n = page->navigations.at(1);
    QVERIFY(n.frame.isNull());
    QCOMPARE(n.request.url().toString(), QString("data:text/html,Reached"));
    QVERIFY(n.type == QWebPage::NavigationTypeLinkClicked);

    QCOMPARE(page->createdWindows.count(), 1);
}

class TestNetworkManager : public QNetworkAccessManager
{
public:
    TestNetworkManager(QObject* parent) : QNetworkAccessManager(parent) {}

    QList<QUrl> requestedUrls;
    QList<QNetworkRequest> requests;

protected:
    virtual QNetworkReply* createRequest(Operation op, const QNetworkRequest &request, QIODevice* outgoingData) {
        requests.append(request);
        requestedUrls.append(request.url());
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
};

void tst_QWebPage::userStyleSheet()
{
    TestNetworkManager* networkManager = new TestNetworkManager(m_page);
    m_page->setNetworkAccessManager(networkManager);

    m_page->settings()->setUserStyleSheetUrl(QUrl("data:text/css;charset=utf-8;base64,"
            + QByteArray("p { background-image: url('http://does.not/exist.png');}").toBase64()));
    m_view->setHtml("<p>hello world</p>");
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QVERIFY(networkManager->requestedUrls.count() >= 1);
    QCOMPARE(networkManager->requestedUrls.at(0), QUrl("http://does.not/exist.png"));
}

void tst_QWebPage::userStyleSheetFromLocalFileUrl()
{
    TestNetworkManager* networkManager = new TestNetworkManager(m_page);
    m_page->setNetworkAccessManager(networkManager);

    QUrl styleSheetUrl = QUrl::fromLocalFile(TESTS_SOURCE_DIR + QLatin1String("qwebpage/resources/user.css"));
    m_page->settings()->setUserStyleSheetUrl(styleSheetUrl);
    m_view->setHtml("<p>hello world</p>");
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QVERIFY(networkManager->requestedUrls.count() >= 1);
    QCOMPARE(networkManager->requestedUrls.at(0), QUrl("http://does.not/exist.png"));
}

void tst_QWebPage::userStyleSheetFromQrcUrl()
{
    TestNetworkManager* networkManager = new TestNetworkManager(m_page);
    m_page->setNetworkAccessManager(networkManager);

    m_page->settings()->setUserStyleSheetUrl(QUrl("qrc:///resources/user.css"));
    m_view->setHtml("<p>hello world</p>");
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QVERIFY(networkManager->requestedUrls.count() >= 1);
    QCOMPARE(networkManager->requestedUrls.at(0), QUrl("http://does.not/exist.png"));
}

void tst_QWebPage::loadHtml5Video()
{
#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
    QByteArray url("http://does.not/exist?a=1%2Cb=2");
    m_view->setHtml("<p><video id ='video' src='" + url + "' autoplay/></p>");
    QTest::qWait(2000);
    QUrl mUrl = DumpRenderTreeSupportQt::mediaContentUrlByElementId(m_page->mainFrame()->handle(), "video");
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=65452", Continue);
    QCOMPARE(mUrl.toEncoded(), url);
#else
    W_QSKIP("This test requires Qt Multimedia", SkipAll);
#endif
}

void tst_QWebPage::viewModes()
{
    m_view->setHtml("<body></body>");
    m_page->setProperty("_q_viewMode", "minimized");

    QVariant empty = m_page->mainFrame()->evaluateJavaScript("window.styleMedia.matchMedium(\"(-webkit-view-mode)\")");
    QVERIFY(empty.type() == QVariant::Bool && empty.toBool());

    QVariant minimized = m_page->mainFrame()->evaluateJavaScript("window.styleMedia.matchMedium(\"(-webkit-view-mode: minimized)\")");
    QVERIFY(minimized.type() == QVariant::Bool && minimized.toBool());

    QVariant maximized = m_page->mainFrame()->evaluateJavaScript("window.styleMedia.matchMedium(\"(-webkit-view-mode: maximized)\")");
    QVERIFY(maximized.type() == QVariant::Bool && !maximized.toBool());
}

void tst_QWebPage::modified()
{
    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body>blub"));
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body id=foo contenteditable>blah"));
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QVERIFY(!m_page->isModified());

//    m_page->mainFrame()->evaluateJavaScript("alert(document.getElementById('foo'))");
    m_page->mainFrame()->evaluateJavaScript("document.getElementById('foo').focus()");
    m_page->mainFrame()->evaluateJavaScript("document.execCommand('InsertText', true, 'Test');");

    QVERIFY(m_page->isModified());

    m_page->mainFrame()->evaluateJavaScript("document.execCommand('Undo', true);");

    QVERIFY(!m_page->isModified());

    m_page->mainFrame()->evaluateJavaScript("document.execCommand('Redo', true);");

    QVERIFY(m_page->isModified());

    QVERIFY(m_page->history()->canGoBack());
    QVERIFY(!m_page->history()->canGoForward());
    QCOMPARE(m_page->history()->count(), 2);
    QVERIFY(m_page->history()->backItem().isValid());
    QVERIFY(!m_page->history()->forwardItem().isValid());

    m_page->history()->back();
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QVERIFY(!m_page->history()->canGoBack());
    QVERIFY(m_page->history()->canGoForward());

    QVERIFY(!m_page->isModified());

    QVERIFY(m_page->history()->currentItemIndex() == 0);

    m_page->history()->setMaximumItemCount(3);
    QVERIFY(m_page->history()->maximumItemCount() == 3);

    QVariant variant("string test");
    m_page->history()->currentItem().setUserData(variant);
    QVERIFY(m_page->history()->currentItem().userData().toString() == "string test");

    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body>This is second page"));
    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body>This is third page"));
    QVERIFY(m_page->history()->count() == 2);
    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body>This is fourth page"));
    QVERIFY(m_page->history()->count() == 2);
    m_page->mainFrame()->setUrl(QUrl("data:text/html,<body>This is fifth page"));
    QVERIFY(::waitForSignal(m_page, SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*))));
}

// https://bugs.webkit.org/show_bug.cgi?id=51331
void tst_QWebPage::updatePositionDependentActionsCrash()
{
    QWebView view;
    view.setHtml("<p>test");
    QPoint pos(0, 0);
    view.page()->updatePositionDependentActions(pos);
    QMenu* contextMenu = 0;
    foreach (QObject* child, view.children()) {
        contextMenu = qobject_cast<QMenu*>(child);
        if (contextMenu)
            break;
    }
    QVERIFY(!contextMenu);
}

// https://bugs.webkit.org/show_bug.cgi?id=20357
void tst_QWebPage::contextMenuCrash()
{
    QWebView view;
    view.setHtml("<p>test");
    QPoint pos(0, 0);
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos);
    view.page()->swallowContextMenuEvent(&event);
    view.page()->updatePositionDependentActions(pos);
    QMenu* contextMenu = 0;
    foreach (QObject* child, view.children()) {
        contextMenu = qobject_cast<QMenu*>(child);
        if (contextMenu)
            break;
    }
    QVERIFY(contextMenu);
    delete contextMenu;
}

void tst_QWebPage::database()
{
    QString path = tmpDirPath();
    m_page->settings()->setOfflineStoragePath(path);
    QVERIFY(m_page->settings()->offlineStoragePath() == path);

    QWebSettings::setOfflineStorageDefaultQuota(1024 * 1024);
    QVERIFY(QWebSettings::offlineStorageDefaultQuota() == 1024 * 1024);

    m_page->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    m_page->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);

    QString dbFileName = path + "Databases.db";

    if (QFile::exists(dbFileName))
        QFile::remove(dbFileName);

    qRegisterMetaType<QWebFrame*>("QWebFrame*");
    QSignalSpy spy(m_page, SIGNAL(databaseQuotaExceeded(QWebFrame*,QString)));
    m_view->setHtml(QString("<html><head><script>var db; db=openDatabase('testdb', '1.0', 'test database API', 50000); </script></head><body><div></div></body></html>"), QUrl("http://www.myexample.com"));
    QTRY_COMPARE(spy.count(), 1);
    m_page->mainFrame()->evaluateJavaScript("var db2; db2=openDatabase('testdb', '1.0', 'test database API', 50000);");
    QTRY_COMPARE(spy.count(),1);

    m_page->mainFrame()->evaluateJavaScript("localStorage.test='This is a test for local storage';");
    m_view->setHtml(QString("<html><body id='b'>text</body></html>"), QUrl("http://www.myexample.com"));

    QVariant s1 = m_page->mainFrame()->evaluateJavaScript("localStorage.test");
    QCOMPARE(s1.toString(), QString("This is a test for local storage"));

    m_page->mainFrame()->evaluateJavaScript("sessionStorage.test='This is a test for session storage';");
    m_view->setHtml(QString("<html><body id='b'>text</body></html>"), QUrl("http://www.myexample.com"));
    QVariant s2 = m_page->mainFrame()->evaluateJavaScript("sessionStorage.test");
    QCOMPARE(s2.toString(), QString("This is a test for session storage"));

    m_view->setHtml(QString("<html><head></head><body><div></div></body></html>"), QUrl("http://www.myexample.com"));
    m_page->mainFrame()->evaluateJavaScript("var db3; db3=openDatabase('testdb', '1.0', 'test database API', 50000);db3.transaction(function(tx) { tx.executeSql('CREATE TABLE IF NOT EXISTS Test (text TEXT)', []); }, function(tx, result) { }, function(tx, error) { });");
    QTest::qWait(200);

    // Remove all databases.
    QWebSecurityOrigin origin = m_page->mainFrame()->securityOrigin();
    QList<QWebDatabase> dbs = origin.databases();
    for (int i = 0; i < dbs.count(); i++) {
        QString fileName = dbs[i].fileName();
        QVERIFY(QFile::exists(fileName));
        QWebDatabase::removeDatabase(dbs[i]);
        QVERIFY(!QFile::exists(fileName));
    }
    QVERIFY(!origin.databases().size());
    // Remove removed test :-)
    QWebDatabase::removeAllDatabases();
    QVERIFY(!origin.databases().size());
}

class PluginPage : public QWebPage
{
public:
    PluginPage(QObject *parent = 0)
        : QWebPage(parent) {}

    struct CallInfo
    {
        CallInfo(const QString &c, const QUrl &u,
                 const QStringList &pn, const QStringList &pv,
                 QObject *r)
            : classid(c), url(u), paramNames(pn),
              paramValues(pv), returnValue(r)
            {}
        QString classid;
        QUrl url;
        QStringList paramNames;
        QStringList paramValues;
        QObject *returnValue;
    };

    QList<CallInfo> calls;

protected:
    virtual QObject *createPlugin(const QString &classid, const QUrl &url,
                                  const QStringList &paramNames,
                                  const QStringList &paramValues)
    {
        QObject *result = 0;
        if (classid == "pushbutton")
            result = new QPushButton();
#ifndef QT_NO_INPUTDIALOG
        else if (classid == "lineedit")
            result = new QLineEdit();
#endif
        else if (classid == "graphicswidget")
            result = new QGraphicsWidget();
        if (result)
            result->setObjectName(classid);
        calls.append(CallInfo(classid, url, paramNames, paramValues, result));
        return result;
    }
};

static void createPlugin(QWebView *view)
{
    QSignalSpy loadSpy(view, SIGNAL(loadFinished(bool)));

    PluginPage* newPage = new PluginPage(view);
    view->setPage(newPage);

    // type has to be application/x-qt-plugin
    view->setHtml(QString("<html><body><object type='application/x-foobarbaz' classid='pushbutton' id='mybutton'/></body></html>"));
    QTRY_COMPARE(loadSpy.count(), 1);
    QCOMPARE(newPage->calls.count(), 0);

    view->setHtml(QString("<html><body><object type='application/x-qt-plugin' classid='pushbutton' id='mybutton'/></body></html>"));
    QTRY_COMPARE(loadSpy.count(), 2);
    QCOMPARE(newPage->calls.count(), 1);
    {
        PluginPage::CallInfo ci = newPage->calls.takeFirst();
        QCOMPARE(ci.classid, QString::fromLatin1("pushbutton"));
        QCOMPARE(ci.url, QUrl());
        QCOMPARE(ci.paramNames.count(), 3);
        QCOMPARE(ci.paramValues.count(), 3);
        QCOMPARE(ci.paramNames.at(0), QString::fromLatin1("type"));
        QCOMPARE(ci.paramValues.at(0), QString::fromLatin1("application/x-qt-plugin"));
        QCOMPARE(ci.paramNames.at(1), QString::fromLatin1("classid"));
        QCOMPARE(ci.paramValues.at(1), QString::fromLatin1("pushbutton"));
        QCOMPARE(ci.paramNames.at(2), QString::fromLatin1("id"));
        QCOMPARE(ci.paramValues.at(2), QString::fromLatin1("mybutton"));
        QVERIFY(ci.returnValue != 0);
        QVERIFY(ci.returnValue->inherits("QPushButton"));
    }
    // test JS bindings
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("document.getElementById('mybutton').toString()").toString(),
             QString::fromLatin1("[object HTMLObjectElement]"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mybutton.toString()").toString(),
             QString::fromLatin1("[object HTMLObjectElement]"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("typeof mybutton.objectName").toString(),
             QString::fromLatin1("string"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mybutton.objectName").toString(),
             QString::fromLatin1("pushbutton"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("typeof mybutton.clicked").toString(),
             QString::fromLatin1("function"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mybutton.clicked.toString()").toString(),
             QString::fromLatin1("function clicked() {\n    [native code]\n}"));

    view->setHtml(QString("<html><body><table>"
                            "<tr><object type='application/x-qt-plugin' classid='lineedit' id='myedit'/></tr>"
                            "<tr><object type='application/x-qt-plugin' classid='pushbutton' id='mybutton'/></tr>"
                            "</table></body></html>"), QUrl("http://foo.bar.baz"));
    QTRY_COMPARE(loadSpy.count(), 3);
    QCOMPARE(newPage->calls.count(), 2);
    {
        PluginPage::CallInfo ci = newPage->calls.takeFirst();
        QCOMPARE(ci.classid, QString::fromLatin1("lineedit"));
        QCOMPARE(ci.url, QUrl());
        QCOMPARE(ci.paramNames.count(), 3);
        QCOMPARE(ci.paramValues.count(), 3);
        QCOMPARE(ci.paramNames.at(0), QString::fromLatin1("type"));
        QCOMPARE(ci.paramValues.at(0), QString::fromLatin1("application/x-qt-plugin"));
        QCOMPARE(ci.paramNames.at(1), QString::fromLatin1("classid"));
        QCOMPARE(ci.paramValues.at(1), QString::fromLatin1("lineedit"));
        QCOMPARE(ci.paramNames.at(2), QString::fromLatin1("id"));
        QCOMPARE(ci.paramValues.at(2), QString::fromLatin1("myedit"));
        QVERIFY(ci.returnValue != 0);
        QVERIFY(ci.returnValue->inherits("QLineEdit"));
    }
    {
        PluginPage::CallInfo ci = newPage->calls.takeFirst();
        QCOMPARE(ci.classid, QString::fromLatin1("pushbutton"));
        QCOMPARE(ci.url, QUrl());
        QCOMPARE(ci.paramNames.count(), 3);
        QCOMPARE(ci.paramValues.count(), 3);
        QCOMPARE(ci.paramNames.at(0), QString::fromLatin1("type"));
        QCOMPARE(ci.paramValues.at(0), QString::fromLatin1("application/x-qt-plugin"));
        QCOMPARE(ci.paramNames.at(1), QString::fromLatin1("classid"));
        QCOMPARE(ci.paramValues.at(1), QString::fromLatin1("pushbutton"));
        QCOMPARE(ci.paramNames.at(2), QString::fromLatin1("id"));
        QCOMPARE(ci.paramValues.at(2), QString::fromLatin1("mybutton"));
        QVERIFY(ci.returnValue != 0);
        QVERIFY(ci.returnValue->inherits("QPushButton"));
    }
}

void tst_QWebPage::graphicsWidgetPlugin()
{
    m_view->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QGraphicsWebView webView;

    QSignalSpy loadSpy(&webView, SIGNAL(loadFinished(bool)));

    PluginPage* newPage = new PluginPage(&webView);
    webView.setPage(newPage);

    // type has to be application/x-qt-plugin
    webView.setHtml(QString("<html><body><object type='application/x-foobarbaz' classid='graphicswidget' id='mygraphicswidget'/></body></html>"));
    QTRY_COMPARE(loadSpy.count(), 1);
    QCOMPARE(newPage->calls.count(), 0);

    webView.setHtml(QString("<html><body><object type='application/x-qt-plugin' classid='graphicswidget' id='mygraphicswidget'/></body></html>"));
    QTRY_COMPARE(loadSpy.count(), 2);
    QCOMPARE(newPage->calls.count(), 1);
    {
        PluginPage::CallInfo ci = newPage->calls.takeFirst();
        QCOMPARE(ci.classid, QString::fromLatin1("graphicswidget"));
        QCOMPARE(ci.url, QUrl());
        QCOMPARE(ci.paramNames.count(), 3);
        QCOMPARE(ci.paramValues.count(), 3);
        QCOMPARE(ci.paramNames.at(0), QString::fromLatin1("type"));
        QCOMPARE(ci.paramValues.at(0), QString::fromLatin1("application/x-qt-plugin"));
        QCOMPARE(ci.paramNames.at(1), QString::fromLatin1("classid"));
        QCOMPARE(ci.paramValues.at(1), QString::fromLatin1("graphicswidget"));
        QCOMPARE(ci.paramNames.at(2), QString::fromLatin1("id"));
        QCOMPARE(ci.paramValues.at(2), QString::fromLatin1("mygraphicswidget"));
        QVERIFY(ci.returnValue);
        QVERIFY(ci.returnValue->inherits("QGraphicsWidget"));
    }
    // test JS bindings
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("document.getElementById('mygraphicswidget').toString()").toString(),
             QString::fromLatin1("[object HTMLObjectElement]"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mygraphicswidget.toString()").toString(),
             QString::fromLatin1("[object HTMLObjectElement]"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("typeof mygraphicswidget.objectName").toString(),
             QString::fromLatin1("string"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mygraphicswidget.objectName").toString(),
             QString::fromLatin1("graphicswidget"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("typeof mygraphicswidget.geometryChanged").toString(),
             QString::fromLatin1("function"));
    QCOMPARE(newPage->mainFrame()->evaluateJavaScript("mygraphicswidget.geometryChanged.toString()").toString(),
             QString::fromLatin1("function geometryChanged() {\n    [native code]\n}"));
}

void tst_QWebPage::createPluginWithPluginsEnabled()
{
    m_view->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    createPlugin(m_view);
}

void tst_QWebPage::createPluginWithPluginsDisabled()
{
    // Qt Plugins should be loaded by QtWebKit even when PluginsEnabled is
    // false. The client decides whether a Qt plugin is enabled or not when
    // it decides whether or not to instantiate it.
    m_view->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
    createPlugin(m_view);
}

// Standard base class for template PluginTracerPage. In tests it is used as interface.
class PluginCounterPage : public QWebPage {
public:
    int m_count;
    QPointer<QObject> m_widget;
    QObject* m_pluginParent;
    PluginCounterPage(QObject* parent = 0)
        : QWebPage(parent)
        , m_count(0)
        , m_pluginParent(0)
    {
       settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    }
    ~PluginCounterPage()
    {
        if (m_pluginParent)
            m_pluginParent->deleteLater();
    }
};

template<class T>
class PluginTracerPage : public PluginCounterPage {
public:
    PluginTracerPage(QObject* parent = 0)
        : PluginCounterPage(parent)
    {
        // this is a dummy parent object for the created plugin
        m_pluginParent = new T;
    }
    virtual QObject* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&)
    {
        m_count++;
        m_widget = new T;
        // need a cast to the specific type, as QObject::setParent cannot be called,
        // because it is not virtual. Instead it is necesary to call QWidget::setParent,
        // which also takes a QWidget* instead of a QObject*. Therefore we need to
        // upcast to T*, which is a QWidget.
        static_cast<T*>(m_widget.data())->setParent(static_cast<T*>(m_pluginParent));
        return m_widget.data();
    }
};

class PluginFactory {
public:
    enum FactoredType {QWidgetType, QGraphicsWidgetType};
    static PluginCounterPage* create(FactoredType type, QObject* parent = 0)
    {
        PluginCounterPage* result = 0;
        switch (type) {
        case QWidgetType:
            result = new PluginTracerPage<QWidget>(parent);
            break;
        case QGraphicsWidgetType:
            result = new PluginTracerPage<QGraphicsWidget>(parent);
            break;
        default: {/*Oops*/};
        }
        return result;
    }

    static void prepareTestData()
    {
        QTest::addColumn<int>("type");
        QTest::newRow("QWidget") << (int)PluginFactory::QWidgetType;
        QTest::newRow("QGraphicsWidget") << (int)PluginFactory::QGraphicsWidgetType;
    }
};

void tst_QWebPage::destroyPlugin_data()
{
    PluginFactory::prepareTestData();
}

void tst_QWebPage::destroyPlugin()
{
    QFETCH(int, type);
    PluginCounterPage* page = PluginFactory::create((PluginFactory::FactoredType)type, m_view);
    m_view->setPage(page);

    // we create the plugin, so the widget should be constructed
    QString content("<html><body><object type=\"application/x-qt-plugin\" classid=\"QProgressBar\"></object></body></html>");
    m_view->setHtml(content);
    QVERIFY(page->m_widget);
    QCOMPARE(page->m_count, 1);

    // navigate away, the plugin widget should be destructed
    m_view->setHtml("<html><body>Hi</body></html>");
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!page->m_widget);
}

void tst_QWebPage::createViewlessPlugin_data()
{
    PluginFactory::prepareTestData();
}

void tst_QWebPage::createViewlessPlugin()
{
    QFETCH(int, type);
    PluginCounterPage* page = PluginFactory::create((PluginFactory::FactoredType)type);
    QString content("<html><body><object type=\"application/x-qt-plugin\" classid=\"QProgressBar\"></object></body></html>");
    page->mainFrame()->setHtml(content);
    QCOMPARE(page->m_count, 1);
    QVERIFY(page->m_widget);
    QVERIFY(page->m_pluginParent);
    QVERIFY(page->m_widget.data()->parent() == page->m_pluginParent);
    delete page;

}

void tst_QWebPage::multiplePageGroupsAndLocalStorage()
{
    QDir dir(tmpDirPath());
    dir.mkdir("path1");
    dir.mkdir("path2");

    QWebView view1;
    QWebView view2;

    view1.page()->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    view1.page()->settings()->setLocalStoragePath(QDir::toNativeSeparators(tmpDirPath() + "/path1"));
    DumpRenderTreeSupportQt::webPageSetGroupName(view1.page()->handle(), "group1");
    view2.page()->settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);    
    view2.page()->settings()->setLocalStoragePath(QDir::toNativeSeparators(tmpDirPath() + "/path2"));
    DumpRenderTreeSupportQt::webPageSetGroupName(view2.page()->handle(), "group2");
    QCOMPARE(DumpRenderTreeSupportQt::webPageGroupName(view1.page()->handle()), QString("group1"));
    QCOMPARE(DumpRenderTreeSupportQt::webPageGroupName(view2.page()->handle()), QString("group2"));


    view1.setHtml(QString("<html><body> </body></html>"), QUrl("http://www.myexample.com"));
    view2.setHtml(QString("<html><body> </body></html>"), QUrl("http://www.myexample.com"));

    view1.page()->mainFrame()->evaluateJavaScript("localStorage.test='value1';");
    view2.page()->mainFrame()->evaluateJavaScript("localStorage.test='value2';");

    view1.setHtml(QString("<html><body> </body></html>"), QUrl("http://www.myexample.com"));
    view2.setHtml(QString("<html><body> </body></html>"), QUrl("http://www.myexample.com"));

    QVariant s1 = view1.page()->mainFrame()->evaluateJavaScript("localStorage.test");
    QCOMPARE(s1.toString(), QString("value1"));

    QVariant s2 = view2.page()->mainFrame()->evaluateJavaScript("localStorage.test");
    QCOMPARE(s2.toString(), QString("value2"));

    QTest::qWait(1000);

    QFile::remove(QDir::toNativeSeparators(tmpDirPath() + "/path1/http_www.myexample.com_0.localstorage"));
    QFile::remove(QDir::toNativeSeparators(tmpDirPath() + "/path2/http_www.myexample.com_0.localstorage"));
    dir.rmdir(QDir::toNativeSeparators("./path1"));
    dir.rmdir(QDir::toNativeSeparators("./path2"));
}

class CursorTrackedPage : public QWebPage
{
public:

    CursorTrackedPage(QWidget *parent = 0): QWebPage(parent) {
        setViewportSize(QSize(1024, 768)); // big space
    }

    QString selectedText() {
        return mainFrame()->evaluateJavaScript("window.getSelection().toString()").toString();
    }

    int selectionStartOffset() {
        return mainFrame()->evaluateJavaScript("window.getSelection().getRangeAt(0).startOffset").toInt();
    }

    int selectionEndOffset() {
        return mainFrame()->evaluateJavaScript("window.getSelection().getRangeAt(0).endOffset").toInt();
    }

    // true if start offset == end offset, i.e. no selected text
    int isSelectionCollapsed() {
        return mainFrame()->evaluateJavaScript("window.getSelection().getRangeAt(0).collapsed").toBool();
    }
};

void tst_QWebPage::cursorMovements()
{
    CursorTrackedPage* page = new CursorTrackedPage;
    QString content("<html><body><p id=one>The quick brown fox</p><p id=two>jumps over the lazy dog</p><p>May the source<br/>be with you!</p></body></html>");
    page->mainFrame()->setHtml(content);

    // this will select the first paragraph
    QString script = "var range = document.createRange(); " \
        "var node = document.getElementById(\"one\"); " \
        "range.selectNode(node); " \
        "getSelection().addRange(range);";
    page->mainFrame()->evaluateJavaScript(script);
    QCOMPARE(page->selectedText().trimmed(), QString::fromLatin1("The quick brown fox"));

    QRegExp regExp(" style=\".*\"");
    regExp.setMinimal(true);
    QCOMPARE(page->selectedHtml().trimmed().replace(regExp, ""), QString::fromLatin1("<p id=\"one\">The quick brown fox</p>"));

    // these actions must exist
    QVERIFY(page->action(QWebPage::MoveToNextChar) != 0);
    QVERIFY(page->action(QWebPage::MoveToPreviousChar) != 0);
    QVERIFY(page->action(QWebPage::MoveToNextWord) != 0);
    QVERIFY(page->action(QWebPage::MoveToPreviousWord) != 0);
    QVERIFY(page->action(QWebPage::MoveToNextLine) != 0);
    QVERIFY(page->action(QWebPage::MoveToPreviousLine) != 0);
    QVERIFY(page->action(QWebPage::MoveToStartOfLine) != 0);
    QVERIFY(page->action(QWebPage::MoveToEndOfLine) != 0);
    QVERIFY(page->action(QWebPage::MoveToStartOfBlock) != 0);
    QVERIFY(page->action(QWebPage::MoveToEndOfBlock) != 0);
    QVERIFY(page->action(QWebPage::MoveToStartOfDocument) != 0);
    QVERIFY(page->action(QWebPage::MoveToEndOfDocument) != 0);

    // right now they are disabled because contentEditable is false
    QCOMPARE(page->action(QWebPage::MoveToNextChar)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToPreviousChar)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToNextWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToPreviousWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToNextLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToPreviousLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToStartOfLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToEndOfLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToStartOfBlock)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToEndOfBlock)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToStartOfDocument)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::MoveToEndOfDocument)->isEnabled(), false);

    // make it editable before navigating the cursor
    page->setContentEditable(true);

    // here the actions are enabled after contentEditable is true
    QCOMPARE(page->action(QWebPage::MoveToNextChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToPreviousChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToNextWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToPreviousWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToNextLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToPreviousLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToStartOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToEndOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToStartOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToEndOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToStartOfDocument)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::MoveToEndOfDocument)->isEnabled(), true);

    // cursor will be before the word "jump"
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // cursor will be between 'j' and 'u' in the word "jump"
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 1);

    // cursor will be between 'u' and 'm' in the word "jump"
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 2);

    // cursor will be after the word "jump"
    page->triggerAction(QWebPage::MoveToNextWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 5);

    // cursor will be after the word "lazy"
    page->triggerAction(QWebPage::MoveToNextWord);
    page->triggerAction(QWebPage::MoveToNextWord);
    page->triggerAction(QWebPage::MoveToNextWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 19);

    // cursor will be between 'z' and 'y' in "lazy"
    page->triggerAction(QWebPage::MoveToPreviousChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 18);

    // cursor will be between 'a' and 'z' in "lazy"
    page->triggerAction(QWebPage::MoveToPreviousChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 17);

    // cursor will be before the word "lazy"
    page->triggerAction(QWebPage::MoveToPreviousWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 15);

    // cursor will be before the word "quick"
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 4);

    // cursor will be between 'p' and 's' in the word "jumps"
    page->triggerAction(QWebPage::MoveToNextWord);
    page->triggerAction(QWebPage::MoveToNextWord);
    page->triggerAction(QWebPage::MoveToNextWord);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 4);

    // cursor will be before the word "jumps"
    page->triggerAction(QWebPage::MoveToStartOfLine);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // cursor will be after the word "dog"
    page->triggerAction(QWebPage::MoveToEndOfLine);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 23);

    // cursor will be between 'w' and 'n' in "brown"
    page->triggerAction(QWebPage::MoveToStartOfLine);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 14);

    // cursor will be after the word "fox"
    page->triggerAction(QWebPage::MoveToEndOfLine);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 19);

    // cursor will be before the word "The"
    page->triggerAction(QWebPage::MoveToStartOfDocument);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // cursor will be after the word "you!"
    page->triggerAction(QWebPage::MoveToEndOfDocument);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 12);

    // cursor will be before the word "be"
    page->triggerAction(QWebPage::MoveToStartOfBlock);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // cursor will be after the word "you!"
    page->triggerAction(QWebPage::MoveToEndOfBlock);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 12);

    // try to move before the document start
    page->triggerAction(QWebPage::MoveToStartOfDocument);
    page->triggerAction(QWebPage::MoveToPreviousChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);
    page->triggerAction(QWebPage::MoveToStartOfDocument);
    page->triggerAction(QWebPage::MoveToPreviousWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // try to move past the document end
    page->triggerAction(QWebPage::MoveToEndOfDocument);
    page->triggerAction(QWebPage::MoveToNextChar);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 12);
    page->triggerAction(QWebPage::MoveToEndOfDocument);
    page->triggerAction(QWebPage::MoveToNextWord);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 12);

    delete page;
}

void tst_QWebPage::textSelection()
{
    CursorTrackedPage* page = new CursorTrackedPage;
    QString content("<html><body><p id=one>The quick brown fox</p>" \
        "<p id=two>jumps over the lazy dog</p>" \
        "<p>May the source<br/>be with you!</p></body></html>");
    page->mainFrame()->setHtml(content);

    // these actions must exist
    QVERIFY(page->action(QWebPage::SelectAll) != 0);
    QVERIFY(page->action(QWebPage::SelectNextChar) != 0);
    QVERIFY(page->action(QWebPage::SelectPreviousChar) != 0);
    QVERIFY(page->action(QWebPage::SelectNextWord) != 0);
    QVERIFY(page->action(QWebPage::SelectPreviousWord) != 0);
    QVERIFY(page->action(QWebPage::SelectNextLine) != 0);
    QVERIFY(page->action(QWebPage::SelectPreviousLine) != 0);
    QVERIFY(page->action(QWebPage::SelectStartOfLine) != 0);
    QVERIFY(page->action(QWebPage::SelectEndOfLine) != 0);
    QVERIFY(page->action(QWebPage::SelectStartOfBlock) != 0);
    QVERIFY(page->action(QWebPage::SelectEndOfBlock) != 0);
    QVERIFY(page->action(QWebPage::SelectStartOfDocument) != 0);
    QVERIFY(page->action(QWebPage::SelectEndOfDocument) != 0);

    // right now they are disabled because contentEditable is false and 
    // there isn't an existing selection to modify
    QCOMPARE(page->action(QWebPage::SelectNextChar)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectPreviousChar)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectNextWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectPreviousWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectNextLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectPreviousLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectStartOfLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectEndOfLine)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectStartOfBlock)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectEndOfBlock)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectStartOfDocument)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SelectEndOfDocument)->isEnabled(), false);

    // ..but SelectAll is awalys enabled
    QCOMPARE(page->action(QWebPage::SelectAll)->isEnabled(), true);

    // Verify hasSelection returns false since there is no selection yet...
    QCOMPARE(page->hasSelection(), false);

    // this will select the first paragraph
    QString selectScript = "var range = document.createRange(); " \
        "var node = document.getElementById(\"one\"); " \
        "range.selectNode(node); " \
        "getSelection().addRange(range);";
    page->mainFrame()->evaluateJavaScript(selectScript);
    QCOMPARE(page->selectedText().trimmed(), QString::fromLatin1("The quick brown fox"));
    QRegExp regExp(" style=\".*\"");
    regExp.setMinimal(true);
    QCOMPARE(page->selectedHtml().trimmed().replace(regExp, ""), QString::fromLatin1("<p id=\"one\">The quick brown fox</p>"));

    // Make sure hasSelection returns true, since there is selected text now...
    QCOMPARE(page->hasSelection(), true);

    // here the actions are enabled after a selection has been created
    QCOMPARE(page->action(QWebPage::SelectNextChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectNextWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectNextLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfDocument)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfDocument)->isEnabled(), true);

    // make it editable before navigating the cursor
    page->setContentEditable(true);

    // cursor will be before the word "The", this makes sure there is a charet
    page->triggerAction(QWebPage::MoveToStartOfDocument);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // here the actions are enabled after contentEditable is true
    QCOMPARE(page->action(QWebPage::SelectNextChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousChar)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectNextWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectNextLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectPreviousLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfLine)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfBlock)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectStartOfDocument)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SelectEndOfDocument)->isEnabled(), true);

    delete page;
}

void tst_QWebPage::textEditing()
{
    CursorTrackedPage* page = new CursorTrackedPage;
    QString content("<html><body><p id=one>The quick brown fox</p>" \
        "<p id=two>jumps over the lazy dog</p>" \
        "<p>May the source<br/>be with you!</p></body></html>");
    page->mainFrame()->setHtml(content);

    // these actions must exist
    QVERIFY(page->action(QWebPage::Cut) != 0);
    QVERIFY(page->action(QWebPage::Copy) != 0);
    QVERIFY(page->action(QWebPage::Paste) != 0);
    QVERIFY(page->action(QWebPage::DeleteStartOfWord) != 0);
    QVERIFY(page->action(QWebPage::DeleteEndOfWord) != 0);
    QVERIFY(page->action(QWebPage::SetTextDirectionDefault) != 0);
    QVERIFY(page->action(QWebPage::SetTextDirectionLeftToRight) != 0);
    QVERIFY(page->action(QWebPage::SetTextDirectionRightToLeft) != 0);
    QVERIFY(page->action(QWebPage::ToggleBold) != 0);
    QVERIFY(page->action(QWebPage::ToggleItalic) != 0);
    QVERIFY(page->action(QWebPage::ToggleUnderline) != 0);
    QVERIFY(page->action(QWebPage::InsertParagraphSeparator) != 0);
    QVERIFY(page->action(QWebPage::InsertLineSeparator) != 0);
    QVERIFY(page->action(QWebPage::PasteAndMatchStyle) != 0);
    QVERIFY(page->action(QWebPage::RemoveFormat) != 0);
    QVERIFY(page->action(QWebPage::ToggleStrikethrough) != 0);
    QVERIFY(page->action(QWebPage::ToggleSubscript) != 0);
    QVERIFY(page->action(QWebPage::ToggleSuperscript) != 0);
    QVERIFY(page->action(QWebPage::InsertUnorderedList) != 0);
    QVERIFY(page->action(QWebPage::InsertOrderedList) != 0);
    QVERIFY(page->action(QWebPage::Indent) != 0);
    QVERIFY(page->action(QWebPage::Outdent) != 0);
    QVERIFY(page->action(QWebPage::AlignCenter) != 0);
    QVERIFY(page->action(QWebPage::AlignJustified) != 0);
    QVERIFY(page->action(QWebPage::AlignLeft) != 0);
    QVERIFY(page->action(QWebPage::AlignRight) != 0);

    // right now they are disabled because contentEditable is false
    QCOMPARE(page->action(QWebPage::Cut)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::Paste)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::DeleteStartOfWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::DeleteEndOfWord)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SetTextDirectionDefault)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SetTextDirectionLeftToRight)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::SetTextDirectionRightToLeft)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleBold)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleItalic)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleUnderline)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::InsertParagraphSeparator)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::InsertLineSeparator)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::PasteAndMatchStyle)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::RemoveFormat)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleStrikethrough)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleSubscript)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::ToggleSuperscript)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::InsertUnorderedList)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::InsertOrderedList)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::Indent)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::Outdent)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::AlignCenter)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::AlignJustified)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::AlignLeft)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::AlignRight)->isEnabled(), false);

    // Select everything
    page->triggerAction(QWebPage::SelectAll);

    // make sure it is enabled since there is a selection
    QCOMPARE(page->action(QWebPage::Copy)->isEnabled(), true);

    // make it editable before navigating the cursor
    page->setContentEditable(true);

    // clear the selection
    page->triggerAction(QWebPage::MoveToStartOfDocument);
    QVERIFY(page->isSelectionCollapsed());
    QCOMPARE(page->selectionStartOffset(), 0);

    // make sure it is disabled since there isn't a selection
    QCOMPARE(page->action(QWebPage::Copy)->isEnabled(), false);

    // here the actions are enabled after contentEditable is true
    QCOMPARE(page->action(QWebPage::Paste)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::DeleteStartOfWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::DeleteEndOfWord)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SetTextDirectionDefault)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SetTextDirectionLeftToRight)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::SetTextDirectionRightToLeft)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleBold)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleItalic)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleUnderline)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::InsertParagraphSeparator)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::InsertLineSeparator)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::PasteAndMatchStyle)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleStrikethrough)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleSubscript)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::ToggleSuperscript)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::InsertUnorderedList)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::InsertOrderedList)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::Indent)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::Outdent)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::AlignCenter)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::AlignJustified)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::AlignLeft)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::AlignRight)->isEnabled(), true);
    
    // make sure these are disabled since there isn't a selection
    QCOMPARE(page->action(QWebPage::Cut)->isEnabled(), false);
    QCOMPARE(page->action(QWebPage::RemoveFormat)->isEnabled(), false);
    
    // make sure everything is selected
    page->triggerAction(QWebPage::SelectAll);
    
    // this is only true if there is an editable selection
    QCOMPARE(page->action(QWebPage::Cut)->isEnabled(), true);
    QCOMPARE(page->action(QWebPage::RemoveFormat)->isEnabled(), true);

    delete page;
}

void tst_QWebPage::requestCache()
{
    TestPage page;
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));

    page.mainFrame()->setUrl(QString("data:text/html,<a href=\"data:text/html,Reached\" target=\"_blank\">Click me</a>"));
    QTRY_COMPARE(loadSpy.count(), 1);
    QTRY_COMPARE(page.navigations.count(), 1);

    page.mainFrame()->setUrl(QString("data:text/html,<a href=\"data:text/html,Reached\" target=\"_blank\">Click me2</a>"));
    QTRY_COMPARE(loadSpy.count(), 2);
    QTRY_COMPARE(page.navigations.count(), 2);

    page.triggerAction(QWebPage::Stop);
    QVERIFY(page.history()->canGoBack());
    page.triggerAction(QWebPage::Back);

    QTRY_COMPARE(loadSpy.count(), 3);
    QTRY_COMPARE(page.navigations.count(), 3);
    QCOMPARE(page.navigations.at(0).request.attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt(),
             (int)QNetworkRequest::PreferNetwork);
    QCOMPARE(page.navigations.at(1).request.attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt(),
             (int)QNetworkRequest::PreferNetwork);
    QCOMPARE(page.navigations.at(2).request.attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt(),
             (int)QNetworkRequest::PreferCache);
}

void tst_QWebPage::loadCachedPage()
{
    TestPage page;
    QSignalSpy loadSpy(&page, SIGNAL(loadFinished(bool)));
    page.settings()->setMaximumPagesInCache(3);

    page.mainFrame()->load(QUrl("data:text/html,This is first page"));

    QTRY_COMPARE(loadSpy.count(), 1);
    QTRY_COMPARE(page.navigations.count(), 1);

    QUrl firstPageUrl = page.mainFrame()->url();
    page.mainFrame()->load(QUrl("data:text/html,This is second page"));

    QTRY_COMPARE(loadSpy.count(), 2);
    QTRY_COMPARE(page.navigations.count(), 2);

    page.triggerAction(QWebPage::Stop);
    QVERIFY(page.history()->canGoBack());

    QSignalSpy urlSpy(page.mainFrame(), SIGNAL(urlChanged(QUrl)));
    QVERIFY(urlSpy.isValid());

    page.triggerAction(QWebPage::Back);
    ::waitForSignal(page.mainFrame(), SIGNAL(urlChanged(QUrl)));
    QCOMPARE(urlSpy.size(), 1);

    QList<QVariant> arguments1 = urlSpy.takeFirst();
    QCOMPARE(arguments1.at(0).toUrl(), firstPageUrl);

}
void tst_QWebPage::backActionUpdate()
{
    QWebView view;
    QWebPage *page = view.page();
    QAction *action = page->action(QWebPage::Back);
    QVERIFY(!action->isEnabled());
    QSignalSpy loadSpy(page, SIGNAL(loadFinished(bool)));
    QUrl url = QUrl("qrc:///resources/framedindex.html");
    page->mainFrame()->load(url);
    QTRY_COMPARE(loadSpy.count(), 1);
    QVERIFY(!action->isEnabled());
    QTest::mouseClick(&view, Qt::LeftButton, 0, QPoint(10, 10));
    QTRY_COMPARE(loadSpy.count(), 2);

    QVERIFY(action->isEnabled());
}

void frameAtHelper(QWebPage* webPage, QWebFrame* webFrame, QPoint framePosition)
{
    if (!webFrame)
        return;

    framePosition += QPoint(webFrame->pos());
    QList<QWebFrame*> children = webFrame->childFrames();
    for (int i = 0; i < children.size(); ++i) {
        if (children.at(i)->childFrames().size() > 0)
            frameAtHelper(webPage, children.at(i), framePosition);

        QRect frameRect(children.at(i)->pos() + framePosition, children.at(i)->geometry().size());
        QVERIFY(children.at(i) == webPage->frameAt(frameRect.topLeft()));
    }
}

void tst_QWebPage::frameAt()
{
    QWebView webView;
    QWebPage* webPage = webView.page();
    QSignalSpy loadSpy(webPage, SIGNAL(loadFinished(bool)));
    QUrl url = QUrl("qrc:///resources/iframe.html");
    webPage->mainFrame()->load(url);
    QTRY_COMPARE(loadSpy.count(), 1);
    frameAtHelper(webPage, webPage->mainFrame(), webPage->mainFrame()->pos());
}

void tst_QWebPage::inputMethods_data()
{
    QTest::addColumn<QString>("viewType");
    QTest::newRow("QWebView") << "QWebView";
    QTest::newRow("QGraphicsWebView") << "QGraphicsWebView";
}

static Qt::InputMethodHints inputMethodHints(QObject* object)
{
    if (QGraphicsObject* o = qobject_cast<QGraphicsObject*>(object))
        return o->inputMethodHints();
    if (QWidget* w = qobject_cast<QWidget*>(object))
        return w->inputMethodHints();
    return Qt::InputMethodHints();
}

static bool inputMethodEnabled(QObject* object)
{
    if (QGraphicsObject* o = qobject_cast<QGraphicsObject*>(object))
        return o->flags() & QGraphicsItem::ItemAcceptsInputMethod;
    if (QWidget* w = qobject_cast<QWidget*>(object))
        return w->testAttribute(Qt::WA_InputMethodEnabled);
    return false;
}

static void clickOnPage(QWebPage* page, const QPoint& position)
{
    QMouseEvent evpres(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    page->event(&evpres);
    QMouseEvent evrel(QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    page->event(&evrel);
}

void tst_QWebPage::inputMethods()
{
    QFETCH(QString, viewType);
    QWebPage* page = new QWebPage;
    QObject* view = 0;
    QObject* container = 0;
    if (viewType == "QWebView") {
        QWebView* wv = new QWebView;
        wv->setPage(page);
        view = wv;
        container = view;
    } else if (viewType == "QGraphicsWebView") {
        QGraphicsWebView* wv = new QGraphicsWebView;
        wv->setPage(page);
        view = wv;

        QGraphicsView* gv = new QGraphicsView;
        QGraphicsScene* scene = new QGraphicsScene(gv);
        gv->setScene(scene);
        scene->addItem(wv);
        wv->setGeometry(QRect(0, 0, 500, 500));

        container = gv;
    } else
        QVERIFY2(false, "Unknown view type");

    page->settings()->setFontFamily(QWebSettings::SerifFont, page->settings()->fontFamily(QWebSettings::FixedFont));
    page->mainFrame()->setHtml("<html><body>" \
                                            "<input type='text' id='input1' style='font-family: serif' value='' maxlength='20'/><br>" \
                                            "<input type='password'/>" \
                                            "</body></html>");
    page->mainFrame()->setFocus();

    TestInputContext testContext;

    QWebElementCollection inputs = page->mainFrame()->documentElement().findAll("input");
    QPoint textInputCenter = inputs.at(0).geometry().center();

    clickOnPage(page, textInputCenter);

    // This part of the test checks if the SIP (Software Input Panel) is triggered,
    // which normally happens on mobile platforms, when a user input form receives
    // a mouse click.
    int  inputPanel = 0;
    if (viewType == "QWebView") {
        if (QWebView* wv = qobject_cast<QWebView*>(view))
            inputPanel = wv->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel);
    } else if (viewType == "QGraphicsWebView") {
        if (QGraphicsWebView* wv = qobject_cast<QGraphicsWebView*>(view))
            inputPanel = wv->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel);
    }

    // For non-mobile platforms RequestSoftwareInputPanel event is not called
    // because there is no SIP (Software Input Panel) triggered. In the case of a
    // mobile platform, an input panel, e.g. virtual keyboard, is usually invoked
    // and the RequestSoftwareInputPanel event is called. For these two situations
    // this part of the test can verified as the checks below.
    if (inputPanel)
        QVERIFY(testContext.isInputPanelVisible());
    else
        QVERIFY(!testContext.isInputPanelVisible());
    testContext.hideInputPanel();

    clickOnPage(page, textInputCenter);
    QVERIFY(testContext.isInputPanelVisible());

    //ImMicroFocus
    QVariant variant = page->inputMethodQuery(Qt::ImMicroFocus);
    QVERIFY(inputs.at(0).geometry().contains(variant.toRect().topLeft()));

    // We assigned the serif font famility to be the same as the fixef font family.
    // Then test ImFont on a serif styled element, we should get our fixef font family.
    variant = page->inputMethodQuery(Qt::ImFont);
    QFont font = variant.value<QFont>();
    QCOMPARE(page->settings()->fontFamily(QWebSettings::FixedFont), font.family());

    QList<QInputMethodEvent::Attribute> inputAttributes;

    //Insert text.
    {
        QInputMethodEvent eventText("QtWebKit", inputAttributes);
        QSignalSpy signalSpy(page, SIGNAL(microFocusChanged()));
        page->event(&eventText);
        QCOMPARE(signalSpy.count(), 0);
    }

    {
        QInputMethodEvent eventText("", inputAttributes);
        eventText.setCommitString(QString("QtWebKit"), 0, 0);
        page->event(&eventText);
    }

    //ImMaximumTextLength
    variant = page->inputMethodQuery(Qt::ImMaximumTextLength);
    QCOMPARE(20, variant.toInt());

    //Set selection
    inputAttributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 3, 2, QVariant());
    QInputMethodEvent eventSelection("",inputAttributes);
    page->event(&eventSelection);

    //ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    int anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 3);

    //ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    int cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 5);

    //ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    QString selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString("eb"));

    //Set selection with negative length
    inputAttributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 6, -5, QVariant());
    QInputMethodEvent eventSelection3("",inputAttributes);
    page->event(&eventSelection3);

    //ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 1);

    //ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 6);

    //ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString("tWebK"));

    //ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    QString value = variant.value<QString>();
    QCOMPARE(value, QString("QtWebKit"));

    {
        QList<QInputMethodEvent::Attribute> attributes;
        // Clear the selection, so the next test does not clear any contents.
        QInputMethodEvent::Attribute newSelection(QInputMethodEvent::Selection, 0, 0, QVariant());
        attributes.append(newSelection);
        QInputMethodEvent event("composition", attributes);
        page->event(&event);
    }

    // A ongoing composition should not change the surrounding text before it is committed.
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    value = variant.value<QString>();
    QCOMPARE(value, QString("QtWebKit"));

    // Cancel current composition first
    inputAttributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 0, 0, QVariant());
    QInputMethodEvent eventSelection4("", inputAttributes);
    page->event(&eventSelection4);

    // START - Tests for Selection when the Editor is NOT in Composition mode

    // LEFT to RIGHT selection
    // Deselect the selection by sending MouseButtonPress events
    // This moves the current cursor to the end of the text
    clickOnPage(page, textInputCenter);

    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event(QString(), attributes);
        event.setCommitString("XXX", 0, 0);
        page->event(&event);
        event.setCommitString(QString(), -2, 2); // Erase two characters.
        page->event(&event);
        event.setCommitString(QString(), -1, 1); // Erase one character.
        page->event(&event);
        variant = page->inputMethodQuery(Qt::ImSurroundingText);
        value = variant.value<QString>();
        QCOMPARE(value, QString("QtWebKit"));
    }

    //Move to the start of the line
    page->triggerAction(QWebPage::MoveToStartOfLine);

    QKeyEvent keyRightEventPress(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QKeyEvent keyRightEventRelease(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier);

    //Move 2 characters RIGHT
    for (int j = 0; j < 2; ++j) {
        page->event(&keyRightEventPress);
        page->event(&keyRightEventRelease);
    }

    //Select to the end of the line
    page->triggerAction(QWebPage::SelectEndOfLine);

    //ImAnchorPosition QtWebKit
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 2);

    //ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 8);

    //ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString("WebKit"));

    //RIGHT to LEFT selection
    //Deselect the selection (this moves the current cursor to the end of the text)
    clickOnPage(page, textInputCenter);

    //ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 8);

    //ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 8);

    //ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    QKeyEvent keyLeftEventPress(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    QKeyEvent keyLeftEventRelease(QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier);

    //Move 2 characters LEFT
    for (int i = 0; i < 2; ++i) {
        page->event(&keyLeftEventPress);
        page->event(&keyLeftEventRelease);
    }

    //Select to the start of the line
    page->triggerAction(QWebPage::SelectStartOfLine);

    //ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 6);

    //ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 0);

    //ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString("QtWebK"));

    //END - Tests for Selection when the Editor is not in Composition mode

    //ImhHiddenText
    QPoint passwordInputCenter = inputs.at(1).geometry().center();
    clickOnPage(page, passwordInputCenter);

    QVERIFY(inputMethodEnabled(view));
    QVERIFY(inputMethodHints(view) & Qt::ImhHiddenText);

    clickOnPage(page, textInputCenter);
    QVERIFY(!(inputMethodHints(view) & Qt::ImhHiddenText));

    page->mainFrame()->setHtml("<html><body><p>nothing to input here");
    testContext.hideInputPanel();

    QWebElement para = page->mainFrame()->findFirstElement("p");
    clickOnPage(page, para.geometry().center());

    QVERIFY(!testContext.isInputPanelVisible());

    //START - Test for sending empty QInputMethodEvent
    page->mainFrame()->setHtml("<html><body>" \
                                            "<input type='text' id='input3' value='QtWebKit2'/>" \
                                            "</body></html>");
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input3'); inputEle.focus(); inputEle.select();");

    //Send empty QInputMethodEvent
    QInputMethodEvent emptyEvent;
    page->event(&emptyEvent);

    QString inputValue = page->mainFrame()->evaluateJavaScript("document.getElementById('input3').value").toString();
    QCOMPARE(inputValue, QString("QtWebKit2"));
    //END - Test for sending empty QInputMethodEvent

    page->mainFrame()->setHtml("<html><body>" \
                                            "<input type='text' id='input4' value='QtWebKit inputMethod'/>" \
                                            "</body></html>");
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input4'); inputEle.focus(); inputEle.select();");

    // Clear the selection, also cancel the ongoing composition if there is one.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent::Attribute newSelection(QInputMethodEvent::Selection, 0, 0, QVariant());
        attributes.append(newSelection);
        QInputMethodEvent event("", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    QString surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("QtWebKit inputMethod"));

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 0);

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 0);

    // 1. Insert a character to the begining of the line.
    // Send temporary text, which makes the editor has composition 'm'.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("m", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("QtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 0);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 0);

    // Send temporary text, which makes the editor has composition 'n'.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("n", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("QtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 0);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 0);

    // Send commit text, which makes the editor conforms composition.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("", attributes);
        event.setCommitString("o");
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oQtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 1);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 1);

    // 2. insert a character to the middle of the line.
    // Send temporary text, which makes the editor has composition 'd'.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("d", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oQtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 1);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 1);

    // Send commit text, which makes the editor conforms composition.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("", attributes);
        event.setCommitString("e");
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 2);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 2);

    // 3. Insert a character to the end of the line.
    page->triggerAction(QWebPage::MoveToEndOfLine);
    
    // Send temporary text, which makes the editor has composition 't'.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("t", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit inputMethod"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 22);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 22);

    // Send commit text, which makes the editor conforms composition.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("", attributes);
        event.setCommitString("t");
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit inputMethodt"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 23);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 23);

    // 4. Replace the selection.
    page->triggerAction(QWebPage::SelectPreviousWord);

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString("inputMethodt"));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit inputMethodt"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 11);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 23);

    // Send temporary text, which makes the editor has composition 'w'.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("w", attributes);
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit "));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 11);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 11);

    // Send commit text, which makes the editor conforms composition.
    {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event("", attributes);
        event.setCommitString("2");
        page->event(&event);
    }

    // ImCurrentSelection
    variant = page->inputMethodQuery(Qt::ImCurrentSelection);
    selectionValue = variant.value<QString>();
    QCOMPARE(selectionValue, QString(""));

    // ImSurroundingText
    variant = page->inputMethodQuery(Qt::ImSurroundingText);
    surroundingValue = variant.value<QString>();
    QCOMPARE(surroundingValue, QString("oeQtWebKit 2"));

    // ImCursorPosition
    variant = page->inputMethodQuery(Qt::ImCursorPosition);
    cursorPosition =  variant.toInt();
    QCOMPARE(cursorPosition, 12);

    // ImAnchorPosition
    variant = page->inputMethodQuery(Qt::ImAnchorPosition);
    anchorPosition =  variant.toInt();
    QCOMPARE(anchorPosition, 12);

    // Check sending RequestSoftwareInputPanel event
    page->mainFrame()->setHtml("<html><body>" \
                                            "<input type='text' id='input5' value='QtWebKit inputMethod'/>" \
                                            "<div id='btnDiv' onclick='i=document.getElementById(&quot;input5&quot;); i.focus();'>abc</div>"\
                                            "</body></html>");
    QWebElement inputElement = page->mainFrame()->findFirstElement("div");
    clickOnPage(page, inputElement.geometry().center());

    QVERIFY(!testContext.isInputPanelVisible());

    // START - Newline test for textarea
    qApp->processEvents();
    page->mainFrame()->setHtml("<html><body>" \
                                            "<textarea rows='5' cols='1' id='input5' value=''/>" \
                                            "</body></html>");
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input5'); inputEle.focus(); inputEle.select();");
    
    // Enter Key without key text
    QKeyEvent keyEnter(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    page->event(&keyEnter);
    QList<QInputMethodEvent::Attribute> attribs;

    QInputMethodEvent eventText(QString(), attribs);
    eventText.setCommitString("\n");
    page->event(&eventText);

    QInputMethodEvent eventText2(QString(), attribs);
    eventText2.setCommitString("third line");
    page->event(&eventText2);
    qApp->processEvents();

    QString inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString("\n\nthird line"));

    // Enter Key with key text '\r'
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input5'); inputEle.value = ''; inputEle.focus(); inputEle.select();");
    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString(""));

    QKeyEvent keyEnterWithCarriageReturn(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier, "\r");
    page->event(&keyEnterWithCarriageReturn);
    page->event(&eventText);
    page->event(&eventText2);
    qApp->processEvents();

    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString("\n\nthird line"));

    // Enter Key with key text '\n'
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input5'); inputEle.value = ''; inputEle.focus(); inputEle.select();");
    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString(""));

    QKeyEvent keyEnterWithLineFeed(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier, "\n");
    page->event(&keyEnterWithLineFeed);
    page->event(&eventText);
    page->event(&eventText2);
    qApp->processEvents();

    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString("\n\nthird line"));

    // Enter Key with key text "\n\r"
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input5'); inputEle.value = ''; inputEle.focus(); inputEle.select();");
    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString(""));

    QKeyEvent keyEnterWithLFCR(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier, "\n\r");
    page->event(&keyEnterWithLFCR);
    page->event(&eventText);
    page->event(&eventText2);
    qApp->processEvents();

    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString("\n\nthird line"));

    // Return Key without key text
    page->mainFrame()->evaluateJavaScript("var inputEle = document.getElementById('input5'); inputEle.value = ''; inputEle.focus(); inputEle.select();");
    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString(""));

    QKeyEvent keyReturn(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    page->event(&keyReturn);
    page->event(&eventText);
    page->event(&eventText2);
    qApp->processEvents();

    inputValue2 = page->mainFrame()->evaluateJavaScript("document.getElementById('input5').value").toString();
    QCOMPARE(inputValue2, QString("\n\nthird line"));

    // END - Newline test for textarea

    delete container;
}

void tst_QWebPage::inputMethodsTextFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("length");

    QTest::newRow("") << QString("") << 0 << 0;
    QTest::newRow("Q") << QString("Q") << 0 << 1;
    QTest::newRow("Qt") << QString("Qt") << 0 << 1;
    QTest::newRow("Qt") << QString("Qt") << 0 << 2;
    QTest::newRow("Qt") << QString("Qt") << 1 << 1;
    QTest::newRow("Qt ") << QString("Qt ") << 0 << 1;
    QTest::newRow("Qt ") << QString("Qt ") << 1 << 1;
    QTest::newRow("Qt ") << QString("Qt ") << 2 << 1;
    QTest::newRow("Qt ") << QString("Qt ") << 2 << -1;
    QTest::newRow("Qt ") << QString("Qt ") << -2 << 3;
    QTest::newRow("Qt ") << QString("Qt ") << 0 << 3;
    QTest::newRow("Qt by") << QString("Qt by") << 0 << 1;
    QTest::newRow("Qt by Nokia") << QString("Qt by Nokia") << 0 << 1;
}


void tst_QWebPage::inputMethodsTextFormat()
{
    QWebPage* page = new QWebPage;
    QWebView* view = new QWebView;
    view->setPage(page);
    page->settings()->setFontFamily(QWebSettings::SerifFont, "FooSerifFont");
    page->mainFrame()->setHtml("<html><body>" \
                                            "<input type='text' id='input1' style='font-family: serif' value='' maxlength='20'/>");
    page->mainFrame()->evaluateJavaScript("document.getElementById('input1').focus()");
    page->mainFrame()->setFocus();
    view->show();

    QFETCH(QString, string);
    QFETCH(int, start);
    QFETCH(int, length);

    QList<QInputMethodEvent::Attribute> attrs;
    QTextCharFormat format;
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    format.setUnderlineColor(Qt::red);
    attrs.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, start, length, format));
    QInputMethodEvent im(string, attrs);
    page->event(&im);

    QTest::qWait(1000);

    delete view;
}

void tst_QWebPage::protectBindingsRuntimeObjectsFromCollector()
{
    QSignalSpy loadSpy(m_view, SIGNAL(loadFinished(bool)));

    PluginPage* newPage = new PluginPage(m_view);
    m_view->setPage(newPage);

    m_view->settings()->setAttribute(QWebSettings::PluginsEnabled, true);

    m_view->setHtml(QString("<html><body><object type='application/x-qt-plugin' classid='lineedit' id='mylineedit'/></body></html>"));
    QTRY_COMPARE(loadSpy.count(), 1);

    newPage->mainFrame()->evaluateJavaScript("function testme(text) { var lineedit = document.getElementById('mylineedit'); lineedit.setText(text); lineedit.selectAll(); }");

    newPage->mainFrame()->evaluateJavaScript("testme('foo')");

    DumpRenderTreeSupportQt::garbageCollectorCollect();

    // don't crash!
    newPage->mainFrame()->evaluateJavaScript("testme('bar')");
}

void tst_QWebPage::localURLSchemes()
{
    int i = QWebSecurityOrigin::localSchemes().size();

    QWebSecurityOrigin::removeLocalScheme("file");
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i);
    QWebSecurityOrigin::addLocalScheme("file");
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i);

    QWebSecurityOrigin::removeLocalScheme("qrc");
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i - 1);
    QWebSecurityOrigin::addLocalScheme("qrc");
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i);

    QString myscheme = "myscheme";
    QWebSecurityOrigin::addLocalScheme(myscheme);
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i + 1);
    QVERIFY(QWebSecurityOrigin::localSchemes().contains(myscheme));
    QWebSecurityOrigin::removeLocalScheme(myscheme);
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i);
    QWebSecurityOrigin::removeLocalScheme(myscheme);
    QTRY_COMPARE(QWebSecurityOrigin::localSchemes().size(), i);
}

static inline bool testFlag(QWebPage& webPage, QWebSettings::WebAttribute settingAttribute, const QString& jsObjectName, bool settingValue)
{
    webPage.settings()->setAttribute(settingAttribute, settingValue);
    return webPage.mainFrame()->evaluateJavaScript(QString("(window.%1 != undefined)").arg(jsObjectName)).toBool();
}

void tst_QWebPage::testOptionalJSObjects()
{
    // Once a feature is enabled and the JS object is accessed turning off the setting will not turn off
    // the visibility of the JS object any more. For this reason this test uses two QWebPage instances.
    // Part of the test is to make sure that the QWebPage instances do not interfere with each other so turning on
    // a feature for one instance will not turn it on for another.

    QWebPage webPage1;
    QWebPage webPage2;

    webPage1.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("http://www.example.com/"));
    webPage2.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("http://www.example.com/"));

    QEXPECT_FAIL("","Feature enabled/disabled checking problem. Look at bugs.webkit.org/show_bug.cgi?id=29867", Continue);
    QCOMPARE(testFlag(webPage1, QWebSettings::OfflineWebApplicationCacheEnabled, "applicationCache", false), false);
    QCOMPARE(testFlag(webPage2, QWebSettings::OfflineWebApplicationCacheEnabled, "applicationCache", true),  true);
    QEXPECT_FAIL("","Feature enabled/disabled checking problem. Look at bugs.webkit.org/show_bug.cgi?id=29867", Continue);
    QCOMPARE(testFlag(webPage1, QWebSettings::OfflineWebApplicationCacheEnabled, "applicationCache", false), false);
    QCOMPARE(testFlag(webPage2, QWebSettings::OfflineWebApplicationCacheEnabled, "applicationCache", false), true);

    QCOMPARE(testFlag(webPage1, QWebSettings::LocalStorageEnabled, "localStorage", false), false);
    QCOMPARE(testFlag(webPage2, QWebSettings::LocalStorageEnabled, "localStorage", true),  true);
    QCOMPARE(testFlag(webPage1, QWebSettings::LocalStorageEnabled, "localStorage", false), false);
    QCOMPARE(testFlag(webPage2, QWebSettings::LocalStorageEnabled, "localStorage", false), true);
}

static inline bool checkLocalStorageVisibility(QWebPage& webPage, bool localStorageEnabled)
{
    webPage.settings()->setAttribute(QWebSettings::LocalStorageEnabled, localStorageEnabled);
    return webPage.mainFrame()->evaluateJavaScript(QString("(window.localStorage != undefined)")).toBool();
}

void tst_QWebPage::testLocalStorageVisibility()
{
    // Local storage's visibility depends on its security origin, which depends on base url.
    // Initially, it will test it with base urls that get a globally unique origin, which may not
    // be able to use local storage even if the feature is enabled. Then later the same test is
    // done but with urls that would get a valid origin, so local storage could be used.
    // Before every test case it checks if local storage is not already visible.

    QWebPage webPage;

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl());

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("invalid"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("://misparsed.com"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("http://"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("about:blank"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("data:text/html,test"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), false);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("file:///"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), true);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("http://www.example.com"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), true);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("https://www.example.com"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), true);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("ftp://files.example.com"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), true);

    webPage.currentFrame()->setHtml(QString("<html><body>test</body></html>"), QUrl("file:///path/to/index.html"));

    QCOMPARE(checkLocalStorageVisibility(webPage, false), false);
    QCOMPARE(checkLocalStorageVisibility(webPage, true), true);
}

void tst_QWebPage::testEnablePersistentStorage()
{
    QWebPage webPage;

    // By default all persistent options should be disabled
    QCOMPARE(webPage.settings()->testAttribute(QWebSettings::LocalStorageEnabled), false);
    QCOMPARE(webPage.settings()->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled), false);
    QCOMPARE(webPage.settings()->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled), false);
    QVERIFY(webPage.settings()->iconDatabasePath().isEmpty());

    QWebSettings::enablePersistentStorage();


    QTRY_COMPARE(webPage.settings()->testAttribute(QWebSettings::LocalStorageEnabled), true);
    QTRY_COMPARE(webPage.settings()->testAttribute(QWebSettings::OfflineStorageDatabaseEnabled), true);
    QTRY_COMPARE(webPage.settings()->testAttribute(QWebSettings::OfflineWebApplicationCacheEnabled), true);

    QTRY_VERIFY(!webPage.settings()->offlineStoragePath().isEmpty());
    QTRY_VERIFY(!webPage.settings()->offlineWebApplicationCachePath().isEmpty());
    QTRY_VERIFY(!webPage.settings()->iconDatabasePath().isEmpty());
}

void tst_QWebPage::defaultTextEncoding()
{
    QWebFrame* mainFrame = m_page->mainFrame();

    QString defaultCharset = mainFrame->evaluateJavaScript("document.defaultCharset").toString();
    QVERIFY(!defaultCharset.isEmpty());
    QCOMPARE(QWebSettings::globalSettings()->defaultTextEncoding(), defaultCharset);

    m_page->settings()->setDefaultTextEncoding(QString("utf-8"));
    QString charset = mainFrame->evaluateJavaScript("document.defaultCharset").toString();
    QCOMPARE(charset, QString("utf-8"));
    QCOMPARE(m_page->settings()->defaultTextEncoding(), charset);

    m_page->settings()->setDefaultTextEncoding(QString());
    charset = mainFrame->evaluateJavaScript("document.defaultCharset").toString();
    QVERIFY(!charset.isEmpty());
    QCOMPARE(charset, defaultCharset);

    QWebSettings::globalSettings()->setDefaultTextEncoding(QString("utf-8"));
    charset = mainFrame->evaluateJavaScript("document.defaultCharset").toString();
    QCOMPARE(charset, QString("utf-8"));
    QCOMPARE(QWebSettings::globalSettings()->defaultTextEncoding(), charset);
}

class ErrorPage : public QWebPage
{
public:

    ErrorPage(QWidget* parent = 0): QWebPage(parent)
    {
    }

    virtual bool supportsExtension(Extension extension) const
    {
        return extension == ErrorPageExtension;
    }

    virtual bool extension(Extension, const ExtensionOption* option, ExtensionReturn* output)
    {
        ErrorPageExtensionReturn* errorPage = static_cast<ErrorPageExtensionReturn*>(output);

        errorPage->contentType = "text/html";
        errorPage->content = "error";
        return true;
    }
};

void tst_QWebPage::errorPageExtension()
{
    ErrorPage page;
    m_view->setPage(&page);

    QSignalSpy spyLoadFinished(m_view, SIGNAL(loadFinished(bool)));

    m_view->setUrl(QUrl("data:text/html,foo"));
    QTRY_COMPARE(spyLoadFinished.count(), 1);

    page.mainFrame()->setUrl(QUrl("http://non.existent/url"));
    QTRY_COMPARE(spyLoadFinished.count(), 2);
    QCOMPARE(page.mainFrame()->toPlainText(), QString("error"));
    QCOMPARE(page.history()->count(), 2);
    QCOMPARE(page.history()->currentItem().url(), QUrl("http://non.existent/url"));
    QCOMPARE(page.history()->canGoBack(), true);
    QCOMPARE(page.history()->canGoForward(), false);

    page.triggerAction(QWebPage::Back);
    QTRY_COMPARE(page.history()->canGoBack(), false);
    QTRY_COMPARE(page.history()->canGoForward(), true);

    page.triggerAction(QWebPage::Forward);
    QTRY_COMPARE(page.history()->canGoBack(), true);
    QTRY_COMPARE(page.history()->canGoForward(), false);

    page.triggerAction(QWebPage::Back);
    QTRY_COMPARE(page.history()->canGoBack(), false);
    QTRY_COMPARE(page.history()->canGoForward(), true);
    QTRY_COMPARE(page.history()->currentItem().url(), QUrl("data:text/html,foo"));

    m_view->setPage(0);
}

void tst_QWebPage::errorPageExtensionInIFrames()
{
    ErrorPage page;
    m_view->setPage(&page);

    m_view->page()->mainFrame()->load(QUrl(
        "data:text/html,"
        "<h1>h1</h1>"
        "<iframe src='data:text/html,<p/>p'></iframe>"
        "<iframe src='http://non.existent/url'></iframe>"));
    QSignalSpy spyLoadFinished(m_view, SIGNAL(loadFinished(bool)));
    QTRY_COMPARE(spyLoadFinished.count(), 1);

    QCOMPARE(page.mainFrame()->childFrames()[1]->toPlainText(), QString("error"));

    m_view->setPage(0);
}

void tst_QWebPage::errorPageExtensionInFrameset()
{
    ErrorPage page;
    m_view->setPage(&page);

    m_view->load(QUrl("qrc:///resources/index.html"));

    QSignalSpy spyLoadFinished(m_view, SIGNAL(loadFinished(bool)));
    QTRY_COMPARE(spyLoadFinished.count(), 1);
    QCOMPARE(page.mainFrame()->childFrames().count(), 2);
    QCOMPARE(page.mainFrame()->childFrames()[1]->toPlainText(), QString("error"));

    m_view->setPage(0);
}

void tst_QWebPage::errorPageExtensionLoadFinished()
{
    ErrorPage page;
    m_view->setPage(&page);

    QSignalSpy spyLoadFinished(m_view, SIGNAL(loadFinished(bool)));
    QSignalSpy spyFrameLoadFinished(m_view->page()->mainFrame(), SIGNAL(loadFinished(bool)));

    m_view->setUrl(QUrl("data:text/html,foo"));
    QTRY_COMPARE(spyLoadFinished.count(), 1);
    QTRY_COMPARE(spyFrameLoadFinished.count(), 1);

    const bool loadSucceded = spyLoadFinished.at(0).at(0).toBool();
    QVERIFY(loadSucceded);
    const bool frameLoadSucceded = spyFrameLoadFinished.at(0).at(0).toBool();
    QVERIFY(frameLoadSucceded);

    m_view->page()->mainFrame()->setUrl(QUrl("http://non.existent/url"));
    QTRY_COMPARE(spyLoadFinished.count(), 2);
    QTRY_COMPARE(spyFrameLoadFinished.count(), 2);

    const bool nonExistantLoadSucceded = spyLoadFinished.at(1).at(0).toBool();
    QVERIFY(nonExistantLoadSucceded);
    const bool nonExistantFrameLoadSucceded = spyFrameLoadFinished.at(1).at(0).toBool();
    QVERIFY(nonExistantFrameLoadSucceded);

    m_view->setPage(0);
}

class FriendlyWebPage : public QWebPage
{
public:
    friend class tst_QWebPage;
};

void tst_QWebPage::userAgentApplicationName()
{
    const QString oldApplicationName = QCoreApplication::applicationName();
    FriendlyWebPage page;

    const QString applicationNameMarker = QString::fromUtf8("StrangeName\342\210\236");
    QCoreApplication::setApplicationName(applicationNameMarker);
    QVERIFY(page.userAgentForUrl(QUrl()).contains(applicationNameMarker));

    QCoreApplication::setApplicationName(oldApplicationName);
}

class CustomUserAgentWebPage : public QWebPage
{
public:
    static const QLatin1String filteredUserAgent;
protected:
    virtual QString userAgentForUrl(const QUrl& url) const
    {
        return QString("My User Agent\nX-New-Http-Header: Oh Noes!");
    }
};
const QLatin1String CustomUserAgentWebPage::filteredUserAgent("My User AgentX-New-Http-Header: Oh Noes!");

void tst_QWebPage::userAgentNewlineStripping()
{
    CustomUserAgentWebPage page;
    QWebFrame* mainFrame = page.mainFrame();
    mainFrame->setHtml("<html><body></body></html>");
    QCOMPARE(mainFrame->evaluateJavaScript("navigator.userAgent").toString(), CustomUserAgentWebPage::filteredUserAgent);
}

void tst_QWebPage::crashTests_LazyInitializationOfMainFrame()
{
    {
        QWebPage webPage;
    }
    {
        QWebPage webPage;
        webPage.selectedText();
    }
    {
        QWebPage webPage;
        webPage.selectedHtml();
    }
    {
        QWebPage webPage;
        webPage.triggerAction(QWebPage::Back, true);
    }
    {
        QWebPage webPage;
        QPoint pos(10,10);
        webPage.updatePositionDependentActions(pos);
    }
}

static void takeScreenshot(QWebPage* page)
{
    QWebFrame* mainFrame = page->mainFrame();
    page->setViewportSize(mainFrame->contentsSize());
    QImage image(page->viewportSize(), QImage::Format_ARGB32);
    QPainter painter(&image);
    mainFrame->render(&painter);
    painter.end();
}

void tst_QWebPage::screenshot_data()
{
    QTest::addColumn<QString>("html");
    QTest::newRow("WithoutPlugin") << "<html><body id='b'>text</body></html>";
    QTest::newRow("WindowedPlugin") << QString("<html><body id='b'>text<embed src='resources/test.swf'></embed></body></html>");
    QTest::newRow("WindowlessPlugin") << QString("<html><body id='b'>text<embed src='resources/test.swf' wmode='transparent'></embed></body></html>");
}

void tst_QWebPage::screenshot()
{
    if (!QDir(TESTS_SOURCE_DIR).exists())
        W_QSKIP(QString("This test requires access to resources found in '%1'").arg(TESTS_SOURCE_DIR).toLatin1().constData(), SkipAll);

    QDir::setCurrent(TESTS_SOURCE_DIR);

    QFETCH(QString, html);
    QWebPage* page = new QWebPage;
    page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QWebFrame* mainFrame = page->mainFrame();
    mainFrame->setHtml(html, QUrl::fromLocalFile(TESTS_SOURCE_DIR));
    ::waitForSignal(mainFrame, SIGNAL(loadFinished(bool)), 2000);

    // take screenshot without a view
    takeScreenshot(page);

    QWebView* view = new QWebView;
    view->setPage(page);

    // take screenshot when attached to a view
    takeScreenshot(page);

    delete page;
    delete view;

    QDir::setCurrent(QApplication::applicationDirPath());
}

#if defined(ENABLE_WEBGL) && ENABLE_WEBGL
// https://bugs.webkit.org/show_bug.cgi?id=54138
static void webGLScreenshotWithoutView(bool accelerated)
{
    QWebPage page;
    page.settings()->setAttribute(QWebSettings::WebGLEnabled, true);
    page.settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, accelerated);
    QWebFrame* mainFrame = page.mainFrame();
    mainFrame->setHtml("<html><body>"
                       "<canvas id='webgl' width='300' height='300'></canvas>"
                       "<script>document.getElementById('webgl').getContext('experimental-webgl')</script>"
                       "</body></html>");

    takeScreenshot(&page);
}

void tst_QWebPage::acceleratedWebGLScreenshotWithoutView()
{
    webGLScreenshotWithoutView(true);
}

void tst_QWebPage::unacceleratedWebGLScreenshotWithoutView()
{
    webGLScreenshotWithoutView(false);
}
#endif

void tst_QWebPage::originatingObjectInNetworkRequests()
{
    TestNetworkManager* networkManager = new TestNetworkManager(m_page);
    m_page->setNetworkAccessManager(networkManager);
    networkManager->requests.clear();

    m_view->setHtml(QString("<frameset cols=\"25%,75%\"><frame src=\"data:text/html,"
                            "<head><meta http-equiv='refresh' content='1'></head>foo \">"
                            "<frame src=\"data:text/html,bar\"></frameset>"), QUrl());
    QVERIFY(::waitForSignal(m_view, SIGNAL(loadFinished(bool))));

    QCOMPARE(networkManager->requests.count(), 2);

    QList<QWebFrame*> childFrames = m_page->mainFrame()->childFrames();
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=118660", Continue);
    QCOMPARE(childFrames.count(), 2);

    for (int i = 0; i < 2; ++i)
        QVERIFY(qobject_cast<QWebFrame*>(networkManager->requests.at(i).originatingObject()) == childFrames.at(i));
}

void tst_QWebPage::networkReplyParentDidntChange()
{
    TestNetworkManager* networkManager = new TestNetworkManager(m_page);
    m_page->setNetworkAccessManager(networkManager);
    networkManager->requests.clear();

    // Trigger a load and check that pending QNetworkReplies haven't been reparented before returning to the event loop.
    m_view->load(QUrl("qrc:///resources/content.html"));

    QVERIFY(networkManager->requests.count() > 0);
    QVERIFY(networkManager->findChildren<QNetworkReply*>().size() > 0);
}

void tst_QWebPage::destroyQNAMBeforeAbortDoesntCrash()
{
    QNetworkAccessManager* networkManager = new QNetworkAccessManager;
    m_page->setNetworkAccessManager(networkManager);

    m_view->load(QUrl("qrc:///resources/content.html"));
    delete networkManager;
    // This simulates what PingLoader does with its QNetworkReply when it times out.
    // PingLoader isn't attached to a QWebPage and can be kept alive
    // for 60000 seconds (~16.7 hours) to then cancel its ResourceHandle.
    m_view->stop();
}

/**
 * Test fixups for https://bugs.webkit.org/show_bug.cgi?id=30914
 *
 * From JS we test the following conditions.
 *
 *   OK     + QString() => SUCCESS, empty string (but not null)
 *   OK     + "text"    => SUCCESS, "text"
 *   CANCEL + QString() => CANCEL, null string
 *   CANCEL + "text"    => CANCEL, null string
 */
class JSPromptPage : public QWebPage {
    Q_OBJECT
public:
    JSPromptPage()
    {}

    bool javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result)
    {
        if (msg == QLatin1String("test1")) {
            *result = QString();
            return true;
        } else if (msg == QLatin1String("test2")) {
            *result = QLatin1String("text");
            return true;
        } else if (msg == QLatin1String("test3")) {
            *result = QString();
            return false;
        } else if (msg == QLatin1String("test4")) {
            *result = QLatin1String("text");
            return false;
        }

        qFatal("Unknown msg.");
        return QWebPage::javaScriptPrompt(frame, msg, defaultValue, result);
    }
};

void tst_QWebPage::testJSPrompt()
{
    JSPromptPage page;
    bool res;

    // OK + QString()
    res = page.mainFrame()->evaluateJavaScript(
            "var retval = prompt('test1');"
            "retval=='' && retval.length == 0;").toBool();
    QVERIFY(res);

    // OK + "text"
    res = page.mainFrame()->evaluateJavaScript(
            "var retval = prompt('test2');"
            "retval=='text' && retval.length == 4;").toBool();
    QVERIFY(res);

    // Cancel + QString()
    res = page.mainFrame()->evaluateJavaScript(
            "var retval = prompt('test3');"
            "retval===null;").toBool();
    QVERIFY(res);

    // Cancel + "text"
    res = page.mainFrame()->evaluateJavaScript(
            "var retval = prompt('test4');"
            "retval===null;").toBool();
    QVERIFY(res);
}

class TestModalPage : public QWebPage
{
    Q_OBJECT
public:
    TestModalPage(QObject* parent = 0) : QWebPage(parent) {
    }
    virtual QWebPage* createWindow(WebWindowType) {
        QWebPage* page = new TestModalPage();
        connect(page, SIGNAL(windowCloseRequested()), page, SLOT(deleteLater()));
        return page;
    }
};

void tst_QWebPage::showModalDialog()
{
    TestModalPage page;
    page.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    page.mainFrame()->setHtml(QString("<html></html>"));
    QString res = page.mainFrame()->evaluateJavaScript("window.showModalDialog('javascript:window.returnValue=dialogArguments; window.close();', 'This is a test');").toString();
    QCOMPARE(res, QString("This is a test"));
}

void tst_QWebPage::testStopScheduledPageRefresh()
{    
    // Without QWebPage::StopScheduledPageRefresh
    QWebPage page1;
    page1.setNetworkAccessManager(new TestNetworkManager(&page1));
    page1.mainFrame()->setHtml("<html><head>"
                                "<meta http-equiv=\"refresh\"content=\"0;URL=qrc:///resources/index.html\">"
                                "</head><body><h1>Page redirects immediately...</h1>"
                                "</body></html>");
    QVERIFY(::waitForSignal(&page1, SIGNAL(loadFinished(bool))));
    QTest::qWait(500);
    QCOMPARE(page1.mainFrame()->url(), QUrl(QLatin1String("qrc:///resources/index.html")));
    
    // With QWebPage::StopScheduledPageRefresh
    QWebPage page2;
    page2.setNetworkAccessManager(new TestNetworkManager(&page2));
    page2.mainFrame()->setHtml("<html><head>"
                               "<meta http-equiv=\"refresh\"content=\"1;URL=qrc:///resources/index.html\">"
                               "</head><body><h1>Page redirect test with 1 sec timeout...</h1>"
                               "</body></html>");
    page2.triggerAction(QWebPage::StopScheduledPageRefresh);
    QTest::qWait(1500);
    QEXPECT_FAIL("", "https://bugs.webkit.org/show_bug.cgi?id=118673", Continue);
    QCOMPARE(page2.mainFrame()->url().toString(), QLatin1String("about:blank"));
}

void tst_QWebPage::findText()
{
    m_view->setHtml(QString("<html><head></head><body><div>foo bar</div></body></html>"));
    m_page->triggerAction(QWebPage::SelectAll);
    QVERIFY(!m_page->selectedText().isEmpty());
    QVERIFY(!m_page->selectedHtml().isEmpty());
    m_page->findText("");
    QVERIFY(m_page->selectedText().isEmpty());
    QVERIFY(m_page->selectedHtml().isEmpty());
    QStringList words = (QStringList() << "foo" << "bar");
    foreach (QString subString, words) {
        m_page->findText(subString, QWebPage::FindWrapsAroundDocument);
        QCOMPARE(m_page->selectedText(), subString);
        QVERIFY(m_page->selectedHtml().contains(subString));
        m_page->findText("");
        QVERIFY(m_page->selectedText().isEmpty());
        QVERIFY(m_page->selectedHtml().isEmpty());
    }
}

static QString getMimeTypeForExtension(const QString &ext)
{
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QStringLiteral("filename.") + ext.toLower(), QMimeDatabase::MatchExtension);
    if (mimeType.isValid() && !mimeType.isDefault())
        return mimeType.name();

    return QString();
}

void tst_QWebPage::supportedContentType()
{
    QStringList contentTypes;

    // Add supported non image types...
    contentTypes << "text/html" << "text/xml" << "text/xsl" << "text/plain" << "text/"
                 << "application/xml" << "application/xhtml+xml" << "application/vnd.wap.xhtml+xml"
                 << "application/rss+xml" << "application/atom+xml" << "application/json";

#if ENABLE_MHTML
    contentTypes << "application/x-mimearchive";
#endif

    // Add supported image types...
    Q_FOREACH(const QByteArray& imageType, QImageWriter::supportedImageFormats()) {
        const QString mimeType = getMimeTypeForExtension(imageType);
        if (!mimeType.isEmpty())
            contentTypes << mimeType;
    }

    // Get the mime types supported by webkit...
    const QStringList supportedContentTypes = m_page->supportedContentTypes();

    Q_FOREACH(const QString& mimeType, contentTypes)
        QVERIFY2(supportedContentTypes.contains(mimeType), QString("'%1' is not a supported content type!").arg(mimeType).toLatin1());
      
    Q_FOREACH(const QString& mimeType, contentTypes)
        QVERIFY2(m_page->supportsContentType(mimeType), QString("Cannot handle content types '%1'!").arg(mimeType).toLatin1());
}


void tst_QWebPage::navigatorCookieEnabled()
{
    m_page->networkAccessManager()->setCookieJar(0);
    QVERIFY(!m_page->networkAccessManager()->cookieJar());
    QVERIFY(!m_page->mainFrame()->evaluateJavaScript("navigator.cookieEnabled").toBool());

    m_page->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
    QVERIFY(m_page->networkAccessManager()->cookieJar());
    QVERIFY(m_page->mainFrame()->evaluateJavaScript("navigator.cookieEnabled").toBool());
}

void tst_QWebPage::thirdPartyCookiePolicy()
{
    QWebSettings::globalSettings()->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
    m_page->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
    QVERIFY(m_page->networkAccessManager()->cookieJar());

    // These are all first-party cookies, so should pass.
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.com"), QUrl("http://example.com")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.com"), QUrl("http://doc.example.com")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://aaa.www.example.com"), QUrl("http://doc.example.com")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://example.com"), QUrl("http://www.example.com")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.co.uk"), QUrl("http://example.co.uk")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.co.uk"), QUrl("http://doc.example.co.uk")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://aaa.www.example.co.uk"), QUrl("http://doc.example.co.uk")));
    QVERIFY(DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://example.co.uk"), QUrl("http://www.example.co.uk")));

    // These are all third-party cookies, so should fail.
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.com"), QUrl("http://slashdot.org")));
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://example.com"), QUrl("http://anotherexample.com")));
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://anotherexample.com"), QUrl("http://example.com")));
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://www.example.co.uk"), QUrl("http://slashdot.co.uk")));
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://example.co.uk"), QUrl("http://anotherexample.co.uk")));
    QVERIFY(!DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(m_page->handle(),
            QUrl("http://anotherexample.co.uk"), QUrl("http://example.co.uk")));
}

#ifdef Q_OS_MAC
void tst_QWebPage::macCopyUnicodeToClipboard()
{
    QString unicodeText = QString::fromUtf8("αβγδεζηθικλμπ");
    m_page->mainFrame()->setHtml(QString("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /></head><body>%1</body></html>").arg(unicodeText));
    m_page->triggerAction(QWebPage::SelectAll);
    m_page->triggerAction(QWebPage::Copy);

    QString clipboardData = QString::fromUtf8(QApplication::clipboard()->mimeData()->data(QLatin1String("text/html")));

    QVERIFY(clipboardData.contains(QLatin1String("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />")));
    QVERIFY(clipboardData.contains(unicodeText));
}
#endif

void tst_QWebPage::changeVisibilityState()
{
#if ENABLE_PAGE_VISIBILITY_API
    QVariant stateBool, stateString, cpt;

    m_page->mainFrame()->setHtml("<html><body></body></html>");
    m_page->mainFrame()->evaluateJavaScript("var stateBool = undefined, stateString = undefined, cpt = 0; var visibilityCallBack = function () {stateBool = document['hidden']; stateString = document['visibilityState']; cpt++;};document.addEventListener('visibilitychange', visibilityCallBack, false);");

    // The visibility state should be initialised to visible.
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStateVisible);
    stateBool = m_page->mainFrame()->evaluateJavaScript("document['hidden']");
    QVERIFY(stateBool.type() == QVariant::Bool && !stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("document['visibilityState']");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("visible"));

    // Try to change with the same value.
    m_page->setVisibilityState(QWebPage::VisibilityStateVisible);
    QVERIFY(m_page->visibilityState() == QWebPage::VisibilityStateVisible);
    // We check that there isn't any JS event that has been fired and
    // visibility properties are still equals to visible.
    stateBool = m_page->mainFrame()->evaluateJavaScript("document['hidden']");
    QVERIFY(stateBool.type() == QVariant::Bool && !stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("document['visibilityState']");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("visible"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && !cpt.toDouble());

    // Try to change to with different values then check if a JS event has been fired
    // and visibility properties have been updated correctly.
    m_page->setVisibilityState(QWebPage::VisibilityStatePrerender);
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStatePrerender);
    stateBool = m_page->mainFrame()->evaluateJavaScript("stateBool");
    QVERIFY(stateBool.type() == QVariant::Bool && stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("stateString");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("prerender"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && cpt.toDouble() == 1);

    m_page->setVisibilityState(QWebPage::VisibilityStateUnloaded);
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStateUnloaded);
    stateBool = m_page->mainFrame()->evaluateJavaScript("stateBool");
    QVERIFY(stateBool.type() == QVariant::Bool && stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("stateString");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("unloaded"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && cpt.toDouble() == 2);

    m_page->setVisibilityState(QWebPage::VisibilityStateVisible);
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStateVisible);
    stateBool = m_page->mainFrame()->evaluateJavaScript("stateBool");
    QVERIFY(stateBool.type() == QVariant::Bool && !stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("stateString");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("visible"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && cpt.toDouble() == 3);

    m_page->setVisibilityState(QWebPage::VisibilityStateHidden);
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStateHidden);
    stateBool = m_page->mainFrame()->evaluateJavaScript("stateBool");
    QVERIFY(stateBool.type() == QVariant::Bool && stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("stateString");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("hidden"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && cpt.toDouble() == 4);

    m_page->setVisibilityState(QWebPage::VisibilityStateVisible);
    QCOMPARE(m_page->visibilityState(), QWebPage::VisibilityStateVisible);
    stateBool = m_page->mainFrame()->evaluateJavaScript("stateBool");
    QVERIFY(stateBool.type() == QVariant::Bool && !stateBool.toBool());
    stateString = m_page->mainFrame()->evaluateJavaScript("stateString");
    QVERIFY(stateString.type() == QVariant::String && stateString.toString() == QString("visible"));
    cpt = m_page->mainFrame()->evaluateJavaScript("cpt");
    QVERIFY(cpt.type() == QVariant::Double && cpt.toDouble() == 5);
#endif
}

void tst_QWebPage::contextMenuCopy()
{
    QWebView view;

    view.setHtml("<a href=\"http://www.google.com\">You cant miss this</a>");

    view.page()->triggerAction(QWebPage::SelectAll);
    QVERIFY(!view.page()->selectedText().isEmpty());

    QWebElement link = view.page()->mainFrame()->findFirstElement("a");
    QPoint pos(link.geometry().center());
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos);
    view.page()->swallowContextMenuEvent(&event);
    view.page()->updatePositionDependentActions(pos);

    QList<QMenu*> contextMenus = view.findChildren<QMenu*>();
    QVERIFY(!contextMenus.isEmpty());
    QMenu* contextMenu = contextMenus.first();
    QVERIFY(contextMenu);
    
    QList<QAction *> list = contextMenu->actions();
    int index = list.indexOf(view.page()->action(QWebPage::Copy));
    QVERIFY(index != -1);
}

// https://bugs.webkit.org/show_bug.cgi?id=62139
void tst_QWebPage::contextMenuPopulatedOnce()
{
    QWebView view;

    view.setHtml("<input type=\"text\">");

    QWebElement link = view.page()->mainFrame()->findFirstElement("input");
    QPoint pos(link.geometry().center());
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos);
    view.page()->swallowContextMenuEvent(&event);
    view.page()->updatePositionDependentActions(pos);

    QList<QMenu*> contextMenus = view.findChildren<QMenu*>();
    QVERIFY(!contextMenus.isEmpty());
    QMenu* contextMenu = contextMenus.first();
    QVERIFY(contextMenu);

    QList<QAction *> list = contextMenu->actions();
    QStringList entries;
    while (!list.isEmpty()) {
        QString entry = list.takeFirst()->text();
        QVERIFY(!entries.contains(entry));
        entries << entry;
    }
}

void tst_QWebPage::deleteQWebViewTwice()
{
    for (int i = 0; i < 2; ++i) {
        QMainWindow mainWindow;
        QWebView* webView = new QWebView(&mainWindow);
        mainWindow.setCentralWidget(webView);
        webView->load(QUrl("qrc:///resources/frame_a.html"));
        mainWindow.show();
        QVERIFY(::waitForSignal(webView, SIGNAL(loadFinished(bool))));
    }
}

class RepaintRequestedRenderer : public QObject {
    Q_OBJECT
public:
    RepaintRequestedRenderer(QWebPage* page, QPainter* painter)
        : m_page(page)
        , m_painter(painter)
        , m_recursionCount(0)
    {
        connect(m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(onRepaintRequested(QRect)));
    }

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onRepaintRequested(const QRect& rect)
    {
        QCOMPARE(m_recursionCount, 0);

        m_recursionCount++;
        m_page->mainFrame()->render(m_painter, rect);
        m_recursionCount--;

        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }

private:
    QWebPage* m_page;
    QPainter* m_painter;
    int m_recursionCount;
};

void tst_QWebPage::renderOnRepaintRequestedShouldNotRecurse()
{
    QSize viewportSize(720, 576);
    QWebPage page;

    QImage image(viewportSize, QImage::Format_ARGB32);
    QPainter painter(&image);

    page.setPreferredContentsSize(viewportSize);
    page.setViewportSize(viewportSize);
    RepaintRequestedRenderer r(&page, &painter);

    page.mainFrame()->setHtml("zalan loves trunk", QUrl());

    QVERIFY(::waitForSignal(&r, SIGNAL(finished())));
}

class SpyForLoadSignalsOrder : public QStateMachine {
    Q_OBJECT
public:
    SpyForLoadSignalsOrder(QWebPage* page, QObject* parent = 0)
        : QStateMachine(parent)
    {
        connect(page, SIGNAL(loadProgress(int)), SLOT(onLoadProgress(int)));

        QState* waitingForLoadStarted = new QState(this);
        QState* waitingForLastLoadProgress = new QState(this);
        QState* waitingForLoadFinished = new QState(this);
        QFinalState* final = new QFinalState(this);

        waitingForLoadStarted->addTransition(page, SIGNAL(loadStarted()), waitingForLastLoadProgress);
        waitingForLastLoadProgress->addTransition(this, SIGNAL(lastLoadProgress()), waitingForLoadFinished);
        waitingForLoadFinished->addTransition(page, SIGNAL(loadFinished(bool)), final);

        setInitialState(waitingForLoadStarted);
        start();
    }
    bool isFinished() const
    {
        return !isRunning();
    }
public Q_SLOTS:
    void onLoadProgress(int progress)
    {
        if (progress == 100)
            emit lastLoadProgress();
    }
Q_SIGNALS:
    void lastLoadProgress();
};

void tst_QWebPage::loadSignalsOrder_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::newRow("inline data") << QUrl("data:text/html,This is first page");
    QTest::newRow("simple page") << QUrl("qrc:///resources/content.html");
    QTest::newRow("frameset page") << QUrl("qrc:///resources/index.html");
}

void tst_QWebPage::loadSignalsOrder()
{
    QFETCH(QUrl, url);
    QWebPage page;
    SpyForLoadSignalsOrder loadSpy(&page);
    waitForSignal(&loadSpy, SIGNAL(started()));
    page.mainFrame()->load(url);
    QTRY_VERIFY(loadSpy.isFinished());
}

void tst_QWebPage::undoActionHaveCustomText()
{
    m_page->mainFrame()->setHtml("<div id=test contenteditable></div>");
    m_page->mainFrame()->evaluateJavaScript("document.getElementById('test').focus()");

    m_page->mainFrame()->evaluateJavaScript("document.execCommand('insertText', true, 'Test');");
    QString typingActionText = m_page->action(QWebPage::Undo)->text();

    m_page->mainFrame()->evaluateJavaScript("document.execCommand('indent', true);");
    QString alignActionText = m_page->action(QWebPage::Undo)->text();

    QVERIFY(typingActionText != alignActionText);
}

void tst_QWebPage::openWindowDefaultSize()
{
    TestPage page;
    page.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    // Open a default window.
    page.mainFrame()->evaluateJavaScript("window.open()");
    // Open a too small window.
    page.mainFrame()->evaluateJavaScript("window.open('', '', 'width=10,height=10')");

    QTest::qWait(500);
    // The number of popups created should be two.
    QVERIFY(page.createdWindows.size() == 2);

    QRect requestedGeometry = page.createdWindows[0]->requestedGeometry;
    // Check default size has been requested.
    QVERIFY(requestedGeometry.width() == 0);
    QVERIFY(requestedGeometry.height() == 0);

    requestedGeometry = page.createdWindows[1]->requestedGeometry;
    // Check minimum size has been requested.
    QVERIFY(requestedGeometry.width() == 100);
    QVERIFY(requestedGeometry.height() == 100);
}

void tst_QWebPage::cssMediaTypeGlobalSetting()
{
    QString testHtml("<style>@media tv {body{background-color:red;}}@media handheld {body{background-color:green;}}@media screen {body{background-color:blue;}}</style>");
    QSignalSpy loadSpy(m_view, SIGNAL(loadFinished(bool)));

    QWebSettings::globalSettings()->setCSSMediaType("tv");
    // Clear page specific setting to read from global setting
    m_view->page()->settings()->setCSSMediaType(QString());
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 1);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('tv').matches == true").toBool()); 
    QVERIFY(QWebSettings::globalSettings()->cssMediaType() == "tv");

    QWebSettings::globalSettings()->setCSSMediaType("handheld");
    // Clear page specific setting to read from global setting
    m_view->page()->settings()->setCSSMediaType(QString());
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 2);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('handheld').matches == true").toBool());
    QVERIFY(QWebSettings::globalSettings()->cssMediaType() == "handheld");

    QWebSettings::globalSettings()->setCSSMediaType("screen");
    // Clear page specific setting to read from global setting
    m_view->page()->settings()->setCSSMediaType(QString());
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 3);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('screen').matches == true").toBool()); 
    QVERIFY(QWebSettings::globalSettings()->cssMediaType() == "screen"); 
}

void tst_QWebPage::cssMediaTypePageSetting()
{
    QString testHtml("<style>@media tv {body{background-color:red;}}@media handheld {body{background-color:green;}}@media screen {body{background-color:blue;}}</style>");
    QSignalSpy loadSpy(m_view, SIGNAL(loadFinished(bool)));

    m_view->page()->settings()->setCSSMediaType("tv");
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 1);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('tv').matches == true").toBool()); 
    QVERIFY(m_view->page()->settings()->cssMediaType() == "tv"); 

    m_view->page()->settings()->setCSSMediaType("handheld");
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 2);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('handheld').matches == true").toBool());
    QVERIFY(m_view->page()->settings()->cssMediaType() == "handheld");

    m_view->page()->settings()->setCSSMediaType("screen");
    m_view->setHtml(testHtml);
    QTRY_COMPARE(loadSpy.count(), 3);
    QVERIFY(m_view->page()->mainFrame()->evaluateJavaScript("window.matchMedia('screen').matches == true").toBool()); 
    QVERIFY(m_view->page()->settings()->cssMediaType() == "screen"); 
}

QTEST_MAIN(tst_QWebPage)
#include "tst_qwebpage.moc"
