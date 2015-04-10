import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView

    width: 400
    height: 400

    property int initialSelection
    property int finalSelection
    property bool useAcceptDirectly
    property bool selectorLoaded

    experimental.itemSelector: Item {
        Component.onCompleted: {
            if (WebView.view.initialSelection != -1)
                model.items.select(WebView.view.initialSelection)

            if (WebView.view.finalSelection == -1)
                model.reject()
            else {
                if (useAcceptDirectly) {
                    model.accept(WebView.view.finalSelection)
                } else {
                    model.items.select(WebView.view.finalSelection)
                    model.accept()
                }
            }

            WebView.view.selectorLoaded = true
        }
    }

    SignalSpy {
        id: titleSpy
        target: webView
        signalName: "titleChanged"
    }

    TestCase {
        id: test
        name: "WebViewItemSelector"
        when: windowShown

        function init() {
            webView.initialSelection = -1
            webView.finalSelection = -1
            webView.useAcceptDirectly = false
            webView.selectorLoaded = false
            webView.url = Qt.resolvedUrl("../common/select.html")
            verify(webView.waitForLoadSucceeded())
            titleSpy.clear()
        }

        function openItemSelector() {
            webView.experimental.test.touchTap(webView, 15, 15)
        }

        function test_accept() {
            webView.finalSelection = 1
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "__closed__")
        }

        function test_acceptDirectly() {
            webView.finalSelection = 1
            webView.useAcceptDirectly = true
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "__closed__")
        }

        function test_selectFirstThenAccept() {
            webView.initialSelection = 1
            webView.finalSelection = 2
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "__all__")
        }

        function test_selectFirstThenAcceptDirectly() {
            webView.initialSelection = 1
            webView.finalSelection = 2
            webView.useAcceptDirectly = true
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "__all__")
        }

        function test_reject() {
            openItemSelector()
            tryCompare(webView, "selectorLoaded", true)
            compare(webView.title, "No new selection was made")
        }

        function test_selectFirstThenReject() {
            webView.initialSelection = 1
            webView.finalSelection = -1
            openItemSelector()
            tryCompare(webView, "selectorLoaded", true)
            compare(webView.title, "No new selection was made")
        }

        function test_selectWithSize() {
            webView.url = Qt.resolvedUrl("../common/selectwithsize.html")
            verify(webView.waitForLoadSucceeded())
            titleSpy.clear()

            test_selectFirstThenAcceptDirectly()
        }
    }
}
