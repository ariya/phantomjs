/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.ResourceWebSocketFrameView = function(resource)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("resource-websocket");
    this.resource = resource;
    this.element.removeChildren();

    var dataGrid = new WebInspector.DataGrid([
        {id: "data", title: WebInspector.UIString("Data"), sortable: false},
        {id: "length", title: WebInspector.UIString("Length"), sortable: false, alig: WebInspector.DataGrid.Align.Right, width: "50px"},
        {id: "time", title: WebInspector.UIString("Time"), width: "70px"}
    ]);

    var frames = this.resource.frames();
    for (var i = 0; i < frames.length; i++) {
        var payload = frames[i];

        var date = new Date(payload.time * 1000);
        var row = {
            data: "",
            length: typeof payload.payloadData === "undefined" ? payload.errorMessage.length.toString() : payload.payloadData.length.toString(),
            time: date.toLocaleTimeString()
        };

        var rowClass = "";
        if (payload.errorMessage) {
            rowClass = "error";
            row.data = payload.errorMessage;
        } else if (payload.opcode == WebInspector.ResourceWebSocketFrameView.OpCodes.TextFrame) {
            if (payload.sent)
                rowClass = "outcoming";

            row.data = payload.payloadData;
        } else {
            rowClass = "opcode";
            var opcodeMeaning = "";
            switch (payload.opcode) {
            case WebInspector.ResourceWebSocketFrameView.OpCodes.ContinuationFrame:
                opcodeMeaning = WebInspector.UIString("Continuation Frame");
                break;
            case WebInspector.ResourceWebSocketFrameView.OpCodes.BinaryFrame:
                opcodeMeaning = WebInspector.UIString("Binary Frame");
                break;
            case WebInspector.ResourceWebSocketFrameView.OpCodes.ConnectionCloseFrame:
                opcodeMeaning = WebInspector.UIString("Connection Close Frame");
                break;
            case WebInspector.ResourceWebSocketFrameView.OpCodes.PingFrame:
                opcodeMeaning = WebInspector.UIString("Ping Frame");
                break;
            case WebInspector.ResourceWebSocketFrameView.OpCodes.PongFrame:
                opcodeMeaning = WebInspector.UIString("Pong Frame");
                break;
            }
            row.data = WebInspector.UIString("%s (Opcode %d%s)", opcodeMeaning, payload.opcode, (payload.mask ? ", mask" : ""));
        }

        var node = new WebInspector.DataGridNode(row, false);
        dataGrid.rootNode().appendChild(node);

        if (rowClass)
            node.element.classList.add("resource-websocket-row-" + rowClass);

    }
    dataGrid.show(this.element);
}

WebInspector.ResourceWebSocketFrameView.OpCodes = {
    ContinuationFrame: 0,
    TextFrame: 1,
    BinaryFrame: 2,
    ConnectionCloseFrame: 8,
    PingFrame: 9,
    PongFrame: 10
};

WebInspector.ResourceWebSocketFrameView.prototype = {
    __proto__: WebInspector.View.prototype
}
