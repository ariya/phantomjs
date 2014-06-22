import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 200
    height: 400

    TestCase {
        name: "WebViewLoadHtml"

        function test_loadProgressAfterLoadHtml() {
            compare(webView.loadProgress, 0)
            webView.loadHtml("<html><head><title>Test page 1</title></head><body>Hello.</body></html>")
            verify(webView.waitForLoadSucceeded())
            compare(webView.loadProgress, 100)
        }
    }
}
