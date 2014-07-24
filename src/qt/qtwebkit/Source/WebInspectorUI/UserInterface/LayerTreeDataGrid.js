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

WebInspector.LayerTreeDataGrid = function() {
    WebInspector.DataGrid.call(this, {
        name: { title: WebInspector.UIString("Node"), sortable: false },
        paintCount: { title: WebInspector.UIString("Paints"), sortable: true, aligned: "right", width: "50px" },
        memory: { title: WebInspector.UIString("Memory"), sortable: true, sort: "descending", aligned: "right", width: "70px" }
    });
};

WebInspector.LayerTreeDataGrid.prototype = {
    constructor: WebInspector.LayerTreeDataGrid,
    
    insertChild: function(child)
    {
        WebInspector.DataGrid.prototype.insertChild.call(this, child);

        this._updateChildren();
    },
    
    removeChild: function(child)
    {
        WebInspector.DataGrid.prototype.removeChild.call(this, child);

        this._updateChildren();
    },
    
    setChildren: function(children)
    {
        this._suspendLayout = true;

        var removedChildren = this.children.filter(function(child) {
            return !children.contains(child);
        });

        while (removedChildren.length)
            this.removeChild(removedChildren.pop());

        children.forEach(function(child) {
            if (child.parent !== this)
                this.appendChild(child);
        }.bind(this));

        this._suspendLayout = false;
        
        this.children = children;
        this._updateChildren();
    },
    
    _updateChildren: function()
    {
        if (this._suspendLayout)
            return;

        // Iterate through nodes by DOM order first so we can establish
        // the DOM index.
        var elements = this.dataTableBody.rows;
        for (var i = 0, count = elements.length - 1; i < count; ++i)
            elements[i]._dataGridNode._domIndex = i;

        // Now iterate through children to set up their sibling relationship
        // and update the element style to offset the position of the node
        // to match its position in the children list vs. DOM order.
        var children = this.children;
        for (var i = 0, count = children.length; i < count; ++i) {
            var child = children[i];
            child.previousSibling = i > 0 ? children[i - 1] : null;
            child.nextSibling = i + 1 < count ? children[i + 1] : null;
            
            var ty = (i - child._domIndex) * 100;
            child.element.style.webkitTransform = "translateY(" + ty + "%)";
        }

        this.hasChildren = this.children.length > 0;
    },
    
    _recalculateSiblings: function(myIndex)
    {
        // Overriding default implementation to do nothing at all
        // since we're setting sibling relationship in _updateChildren.
    }
};

WebInspector.LayerTreeDataGrid.prototype.__proto__ = WebInspector.DataGrid.prototype;
