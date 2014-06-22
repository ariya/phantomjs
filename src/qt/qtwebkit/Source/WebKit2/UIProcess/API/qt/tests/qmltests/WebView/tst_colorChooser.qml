import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView

    width: 400
    height: 400

    property bool featureEnabled

    property string selectedColor
    property bool shouldReject
    property bool shouldAcceptCurrent

    experimental.colorChooser: Item {
        Component.onCompleted: {
            if (WebView.view.shouldReject)
                model.reject()
            else if (WebView.view.shouldAcceptCurrent)
                model.accept(model.currentColor)
            else
                model.accept(WebView.view.selectedColor)
        }
    }

    function openColorChooser() {
        webView.experimental.test.touchTap(webView, 25, 25)
    }

    SignalSpy {
        id: titleSpy
        target: webView
        signalName: "titleChanged"
    }

    TestCase {
        id: test
        name: "WebViewColorChooser"
        when: windowShown

        function init() {
            webView.url = Qt.resolvedUrl("../common/colorChooser.html")
            verify(webView.waitForLoadSucceeded())

            while (webView.title != "Feature enabled" && webView.title != "Feature disabled")
                wait(0)

            webView.featureEnabled = (webView.title == "Feature enabled")
            if (!webView.featureEnabled)
                return

            titleSpy.clear()

            webView.shouldReject = false
            webView.shouldAcceptCurrent = false
        }

        function cleanup() {
            titleSpy.clear()
        }

        function test_accept() {
            if (!webView.featureEnabled)
                return

            // The title changes here twice: first
            // when we click, it changes from "Feature enabled"
            // to the sanitized color and next, when we
            // pick a new color with the chooser.
            webView.selectedColor = "#020020"
            openColorChooser()
            while (titleSpy.count != 2)
                wait(0)
            compare(webView.title, "#020020")
        }

        function test_currentValue() {
            if (!webView.featureEnabled)
                return

            webView.shouldAcceptCurrent = true
            openColorChooser()
            titleSpy.wait()
            compare(titleSpy.count, 1)
            compare(webView.title, "#000000")
        }

        function test_reject() {
            if (!webView.featureEnabled)
                return

            webView.shouldReject = true;
            openColorChooser()
            titleSpy.wait()
            compare(titleSpy.count, 1)
            compare(webView.title, "#000000")
        }
    }
}
