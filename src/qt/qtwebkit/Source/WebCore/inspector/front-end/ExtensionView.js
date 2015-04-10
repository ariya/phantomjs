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
 * @extends {WebInspector.View}
 * @param {string} id
 * @param {string} src
 * @param {string} className
 */
WebInspector.ExtensionView = function(id, src, className)
{
    WebInspector.View.call(this);
    this.element.className = "fill";

    this._id = id;
    this._iframe = document.createElement("iframe");
    this._iframe.addEventListener("load", this._onLoad.bind(this), false);
    this._iframe.src = src;
    this._iframe.className = className;
    this.setDefaultFocusedElement(this._iframe);

    this.element.appendChild(this._iframe);
}

WebInspector.ExtensionView.prototype = {
    wasShown: function()
    {
        if (typeof this._frameIndex === "number")
            WebInspector.extensionServer.notifyViewShown(this._id, this._frameIndex);
    },

    willHide: function()
    {
        if (typeof this._frameIndex === "number")
            WebInspector.extensionServer.notifyViewHidden(this._id);
    },

    _onLoad: function()
    {
        this._frameIndex = Array.prototype.indexOf.call(window.frames, this._iframe.contentWindow);
        if (this.isShowing())
            WebInspector.extensionServer.notifyViewShown(this._id, this._frameIndex);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {string} id
 */
WebInspector.ExtensionNotifierView = function(id)
{
    WebInspector.View.call(this);

    this._id = id;
}

WebInspector.ExtensionNotifierView.prototype = {
    wasShown: function()
    {
        WebInspector.extensionServer.notifyViewShown(this._id);
    },

    willHide: function()
    {
        WebInspector.extensionServer.notifyViewHidden(this._id);
    },

    __proto__: WebInspector.View.prototype
}
