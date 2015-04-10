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
    property bool returnEmpty: false
    property bool acceptMultiple: false

    experimental.filePicker: Item {
        Component.onCompleted: {
            if (returnEmpty)
                model.accept("");
            else if (selectFile) {
                var selectedFiles = ["filename1", "filename2"];
                if (acceptMultiple)
                    model.accept(selectedFiles);
                else
                    model.accept("acceptedfilename");
            } else
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
        name: "WebViewSingleFilePicker"
        when: windowShown

        function init() {
            webView.url = Qt.resolvedUrl("../common/singlefileupload.html")
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
            compare(webView.title, "acceptedfilename")
        }

        function test_multiple() {
            webView.selectFile = true;
            webView.returnEmpty = false;
            webView.acceptMultiple = true;
            openItemSelector()
            titleSpy.wait()
            compare(webView.title, "filename1")
        }

        function test_rejectIfEmptyAccept() {
            var oldTitle = webView.title
            webView.selectFile = false;
            webView.returnEmpty = true;
            openItemSelector()
            compare(webView.title, oldTitle)
        }

        function test_reject() {
            var oldTitle = webView.title
            webView.selectFile = false;
            openItemSelector()
            compare(webView.title, oldTitle)
        }
    }
}
