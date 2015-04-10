import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView
    width: 400
    height: 300

    property int matchCount: -1
    property bool findFailed: false
    function clear() {
        textFoundSpy.clear()
        findFailed = false
        matchCount = -1
    }

    SignalSpy {
        id: textFoundSpy
        target: webView.experimental
        signalName: "textFound"
    }

    experimental.onTextFound: {
        webView.matchCount = matchCount
        findFailed = matchCount == 0
    }
    TestCase {
        name: "WebViewFindText"

        function test_findText() {
            var findFlags = WebViewExperimental.FindHighlightAllOccurrences |
                              WebViewExperimental.FindCaseSensitively
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("Hello", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 1)
        }
        function test_findTextCaseInsensitive() {
            var findFlags = 0

            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("heLLo", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 1)
        }
        function test_findTextManyMatches() {
            var findFlags = WebViewExperimental.FindHighlightAllOccurrences
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test4.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("bla", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 100)
        }
        function test_findTextBackward() {
            var findFlags = WebViewExperimental.FindHighlightAllOccurrences
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test4.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 10)
            for(var i=0; i < 9; i++) {
                webView.experimental.findText("bla0", findFlags)
                textFoundSpy.wait()
            }
            compare(textFoundSpy.count, 10)
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 11)
            compare(findFailed, true)

            webView.clear()
            findFlags |= WebViewExperimental.FindBackward
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
        }
        function test_findTextFailNoWrap() {
            var findFlags = WebViewExperimental.FindHighlightAllOccurrences
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test4.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 10)
            for(var i=0; i < 9; i++) {
                webView.experimental.findText("bla0", findFlags)
                textFoundSpy.wait()
            }
            compare(textFoundSpy.count, 10)
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 11)
            compare(findFailed, true)
        }
        function test_findTextWrap() {
            var findFlags = WebViewExperimental.FindHighlightAllOccurrences
            findFlags |= WebViewExperimental.FindWrapsAroundDocument
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test4.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("bla0", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(matchCount, 10)
            for(var i=0; i < 19; i++) {
                webView.experimental.findText("bla0", findFlags)
                textFoundSpy.wait()
            }
            compare(textFoundSpy.count, 20)
        }
        function test_findTextFailCaseSensitive() {
            var findFlags = WebViewExperimental.FindCaseSensitively
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("heLLo", findFlags)
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(findFailed, true)
        }
        function test_findTextNotFound() {
            webView.clear()
            webView.url = Qt.resolvedUrl("../common/test1.html")
            verify(webView.waitForLoadSucceeded())
            webView.experimental.findText("string-that-is-not-threre")
            textFoundSpy.wait()
            compare(textFoundSpy.count, 1)
            compare(findFailed, true)
        }
    }
}
