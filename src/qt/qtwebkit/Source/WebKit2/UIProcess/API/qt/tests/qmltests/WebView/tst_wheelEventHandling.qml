import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import Test 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 800
        height: 600
        url: Qt.resolvedUrl("../common/test4.html")
    }

    SignalSpy {
        id: scrollSpy
        target: webView
        signalName: "contentYChanged"
    }

    TestCase {
        name: "WheelEventHandling"
        when: windowShown

        property variant test: webView.experimental.test

        function init() {
            webView.url = Qt.resolvedUrl("../common/test4.html")
            verify(webView.waitForViewportReady())
            webView.contentY = 0
        }

        function test_wheelScrollEvent() {
            scrollSpy.clear()
            var centerPoint = Qt.point(webView.width / 2, webView.height / 2)
            test.wheelEvent(webView, centerPoint.x, centerPoint.y, -500);
            // The signal spy below will time out if the wheel event did not scroll the content.
            scrollSpy.wait()
            var position = webView.contentY
            webView.reload()
            verify(webView.waitForViewportReady())
            // The check below will fail if the previous position was not restored after reload.
            verify(position == webView.contentY)
        }

        function test_wheelScrollEventAfterReload() {
            scrollSpy.clear()
            webView.reload()
            verify(webView.waitForViewportReady())
            var centerPoint = Qt.point(webView.width / 2, webView.height / 2)
            test.wheelEvent(webView, centerPoint.x, centerPoint.y, -500);
            // The signal spy below will time out if the wheel event did not scroll the content.
            scrollSpy.wait()
        }
    }

}
