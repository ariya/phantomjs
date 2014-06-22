/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestCookiesView = function(request)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("resource-cookies-view");

    this._request = request;
}

WebInspector.RequestCookiesView.prototype = {
    wasShown: function()
    {
        this._request.addEventListener(WebInspector.NetworkRequest.Events.RequestHeadersChanged, this._refreshCookies, this);
        this._request.addEventListener(WebInspector.NetworkRequest.Events.ResponseHeadersChanged, this._refreshCookies, this);

        if (!this._gotCookies) {
            if (!this._emptyView) {
                this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("This request has no cookies."));
                this._emptyView.show(this.element);
            }
            return;
        }

        if (!this._cookiesTable)
            this._buildCookiesTable();
    },

    willHide: function()
    {
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.RequestHeadersChanged, this._refreshCookies, this);
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.ResponseHeadersChanged, this._refreshCookies, this);
    },

    get _gotCookies()
    {
        return (this._request.requestCookies && this._request.requestCookies.length) || (this._request.responseCookies && this._request.responseCookies.length);
    },

    _buildCookiesTable: function()
    {
        this.detachChildViews();

        this._cookiesTable = new WebInspector.CookiesTable(true);
        this._cookiesTable.setCookieFolders([
            {folderName: WebInspector.UIString("Request Cookies"), cookies: this._request.requestCookies},
            {folderName: WebInspector.UIString("Response Cookies"), cookies: this._request.responseCookies}
        ]);
        this._cookiesTable.show(this.element);
    },

    _refreshCookies: function()
    {
        delete this._cookiesTable;
        if (!this._gotCookies || !this.isShowing())
            return;
        this._buildCookiesTable();
        this._cookiesTable.updateWidths();
    },

    __proto__: WebInspector.View.prototype
}
