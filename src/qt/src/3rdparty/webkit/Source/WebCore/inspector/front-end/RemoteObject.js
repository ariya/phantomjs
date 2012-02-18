/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

WebInspector.RemoteObject = function(objectId, type, description, hasChildren)
{
    this._objectId = objectId;
    this._type = type;
    this._description = description;
    this._hasChildren = hasChildren;
}

WebInspector.RemoteObject.fromPrimitiveValue = function(value)
{
    return new WebInspector.RemoteObject(null, typeof value, value);
}

WebInspector.RemoteObject.fromLocalObject = function(value)
{
    return new WebInspector.LocalJSONObject(value);
}

WebInspector.RemoteObject.resolveNode = function(node, callback)
{
    function mycallback(error, object)
    {
        if (!callback)
            return;

        if (error || !object)
            callback(null);
        else
            callback(WebInspector.RemoteObject.fromPayload(object));
    }
    DOMAgent.resolveNode(node.id, mycallback);
}

WebInspector.RemoteObject.fromPayload = function(payload)
{
    if (typeof payload === "object")
        return new WebInspector.RemoteObject(payload.objectId, payload.type, payload.description, payload.hasChildren);
    // FIXME: make sure we only get here with real payloads in the new DebuggerAgent.js.
    return payload;
}

WebInspector.RemoteObject.type = function(remoteObject)
{
    if (remoteObject === null)
        return "null";

    var type = typeof remoteObject;
    if (type !== "object" && type !== "function")
        return type;

    return remoteObject.type;
}

WebInspector.RemoteObject.prototype = {
    get objectId()
    {
        return this._objectId;
    },

    get type()
    {
        return this._type;
    },

    get description()
    {
        return this._description;
    },

    get hasChildren()
    {
        return this._hasChildren;
    },

    isError: function()
    {
        return this._type === "error";
    },

    getOwnProperties: function(callback)
    {
        this._getProperties(false, callback);
    },

    getAllProperties: function(callback)
    {
        this._getProperties(true, callback);
    },

    _getProperties: function(ignoreHasOwnProperty, callback)
    {
        if (!this._objectId) {
            callback([]);
            return;
        }
        function remoteObjectBinder(error, properties)
        {
            if (error)
                return;
            for (var i = 0; properties && i < properties.length; ++i)
                properties[i].value = WebInspector.RemoteObject.fromPayload(properties[i].value);
            callback(properties);
        }
        RuntimeAgent.getProperties(this._objectId, !!ignoreHasOwnProperty, remoteObjectBinder);
    },

    setPropertyValue: function(name, value, callback)
    {
        if (!this._objectId) {
            callback("Can't get a property of non-object.");
            return;
        }
        RuntimeAgent.setPropertyValue(this._objectId, name, value, callback);
    },

    pushNodeToFrontend: function(callback)
    {
        if (this._objectId)
            WebInspector.domAgent.pushNodeToFrontend(this._objectId, callback);
        else
            callback(0);
    },

    evaluate: function(expression, callback)
    {
        RuntimeAgent.evaluateOn(this._objectId, expression, callback);
    },

    release: function()
    {
        RuntimeAgent.releaseObject(this._objectId);
    }
}

WebInspector.RemoteObjectProperty = function(name, value)
{
    this.name = name;
    this.value = value;
}

// The below is a wrapper around a local object that provides an interface comaptible
// with RemoteObject, to be used by the UI code (primarily ObjectPropertiesSection).
// Note that only JSON-compliant objects are currently supported, as there's no provision
// for traversing prototypes, extracting class names via constuctor, handling properties
// or functions.

WebInspector.LocalJSONObject = function(value)
{
    this._value = value;
}

WebInspector.LocalJSONObject.prototype = {
    get description()
    {
        if (this._cachedDescription)
            return this._cachedDescription;

        var type = this.type;

        switch (type) {
            case "array":
                function formatArrayItem(property)
                {
                    return property.value.description;
                }
                this._cachedDescription = this._concatenate("[", "]", formatArrayItem);
                break;
            case "object":
                function formatObjectItem(property)
                {
                    return property.name + ":" + property.value.description;
                }
                this._cachedDescription = this._concatenate("{", "}", formatObjectItem);
                break;
            default:
                this._cachedDescription = String(this._value);
        }
        return this._cachedDescription;
    },

    _concatenate: function(prefix, suffix, formatProperty)
    {
        const previewChars = 100;

        var buffer = prefix;
        var children = this._children();
        for (var i = 0; i < children.length; ++i) {
            var itemDescription = formatProperty(children[i]);
            if (buffer.length + itemDescription.length > previewChars) {
                buffer += ",\u2026";
                break;
            }
            if (i)
                buffer += ", ";
            buffer += itemDescription;
        }
        buffer += suffix;
        return buffer;
    },

    get type()
    {
        if (this._value === null)
            return "null";
        if (this._value instanceof Array)
            return "array";
        return typeof this._value;
    },

    get hasChildren()
    {
        return typeof this._value === "object" && this._value !== null && Object.keys(this._value).length;
    },

    getOwnProperties: function(callback)
    {
        callback(this._children());
    },

    getAllProperties: function(callback)
    {
        callback(this._children());
    },

    _children: function()
    {
        function buildProperty(propName)
        {
            return new WebInspector.RemoteObjectProperty(propName, new WebInspector.LocalJSONObject(this._value[propName]));
        }
        if (!this._cachedChildren)
            this._cachedChildren = Object.keys(this._value).map(buildProperty.bind(this));
        return this._cachedChildren;
    },

    isError: function()
    {
        return false;
    }
}
