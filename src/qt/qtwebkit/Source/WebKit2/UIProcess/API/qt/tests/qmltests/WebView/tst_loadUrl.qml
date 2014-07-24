import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    property variant lastUrl
    property bool watchProgress: false
    property int numLoadStarted: 0
    property int numLoadSucceeded: 0

    focus: true

    onLoadProgressChanged: {
        if (watchProgress && webView.loadProgress != 100) {
            watchProgress = false
            url = ''
        }
    }

    onLoadingChanged: {
        if (loadRequest.status == WebView.LoadStartedStatus)
            ++numLoadStarted
        if (loadRequest.status == WebView.LoadSucceededStatus)
            ++numLoadSucceeded
    }

    TestCase {
        id: test
        name: "WebViewLoadUrl"
        when: windowShown

        function test_loadIgnoreEmptyUrl() {
            var url = Qt.resolvedUrl("../common/test1.html")

            webView.url = url
            verify(webView.waitForLoadSucceeded())
            compare(numLoadStarted, 1)
            compare(numLoadSucceeded, 1)
            compare(webView.url, url)

            lastUrl = webView.url
            webView.url = ''
            wait(1000)
            compare(numLoadStarted, 1)
            compare(numLoadSucceeded, 1)
            compare(webView.url, lastUrl)

            webView.url = 'about:blank'
            verify(webView.waitForLoadSucceeded())
            compare(numLoadStarted, 2)
            compare(numLoadSucceeded, 2)
            compare(webView.url, 'about:blank')

            // It shouldn't interrupt any ongoing load when an empty url is used.
            watchProgress = true
            webView.url = url
            webView.waitForLoadSucceeded()
            compare(numLoadStarted, 3)
            compare(numLoadSucceeded, 3)
            verify(!watchProgress)
            compare(webView.url, url)
        }

        function test_urlProperty() {
            var url = Qt.resolvedUrl("../common/test1.html")

            webView.url = url
            compare(webView.url, url)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, url)

            var bogusSite = "http://www.somesitethatdoesnotexist.abc/"
            webView.url = bogusSite
            compare(webView.url, bogusSite)
            verify(webView.waitForLoadFailed())
            compare(webView.url, bogusSite)

            webView.url = "about:blank" // Reset from previous test
            verify(webView.waitForLoadSucceeded())

            var handleLoadFailed = function(loadRequest) {
                if (loadRequest.status == WebView.LoadFailedStatus) {
                    compare(webView.url, bogusSite)
                    compare(loadRequest.url, bogusSite)
                    webView.loadHtml("load failed", bogusSite, bogusSite)
                }
            }
            webView.loadingChanged.connect(handleLoadFailed)
            webView.url = bogusSite
            compare(webView.url, bogusSite)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, bogusSite)
            webView.loadingChanged.disconnect(handleLoadFailed)

            var dataUrl = "data:text/html,foo"
            webView.url = dataUrl
            compare(webView.url, dataUrl)

            var redirectUrl = Qt.resolvedUrl("../common/redirect.html")
            webView.url = redirectUrl
            compare(webView.url, redirectUrl)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, redirectUrl)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, url)

            var linkUrl = Qt.resolvedUrl("../common/link.html")
            webView.url = linkUrl
            compare(webView.url, linkUrl)
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, linkUrl)
            webView.loadingChanged.connect(function(loadRequest) {
                compare(webView.url, loadRequest.url)
                compare(webView.url, url)
            })
            webView.forceActiveFocus()
            keyPress(Qt.Key_Return) // Link is focused
            verify(webView.waitForLoadSucceeded())
            compare(webView.url, url)
        }

        function test_stopStatus() {
            var url = Qt.resolvedUrl("../common/test1.html")

            webView.loadingChanged.connect(function(loadRequest) {
                if (loadRequest.status == WebView.LoadStopStatus) {
                    compare(webView.url, url)
                    compare(loadRequest.url, url)
                }
            })

            webView.url = url
            compare(webView.url, url)
            webView.stop()
            verify(webView.waitForLoadStopped())
            compare(webView.url, url)
        }
    }
}
