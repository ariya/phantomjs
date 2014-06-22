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
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestView = function(request)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("resourceView.css");

    this.element.addStyleClass("resource-view");
    this.request = request;
}

WebInspector.RequestView.prototype = {
    hasContent: function()
    {
        return false;
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestView.hasTextContent = function(request)
{
    if (request.type.isTextType())
        return true; 
    if (request.type === WebInspector.resourceTypes.Other || request.hasErrorStatusCode())
        return request.content && !request.contentEncoded;
    return false;
}

/**
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestView.nonSourceViewForRequest = function(request)
{
    switch (request.type) {
    case WebInspector.resourceTypes.Image:
        return new WebInspector.ImageView(request);
    case WebInspector.resourceTypes.Font:
        return new WebInspector.FontView(request);
    default:
        return new WebInspector.RequestView(request);
    }
}
