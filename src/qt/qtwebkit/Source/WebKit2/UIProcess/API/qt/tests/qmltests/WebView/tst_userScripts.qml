import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

Item {
    TestWebView {
        id: webView
        width: 400
        height: 300
    }

    TestWebView {
        id: webViewWithConditionalUserScripts
        width: 400
        height: 300

        onNavigationRequested: {
            var urlString = request.url.toString();
            if (urlString.indexOf("test1.html") !== -1)
                experimental.userScripts = [Qt.resolvedUrl("../common/change-document-title.js")];
            else if (urlString.indexOf("test2.html") !== -1)
                experimental.userScripts = [Qt.resolvedUrl("../common/append-document-title.js")];
            else
                experimental.userScripts = [];
        }
    }

    TestCase {
        name: "WebViewUserScripts"

        function init() {
            webView.url = "";
            webView.experimental.userScripts = [];
        }

        function test_oneScript() {
            webView.url = Qt.resolvedUrl("../common/test1.html");
            webView.waitForLoadSucceeded();
            compare(webView.title, "Test page 1");

            webView.experimental.userScripts = [Qt.resolvedUrl("../common/change-document-title.js")];
            compare(webView.title, "Test page 1");

            webView.reload();
            webView.waitForLoadSucceeded();
            compare(webView.title, "New title");

            webView.url = Qt.resolvedUrl("../common/test2.html");
            webView.waitForLoadSucceeded();
            compare(webView.title, "New title");

            webView.experimental.userScripts = [];
            compare(webView.title, "New title");

            webView.reload();
            webView.waitForLoadSucceeded();
            compare(webView.title, "Test page with huge link area");
        }

        function test_twoScripts() {
            webView.url = Qt.resolvedUrl("../common/test1.html");
            webView.waitForLoadSucceeded();
            compare(webView.title, "Test page 1");

            webView.experimental.userScripts = [Qt.resolvedUrl("../common/change-document-title.js"), Qt.resolvedUrl("../common/append-document-title.js")];
            webView.reload();
            webView.waitForLoadSucceeded();
            compare(webView.title, "New title with appendix");

            // Make sure we can remove scripts from the preload list.
            webView.experimental.userScripts = [Qt.resolvedUrl("../common/append-document-title.js")];
            webView.reload();
            webView.waitForLoadSucceeded();
            compare(webView.title, "Test page 1 with appendix");

            // Make sure the scripts are loaded in order.
            webView.experimental.userScripts = [Qt.resolvedUrl("../common/append-document-title.js"), Qt.resolvedUrl("../common/change-document-title.js")];
            webView.reload();
            webView.waitForLoadSucceeded();
            compare(webView.title, "New title");
        }

        function test_setUserScriptsConditionally() {
            webViewWithConditionalUserScripts.url = Qt.resolvedUrl("../common/test1.html");
            webViewWithConditionalUserScripts.waitForLoadSucceeded();
            compare(webViewWithConditionalUserScripts.title, "New title");

            webViewWithConditionalUserScripts.url = Qt.resolvedUrl("../common/test2.html");
            webViewWithConditionalUserScripts.waitForLoadSucceeded();
            compare(webViewWithConditionalUserScripts.title, "Test page with huge link area with appendix");

            webViewWithConditionalUserScripts.url = Qt.resolvedUrl("../common/test3.html");
            webViewWithConditionalUserScripts.waitForLoadSucceeded();
            compare(webViewWithConditionalUserScripts.title, "Test page 3");
        }

        function test_bigScript() {
            webView.experimental.userScripts = [Qt.resolvedUrl("../common/big-user-script.js")];
            webView.url = Qt.resolvedUrl("../common/test1.html");
            webView.waitForLoadSucceeded();
            compare(webView.title, "Big user script changed title");
        }

        function test_fromResourceFile() {
            webView.experimental.userScripts = ["qrc:///common/change-document-title.js"];
            webView.url = Qt.resolvedUrl("../common/test1.html");
            webView.waitForLoadSucceeded();
            compare(webView.title, "New title");
        }
    }
}
