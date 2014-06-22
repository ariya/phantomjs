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
 * @extends {WebInspector.RequestContentView}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestPreviewView = function(request, responseView)
{
    WebInspector.RequestContentView.call(this, request);
    this._responseView = responseView;
}

WebInspector.RequestPreviewView.prototype = {
    contentLoaded: function()
    {
        if (!this.request.content) {
            if (!this._emptyView) {
                this._emptyView = this._createEmptyView();
                this._emptyView.show(this.element);
                this.innerView = this._emptyView;
            }
        } else {
            if (this._emptyView) {
                this._emptyView.detach();
                delete this._emptyView;
            }

            if (!this._previewView)
                this._previewView = this._createPreviewView();
            this._previewView.show(this.element);
            this.innerView = this._previewView;
        }
    },

    _createEmptyView: function()
    {
        return new WebInspector.EmptyView(WebInspector.UIString("This request has no preview available."));
    },

    _jsonView: function()
    {
        var parsedJSON = WebInspector.RequestJSONView.parseJSON(this.request.content);
        if (parsedJSON)
            return new WebInspector.RequestJSONView(this.request, parsedJSON);
    },

    _htmlView: function()
    {
        var dataURL = this.request.asDataURL();
        if (dataURL !== null)
            return new WebInspector.RequestHTMLView(this.request, dataURL);
    },

    _createPreviewView: function()
    {
        if (this.request.content) {
            if (this.request.hasErrorStatusCode()) {
                var htmlView = this._htmlView();
                if (htmlView)
                    return htmlView;
            }

            if (this.request.type === WebInspector.resourceTypes.XHR) {
                var jsonView = this._jsonView();
                if (jsonView)
                    return jsonView;
            }

            if (this.request.type === WebInspector.resourceTypes.XHR && this.request.mimeType === "text/html") {
                var htmlView = this._htmlView();
                if (htmlView)
                    return htmlView;
            }

            if (this.request.type === WebInspector.resourceTypes.Script && this.request.mimeType === "application/json") {
                var jsonView = this._jsonView();
                if (jsonView)
                    return jsonView;
            }
        }

        if (this._responseView.sourceView)
            return this._responseView.sourceView;

        if (this.request.type === WebInspector.resourceTypes.Other)
            return this._createEmptyView();

        return WebInspector.RequestView.nonSourceViewForRequest(this.request);
    },

    __proto__: WebInspector.RequestContentView.prototype
}
