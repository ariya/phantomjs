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

WebInspector.LayoutTimelineDataGrid = function(columns)
{
    WebInspector.TimelineDataGrid.call(this, columns);

    this._showingHighlight = false;

    this.addEventListener(WebInspector.DataGrid.Event.SelectedNodeChanged, this._layoutDataGridSelectedNodeChanged, this);
}

WebInspector.LayoutTimelineDataGrid.prototype = {
    constructor: WebInspector.LayoutTimelineDataGrid,

    // Protected

    reset: function()
    {
        WebInspector.TimelineDataGrid.prototype.reset.call(this);

        this._hideHighlightIfNeeded();
    },

    callFramePopoverAnchorElement: function()
    {
        return this.selectedNode.elementWithColumnIdentifier("initiatorCallFrame");
    },

    hidden: function()
    {
        WebInspector.TimelineDataGrid.prototype.hidden.call(this);

        this._hideHighlightIfNeeded();
    },

    // Private

    _layoutDataGridSelectedNodeChanged: function(event)
    {
        if (!this.selectedNode) {
            this._hideHighlightIfNeeded();
            return;
        }

        var record = this.selectedNode.record;
        const contentColor = {r: 111, g: 168, b: 220, a: 0.66};
        const outlineColor = {r: 255, g: 229, b: 153, a: 0.66};

        var quad = record.quad;
        if (quad && DOMAgent.highlightQuad) {
            DOMAgent.highlightQuad(quad.toProtocol(), contentColor, outlineColor);
            this._showingHighlight = true;
            return;
        }

        // COMPATIBILITY (iOS 6): iOS 6 included Rect information instead of Quad information. Fallback to highlighting the rect.
        var rect = record.rect;
        if (rect) {
            DOMAgent.highlightRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, contentColor, outlineColor);
            this._showingHighlight = true;
            return;
        }
    },

    _hideHighlightIfNeeded: function()
    {
        if (this._showingHighlight) {
            DOMAgent.hideHighlight();
            this._showingHighlight = false;
        }
    }
}

WebInspector.LayoutTimelineDataGrid.prototype.__proto__ = WebInspector.TimelineDataGrid.prototype;
