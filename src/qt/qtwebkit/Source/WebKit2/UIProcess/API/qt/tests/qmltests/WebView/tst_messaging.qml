import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        property variant lastMessage
        experimental.preferences.navigatorQtObjectEnabled: true
        experimental.onMessageReceived: {
            lastMessage = message
        }
    }

    TestWebView {
        id: otherWebView
        property variant lastMessage
        experimental.preferences.navigatorQtObjectEnabled: true
        experimental.onMessageReceived: {
            lastMessage = message
        }
    }

    TestWebView {
        id: disabledWebView
        property bool receivedMessage
        experimental.preferences.navigatorQtObjectEnabled: false
        experimental.onMessageReceived: {
            receivedMessage = true
        }
    }

    SignalSpy {
        id: messageSpy
        target: webView.experimental
        signalName: "messageReceived"
    }

    SignalSpy {
        id: otherMessageSpy
        target: otherWebView.experimental
        signalName: "messageReceived"
    }

    TestCase {
        name: "WebViewMessaging"
        property url testUrl: Qt.resolvedUrl("../common/messaging.html")

        function init() {
            messageSpy.clear()
            webView.lastMessage = null
            otherMessageSpy.clear()
            otherWebView.lastMessage = null
        }

        function test_basic() {
            webView.url = testUrl
            verify(webView.waitForLoadSucceeded())
            webView.experimental.postMessage("HELLO")
            messageSpy.wait()
            compare(webView.lastMessage.data, "OLLEH")
            compare(webView.lastMessage.origin.toString(), testUrl.toString())
        }

        function test_twoWebViews() {
            webView.url = testUrl
            otherWebView.url = testUrl
            verify(webView.waitForLoadSucceeded())
            verify(otherWebView.waitForLoadSucceeded())
            webView.experimental.postMessage("FIRST")
            otherWebView.experimental.postMessage("SECOND")
            messageSpy.wait()
            otherMessageSpy.wait()
            compare(webView.lastMessage.data, "TSRIF")
            compare(otherWebView.lastMessage.data, "DNOCES")
        }

        function test_disabled() {
            disabledWebView.url = testUrl
            verify(!disabledWebView.experimental.preferences.navigatorQtObjectEnabled)
            verify(disabledWebView.waitForLoadSucceeded())
            disabledWebView.experimental.postMessage("HI")
            wait(1000)
            verify(!disabledWebView.receivedMessage)
        }
    }
}
