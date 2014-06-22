import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import Test 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 480
        height: 720

        property variant result

        property variant content: "data:text/html," +
            "<head>" +
            "    <meta name='viewport' content='width=device-width'>" +
            "</head>" +
            "<body style='margin: 0px'>" +
            "    <div id='target' style='display:none; width:960px; height:1440px;'></div>" +
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
        name: "FitToView"
        when: windowShown

        property variant test: webView.experimental.test

        function init() {
            resultSpy.clear()
            scaleSpy.clear()
        }

        function run(signalSpy, script) {
            signalSpy.clear();
            var result;
             webView.experimental.evaluateJavaScript(
                script, function(value) { webView.resultReceived(); result = value });
            signalSpy.wait();
            return result;
        }

        function documentSize() {
            return run(resultSpy, "document.width + 'x' + document.height");
        }

        function setDisplay(id, value) {
            // When changing to/from 'none' to 'block', this will result in a
            // contentsScaleCommitted scale, even if it results in  the same
            // scale, making it possible to check whether user interaction
            // blocks fit-to-view or not.
            run(scaleSpy, "document.getElementById('" + id + "').style.display = '" + value + "';");
        }

        function test_basic() {
            webView.url = webView.content
            verify(webView.waitForViewportReady())

            compare(documentSize(), "480x720")
            compare(test.contentsScale, 1.0)

            setDisplay("target", "block")
            compare(documentSize(), "960x1440")
            compare(test.contentsScale, 0.5)

            // Add user interaction.
            test.touchTap(webView, 10, 10)

            // We are no longer within valid bounds after this change
            // so we have to change our scale back to 1.0.
            setDisplay("target", "none")
            compare(documentSize(), "480x720")
            compare(test.contentsScale, 1.0)

            // We had user interaction, size should change but not scale.
            setDisplay("target", "block")
            compare(documentSize(), "960x1440")
            compare(test.contentsScale, 1.0)
        }

        function test_localPageDeviceWidth() {
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.url = "../common/test5.html"
            verify(webView.waitForLoadSucceeded())
            compare(test.contentsScale, 0.5)

            // Add user interaction.
            test.touchTap(webView, 10, 10)

            webView.reload()
            verify(webView.waitForLoadSucceeded())
            // The page should still fit to view after a reload
            compare(test.contentsScale, 0.5)
        }

        function test_localPageInitialScale() {
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.url = "../common/test4.html"
            verify(webView.waitForLoadSucceeded())

            compare(test.contentsScale, 2.0)

            // Add user interaction.
            test.touchTap(webView, 10, 10)

            webView.reload()
            verify(webView.waitForLoadSucceeded())
            compare(test.contentsScale, 2.0)
        }
    }
}
