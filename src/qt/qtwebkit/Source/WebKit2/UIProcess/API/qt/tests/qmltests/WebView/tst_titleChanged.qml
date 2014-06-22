import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    SignalSpy {
        id: spyTitle
        target: webView
        signalName: "titleChanged"
    }

    TestCase {
        name: "WebViewTitleChangedSignal"

        function test_titleFirstLoad() {
            compare(spyTitle.count, 0)
            var testUrl = Qt.resolvedUrl("../common/test3.html")
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())            
            compare(webView.title, "Test page 3")
            spyTitle.clear()

            spyTitle.wait()
            compare(webView.title, "New Title")
        }
    }
}
