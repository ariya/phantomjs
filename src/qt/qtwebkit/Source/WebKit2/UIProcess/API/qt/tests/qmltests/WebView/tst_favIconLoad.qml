import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView

    SignalSpy {
        id: spy
        target: webView
        signalName: "iconChanged"
    }

    Image {
        id: favicon
        asynchronous: true
        source: webView.icon
    }

    TestCase {
        id: test
        name: "WebViewLoadFavIcon"
        when: windowShown

        function init() {
            if (webView.icon != '') {
                // If this is not the first test, then load a blank page without favicon, restoring the initial state.
                webView.url = 'about:blank'
                verify(webView.waitForLoadSucceeded())
                spy.wait()
            }
            spy.clear()
        }

        function test_favIconLoad() {
            compare(spy.count, 0)
            var url = Qt.resolvedUrl("../common/favicon.html")
            webView.url = url
            verify(webView.waitForLoadSucceeded())
            spy.wait()
            compare(spy.count, 1)
            compare(favicon.width, 48)
            compare(favicon.height, 48)
        }

        function test_favIconLoadEncodedUrl() {
            compare(spy.count, 0)
            var url = Qt.resolvedUrl("../common/favicon2.html?favicon=load should work with#whitespace!")
            webView.url = url
            verify(webView.waitForLoadSucceeded())
            spy.wait()
            compare(spy.count, 1)
            compare(favicon.width, 16)
            compare(favicon.height, 16)

        }
    }
}
