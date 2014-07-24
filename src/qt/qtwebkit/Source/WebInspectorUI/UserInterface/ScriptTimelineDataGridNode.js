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

WebInspector.ScriptTimelineDataGridNode = function(scriptTimelineRecord, baseStartTime)
{
    WebInspector.DataGridNode.call(this, {});

    this._record = scriptTimelineRecord;
    this._baseStartTime = baseStartTime || 0;
};

WebInspector.Object.addConstructorFunctions(WebInspector.ScriptTimelineDataGridNode);

WebInspector.ScriptTimelineDataGridNode.IconStyleClassName = "icon";

WebInspector.ScriptTimelineDataGridNode.prototype = {
    constructor: WebInspector.ScriptTimelineDataGridNode,

    // Public

    get record()
    {
        return this._record;
    },

    get data()
    {
        return this._record;
    },

    createCellContent: function(columnIdentifier, cell)
    {
        const emptyValuePlaceholderString = "\u2014";
        var value = this.data[columnIdentifier];

        switch (columnIdentifier) {
        case "eventType":
            return WebInspector.ScriptTimelineRecord.EventType.displayName(value);

        case "details":
            return value ? value : emptyValuePlaceholderString;

        case "resource":
            if (!value)
                return emptyValuePlaceholderString;

            console.assert(this.data.sourceCodeLocation);

            cell.classList.add(WebInspector.ResourceTreeElement.ResourceIconStyleClassName);
            cell.classList.add(value.type);

            // Give the whole cell a tooltip and keep it up to date.
            var sourceCodeLocation = this.data.sourceCodeLocation;
            sourceCodeLocation.populateLiveDisplayLocationTooltip(cell);

            var fragment = document.createDocumentFragment();

            var goToArrowButtonLink = WebInspector.createSourceCodeLocationLink(sourceCodeLocation, false, true);
            fragment.appendChild(goToArrowButtonLink);

            var icon = document.createElement("div");
            icon.className = WebInspector.ScriptTimelineDataGridNode.IconStyleClassName;
            fragment.appendChild(icon);

            var titleElement = document.createElement("span");
            sourceCodeLocation.populateLiveDisplayLocationString(titleElement, "textContent");
            fragment.appendChild(titleElement);

            return fragment;

        case "startTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value - this._baseStartTime);

        case "duration":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value);
        }

        return WebInspector.DataGridNode.prototype.createCellContent.call(this, columnIdentifier);
    }
}

WebInspector.ScriptTimelineDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
