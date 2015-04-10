import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

WebView {
    id: webView
    width: 200
    height: 200

    property int expectedLength: 0
    property int totalBytes: 0

    signal downloadFinished()

    SignalSpy {
        id: spy
        target: experimental
        signalName: "downloadRequested"
    }

    SignalSpy {
        id: downloadFinishedSpy
        target: webView
        signalName: "downloadFinished"
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

    TestCase {
        name: "WebViewDownload"

        // Delayed windowShown to workaround problems with Qt5 in debug mode.
        when: false
        Timer {
            running: parent.windowShown
            repeat: false
            interval: 1
            onTriggered: parent.when = true
        }

        function init() {
            spy.clear()
            downloadFinishedSpy.clear()
            expectedLength = 0
        }

        function test_downloadRequest() {
            compare(spy.count, 0)
            webView.url = Qt.resolvedUrl("../common/download.zip")
            spy.wait()
            compare(spy.count, 1)
        }

        function test_expectedLength() {
            compare(spy.count, 0)
            webView.url = Qt.resolvedUrl("../common/download.zip")
            spy.wait()
            compare(spy.count, 1)
            compare(expectedLength, 325)
        }

        function test_succeeded() {
            compare(spy.count, 0)
            webView.url = Qt.resolvedUrl("../common/download.zip")
            spy.wait()
            compare(spy.count, 1)
            downloadFinishedSpy.wait()
            compare(totalBytes, expectedLength)
        }
    }
}
