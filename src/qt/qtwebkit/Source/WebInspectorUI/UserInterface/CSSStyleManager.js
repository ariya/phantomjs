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

WebInspector.CSSStyleManager = function()
{
    WebInspector.Object.call(this);

    CSSAgent.enable();

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasAdded, this._resourceAdded, this);
    WebInspector.Resource.addEventListener(WebInspector.SourceCode.Event.ContentDidChange, this._resourceContentDidChange, this);
    WebInspector.Resource.addEventListener(WebInspector.Resource.Event.TypeDidChange, this._resourceTypeDidChange, this);

    WebInspector.DOMNode.addEventListener(WebInspector.DOMNode.Event.AttributeModified, this._nodeAttributesDidChange, this);
    WebInspector.DOMNode.addEventListener(WebInspector.DOMNode.Event.AttributeRemoved, this._nodeAttributesDidChange, this);
    WebInspector.DOMNode.addEventListener(WebInspector.DOMNode.Event.EnabledPseudoClassesChanged, this._nodePseudoClassesDidChange, this);

    this._colorFormatSetting = new WebInspector.Setting("default-color-format", WebInspector.Color.Format.Original);

    this._styleSheetIdentifierMap = {};
    this._styleSheetFrameURLMap = {};
    this._nodeStylesMap = {};
}

WebInspector.CSSStyleManager.ForceablePseudoClasses = ["active", "focus", "hover", "visited"];

WebInspector.CSSStyleManager.prototype = {
    constructor: WebInspector.CSSStyleManager,

    // Public

    get preferredColorFormat()
    {
        return this._colorFormatSetting.value;
    },

    canForcePseudoClasses: function()
    {
        return !!CSSAgent.forcePseudoState;
    },

    propertyNameHasOtherVendorPrefix: function(name)
    {
        if (!name || name.length < 4 || name.charAt(0) !== "-")
            return false;

        var match = name.match(/^(?:-moz-|-ms-|-o-|-epub-)/);
        if (!match)
            return false;

        return true;
    },

    propertyValueHasOtherVendorKeyword: function(value)
    {
        var match = value.match(/(?:-moz-|-ms-|-o-|-epub-)[-\w]+/);
        if (!match)
            return false;

        return true;
    },

    canonicalNameForPropertyName: function(name)
    {
        if (!name || name.length < 8 || name.charAt(0) !== "-")
            return name;

        var match = name.match(/^(?:-webkit-|-khtml-|-apple-)(.+)/);
        if (!match)
            return name;

        return match[1];
    },

    styleSheetForIdentifier: function(id)
    {
        if (id in this._styleSheetIdentifierMap)
            return this._styleSheetIdentifierMap[id];

        var styleSheet = new WebInspector.CSSStyleSheet(id);
        this._styleSheetIdentifierMap[id] = styleSheet;
        return styleSheet;
    },

    stylesForNode: function(node)
    {
        if (node.id in this._nodeStylesMap)
            return this._nodeStylesMap[node.id];

        var styles = new WebInspector.DOMNodeStyles(node);
        this._nodeStylesMap[node.id] = styles;
        return styles;
    },

    // Protected

    mediaQueryResultChanged: function()
    {
        // Called from WebInspector.CSSObserver.

        for (var key in this._nodeStylesMap)
            this._nodeStylesMap[key].mediaQueryResultDidChange();
    },

    styleSheetChanged: function(styleSheetIdentifier)
    {
        // Called from WebInspector.CSSObserver.

        var styleSheet = this.styleSheetForIdentifier(styleSheetIdentifier);
        console.assert(styleSheet);

        styleSheet.noteContentDidChange();

        this._updateResourceContent(styleSheet);
    },

    // Private

    _nodePseudoClassesDidChange: function(event)
    {
        var node = event.target;

        for (var key in this._nodeStylesMap) {
            var nodeStyles = this._nodeStylesMap[key];
            if (nodeStyles.node !== node && !nodeStyles.node.isDescendant(node))
                continue;
            nodeStyles.pseudoClassesDidChange(node);
        }
    },

    _nodeAttributesDidChange: function(event)
    {
        var node = event.target;

        for (var key in this._nodeStylesMap) {
            var nodeStyles = this._nodeStylesMap[key];
            if (nodeStyles.node !== node && !nodeStyles.node.isDescendant(node))
                continue;
            nodeStyles.attributeDidChange(node, event.data.name);
        }
    },

    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        // Clear our maps when the main frame navigates.

        this._styleSheetIdentifierMap = {};
        this._styleSheetFrameURLMap = {};
        this._nodeStylesMap = {};
    },

    _resourceAdded: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        var resource = event.data.resource;
        console.assert(resource);

        if (resource.type !== WebInspector.Resource.Type.Stylesheet)
            return;

        this._clearStyleSheetsForResource(resource);
    },

    _resourceTypeDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Resource);

        var resource = event.target;
        if (resource.type !== WebInspector.Resource.Type.Stylesheet)
            return;

        this._clearStyleSheetsForResource(resource);
    },

    _clearStyleSheetsForResource: function(resource)
    {
        // Clear known stylesheets for this URL and frame. This will cause the stylesheets to
        // be updated next time _fetchInfoForAllStyleSheets is called.
        // COMPATIBILITY (iOS 6): The frame's id was not available for the key, so delete just the url too.
        delete this._styleSheetFrameURLMap[this._frameURLMapKey(resource.parentFrame, resource.url)];
        delete this._styleSheetFrameURLMap[resource.url];
    },

    _frameURLMapKey: function(frame, url)
    {
        return (frame ? frame.id + ":" : "") + url;
    },

    _lookupStyleSheetForResource: function(resource, callback)
    {
        this._lookupStyleSheet(resource.parentFrame, resource.url, callback);
    },

    _lookupStyleSheet: function(frame, url, callback)
    {
        console.assert(frame instanceof WebInspector.Frame);

        function syleSheetsFetched()
        {
            callback(this._styleSheetFrameURLMap[key] || this._styleSheetFrameURLMap[url] || null);
        }

        var key = this._frameURLMapKey(frame, url);

        // COMPATIBILITY (iOS 6): The frame's id was not available for the key, so check for just the url too.
        if (key in this._styleSheetFrameURLMap || url in this._styleSheetFrameURLMap)
            callback(this._styleSheetFrameURLMap[key] || this._styleSheetFrameURLMap[url] || null);
        else
            this._fetchInfoForAllStyleSheets(syleSheetsFetched.bind(this));
    },

    _fetchInfoForAllStyleSheets: function(callback)
    {
        console.assert(typeof callback === "function");

        function processStyleSheets(error, styleSheets)
        {
            this._styleSheetFrameURLMap = {};

            if (error) {
                callback();
                return;
            }

            for (var i = 0; i < styleSheets.length; ++i) {
                var styleSheetInfo = styleSheets[i];

                // COMPATIBILITY (iOS 6): The info did not have 'frameId', so make parentFrame null in that case.
                var parentFrame = "frameId" in styleSheetInfo ? WebInspector.frameResourceManager.frameForIdentifier(styleSheetInfo.frameId) : null;

                var styleSheet = this.styleSheetForIdentifier(styleSheetInfo.styleSheetId);
                styleSheet.updateInfo(styleSheetInfo.sourceURL, parentFrame);

                var key = this._frameURLMapKey(parentFrame, styleSheetInfo.sourceURL);
                this._styleSheetFrameURLMap[key] = styleSheet;
            }

            callback();
        }

        CSSAgent.getAllStyleSheets(processStyleSheets.bind(this));
    },

    _resourceContentDidChange: function(event)
    {
        var resource = event.target;
        if (resource === this._ignoreResourceContentDidChangeEventForResource)
            return;

        // Ignore if it isn't a CSS stylesheet.
        if (resource.type !== WebInspector.Resource.Type.Stylesheet || resource.syntheticMIMEType !== "text/css")
            return;

        function applyStyleSheetChanges()
        {
            function styleSheetFound(styleSheet)
            {
                delete resource.__pendingChangeTimeout;

                console.assert(styleSheet);
                if (!styleSheet)
                    return;

                // To prevent updating a TextEditor's content while the user is typing in it we want to
                // ignore the next _updateResourceContent call.
                resource.__ignoreNextUpdateResourceContent = true;

                WebInspector.branchManager.currentBranch.revisionForRepresentedObject(styleSheet).content = resource.content;
            }

            this._lookupStyleSheetForResource(resource, styleSheetFound.bind(this));
        }

        if (resource.__pendingChangeTimeout)
            clearTimeout(resource.__pendingChangeTimeout);
        resource.__pendingChangeTimeout = setTimeout(applyStyleSheetChanges.bind(this), 500);
    },

    _updateResourceContent: function(styleSheet)
    {
        console.assert(styleSheet);

        function fetchedStyleSheetContent(styleSheet, content)
        {
            delete styleSheet.__pendingChangeTimeout;

            console.assert(styleSheet.url);
            if (!styleSheet.url)
                return;

            var resource = null;

            // COMPATIBILITY (iOS 6): The stylesheet did not always have a frame, so fallback to looking
            // for the resource in all frames.
            if (styleSheet.parentFrame)
                resource = styleSheet.parentFrame.resourceForURL(styleSheet.url);
            else
                resource = WebInspector.frameResourceManager.resourceForURL(styleSheet.url);

            if (!resource)
                return;

            // Only try to update stylesheet resources. Other resources, like documents, can contain
            // multiple stylesheets and we don't have the source ranges to update those.
            if (resource.type !== WebInspector.Resource.Type.Stylesheet)
                return;

            if (resource.__ignoreNextUpdateResourceContent) {
                delete resource.__ignoreNextUpdateResourceContent;
                return;
            }

            this._ignoreResourceContentDidChangeEventForResource = resource;
            WebInspector.branchManager.currentBranch.revisionForRepresentedObject(resource).content = content;
            delete this._ignoreResourceContentDidChangeEventForResource;
        }

        function styleSheetReady()
        {
            styleSheet.requestContent(fetchedStyleSheetContent.bind(this));
        }

        function applyStyleSheetChanges()
        {
            if (styleSheet.url)
                styleSheetReady.call(this);
            else
                this._fetchInfoForAllStyleSheets(styleSheetReady.bind(this));
        }

        if (styleSheet.__pendingChangeTimeout)
            clearTimeout(styleSheet.__pendingChangeTimeout);
        styleSheet.__pendingChangeTimeout = setTimeout(applyStyleSheetChanges.bind(this), 500);
    }
}

WebInspector.CSSStyleManager.prototype.__proto__ = WebInspector.Object.prototype;
