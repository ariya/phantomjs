/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @extends {WebInspector.Panel}
 * @param {string} id
 * @param {string} pageURL
 */
WebInspector.ExtensionPanel = function(id, pageURL)
{
    WebInspector.Panel.call(this, id);
    this.setHideOnDetach();
    this._statusBarItems = [];
    var extensionView = new WebInspector.ExtensionView(id, pageURL, "extension panel");
    extensionView.show(this.element);
    this.setDefaultFocusedElement(extensionView.defaultFocusedElement());
}

WebInspector.ExtensionPanel.prototype = {
    defaultFocusedElement: function()
    {
        return WebInspector.View.prototype.defaultFocusedElement.call(this);
    },

    statusBarItems: function()
    {
        return this._statusBarItems;
    },

    /**
     * @param {Element} element
     */
    addStatusBarItem: function(element)
    {
        this._statusBarItems.push(element);
    },

    searchCanceled: function(startingNewSearch)
    {
        WebInspector.extensionServer.notifySearchAction(this.name, WebInspector.extensionAPI.panels.SearchAction.CancelSearch);
        WebInspector.Panel.prototype.searchCanceled.apply(this, arguments);
    },

    /**
     * @param {string} query
     */
    performSearch: function(query)
    {
        WebInspector.extensionServer.notifySearchAction(this.name, WebInspector.extensionAPI.panels.SearchAction.PerformSearch, query);
        WebInspector.Panel.prototype.performSearch.apply(this, arguments);
    },

    jumpToNextSearchResult: function()
    {
        WebInspector.extensionServer.notifySearchAction(this.name, WebInspector.extensionAPI.panels.SearchAction.NextSearchResult);
        WebInspector.Panel.prototype.jumpToNextSearchResult.call(this);
    },

    jumpToPreviousSearchResult: function()
    {
        WebInspector.extensionServer.notifySearchAction(this.name, WebInspector.extensionAPI.panels.SearchAction.PreviousSearchResult);
        WebInspector.Panel.prototype.jumpToPreviousSearchResult.call(this);
    },

    __proto__: WebInspector.Panel.prototype
}

/**
 * @constructor
 * @param {string} id
 * @param {string} iconURL
 * @param {string=} tooltip
 * @param {boolean=} disabled
 */
WebInspector.ExtensionButton = function(id, iconURL, tooltip, disabled)
{
    this._id = id;
    this.element = document.createElement("button");
    this.element.className = "status-bar-item extension";
    this.element.addEventListener("click", this._onClicked.bind(this), false);
    this.update(iconURL, tooltip, disabled);
}

WebInspector.ExtensionButton.prototype = {
    /**
     * @param {string} iconURL
     * @param {string=} tooltip
     * @param {boolean=} disabled
     */
    update: function(iconURL, tooltip, disabled)
    {
        if (typeof iconURL === "string")
            this.element.style.backgroundImage = "url(" + iconURL + ")";
        if (typeof tooltip === "string")
            this.element.title = tooltip;
        if (typeof disabled === "boolean")
            this.element.disabled = disabled;
    },

    _onClicked: function()
    {
        WebInspector.extensionServer.notifyButtonClicked(this._id);
    }
}

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 * @param {string} title
 * @param {string} id
 */
WebInspector.ExtensionSidebarPane = function(title, id)
{
    WebInspector.SidebarPane.call(this, title);
    this.setHideOnDetach();
    this._id = id;
}

WebInspector.ExtensionSidebarPane.prototype = {
    /**
     * @param {Object} object
     * @param {string} title
     * @param {function(?string=)} callback
     */
    setObject: function(object, title, callback)
    {
        this._createObjectPropertiesView();
        this._setObject(WebInspector.RemoteObject.fromLocalObject(object), title, callback);
    },

    /**
     * @param {string} expression
     * @param {string} title
     * @param {function(?string=)} callback
     */
    setExpression: function(expression, title, evaluateOptions, securityOrigin, callback)
    {
        this._createObjectPropertiesView();
        return WebInspector.extensionServer.evaluate(expression, true, false, evaluateOptions, securityOrigin, this._onEvaluate.bind(this, title, callback));
    },

    /**
     * @param {string} url
     */
    setPage: function(url)
    {
        if (this._objectPropertiesView) {
            this._objectPropertiesView.detach();
            delete this._objectPropertiesView;
        }
        if (this._extensionView)
            this._extensionView.detach(true);

        this._extensionView = new WebInspector.ExtensionView(this._id, url, "extension fill");
        this._extensionView.show(this.bodyElement);

        if (!this.bodyElement.style.height)
            this.setHeight("150px");
    },

    /**
     * @param {string} height
     */
    setHeight: function(height)
    {
        this.bodyElement.style.height = height;
    },

    /**
     * @param {string} title
     * @param {function(?string=)} callback
     * @param {?Protocol.Error} error
     * @param {RuntimeAgent.RemoteObject} result
     * @param {boolean=} wasThrown
     */
    _onEvaluate: function(title, callback, error, result, wasThrown)
    {
        if (error)
            callback(error.toString());
        else
            this._setObject(WebInspector.RemoteObject.fromPayload(result), title, callback);
    },

    _createObjectPropertiesView: function()
    {
        if (this._objectPropertiesView)
            return;
        if (this._extensionView) {
            this._extensionView.detach(true);
            delete this._extensionView;
        }
        this._objectPropertiesView = new WebInspector.ExtensionNotifierView(this._id);
        this._objectPropertiesView.show(this.bodyElement);
    },

    /**
     * @param {WebInspector.RemoteObject} object
     * @param {string} title
     * @param {function(?string=)} callback
     */
    _setObject: function(object, title, callback)
    {
        // This may only happen if setPage() was called while we were evaluating the expression.
        if (!this._objectPropertiesView) {
            callback("operation cancelled");
            return;
        }
        this._objectPropertiesView.element.removeChildren();
        var section = new WebInspector.ObjectPropertiesSection(object, title);
        if (!title)
            section.headerElement.addStyleClass("hidden");
        section.expanded = true;
        section.editable = false;
        this._objectPropertiesView.element.appendChild(section.element);
        callback();
    },

    __proto__: WebInspector.SidebarPane.prototype
}
