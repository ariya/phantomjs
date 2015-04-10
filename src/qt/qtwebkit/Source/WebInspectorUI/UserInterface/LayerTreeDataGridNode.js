/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.LayerTreeDataGridNode = function(layer)
{
    WebInspector.DataGridNode.call(this);

    this._outlets = {};

    this.layer = layer;
};

WebInspector.LayerTreeDataGridNode.prototype = {
    constructor: WebInspector.DataGridNode,

    // DataGridNode Overrides.

    createCells: function()
    {
        WebInspector.DataGridNode.prototype.createCells.call(this);

        this._cellsWereCreated = true;
    },

    createCellContent: function(columnIdentifier, cell)
    {
        var cell = columnIdentifier === "name" ? this._makeNameCell() : this._makeOutlet(columnIdentifier, document.createTextNode());
        this._updateCell(columnIdentifier);
        return cell;
    },

    // Public

    get layer()
    {
        return this._layer;
    },

    set layer(layer)
    {
        this._layer = layer;

        var domNode = WebInspector.domTreeManager.nodeForId(layer.nodeId);

        this.data = {
            name: domNode ? WebInspector.displayNameForNode(domNode) : WebInspector.UIString("Unknown node"),
            paintCount: layer.paintCount || "\u2014",
            memory: Number.bytesToString(layer.memory || 0)
        };
    },

    get data()
    {
        return this._data;
    },

    set data(data)
    {
        Object.keys(data).forEach(function(columnIdentifier) {
            if (this._data[columnIdentifier] === data[columnIdentifier])
                return;

            this._data[columnIdentifier] = data[columnIdentifier];
            if (this._cellsWereCreated)
                this._updateCell(columnIdentifier);
        }, this);
    },

    // Private

    _makeOutlet: function(name, element)
    {
        return this._outlets[name] = element;
    },

    _makeNameCell: function()
    {
        var fragment = document.createDocumentFragment();

        fragment.appendChild(document.createElement("img")).className = "icon";

        var label = this._makeOutlet("label", fragment.appendChild(document.createElement("div")));
        label.className = "label";
    
        var nameLabel = this._makeOutlet("nameLabel", label.appendChild(document.createElement("span")));
        nameLabel.className = "name";

        var pseudoLabel = this._makeOutlet("pseudoLabel", document.createElement("span"));
        pseudoLabel.className = "pseudo-element";

        var reflectionLabel = this._makeOutlet("reflectionLabel", document.createElement("span"));
        reflectionLabel.className = "reflection";
        reflectionLabel.textContent = " \u2014 " + WebInspector.UIString("Reflection");

        var goToButton = this._makeOutlet("goToButton", fragment.appendChild(WebInspector.createGoToArrowButton()));
        goToButton.addEventListener("click", this._goToArrowWasClicked.bind(this), false);
        
        return fragment;
    },

    _updateCell: function(columnIdentifier)
    {
        var data = this._data[columnIdentifier];
        if (columnIdentifier === "name")
            this._updateNameCellData(data);
        else
            this._outlets[columnIdentifier].textContent = data;
    },

    _updateNameCellData: function(data)
    {
        var layer = this._layer;
        var label = this._outlets.label;

        this._outlets.nameLabel.textContent = data;

        if (layer.pseudoElement)
            label.appendChild(this._outlets.pseudoLabel).textContent = "::" + layer.pseudoElement;
        else if (this._outlets.pseudoLabel.parentNode)
            label.removeChild(this._outlets.pseudoLabel);

        if (layer.isReflection)
            label.appendChild(this._outlets.reflectionLabel);
        else if (this._outlets.reflectionLabel.parentNode)
            label.removeChild(this._outlets.reflectionLabel);

        if (WebInspector.domTreeManager.nodeForId(layer.nodeId))
            label.parentNode.appendChild(this._outlets.goToButton);
        else if (this._outlets.goToButton.parentNode)
            label.parentNode.removeChild(this._outlets.goToButton);

        var element = this.element;
        if (layer.isReflection)
            element.classList.add("reflection");
        else if (layer.pseudoElement)
            element.classList.add("pseudo-element");
    },

    _goToArrowWasClicked: function()
    {
        var domNode = WebInspector.domTreeManager.nodeForId(this._layer.nodeId);
        WebInspector.resourceSidebarPanel.showMainFrameDOMTree(domNode, true);
    }
};

WebInspector.LayerTreeDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
