/*
 * Copyright (C) IBM Corp. 2009  All rights reserved.
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
 *     * Neither the name of IBM Corp. nor the names of its
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
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.WatchExpressionsSidebarPane = function()
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Watch Expressions"));

    this.section = new WebInspector.WatchExpressionsSection();
    this.bodyElement.appendChild(this.section.element);

    var refreshButton = document.createElement("button");
    refreshButton.className = "pane-title-button refresh";
    refreshButton.addEventListener("click", this._refreshButtonClicked.bind(this), false);
    refreshButton.title = WebInspector.UIString("Refresh");
    this.titleElement.appendChild(refreshButton);

    var addButton = document.createElement("button");
    addButton.className = "pane-title-button add";
    addButton.addEventListener("click", this._addButtonClicked.bind(this), false);
    this.titleElement.appendChild(addButton);
    addButton.title = WebInspector.UIString("Add watch expression");

    this._requiresUpdate = true;
}

WebInspector.WatchExpressionsSidebarPane.prototype = {
    wasShown: function()
    {
        this._refreshExpressionsIfNeeded();
    },

    reset: function()
    {
        this.refreshExpressions();
    },

    refreshExpressions: function()
    {
        this._requiresUpdate = true;
        this._refreshExpressionsIfNeeded();
    },

    addExpression: function(expression)
    {
        this.section.addExpression(expression);
        this.expand();
    },

    _refreshExpressionsIfNeeded: function()
    {
        if (this._requiresUpdate && this.isShowing()) {
            this.section.update();
            delete this._requiresUpdate;
        } else
            this._requiresUpdate = true;
    },

    _addButtonClicked: function(event)
    {
        event.consume();
        this.expand();
        this.section.addNewExpressionAndEdit();
    },

    _refreshButtonClicked: function(event)
    {
        event.consume();
        this.refreshExpressions();
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ObjectPropertiesSection}
 */
WebInspector.WatchExpressionsSection = function()
{
    this._watchObjectGroupId = "watch-group";

    WebInspector.ObjectPropertiesSection.call(this, WebInspector.RemoteObject.fromPrimitiveValue(""));

    this.treeElementConstructor = WebInspector.WatchedPropertyTreeElement;
    this._expandedExpressions = {};
    this._expandedProperties = {};

    this.emptyElement = document.createElement("div");
    this.emptyElement.className = "info";
    this.emptyElement.textContent = WebInspector.UIString("No Watch Expressions");

    this.watchExpressions = WebInspector.settings.watchExpressions.get();

    this.headerElement.className = "hidden";
    this.editable = true;
    this.expanded = true;
    this.propertiesElement.addStyleClass("watch-expressions");

    this.element.addEventListener("mousemove", this._mouseMove.bind(this), true);
    this.element.addEventListener("mouseout", this._mouseOut.bind(this), true);
    this.element.addEventListener("dblclick", this._sectionDoubleClick.bind(this), false);
    this.emptyElement.addEventListener("contextmenu", this._emptyElementContextMenu.bind(this), false);
}

WebInspector.WatchExpressionsSection.NewWatchExpression = "\xA0";

WebInspector.WatchExpressionsSection.prototype = {
    update: function(e)
    {
        if (e)
            e.consume();

        function appendResult(expression, watchIndex, result, wasThrown)
        {
            if (!result)
                return;

            var property = new WebInspector.RemoteObjectProperty(expression, result);
            property.watchIndex = watchIndex;
            property.wasThrown = wasThrown;

            // To clarify what's going on here:
            // In the outer function, we calculate the number of properties
            // that we're going to be updating, and set that in the
            // propertyCount variable.
            // In this function, we test to see when we are processing the
            // last property, and then call the superclass's updateProperties()
            // method to get all the properties refreshed at once.
            properties.push(property);

            if (properties.length == propertyCount) {
                this.updateProperties(properties, [], WebInspector.WatchExpressionTreeElement, WebInspector.WatchExpressionsSection.CompareProperties);

                // check to see if we just added a new watch expression,
                // which will always be the last property
                if (this._newExpressionAdded) {
                    delete this._newExpressionAdded;

                    var treeElement = this.findAddedTreeElement();
                    if (treeElement)
                        treeElement.startEditing();
                }

                // Force displaying delete button for hovered element.
                if (this._lastMouseMovePageY)
                    this._updateHoveredElement(this._lastMouseMovePageY);
            }
        }

        // TODO: pass exact injected script id.
        RuntimeAgent.releaseObjectGroup(this._watchObjectGroupId)
        var properties = [];

        // Count the properties, so we known when to call this.updateProperties()
        // in appendResult()
        var propertyCount = 0;
        for (var i = 0; i < this.watchExpressions.length; ++i) {
            if (!this.watchExpressions[i])
                continue;
            ++propertyCount;
        }

        // Now process all the expressions, since we have the actual count,
        // which is checked in the appendResult inner function.
        for (var i = 0; i < this.watchExpressions.length; ++i) {
            var expression = this.watchExpressions[i];
            if (!expression)
                continue;

            WebInspector.runtimeModel.evaluate(expression, this._watchObjectGroupId, false, true, false, false, appendResult.bind(this, expression, i));
        }

        if (!propertyCount) {
            if (!this.emptyElement.parentNode)
                this.element.appendChild(this.emptyElement);
        } else {
            if (this.emptyElement.parentNode)
                this.element.removeChild(this.emptyElement);
        }

        // note this is setting the expansion of the tree, not the section;
        // with no expressions, and expanded tree, we get some extra vertical
        // white space
        this.expanded = (propertyCount != 0);
    },

    addExpression: function(expression)
    {
        this.watchExpressions.push(expression);
        this.saveExpressions();
        this.update();
    },

    addNewExpressionAndEdit: function()
    {
        this._newExpressionAdded = true;
        this.watchExpressions.push(WebInspector.WatchExpressionsSection.NewWatchExpression);
        this.update();
    },

    _sectionDoubleClick: function(event)
    {
        if (event.target !== this.element && event.target !== this.propertiesElement && event.target !== this.emptyElement)
            return;
        event.consume();
        this.addNewExpressionAndEdit();
    },

    updateExpression: function(element, value)
    {
        if (value === null) {
            var index = element.property.watchIndex;
            this.watchExpressions.splice(index, 1);
        }
        else
            this.watchExpressions[element.property.watchIndex] = value;
        this.saveExpressions();
        this.update();
    },
    
    _deleteAllExpressions: function()
    {
        this.watchExpressions = [];
        this.saveExpressions();
        this.update();
    },

    findAddedTreeElement: function()
    {
        var children = this.propertiesTreeOutline.children;
        for (var i = 0; i < children.length; ++i) {
            if (children[i].property.name === WebInspector.WatchExpressionsSection.NewWatchExpression)
                return children[i];
        }
    },

    saveExpressions: function()
    {
        var toSave = [];
        for (var i = 0; i < this.watchExpressions.length; i++)
            if (this.watchExpressions[i])
                toSave.push(this.watchExpressions[i]);

        WebInspector.settings.watchExpressions.set(toSave);
        return toSave.length;
    },

    _mouseMove: function(e)
    {
        if (this.propertiesElement.firstChild)
            this._updateHoveredElement(e.pageY);
    },

    _mouseOut: function()
    {
        if (this._hoveredElement) {
            this._hoveredElement.removeStyleClass("hovered");
            delete this._hoveredElement;
        }
        delete this._lastMouseMovePageY;
    },

    _updateHoveredElement: function(pageY)
    {
        var candidateElement = this.propertiesElement.firstChild;
        while (true) {
            var next = candidateElement.nextSibling;
            while (next && !next.clientHeight)
                next = next.nextSibling;
            if (!next || next.totalOffsetTop() > pageY)
                break;
            candidateElement = next;
        }

        if (this._hoveredElement !== candidateElement) {
            if (this._hoveredElement)
                this._hoveredElement.removeStyleClass("hovered");
            if (candidateElement)
                candidateElement.addStyleClass("hovered");
            this._hoveredElement = candidateElement;
        }

        this._lastMouseMovePageY = pageY;
    },

    _emptyElementContextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add watch expression" : "Add Watch Expression"), this.addNewExpressionAndEdit.bind(this));
        contextMenu.show();
    },

    __proto__: WebInspector.ObjectPropertiesSection.prototype
}

WebInspector.WatchExpressionsSection.CompareProperties = function(propertyA, propertyB)
{
    if (propertyA.watchIndex == propertyB.watchIndex)
        return 0;
    else if (propertyA.watchIndex < propertyB.watchIndex)
        return -1;
    else
        return 1;
}

/**
 * @constructor
 * @extends {WebInspector.ObjectPropertyTreeElement}
 * @param {WebInspector.RemoteObjectProperty} property
 */
WebInspector.WatchExpressionTreeElement = function(property)
{
    WebInspector.ObjectPropertyTreeElement.call(this, property);
}

WebInspector.WatchExpressionTreeElement.prototype = {
    onexpand: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.onexpand.call(this);
        this.treeOutline.section._expandedExpressions[this._expression()] = true;
    },

    oncollapse: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.oncollapse.call(this);
        delete this.treeOutline.section._expandedExpressions[this._expression()];
    },

    onattach: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.onattach.call(this);
        if (this.treeOutline.section._expandedExpressions[this._expression()])
            this.expanded = true;
    },

    _expression: function()
    {
        return this.property.name;
    },

    update: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.update.call(this);

        if (this.property.wasThrown) {
            this.valueElement.textContent = WebInspector.UIString("<not available>");
            this.listItemElement.addStyleClass("dimmed");
        } else
            this.listItemElement.removeStyleClass("dimmed");

        var deleteButton = document.createElement("input");
        deleteButton.type = "button";
        deleteButton.title = WebInspector.UIString("Delete watch expression.");
        deleteButton.addStyleClass("enabled-button");
        deleteButton.addStyleClass("delete-button");
        deleteButton.addEventListener("click", this._deleteButtonClicked.bind(this), false);
        this.listItemElement.addEventListener("contextmenu", this._contextMenu.bind(this), false);
        this.listItemElement.insertBefore(deleteButton, this.listItemElement.firstChild);
    },

    /**
     * @param {WebInspector.ContextMenu} contextMenu
     * @override
     */
    populateContextMenu: function(contextMenu)
    {
        if (!this.isEditing()) {
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add watch expression" : "Add Watch Expression"), this.treeOutline.section.addNewExpressionAndEdit.bind(this.treeOutline.section));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Delete watch expression" : "Delete Watch Expression"), this._deleteButtonClicked.bind(this));
        }
        if (this.treeOutline.section.watchExpressions.length > 1)
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Delete all watch expressions" : "Delete All Watch Expressions"), this._deleteAllButtonClicked.bind(this));
    },

    _contextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        this.populateContextMenu(contextMenu);
        contextMenu.show();
    },

    _deleteAllButtonClicked: function()
    {
        this.treeOutline.section._deleteAllExpressions();
    },

    _deleteButtonClicked: function()
    {
        this.treeOutline.section.updateExpression(this, null);
    },

    renderPromptAsBlock: function()
    {
        return true;
    },

    /**
     * @param {Event=} event
     */
    elementAndValueToEdit: function(event)
    {
        return [this.nameElement, this.property.name.trim()];
    },

    editingCancelled: function(element, context)
    {
        if (!context.elementToEdit.textContent)
            this.treeOutline.section.updateExpression(this, null);

        WebInspector.ObjectPropertyTreeElement.prototype.editingCancelled.call(this, element, context);
    },

    applyExpression: function(expression, updateInterface)
    {
        expression = expression.trim();

        if (!expression)
            expression = null;

        this.property.name = expression;
        this.treeOutline.section.updateExpression(this, expression);
    },

    __proto__: WebInspector.ObjectPropertyTreeElement.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ObjectPropertyTreeElement}
 * @param {WebInspector.RemoteObjectProperty} property
 */
WebInspector.WatchedPropertyTreeElement = function(property)
{
    WebInspector.ObjectPropertyTreeElement.call(this, property);
}

WebInspector.WatchedPropertyTreeElement.prototype = {
    onattach: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.onattach.call(this);
        if (this.hasChildren && this.propertyPath() in this.treeOutline.section._expandedProperties)
            this.expand();
    },

    onexpand: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.onexpand.call(this);
        this.treeOutline.section._expandedProperties[this.propertyPath()] = true;
    },

    oncollapse: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.oncollapse.call(this);
        delete this.treeOutline.section._expandedProperties[this.propertyPath()];
    },

    __proto__: WebInspector.ObjectPropertyTreeElement.prototype
}
