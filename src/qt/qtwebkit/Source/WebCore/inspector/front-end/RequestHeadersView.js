/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) IBM Corp. 2009  All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestHeadersView = function(request)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("resourceView.css");
    this.element.addStyleClass("resource-headers-view");

    this._request = request;

    this._headersListElement = document.createElement("ol");
    this._headersListElement.className = "outline-disclosure";
    this.element.appendChild(this._headersListElement);

    this._headersTreeOutline = new TreeOutline(this._headersListElement);
    this._headersTreeOutline.expandTreeElementsWhenArrowing = true;

    this._urlTreeElement = new TreeElement("", null, false);
    this._urlTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._urlTreeElement);

    this._requestMethodTreeElement = new TreeElement("", null, false);
    this._requestMethodTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._requestMethodTreeElement);

    this._statusCodeTreeElement = new TreeElement("", null, false);
    this._statusCodeTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._statusCodeTreeElement);

    this._requestHeadersTreeElement = new TreeElement("", null, true);
    this._requestHeadersTreeElement.expanded = true;
    this._requestHeadersTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._requestHeadersTreeElement);

    this._decodeRequestParameters = true;

    this._showRequestHeadersText = false;
    this._showResponseHeadersText = false;

    this._queryStringTreeElement = new TreeElement("", null, true);
    this._queryStringTreeElement.expanded = true;
    this._queryStringTreeElement.selectable = false;
    this._queryStringTreeElement.hidden = true;
    this._headersTreeOutline.appendChild(this._queryStringTreeElement);

    this._urlFragmentTreeElement = new TreeElement("", null, true);
    this._urlFragmentTreeElement.expanded = true;
    this._urlFragmentTreeElement.selectable = false;
    this._urlFragmentTreeElement.hidden = true;
    this._headersTreeOutline.appendChild(this._urlFragmentTreeElement);

    this._formDataTreeElement = new TreeElement("", null, true);
    this._formDataTreeElement.expanded = true;
    this._formDataTreeElement.selectable = false;
    this._formDataTreeElement.hidden = true;
    this._headersTreeOutline.appendChild(this._formDataTreeElement);

    this._requestPayloadTreeElement = new TreeElement(WebInspector.UIString("Request Payload"), null, true);
    this._requestPayloadTreeElement.expanded = true;
    this._requestPayloadTreeElement.selectable = false;
    this._requestPayloadTreeElement.hidden = true;
    this._headersTreeOutline.appendChild(this._requestPayloadTreeElement);

    this._responseHeadersTreeElement = new TreeElement("", null, true);
    this._responseHeadersTreeElement.expanded = true;
    this._responseHeadersTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._responseHeadersTreeElement);
}

WebInspector.RequestHeadersView.prototype = {

    wasShown: function()
    {
        this._request.addEventListener(WebInspector.NetworkRequest.Events.RequestHeadersChanged, this._refreshRequestHeaders, this);
        this._request.addEventListener(WebInspector.NetworkRequest.Events.ResponseHeadersChanged, this._refreshResponseHeaders, this);
        this._request.addEventListener(WebInspector.NetworkRequest.Events.FinishedLoading, this._refreshHTTPInformation, this);

        this._refreshURL();
        this._refreshQueryString();
        this._refreshUrlFragment();
        this._refreshRequestHeaders();
        this._refreshResponseHeaders();
        this._refreshHTTPInformation();
    },

    willHide: function()
    {
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.RequestHeadersChanged, this._refreshRequestHeaders, this);
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.ResponseHeadersChanged, this._refreshResponseHeaders, this);
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.FinishedLoading, this._refreshHTTPInformation, this);
    },

    /**
     * @param {string} name
     * @param {string} value
     */
    _formatHeader: function(name, value)
    {
        var fragment = document.createDocumentFragment();
        fragment.createChild("div", "header-name").textContent = name + ":";
        fragment.createChild("div", "header-value source-code").textContent = value;

        return fragment;
    },

    /**
     * @param {string} value
     * @param {string} className
     * @param {boolean} decodeParameters
     */
    _formatParameter: function(value, className, decodeParameters)
    {
        var errorDecoding = false;

        if (decodeParameters) {
            value = value.replace(/\+/g, " ");
            if (value.indexOf("%") >= 0) {
                try {
                    value = decodeURIComponent(value);
                } catch (e) {
                    errorDecoding = true;
                }
            }
        }
        var div = document.createElement("div");
        div.className = className;
        if (errorDecoding)
            div.createChild("span", "error-message").textContent = WebInspector.UIString("(unable to decode value)");
        else
            div.textContent = value;
        return div;
    },

    _refreshURL: function()
    {
        this._urlTreeElement.title = this._formatHeader(WebInspector.UIString("Request URL"), this._request.url);
    },

    _refreshQueryString: function()
    {
        var queryString = this._request.queryString();
        var queryParameters = this._request.queryParameters;
        this._queryStringTreeElement.hidden = !queryParameters;
        if (queryParameters)
            this._refreshParams(WebInspector.UIString("Query String Parameters"), queryParameters, queryString, this._queryStringTreeElement);
    },

    _refreshUrlFragment: function()
    {
        var urlFragment = this._request.parsedURL.fragment;
        this._urlFragmentTreeElement.hidden = !urlFragment;

        if (!urlFragment)
            return;

        var sectionTitle = WebInspector.UIString("URL fragment");

        this._urlFragmentTreeElement.removeChildren();
        this._urlFragmentTreeElement.listItemElement.removeChildren();
        this._urlFragmentTreeElement.listItemElement.appendChild(document.createTextNode(sectionTitle));

        var fragmentTreeElement = new TreeElement(null, null, false);
        fragmentTreeElement.title = this._formatHeader("#", urlFragment);
        fragmentTreeElement.selectable = false;
        this._urlFragmentTreeElement.appendChild(fragmentTreeElement);
    },

    _refreshFormData: function()
    {
        this._formDataTreeElement.hidden = true;
        this._requestPayloadTreeElement.hidden = true;

        var formData = this._request.requestFormData;
        if (!formData)
            return;

        var formParameters = this._request.formParameters;
        if (formParameters) {
            this._formDataTreeElement.hidden = false;
            this._refreshParams(WebInspector.UIString("Form Data"), formParameters, formData, this._formDataTreeElement);
        } else {
            this._requestPayloadTreeElement.hidden = false;
            try {
                var json = JSON.parse(formData);
                this._refreshRequestJSONPayload(json, formData, false);
            } catch (e) {
                this._populateTreeElementWithSourceText(this._requestPayloadTreeElement, formData);
            }
        }
    },

    _populateTreeElementWithSourceText: function(treeElement, sourceText)
    {
        treeElement.removeChildren();

        var sourceTreeElement = new TreeElement(null, null, false);
        sourceTreeElement.selectable = false;
        treeElement.appendChild(sourceTreeElement);

        var sourceTextElement = document.createElement("span");
        sourceTextElement.addStyleClass("header-value");
        sourceTextElement.addStyleClass("source-code");
        sourceTextElement.textContent = String(sourceText).trim();
        sourceTreeElement.listItemElement.appendChild(sourceTextElement);
    },

    _refreshParams: function(title, params, sourceText, paramsTreeElement)
    {
        paramsTreeElement.removeChildren();

        paramsTreeElement.listItemElement.removeChildren();
        paramsTreeElement.listItemElement.appendChild(document.createTextNode(title));

        var headerCount = document.createElement("span");
        headerCount.addStyleClass("header-count");
        headerCount.textContent = WebInspector.UIString(" (%d)", params.length);
        paramsTreeElement.listItemElement.appendChild(headerCount);

        function toggleViewSource()
        {
            paramsTreeElement._viewSource = !paramsTreeElement._viewSource;
            this._refreshParams(title, params, sourceText, paramsTreeElement);
        }

        paramsTreeElement.listItemElement.appendChild(this._createViewSourceToggle(paramsTreeElement._viewSource, toggleViewSource.bind(this)));
        
        if (paramsTreeElement._viewSource) {
            this._populateTreeElementWithSourceText(paramsTreeElement, sourceText);
            return;
        }

        var toggleTitle = this._decodeRequestParameters ? WebInspector.UIString("view URL encoded") : WebInspector.UIString("view decoded");
        var toggleButton = this._createToggleButton(toggleTitle);
        toggleButton.addEventListener("click", this._toggleURLDecoding.bind(this));
        paramsTreeElement.listItemElement.appendChild(toggleButton);

        for (var i = 0; i < params.length; ++i) {
            var paramNameValue = document.createDocumentFragment();
            var name = this._formatParameter(params[i].name + ":", "header-name", this._decodeRequestParameters);
            var value = this._formatParameter(params[i].value, "header-value source-code", this._decodeRequestParameters);
            paramNameValue.appendChild(name);
            paramNameValue.appendChild(value);

            var parmTreeElement = new TreeElement(paramNameValue, null, false);
            parmTreeElement.selectable = false;
            paramsTreeElement.appendChild(parmTreeElement);
        }
    },

    /**
     * @param {Object} parsedObject
     * @param {string} sourceText
     * @param {boolean} viewSource
     */
    _refreshRequestJSONPayload: function(parsedObject, sourceText, viewSource)
    {
        this._requestPayloadTreeElement.removeChildren();

        var listItem = this._requestPayloadTreeElement.listItemElement;
        listItem.removeChildren();
        listItem.appendChild(document.createTextNode(this._requestPayloadTreeElement.title));

        var setViewSource = this._refreshRequestJSONPayload.bind(this, parsedObject, sourceText);

        if (viewSource) {
            listItem.appendChild(this._createViewSourceToggle(true, setViewSource.bind(this, false)));
            this._populateTreeElementWithSourceText(this._requestPayloadTreeElement, sourceText);
        } else {
            listItem.appendChild(this._createViewSourceToggle(false, setViewSource.bind(this, true)));
            var object = WebInspector.RemoteObject.fromLocalObject(parsedObject);
            var section = new WebInspector.ObjectPropertiesSection(object, object.description);
            section.expand();
            section.editable = false;
            listItem.appendChild(section.element);
        }
    },

    /**
     * @param {boolean} viewSource
     * @param {Function} handler
     */
    _createViewSourceToggle: function(viewSource, handler)
    {
        var viewSourceToggleTitle = viewSource ? WebInspector.UIString("view parsed") : WebInspector.UIString("view source");
        var viewSourceToggleButton = this._createToggleButton(viewSourceToggleTitle);
        viewSourceToggleButton.addEventListener("click", handler);
        return viewSourceToggleButton;
    },

    _toggleURLDecoding: function(event)
    {
        this._decodeRequestParameters = !this._decodeRequestParameters;
        this._refreshQueryString();
        this._refreshFormData();
    },

    _getHeaderValue: function(headers, key)
    {
        var lowerKey = key.toLowerCase();
        for (var testKey in headers) {
            if (testKey.toLowerCase() === lowerKey)
                return headers[testKey];
        }
    },

    _refreshRequestHeaders: function()
    {
        if (this._showRequestHeadersText)
            this._refreshHeadersText(WebInspector.UIString("Request Headers"), this._request.sortedRequestHeaders, this._request.requestHeadersText, this._requestHeadersTreeElement);
        else
            this._refreshHeaders(WebInspector.UIString("Request Headers"), this._request.sortedRequestHeaders, this._requestHeadersTreeElement);

        if (this._request.requestHeadersText) {
            var toggleButton = this._createHeadersToggleButton(this._showRequestHeadersText);
            toggleButton.addEventListener("click", this._toggleRequestHeadersText.bind(this));
            this._requestHeadersTreeElement.listItemElement.appendChild(toggleButton);
        }

        this._refreshFormData();
    },

    _refreshResponseHeaders: function()
    {
        if (this._showResponseHeadersText)
            this._refreshHeadersText(WebInspector.UIString("Response Headers"), this._request.sortedResponseHeaders, this._request.responseHeadersText, this._responseHeadersTreeElement);
        else
            this._refreshHeaders(WebInspector.UIString("Response Headers"), this._request.sortedResponseHeaders, this._responseHeadersTreeElement);

        if (this._request.responseHeadersText) {
            var toggleButton = this._createHeadersToggleButton(this._showResponseHeadersText);
            toggleButton.addEventListener("click", this._toggleResponseHeadersText.bind(this));
            this._responseHeadersTreeElement.listItemElement.appendChild(toggleButton);
        }
    },

    _refreshHTTPInformation: function()
    {
        var requestMethodElement = this._requestMethodTreeElement;
        requestMethodElement.hidden = !this._request.statusCode;
        var statusCodeElement = this._statusCodeTreeElement;
        statusCodeElement.hidden = !this._request.statusCode;

        if (this._request.statusCode) {
            var statusImageSource = "";
            if (this._request.statusCode < 300 || this._request.statusCode === 304)
                statusImageSource = "Images/successGreenDot.png";
            else if (this._request.statusCode < 400)
                statusImageSource = "Images/warningOrangeDot.png";
            else
                statusImageSource = "Images/errorRedDot.png";

            requestMethodElement.title = this._formatHeader(WebInspector.UIString("Request Method"), this._request.requestMethod);

            var statusCodeFragment = document.createDocumentFragment();
            statusCodeFragment.createChild("div", "header-name").textContent = WebInspector.UIString("Status Code") + ":";

            var statusCodeImage = statusCodeFragment.createChild("img", "resource-status-image");
            statusCodeImage.src = statusImageSource;
            statusCodeImage.title = this._request.statusCode + " " + this._request.statusText;
            var value = statusCodeFragment.createChild("div", "header-value source-code");
            value.textContent = this._request.statusCode + " " + this._request.statusText;
            if (this._request.cached)
                value.createChild("span", "status-from-cache").textContent = " " + WebInspector.UIString("(from cache)");

            statusCodeElement.title = statusCodeFragment;
        }
    },

    _refreshHeadersTitle: function(title, headersTreeElement, headersLength)
    {
        headersTreeElement.listItemElement.removeChildren();
        headersTreeElement.listItemElement.appendChild(document.createTextNode(title));

        var headerCount = document.createElement("span");
        headerCount.addStyleClass("header-count");
        headerCount.textContent = WebInspector.UIString(" (%d)", headersLength);
        headersTreeElement.listItemElement.appendChild(headerCount);
    },

    _refreshHeaders: function(title, headers, headersTreeElement)
    {
        headersTreeElement.removeChildren();

        var length = headers.length;
        this._refreshHeadersTitle(title, headersTreeElement, length);
        headersTreeElement.hidden = !length;
        for (var i = 0; i < length; ++i) {
            var headerTreeElement = new TreeElement(null, null, false);
            headerTreeElement.title = this._formatHeader(headers[i].name, headers[i].value);
            headerTreeElement.selectable = false;
            headersTreeElement.appendChild(headerTreeElement);
        }
    },

    _refreshHeadersText: function(title, headers, headersText, headersTreeElement)
    {
        this._populateTreeElementWithSourceText(headersTreeElement, headersText);
        this._refreshHeadersTitle(title, headersTreeElement, headers.length);
    },

    _toggleRequestHeadersText: function(event)
    {
        this._showRequestHeadersText = !this._showRequestHeadersText;
        this._refreshRequestHeaders();
    },

    _toggleResponseHeadersText: function(event)
    {
        this._showResponseHeadersText = !this._showResponseHeadersText;
        this._refreshResponseHeaders();
    },

    _createToggleButton: function(title)
    {
        var button = document.createElement("span");
        button.addStyleClass("header-toggle");
        button.textContent = title;
        return button;
    },

    _createHeadersToggleButton: function(isHeadersTextShown)
    {
        var toggleTitle = isHeadersTextShown ? WebInspector.UIString("view parsed") : WebInspector.UIString("view source");
        return this._createToggleButton(toggleTitle);
    },

    __proto__: WebInspector.View.prototype
}
