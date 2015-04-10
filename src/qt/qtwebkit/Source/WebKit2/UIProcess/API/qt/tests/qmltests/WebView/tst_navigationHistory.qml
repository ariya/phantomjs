import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    ListView {
        id: backItemsList
        anchors.fill: parent
        model: webView.experimental.navigationHistory.backItems
        delegate:
            Text {
                color:"black"
                text: "title : " + title
            }
    }

    ListView {
        id: forwardItemsList
        anchors.fill: parent
        model: webView.experimental.navigationHistory.forwardItems
        delegate:
            Text {
                color:"black"
                text: "title : " + title
            }
    }

    TestCase {
        name: "WebViewNavigationHistory"

        function test_navigationHistory() {
            compare(webView.loadProgress, 0)
            webView.url = Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.canGoBack, false)
            compare(webView.canGoForward, false)
            compare(backItemsList.count, 0)
            compare(forwardItemsList.count, 0)
            webView.url = Qt.resolvedUrl("../common/test2.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/test2.html"))
            compare(webView.canGoBack, true)
            compare(webView.canGoForward, false)
            compare(backItemsList.count, 1)
            webView.experimental.goBackTo(0)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/test1.html"))
            compare(webView.canGoBack, false)
            compare(webView.canGoForward, true)
            compare(backItemsList.count, 0)
            compare(forwardItemsList.count, 1)
            webView.goForward()
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/test2.html"))
            compare(webView.canGoBack, true)
            compare(webView.canGoForward, false)
            compare(backItemsList.count, 1)
            compare(forwardItemsList.count, 0)
            webView.url = Qt.resolvedUrl("../common/javascript.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/javascript.html"))
            compare(webView.canGoBack, true)
            compare(webView.canGoForward, false)
            compare(backItemsList.count, 2)
            compare(forwardItemsList.count, 0)
            webView.experimental.goBackTo(1)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/test1.html"))
            compare(webView.canGoBack, false)
            compare(webView.canGoForward, true)
            compare(backItemsList.count, 0)
            compare(forwardItemsList.count, 2)
            webView.experimental.goForwardTo(1)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/javascript.html"))
            compare(webView.canGoBack, true)
            compare(webView.canGoForward, false)
            compare(backItemsList.count, 2)
            compare(forwardItemsList.count, 0)
            webView.goBack()
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, Qt.resolvedUrl("../common/test2.html"))
            compare(webView.canGoBack, true)
            compare(webView.canGoForward, true)
            compare(backItemsList.count, 1)
            compare(forwardItemsList.count, 1)
        }
    }
}
