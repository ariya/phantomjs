import QtQuick 2.0
import QtWebKit 3.0

Item {
    property var test
    property var preferredMinimumContentsWidth

    function formatScale(value) {
        return "<b>" + parseFloat(value.toFixed(4)) + "</b>x";
    }

    function formatSize(value) {
        return "<b>" + value.width.toFixed() + "x" + value.height.toFixed() + "</b>px"
    }

    function formatBool(value) {
        return "<b>" + (value ? "yes" : "no") + "</b>"
    }

    Rectangle {
        id: title;

        anchors {
            top: parent.top
            left: parent.left
            margins: 10
        }

        height: 50
        width: 250

        color: "blue"

        Text {
            id: viewportInfoLabel

            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 10
            }

            text: "Viewport Info"
            color: "white"
            font.family: "Nokia Pure"
            font.pointSize: 24
        }
    }

    Rectangle {
        color: "gray"
        opacity: 0.9

        anchors {
            top: title.bottom
            left: title.left
            topMargin: 10
        }

        width: 340
        height: 270

        Item {
            id: textBox

            anchors {
                fill: parent
                margins: 10
            }

            property string fontFamily: "Nokia Pure"
            property color fontColor: "black"

            Column {
                anchors.fill: parent
                spacing: 20
                Column {
                    Text {
                        text: "Current scale: " + formatScale(test.contentsScale)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                }

                Column {
                    Text {
                        text: "Minimum scale: " + formatScale(test.viewport.minimumScale)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                    Text {
                        text: "Maximum scale: " + formatScale(test.viewport.maximumScale)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                }

                Column {
                    Text {
                        text: "Device pixel ratio: " + formatScale(test.devicePixelRatio)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                    Text {
                        text: "Contents size: " + formatSize(test.contentsSize)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                    Text {
                        text: "Viewport layout size: " + formatSize(test.viewport.layoutSize)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                }

                Column {
                    Text {
                        text: "Adapt for small screens: " + formatBool(preferredMinimumContentsWidth)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                    Text {
                        text: "Allows scaling: " + formatBool(test.viewport.isScalable)
                        font.family: textBox.fontFamily
                        color: textBox.fontColor
                    }
                }
            }
        }
    }
}
