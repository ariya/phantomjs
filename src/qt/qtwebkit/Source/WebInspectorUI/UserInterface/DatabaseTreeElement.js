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

WebInspector.DatabaseTreeElement = function(representedObject)
{
    console.assert(representedObject instanceof WebInspector.DatabaseObject);

    WebInspector.GeneralTreeElement.call(this, WebInspector.DatabaseTreeElement.DatabaseIconStyleClassName, representedObject.name, null, representedObject, true);

    this.small = true;
    this.hasChildren = false;

    // Since we are initially telling the tree element we don't have any children, make sure that we try to populate
    // the tree element (which will get a list of tables) when the element is created.
    this.onpopulate();
};

WebInspector.DatabaseTreeElement.DatabaseIconStyleClassName = "database-icon";

WebInspector.DatabaseTreeElement.prototype = {
    constructor: WebInspector.DatabaseTreeElement,

    // Overrides from TreeElement (Private)

    oncollapse: function()
    {
        this.shouldRefreshChildren = true;
    },

    onpopulate: function()
    {
        if (this.children.length && !this.shouldRefreshChildren)
            return;

        this.shouldRefreshChildren = false;

        this.removeChildren();

        function tableNamesCallback(tableNames)
        {
            for (var i = 0; i < tableNames.length; ++i) {
                var databaseTable = new WebInspector.DatabaseTableObject(tableNames[i], this.representedObject);
                this.appendChild(new WebInspector.DatabaseTableTreeElement(databaseTable));
            }

            this.hasChildren = tableNames.length;
        }

        this.representedObject.getTableNames(tableNamesCallback.bind(this));
    }
};

WebInspector.DatabaseTreeElement.prototype.__proto__ = WebInspector.GeneralTreeElement.prototype;
