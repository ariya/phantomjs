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
        height: 480

        property variant result

        property variant content: "data:text/html," +
            "<head>" +
            "    <meta name='viewport' content='width=device-width'>" +
            "</head>" +
            "<body>" +
            "    <div id='target' style='width: 240px; height: 360px;'>" +
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

    SignalSpy {
        id: sizeSpy
        target: webView.experimental.test
        signalName: "contentsSizeChanged"
    }

    TestCase {
        name: "Resize"
        when: windowShown

        property variant test: webView.experimental.test

        function init() {
            resultSpy.clear()
            scaleSpy.clear()
            sizeSpy.clear()
        }

        function run(signalSpy, script) {
            signalSpy.clear();
            var result;
            webView.experimental.evaluateJavaScript(
                script,
                function(value) { webView.resultReceived(); result = value });
            signalSpy.wait();
            return result;
        }

        function contentsSize() {
            return test.contentsSize.width + "x" + test.contentsSize.height;
        }

        function elementRect(id) {
            return JSON.parse(run(resultSpy, "JSON.stringify(document.getElementById('" + id + "').getBoundingClientRect());"))
        }

        function doubleTapAtPoint(x, y) {
            scaleSpy.clear()
            test.touchDoubleTap(webView, x, y)
            scaleSpy.wait()
        }

        function resize(w, h) {
            sizeSpy.clear()
            webView.width = w
            sizeSpy.wait()
            webView.height = h
            sizeSpy.wait()
        }

        function test_basic() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)

            resize(480, 720)
            compare(contentsSize(), "480x720")
            compare(test.contentsScale, 1.0)

            resize(320, 480)
            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)

        }

        function test_resizeAfterNeutralZoom() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px

            // Zoom in and out.
            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, targetScale)

            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, 1.0)

            // Now check resizing still works as expected.
            resize(480, 720)
            compare(contentsSize(), "480x720")
            compare(test.contentsScale, 1.0)

            resize(320, 480)
            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)
        }

        function test_resizeZoomedIn() {
            // Note that if we change the behavior of resize on zoomed-in content, for instance
            // to preserve the visible width (like rotate), this test will need to be updated.
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)

            var target = elementRect("target");
            var targetScale = webView.width / (target.width + 2 * 10) // inflated by 10px

            // Double tap to zoom in.
            doubleTapAtPoint(100, 50)

            compare(test.contentsScale, targetScale)

            // Resize just a small bit, not changing scale.
            resize(288, 432)
            compare(contentsSize(), "288x432")
            compare(test.contentsScale, targetScale)

            // And double tap to reset zoom.
            target = elementRect("target");
            targetScale = webView.width / (target.width + 2 * 10)
            doubleTapAtPoint(100, 50)
            compare(test.contentsScale, targetScale)

            // Double tap again to zoom out.
            doubleTapAtPoint(100, 50)
            compare(contentsSize(), "288x432")
            compare(test.contentsScale, 1.0)

            // And reset
            resize(320, 480)
            compare(contentsSize(), "320x480")
            compare(test.contentsScale, 1.0)
        }
    }
}
