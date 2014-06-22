import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    TestCase {
        name: "WebViewProperties"

        function test_title() {
            webView.url =  Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.title, "Test page 1")
        }

        function test_url() {
            var testUrl = Qt.resolvedUrl("../common/test1.html")
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, testUrl)
        }
    }
}
