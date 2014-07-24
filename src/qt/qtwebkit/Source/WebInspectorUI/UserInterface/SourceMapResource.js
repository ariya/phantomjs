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

WebInspector.SourceMapResource = function(url, sourceMap)
{
    WebInspector.Resource.call(this, url, null);

    console.assert(url);
    console.assert(sourceMap);

    this._sourceMap = sourceMap;
};

WebInspector.SourceMapResource.prototype = {
    constructor: WebInspector.SourceMapResource,

    // Public

    get sourceMap()
    {
        return this._sourceMap;
    },

    get sourceMapDisplaySubpath()
    {
        var sourceMappingBasePathURLComponents = this._sourceMap.sourceMappingBasePathURLComponents;
        var resourceURLComponents = this.urlComponents;

        // Different schemes / hosts. Return the host + path of this resource.
        if (resourceURLComponents.scheme !== sourceMappingBasePathURLComponents.scheme || resourceURLComponents.host !== sourceMappingBasePathURLComponents.host)
            return resourceURLComponents.host + (resourceURLComponents.port ? (":" + resourceURLComponents.port) : "") + resourceURLComponents.path;

        // Same host, but not a subpath of the base. This implies a ".." in the relative path.
        if (!resourceURLComponents.path.startsWith(sourceMappingBasePathURLComponents.path))
            return relativePath(resourceURLComponents.path, sourceMappingBasePathURLComponents.path);

        // Same host. Just a subpath of the base.
        return resourceURLComponents.path.substring(sourceMappingBasePathURLComponents.path.length, resourceURLComponents.length);
    },

    canRequestContentFromBackend: function()
    {
        return !this.finished;
    },

    requestContentFromBackend: function(callback)
    {
        function requestAsyncCallback(body, base64encoded, mimeType)
        {
            if (body === null) {
                this.markAsFailed();
                callback("Failed to load resource", body, base64encoded);
                return;
            }

            var oldType = this._type;
            var oldMIMEType = this._mimeType;

            this._mimeType = mimeType;
            this._type = WebInspector.Resource.Type.fromMIMEType(this._mimeType);

            if (oldMIMEType !== mimeType)
                this.dispatchEventToListeners(WebInspector.Resource.Event.MIMETypeDidChange, {oldMIMEType: oldMIMEType});

            if (oldType !== this._type)
                this.dispatchEventToListeners(WebInspector.Resource.Event.TypeDidChange, {oldType: oldType});

            this.markAsFinished();

            callback(null, body, base64encoded);
        }

        this._requestResourceAsynchronously(requestAsyncCallback.bind(this));
        return true;
    },

    createSourceCodeLocation: function(lineNumber, columnNumber)
    {
        // SourceCodeLocations are always constructed with raw resources and raw locations. Lookup the raw location.
        var entry = this._sourceMap.findEntryReversed(this.url, lineNumber);
        var rawLineNumber = entry[0];
        var rawColumnNumber = entry[1];

        // If the raw location is an inline script we need to include that offset.
        var originalSourceCode = this._sourceMap.originalSourceCode;
        if (originalSourceCode instanceof WebInspector.Script) {
            if (rawLineNumber === 0)
                rawColumnNumber += originalSourceCode.range.startColumn;
            rawLineNumber += originalSourceCode.range.startLine;
        }

        // Create the SourceCodeLocation and since we already know the the mapped location set it directly.
        var location = originalSourceCode.createSourceCodeLocation(rawLineNumber, rawColumnNumber);
        location._setMappedLocation(this, lineNumber, columnNumber);
        return location;
    },

    createSourceCodeTextRange: function(textRange)
    {
        // SourceCodeTextRanges are always constructed with raw resources and raw locations.
        // However, we can provide the most accurate mapped locations in construction.
        var startSourceCodeLocation = this.createSourceCodeLocation(textRange.startLine, textRange.startColumn);
        var endSourceCodeLocation = this.createSourceCodeLocation(textRange.endLine, textRange.endColumn);
        return new WebInspector.SourceCodeTextRange(this._sourceMap.originalSourceCode, startSourceCodeLocation, endSourceCodeLocation);
    },

    // Private

    _requestResourceAsynchronously: function(callback)
    {
        // FIXME: <rdar://problem/13238886> Source Maps: Frontend needs asynchronous resource loading of content + mime type

        function async()
        {
            var body = this._sourceMap.sourceContent(this.url) || InspectorFrontendHost.loadResourceSynchronously(this.url);
            if (body === undefined)
                body = null;
            var fileExtension = WebInspector.fileExtensionForURL(this.url);
            var mimeType = WebInspector.mimeTypeForFileExtension(fileExtension, true) || "text/javascript";
            callback(body, false, mimeType);
        }

        setTimeout(async.bind(this), 0);
    }
};

WebInspector.SourceMapResource.prototype.__proto__ = WebInspector.Resource.prototype;
