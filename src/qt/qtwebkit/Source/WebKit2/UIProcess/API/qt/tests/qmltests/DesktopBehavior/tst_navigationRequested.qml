import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

Item {
    property int expectedLength: 0
    property int totalBytes: 0
    property bool shouldDownload: false
    property url beginUrl: Qt.resolvedUrl("../common/test2.html")
    property url endUrl: Qt.resolvedUrl("../common/test1.html")

    TestWebView {
        id: webView
        width: 200
        height: 200

        signal downloadFinished()

        onNavigationRequested: {
            if (shouldDownload)
                request.action = WebViewExperimental.DownloadRequest
            else if (request.mouseButton == Qt.MiddleButton && request.keyboardModifiers & Qt.ControlModifier) {
                otherWebView.url = request.url
                request.action = WebView.IgnoreRequest
            }
        }

        experimental.onDownloadRequested: {
            download.target = downloadItem
            expectedLength = downloadItem.expectedContentLength
            downloadItem.destinationPath = downloadItem.suggestedFilename
            downloadItem.start()
        }

        Connections {
            id: download
            ignoreUnknownSignals: true
            onSucceeded: {
                totalBytes = download.target.totalBytesReceived
                webView.downloadFinished()
            }
        }
    }

    TestWebView {
        id: otherWebView
    }

    SignalSpy {
        id: downloadSpy
        target: webView.experimental
        signalName: "downloadRequested"
    }

    SignalSpy {
        id: downloadFinishedSpy
        target: webView
        signalName: "downloadFinished"
    }

    TestCase {
        name: "DesktopWebViewNavigationRequested"

        // Delayed windowShown to workaround problems with Qt5 in debug mode.
        when: false
        Timer {
            running: parent.windowShown
            repeat: false
            interval: 1
            onTriggered: parent.when = true
        }

        function init() {
            downloadSpy.clear()
            downloadFinishedSpy.clear()
            shouldDownload = false
        }

        function test_usePolicy() {
            webView.url = beginUrl
            verify(webView.waitForLoadSucceeded())
            mouseClick(webView, 100, 100, Qt.LeftButton)
            verify(webView.waitForLoadSucceeded())
            compare(webView.title, "Test page 1")
            compare(webView.url, endUrl)
        }

        function test_ignorePolicy() {
            webView.url = beginUrl
            verify(webView.waitForLoadSucceeded())
            mouseClick(webView, 100, 100, Qt.MiddleButton, Qt.ControlModifier)
            verify(otherWebView.waitForLoadSucceeded())
            verify(webView.loadStatus == null)
            compare(webView.url, beginUrl)
            compare(otherWebView.title, "Test page 1")
            compare(otherWebView.url, endUrl)
        }

        function test_downloadPolicy() {
            webView.url = beginUrl
            verify(webView.waitForLoadSucceeded())
            downloadSpy.clear()
            downloadFinishedSpy.clear()
            expectedLength = 0
            shouldDownload = true
            mouseClick(webView, 100, 100, Qt.LeftButton)
            downloadSpy.wait()
            compare(downloadSpy.count, 1)
            downloadFinishedSpy.wait()
            compare(downloadFinishedSpy.count, 1)
            compare(totalBytes, expectedLength)
        }
    }
}
