import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView

    property bool modelMessageEqualsMessage: false
    property string messageFromAlertDialog: ""
    property int confirmCount: 0
    property int promptCount: 0

    experimental {
        alertDialog: Item {
            Component.onCompleted: {
                // Testing both attached property and id defined in the Component context.
                WebView.view.messageFromAlertDialog = message
                parent.modelMessageEqualsMessage = Boolean(model.message == message)
                model.dismiss()
            }
        }

        confirmDialog: Item {
            Component.onCompleted: {
                WebView.view.confirmCount += 1
                if (message == "ACCEPT")
                    model.accept()
                else
                    model.reject()
            }
        }

        promptDialog: Item {
            Component.onCompleted: {
                WebView.view.promptCount += 1
                if (message == "REJECT")
                    model.reject()
                else {
                    var reversedDefaultValue = defaultValue.split("").reverse().join("")
                    model.accept(reversedDefaultValue)
                }
            }
        }
    }

    TestCase {
        id: test
        name: "WebViewJavaScriptDialogs"

        function init() {
            webView.modelMessageEqualsMessage = false
            webView.messageFromAlertDialog = ""
            webView.confirmCount = 0
            webView.promptCount = 0
        }

        function test_alert() {
            webView.url = Qt.resolvedUrl("../common/alert.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.messageFromAlertDialog, "Hello Qt")
            verify(webView.modelMessageEqualsMessage)
        }

        function test_alertWithoutDialog() {
            webView.experimental.alertDialog = null
            webView.url = Qt.resolvedUrl("../common/alert.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.messageFromAlertDialog, "")
        }

        function test_confirm() {
            webView.url = Qt.resolvedUrl("../common/confirm.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.confirmCount, 2)
            compare(webView.title, "ACCEPTED REJECTED")
        }

        function test_confirmWithoutDialog() {
            webView.experimental.confirmDialog = null
            webView.url = Qt.resolvedUrl("../common/confirm.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.confirmCount, 0)
            compare(webView.title, "ACCEPTED ACCEPTED")
        }

        function test_prompt() {
            webView.url = Qt.resolvedUrl("../common/prompt.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.promptCount, 2)
            compare(webView.title, "tQ olleH")
        }

        function test_promptWithoutDialog() {
            webView.experimental.promptDialog = null
            webView.url = Qt.resolvedUrl("../common/prompt.html")
            verify(webView.waitForLoadSucceeded())
            compare(webView.promptCount, 0)
            compare(webView.title, "FAIL")
        }
    }
}
