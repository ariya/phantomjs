import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    TestCase {
        name: "WebViewLoadProgress"

        function test_loadProgress() {
            compare(webView.loadProgress, 0)
            webView.url = Qt.resolvedUrl("../common/test1.html")
            compare(webView.loadProgress, 0)
            verify(webView.waitForLoadSucceeded())
            compare(webView.loadProgress, 100)
        }
    }
}
