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

WebInspector.DashboardView = function(element)
{
    WebInspector.Object.call(this);

    this._element = element;

    this._items = {
        resourcesCount: {
            tooltip: WebInspector.UIString("Total number of resources, click to show the Resources navigation sidebar"),
            handler: this._resourcesWasClicked
        },
        resourcesSize: {
            tooltip: WebInspector.UIString("Total size of all resources, click to show the Network Requests timeline"),
            handler: this._networkItemWasClicked
        },
        time: {
            tooltip: WebInspector.UIString("Time until the load event fired, click to show the Network Requests timeline"),
            handler: this._networkItemWasClicked
        },
        logs: {
            tooltip: WebInspector.UIString("Console logs, click to show the Console"),
            handler: this._consoleItemWasClicked.bind(this, WebInspector.LogContentView.Scopes.Logs)
        },
        errors: {
            tooltip: WebInspector.UIString("Console errors, click to show the Console"),
            handler: this._consoleItemWasClicked.bind(this, WebInspector.LogContentView.Scopes.Errors)
        },
        issues: {
            tooltip: WebInspector.UIString("Console warnings, click to show the Console"),
            handler: this._consoleItemWasClicked.bind(this, WebInspector.LogContentView.Scopes.Warnings)
        }
    };

    for (var name in this._items)
        this._appendElementForNamedItem(name);

    this.resourcesCount = 0;
    this.resourcesSize = 0;
    this.time = 0;
    this.logs = 0;
    this.errors = 0;
    this.issues = 0;
};

WebInspector.DashboardView.EnabledStyleClassName = "enabled";

WebInspector.DashboardView.prototype = {
    constructor: WebInspector.DashboardView,

    // Public

    get logs()
    {
        return this._logs;
    },

    set logs(logs)
    {
        this._logs = logs;
        
        var item = this._items.logs;
        item.text = this._formatPossibleLargeNumber(logs);
        this._setItemEnabled(item, logs > 0);
    },

    get issues()
    {
        return this._issues;
    },

    set issues(issues)
    {
        this._issues = issues;

        var item = this._items.issues;
        item.text = this._formatPossibleLargeNumber(issues);
        this._setItemEnabled(item, issues > 0);
    },

    get errors()
    {
        return this._errors;
    },

    set errors(errors)
    {
        this._errors = errors;

        var item = this._items.errors;
        item.text = this._formatPossibleLargeNumber(errors);
        this._setItemEnabled(item, errors > 0);
    },

    set time(time)
    {
        var item = this._items.time;
        item.text = time ? Number.secondsToString(time) : "\u2014";
        this._setItemEnabled(item, time > 0);
    },

    get resourcesCount()
    {
        return this._resourcesCount;
    },

    set resourcesCount(resourcesCount)
    {
        this._resourcesCount = resourcesCount;

        var item = this._items.resourcesCount;
        item.text = this._formatPossibleLargeNumber(resourcesCount);
        this._setItemEnabled(item, resourcesCount > 0);
    },

    get resourcesSize()
    {
        return this._resourcesSize;
    },

    set resourcesSize(resourcesSize)
    {
        this._resourcesSize = resourcesSize;

        var item = this._items.resourcesSize;
        item.text = resourcesSize ? Number.bytesToString(resourcesSize, false) : "\u2014";
        this._setItemEnabled(item, resourcesSize > 0);
    },

    // Private

    _formatPossibleLargeNumber: function(number)
    {
        return number > 999 ? WebInspector.UIString("999+") : number;
    },

    _appendElementForNamedItem: function(name)
    {
        var item = this._items[name];

        item.container = this._element.appendChild(document.createElement("div"));
        item.container.className = "item " + name;
        item.container.title = item.tooltip;

        item.container.appendChild(document.createElement("img"));

        item.outlet = item.container.appendChild(document.createElement("div"));

        Object.defineProperty(item, "text",
        {
            set: function(newText)
            {
                if (newText === item.outlet.textContent)
                    return;
                item.outlet.textContent = newText;
            }
        });

        item.container.addEventListener("click", function(event) {
            this._itemWasClicked(name);
        }.bind(this));
    },

    _itemWasClicked: function(name)
    {
        var item = this._items[name];
        if (!item.container.classList.contains(WebInspector.DashboardView.EnabledStyleClassName))
            return;

        if (item.handler)
            item.handler.call(this);
    },

    _resourcesWasClicked: function()
    {
        WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.resourceSidebarPanel;
        WebInspector.navigationSidebar.collapsed = false;
    },

    _networkItemWasClicked: function()
    {
        WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.instrumentSidebarPanel;
        WebInspector.instrumentSidebarPanel.showTimelineForRecordType(WebInspector.TimelineRecord.Type.Network);
    },

    _consoleItemWasClicked: function(scope)
    {
        WebInspector.showConsoleView(scope);
    },

    _setItemEnabled: function(item, enabled)
    {
        if (enabled)
            item.container.classList.add(WebInspector.DashboardView.EnabledStyleClassName);
        else
            item.container.classList.remove(WebInspector.DashboardView.EnabledStyleClassName);
    }
};

WebInspector.DashboardView.prototype.__proto__ = WebInspector.Object.prototype;
