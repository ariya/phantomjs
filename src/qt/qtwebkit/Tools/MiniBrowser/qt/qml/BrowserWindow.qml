/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
 * Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import QtQuick 2.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

Rectangle {
    id: root
    // Do not define anchors or an initial size here! This would mess up with QSGView::SizeRootObjectToView.

    property alias webview: webView
    color: "#333"

    signal pageTitleChanged(string title)
    signal newWindow(string url)

    function load(address) {
        webView.url = address
        webView.forceActiveFocus()
    }

    function reload() {
        webView.reload()
        webView.forceActiveFocus()
    }

    function focusAddressBar() {
        addressLine.forceActiveFocus()
        addressLine.selectAll()
    }
    function toggleFind() {
        findBar.toggle()
    }
    Rectangle {
        id: findBar
        z: webView.z + 1
        y: navigationBar.y
        anchors {
            left: parent.left
            right: parent.right
        }
        height: navigationBar.height
        color: "#efefef"
        visible: y > navigationBar.y

        Behavior on y {NumberAnimation {duration: 250}}

        function toggle() {
            if (y == navigationBar.y) {
                findTextInput.forceActiveFocus()
                y += height
            } else {
                webView.forceActiveFocus()
                y = navigationBar.y
                find("",0);
            }
        }

        function find(str, options) {
            var findOptions = options | WebViewExperimental.FindHighlightAllOccurrences
            findOptions |= WebViewExperimental.FindWrapsAroundDocument
            webView.experimental.findText(str, findOptions)
        }

        Connections {
            target: webView.experimental
            onTextFound: {
                failedOverlay.visible = matchCount == 0
            }
        }
        Item {
            anchors.fill: parent
            Rectangle {
                id: inputArea
                height: 26
                anchors {
                    left: parent.left
                    right: prevButton.left
                    margins: 6
                    verticalCenter: parent.verticalCenter
                }
                color: "white"
                border.width: 1
                border.color: "#bfbfbf"
                radius: 3
                Rectangle {
                    id: failedOverlay
                    anchors.fill: parent
                    color: "red"
                    opacity: 0.5
                    radius: 6
                    visible: false
                }
                TextInput {
                    id: findTextInput
                    clip: true
                    selectByMouse: true
                    horizontalAlignment: TextInput.AlignLeft
                    anchors.fill: parent
                    anchors.margins: 3
                    font {
                        pointSize: 11
                        family: "Sans"
                    }
                    text: ""
                    readOnly: !findBar.visible
                    function doFind() {
                        if (!findBar.visible) {
                            return;
                        }
                        if (findTextInput.text == "") {
                            failedOverlay.visible = false
                        }
                        findBar.find(findTextInput.text)
                    }
                    onTextChanged: {
                        doFind()
                    }
                    Keys.onReturnPressed:{
                        doFind()
                    }
                }
            }
            Rectangle {
                id: prevButton
                height: inputArea.height
                width: height
                anchors.right: nextButton.left
                anchors.verticalCenter: parent.verticalCenter
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: "../icons/previous.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: parent.color
                    radius: parent.radius
                    opacity: 0.8
                    visible: !parent.enabled
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: { if (parent.enabled) parent.color = "#cfcfcf" }
                    onReleased: { parent.color = "#efefef" }
                    onClicked: {
                        findBar.find(findTextInput.text, WebViewExperimental.FindBackward)
                    }
                }
            }
            Rectangle {
                id: nextButton
                height: inputArea.height
                width: height
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: "../icons/next.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: parent.color
                    radius: parent.radius
                    opacity: 0.8
                    visible: !parent.enabled
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: { if (parent.enabled) parent.color = "#cfcfcf" }
                    onReleased: { parent.color = "#efefef" }
                    onClicked: {
                        findBar.find(findTextInput.text, 0)
                    }
                }
            }
        }
    }
    Rectangle {
        id: navigationBar
        color: "#efefef"
        height: 38
        z: webView.z + 1
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Row {
            height: parent.height - 6
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 3
            }
            spacing: 3
            id: controlsRow
            Rectangle {
                id: backButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                property alias enabled: webView.canGoBack

                Image {
                    anchors.centerIn: parent
                    source: "../icons/previous.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: parent.color
                    radius: parent.radius
                    opacity: 0.8
                    visible: !parent.enabled
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: { if (parent.enabled) parent.color = "#cfcfcf" }
                    onReleased: { parent.color = "#efefef" }
                    onClicked: {
                        if (parent.enabled) {
                            console.log("MiniBrowser: Going backward in session history.")
                            webView.goBack()
                            webView.forceActiveFocus()
                        }
                    }
                }
            }
            Rectangle {
                id: forwardButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                property alias enabled: webView.canGoForward

                Image {
                    anchors.centerIn: parent
                    source: "../icons/next.png"
                }

                Rectangle {
                    anchors.fill: parent
                    color: parent.color
                    radius: parent.radius
                    opacity: 0.8
                    visible: !parent.enabled
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: { if (parent.enabled) parent.color = "#cfcfcf" }
                    onReleased: { parent.color = "#efefef" }
                    onClicked: {
                        if (parent.enabled) {
                            console.log("MiniBrowser: Going forward in session history.")
                            webView.goForward()
                            webView.forceActiveFocus()
                        }
                    }
                }
            }
            Rectangle {
                id: reloadButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: webView.loading ? "../icons/stop.png" : "../icons/refresh.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: { parent.color = "#cfcfcf" }
                    onReleased: { parent.color = "#efefef" }
                    onClicked: {
                        if (webView.loading) {
                            webView.stop()
                        } else {
                            reload()
                        }
                    }
                }
            }

            Rectangle {
                id: viewportInfoButton
                height: parent.height
                width: height
                color: { viewportInfoItem.visible ? "#cfcfcf" : "#efefef" }
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: "../icons/info.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                       viewportInfoItem.visible = !viewportInfoItem.visible
                    }
                }
            }

            Rectangle {
                id: preferredMininumContentsWidthButton
                height: parent.height
                width: height
                color: "#efefef"
                opacity: { webView.experimental.preferredMinimumContentsWidth === 0 ? 0.1 : 0.6 }
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: "../icons/contents_width.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        webView.experimental.preferredMinimumContentsWidth = webView.experimental.preferredMinimumContentsWidth === 0 ? 980 : 0
                    }
                }
            }

            Rectangle {
                id: findButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    opacity: 0.6
                    source: "../icons/find.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        findBar.toggle()
                    }
                }
            }

            Rectangle {
                id: touchEventsButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    opacity: options.touchMockingEnabled ? 0.6 : 0.1
                    source: "../icons/touch.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (options.touchMockingEnabled) {
                            console.log("Touch Mocking Disabled")
                        } else {
                            console.log("Touch Mocking Enabled")
                        }

                        options.touchMockingEnabled = !options.touchMockingEnabled
                    }
                }
            }

            Rectangle {
                id: newBrowserWindowButton
                height: parent.height
                width: height
                color: "#efefef"
                radius: 6

                Image {
                    anchors.centerIn: parent
                    source: "../icons/plus.png"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        newWindow("about:blank")
                    }
                }
            }
        }
        Rectangle {
            color: "white"
            height: 26
            border.width: 1
            border.color: "#bfbfbf"
            radius: 3
            anchors {
                left: controlsRow.right
                right: parent.right
                margins: 6
                verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                }
                radius: 3
                width: parent.width / 100 * Math.max(5, webView.loadProgress)
                color: "blue"
                opacity: 0.3
                visible: webView.loading
            }
            Image {
                id: favIcon
                source: webView.icon != '' ? webView.icon : '../icons/favicon.png'
                width: 16
                height: 16
                anchors {
                    left: parent.left
                    leftMargin: 6
                    verticalCenter: parent.verticalCenter
                }
            }
            TextInput {
                id: addressLine
                clip: true
                selectByMouse: true
                horizontalAlignment: TextInput.AlignLeft
                font {
                    pointSize: 11
                    family: "Sans"
                }
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: favIcon.right
                    right: parent.right
                    margins: 6
                }

                Keys.onReturnPressed:{
                    console.log("Navigating to: ", addressLine.text)
                    load(utils.urlFromUserInput(addressLine.text))
                }

                property url url

                onUrlChanged: {
                    if (activeFocus)
                        return;

                    text = url
                    cursorPosition = 0
                }

                onActiveFocusChanged: url = webView.url
            }
        }
    }

    WebView {
        id: webView
        clip: false

        anchors {
            top: findBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        onTitleChanged: pageTitleChanged(title)
        onUrlChanged: {
            addressLine.url = webView.url

            if (options.printLoadedUrls)
                console.log("WebView url changed:", webView.url.toString());
        }

        onLoadingChanged: {
            if (!loading && loadRequest.status == WebView.LoadFailedStatus)
                webView.loadHtml("Failed to load " + loadRequest.url, "", loadRequest.url)
        }

        experimental.preferences.fullScreenEnabled: true
        experimental.preferences.webGLEnabled: true
        experimental.preferences.webAudioEnabled: true
        experimental.preferredMinimumContentsWidth: 980
        experimental.itemSelector: ItemSelector { }
        experimental.alertDialog: AlertDialog { }
        experimental.confirmDialog: ConfirmDialog { }
        experimental.promptDialog: PromptDialog { }
        experimental.authenticationDialog: AuthenticationDialog { }
        experimental.proxyAuthenticationDialog: ProxyAuthenticationDialog { }
        experimental.filePicker: FilePicker { }
        experimental.preferences.developerExtrasEnabled: true
        experimental.databaseQuotaDialog: Item {
            Timer {
                interval: 1
                running: true
                onTriggered: {
                    var size = model.expectedUsage / 1024 / 1024
                    console.log("Creating database '" + model.displayName + "' of size " + size.toFixed(2) + " MB for " + model.origin.scheme + "://" + model.origin.host + ":" + model.origin.port)
                    model.accept(model.expectedUsage)
                }
            }
        }
        experimental.colorChooser: ColorChooser { }
        experimental.onEnterFullScreenRequested : {
            navigationBar.visible = false;
            Window.showFullScreen();
        }
        experimental.onExitFullScreenRequested : {
            Window.showNormal();
            navigationBar.visible = true;
        }
    }

    ScrollIndicator {
        flickableItem: webView
    }

    ViewportInfoItem {
        id: viewportInfoItem
        anchors {
            top: navigationBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        visible: false
        test : webView.experimental.test
        preferredMinimumContentsWidth: webView.experimental.preferredMinimumContentsWidth
    }
}
