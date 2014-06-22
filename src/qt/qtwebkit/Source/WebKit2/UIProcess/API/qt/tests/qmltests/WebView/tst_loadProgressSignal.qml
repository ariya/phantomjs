import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    SignalSpy {
        id: spyProgress
        target: webView
        signalName: "loadProgressChanged"
    }

    TestCase {
        name: "WebViewLoadProgressSignal"

        function test_loadProgressSignal() {
            compare(spyProgress.count, 0)
            compare(webView.loadProgress, 0)
            webView.url = Qt.resolvedUrl("../common/test1.html")
            spyProgress.wait()
            compare(true, webView.loadProgress > -1 && webView.loadProgress < 101)
            if (webView.loadProgress > 0 && webView.loadProgress < 100) {
                verify(webView.waitForLoadSucceeded())
                spyProgress.wait()
                compare(webView.loadProgress, 100)
            }
        }
    }
}
