import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    property variant testUrl

    TestCase {
        id: test
        name: "WebViewLoadFail"
        function test_fail() {
            testUrl = Qt.resolvedUrl("file_that_does_not_exist.html")
            webView.url = testUrl
            verify(webView.waitForLoadFailed())
        }
    }

    onLoadingChanged: {
        if (loadRequest.status == WebView.LoadFailedStatus) {
            test.compare(loadRequest.url, testUrl)
            test.compare(loadRequest.errorCode, NetworkReply.ContentNotFoundError)
            test.compare(loadRequest.errorDomain, WebView.NetworkErrorDomain)
        }
    }
}
