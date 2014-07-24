import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

WebView {
    id: webView
    width: 200
    height: 200

    property bool success: true
    property int port: 0
    property string scheme: "file"

    SignalSpy {
        id: spy
        target: experimental
        signalName: "permissionRequested"
    }

    experimental.onPermissionRequested: {
        if (permission.origin.port != webView.port) {
            console.log("Expected port value should be zero.")
            webView.success = false
        }

        if (permission.origin.scheme != webView.scheme) {
            console.log("Expected scheme should be \"file\".")
            webView.success = false
        }
    }

    TestCase {
        name: "WebViewSecurityOrigin"

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
        }

        function test_permissionRequest() {
            compare(spy.count, 0)
            webView.url = Qt.resolvedUrl("../common/geolocation.html")
            spy.wait()
            compare(spy.count, 1)
            compare(webView.success, true)
        }
    }
}
