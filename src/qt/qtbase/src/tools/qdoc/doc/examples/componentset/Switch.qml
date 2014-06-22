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
    \qmltype ToggleSwitch
    \inqmlmodule UIComponents 1.0
    \brief A component that can be turned on or off

    A toggle switch has two states: an \c on and an \c off state. The \c off
    state is when the \l on property is set to \c false.

    The ToggleSwitch component is part of the \l {UI Components} module.

    This documentation is part of the \l{componentset}{UIComponents} example.

*/
Item {
    id: toggleswitch
    width: background.width; height: background.height

    /*!
       Indicates the state of the switch. If \c false, then the switch is in
       the \c off state.

       \omit
            The \qmlproperty <type> <propertyname> is not necessary as QDoc
            will associate this property to the ToggleSwitch

           QDoc will not publish the documentation within omit and endomit.
       \endomit
    */
    property bool on: false


    /*!
        A method to toggle the switch. If the switch is \c on, the toggling it
        will turn it \c off. Toggling a switch in the \c off position will
        turn it \c on.
    */
    function toggle() {
        if (toggleswitch.state == "on")
            toggleswitch.state = "off";
        else
            toggleswitch.state = "on";
    }


    /*!
        \internal

        An internal function to synchronize the switch's internals. This
        function is not for public access. The \internal command will
        prevent QDoc from publishing this comment in the public API.
    */
    function releaseSwitch() {
        if (knob.x == 1) {
            if (toggleswitch.state == "off") return;
        }
        if (knob.x == 78) {
            if (toggleswitch.state == "on") return;
        }
        toggle();
    }

    Rectangle {
        id: background
        width: 130; height: 48
        radius: 48
        color: "lightsteelblue"
        MouseArea { anchors.fill: parent; onClicked: toggle() }
    }

    Rectangle {
        id: knob
        width: 48; height: 48
        radius: width
        color: "lightblue"

        MouseArea {
            anchors.fill: parent
            drag.target: knob; drag.axis: Drag.XAxis; drag.minimumX: 1; drag.maximumX: 78
            onClicked: toggle()
            onReleased: releaseSwitch()
        }
    }

    states: [
        State {
            name: "on"
            PropertyChanges { target: knob; x: 78 }
            PropertyChanges { target: toggleswitch; on: true }
        },
        State {
            name: "off"
            PropertyChanges { target: knob; x: 1 }
            PropertyChanges { target: toggleswitch; on: false }
        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "x"; easing.type: Easing.InOutQuad; duration: 200 }
    }
}
