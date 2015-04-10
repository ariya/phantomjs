/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
    // To avoid conflicting with ListView.model when inside ListView context.
    property QtObject selectorModel: model
    anchors.fill: parent
    onClicked: selectorModel.reject()

    Rectangle {
        clip: true
        width: 200
        height: Math.min(listView.contentItem.height + listView.anchors.topMargin + listView.anchors.bottomMargin
                         , Math.max(selectorModel.elementRect.y, parent.height - selectorModel.elementRect.y - selectorModel.elementRect.height))
        x: (selectorModel.elementRect.x + 200 > parent.width) ? parent.width - 200 : selectorModel.elementRect.x
        y: (selectorModel.elementRect.y + selectorModel.elementRect.height + height < parent.height ) ? selectorModel.elementRect.y + selectorModel.elementRect.height
                                                         : selectorModel.elementRect.y - height;
        radius: 5
        color: "gainsboro"
        opacity: 0.8

        ListView {
            id: listView
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5
            model: selectorModel.items

            delegate: Rectangle {
                color: model.selected ? "gold" : "silver"
                height: 50
                width: parent.width

                Text {
                    anchors.centerIn: parent
                    text: model.text
                    color: model.enabled ? "black" : "gainsboro"
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: model.enabled
                    onClicked: selectorModel.accept(model.index)
                }
            }

            section.property: "group"
            section.delegate: Rectangle {
                height: 30
                width: parent.width
                color: "silver"
                Text {
                    anchors.centerIn: parent
                    text: section
                    font.bold: true
                }
            }
        }
    }
}
