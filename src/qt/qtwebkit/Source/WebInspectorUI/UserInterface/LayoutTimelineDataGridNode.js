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

WebInspector.LayoutTimelineDataGridNode = function(layoutTimelineRecord, baseStartTime)
{
    WebInspector.DataGridNode.call(this, {});

    this._record = layoutTimelineRecord;
    this._baseStartTime = baseStartTime || 0;
};

WebInspector.Object.addConstructorFunctions(WebInspector.LayoutTimelineDataGridNode);

WebInspector.LayoutTimelineDataGridNode.IconStyleClassName = "icon";
WebInspector.LayoutTimelineDataGridNode.SubtitleStyleClassName = "subtitle";

WebInspector.LayoutTimelineDataGridNode.prototype = {
    constructor: WebInspector.LayoutTimelineDataGridNode,

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
            return WebInspector.LayoutTimelineRecord.EventType.displayName(value);

        case "initiatorCallFrame":
            var callFrame = value;
            if (!callFrame)
                return emptyValuePlaceholderString;

            var isAnonymousFunction = false;
            var functionName = callFrame.functionName;
            if (!functionName) {
                functionName = WebInspector.UIString("(anonymous function)");
                isAnonymousFunction = true;
            }

            cell.classList.add(WebInspector.CallFrameTreeElement.FunctionIconStyleClassName);

            var fragment = document.createDocumentFragment();

            if (callFrame.sourceCodeLocation && callFrame.sourceCodeLocation.sourceCode) {

                // Give the whole cell a tooltip and keep it up to date.
                callFrame.sourceCodeLocation.populateLiveDisplayLocationTooltip(cell);

                var goToArrowButtonLink = WebInspector.createSourceCodeLocationLink(callFrame.sourceCodeLocation, false, true);
                fragment.appendChild(goToArrowButtonLink);

                var icon = document.createElement("div");
                icon.className = WebInspector.LayoutTimelineDataGridNode.IconStyleClassName;
                fragment.appendChild(icon);

                if (isAnonymousFunction) {
                    // For anonymous functions we show the resource or script icon and name.
                    if (callFrame.sourceCodeLocation.sourceCode instanceof WebInspector.Resource) {
                        cell.classList.add(WebInspector.ResourceTreeElement.ResourceIconStyleClassName);
                        cell.classList.add(callFrame.sourceCodeLocation.sourceCode.type);
                    } else if (callFrame.sourceCodeLocation.sourceCode instanceof WebInspector.Script) {
                        if (callFrame.sourceCodeLocation.sourceCode.url) {
                            cell.classList.add(WebInspector.ResourceTreeElement.ResourceIconStyleClassName);
                            cell.classList.add(WebInspector.Resource.Type.Script);
                        } else
                            cell.classList.add(WebInspector.ScriptTreeElement.AnonymousScriptIconStyleClassName);
                    } else {
                        console.error("Unknown SourceCode subclass.");
                    }

                    var titleElement = document.createElement("span");
                    callFrame.sourceCodeLocation.populateLiveDisplayLocationString(titleElement, "textContent");

                    fragment.appendChild(titleElement);
                } else {
                    // Show the function name and icon.
                    cell.classList.add(WebInspector.CallFrameTreeElement.FunctionIconStyleClassName);

                    fragment.appendChild(document.createTextNode(functionName));

                    var subtitleElement = document.createElement("span");
                    subtitleElement.className = WebInspector.LayoutTimelineDataGridNode.SubtitleStyleClassName;
                    callFrame.sourceCodeLocation.populateLiveDisplayLocationString(subtitleElement, "textContent");

                    fragment.appendChild(subtitleElement);
                }

                return fragment;
            }

            var icon = document.createElement("div");
            icon.className = WebInspector.LayoutTimelineDataGridNode.IconStyleClassName;
            fragment.appendChild(icon);

            fragment.appendChild(document.createTextNode(functionName));

            return fragment;

        case "width":
        case "height":
            return isNaN(value) ? emptyValuePlaceholderString : WebInspector.UIString("%fpx").format(value);

        case "area":
            return isNaN(value) ? emptyValuePlaceholderString : WebInspector.UIString("%fpx²").format(value);

        case "startTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value - this._baseStartTime);

        case "duration":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value);
        }

        return WebInspector.DataGridNode.prototype.createCellContent.call(this, columnIdentifier);
    }
}

WebInspector.LayoutTimelineDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
