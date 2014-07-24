/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 1.0

/*!
    \qmltype TabWidget
    \inqmlmodule UIComponents 1.0
    \brief A widget that places its children as tabs

    A TabWidget places its children as tabs in a view. Selecting
    a tab involves selecting the tab at the top.

    The TabWidget component is part of the \l {UI Components} module.

    This documentation is part of the \l{componentset}{UIComponents} example.

    \section1 Adding Tabs

    To add a tab, declare the tab as a child of the TabWidget.

    \code
    TabWidget {
        id: tabwidget

        Rectangle {
            id: tab1
            color: "red"
            //... omitted
        }
        Rectangle {
            id: tab2
            color: "blue"
            //... omitted
        }

    }
    \endcode

*/
Item {
    id: tabWidget

    /*!
    \internal

    Setting the default property to stack.children means any child items
    of the TabWidget are actually added to the 'stack' item's children.

    See the \l{"Property Binding in QML"}
    documentation for details on default properties.

    This is an implementation detail, not meant for public knowledge. Putting
    the \internal command at the beginning will cause QDoc to not publish this
    documentation in the public API page.

    Normally, a property alias needs to have a
    "\qmlproperty <type> <propertyname>" to assign the alias a type.

    */
    default property alias content: stack.children


    /*!
        The currently active tab in the TabWidget.
    */
    property int current: 0

    /*!
        A sample \c{read-only} property.
        A contrived property to demonstrate QDoc's ability to detect
        read-only properties.

        The signature is:
        \code
        readonly property int sampleReadOnlyProperty: 0
        \endcode

        Note that the property must be initialized to a value.

    */
    readonly property int sampleReadOnlyProperty: 0

    /*!
    \internal

    This handler is an implementation
    detail. The \c{\internal} command will prevent QDoc from publishing this
    documentation on the public API.
    */
    onCurrentChanged: setOpacities()
    Component.onCompleted: setOpacities()

    /*!
    \internal

    An internal function to set the opacity.
    The \internal command will prevent QDoc from publishing this
    documentation on the public API.
    */
    function setOpacities() {
        for (var i = 0; i < stack.children.length; ++i) {
            stack.children[i].opacity = (i == current ? 1 : 0)
        }
    }

    Row {
        id: header

        Repeater {
            model: stack.children.length
            delegate: Rectangle {
                width: tabWidget.width / stack.children.length; height: 36

                Rectangle {
                    width: parent.width; height: 1
                    anchors { bottom: parent.bottom; bottomMargin: 1 }
                    color: "#acb2c2"
                }
                BorderImage {
                    anchors { fill: parent; leftMargin: 2; topMargin: 5; rightMargin: 1 }
                    border { left: 7; right: 7 }
                    source: "tab.png"
                    visible: tabWidget.current == index
                }
                Text {
                    horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter
                    anchors.fill: parent
                    text: stack.children[index].title
                    elide: Text.ElideRight
                    font.bold: tabWidget.current == index
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: tabWidget.current = index
                }
            }
        }
    }

    Item {
        id: stack
        width: tabWidget.width
        anchors.top: header.bottom; anchors.bottom: tabWidget.bottom
    }
}
