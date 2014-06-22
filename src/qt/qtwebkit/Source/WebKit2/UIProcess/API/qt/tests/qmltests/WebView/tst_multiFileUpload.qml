import QtQuick 2.0
import QtTest 1.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "../common"

TestWebView {
    id: webView

    width: 400
    height: 400

    property bool selectFile

    experimental.filePicker: Item {
        Component.onCompleted: {
            var selectedFiles = ["filename1", "filename2"]
            if (selectFile)
                model.accept(selectedFiles)
            else
                model.reject();
        }
    }

    SignalSpy {
        id: titleSpy
        target: webView
        signalName: "titleChanged"
    }

    TestCase {
        id: test
        name: "WebViewMultiFilePicker"
        when: windowShown

        function init() {
            webView.url = Qt.resolvedUrl("../common/multifileupload.html")
            verify(webView.waitForLoadSucceeded())
            titleSpy.clear()
        }

        function openItemSelector() {
            webView.experimental.test.touchTap(webView, 15, 15)
        }

        function test_accept() {
            webView.selectFile = true;
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "filename1,filename2")
        }

        function test_reject() {
            var oldTitle = webView.title
            webView.selectFile = false;
            openItemSelector()
            compare(webView.title, oldTitle)
        }
    }
}
