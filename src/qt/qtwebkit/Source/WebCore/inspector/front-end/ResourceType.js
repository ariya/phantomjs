/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {string} name
 * @param {string} title
 * @param {string} categoryTitle
 * @param {string} color
 * @param {boolean} isTextType
 */
WebInspector.ResourceType = function(name, title, categoryTitle, color, isTextType)
{
    this._name = name;
    this._title = title;
    this._categoryTitle = categoryTitle;
    this._color = color;
    this._isTextType = isTextType;
}

WebInspector.ResourceType.prototype = {
    /**
     * @return {string}
     */
    name: function()
    {
        return this._name;
    },

    /**
     * @return {string}
     */
    title: function()
    {
        return this._title;
    },

    /**
     * @return {string}
     */
    categoryTitle: function()
    {
        return this._categoryTitle;
    },

    /**
     * @return {string}
     */
    color: function()
    {
        return this._color;
    },

    /**
     * @return {boolean}
     */
    isTextType: function()
    {
        return this._isTextType;
    },

    /**
     * @return {string}
     */
    toString: function()
    {
        return this._name;
    },

    /**
     * @return {string}
     */
    canonicalMimeType: function()
    {
        if (this === WebInspector.resourceTypes.Document)
            return "text/html";
        if (this === WebInspector.resourceTypes.Script)
            return "text/javascript";
        if (this === WebInspector.resourceTypes.Stylesheet)
            return "text/css";
        return "";
    }
}

/**
 * Keep these in sync with WebCore::InspectorPageAgent::resourceTypeJson
 * @enum {!WebInspector.ResourceType}
 */
WebInspector.resourceTypes = {
    Document: new WebInspector.ResourceType("document", "Document", "Documents", "rgb(47,102,236)", true),
    Stylesheet: new WebInspector.ResourceType("stylesheet", "Stylesheet", "Stylesheets", "rgb(157,231,119)", true),
    Image: new WebInspector.ResourceType("image", "Image", "Images", "rgb(164,60,255)", false),
    Script: new WebInspector.ResourceType("script", "Script", "Scripts", "rgb(255,121,0)", true),
    XHR: new WebInspector.ResourceType("xhr", "XHR", "XHR", "rgb(231,231,10)", true),
    Font: new WebInspector.ResourceType("font", "Font", "Fonts", "rgb(255,82,62)", false),
    WebSocket: new WebInspector.ResourceType("websocket", "WebSocket", "WebSockets", "rgb(186,186,186)", false), // FIXME: Decide the color.
    Other: new WebInspector.ResourceType("other", "Other", "Other", "rgb(186,186,186)", false)
}
