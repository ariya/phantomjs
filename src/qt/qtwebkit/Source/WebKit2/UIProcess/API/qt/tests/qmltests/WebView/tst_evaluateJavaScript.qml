import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        property variant lastMessage
        property variant lastResult

        experimental.preferences.navigatorQtObjectEnabled: true
        experimental.onMessageReceived: {
            lastMessage = message
        }
    }

    SignalSpy {
        id: messageSpy
        target: webView.experimental
        signalName: "messageReceived"
    }

    SignalSpy {
        id: resultSpy
        target: webView
        signalName: "lastResultChanged"
    }

    TestCase {
        name: "JavaScriptEvaluation"

        function init() {
            messageSpy.clear()
            webView.lastMessage = null

            resultSpy.clear()
            webView.lastResult = null
        }

        function test_basic() {
            messageSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "navigator.qt.onmessage = function(message) {" +
                "    var result = message.data.split('');" +
                "    result = result.reverse().join('');" +
                "    navigator.qt.postMessage(result);" +
                "}");

            webView.experimental.postMessage("DLROW OLLEH");
            messageSpy.wait()
            compare(webView.lastMessage.data, "HELLO WORLD")
        }

        function test_propertyObjectWithChild() {
            resultSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() {" +
                "    var parent = new Object;" +
                "    var child = new Object;" +
                "    parent['level'] = '1';" +
                "    child['level'] = 2;" +
                "    parent['child'] = child;" +
                "    return parent;" +
                "})()",

                function(result) {
                    webView.lastResult = result
                });

            resultSpy.wait()

            compare(JSON.stringify(webView.lastResult),
                '{"child":{"level":2},"level":"1"}')
        }

        function test_booleanValue() {
            resultSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() { return true })()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            verify(typeof webView.lastResult === "boolean")
            compare(webView.lastResult, true)
        }

        function test_stringValue() {
            resultSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() { return 'dongs' })()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            verify(typeof webView.lastResult === "string")
            compare(webView.lastResult, "dongs")
        }

        function test_integerValue() {
            resultSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() { return 1337 })()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            verify(typeof webView.lastResult === "number")
            compare(webView.lastResult, 1337)
        }

        function test_floatValue() {
            resultSpy.clear()
            webView.url = "about:blank"
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() { return 13.37 })()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            verify(typeof webView.lastResult === "number")
            compare(webView.lastResult, 13.37)
        }

        function test_queryTitle() {
            resultSpy.clear()
            var testUrl = Qt.resolvedUrl("../common/evaluatejavascript.html")
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() {" +
                "   return document.title" +
                "})()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            compare(webView.lastResult, "Evaluate JavaScript")
        }

        function test_queryById() {
            resultSpy.clear()
            var testUrl = Qt.resolvedUrl("../common/evaluatejavascript.html")
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())

            webView.experimental.evaluateJavaScript(
                "(function() {" +
                "   return document.getElementById('text').innerHTML" +
                "})()",

                function(result) {
                    webView.lastResult = result
                })

            resultSpy.wait()
            compare(webView.lastResult, "Hello from the WebProcess :-)")
        }
    }
}
