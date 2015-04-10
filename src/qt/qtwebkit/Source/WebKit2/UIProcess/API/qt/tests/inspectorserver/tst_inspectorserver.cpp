/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "../testwindow.h"
#include "../util.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QtQml/QQmlEngine>
#include <QtTest/QtTest>
#include <private/qquickwebview_p.h>
#include <private/qwebpreferences_p.h>

#define INSPECTOR_SERVER_PORT "23654"
static const QUrl s_inspectorServerHttpBaseUrl("http://localhost:" INSPECTOR_SERVER_PORT);
static const QUrl s_inspectorServerWebSocketBaseUrl("ws://localhost:" INSPECTOR_SERVER_PORT);

class tst_InspectorServer : public QObject {
    Q_OBJECT
public:
    tst_InspectorServer();

private Q_SLOTS:
    void init();
    void cleanup();

    void testPageList();
    void testRemoteDebuggingMessage();
    void openRemoteDebuggingSession();
private:
    void prepareWebViewComponent();
    inline QQuickWebView* newWebView();
    inline QQuickWebView* webView() const;
    QJsonArray fetchPageList() const;
    QScopedPointer<TestWindow> m_window;
    QScopedPointer<QQmlComponent> m_component;
};

tst_InspectorServer::tst_InspectorServer()
{
    qputenv("QTWEBKIT_INSPECTOR_SERVER", INSPECTOR_SERVER_PORT);
    addQtWebProcessToPath();
    prepareWebViewComponent();
}

void tst_InspectorServer::prepareWebViewComponent()
{
    static QQmlEngine* engine = new QQmlEngine(this);
    engine->addImportPath(QString::fromUtf8(IMPORT_DIR));

    m_component.reset(new QQmlComponent(engine, this));

    m_component->setData(QByteArrayLiteral("import QtQuick 2.0\n"
                                           "import QtWebKit 3.0\n"
                                           "WebView {}")
                         , QUrl());
}

QQuickWebView* tst_InspectorServer::newWebView()
{
    QObject* viewInstance = m_component->create();

    return qobject_cast<QQuickWebView*>(viewInstance);
}

void tst_InspectorServer::init()
{
    m_window.reset(new TestWindow(newWebView()));
    webView()->experimental()->preferences()->setDeveloperExtrasEnabled(true);
}

void tst_InspectorServer::cleanup()
{
    m_window.reset();
}

inline QQuickWebView* tst_InspectorServer::webView() const
{
    return static_cast<QQuickWebView*>(m_window->webView.data());
}

QJsonArray tst_InspectorServer::fetchPageList() const
{
    QNetworkAccessManager qnam;
    QScopedPointer<QNetworkReply> reply(qnam.get(QNetworkRequest(s_inspectorServerHttpBaseUrl.resolved(QUrl("pagelist.json")))));
    waitForSignal(reply.data(), SIGNAL(finished()));
    return QJsonDocument::fromJson(reply->readAll()).array();
}

void tst_InspectorServer::testPageList()
{
    QUrl testPageUrl = QUrl::fromLocalFile(QLatin1String(TESTS_SOURCE_DIR "/html/basic_page.html"));
    LoadStartedCatcher catcher(webView());
    webView()->setUrl(testPageUrl);
    waitForSignal(&catcher, SIGNAL(finished()));

    // Our page has developerExtrasEnabled and should be the only one in the list.
    QJsonArray pageList = fetchPageList();
    QCOMPARE(pageList.size(), 1);
    QCOMPARE(testPageUrl.toString(), pageList.at(0).toObject().value("url").toString());
}

void tst_InspectorServer::testRemoteDebuggingMessage()
{
    QJsonArray pageList = fetchPageList();
    QCOMPARE(pageList.size(), 1);

    // Test sending a raw remote debugging message through our web socket server.
    // For this specific message see: http://code.google.com/chrome/devtools/docs/protocol/tot/runtime.html#command-evaluate
    QLatin1String jsExpression("2 + 2");
    QLatin1String jsExpressionResult("4");
    QScopedPointer<QQuickWebView> webSocketQueryWebView(newWebView());
    webSocketQueryWebView->loadHtml(QString(
        "<script type=\"text/javascript\">\n"
        "var socket = new WebSocket('%1/devtools/page/%2');\n"
        "socket.onmessage = function(message) {\n"
            "var response = JSON.parse(message.data);\n"
            "if (response.id === 1)\n"
                "document.title = response.result.result.value;\n"
        "}\n"
        "socket.onopen = function() {\n"
            "socket.send('{\"id\": 1, \"method\": \"Runtime.evaluate\", \"params\": {\"expression\": \"%3\" } }');\n"
        "}\n"
        "</script>")
        .arg(s_inspectorServerWebSocketBaseUrl.toString())
        .arg(pageList.at(0).toObject().value("id").toDouble())
        .arg(jsExpression));

    for (int i = 0; i < 10; ++i) {
        if (!webSocketQueryWebView->title().isEmpty())
            break;
        waitForSignal(webSocketQueryWebView.data(), SIGNAL(titleChanged()), 500);
    }

    QCOMPARE(webSocketQueryWebView->title(), jsExpressionResult);
}

void tst_InspectorServer::openRemoteDebuggingSession()
{
    QJsonArray pageList = fetchPageList();
    QCOMPARE(pageList.size(), 1);

    QScopedPointer<QQuickWebView> inspectorWebView(newWebView());
    LoadStartedCatcher catcher2(inspectorWebView.data());
    inspectorWebView->setUrl(s_inspectorServerHttpBaseUrl.resolved(QUrl(pageList.at(0).toObject().value("inspectorUrl").toString())));
    waitForSignal(&catcher2, SIGNAL(finished()));
    for (int i = 0; i < 10; ++i) {
        if (!inspectorWebView->title().isEmpty())
            break;
        waitForSignal(inspectorWebView.data(), SIGNAL(titleChanged()), 500);
    }

    // To test the whole pipeline this exploits a behavior of the inspector front-end which won't provide any title unless the
    // debugging session was established correctly through web socket. It should be something like "Web Inspector - <Page URL>".
    // So this test case will fail if:
    // - The page list didn't return a valid inspector URL
    // - Or the front-end couldn't be loaded through the inspector HTTP server
    // - Or the web socket connection couldn't be established between the front-end and the page through the inspector server
    // Let's see if this test isn't raising too many false positives, in which case we should use a better predicate if available.
    QVERIFY(!inspectorWebView->title().isEmpty());
}

QTEST_MAIN(tst_InspectorServer)

#include "tst_inspectorserver.moc"
