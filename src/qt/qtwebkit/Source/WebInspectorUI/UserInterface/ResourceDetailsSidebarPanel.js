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

WebInspector.ResourceDetailsSidebarPanel = function() {
    WebInspector.DetailsSidebarPanel.call(this, "resource-details", WebInspector.UIString("Resource"), WebInspector.UIString("Resource"), "Images/NavigationItemFile.pdf", "1");

    this.element.classList.add(WebInspector.ResourceDetailsSidebarPanel.StyleClassName);

    this._resource = null;

    this._typeMIMETypeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("MIME Type"));
    this._typeResourceTypeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Resource Type"));

    this._typeSection = new WebInspector.DetailsSection("resource-type", WebInspector.UIString("Type"));
    this._typeSection.groups = [new WebInspector.DetailsSectionGroup([this._typeMIMETypeRow, this._typeResourceTypeRow])];

    this._locationFullURLRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Full URL"));
    this._locationSchemeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Scheme"));
    this._locationHostRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Host"));
    this._locationPortRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Port"));
    this._locationPathRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Path"));
    this._locationQueryStringRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Query String"));
    this._locationFragmentRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Fragment"));
    this._locationFilenameRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Filename"));

    var firstGroup = [this._locationFullURLRow];
    var secondGroup = [this._locationSchemeRow, this._locationHostRow, this._locationPortRow, this._locationPathRow,
        this._locationQueryStringRow, this._locationFragmentRow, this._locationFilenameRow];

    this._fullURLGroup = new WebInspector.DetailsSectionGroup(firstGroup);
    this._locationURLComponentsGroup = new WebInspector.DetailsSectionGroup(secondGroup);

    this._locationSection = new WebInspector.DetailsSection("resource-location", WebInspector.UIString("Location"), [this._fullURLGroup, this._locationURLComponentsGroup]);

    this._queryParametersRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Query Parameters"));
    this._queryParametersSection = new WebInspector.DetailsSection("resource-query-parameters", WebInspector.UIString("Query Parameters"));
    this._queryParametersSection.groups = [new WebInspector.DetailsSectionGroup([this._queryParametersRow])];

    this._requestDataSection = new WebInspector.DetailsSection("resource-request-data", WebInspector.UIString("Request Data"));

    this._requestMethodRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Method"));
    this._cachedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Cached"));

    this._statusTextRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Status"));
    this._statusCodeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Code"));

    this._encodedSizeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Encoded"));
    this._decodedSizeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Decoded"));
    this._transferSizeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Transfered"));

    this._compressedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Compressed"));
    this._compressionRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Compression"));

    var requestGroup = new WebInspector.DetailsSectionGroup([this._requestMethodRow, this._cachedRow]);
    var statusGroup = new WebInspector.DetailsSectionGroup([this._statusTextRow, this._statusCodeRow]);
    var sizeGroup = new WebInspector.DetailsSectionGroup([this._encodedSizeRow, this._decodedSizeRow, this._transferSizeRow]);
    var compressionGroup = new WebInspector.DetailsSectionGroup([this._compressedRow, this._compressionRow]);

    this._requestAndResponseSection = new WebInspector.DetailsSection("resource-request-response", WebInspector.UIString("Request & Response"), [requestGroup, statusGroup, sizeGroup, compressionGroup]);

    this._requestHeadersRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Request Headers"));
    this._requestHeadersSection = new WebInspector.DetailsSection("resource-request-headers", WebInspector.UIString("Request Headers"));
    this._requestHeadersSection.groups = [new WebInspector.DetailsSectionGroup([this._requestHeadersRow])];

    this._responseHeadersRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Response Headers"));
    this._responseHeadersSection = new WebInspector.DetailsSection("resource-response-headers", WebInspector.UIString("Response Headers"));
    this._responseHeadersSection.groups = [new WebInspector.DetailsSectionGroup([this._responseHeadersRow])];

    // Rows for the "Image Size" section.
    this._imageWidthRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Width"));
    this._imageHeightRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Height"));

    // "Image Size" section where we display intrinsic metrics for image resources.
    this._imageSizeSection = new WebInspector.DetailsSection("resource-type", WebInspector.UIString("Image Size"));
    this._imageSizeSection.groups = [new WebInspector.DetailsSectionGroup([this._imageWidthRow, this._imageHeightRow])];

    this.element.appendChild(this._typeSection.element);
    this.element.appendChild(this._locationSection.element);
    this.element.appendChild(this._requestAndResponseSection.element);
    this.element.appendChild(this._requestHeadersSection.element);
    this.element.appendChild(this._responseHeadersSection.element);
};

WebInspector.ResourceDetailsSidebarPanel.StyleClassName = "resource";

WebInspector.ResourceDetailsSidebarPanel.prototype = {
    constructor: WebInspector.ResourceDetailsSidebarPanel,

    // Public

    inspect: function(objects)
    {
        // Convert to a single item array if needed.
        if (!(objects instanceof Array))
            objects = [objects];

        var resourceToInspect = null;

        // Iterate over the objects to find a WebInspector.Resource to inspect.
        for (var i = 0; i < objects.length; ++i) {
            if (objects[i] instanceof WebInspector.Resource) {
                resourceToInspect = objects[i];
                break;
            }

            if (objects[i] instanceof WebInspector.Frame) {
                resourceToInspect = objects[i].mainResource;
                break;
            }
        }

        this.resource = resourceToInspect;

        return !!this._resource;
    },

    get resource()
    {
        return this._resource;
    },

    set resource(resource)
    {
        if (resource === this._resource)
            return;

        if (this._resource) {
            this._resource.removeEventListener(WebInspector.Resource.Event.URLDidChange, this._refreshURL, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.MIMETypeDidChange, this._refreshMIMEType, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.TypeDidChange, this._refreshResourceType, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.RequestHeadersDidChange, this._refreshRequestHeaders, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.ResponseReceived, this._refreshRequestAndResponse, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.CacheStatusDidChange, this._refreshRequestAndResponse, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.SizeDidChange, this._refreshDecodedSize, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.TransferSizeDidChange, this._refreshTransferSize, this);
        }

        this._resource = resource;

        if (this._resource) {
            this._resource.addEventListener(WebInspector.Resource.Event.URLDidChange, this._refreshURL, this);
            this._resource.addEventListener(WebInspector.Resource.Event.MIMETypeDidChange, this._refreshMIMEType, this);
            this._resource.addEventListener(WebInspector.Resource.Event.TypeDidChange, this._refreshResourceType, this);
            this._resource.addEventListener(WebInspector.Resource.Event.RequestHeadersDidChange, this._refreshRequestHeaders, this);
            this._resource.addEventListener(WebInspector.Resource.Event.ResponseReceived, this._refreshRequestAndResponse, this);
            this._resource.addEventListener(WebInspector.Resource.Event.CacheStatusDidChange, this._refreshRequestAndResponse, this);
            this._resource.addEventListener(WebInspector.Resource.Event.SizeDidChange, this._refreshDecodedSize, this);
            this._resource.addEventListener(WebInspector.Resource.Event.TransferSizeDidChange, this._refreshTransferSize, this);
        }

        this.needsRefresh();
    },

    refresh: function()
    {
        if (!this._resource)
            return;

        this._refreshURL();
        this._refreshMIMEType();
        this._refreshResourceType();
        this._refreshRequestAndResponse();
        this._refreshDecodedSize();
        this._refreshTransferSize();
        this._refreshRequestHeaders();
        this._refreshImageSizeSection();
        this._refreshRequestDataSection();
    },

    // Private

    _refreshURL: function()
    {
        if (!this._resource)
            return;

        this._locationFullURLRow.value = this._resource.url.insertWordBreakCharacters();

        var urlComponents = this._resource.urlComponents;
        if (urlComponents.scheme) {
            this._locationSection.groups = [this._fullURLGroup, this._locationURLComponentsGroup];

            this._locationSchemeRow.value = urlComponents.scheme ? urlComponents.scheme : null;
            this._locationHostRow.value = urlComponents.host ? urlComponents.host : null;
            this._locationPortRow.value = urlComponents.port ? urlComponents.port : null;
            this._locationPathRow.value = urlComponents.path ? urlComponents.path.insertWordBreakCharacters() : null;
            this._locationQueryStringRow.value = urlComponents.queryString ? urlComponents.queryString.insertWordBreakCharacters() : null;
            this._locationFragmentRow.value = urlComponents.fragment ? urlComponents.fragment.insertWordBreakCharacters() : null;
            this._locationFilenameRow.value = urlComponents.lastPathComponent ? urlComponents.lastPathComponent.insertWordBreakCharacters() : null;
        } else
            this._locationSection.groups = [this._fullURLGroup];

        if (urlComponents.queryString) {
            // Ensure the "Query Parameters" section is displayed, right after the "Request & Response" section.
            this.element.insertBefore(this._queryParametersSection.element, this._requestAndResponseSection.element.nextSibling);

            this._queryParametersRow.dataGrid = this._createNameValueDataGrid(parseQueryString(urlComponents.queryString, true));
        } else {
            // Hide the "Query Parameters" section if we don't have a query string.
            var queryParametersSectionElement = this._queryParametersSection.element;
            if (queryParametersSectionElement.parentNode)
                queryParametersSectionElement.parentNode.removeChild(queryParametersSectionElement);
        }
    },

    _refreshResourceType: function()
    {
        if (!this._resource)
            return;

        this._typeResourceTypeRow.value = WebInspector.Resource.Type.displayName(this._resource.type);
    },

    _refreshMIMEType: function()
    {
        if (!this._resource)
            return;

        this._typeMIMETypeRow.value = this._resource.mimeType;
    },

    _refreshRequestAndResponse: function()
    {
        var resource = this._resource;
        if (!resource)
            return;

        // If we don't have a value, we set an em-dash to keep the row from hiding.
        // This keeps the UI from shifting around as data comes in.
        const emDash = "\u2014";

        this._requestMethodRow.value = resource.requestMethod || emDash;

        this._cachedRow.value = resource.cached ? WebInspector.UIString("Yes") : WebInspector.UIString("No");

        this._statusCodeRow.value = resource.statusCode || emDash;
        this._statusTextRow.value = resource.statusText || emDash;

        this._refreshResponseHeaders();
        this._refreshCompressed();
    },

    _valueForSize: function(size)
    {
        // If we don't have a value, we set an em-dash to keep the row from hiding.
        // This keeps the UI from shifting around as data comes in.
        const emDash = "\u2014";
        return size > 0 ? Number.bytesToString(size) : emDash;
    },

    _refreshCompressed: function()
    {
        this._compressedRow.value = this._resource.compressed ? WebInspector.UIString("Yes") : WebInspector.UIString("No");
        this._compressionRow.value = this._resource.compressed ? WebInspector.UIString("%.2f\u00d7").format(this._resource.size / this._resource.encodedSize) : null;
    },

    _refreshDecodedSize: function()
    {
        if (!this._resource)
            return;

        this._encodedSizeRow.value = this._valueForSize(this._resource.encodedSize);
        this._decodedSizeRow.value = this._valueForSize(this._resource.size);

        this._refreshCompressed();
    },

    _refreshTransferSize: function()
    {
        if (!this._resource)
            return;

        this._encodedSizeRow.value = this._valueForSize(this._resource.encodedSize);
        this._transferSizeRow.value = this._valueForSize(this._resource.transferSize);

        this._refreshCompressed();
    },

    _refreshRequestHeaders: function()
    {
        if (!this._resource)
            return;

        this._requestHeadersRow.dataGrid = this._createNameValueDataGrid(this._resource.requestHeaders);
    },

    _refreshResponseHeaders: function()
    {
        if (!this._resource)
            return;

        this._responseHeadersRow.dataGrid = this._createNameValueDataGrid(this._resource.responseHeaders);
    },

    _createNameValueDataGrid: function(data)
    {
        if (!data || data instanceof Array ? !data.length : isEmptyObject(data))
            return null;

        var dataGrid = new WebInspector.DataGrid({
            name: {title: WebInspector.UIString("Name"), width: "30%", sortable: true},
            value: {title: WebInspector.UIString("Value"), sortable: true}
        });

        function addDataGridNode(nodeValue)
        {
            console.assert(typeof nodeValue.name === "string");
            console.assert(typeof nodeValue.value === "string");

            var node = new WebInspector.DataGridNode({name: nodeValue.name, value: nodeValue.value}, false);
            node.selectable = true;
            dataGrid.appendChild(node);
        }

        if (data instanceof Array) {
            for (var i = 0; i < data.length; ++i)
                addDataGridNode(data[i]);
        } else {
            for (var name in data)
                addDataGridNode({name: name, value: data[name] || ""});
        }

        dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, sortDataGrid, this);

        function sortDataGrid()
        {
            var nodes = dataGrid.children.slice();
            var sortColumnIdentifier = dataGrid.sortColumnIdentifier;
            var sortDirection = dataGrid.sortOrder === "ascending" ? 1 : -1;

            function comparator(a, b)
            {
                var item1 = a.data[sortColumnIdentifier];
                var item2 = b.data[sortColumnIdentifier];
                return sortDirection * item1.localeCompare(item2);
            }

            nodes.sort(comparator);

            dataGrid.removeChildren();
            for (var i = 0; i < nodes.length; i++)
                dataGrid.appendChild(nodes[i]);
        }

        return dataGrid;
    },

    _refreshImageSizeSection: function()
    {
        var resource = this._resource;

        if (!resource)
            return;
        
        // Hide the section if we're not dealing with an image or if the load failed.
        if (resource.type !== WebInspector.Resource.Type.Image || resource.failed) {
            var imageSectionElement = this._imageSizeSection.element;
            if (imageSectionElement.parentNode)
                this.element.removeChild(imageSectionElement);
            return;
        }
        
        // Ensure the section is displayed, right before the "Location" section.
        this.element.insertBefore(this._imageSizeSection.element, this._locationSection.element);
        
        // Get the metrics for this resource and fill in the metrics rows with that information.
        resource.getImageSize(function(size) {
            this._imageWidthRow.value = WebInspector.UIString("%fpx").format(size.width);
            this._imageHeightRow.value = WebInspector.UIString("%fpx").format(size.height);
        }.bind(this));
    },

    _goToRequestDataClicked: function()
    {
        WebInspector.resourceSidebarPanel.showResourceRequest(this._resource);
    },

    _refreshRequestDataSection: function()
    {
        var resource = this._resource;

        if (!resource)
            return;

        // Hide the section if we're not dealing with a request with data.
        var requestData = resource.requestData;
        if (!requestData) {
            this._requestDataSection.element.remove();
            return;
        }

        // Ensure the section is displayed, right before the "Request Headers" section.
        this.element.insertBefore(this._requestDataSection.element, this._requestHeadersSection.element);

        var requestDataContentType = resource.requestDataContentType || "";
        if (requestDataContentType && requestDataContentType.match(/^application\/x-www-form-urlencoded\s*(;.*)?$/i)) {
            // Simple form data that should be parsable like a query string.
            var parametersRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Parameters"));
            parametersRow.dataGrid = this._createNameValueDataGrid(parseQueryString(requestData, true));

            this._requestDataSection.groups = [new WebInspector.DetailsSectionGroup([parametersRow])];
            return;
        }

        // Not simple form data, so we can really only show the size and type here.
        // FIXME: Add a go-to arrow here to show the data in the content browser.

        var typeParts = requestDataContentType.split(/\s*;\s*/);
        console.assert(typeParts.length >= 1);

        var mimeType = typeParts[0];
        var boundary = null;
        var encoding = null;

        for (var i = 1; i < typeParts.length; ++i) {
            var subparts = typeParts[i].split(/\s*=\s*/);
            if (subparts.length !== 2)
                continue;

            if (subparts[0].toLowerCase() === "boundary")
                boundary = subparts[1];
            else if (subparts[0].toLowerCase() === "charset")
                encoding = subparts[1].replace("^\"|\"$", ""); // Trim quotes.
        }

        var rows = [];

        var mimeTypeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("MIME Type"));
        mimeTypeRow.value = mimeType;
        rows.push(mimeTypeRow);

        if (boundary) {
            var boundryRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Boundary"));
            boundryRow.value = boundary;
            rows.push(boundryRow);
        }

        if (encoding) {
            var encodingRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Encoding"));
            encodingRow.value = encoding;
            rows.push(encodingRow);
        }

        var sizeValue = Number.bytesToString(requestData.length);

        var dataValue = document.createDocumentFragment();

        dataValue.appendChild(document.createTextNode(sizeValue));

        var goToButton = dataValue.appendChild(WebInspector.createGoToArrowButton());
        goToButton.addEventListener("click", this._goToRequestDataClicked.bind(this));

        var dataRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Data"));
        dataRow.value = dataValue;
        rows.push(dataRow);

        this._requestDataSection.groups = [new WebInspector.DetailsSectionGroup(rows)];
    }
};

WebInspector.ResourceDetailsSidebarPanel.prototype.__proto__ = WebInspector.DetailsSidebarPanel.prototype;
