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

WebInspector.CookieStorageContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);

    this.element.classList.add(WebInspector.CookieStorageContentView.StyleClassName);

    this.update();
};

WebInspector.CookieStorageContentView.StyleClassName = "cookie-storage";

WebInspector.CookieStorageContentView.prototype = {
    constructor: WebInspector.CookieStorageContentView,

    // Public

    update: function()
    {
        function callback(error, cookies, cookiesString)
        {
            if (error)
                return;

            this._cookies = this._filterCookies(cookies);
            this._rebuildTable();
        }
    
        PageAgent.getCookies(callback.bind(this));
    },

    updateLayout: function()
    {
        if (this._dataGrid)
            this._dataGrid.updateLayout();
    },

    get scrollableElements()
    {
        if (!this._dataGrid)
            return [];
        return [this._dataGrid.scrollContainer];
    },

    // Private

    _rebuildTable: function()
    {
        // FIXME: If there are no cookies, do we want to show an empty datagrid, or do something like the old
        // inspector and show some text saying there are no cookies?
        if (!this._dataGrid) {
            var columns = { 0: {}, 1: {}, 2: {}, 3: {}, 4: {}, 5: {}, 6: {}, 7: {} };
            columns[0].title = WebInspector.UIString("Name");
            columns[0].sortable = true;
            columns[0].width = "24%";
            columns[1].title = WebInspector.UIString("Value");
            columns[1].sortable = true;
            columns[1].width = "34%";
            columns[2].title = WebInspector.UIString("Domain");
            columns[2].sortable = true;
            columns[2].width = "7%";
            columns[3].title = WebInspector.UIString("Path");
            columns[3].sortable = true;
            columns[3].width = "7%";
            columns[4].title = WebInspector.UIString("Expires");
            columns[4].sortable = true;
            columns[4].width = "7%";
            columns[5].title = WebInspector.UIString("Size");
            columns[5].aligned = "right";
            columns[5].sortable = true;
            columns[5].width = "7%";
            columns[6].title = WebInspector.UIString("HTTP");
            columns[6].aligned = "centered";
            columns[6].sortable = true;
            columns[6].width = "7%";
            columns[7].title = WebInspector.UIString("Secure");
            columns[7].aligned = "centered";
            columns[7].sortable = true;
            columns[7].width = "7%";

            this._dataGrid = new WebInspector.DataGrid(columns, null, this._deleteCallback.bind(this));
            this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._rebuildTable, this);

            this.element.appendChild(this._dataGrid.element);
            this._dataGrid.updateLayout();
        }
        
        console.assert(this._dataGrid);
        this._dataGrid.removeChildren();

        this._sortCookies(this._cookies);

        for (var i = 0; i < this._cookies.length; ++i) {
            const cookie = this._cookies[i];

            var data = {};
            data[0] = cookie.name;
            data[1] = cookie.value;
            data[2] = cookie.domain || "";
            data[3] = cookie.path || "";

            if (cookie.type === WebInspector.CookieType.Request)
                data[4] = "";
            else
                data[4] = cookie.session ? WebInspector.UIString("Session") : new Date(cookie.expires).toLocaleString();

            data[5] = Number.bytesToString(cookie.size);
            const checkmark = "\u2713";
            data[6] = cookie.httpOnly ? checkmark : "";
            data[7] = cookie.secure ? checkmark : "";

            var node = new WebInspector.DataGridNode(data);
            node.cookie = cookie;
            node.selectable = true;
            
            this._dataGrid.appendChild(node);
        }
    },

    _filterCookies: function(cookies)
    {
        var filteredCookies = [];
        var resourcesForDomain = [];

        var frames = WebInspector.frameResourceManager.frames;
        for (var i = 0; i < frames.length; ++i) {
            var resources = frames[i].resources;
            for (var j = 0; j < resources.length; ++j) {
                var urlComponents = resources[j].urlComponents;
                if (urlComponents && urlComponents.host && urlComponents.host === this.representedObject.host)
                    resourcesForDomain.push(resources[j].url);
            }

            // The main resource isn't always in the list of resources, make sure to add it to the list of resources
            // we get the URLs from.
            var mainResourceURLComponents = frames[i].mainResource.urlComponents;
            if (mainResourceURLComponents && mainResourceURLComponents.host && mainResourceURLComponents.host == this.representedObject.host)
                resourcesForDomain.push(frames[i].mainResource.url);
        }

        for (var i = 0; i < cookies.length; ++i) {
            for (var j = 0; j < resourcesForDomain.length; ++j) {
                if (WebInspector.cookieMatchesResourceURL(cookies[i], resourcesForDomain[j])) {
                    filteredCookies.push(cookies[i]);
                    break;
                }
            }
        }
        
        return filteredCookies;
    },
    
    _sortCookies: function(cookies)
    {
        var sortDirection = this._dataGrid.sortOrder === "ascending" ? 1 : -1;

        function localeCompare(field, cookie1, cookie2)
        {
            return sortDirection * (cookie1[field] + "").localeCompare(cookie2[field] + "")
        }

        function numberCompare(field, cookie1, cookie2)
        {
            return sortDirection * (cookie1[field] - cookie2[field]);
        }

        function expiresCompare(cookie1, cookie2)
        {
            if (cookie1.session !== cookie2.session)
                return sortDirection * (cookie1.session ? 1 : -1);

            if (cookie1.session)
                return 0;

            return sortDirection * (cookie1.expires - cookie2.expires);
        }

        var comparator;
        switch (parseInt(this._dataGrid.sortColumnIdentifier, 10)) {
            case 0: comparator = localeCompare.bind(this, "name"); break;
            case 1: comparator = localeCompare.bind(this, "value"); break;
            case 2: comparator = localeCompare.bind(this, "domain"); break;
            case 3: comparator = localeCompare.bind(this, "path"); break;
            case 4: comparator = expiresCompare; break;
            case 5: comparator = numberCompare.bind(this, "size"); break;
            case 6: comparator = localeCompare.bind(this, "httpOnly"); break;
            case 7: comparator = localeCompare.bind(this, "secure"); break;
            default: localeCompare.bind(this, "name");
        }

        cookies.sort(comparator);
    },

    _deleteCallback: function(node)
    {
        if (!node || !node.cookie)
            return;

        var cookie = node.cookie;
        var cookieURL = (cookie.secure ? "https://" : "http://") + cookie.domain + cookie.path;

        // COMPATIBILITY (iOS 6): PageAgent.deleteCookie used to take 'domain', now takes 'url'. Send both.
        PageAgent.deleteCookie.invoke({cookieName: cookie.name, domain: cookie.domain, url: cookieURL});

        this.update();
    }
};

WebInspector.CookieStorageContentView.prototype.__proto__ = WebInspector.ContentView.prototype;

WebInspector.cookieMatchesResourceURL = function(cookie, resourceURL)
{
    var parsedURL = parseURL(resourceURL);
    if (!parsedURL || !WebInspector.cookieDomainMatchesResourceDomain(cookie.domain, parsedURL.host))
        return false;

    return (parsedURL.path.startsWith(cookie.path)
        && (!cookie.port || parsedURL.port == cookie.port)
        && (!cookie.secure || parsedURL.scheme === "https"));
}

WebInspector.cookieDomainMatchesResourceDomain = function(cookieDomain, resourceDomain)
{
    if (cookieDomain.charAt(0) !== '.')
        return resourceDomain === cookieDomain;
    return !!resourceDomain.match(new RegExp("^([^\\.]+\\.)?" + cookieDomain.substring(1).escapeForRegExp() + "$"), "i");
}

WebInspector.CookieType = {
    Request: 0,
    Response: 1
};
