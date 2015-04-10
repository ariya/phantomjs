/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

MouseArea {
    anchors.fill: parent
    onClicked: model.reject()

    Rectangle {
        id: dialog
        property int spacing: 10

        color: "gainsboro"
        opacity: 0.8
        radius: 5
        width: 200
        height: 200
        x: (model.elementRect.x + width > parent.width) ? parent.width - width : model.elementRect.x
        y: (model.elementRect.y + model.elementRect.height + height < parent.height ) ? model.elementRect.y + model.elementRect.height
                                                                                      : model.elementRect.y - height;

        Rectangle {
            color: "red"
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: dialog.spacing
            width: parent.width / 2 - dialog.spacing
            height: parent.height / 2 - dialog.spacing

            MouseArea {
                anchors.fill: parent
                onClicked: model.accept(parent.color)
            }
        }

        Rectangle {
            color: "blue"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: dialog.spacing
            width: parent.width / 2 - dialog.spacing
            height: parent.height / 2 - dialog.spacing

            MouseArea {
                anchors.fill: parent
                onClicked: model.accept(parent.color)
            }
        }

        Rectangle {
            color: "green"
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: dialog.spacing
            width: parent.width / 2 - dialog.spacing
            height: parent.height / 2 - dialog.spacing

            MouseArea {
                anchors.fill: parent
                onClicked: model.accept(parent.color)
            }
        }

        Rectangle {
            color: model.currentColor
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: dialog.spacing * 2
            width: parent.width / 2 - dialog.spacing * 4
            height: parent.height / 2 - dialog.spacing * 4

            Text {
                text: "Current"
                anchors.bottom: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
