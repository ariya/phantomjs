/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 * @param {function(number, number)} callback
 * @param {number} startPosition
 * @param {number} endPosition
 * @param {number} chunkSize
 */
WebInspector.ShowMoreDataGridNode = function(callback, startPosition, endPosition, chunkSize)
{
    WebInspector.DataGridNode.call(this, {summaryRow:true}, false);
    this._callback = callback;
    this._startPosition = startPosition;
    this._endPosition = endPosition;
    this._chunkSize = chunkSize;

    this.showNext = document.createElement("button");
    this.showNext.setAttribute("type", "button");
    this.showNext.addEventListener("click", this._showNextChunk.bind(this), false);
    this.showNext.textContent = WebInspector.UIString("Show %d before", this._chunkSize);

    this.showAll = document.createElement("button");
    this.showAll.setAttribute("type", "button");
    this.showAll.addEventListener("click", this._showAll.bind(this), false);

    this.showLast = document.createElement("button");
    this.showLast.setAttribute("type", "button");
    this.showLast.addEventListener("click", this._showLastChunk.bind(this), false);
    this.showLast.textContent = WebInspector.UIString("Show %d after", this._chunkSize);

    this._updateLabels();
    this.selectable = false;
}

WebInspector.ShowMoreDataGridNode.prototype = {
    _showNextChunk: function()
    {
        this._callback(this._startPosition, this._startPosition + this._chunkSize);
    },

    _showAll: function()
    {
        this._callback(this._startPosition, this._endPosition);
    },

    _showLastChunk: function()
    {
        this._callback(this._endPosition - this._chunkSize, this._endPosition);
    },

    _updateLabels: function()
    {
        var totalSize = this._endPosition - this._startPosition;
        if (totalSize > this._chunkSize) {
            this.showNext.removeStyleClass("hidden");
            this.showLast.removeStyleClass("hidden");
        } else {
            this.showNext.addStyleClass("hidden");
            this.showLast.addStyleClass("hidden");
        }
        this.showAll.textContent = WebInspector.UIString("Show all %d", totalSize);
    },

    createCells: function()
    {
        var cell = document.createElement("td");
        if (this.depth)
            cell.style.setProperty("padding-left", (this.depth * this.dataGrid.indentWidth) + "px");
        cell.appendChild(this.showNext);
        cell.appendChild(this.showAll);
        cell.appendChild(this.showLast);
        this._element.appendChild(cell);

        var columns = this.dataGrid.columns;
        var count = 0;
        for (var c in columns)
            ++count;
        while (--count > 0) {
            cell = document.createElement("td");
            this._element.appendChild(cell);
        }
    },

    /**
     * @param {number} from
     */
    setStartPosition: function(from)
    {
        this._startPosition = from;
        this._updateLabels();
    },

    /**
     * @param {number} to
     */
    setEndPosition: function(to)
    {
        this._endPosition = to;
        this._updateLabels();
    },

    /**
     * @override
     * @return {number}
     */
    nodeHeight: function()
    {
        return 32;
    },

    dispose: function()
    {
    },

    __proto__: WebInspector.DataGridNode.prototype
}

