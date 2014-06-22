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

WebInspector.DetailsSectionDataGridRow = function(dataGrid, emptyMessage) {
    WebInspector.DetailsSectionRow.call(this, emptyMessage);

    this.element.classList.add(WebInspector.DetailsSectionDataGridRow.StyleClassName);

    this.dataGrid = dataGrid;
};

WebInspector.DetailsSectionDataGridRow.StyleClassName = "data-grid";

WebInspector.DetailsSectionDataGridRow.prototype = {
    constructor: WebInspector.DetailsSectionDataGridRow,

    // Public

    get dataGrid()
    {
        return this._dataGrid;
    },

    set dataGrid(dataGrid)
    {
        if (this._dataGrid === dataGrid)
            return;

        this._dataGrid = dataGrid || null;

        if (dataGrid) {
            dataGrid.element.classList.add("inline");

            this.hideEmptyMessage();
            this.element.appendChild(dataGrid.element);
        } else
            this.showEmptyMessage();
    }
};

WebInspector.DetailsSectionDataGridRow.prototype.__proto__ = WebInspector.DetailsSectionRow.prototype;
