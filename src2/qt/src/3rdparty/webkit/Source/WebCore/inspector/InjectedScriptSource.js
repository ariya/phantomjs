/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

(function (InjectedScriptHost, inspectedWindow, injectedScriptId) {

function bind(thisObject, memberFunction)
{
    var func = memberFunction;
    var args = Array.prototype.slice.call(arguments, 2);
    function bound()
    {
        return func.apply(thisObject, args.concat(Array.prototype.slice.call(arguments, 0)));
    }
    bound.toString = function() {
        return "bound: " + func;
    };
    return bound;
}

var InjectedScript = function()
{
    this._lastBoundObjectId = 1;
    this._idToWrappedObject = {};
    this._idToObjectGroupName = {};
    this._objectGroups = {};
}

InjectedScript.prototype = {
    wrapObject: function(object, groupName, canAccessInspectedWindow)
    {
        if (canAccessInspectedWindow)
            return this._wrapObject(object, groupName);
        var result = {};
        result.type = typeof object;
        result.description = this._toString(object);
        return result;
    },

    inspectNode: function(object)
    {
        this._inspect(object);
    },

    _inspect: function(object)
    {
        if (arguments.length === 0)
            return;

        var objectId = this._wrapObject(object, "");
        var hints = {};

        switch (injectedScript._describe(object)) {
            case "Database":
                var databaseId = InjectedScriptHost.databaseId(object)
                if (databaseId)
                    hints.databaseId = databaseId;
                break;
            case "Storage":
                var storageId = InjectedScriptHost.storageId(object)
                if (storageId)
                    hints.domStorageId = storageId;
                break;
        }
        InjectedScriptHost.inspect(objectId, hints);
        return object;
    },

    // This method cannot throw.
    _wrapObject: function(object, objectGroupName)
    {
        try {
            if (typeof object === "object" || typeof object === "function" || this._isHTMLAllCollection(object)) {
                var id = this._lastBoundObjectId++;
                this._idToWrappedObject[id] = object;
                var objectId = "{\"injectedScriptId\":" + injectedScriptId + ",\"id\":" + id + "}";
                if (objectGroupName) {
                    var group = this._objectGroups[objectGroupName];
                    if (!group) {
                        group = [];
                        this._objectGroups[objectGroupName] = group;
                    }
                    group.push(id);
                    this._idToObjectGroupName[id] = objectGroupName;
                }
            }
            return InjectedScript.RemoteObject.fromObject(object, objectId);
        } catch (e) {
            return InjectedScript.RemoteObject.fromException(e);
        }
    },

    _parseObjectId: function(objectId)
    {
        return eval("(" + objectId + ")");
    },

    releaseObjectGroup: function(objectGroupName)
    {
        var group = this._objectGroups[objectGroupName];
        if (!group)
            return;
        for (var i = 0; i < group.length; i++)
            this._releaseObject(group[i]);
        delete this._objectGroups[objectGroupName];
    },

    dispatch: function(methodName, args)
    {
        var argsArray = eval("(" + args + ")");
        var result = this[methodName].apply(this, argsArray);
        if (typeof result === "undefined") {
            inspectedWindow.console.error("Web Inspector error: InjectedScript.%s returns undefined", methodName);
            result = null;
        }
        return result;
    },

    getProperties: function(objectId, ignoreHasOwnProperty)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        var objectGroupName = this._idToObjectGroupName[parsedObjectId.id];

        if (!this._isDefined(object))
            return false;
        var properties = [];

        var propertyNames = ignoreHasOwnProperty ? this._getPropertyNames(object) : Object.getOwnPropertyNames(object);
        if (!ignoreHasOwnProperty && object.__proto__)
            propertyNames.push("__proto__");

        // Go over properties, prepare results.
        for (var i = 0; i < propertyNames.length; ++i) {
            var propertyName = propertyNames[i];

            var property = {};
            property.name = propertyName + "";
            var isGetter = object["__lookupGetter__"] && object.__lookupGetter__(propertyName);
            if (!isGetter) {
                try {
                    var value = object[propertyName];
                } catch(e) {
                    var value = e;
                    property.wasThrown = true;
                }
                property.value = this._wrapObject(value, objectGroupName);
            } else {
                // FIXME: this should show something like "getter" (bug 16734).
                property.value = InjectedScript.RemoteObject.fromObject("\u2014"); // em dash
                property.isGetter = true;
            }
            properties.push(property);
        }
        return properties;
    },

    setPropertyValue: function(objectId, propertyName, expression)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        if (!this._isDefined(object))
            return "Object with given id not found";

        var expressionLength = expression.length;
        if (!expressionLength) {
            delete object[propertyName];
            return propertyName in object ? "Cound not delete property." : undefined;
        }

        try {
            // Surround the expression in parenthesis so the result of the eval is the result
            // of the whole expression not the last potential sub-expression.

            // There is a regression introduced here: eval is now happening against global object,
            // not call frame while on a breakpoint.
            // TODO: bring evaluation against call frame back.
            var result = inspectedWindow.eval("(" + expression + ")");
            // Store the result in the property.
            object[propertyName] = result;
        } catch(e) {
            try {
                var result = inspectedWindow.eval("\"" + expression.replace(/"/g, "\\\"") + "\"");
                object[propertyName] = result;
            } catch(e) {
                return e.toString();
            }
        }
    },

    releaseObject: function(objectId)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        this._releaseObject(parsedObjectId.id);
    },

    _releaseObject: function(id)
    {
        delete this._idToWrappedObject[id];
        delete this._idToObjectGroupName[id];
    },

    _populatePropertyNames: function(object, resultSet)
    {
        for (var o = object; o; o = o.__proto__) {
            try {
                var names = Object.getOwnPropertyNames(o);
                for (var i = 0; i < names.length; ++i)
                    resultSet[names[i]] = true;
            } catch (e) {
            }
        }
    },

    _getPropertyNames: function(object, resultSet)
    {
        var propertyNameSet = {};
        this._populatePropertyNames(object, propertyNameSet);
        return Object.keys(propertyNameSet);
    },

    evaluate: function(expression, objectGroup, injectCommandLineAPI)
    {
        return this._evaluateAndWrap(inspectedWindow.eval, inspectedWindow, expression, objectGroup, false, injectCommandLineAPI);
    },

    evaluateOn: function(objectId, expression)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        if (!object)
            return "Could not find object with given id";
        try {
            inspectedWindow.console._objectToEvaluateOn = object;
            return this._evaluateAndWrap(inspectedWindow.eval, inspectedWindow, "(function() {" + expression + "}).call(window.console._objectToEvaluateOn)", parsedObjectId.objectGroup, false, false);
        } finally {
            delete inspectedWindow.console._objectToEvaluateOn;
        }
    },

    _evaluateAndWrap: function(evalFunction, object, expression, objectGroup, isEvalOnCallFrame, injectCommandLineAPI)
    {
        try {
            return { wasThrown: false,
                     result: this._wrapObject(this._evaluateOn(evalFunction, object, expression, isEvalOnCallFrame, injectCommandLineAPI), objectGroup) };
        } catch (e) {
            return { wasThrown: true,
                     result: this._wrapObject(e, objectGroup) };
        }
    },

    _evaluateOn: function(evalFunction, object, expression, isEvalOnCallFrame, injectCommandLineAPI)
    {
        // Only install command line api object for the time of evaluation.
        // Surround the expression in with statements to inject our command line API so that
        // the window object properties still take more precedent than our API functions.

        try {
            if (injectCommandLineAPI && inspectedWindow.console) {
                inspectedWindow.console._commandLineAPI = new CommandLineAPI(this._commandLineAPIImpl, isEvalOnCallFrame ? object : null);
                expression = "with ((window && window.console && window.console._commandLineAPI) || {}) {\n" + expression + "\n}";
            }
            return evalFunction.call(object, expression);
        } finally {
            if (injectCommandLineAPI && inspectedWindow.console)
                delete inspectedWindow.console._commandLineAPI;
        }
    },

    wrapCallFrames: function(callFrame)
    {
        if (!callFrame)
            return false;

        var result = [];
        var depth = 0;
        do {
            result.push(new InjectedScript.CallFrameProxy(depth++, callFrame));
            callFrame = callFrame.caller;
        } while (callFrame);
        return result;
    },

    evaluateOnCallFrame: function(topCallFrame, callFrameId, expression, objectGroup, injectCommandLineAPI)
    {
        var callFrame = this._callFrameForId(topCallFrame, callFrameId);
        if (!callFrame)
            return "Could not find call frame with given id";
        return this._evaluateAndWrap(callFrame.evaluate, callFrame, expression, objectGroup, true, injectCommandLineAPI);
    },

    _callFrameForId: function(topCallFrame, callFrameId)
    {
        var parsedCallFrameId = eval("(" + callFrameId + ")");
        var ordinal = parsedCallFrameId.ordinal;
        var callFrame = topCallFrame;
        while (--ordinal >= 0 && callFrame)
            callFrame = callFrame.caller;
        return callFrame;
    },

    _objectForId: function(objectId)
    {
        return this._idToWrappedObject[objectId.id];
    },

    nodeForObjectId: function(objectId)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        if (!object || this._type(object) !== "node")
            return null;
        return object;
    },

    _isDefined: function(object)
    {
        return object || this._isHTMLAllCollection(object);
    },

    _isHTMLAllCollection: function(object)
    {
        // document.all is reported as undefined, but we still want to process it.
        return (typeof object === "undefined") && InjectedScriptHost.isHTMLAllCollection(object);
    },

    _type: function(obj)
    {
        if (obj === null)
            return "null";

        var type = typeof obj;
        if (type !== "object" && type !== "function") {
            // FIXME(33716): typeof document.all is always 'undefined'.
            if (this._isHTMLAllCollection(obj))
                return "array";
            return type;
        }

        var preciseType = InjectedScriptHost.type(obj);
        if (preciseType)
            return preciseType;

        // FireBug's array detection.
        try {
            if (isFinite(obj.length) && typeof obj.splice === "function")
                return "array";
            if (isFinite(obj.length) && typeof obj.callee === "function") // arguments.
                return "array";
        } catch (e) {
        }

        // If owning frame has navigated to somewhere else window properties will be undefined.
        // In this case just return result of the typeof.
        return type;
    },

    _describe: function(obj)
    {
        var type = this._type(obj);

        switch (type) {
        case "object":
            // Fall through.
        case "node":
            var result = InjectedScriptHost.internalConstructorName(obj);
            if (result === "Object") {
                // In Chromium DOM wrapper prototypes will have Object as their constructor name,
                // get the real DOM wrapper name from the constructor property.
                var constructorName = obj.constructor && obj.constructor.name;
                if (constructorName)
                    return constructorName;
            }
            return result;
        case "array":
            var className = InjectedScriptHost.internalConstructorName(obj);
            if (typeof obj.length === "number")
                className += "[" + obj.length + "]";
            return className;
        case "string":
            return obj;
        case "function":
            // Fall through.
        default:
            return this._toString(obj);
        }
    },

    _toString: function(obj)
    {
        // We don't use String(obj) because inspectedWindow.String is undefined if owning frame navigated to another page.
        return "" + obj;
    }
}

var injectedScript = new InjectedScript();

InjectedScript.RemoteObject = function(objectId, type, description, hasChildren)
{
    if (objectId) {
        this.objectId = objectId;
        this.hasChildren = hasChildren;
    }
    this.type = type;
    this.description = description;
}

InjectedScript.RemoteObject.fromException = function(e)
{
    try {
        var description = injectedScript._describe(e);
    } catch (ex) {
        var description = "<failed to convert exception to string>";
    }
    return new InjectedScript.RemoteObject(null, "string", "[ Exception: " + description + " ]");
}

// This method may throw
InjectedScript.RemoteObject.fromObject = function(object, objectId)
{
    var type = injectedScript._type(object);
    var rawType = typeof object;
    var hasChildren = (rawType === "object" && object !== null && (!!Object.getOwnPropertyNames(object).length || !!object.__proto__)) || rawType === "function";
    var description = "";
    var description = injectedScript._describe(object);
    return new InjectedScript.RemoteObject(objectId, type, description, hasChildren);
}

InjectedScript.CallFrameProxy = function(ordinal, callFrame)
{
    this.id = "{\"ordinal\":" + ordinal + ",\"injectedScriptId\":" + injectedScriptId + "}";
    this.functionName = (callFrame.type === "function" ? callFrame.functionName : "");
    this.location = { sourceID: String(callFrame.sourceID), lineNumber: callFrame.line, columnNumber: callFrame.column };
    this.scopeChain = this._wrapScopeChain(callFrame);
}

InjectedScript.CallFrameProxy.prototype = {
    _wrapScopeChain: function(callFrame)
    {
        const GLOBAL_SCOPE = 0;
        const LOCAL_SCOPE = 1;
        const WITH_SCOPE = 2;
        const CLOSURE_SCOPE = 3;
        const CATCH_SCOPE = 4;

        var scopeTypeNames = {};
        scopeTypeNames[GLOBAL_SCOPE] = "global";
        scopeTypeNames[LOCAL_SCOPE] = "local";
        scopeTypeNames[WITH_SCOPE] = "with";
        scopeTypeNames[CLOSURE_SCOPE] = "closure";
        scopeTypeNames[CATCH_SCOPE] = "catch";

        var scopeChain = callFrame.scopeChain;
        var scopeChainProxy = [];
        var foundLocalScope = false;
        for (var i = 0; i < scopeChain.length; i++) {
            var scope = {};
            scope.object = injectedScript._wrapObject(scopeChain[i], "backtrace");

            var scopeType = callFrame.scopeType(i);
            scope.type = scopeTypeNames[scopeType];

            if (scopeType === LOCAL_SCOPE)
                scope.this = injectedScript._wrapObject(callFrame.thisObject, "backtrace");

            scopeChainProxy.push(scope);
        }
        return scopeChainProxy;
    }
}

function CommandLineAPI(commandLineAPIImpl, callFrame)
{
    function inScopeVariables(member)
    {
        if (!callFrame)
            return false;

        var scopeChain = callFrame.scopeChain;
        for (var i = 0; i < scopeChain.length; ++i) {
            if (member in scopeChain[i])
                return true;
        }
        return false;
    }

    for (var i = 0; i < CommandLineAPI.members_.length; ++i) {
        var member = CommandLineAPI.members_[i];
        if (member in inspectedWindow || inScopeVariables(member))
            continue;

        this[member] = bind(commandLineAPIImpl, commandLineAPIImpl[member]);
    }

    for (var i = 0; i < 5; ++i) {
        var member = "$" + i;
        if (member in inspectedWindow || inScopeVariables(member))
            continue;

        this.__defineGetter__("$" + i, bind(commandLineAPIImpl, commandLineAPIImpl._inspectedNode, i));
    }
}

CommandLineAPI.members_ = [
    "$", "$$", "$x", "dir", "dirxml", "keys", "values", "profile", "profileEnd",
    "monitorEvents", "unmonitorEvents", "inspect", "copy", "clear"
];

function CommandLineAPIImpl()
{
}

CommandLineAPIImpl.prototype = {
    $: function()
    {
        return document.getElementById.apply(document, arguments)
    },

    $$: function()
    {
        return document.querySelectorAll.apply(document, arguments)
    },

    $x: function(xpath, context)
    {
        var nodes = [];
        try {
            var doc = (context && context.ownerDocument) || inspectedWindow.document;
            var results = doc.evaluate(xpath, context || doc, null, XPathResult.ANY_TYPE, null);
            var node;
            while (node = results.iterateNext())
                nodes.push(node);
        } catch (e) {
        }
        return nodes;
    },

    dir: function()
    {
        return console.dir.apply(console, arguments)
    },

    dirxml: function()
    {
        return console.dirxml.apply(console, arguments)
    },

    keys: function(object)
    {
        return Object.keys(object);
    },

    values: function(object)
    {
        var result = [];
        for (var key in object)
            result.push(object[key]);
        return result;
    },

    profile: function()
    {
        return console.profile.apply(console, arguments)
    },

    profileEnd: function()
    {
        return console.profileEnd.apply(console, arguments)
    },

    monitorEvents: function(object, types)
    {
        if (!object || !object.addEventListener || !object.removeEventListener)
            return;
        types = this._normalizeEventTypes(types);
        for (var i = 0; i < types.length; ++i) {
            object.removeEventListener(types[i], this._logEvent, false);
            object.addEventListener(types[i], this._logEvent, false);
        }
    },

    unmonitorEvents: function(object, types)
    {
        if (!object || !object.addEventListener || !object.removeEventListener)
            return;
        types = this._normalizeEventTypes(types);
        for (var i = 0; i < types.length; ++i)
            object.removeEventListener(types[i], this._logEvent, false);
    },

    inspect: function(object)
    {
        return injectedScript._inspect(object);
    },

    copy: function(object)
    {
        if (injectedScript._type(object) === "node")
            object = object.outerHTML;
        InjectedScriptHost.copyText(object);
    },

    clear: function()
    {
        InjectedScriptHost.clearConsoleMessages();
    },

    _inspectedNode: function(num)
    {
        return InjectedScriptHost.inspectedNode(num);
    },

    _normalizeEventTypes: function(types)
    {
        if (typeof types === "undefined")
            types = [ "mouse", "key", "load", "unload", "abort", "error", "select", "change", "submit", "reset", "focus", "blur", "resize", "scroll" ];
        else if (typeof types === "string")
            types = [ types ];

        var result = [];
        for (var i = 0; i < types.length; i++) {
            if (types[i] === "mouse")
                result.splice(0, 0, "mousedown", "mouseup", "click", "dblclick", "mousemove", "mouseover", "mouseout");
            else if (types[i] === "key")
                result.splice(0, 0, "keydown", "keyup", "keypress");
            else
                result.push(types[i]);
        }
        return result;
    },

    _logEvent: function(event)
    {
        console.log(event.type, event);
    }
}

injectedScript._commandLineAPIImpl = new CommandLineAPIImpl();
return injectedScript;
})
