import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import Test 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 320
        height: 240

        property variant result

        property variant content: "data:text/html," +
            "<head>" +
            "    <meta name='viewport' content='width=device-width'>" +
            "</head>" +
            "<body>" +
            "    <div id='target' " +
            "         style='position:absolute; left:20; top:20; width:220; height:80;'>" +
            "    </div>" +
            "    <div id='smalltarget' " +
            "         style='position:absolute; left:20; top:120; width:140; height:80;'>" +
            "    </div>" +
            "</body>"

        signal resultReceived
    }

    SignalSpy {
        id: resultSpy
        target: webView
        signalName: "resultReceived"
    }

    SignalSpy {
        id: scaleSpy
        target: webView.experimental.test
        signalName: "contentsScaleCommitted"
    }

    TestCase {
        name: "DoubleTapToZoom"
        when: windowShown

        property variant test: webView.experimental.test

        function init() {
            resultSpy.clear()
            scaleSpy.clear()
        }

        function windowSize() {
            resultSpy.clear();
            var result;

             webView.experimental.evaluateJavaScript(
                "window.innerWidth + 'x' + window.innerHeight",
                function(size) { webView.resultReceived(); result = size });
            resultSpy.wait();
            return result;
        }

        function elementRect(id) {
            resultSpy.clear();
            var result;

             webView.experimental.evaluateJavaScript(
                "JSON.stringify(document.getElementById('" + id + "').getBoundingClientRect());",
                function(rect) { webView.resultReceived(); result = JSON.parse(rect); });
            resultSpy.wait();
            return result;
        }

        function doubleTapAtPoint(x, y) {
            scaleSpy.clear()
            test.touchDoubleTap(webView, x, y)
            scaleSpy.wait()
        }

        function test_basic_zoomInAndBack() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(windowSize(), "320x240")

            compare(test.contentsScale, 1.0)

            var rect = elementRect("target");
            var newScale = webView.width / (rect.width + 2 * 10) // inflated by 10px
            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, newScale)

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, 1.0)
        }

        function test_double_zoomInAndBack() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(windowSize(), "320x240")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var smalltarget = elementRect("smalltarget");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px
            var smallTargetScale = webView.width / (smalltarget.width + 2 * 10) // inflated by 10px

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 160)

            compare(test.contentsScale, smallTargetScale)

            // Zoom out by double clicking first the small target and then the large target.
            doubleTapAtPoint(100, 120)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, 1.0)
        }

        function test_double_zoomInAndBack2() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(windowSize(), "320x240")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var smalltarget = elementRect("smalltarget");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px
            var smallTargetScale = webView.width / (smalltarget.width + 2 * 10) // inflated by 10px

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 160)

            compare(test.contentsScale, smallTargetScale)

            // Zoom out by double clicking the small target twice.
            doubleTapAtPoint(100, 120)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 160)

            compare(test.contentsScale, 1.0)
        }

        function test_double_zoomInOutAndBack() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(windowSize(), "320x240")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var smalltarget = elementRect("smalltarget");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px
            var smallTargetScale = webView.width / (smalltarget.width + 2 * 10) // inflated by 10px

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 160)

            compare(test.contentsScale, smallTargetScale)

            // Zoom out by double clicking the large target twice.
            doubleTapAtPoint(100, 40)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, 1.0)
        }

        function test_double_zoomInOutAndBack2() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(windowSize(), "320x240")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var smalltarget = elementRect("smalltarget");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px
            var smallTargetScale = webView.width / (smalltarget.width + 2 * 10) // inflated by 10px

            // Zoom in directly to the small target, and then out over the large target.
            doubleTapAtPoint(100, 140)

            compare(test.contentsScale, smallTargetScale)

            doubleTapAtPoint(100, 20)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, 1.0)
        }
    }
}
