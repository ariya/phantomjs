import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

WebView {
    id: webView
    width: 200
    height: 200

    property bool expectedPermission: false

    SignalSpy {
        id: spy
        target: experimental
        signalName: "permissionRequested"
    }

    experimental.onPermissionRequested: {
        //Must be false by default
        if (!permission.allow) {
           permission.allow = true
        } else
           console.log("Fail: permission must be set to false")

        if (permission.type == PermissionRequest.Geolocation) {
            console.log("Permission is geotype")
        }
    }

    TestCase {
        name: "WebViewGeopermission"

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
        }
    }
}
