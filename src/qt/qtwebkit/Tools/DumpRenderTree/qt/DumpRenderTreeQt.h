/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef DumpRenderTreeQt_h
#define DumpRenderTreeQt_h

#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTextStream>
#include <QSocketNotifier>

#ifndef QT_NO_OPENSSL
#include <QSslError>
#endif

#include "DumpRenderTreeSupportQt.h"
#include "TestRunner.h"
#include <qgraphicsview.h>
#include <qgraphicswebview.h>
#include <qwebframe.h>
#include <qwebinspector.h>
#include <qwebpage.h>
#include <qwebview.h>
#include <wtf/RefPtr.h>

QT_BEGIN_NAMESPACE
class QUrl;
class QFile;
QT_END_NAMESPACE

class QWebFrameAdapter;
class QWebPageAdapter;

class TestRunnerQt;
class DumpRenderTreeSupportQt;
class EventSender;
class TextInputController;
class GCController;

class WebPage;
class NetworkAccessManager;

class DumpRenderTree : public QObject {
Q_OBJECT

public:
    DumpRenderTree();
    virtual ~DumpRenderTree();

    static DumpRenderTree* instance();

    // Initialize in single-file mode.
    void open(const QUrl& url);

    void setTextOutputEnabled(bool enable) { m_enableTextOutput = enable; }
    bool isTextOutputEnabled() { return m_enableTextOutput; }

    void setGraphicsBased(bool flag) { m_graphicsBased = flag; }
    bool isGraphicsBased() { return m_graphicsBased; }

    void closeRemainingWindows();
    void resetToConsistentStateBeforeTesting(const QUrl&);

    TestRunnerQt *testRunner() const { return m_controller; }
    TestRunner *jscTestRunner() const { return m_jscController.get(); }
    EventSender *eventSender() const { return m_eventSender; }
    TextInputController *textInputController() const { return m_textInputController; }
    QString persistentStoragePath() const { return m_persistentStoragePath; }
    NetworkAccessManager *networkAccessManager() const { return m_networkAccessManager; }

    QWebPage *createWindow();
    int windowCount() const;

    void switchFocus(bool focused);

    WebPage *webPage() const { return m_page; }
    QWebPageAdapter *pageAdapter() const;
    QWebFrameAdapter *mainFrameAdapter() const;

    QList<WebPage*> getAllPages() const;

    void processArgsLine(const QStringList&);
    void setRedirectOutputFileName(const QString& fileName) { m_redirectOutputFileName = fileName; }
    void setRedirectErrorFileName(const QString& fileName) { m_redirectErrorFileName = fileName; }

    void setTimeout(int);
    void setShouldTimeout(bool flag);
    void setShouldDumpPixelsForAllTests() { m_dumpPixelsForAllTests = true; }

public Q_SLOTS:
    void initJSObjects();

    void readLine();
    void processLine(const QString&);

    void dump();
    void titleChanged(const QString &s);
    void connectFrame(QWebFrame *frame);
    void dumpDatabaseQuota(QWebFrame* frame, const QString& dbName);
    void dumpApplicationCacheQuota(QWebSecurityOrigin* origin, quint64 defaultOriginQuota, quint64 totalSpaceNeeded);
    void statusBarMessage(const QString& message);
    void windowCloseRequested();

Q_SIGNALS:
    void quit();
    void ready();

private Q_SLOTS:
    void showPage();
    void hidePage();
    void dryRunPrint(QWebFrame*);
    void loadNextTestInStandAloneMode();
    void geolocationPermissionSet();

private:
    void setStandAloneMode(bool flag) { m_standAloneMode = flag; }
    bool isStandAloneMode() { return m_standAloneMode; }

    QString dumpFramesAsText(QWebFrame* frame);
    QString dumpBackForwardList(QWebPage* page);
    QString dumpFrameScrollPosition(QWebFrame* frame);
    TestRunnerQt *m_controller;
    RefPtr<TestRunner> m_jscController;

    bool m_dumpPixelsForCurrentTest;
    bool m_dumpPixelsForAllTests;
    QString m_expectedHash;
    QStringList m_standAloneModeTestList;

    WebPage *m_page;
    QWidget* m_mainView;

    EventSender *m_eventSender;
    TextInputController *m_textInputController;
    QScopedPointer<GCController> m_gcController;
    NetworkAccessManager* m_networkAccessManager;

    QFile *m_stdin;

    QList<QObject*> windows;
    bool m_enableTextOutput;
    bool m_standAloneMode;
    bool m_graphicsBased;
    QString m_persistentStoragePath;
    QString m_redirectOutputFileName;
    QString m_redirectErrorFileName;
};

class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    NetworkAccessManager(QObject* parent);

private Q_SLOTS:
#ifndef QT_NO_OPENSSL
    void sslErrorsEncountered(QNetworkReply*, const QList<QSslError>&);
#endif
};

class WebPage : public QWebPage {
    Q_OBJECT
public:
    WebPage(QObject* parent, DumpRenderTree*);
    virtual ~WebPage();
    QWebInspector* webInspector();
    void closeWebInspector();

    QWebPage *createWindow(QWebPage::WebWindowType);

    void javaScriptAlert(QWebFrame *frame, const QString& message);
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
    bool javaScriptConfirm(QWebFrame *frame, const QString& msg);
    bool javaScriptPrompt(QWebFrame *frame, const QString& msg, const QString& defaultValue, QString* result);

    void resetSettings();

    virtual bool supportsExtension(QWebPage::Extension extension) const;
    virtual bool extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output);

    QObject* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&);

    void permissionSet(QWebPage::Feature feature);

    virtual bool shouldInterruptJavaScript() { return false; }

public Q_SLOTS:
    void requestPermission(QWebFrame* frame, QWebPage::Feature feature);
    void cancelPermission(QWebFrame* frame, QWebPage::Feature feature);

protected:
    bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);
    bool isTextOutputEnabled() { return m_drt->isTextOutputEnabled(); }

private Q_SLOTS:
    void setViewGeometry(const QRect&);

private:
    QWebInspector* m_webInspector;
    QList<QWebFrame*> m_pendingGeolocationRequests;
    DumpRenderTree *m_drt;
};

class WebViewGraphicsBased : public QGraphicsView {
    Q_OBJECT

public:
    WebViewGraphicsBased(QWidget* parent);
    QGraphicsWebView* graphicsView() const { return m_item; }
    void setPage(QWebPage* page) { m_item->setPage(page); }

private:
    QGraphicsWebView* m_item;
};

#endif
