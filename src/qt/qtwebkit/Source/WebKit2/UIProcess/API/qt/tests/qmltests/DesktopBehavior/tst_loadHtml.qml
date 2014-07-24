import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import "../common"

TestWebView {
    id: webView
    width: 200
    height: 400
    focus: true

    property string lastUrl

    SignalSpy {
        id: linkHoveredSpy
        target: webView
        signalName: "linkHovered"
    }

    onLinkHovered: {
        webView.lastUrl = hoveredUrl
    }

    TestCase {
        name: "DesktopWebViewLoadHtml"

        // Delayed windowShown to workaround problems with Qt5 in debug mode.
        when: false
        Timer {
            running: parent.windowShown
            repeat: false
            interval: 1
            onTriggered: parent.when = true
        }

        function init() {
            webView.lastUrl = ""
            linkHoveredSpy.clear()
        }

        function test_baseUrlAfterLoadHtml() {
            linkHoveredSpy.clear()
            compare(linkHoveredSpy.count, 0)
            webView.loadHtml("<html><head><title>Test page with huge link area</title></head><body><a title=\"A title\" href=\"test1.html\"><img width=200 height=200></a></body></html>", "http://www.example.foo.com")
            verify(webView.waitForLoadSucceeded())
            compare("http://www.example.foo.com/", webView.url)
            mouseMove(webView, 100, 100)
            linkHoveredSpy.wait()
            compare(linkHoveredSpy.count, 1)
            compare(webView.lastUrl, "http://www.example.foo.com/test1.html")
        }
    }
}
