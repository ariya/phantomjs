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
 * @param {InjectedScriptHost} InjectedScriptHost
 * @param {Window} inspectedWindow
 * @param {number} injectedScriptId
 */
(function (InjectedScriptHost, inspectedWindow, injectedScriptId) {

var TypeUtils = {
    /**
     * http://www.khronos.org/registry/typedarray/specs/latest/#7
     * @const
     * @type {!Array.<function(new:ArrayBufferView, ArrayBufferView)>}
     */
    _typedArrayClasses: (function(typeNames) {
        var result = [];
        for (var i = 0, n = typeNames.length; i < n; ++i) {
            if (inspectedWindow[typeNames[i]])
                result.push(inspectedWindow[typeNames[i]]);
        }
        return result;
    })(["Int8Array", "Uint8Array", "Uint8ClampedArray", "Int16Array", "Uint16Array", "Int32Array", "Uint32Array", "Float32Array", "Float64Array"]),

    /**
     * @const
     * @type {!Array.<string>}
     */
    _supportedPropertyPrefixes: ["webkit"],

    /**
     * @param {*} array
     * @return {function(new:ArrayBufferView, ArrayBufferView)|null}
     */
    typedArrayClass: function(array)
    {
        var classes = TypeUtils._typedArrayClasses;
        for (var i = 0, n = classes.length; i < n; ++i) {
            if (array instanceof classes[i])
                return classes[i];
        }
        return null;
    },

    /**
     * @param {*} obj
     * @return {*}
     */
    clone: function(obj)
    {
        if (!obj)
            return obj;

        var type = typeof obj;
        if (type !== "object" && type !== "function")
            return obj;

        // Handle Array and ArrayBuffer instances.
        if (typeof obj.slice === "function") {
            console.assert(obj instanceof Array || obj instanceof ArrayBuffer);
            return obj.slice(0);
        }

        var typedArrayClass = TypeUtils.typedArrayClass(obj);
        if (typedArrayClass)
            return new typedArrayClass(/** @type {ArrayBufferView} */ (obj));

        if (obj instanceof HTMLImageElement) {
            var img = /** @type {HTMLImageElement} */ (obj);
            // Special case for Images with Blob URIs: cloneNode will fail if the Blob URI has already been revoked.
            // FIXME: Maybe this is a bug in WebKit core?
            if (/^blob:/.test(img.src))
                return TypeUtils.cloneIntoCanvas(img);
            return img.cloneNode(true);
        }

        if (obj instanceof HTMLCanvasElement)
            return TypeUtils.cloneIntoCanvas(obj);

        if (obj instanceof HTMLVideoElement)
            return TypeUtils.cloneIntoCanvas(obj, obj.videoWidth, obj.videoHeight);

        if (obj instanceof ImageData) {
            var context = TypeUtils._dummyCanvas2dContext();
            // FIXME: suppress type checks due to outdated builtin externs for createImageData.
            var result = (/** @type {?} */ (context)).createImageData(obj);
            for (var i = 0, n = obj.data.length; i < n; ++i)
              result.data[i] = obj.data[i];
            return result;
        }

        console.error("ASSERT_NOT_REACHED: failed to clone object: ", obj);
        return obj;
    },

    /**
     * @param {HTMLImageElement|HTMLCanvasElement|HTMLVideoElement} obj
     * @param {number=} width
     * @param {number=} height
     * @return {HTMLCanvasElement}
     */
    cloneIntoCanvas: function(obj, width, height)
    {
        var canvas = /** @type {HTMLCanvasElement} */ (inspectedWindow.document.createElement("canvas"));
        canvas.width = width || +obj.width;
        canvas.height = height || +obj.height;
        var context = /** @type {CanvasRenderingContext2D} */ (Resource.wrappedObject(canvas.getContext("2d")));
        context.drawImage(obj, 0, 0);
        return canvas;
    },

    /**
     * @param {Object=} obj
     * @return {Object}
     */
    cloneObject: function(obj)
    {
        if (!obj)
            return null;
        var result = {};
        for (var key in obj)
            result[key] = obj[key];
        return result;
    },

    /**
     * @param {!Array.<string>} names
     * @return {!Object.<string, boolean>}
     */
    createPrefixedPropertyNamesSet: function(names)
    {
        var result = Object.create(null);
        for (var i = 0, name; name = names[i]; ++i) {
            result[name] = true;
            var suffix = name.substr(0, 1).toUpperCase() + name.substr(1);
            for (var j = 0, prefix; prefix = TypeUtils._supportedPropertyPrefixes[j]; ++j)
                result[prefix + suffix] = true;
        }
        return result;
    },

    /**
     * @return {CanvasRenderingContext2D}
     */
    _dummyCanvas2dContext: function()
    {
        var context = TypeUtils._dummyCanvas2dContextInstance;
        if (!context) {
            var canvas = /** @type {HTMLCanvasElement} */ (inspectedWindow.document.createElement("canvas"));
            context = /** @type {CanvasRenderingContext2D} */ (Resource.wrappedObject(canvas.getContext("2d")));
            TypeUtils._dummyCanvas2dContextInstance = context;
        }
        return context;
    }
}

/**
 * @interface
 */
function StackTrace()
{
}

StackTrace.prototype = {
    /**
     * @param {number} index
     * @return {{sourceURL: string, lineNumber: number, columnNumber: number}|undefined}
     */
    callFrame: function(index)
    {
    }
}

/**
 * @param {number=} stackTraceLimit
 * @param {Function=} topMostFunctionToIgnore
 * @return {StackTrace}
 */
StackTrace.create = function(stackTraceLimit, topMostFunctionToIgnore)
{
    // FIXME: Support JSC, and maybe other browsers.
    return null;
}

/**
 * @constructor
 */
function Cache()
{
    this.reset();
}

Cache.prototype = {
    /**
     * @return {number}
     */
    size: function()
    {
        return this._size;
    },

    reset: function()
    {
        /** @type {!Object.<number, Object>} */
        this._items = Object.create(null);
        /** @type {number} */
        this._size = 0;
    },

    /**
     * @param {number} key
     * @return {boolean}
     */
    has: function(key)
    {
        return key in this._items;
    },

    /**
     * @param {number} key
     * @return {Object}
     */
    get: function(key)
    {
        return this._items[key];
    },

    /**
     * @param {number} key
     * @param {Object} item
     */
    put: function(key, item)
    {
        if (!this.has(key))
            ++this._size;
        this._items[key] = item;
    }
}

/**
 * @constructor
 * @param {Resource|Object} thisObject
 * @param {string} functionName
 * @param {Array|Arguments} args
 * @param {Resource|*=} result
 * @param {StackTrace=} stackTrace
 */
function Call(thisObject, functionName, args, result, stackTrace)
{
    this._thisObject = thisObject;
    this._functionName = functionName;
    this._args = Array.prototype.slice.call(args, 0);
    this._result = result;
    this._stackTrace = stackTrace || null;

    if (!this._functionName)
        console.assert(this._args.length === 2 && typeof this._args[0] === "string");
}

Call.prototype = {
    /**
     * @return {Resource}
     */
    resource: function()
    {
        return Resource.forObject(this._thisObject);
    },

    /**
     * @return {string}
     */
    functionName: function()
    {
        return this._functionName;
    },

    /**
     * @return {boolean}
     */
    isPropertySetter: function()
    {
        return !this._functionName;
    },
    
    /**
     * @return {!Array}
     */
    args: function()
    {
        return this._args;
    },

    /**
     * @return {*}
     */
    result: function()
    {
        return this._result;
    },

    /**
     * @return {StackTrace}
     */
    stackTrace: function()
    {
        return this._stackTrace;
    },

    /**
     * @param {StackTrace} stackTrace
     */
    setStackTrace: function(stackTrace)
    {
        this._stackTrace = stackTrace;
    },

    /**
     * @param {*} result
     */
    setResult: function(result)
    {
        this._result = result;
    },

    /**
     * @param {string} name
     * @param {Object} attachment
     */
    setAttachment: function(name, attachment)
    {
        if (attachment) {
            /** @type {Object.<string, Object>} */
            this._attachments = this._attachments || Object.create(null);
            this._attachments[name] = attachment;
        } else if (this._attachments)
            delete this._attachments[name];
    },

    /**
     * @param {string} name
     * @return {Object}
     */
    attachment: function(name)
    {
        return this._attachments && this._attachments[name];
    },

    freeze: function()
    {
        if (this._freezed)
            return;
        this._freezed = true;
        for (var i = 0, n = this._args.length; i < n; ++i) {
            // FIXME: freeze the Resources also!
            if (!Resource.forObject(this._args[i]))
                this._args[i] = TypeUtils.clone(this._args[i]);
        }
    },

    /**
     * @param {!Cache} cache
     * @return {!ReplayableCall}
     */
    toReplayable: function(cache)
    {
        this.freeze();
        var thisObject = /** @type {ReplayableResource} */ (Resource.toReplayable(this._thisObject, cache));
        var result = Resource.toReplayable(this._result, cache);
        var args = this._args.map(function(obj) {
            return Resource.toReplayable(obj, cache);
        });
        var attachments = TypeUtils.cloneObject(this._attachments);
        return new ReplayableCall(thisObject, this._functionName, args, result, this._stackTrace, attachments);
    },

    /**
     * @param {!ReplayableCall} replayableCall
     * @param {!Cache} cache
     * @return {!Call}
     */
    replay: function(replayableCall, cache)
    {
        var replayObject = ReplayableResource.replay(replayableCall.replayableResource(), cache);
        var replayArgs = replayableCall.args().map(function(obj) {
            return ReplayableResource.replay(obj, cache);
        });
        var replayResult = undefined;

        if (replayableCall.isPropertySetter())
            replayObject[replayArgs[0]] = replayArgs[1];
        else {
            var replayFunction = replayObject[replayableCall.functionName()];
            console.assert(typeof replayFunction === "function", "Expected a function to replay");
            replayResult = replayFunction.apply(replayObject, replayArgs);
            if (replayableCall.result() instanceof ReplayableResource) {
                var resource = replayableCall.result().replay(cache);
                if (!resource.wrappedObject())
                    resource.setWrappedObject(replayResult);
            }
        }
    
        this._thisObject = replayObject;
        this._functionName = replayableCall.functionName();
        this._args = replayArgs;
        this._result = replayResult;
        this._stackTrace = replayableCall.stackTrace();
        this._freezed = true;
        var attachments = replayableCall.attachments();
        if (attachments)
            this._attachments = TypeUtils.cloneObject(attachments);
        return this;
    }
}

/**
 * @constructor
 * @param {ReplayableResource} thisObject
 * @param {string} functionName
 * @param {Array.<ReplayableResource|*>} args
 * @param {ReplayableResource|*} result
 * @param {StackTrace} stackTrace
 * @param {Object.<string, Object>} attachments
 */
function ReplayableCall(thisObject, functionName, args, result, stackTrace, attachments)
{
    this._thisObject = thisObject;
    this._functionName = functionName;
    this._args = args;
    this._result = result;
    this._stackTrace = stackTrace;
    if (attachments)
        this._attachments = attachments;
}

ReplayableCall.prototype = {
    /**
     * @return {ReplayableResource}
     */
    replayableResource: function()
    {
        return this._thisObject;
    },

    /**
     * @return {string}
     */
    functionName: function()
    {
        return this._functionName;
    },

    /**
     * @return {boolean}
     */
    isPropertySetter: function()
    {
        return !this._functionName;
    },

    /**
     * @return {Array.<ReplayableResource|*>}
     */
    args: function()
    {
        return this._args;
    },

    /**
     * @return {ReplayableResource|*}
     */
    result: function()
    {
        return this._result;
    },

    /**
     * @return {StackTrace}
     */
    stackTrace: function()
    {
        return this._stackTrace;
    },

    /**
     * @return {Object.<string, Object>}
     */
    attachments: function()
    {
        return this._attachments;
    },

    /**
     * @param {string} name
     * @return {Object}
     */
    attachment: function(name)
    {
        return this._attachments && this._attachments[name];
    },

    /**
     * @param {Cache} cache
     * @return {!Call}
     */
    replay: function(cache)
    {
        var call = /** @type {!Call} */ (Object.create(Call.prototype));
        return call.replay(this, cache);
    }
}

/**
 * @constructor
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function Resource(wrappedObject, name)
{
    /** @type {number} */
    this._id = ++Resource._uniqueId;
    /** @type {string} */
    this._name = name || "Resource";
    /** @type {number} */
    this._kindId = Resource._uniqueKindIds[this._name] = (Resource._uniqueKindIds[this._name] || 0) + 1;
    /** @type {ResourceTrackingManager} */
    this._resourceManager = null;
    /** @type {!Array.<Call>} */
    this._calls = [];
    /**
     * This is to prevent GC from collecting associated resources.
     * Otherwise, for example in WebGL, subsequent calls to gl.getParameter()
     * may return a recently created instance that is no longer bound to a
     * Resource object (thus, no history to replay it later).
     *
     * @type {!Object.<string, Resource>}
     */
    this._boundResources = Object.create(null);
    this.setWrappedObject(wrappedObject);
}

/**
 * @type {number}
 */
Resource._uniqueId = 0;

/**
 * @type {!Object.<string, number>}
 */
Resource._uniqueKindIds = {};

/**
 * @param {*} obj
 * @return {Resource}
 */
Resource.forObject = function(obj)
{
    if (!obj)
        return null;
    if (obj instanceof Resource)
        return obj;
    if (typeof obj === "object")
        return obj["__resourceObject"];
    return null;
}

/**
 * @param {Resource|*} obj
 * @return {*}
 */
Resource.wrappedObject = function(obj)
{
    var resource = Resource.forObject(obj);
    return resource ? resource.wrappedObject() : obj;
}

/**
 * @param {Resource|*} obj
 * @param {!Cache} cache
 * @return {ReplayableResource|*}
 */
Resource.toReplayable = function(obj, cache)
{
    var resource = Resource.forObject(obj);
    return resource ? resource.toReplayable(cache) : obj;
}

Resource.prototype = {
    /**
     * @return {number}
     */
    id: function()
    {
        return this._id;
    },

    /**
     * @return {Object}
     */
    wrappedObject: function()
    {
        return this._wrappedObject;
    },

    /**
     * @param {!Object} value
     */
    setWrappedObject: function(value)
    {
        console.assert(value, "wrappedObject should not be NULL");
        console.assert(!(value instanceof Resource), "Binding a Resource object to another Resource object?");
        this._wrappedObject = value;
        this._bindObjectToResource(value);
    },

    /**
     * @return {Object}
     */
    proxyObject: function()
    {
        if (!this._proxyObject)
            this._proxyObject = this._wrapObject();
        return this._proxyObject;
    },

    /**
     * @return {ResourceTrackingManager}
     */
    manager: function()
    {
        return this._resourceManager;
    },

    /**
     * @param {ResourceTrackingManager} value
     */
    setManager: function(value)
    {
        this._resourceManager = value;
    },

    /**
     * @return {!Array.<Call>}
     */
    calls: function()
    {
        return this._calls;
    },

    /**
     * @return {ContextResource}
     */
    contextResource: function()
    {
        if (this instanceof ContextResource)
            return /** @type {ContextResource} */ (this);

        if (this._calculatingContextResource)
            return null;

        this._calculatingContextResource = true;
        var result = null;
        for (var i = 0, n = this._calls.length; i < n; ++i) {
            result = this._calls[i].resource().contextResource();
            if (result)
                break;
        }
        delete this._calculatingContextResource;
        console.assert(result, "Failed to find context resource for " + this._name + "@" + this._kindId);
        return result;
    },

    /**
     * @return {string}
     */
    toDataURL: function()
    {
        return "";
    },

    /**
     * @param {!Cache} cache
     * @return {!ReplayableResource}
     */
    toReplayable: function(cache)
    {
        var result = /** @type {ReplayableResource} */ (cache.get(this._id));
        if (result)
            return result;
        var data = {
            id: this._id,
            name: this._name,
            kindId: this._kindId
        };
        result = new ReplayableResource(this, data);
        cache.put(this._id, result); // Put into the cache early to avoid loops.
        data.calls = this._calls.map(function(call) {
            return call.toReplayable(cache);
        });
        this._populateReplayableData(data, cache);
        var contextResource = this.contextResource();
        if (contextResource !== this)
            data.contextResource = Resource.toReplayable(contextResource, cache);
        return result;
    },

    /**
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _populateReplayableData: function(data, cache)
    {
        // Do nothing. Should be overridden by subclasses.
    },

    /**
     * @param {!Object} data
     * @param {!Cache} cache
     * @return {!Resource}
     */
    replay: function(data, cache)
    {
        var resource = /** @type {Resource} */ (cache.get(data.id));
        if (resource)
            return resource;
        this._id = data.id;
        this._name = data.name;
        this._kindId = data.kindId;
        this._resourceManager = null;
        this._calls = [];
        this._boundResources = Object.create(null);
        this._wrappedObject = null;
        cache.put(data.id, this); // Put into the cache early to avoid loops.
        this._doReplayCalls(data, cache);
        console.assert(this._wrappedObject, "Resource should be reconstructed!");
        return this;
    },

    /**
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        for (var i = 0, n = data.calls.length; i < n; ++i)
            this._calls.push(data.calls[i].replay(cache));
    },

    /**
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        call.freeze();
        this._calls.push(call);
    },

    /**
     * @param {!Object} object
     */
    _bindObjectToResource: function(object)
    {
        Object.defineProperty(object, "__resourceObject", {
            value: this,
            writable: false,
            enumerable: false,
            configurable: true
        });
    },

    /**
     * @param {string} key
     * @param {*} obj
     */
    _registerBoundResource: function(key, obj)
    {
        var resource = Resource.forObject(obj);
        if (resource)
            this._boundResources[key] = resource;
        else
            delete this._boundResources[key];
    },

    /**
     * @return {Object}
     */
    _wrapObject: function()
    {
        var wrappedObject = this.wrappedObject();
        if (!wrappedObject)
            return null;
        var proxy = Object.create(wrappedObject.__proto__); // In order to emulate "instanceof".

        var self = this;
        var customWrapFunctions = this._customWrapFunctions();
        function processProperty(property)
        {
            if (typeof wrappedObject[property] === "function") {
                var customWrapFunction = customWrapFunctions[property];
                if (customWrapFunction)
                    proxy[property] = self._wrapCustomFunction(self, wrappedObject, wrappedObject[property], property, customWrapFunction);
                else
                    proxy[property] = self._wrapFunction(self, wrappedObject, wrappedObject[property], property);
            } else if (/^[A-Z0-9_]+$/.test(property) && typeof wrappedObject[property] === "number") {
                // Fast access to enums and constants.
                proxy[property] = wrappedObject[property];
            } else {
                Object.defineProperty(proxy, property, {
                    get: function()
                    {
                        var obj = wrappedObject[property];
                        var resource = Resource.forObject(obj);
                        return resource ? resource : obj;
                    },
                    set: self._wrapPropertySetter(self, wrappedObject, property),
                    enumerable: true
                });
            }
        }

        var isEmpty = true;
        for (var property in wrappedObject) {
            isEmpty = false;
            processProperty(property);
        }
        if (isEmpty)
            return wrappedObject; // Nothing to proxy.

        this._bindObjectToResource(proxy);
        return proxy;
    },

    /**
     * @param {!Resource} resource
     * @param {!Object} originalObject
     * @param {!Function} originalFunction
     * @param {string} functionName
     * @param {!Function} customWrapFunction
     * @return {!Function}
     */
    _wrapCustomFunction: function(resource, originalObject, originalFunction, functionName, customWrapFunction)
    {
        return function()
        {
            var manager = resource.manager();
            var isCapturing = manager && manager.capturing();
            if (isCapturing)
                manager.captureArguments(resource, arguments);
            var wrapFunction = new Resource.WrapFunction(originalObject, originalFunction, functionName, arguments);
            customWrapFunction.apply(wrapFunction, arguments);
            if (isCapturing) {
                var call = wrapFunction.call();
                call.setStackTrace(StackTrace.create(1, arguments.callee));
                manager.captureCall(call);
            }
            return wrapFunction.result();
        };
    },

    /**
     * @param {!Resource} resource
     * @param {!Object} originalObject
     * @param {!Function} originalFunction
     * @param {string} functionName
     * @return {!Function}
     */
    _wrapFunction: function(resource, originalObject, originalFunction, functionName)
    {
        return function()
        {
            var manager = resource.manager();
            if (!manager || !manager.capturing())
                return originalFunction.apply(originalObject, arguments);
            manager.captureArguments(resource, arguments);
            var result = originalFunction.apply(originalObject, arguments);
            var stackTrace = StackTrace.create(1, arguments.callee);
            var call = new Call(resource, functionName, arguments, result, stackTrace);
            manager.captureCall(call);
            return result;
        };
    },

    /**
     * @param {!Resource} resource
     * @param {!Object} originalObject
     * @param {string} propertyName
     * @return {function(*)}
     */
    _wrapPropertySetter: function(resource, originalObject, propertyName)
    {
        return function(value)
        {
            resource._registerBoundResource(propertyName, value);
            var manager = resource.manager();
            if (!manager || !manager.capturing()) {
                originalObject[propertyName] = Resource.wrappedObject(value);
                return;
            }
            var args = [propertyName, value];
            manager.captureArguments(resource, args);
            originalObject[propertyName] = Resource.wrappedObject(value);
            var stackTrace = StackTrace.create(1, arguments.callee);
            var call = new Call(resource, "", args, undefined, stackTrace);
            manager.captureCall(call);
        };
    },

    /**
     * @return {!Object.<string, Function>}
     */
    _customWrapFunctions: function()
    {
        return Object.create(null); // May be overridden by subclasses.
    }
}

/**
 * @constructor
 * @param {Object} originalObject
 * @param {Function} originalFunction
 * @param {string} functionName
 * @param {Array|Arguments} args
 */
Resource.WrapFunction = function(originalObject, originalFunction, functionName, args)
{
    this._originalObject = originalObject;
    this._originalFunction = originalFunction;
    this._functionName = functionName;
    this._args = args;
    this._resource = Resource.forObject(originalObject);
    console.assert(this._resource, "Expected a wrapped call on a Resource object.");
}

Resource.WrapFunction.prototype = {
    /**
     * @return {*}
     */
    result: function()
    {
        if (!this._executed) {
            this._executed = true;
            this._result = this._originalFunction.apply(this._originalObject, this._args);
        }
        return this._result;
    },

    /**
     * @return {!Call}
     */
    call: function()
    {
        if (!this._call)
            this._call = new Call(this._resource, this._functionName, this._args, this.result());
        return this._call;
    },

    /**
     * @param {*} result
     */
    overrideResult: function(result)
    {
        var call = this.call();
        call.setResult(result);
        this._result = result;
    }
}

/**
 * @param {function(new:Resource, !Object, string)} resourceConstructor
 * @param {string} resourceName
 * @return {function(this:Resource.WrapFunction)}
 */
Resource.WrapFunction.resourceFactoryMethod = function(resourceConstructor, resourceName)
{
    /** @this Resource.WrapFunction */
    return function()
    {
        var wrappedObject = /** @type {Object} */ (this.result());
        if (!wrappedObject)
            return;
        var resource = new resourceConstructor(wrappedObject, resourceName);
        var manager = this._resource.manager();
        if (manager)
            manager.registerResource(resource);
        this.overrideResult(resource.proxyObject());
        resource.pushCall(this.call());
    }
}

/**
 * @constructor
 * @param {!Resource} originalResource
 * @param {!Object} data
 */
function ReplayableResource(originalResource, data)
{
    this._proto = originalResource.__proto__;
    this._data = data;
}

ReplayableResource.prototype = {
    /**
     * @return {number}
     */
    id: function()
    {
        return this._data.id;
    },

    /**
     * @return {string}
     */
    name: function()
    {
        return this._data.name;
    },

    /**
     * @return {string}
     */
    description: function()
    {
        return this._data.name + "@" + this._data.kindId;
    },

    /**
     * @return {!ReplayableResource}
     */
    replayableContextResource: function()
    {
        return this._data.contextResource || this;
    },

    /**
     * @param {!Cache} cache
     * @return {!Resource}
     */
    replay: function(cache)
    {
        var result = /** @type {!Resource} */ (Object.create(this._proto));
        result = result.replay(this._data, cache)
        console.assert(result.__proto__ === this._proto, "Wrong type of a replay result");
        return result;
    }
}

/**
 * @param {ReplayableResource|*} obj
 * @param {!Cache} cache
 * @return {*}
 */
ReplayableResource.replay = function(obj, cache)
{
    return (obj instanceof ReplayableResource) ? obj.replay(cache).wrappedObject() : obj;
}

/**
 * @constructor
 * @extends {Resource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function ContextResource(wrappedObject, name)
{
    Resource.call(this, wrappedObject, name);
}

ContextResource.prototype = {
    __proto__: Resource.prototype
}

/**
 * @constructor
 * @extends {Resource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function LogEverythingResource(wrappedObject, name)
{
    Resource.call(this, wrappedObject, name);
}

LogEverythingResource.prototype = {
    /**
     * @override
     * @return {!Object.<string, Function>}
     */
    _customWrapFunctions: function()
    {
        var wrapFunctions = Object.create(null);
        var wrappedObject = this.wrappedObject();
        if (wrappedObject) {
            for (var property in wrappedObject) {
                /** @this Resource.WrapFunction */
                wrapFunctions[property] = function()
                {
                    this._resource.pushCall(this.call());
                }
            }
        }
        return wrapFunctions;
    },

    __proto__: Resource.prototype
}

////////////////////////////////////////////////////////////////////////////////
// WebGL
////////////////////////////////////////////////////////////////////////////////

/**
 * @constructor
 * @extends {Resource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLBoundResource(wrappedObject, name)
{
    Resource.call(this, wrappedObject, name);
    /** @type {!Object.<string, *>} */
    this._state = {};
}

WebGLBoundResource.prototype = {
    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _populateReplayableData: function(data, cache)
    {
        var state = this._state;
        data.state = {};
        Object.keys(state).forEach(function(parameter) {
            data.state[parameter] = Resource.toReplayable(state[parameter], cache);
        });
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        var gl = this._replayContextResource(data, cache).wrappedObject();

        /** @type {!Object.<string, Array.<string>>} */
        var bindingsData = {
            TEXTURE_2D: ["bindTexture", "TEXTURE_BINDING_2D"],
            TEXTURE_CUBE_MAP: ["bindTexture", "TEXTURE_BINDING_CUBE_MAP"],
            ARRAY_BUFFER: ["bindBuffer", "ARRAY_BUFFER_BINDING"],
            ELEMENT_ARRAY_BUFFER: ["bindBuffer", "ELEMENT_ARRAY_BUFFER_BINDING"],
            FRAMEBUFFER: ["bindFramebuffer", "FRAMEBUFFER_BINDING"],
            RENDERBUFFER: ["bindRenderbuffer", "RENDERBUFFER_BINDING"]
        };
        var originalBindings = {};
        Object.keys(bindingsData).forEach(function(bindingTarget) {
            var bindingParameter = bindingsData[bindingTarget][1];
            originalBindings[bindingTarget] = gl.getParameter(gl[bindingParameter]);
        });

        var state = {};
        Object.keys(data.state).forEach(function(parameter) {
            state[parameter] = ReplayableResource.replay(data.state[parameter], cache);
        });
        this._state = state;
        Resource.prototype._doReplayCalls.call(this, data, cache);

        Object.keys(bindingsData).forEach(function(bindingTarget) {
            var bindMethodName = bindingsData[bindingTarget][0];
            gl[bindMethodName].call(gl, gl[bindingTarget], originalBindings[bindingTarget]);
        });
    },

    /**
     * @param {!Object} data
     * @param {!Cache} cache
     * @return {WebGLRenderingContextResource}
     */
    _replayContextResource: function(data, cache)
    {
        var calls = /** @type {!Array.<ReplayableCall>} */ (data.calls);
        for (var i = 0, n = calls.length; i < n; ++i) {
            var resource = ReplayableResource.replay(calls[i].replayableResource(), cache);
            var contextResource = WebGLRenderingContextResource.forObject(resource);
            if (contextResource)
                return contextResource;
        }
        return null;
    },

    /**
     * @param {number} target
     * @param {string} bindMethodName
     */
    pushBinding: function(target, bindMethodName)
    {
        if (this._state.BINDING !== target) {
            this._state.BINDING = target;
            this.pushCall(new Call(WebGLRenderingContextResource.forObject(this), bindMethodName, [target, this]));
        }
    },

    __proto__: Resource.prototype
}

/**
 * @constructor
 * @extends {WebGLBoundResource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLTextureResource(wrappedObject, name)
{
    WebGLBoundResource.call(this, wrappedObject, name);
}

WebGLTextureResource.prototype = {
    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        var gl = this._replayContextResource(data, cache).wrappedObject();

        var state = {};
        WebGLRenderingContextResource.PixelStoreParameters.forEach(function(parameter) {
            state[parameter] = gl.getParameter(gl[parameter]);
        });

        WebGLBoundResource.prototype._doReplayCalls.call(this, data, cache);

        WebGLRenderingContextResource.PixelStoreParameters.forEach(function(parameter) {
            gl.pixelStorei(gl[parameter], state[parameter]);
        });
    },

    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        var gl = WebGLRenderingContextResource.forObject(call.resource()).wrappedObject();
        WebGLRenderingContextResource.PixelStoreParameters.forEach(function(parameter) {
            var value = gl.getParameter(gl[parameter]);
            if (this._state[parameter] !== value) {
                this._state[parameter] = value;
                var pixelStoreCall = new Call(gl, "pixelStorei", [gl[parameter], value]);
                WebGLBoundResource.prototype.pushCall.call(this, pixelStoreCall);
            }
        }, this);

        // FIXME: remove any older calls that no longer contribute to the resource state.
        // FIXME: optimize memory usage: maybe it's more efficient to store one texImage2D call instead of many texSubImage2D.
        WebGLBoundResource.prototype.pushCall.call(this, call);
    },

    /**
     * Handles: texParameteri, texParameterf
     * @param {!Call} call
     */
    pushCall_texParameter: function(call)
    {
        var args = call.args();
        var pname = args[1];
        var param = args[2];
        if (this._state[pname] !== param) {
            this._state[pname] = param;
            WebGLBoundResource.prototype.pushCall.call(this, call);
        }
    },

    /**
     * Handles: copyTexImage2D, copyTexSubImage2D
     * copyTexImage2D and copyTexSubImage2D define a texture image with pixels from the current framebuffer.
     * @param {!Call} call
     */
    pushCall_copyTexImage2D: function(call)
    {
        var glResource = WebGLRenderingContextResource.forObject(call.resource());
        var gl = glResource.wrappedObject();
        var framebufferResource = /** @type {WebGLFramebufferResource} */ (glResource.currentBinding(gl.FRAMEBUFFER));
        if (framebufferResource)
            this.pushCall(new Call(glResource, "bindFramebuffer", [gl.FRAMEBUFFER, framebufferResource]));
        else {
            // FIXME: Implement this case.
            console.error("ASSERT_NOT_REACHED: Could not properly process a gl." + call.functionName() + " call while the DRAWING BUFFER is bound.");
        }
        this.pushCall(call);
    },

    __proto__: WebGLBoundResource.prototype
}

/**
 * @constructor
 * @extends {Resource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLProgramResource(wrappedObject, name)
{
    Resource.call(this, wrappedObject, name);
}

WebGLProgramResource.prototype = {
    /**
     * @override (overrides @return type)
     * @return {WebGLProgram}
     */
    wrappedObject: function()
    {
        return this._wrappedObject;
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _populateReplayableData: function(data, cache)
    {
        var glResource = WebGLRenderingContextResource.forObject(this);
        var gl = glResource.wrappedObject();
        var program = this.wrappedObject();

        var originalErrors = glResource.getAllErrors();

        var uniforms = [];
        var uniformsCount = /** @type {number} */ (gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS));
        for (var i = 0; i < uniformsCount; ++i) {
            var activeInfo = gl.getActiveUniform(program, i);
            if (!activeInfo)
                continue;
            var uniformLocation = gl.getUniformLocation(program, activeInfo.name);
            if (!uniformLocation)
                continue;
            var value = gl.getUniform(program, uniformLocation);
            uniforms.push({
                name: activeInfo.name,
                type: activeInfo.type,
                value: value
            });
        }
        data.uniforms = uniforms;

        glResource.restoreErrors(originalErrors);
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        Resource.prototype._doReplayCalls.call(this, data, cache);
        var gl = WebGLRenderingContextResource.forObject(this).wrappedObject();
        var program = this.wrappedObject();

        var originalProgram = /** @type {WebGLProgram} */ (gl.getParameter(gl.CURRENT_PROGRAM));
        var currentProgram = originalProgram;
        
        data.uniforms.forEach(function(uniform) {
            var uniformLocation = gl.getUniformLocation(program, uniform.name);
            if (!uniformLocation)
                return;
            if (currentProgram !== program) {
                currentProgram = program;
                gl.useProgram(program);
            }
            var methodName = this._uniformMethodNameByType(gl, uniform.type);
            if (methodName.indexOf("Matrix") === -1)
                gl[methodName].call(gl, uniformLocation, uniform.value);
            else
                gl[methodName].call(gl, uniformLocation, false, uniform.value);
        }.bind(this));

        if (currentProgram !== originalProgram)
            gl.useProgram(originalProgram);
    },

    /**
     * @param {WebGLRenderingContext} gl
     * @param {number} type
     * @return {string}
     */
    _uniformMethodNameByType: function(gl, type)
    {
        var uniformMethodNames = WebGLProgramResource._uniformMethodNames;
        if (!uniformMethodNames) {
            uniformMethodNames = {};
            uniformMethodNames[gl.FLOAT] = "uniform1f";
            uniformMethodNames[gl.FLOAT_VEC2] = "uniform2fv";
            uniformMethodNames[gl.FLOAT_VEC3] = "uniform3fv";
            uniformMethodNames[gl.FLOAT_VEC4] = "uniform4fv";
            uniformMethodNames[gl.INT] = "uniform1i";
            uniformMethodNames[gl.BOOL] = "uniform1i";
            uniformMethodNames[gl.SAMPLER_2D] = "uniform1i";
            uniformMethodNames[gl.SAMPLER_CUBE] = "uniform1i";
            uniformMethodNames[gl.INT_VEC2] = "uniform2iv";
            uniformMethodNames[gl.BOOL_VEC2] = "uniform2iv";
            uniformMethodNames[gl.INT_VEC3] = "uniform3iv";
            uniformMethodNames[gl.BOOL_VEC3] = "uniform3iv";
            uniformMethodNames[gl.INT_VEC4] = "uniform4iv";
            uniformMethodNames[gl.BOOL_VEC4] = "uniform4iv";
            uniformMethodNames[gl.FLOAT_MAT2] = "uniformMatrix2fv";
            uniformMethodNames[gl.FLOAT_MAT3] = "uniformMatrix3fv";
            uniformMethodNames[gl.FLOAT_MAT4] = "uniformMatrix4fv";
            WebGLProgramResource._uniformMethodNames = uniformMethodNames;
        }
        console.assert(uniformMethodNames[type], "Unknown uniform type " + type);
        return uniformMethodNames[type];
    },

    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        // FIXME: remove any older calls that no longer contribute to the resource state.
        // FIXME: handle multiple attachShader && detachShader.
        Resource.prototype.pushCall.call(this, call);
    },

    __proto__: Resource.prototype
}

/**
 * @constructor
 * @extends {Resource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLShaderResource(wrappedObject, name)
{
    Resource.call(this, wrappedObject, name);
}

WebGLShaderResource.prototype = {
    /**
     * @return {number}
     */
    type: function()
    {
        var call = this._calls[0];
        if (call && call.functionName() === "createShader")
            return call.args()[0];
        console.error("ASSERT_NOT_REACHED: Failed to restore shader type from the log.", call);
        return 0;
    },

    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        // FIXME: remove any older calls that no longer contribute to the resource state.
        // FIXME: handle multiple shaderSource calls.
        Resource.prototype.pushCall.call(this, call);
    },

    __proto__: Resource.prototype
}

/**
 * @constructor
 * @extends {WebGLBoundResource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLBufferResource(wrappedObject, name)
{
    WebGLBoundResource.call(this, wrappedObject, name);
}

WebGLBufferResource.prototype = {
    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        // FIXME: remove any older calls that no longer contribute to the resource state.
        // FIXME: Optimize memory for bufferSubData.
        WebGLBoundResource.prototype.pushCall.call(this, call);
    },

    __proto__: WebGLBoundResource.prototype
}

/**
 * @constructor
 * @extends {WebGLBoundResource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLFramebufferResource(wrappedObject, name)
{
    WebGLBoundResource.call(this, wrappedObject, name);
}

WebGLFramebufferResource.prototype = {
    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        // FIXME: remove any older calls that no longer contribute to the resource state.
        WebGLBoundResource.prototype.pushCall.call(this, call);
    },

    __proto__: WebGLBoundResource.prototype
}

/**
 * @constructor
 * @extends {WebGLBoundResource}
 * @param {!Object} wrappedObject
 * @param {string} name
 */
function WebGLRenderbufferResource(wrappedObject, name)
{
    WebGLBoundResource.call(this, wrappedObject, name);
}

WebGLRenderbufferResource.prototype = {
    /**
     * @override
     * @param {!Call} call
     */
    pushCall: function(call)
    {
        // FIXME: remove any older calls that no longer contribute to the resource state.
        WebGLBoundResource.prototype.pushCall.call(this, call);
    },

    __proto__: WebGLBoundResource.prototype
}

/**
 * @constructor
 * @extends {ContextResource}
 * @param {!WebGLRenderingContext} glContext
 */
function WebGLRenderingContextResource(glContext)
{
    ContextResource.call(this, glContext, "WebGLRenderingContext");
    /** @type {Object.<number, boolean>} */
    this._customErrors = null;
    /** @type {!Object.<string, boolean>} */
    this._extensions = {};
}

/**
 * @const
 * @type {!Array.<string>}
 */
WebGLRenderingContextResource.GLCapabilities = [
    "BLEND",
    "CULL_FACE",
    "DEPTH_TEST",
    "DITHER",
    "POLYGON_OFFSET_FILL",
    "SAMPLE_ALPHA_TO_COVERAGE",
    "SAMPLE_COVERAGE",
    "SCISSOR_TEST",
    "STENCIL_TEST"
];

/**
 * @const
 * @type {!Array.<string>}
 */
WebGLRenderingContextResource.PixelStoreParameters = [
    "PACK_ALIGNMENT",
    "UNPACK_ALIGNMENT",
    "UNPACK_COLORSPACE_CONVERSION_WEBGL",
    "UNPACK_FLIP_Y_WEBGL",
    "UNPACK_PREMULTIPLY_ALPHA_WEBGL"
];

/**
 * @const
 * @type {!Array.<string>}
 */
WebGLRenderingContextResource.StateParameters = [
    "ACTIVE_TEXTURE",
    "ARRAY_BUFFER_BINDING",
    "BLEND_COLOR",
    "BLEND_DST_ALPHA",
    "BLEND_DST_RGB",
    "BLEND_EQUATION_ALPHA",
    "BLEND_EQUATION_RGB",
    "BLEND_SRC_ALPHA",
    "BLEND_SRC_RGB",
    "COLOR_CLEAR_VALUE",
    "COLOR_WRITEMASK",
    "CULL_FACE_MODE",
    "CURRENT_PROGRAM",
    "DEPTH_CLEAR_VALUE",
    "DEPTH_FUNC",
    "DEPTH_RANGE",
    "DEPTH_WRITEMASK",
    "ELEMENT_ARRAY_BUFFER_BINDING",
    "FRAMEBUFFER_BINDING",
    "FRONT_FACE",
    "GENERATE_MIPMAP_HINT",
    "LINE_WIDTH",
    "PACK_ALIGNMENT",
    "POLYGON_OFFSET_FACTOR",
    "POLYGON_OFFSET_UNITS",
    "RENDERBUFFER_BINDING",
    "SAMPLE_COVERAGE_INVERT",
    "SAMPLE_COVERAGE_VALUE",
    "SCISSOR_BOX",
    "STENCIL_BACK_FAIL",
    "STENCIL_BACK_FUNC",
    "STENCIL_BACK_PASS_DEPTH_FAIL",
    "STENCIL_BACK_PASS_DEPTH_PASS",
    "STENCIL_BACK_REF",
    "STENCIL_BACK_VALUE_MASK",
    "STENCIL_BACK_WRITEMASK",
    "STENCIL_CLEAR_VALUE",
    "STENCIL_FAIL",
    "STENCIL_FUNC",
    "STENCIL_PASS_DEPTH_FAIL",
    "STENCIL_PASS_DEPTH_PASS",
    "STENCIL_REF",
    "STENCIL_VALUE_MASK",
    "STENCIL_WRITEMASK",
    "UNPACK_ALIGNMENT",
    "UNPACK_COLORSPACE_CONVERSION_WEBGL",
    "UNPACK_FLIP_Y_WEBGL",
    "UNPACK_PREMULTIPLY_ALPHA_WEBGL",
    "VIEWPORT"
];

/**
 * @const
 * @type {!Object.<string, boolean>}
 */
WebGLRenderingContextResource.DrawingMethods = TypeUtils.createPrefixedPropertyNamesSet([
    "clear",
    "drawArrays",
    "drawElements"
]);

/**
 * @param {*} obj
 * @return {WebGLRenderingContextResource}
 */
WebGLRenderingContextResource.forObject = function(obj)
{
    var resource = Resource.forObject(obj);
    if (!resource)
        return null;
    resource = resource.contextResource();
    return (resource instanceof WebGLRenderingContextResource) ? resource : null;
}

WebGLRenderingContextResource.prototype = {
    /**
     * @override (overrides @return type)
     * @return {WebGLRenderingContext}
     */
    wrappedObject: function()
    {
        return this._wrappedObject;
    },

    /**
     * @override
     * @return {string}
     */
    toDataURL: function()
    {
        return this.wrappedObject().canvas.toDataURL();
    },

    /**
     * @return {Array.<number>}
     */
    getAllErrors: function()
    {
        var errors = [];
        var gl = this.wrappedObject();
        if (gl) {
            while (true) {
                var error = gl.getError();
                if (error === gl.NO_ERROR)
                    break;
                this.clearError(error);
                errors.push(error);
            }
        }
        if (this._customErrors) {
            for (var key in this._customErrors) {
                var error = Number(key);
                errors.push(error);
            }
            delete this._customErrors;
        }
        return errors;
    },

    /**
     * @param {Array.<number>} errors
     */
    restoreErrors: function(errors)
    {
        var gl = this.wrappedObject();
        if (gl) {
            var wasError = false;
            while (gl.getError() !== gl.NO_ERROR)
                wasError = true;
            console.assert(!wasError, "Error(s) while capturing current WebGL state.");
        }
        if (!errors.length)
            delete this._customErrors;
        else {
            this._customErrors = {};
            for (var i = 0, n = errors.length; i < n; ++i)
                this._customErrors[errors[i]] = true;
        }
    },

    /**
     * @param {number} error
     */
    clearError: function(error)
    {
        if (this._customErrors)
            delete this._customErrors[error];
    },

    /**
     * @return {number}
     */
    nextError: function()
    {
        if (this._customErrors) {
            for (var key in this._customErrors) {
                var error = Number(key);
                delete this._customErrors[error];
                return error;
            }
        }
        delete this._customErrors;
        var gl = this.wrappedObject();
        return gl ? gl.NO_ERROR : 0;
    },

    /**
     * @param {string} name
     */
    addExtension: function(name)
    {
        // FIXME: Wrap OES_vertex_array_object extension.
        this._extensions[name.toLowerCase()] = true;
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _populateReplayableData: function(data, cache)
    {
        var gl = this.wrappedObject();
        data.originalCanvas = gl.canvas;
        data.originalContextAttributes = gl.getContextAttributes();
        data.extensions = TypeUtils.cloneObject(this._extensions);

        var originalErrors = this.getAllErrors();

        // Take a full GL state snapshot.
        var glState = {};
        WebGLRenderingContextResource.GLCapabilities.forEach(function(parameter) {
            glState[parameter] = gl.isEnabled(gl[parameter]);
        });
        WebGLRenderingContextResource.StateParameters.forEach(function(parameter) {
            glState[parameter] = Resource.toReplayable(gl.getParameter(gl[parameter]), cache);
        });

        // VERTEX_ATTRIB_ARRAYS
        var maxVertexAttribs = /** @type {number} */ (gl.getParameter(gl.MAX_VERTEX_ATTRIBS));
        var vertexAttribParameters = ["VERTEX_ATTRIB_ARRAY_BUFFER_BINDING", "VERTEX_ATTRIB_ARRAY_ENABLED", "VERTEX_ATTRIB_ARRAY_SIZE", "VERTEX_ATTRIB_ARRAY_STRIDE", "VERTEX_ATTRIB_ARRAY_TYPE", "VERTEX_ATTRIB_ARRAY_NORMALIZED", "CURRENT_VERTEX_ATTRIB"];
        var vertexAttribStates = [];
        for (var i = 0; i < maxVertexAttribs; ++i) {
            var state = {};
            vertexAttribParameters.forEach(function(attribParameter) {
                state[attribParameter] = Resource.toReplayable(gl.getVertexAttrib(i, gl[attribParameter]), cache);
            });
            state.VERTEX_ATTRIB_ARRAY_POINTER = gl.getVertexAttribOffset(i, gl.VERTEX_ATTRIB_ARRAY_POINTER);
            vertexAttribStates.push(state);
        }
        glState.vertexAttribStates = vertexAttribStates;

        // TEXTURES
        var currentTextureBinding = /** @type {number} */ (gl.getParameter(gl.ACTIVE_TEXTURE));
        var maxTextureImageUnits = /** @type {number} */ (gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS));
        var textureBindings = [];
        for (var i = 0; i < maxTextureImageUnits; ++i) {
            gl.activeTexture(gl.TEXTURE0 + i);
            var state = {
                TEXTURE_2D: Resource.toReplayable(gl.getParameter(gl.TEXTURE_BINDING_2D), cache),
                TEXTURE_CUBE_MAP: Resource.toReplayable(gl.getParameter(gl.TEXTURE_BINDING_CUBE_MAP), cache)
            };
            textureBindings.push(state);
        }
        glState.textureBindings = textureBindings;
        gl.activeTexture(currentTextureBinding);

        data.glState = glState;

        this.restoreErrors(originalErrors);
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        this._customErrors = null;
        this._extensions = TypeUtils.cloneObject(data.extensions) || {};

        var canvas = data.originalCanvas.cloneNode(true);
        var replayContext = null;
        var contextIds = ["experimental-webgl", "webkit-3d", "3d"];
        for (var i = 0, contextId; contextId = contextIds[i]; ++i) {
            replayContext = canvas.getContext(contextId, data.originalContextAttributes);
            if (replayContext)
                break;
        }

        console.assert(replayContext, "Failed to create a WebGLRenderingContext for the replay.");

        var gl = /** @type {!WebGLRenderingContext} */ (Resource.wrappedObject(replayContext));
        this.setWrappedObject(gl);

        // Enable corresponding WebGL extensions.
        for (var name in this._extensions)
            gl.getExtension(name);

        var glState = data.glState;
        gl.bindFramebuffer(gl.FRAMEBUFFER, /** @type {WebGLFramebuffer} */ (ReplayableResource.replay(glState.FRAMEBUFFER_BINDING, cache)));
        gl.bindRenderbuffer(gl.RENDERBUFFER, /** @type {WebGLRenderbuffer} */ (ReplayableResource.replay(glState.RENDERBUFFER_BINDING, cache)));

        // Enable or disable server-side GL capabilities.
        WebGLRenderingContextResource.GLCapabilities.forEach(function(parameter) {
            console.assert(parameter in glState);
            if (glState[parameter])
                gl.enable(gl[parameter]);
            else
                gl.disable(gl[parameter]);
        });

        gl.blendColor(glState.BLEND_COLOR[0], glState.BLEND_COLOR[1], glState.BLEND_COLOR[2], glState.BLEND_COLOR[3]);
        gl.blendEquationSeparate(glState.BLEND_EQUATION_RGB, glState.BLEND_EQUATION_ALPHA);
        gl.blendFuncSeparate(glState.BLEND_SRC_RGB, glState.BLEND_DST_RGB, glState.BLEND_SRC_ALPHA, glState.BLEND_DST_ALPHA);
        gl.clearColor(glState.COLOR_CLEAR_VALUE[0], glState.COLOR_CLEAR_VALUE[1], glState.COLOR_CLEAR_VALUE[2], glState.COLOR_CLEAR_VALUE[3]);
        gl.clearDepth(glState.DEPTH_CLEAR_VALUE);
        gl.clearStencil(glState.STENCIL_CLEAR_VALUE);
        gl.colorMask(glState.COLOR_WRITEMASK[0], glState.COLOR_WRITEMASK[1], glState.COLOR_WRITEMASK[2], glState.COLOR_WRITEMASK[3]);
        gl.cullFace(glState.CULL_FACE_MODE);
        gl.depthFunc(glState.DEPTH_FUNC);
        gl.depthMask(glState.DEPTH_WRITEMASK);
        gl.depthRange(glState.DEPTH_RANGE[0], glState.DEPTH_RANGE[1]);
        gl.frontFace(glState.FRONT_FACE);
        gl.hint(gl.GENERATE_MIPMAP_HINT, glState.GENERATE_MIPMAP_HINT);
        gl.lineWidth(glState.LINE_WIDTH);

        WebGLRenderingContextResource.PixelStoreParameters.forEach(function(parameter) {
            gl.pixelStorei(gl[parameter], glState[parameter]);
        });

        gl.polygonOffset(glState.POLYGON_OFFSET_FACTOR, glState.POLYGON_OFFSET_UNITS);
        gl.sampleCoverage(glState.SAMPLE_COVERAGE_VALUE, glState.SAMPLE_COVERAGE_INVERT);
        gl.stencilFuncSeparate(gl.FRONT, glState.STENCIL_FUNC, glState.STENCIL_REF, glState.STENCIL_VALUE_MASK);
        gl.stencilFuncSeparate(gl.BACK, glState.STENCIL_BACK_FUNC, glState.STENCIL_BACK_REF, glState.STENCIL_BACK_VALUE_MASK);
        gl.stencilOpSeparate(gl.FRONT, glState.STENCIL_FAIL, glState.STENCIL_PASS_DEPTH_FAIL, glState.STENCIL_PASS_DEPTH_PASS);
        gl.stencilOpSeparate(gl.BACK, glState.STENCIL_BACK_FAIL, glState.STENCIL_BACK_PASS_DEPTH_FAIL, glState.STENCIL_BACK_PASS_DEPTH_PASS);
        gl.stencilMaskSeparate(gl.FRONT, glState.STENCIL_WRITEMASK);
        gl.stencilMaskSeparate(gl.BACK, glState.STENCIL_BACK_WRITEMASK);

        gl.scissor(glState.SCISSOR_BOX[0], glState.SCISSOR_BOX[1], glState.SCISSOR_BOX[2], glState.SCISSOR_BOX[3]);
        gl.viewport(glState.VIEWPORT[0], glState.VIEWPORT[1], glState.VIEWPORT[2], glState.VIEWPORT[3]);

        gl.useProgram(/** @type {WebGLProgram} */ (ReplayableResource.replay(glState.CURRENT_PROGRAM, cache)));

        // VERTEX_ATTRIB_ARRAYS
        var maxVertexAttribs = /** @type {number} */ (gl.getParameter(gl.MAX_VERTEX_ATTRIBS));
        for (var i = 0; i < maxVertexAttribs; ++i) {
            var state = glState.vertexAttribStates[i] || {};
            if (state.VERTEX_ATTRIB_ARRAY_ENABLED)
                gl.enableVertexAttribArray(i);
            else
                gl.disableVertexAttribArray(i);
            if (state.CURRENT_VERTEX_ATTRIB)
                gl.vertexAttrib4fv(i, state.CURRENT_VERTEX_ATTRIB);
            var buffer = /** @type {WebGLBuffer} */ (ReplayableResource.replay(state.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, cache));
            if (buffer) {
                gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
                gl.vertexAttribPointer(i, state.VERTEX_ATTRIB_ARRAY_SIZE, state.VERTEX_ATTRIB_ARRAY_TYPE, state.VERTEX_ATTRIB_ARRAY_NORMALIZED, state.VERTEX_ATTRIB_ARRAY_STRIDE, state.VERTEX_ATTRIB_ARRAY_POINTER);
            }
        }
        gl.bindBuffer(gl.ARRAY_BUFFER, /** @type {WebGLBuffer} */ (ReplayableResource.replay(glState.ARRAY_BUFFER_BINDING, cache)));
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, /** @type {WebGLBuffer} */ (ReplayableResource.replay(glState.ELEMENT_ARRAY_BUFFER_BINDING, cache)));

        // TEXTURES
        var maxTextureImageUnits = /** @type {number} */ (gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS));
        for (var i = 0; i < maxTextureImageUnits; ++i) {
            gl.activeTexture(gl.TEXTURE0 + i);
            var state = glState.textureBindings[i] || {};
            gl.bindTexture(gl.TEXTURE_2D, /** @type {WebGLTexture} */ (ReplayableResource.replay(state.TEXTURE_2D, cache)));
            gl.bindTexture(gl.TEXTURE_CUBE_MAP, /** @type {WebGLTexture} */ (ReplayableResource.replay(state.TEXTURE_CUBE_MAP, cache)));
        }
        gl.activeTexture(glState.ACTIVE_TEXTURE);

        ContextResource.prototype._doReplayCalls.call(this, data, cache);
    },

    /**
     * @param {Object|number} target
     * @return {Resource}
     */
    currentBinding: function(target)
    {
        var resource = Resource.forObject(target);
        if (resource)
            return resource;
        var gl = this.wrappedObject();
        var bindingParameter;
        var bindMethodName;
        var bindMethodTarget = target;
        switch (target) {
        case gl.ARRAY_BUFFER:
            bindingParameter = gl.ARRAY_BUFFER_BINDING;
            bindMethodName = "bindBuffer";
            break;
        case gl.ELEMENT_ARRAY_BUFFER:
            bindingParameter = gl.ELEMENT_ARRAY_BUFFER_BINDING;
            bindMethodName = "bindBuffer";
            break;
        case gl.TEXTURE_2D:
            bindingParameter = gl.TEXTURE_BINDING_2D;
            bindMethodName = "bindTexture";
            break;
        case gl.TEXTURE_CUBE_MAP:
        case gl.TEXTURE_CUBE_MAP_POSITIVE_X:
        case gl.TEXTURE_CUBE_MAP_NEGATIVE_X:
        case gl.TEXTURE_CUBE_MAP_POSITIVE_Y:
        case gl.TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case gl.TEXTURE_CUBE_MAP_POSITIVE_Z:
        case gl.TEXTURE_CUBE_MAP_NEGATIVE_Z:
            bindingParameter = gl.TEXTURE_BINDING_CUBE_MAP;
            bindMethodTarget = gl.TEXTURE_CUBE_MAP;
            bindMethodName = "bindTexture";
            break;
        case gl.FRAMEBUFFER:
            bindingParameter = gl.FRAMEBUFFER_BINDING;
            bindMethodName = "bindFramebuffer";
            break;
        case gl.RENDERBUFFER:
            bindingParameter = gl.RENDERBUFFER_BINDING;
            bindMethodName = "bindRenderbuffer";
            break;
        default:
            console.error("ASSERT_NOT_REACHED: unknown binding target " + target);
            return null;
        }
        resource = Resource.forObject(gl.getParameter(bindingParameter));
        if (resource)
            resource.pushBinding(bindMethodTarget, bindMethodName);
        return resource;
    },

    /**
     * @override
     * @return {!Object.<string, Function>}
     */
    _customWrapFunctions: function()
    {
        var wrapFunctions = WebGLRenderingContextResource._wrapFunctions;
        if (!wrapFunctions) {
            wrapFunctions = Object.create(null);

            wrapFunctions["createBuffer"] = Resource.WrapFunction.resourceFactoryMethod(WebGLBufferResource, "WebGLBuffer");
            wrapFunctions["createShader"] = Resource.WrapFunction.resourceFactoryMethod(WebGLShaderResource, "WebGLShader");
            wrapFunctions["createProgram"] = Resource.WrapFunction.resourceFactoryMethod(WebGLProgramResource, "WebGLProgram");
            wrapFunctions["createTexture"] = Resource.WrapFunction.resourceFactoryMethod(WebGLTextureResource, "WebGLTexture");
            wrapFunctions["createFramebuffer"] = Resource.WrapFunction.resourceFactoryMethod(WebGLFramebufferResource, "WebGLFramebuffer");
            wrapFunctions["createRenderbuffer"] = Resource.WrapFunction.resourceFactoryMethod(WebGLRenderbufferResource, "WebGLRenderbuffer");
            wrapFunctions["getUniformLocation"] = Resource.WrapFunction.resourceFactoryMethod(Resource, "WebGLUniformLocation");

            /**
             * @param {string} methodName
             * @param {function(this:Resource, !Call)=} pushCallFunc
             */
            function stateModifyingWrapFunction(methodName, pushCallFunc)
            {
                if (pushCallFunc) {
                    /**
                     * @param {Object|number} target
                     * @this Resource.WrapFunction
                     */
                    wrapFunctions[methodName] = function(target)
                    {
                        var resource = this._resource.currentBinding(target);
                        if (resource)
                            pushCallFunc.call(resource, this.call());
                    }
                } else {
                    /**
                     * @param {Object|number} target
                     * @this Resource.WrapFunction
                     */
                    wrapFunctions[methodName] = function(target)
                    {
                        var resource = this._resource.currentBinding(target);
                        if (resource)
                            resource.pushCall(this.call());
                    }
                }
            }
            stateModifyingWrapFunction("bindAttribLocation");
            stateModifyingWrapFunction("compileShader");
            stateModifyingWrapFunction("detachShader");
            stateModifyingWrapFunction("linkProgram");
            stateModifyingWrapFunction("shaderSource");
            stateModifyingWrapFunction("bufferData");
            stateModifyingWrapFunction("bufferSubData");
            stateModifyingWrapFunction("compressedTexImage2D");
            stateModifyingWrapFunction("compressedTexSubImage2D");
            stateModifyingWrapFunction("copyTexImage2D", WebGLTextureResource.prototype.pushCall_copyTexImage2D);
            stateModifyingWrapFunction("copyTexSubImage2D", WebGLTextureResource.prototype.pushCall_copyTexImage2D);
            stateModifyingWrapFunction("generateMipmap");
            stateModifyingWrapFunction("texImage2D");
            stateModifyingWrapFunction("texSubImage2D");
            stateModifyingWrapFunction("texParameterf", WebGLTextureResource.prototype.pushCall_texParameter);
            stateModifyingWrapFunction("texParameteri", WebGLTextureResource.prototype.pushCall_texParameter);
            stateModifyingWrapFunction("renderbufferStorage");

            /** @this Resource.WrapFunction */
            wrapFunctions["getError"] = function()
            {
                var gl = /** @type {WebGLRenderingContext} */ (this._originalObject);
                var error = this.result();
                if (error !== gl.NO_ERROR)
                    this._resource.clearError(error);
                else {
                    error = this._resource.nextError();
                    if (error !== gl.NO_ERROR)
                        this.overrideResult(error);
                }
            }

            /**
             * @param {string} name
             * @this Resource.WrapFunction
             */
            wrapFunctions["getExtension"] = function(name)
            {
                this._resource.addExtension(name);
            }

            //
            // Register bound WebGL resources.
            //

            /**
             * @param {WebGLProgram} program
             * @param {WebGLShader} shader
             * @this Resource.WrapFunction
             */
            wrapFunctions["attachShader"] = function(program, shader)
            {
                var resource = this._resource.currentBinding(program);
                if (resource) {
                    resource.pushCall(this.call());
                    var shaderResource = /** @type {WebGLShaderResource} */ (Resource.forObject(shader));
                    if (shaderResource) {
                        var shaderType = shaderResource.type();
                        resource._registerBoundResource("__attachShader_" + shaderType, shaderResource);
                    }
                }
            }
            /**
             * @param {number} target
             * @param {number} attachment
             * @param {number} objectTarget
             * @param {WebGLRenderbuffer|WebGLTexture} obj
             * @this Resource.WrapFunction
             */
            wrapFunctions["framebufferRenderbuffer"] = wrapFunctions["framebufferTexture2D"] = function(target, attachment, objectTarget, obj)
            {
                var resource = this._resource.currentBinding(target);
                if (resource) {
                    resource.pushCall(this.call());
                    resource._registerBoundResource("__framebufferAttachmentObjectName", obj);
                }
            }
            /**
             * @param {number} target
             * @param {Object} obj
             * @this Resource.WrapFunction
             */
            wrapFunctions["bindBuffer"] = wrapFunctions["bindFramebuffer"] = wrapFunctions["bindRenderbuffer"] = function(target, obj)
            {
                this._resource._registerBoundResource("__bindBuffer_" + target, obj);
            }
            /**
             * @param {number} target
             * @param {WebGLTexture} obj
             * @this Resource.WrapFunction
             */
            wrapFunctions["bindTexture"] = function(target, obj)
            {
                var gl = /** @type {WebGLRenderingContext} */ (this._originalObject);
                var currentTextureBinding = /** @type {number} */ (gl.getParameter(gl.ACTIVE_TEXTURE));
                this._resource._registerBoundResource("__bindTexture_" + target + "_" + currentTextureBinding, obj);
            }
            /**
             * @param {WebGLProgram} program
             * @this Resource.WrapFunction
             */
            wrapFunctions["useProgram"] = function(program)
            {
                this._resource._registerBoundResource("__useProgram", program);
            }
            /**
             * @param {number} index
             * @this Resource.WrapFunction
             */
            wrapFunctions["vertexAttribPointer"] = function(index)
            {
                var gl = /** @type {WebGLRenderingContext} */ (this._originalObject);
                this._resource._registerBoundResource("__vertexAttribPointer_" + index, gl.getParameter(gl.ARRAY_BUFFER_BINDING));
            }

            WebGLRenderingContextResource._wrapFunctions = wrapFunctions;
        }
        return wrapFunctions;
    },

    __proto__: ContextResource.prototype
}

////////////////////////////////////////////////////////////////////////////////
// 2D Canvas
////////////////////////////////////////////////////////////////////////////////

/**
 * @constructor
 * @extends {ContextResource}
 * @param {!CanvasRenderingContext2D} context
 */
function CanvasRenderingContext2DResource(context)
{
    ContextResource.call(this, context, "CanvasRenderingContext2D");
}

/**
 * @const
 * @type {!Array.<string>}
 */
CanvasRenderingContext2DResource.AttributeProperties = [
    "strokeStyle",
    "fillStyle",
    "globalAlpha",
    "lineWidth",
    "lineCap",
    "lineJoin",
    "miterLimit",
    "shadowOffsetX",
    "shadowOffsetY",
    "shadowBlur",
    "shadowColor",
    "globalCompositeOperation",
    "font",
    "textAlign",
    "textBaseline",
    "lineDashOffset",
    "webkitLineDash",
    "webkitLineDashOffset"
];

/**
 * @const
 * @type {!Array.<string>}
 */
CanvasRenderingContext2DResource.PathMethods = [
    "beginPath",
    "moveTo",
    "closePath",
    "lineTo",
    "quadraticCurveTo",
    "bezierCurveTo",
    "arcTo",
    "arc",
    "rect"
];

/**
 * @const
 * @type {!Array.<string>}
 */
CanvasRenderingContext2DResource.TransformationMatrixMethods = [
    "scale",
    "rotate",
    "translate",
    "transform",
    "setTransform"
];

/**
 * @const
 * @type {!Object.<string, boolean>}
 */
CanvasRenderingContext2DResource.DrawingMethods = TypeUtils.createPrefixedPropertyNamesSet([
    "clearRect",
    "drawImage",
    "drawImageFromRect",
    "drawCustomFocusRing",
    "drawSystemFocusRing",
    "fill",
    "fillRect",
    "fillText",
    "putImageData",
    "putImageDataHD",
    "stroke",
    "strokeRect",
    "strokeText"
]);

CanvasRenderingContext2DResource.prototype = {
    /**
     * @override (overrides @return type)
     * @return {CanvasRenderingContext2D}
     */
    wrappedObject: function()
    {
        return this._wrappedObject;
    },

    /**
     * @override
     * @return {string}
     */
    toDataURL: function()
    {
        return this.wrappedObject().canvas.toDataURL();
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _populateReplayableData: function(data, cache)
    {
        data.currentAttributes = this._currentAttributesState();
        data.originalCanvasCloned = TypeUtils.cloneIntoCanvas(this.wrappedObject().canvas);
    },

    /**
     * @override
     * @param {!Object} data
     * @param {!Cache} cache
     */
    _doReplayCalls: function(data, cache)
    {
        var canvas = TypeUtils.cloneIntoCanvas(data.originalCanvasCloned);
        var ctx = /** @type {!CanvasRenderingContext2D} */ (Resource.wrappedObject(canvas.getContext("2d")));
        this.setWrappedObject(ctx);

        for (var i = 0, n = data.calls.length; i < n; ++i) {
            var replayableCall = /** @type {ReplayableCall} */ (data.calls[i]);
            if (replayableCall.functionName() === "save")
                this._applyAttributesState(replayableCall.attachment("canvas2dAttributesState"));
            this._calls.push(replayableCall.replay(cache));
        }
        this._applyAttributesState(data.currentAttributes);
    },

    /**
     * @param {!Call} call
     */
    pushCall_setTransform: function(call)
    {
        var saveCallIndex = this._lastIndexOfMatchingSaveCall();
        var index = this._lastIndexOfAnyCall(CanvasRenderingContext2DResource.PathMethods);
        index = Math.max(index, saveCallIndex);
        if (this._removeCallsFromLog(CanvasRenderingContext2DResource.TransformationMatrixMethods, index + 1))
            this._removeAllObsoleteCallsFromLog();
        this.pushCall(call);
    },

    /**
     * @param {!Call} call
     */
    pushCall_beginPath: function(call)
    {
        var index = this._lastIndexOfAnyCall(["clip"]);
        if (this._removeCallsFromLog(CanvasRenderingContext2DResource.PathMethods, index + 1))
            this._removeAllObsoleteCallsFromLog();
        this.pushCall(call);
    },

    /**
     * @param {!Call} call
     */
    pushCall_save: function(call)
    {
        call.setAttachment("canvas2dAttributesState", this._currentAttributesState());
        this.pushCall(call);
    },

    /**
     * @param {!Call} call
     */
    pushCall_restore: function(call)
    {
        var lastIndexOfSave = this._lastIndexOfMatchingSaveCall();
        if (lastIndexOfSave === -1)
            return;
        this._calls[lastIndexOfSave].setAttachment("canvas2dAttributesState", null); // No longer needed, free memory.

        var modified = false;
        if (this._removeCallsFromLog(["clip"], lastIndexOfSave + 1))
            modified = true;

        var lastIndexOfAnyPathMethod = this._lastIndexOfAnyCall(CanvasRenderingContext2DResource.PathMethods);
        var index = Math.max(lastIndexOfSave, lastIndexOfAnyPathMethod);
        if (this._removeCallsFromLog(CanvasRenderingContext2DResource.TransformationMatrixMethods, index + 1))
            modified = true;

        if (modified)
            this._removeAllObsoleteCallsFromLog();

        var lastCall = this._calls[this._calls.length - 1];
        if (lastCall && lastCall.functionName() === "save")
            this._calls.pop();
        else
            this.pushCall(call);
    },

    /**
     * @param {number=} fromIndex
     * @return {number}
     */
    _lastIndexOfMatchingSaveCall: function(fromIndex)
    {
        if (typeof fromIndex !== "number")
            fromIndex = this._calls.length - 1;
        else
            fromIndex = Math.min(fromIndex, this._calls.length - 1);
        var stackDepth = 1;
        for (var i = fromIndex; i >= 0; --i) {
            var functionName = this._calls[i].functionName();
            if (functionName === "restore")
                ++stackDepth;
            else if (functionName === "save") {
                --stackDepth;
                if (!stackDepth)
                    return i;
            }
        }
        return -1;
    },

    /**
     * @param {!Array.<string>} functionNames
     * @param {number=} fromIndex
     * @return {number}
     */
    _lastIndexOfAnyCall: function(functionNames, fromIndex)
    {
        if (typeof fromIndex !== "number")
            fromIndex = this._calls.length - 1;
        else
            fromIndex = Math.min(fromIndex, this._calls.length - 1);
        for (var i = fromIndex; i >= 0; --i) {
            if (functionNames.indexOf(this._calls[i].functionName()) !== -1)
                return i;
        }
        return -1;
    },

    _removeAllObsoleteCallsFromLog: function()
    {
        // Remove all PATH methods between clip() and beginPath() calls.
        var lastIndexOfBeginPath = this._lastIndexOfAnyCall(["beginPath"]);
        while (lastIndexOfBeginPath !== -1) {
            var index = this._lastIndexOfAnyCall(["clip"], lastIndexOfBeginPath - 1);
            this._removeCallsFromLog(CanvasRenderingContext2DResource.PathMethods, index + 1, lastIndexOfBeginPath);
            lastIndexOfBeginPath = this._lastIndexOfAnyCall(["beginPath"], index - 1);
        }

        // Remove all TRASFORMATION MATRIX methods before restore() or setTransform() but after any PATH or corresponding save() method.
        var lastRestore = this._lastIndexOfAnyCall(["restore", "setTransform"]);
        while (lastRestore !== -1) {
            var saveCallIndex = this._lastIndexOfMatchingSaveCall(lastRestore - 1);
            var index = this._lastIndexOfAnyCall(CanvasRenderingContext2DResource.PathMethods, lastRestore - 1);
            index = Math.max(index, saveCallIndex);
            this._removeCallsFromLog(CanvasRenderingContext2DResource.TransformationMatrixMethods, index + 1, lastRestore);
            lastRestore = this._lastIndexOfAnyCall(["restore", "setTransform"], index - 1);
        }

        // Remove all save-restore consecutive pairs.
        var restoreCalls = 0;
        for (var i = this._calls.length - 1; i >= 0; --i) {
            var functionName = this._calls[i].functionName();
            if (functionName === "restore") {
                ++restoreCalls;
                continue;
            }
            if (functionName === "save" && restoreCalls > 0) {
                var saveCallIndex = i;
                for (var j = i - 1; j >= 0 && i - j < restoreCalls; --j) {
                    if (this._calls[j].functionName() === "save")
                        saveCallIndex = j;
                    else
                        break;
                }
                this._calls.splice(saveCallIndex, (i - saveCallIndex + 1) * 2);
                i = saveCallIndex;
            }
            restoreCalls = 0;
        }
    },

    /**
     * @param {!Array.<string>} functionNames
     * @param {number} fromIndex
     * @param {number=} toIndex
     * @return {boolean}
     */
    _removeCallsFromLog: function(functionNames, fromIndex, toIndex)
    {
        var oldLength = this._calls.length;
        if (typeof toIndex !== "number")
            toIndex = oldLength;
        else
            toIndex = Math.min(toIndex, oldLength);
        var newIndex = Math.min(fromIndex, oldLength);
        for (var i = newIndex; i < toIndex; ++i) {
            var call = this._calls[i];
            if (functionNames.indexOf(call.functionName()) === -1)
                this._calls[newIndex++] = call;
        }
        if (newIndex >= toIndex)
            return false;
        this._calls.splice(newIndex, toIndex - newIndex);
        return true;
    },

    /**
     * @return {!Object.<string, string>}
     */
    _currentAttributesState: function()
    {
        var ctx = this.wrappedObject();
        var state = {};
        state.attributes = {};
        CanvasRenderingContext2DResource.AttributeProperties.forEach(function(attribute) {
            state.attributes[attribute] = ctx[attribute];
        });
        if (ctx.getLineDash)
            state.lineDash = ctx.getLineDash();
        return state;
    },

    /**
     * @param {Object.<string, string>=} state
     */
    _applyAttributesState: function(state)
    {
        if (!state)
            return;
        var ctx = this.wrappedObject();
        if (state.attributes) {
            Object.keys(state.attributes).forEach(function(attribute) {
                ctx[attribute] = state.attributes[attribute];
            });
        }
        if (ctx.setLineDash)
            ctx.setLineDash(state.lineDash);
    },

    /**
     * @override
     * @return {!Object.<string, Function>}
     */
    _customWrapFunctions: function()
    {
        var wrapFunctions = CanvasRenderingContext2DResource._wrapFunctions;
        if (!wrapFunctions) {
            wrapFunctions = Object.create(null);

            wrapFunctions["createLinearGradient"] = Resource.WrapFunction.resourceFactoryMethod(LogEverythingResource, "CanvasGradient");
            wrapFunctions["createRadialGradient"] = Resource.WrapFunction.resourceFactoryMethod(LogEverythingResource, "CanvasGradient");
            wrapFunctions["createPattern"] = Resource.WrapFunction.resourceFactoryMethod(LogEverythingResource, "CanvasPattern");

            /**
             * @param {string} methodName
             * @param {function(this:Resource, !Call)=} func
             */
            function stateModifyingWrapFunction(methodName, func)
            {
                if (func) {
                    /** @this Resource.WrapFunction */
                    wrapFunctions[methodName] = function()
                    {
                        func.call(this._resource, this.call());
                    }
                } else {
                    /** @this Resource.WrapFunction */
                    wrapFunctions[methodName] = function()
                    {
                        this._resource.pushCall(this.call());
                    }
                }
            }

            for (var i = 0, methodName; methodName = CanvasRenderingContext2DResource.TransformationMatrixMethods[i]; ++i)
                stateModifyingWrapFunction(methodName, methodName === "setTransform" ? this.pushCall_setTransform : undefined);
            for (var i = 0, methodName; methodName = CanvasRenderingContext2DResource.PathMethods[i]; ++i)
                stateModifyingWrapFunction(methodName, methodName === "beginPath" ? this.pushCall_beginPath : undefined);

            stateModifyingWrapFunction("save", this.pushCall_save);
            stateModifyingWrapFunction("restore", this.pushCall_restore);
            stateModifyingWrapFunction("clip");

            CanvasRenderingContext2DResource._wrapFunctions = wrapFunctions;
        }
        return wrapFunctions;
    },

    __proto__: ContextResource.prototype
}

/**
 * @constructor
 * @param {!Object.<string, boolean>=} drawingMethodNames
 */
function CallFormatter(drawingMethodNames)
{
    this._drawingMethodNames = drawingMethodNames || Object.create(null);
}

CallFormatter.prototype = {
    /**
     * @param {!ReplayableCall} replayableCall
     * @return {!Object}
     */
    formatCall: function(replayableCall)
    {
        var result = {};
        var functionName = replayableCall.functionName();
        if (functionName) {
            result.functionName = functionName;
            result.arguments = replayableCall.args().map(this.formatValue.bind(this));
            if (replayableCall.result() !== undefined)
                result.result = this.formatValue(replayableCall.result());
            if (this._drawingMethodNames[functionName])
                result.isDrawingCall = true;
        } else {
            result.property = replayableCall.args()[0];
            result.value = this.formatValue(replayableCall.args()[1]);
        }
        return result;
    },

    /**
     * @param {*} value
     * @return {!Object}
     */
    formatValue: function(value)
    {
        if (value instanceof ReplayableResource)
            var description = value.description();
        else
            var description = "" + value;
        return { description: description };
    }
}

/**
 * @const
 * @type {!Object.<string, !CallFormatter>}
 */
CallFormatter._formatters = {};

/**
 * @param {string} resourceName
 * @param {!CallFormatter} callFormatter
 */
CallFormatter.register = function(resourceName, callFormatter)
{
    CallFormatter._formatters[resourceName] = callFormatter;
}

/**
 * @param {!ReplayableCall} replayableCall
 * @return {!Object}
 */
CallFormatter.formatCall = function(replayableCall)
{
    var resource = replayableCall.replayableResource();
    var formatter = CallFormatter._formatters[resource.name()];
    if (!formatter) {
        var contextResource = resource.replayableContextResource();
        formatter = CallFormatter._formatters[contextResource.name()] || new CallFormatter();
    }
    return formatter.formatCall(replayableCall);
}

CallFormatter.register("CanvasRenderingContext2D", new CallFormatter(CanvasRenderingContext2DResource.DrawingMethods));
CallFormatter.register("WebGLRenderingContext", new CallFormatter(WebGLRenderingContextResource.DrawingMethods));

/**
 * @constructor
 */
function TraceLog()
{
    /** @type {!Array.<ReplayableCall>} */
    this._replayableCalls = [];
    /** @type {!Cache} */
    this._replayablesCache = new Cache();
    /** @type {!Object.<number, boolean>} */
    this._frameEndCallIndexes = {};
}

TraceLog.prototype = {
    /**
     * @return {number}
     */
    size: function()
    {
        return this._replayableCalls.length;
    },

    /**
     * @return {!Array.<ReplayableCall>}
     */
    replayableCalls: function()
    {
        return this._replayableCalls;
    },

    /**
     * @param {number} id
     * @return {ReplayableResource}
     */
    replayableResource: function(id)
    {
        return /** @type {ReplayableResource} */ (this._replayablesCache.get(id));
    },

    /**
     * @param {!Resource} resource
     */
    captureResource: function(resource)
    {
        resource.toReplayable(this._replayablesCache);
    },

    /**
     * @param {!Call} call
     */
    addCall: function(call)
    {
        this._replayableCalls.push(call.toReplayable(this._replayablesCache));
    },

    addFrameEndMark: function()
    {
        var index = this._replayableCalls.length - 1;
        if (index >= 0)
            this._frameEndCallIndexes[index] = true;
    },

    /**
     * @param {number} index
     * @return {boolean}
     */
    isFrameEndCallAt: function(index)
    {
        return !!this._frameEndCallIndexes[index];
    }
}

/**
 * @constructor
 * @param {!TraceLog} traceLog
 */
function TraceLogPlayer(traceLog)
{
    /** @type {!TraceLog} */
    this._traceLog = traceLog;
    /** @type {number} */
    this._nextReplayStep = 0;
    /** @type {!Cache} */
    this._replayWorldCache = new Cache();
}

TraceLogPlayer.prototype = {
    /**
     * @return {!TraceLog}
     */
    traceLog: function()
    {
        return this._traceLog;
    },

    /**
     * @param {number} id
     * @return {Resource}
     */
    replayWorldResource: function(id)
    {
        return /** @type {Resource} */ (this._replayWorldCache.get(id));
    },

    /**
     * @return {number}
     */
    nextReplayStep: function()
    {
        return this._nextReplayStep;
    },

    reset: function()
    {
        this._nextReplayStep = 0;
        this._replayWorldCache.reset();
    },

    /**
     * @return {Call}
     */
    step: function()
    {
        return this.stepTo(this._nextReplayStep);
    },

    /**
     * @param {number} stepNum
     * @return {Call}
     */
    stepTo: function(stepNum)
    {
        stepNum = Math.min(stepNum, this._traceLog.size() - 1);
        console.assert(stepNum >= 0);
        if (this._nextReplayStep > stepNum)
            this.reset();
        // FIXME: Replay all the cached resources first to warm-up.
        var lastCall = null;
        var replayableCalls = this._traceLog.replayableCalls();
        while (this._nextReplayStep <= stepNum)
            lastCall = replayableCalls[this._nextReplayStep++].replay(this._replayWorldCache);
        return lastCall;
    },

    /**
     * @return {Call}
     */
    replay: function()
    {
        return this.stepTo(this._traceLog.size() - 1);
    }
}

/**
 * @constructor
 */
function ResourceTrackingManager()
{
    this._capturing = false;
    this._stopCapturingOnFrameEnd = false;
    this._lastTraceLog = null;
}

ResourceTrackingManager.prototype = {
    /**
     * @return {boolean}
     */
    capturing: function()
    {
        return this._capturing;
    },

    /**
     * @return {TraceLog}
     */
    lastTraceLog: function()
    {
        return this._lastTraceLog;
    },

    /**
     * @param {!Resource} resource
     */
    registerResource: function(resource)
    {
        resource.setManager(this);
    },

    startCapturing: function()
    {
        if (!this._capturing)
            this._lastTraceLog = new TraceLog();
        this._capturing = true;
        this._stopCapturingOnFrameEnd = false;
    },

    /**
     * @param {TraceLog=} traceLog
     */
    stopCapturing: function(traceLog)
    {
        if (traceLog && this._lastTraceLog !== traceLog)
            return;
        this._capturing = false;
        this._stopCapturingOnFrameEnd = false;
        if (this._lastTraceLog)
            this._lastTraceLog.addFrameEndMark();
    },

    /**
     * @param {!TraceLog} traceLog
     */
    dropTraceLog: function(traceLog)
    {
        this.stopCapturing(traceLog);
        if (this._lastTraceLog === traceLog)
            this._lastTraceLog = null;
    },

    captureFrame: function()
    {
        this._lastTraceLog = new TraceLog();
        this._capturing = true;
        this._stopCapturingOnFrameEnd = true;
    },

    /**
     * @param {!Resource} resource
     * @param {Array|Arguments} args
     */
    captureArguments: function(resource, args)
    {
        if (!this._capturing)
            return;
        this._lastTraceLog.captureResource(resource);
        for (var i = 0, n = args.length; i < n; ++i) {
            var res = Resource.forObject(args[i]);
            if (res)
                this._lastTraceLog.captureResource(res);
        }
    },

    /**
     * @param {!Call} call
     */
    captureCall: function(call)
    {
        if (!this._capturing)
            return;
        this._lastTraceLog.addCall(call);
    },

    markFrameEnd: function()
    {
        if (!this._lastTraceLog)
            return;
        this._lastTraceLog.addFrameEndMark();
        if (this._stopCapturingOnFrameEnd && this._lastTraceLog.size())
            this.stopCapturing(this._lastTraceLog);
    }
}

/**
 * @constructor
 */
var InjectedCanvasModule = function()
{
    /** @type {!ResourceTrackingManager} */
    this._manager = new ResourceTrackingManager();
    /** @type {number} */
    this._lastTraceLogId = 0;
    /** @type {!Object.<string, TraceLog>} */
    this._traceLogs = {};
    /** @type {!Object.<string, TraceLogPlayer>} */
    this._traceLogPlayers = {};
}

InjectedCanvasModule.prototype = {
    /**
     * @param {!WebGLRenderingContext} glContext
     * @return {Object}
     */
    wrapWebGLContext: function(glContext)
    {
        var resource = Resource.forObject(glContext) || new WebGLRenderingContextResource(glContext);
        this._manager.registerResource(resource);
        return resource.proxyObject();
    },

    /**
     * @param {!CanvasRenderingContext2D} context
     * @return {Object}
     */
    wrapCanvas2DContext: function(context)
    {
        var resource = Resource.forObject(context) || new CanvasRenderingContext2DResource(context);
        this._manager.registerResource(resource);
        return resource.proxyObject();
    },

    /**
     * @return {CanvasAgent.TraceLogId}
     */
    captureFrame: function()
    {
        return this._callStartCapturingFunction(this._manager.captureFrame);
    },

    /**
     * @return {CanvasAgent.TraceLogId}
     */
    startCapturing: function()
    {
        return this._callStartCapturingFunction(this._manager.startCapturing);
    },

    markFrameEnd: function()
    {
        this._manager.markFrameEnd();
    },

    /**
     * @param {function(this:ResourceTrackingManager)} func
     * @return {CanvasAgent.TraceLogId}
     */
    _callStartCapturingFunction: function(func)
    {
        var oldTraceLog = this._manager.lastTraceLog();
        func.call(this._manager);
        var traceLog = this._manager.lastTraceLog();
        if (traceLog === oldTraceLog) {
            for (var id in this._traceLogs) {
                if (this._traceLogs[id] === traceLog)
                    return id;
            }
        }
        var id = this._makeTraceLogId();
        this._traceLogs[id] = traceLog;
        return id;
    },

    /**
     * @param {CanvasAgent.TraceLogId} id
     */
    stopCapturing: function(id)
    {
        var traceLog = this._traceLogs[id];
        if (traceLog)
            this._manager.stopCapturing(traceLog);
    },

    /**
     * @param {CanvasAgent.TraceLogId} id
     */
    dropTraceLog: function(id)
    {
        var traceLog = this._traceLogs[id];
        if (traceLog)
            this._manager.dropTraceLog(traceLog);
        delete this._traceLogs[id];
        delete this._traceLogPlayers[id];
    },

    /**
     * @param {CanvasAgent.TraceLogId} id
     * @param {number=} startOffset
     * @param {number=} maxLength
     * @return {!CanvasAgent.TraceLog|string}
     */
    traceLog: function(id, startOffset, maxLength)
    {
        var traceLog = this._traceLogs[id];
        if (!traceLog)
            return "Error: Trace log with the given ID not found.";

        // Ensure last call ends a frame.
        traceLog.addFrameEndMark();

        var replayableCalls = traceLog.replayableCalls();
        if (typeof startOffset !== "number")
            startOffset = 0;
        if (typeof maxLength !== "number")
            maxLength = replayableCalls.length;

        var fromIndex = Math.max(0, startOffset);
        var toIndex = Math.min(replayableCalls.length - 1, fromIndex + maxLength - 1);

        var alive = this._manager.capturing() && this._manager.lastTraceLog() === traceLog;
        var result = {
            id: id,
            /** @type {Array.<CanvasAgent.Call>} */
            calls: [],
            alive: alive,
            startOffset: fromIndex,
            totalAvailableCalls: replayableCalls.length
        };
        for (var i = fromIndex; i <= toIndex; ++i) {
            var call = replayableCalls[i];
            var contextResource = call.replayableResource().replayableContextResource();
            var stackTrace = call.stackTrace();
            var callFrame = stackTrace ? stackTrace.callFrame(0) || {} : {};
            var item = CallFormatter.formatCall(call);
            item.contextId = this._makeStringResourceId(contextResource.id());
            item.sourceURL = callFrame.sourceURL;
            item.lineNumber = callFrame.lineNumber;
            item.columnNumber = callFrame.columnNumber;
            item.isFrameEndCall = traceLog.isFrameEndCallAt(i);
            result.calls.push(item);
        }
        return result;
    },

    /**
     * @param {*} obj
     * @return {!CanvasAgent.CallArgument}
     */
    _makeCallArgument: function(obj)
    {
        if (obj instanceof ReplayableResource)
            var description = obj.description();
        else
            var description = "" + obj;
        return { description: description };
    },

    /**
     * @param {CanvasAgent.TraceLogId} traceLogId
     * @param {number} stepNo
     * @return {!CanvasAgent.ResourceState|string}
     */
    replayTraceLog: function(traceLogId, stepNo)
    {
        var traceLog = this._traceLogs[traceLogId];
        if (!traceLog)
            return "Error: Trace log with the given ID not found.";
        this._traceLogPlayers[traceLogId] = this._traceLogPlayers[traceLogId] || new TraceLogPlayer(traceLog);
        var lastCall = this._traceLogPlayers[traceLogId].stepTo(stepNo);
        var resource = lastCall.resource();
        var dataURL = resource.toDataURL();
        if (!dataURL) {
            resource = resource.contextResource();
            dataURL = resource.toDataURL();
        }
        return this._makeResourceState(this._makeStringResourceId(resource.id()), traceLogId, dataURL);
    },

    /**
     * @param {CanvasAgent.ResourceId} stringResourceId
     * @return {!CanvasAgent.ResourceInfo|string}
     */
    resourceInfo: function(stringResourceId)
    {
        var resourceId = this._parseStringId(stringResourceId).resourceId;
        if (!resourceId)
            return "Error: Wrong resource ID: " + stringResourceId;

        var replayableResource = null;
        for (var id in this._traceLogs) {
            replayableResource = this._traceLogs[id].replayableResource(resourceId);
            if (replayableResource)
                break;
        }
        if (!replayableResource)
            return "Error: Resource with the given ID not found.";

        return this._makeResourceInfo(stringResourceId, replayableResource.description());
    },

    /**
     * @param {CanvasAgent.TraceLogId} traceLogId
     * @param {CanvasAgent.ResourceId} stringResourceId
     * @return {!CanvasAgent.ResourceState|string}
     */
    resourceState: function(traceLogId, stringResourceId)
    {
        var traceLog = this._traceLogs[traceLogId];
        if (!traceLog)
            return "Error: Trace log with the given ID not found.";

        var traceLogPlayer = this._traceLogPlayers[traceLogId];
        if (!traceLogPlayer)
            return "Error: Trace log replay has not started yet.";

        var parsedStringId1 = this._parseStringId(traceLogId);
        var parsedStringId2 = this._parseStringId(stringResourceId);
        if (parsedStringId1.injectedScriptId !== parsedStringId2.injectedScriptId)
            return "Error: Both IDs must point to the same injected script.";

        var resourceId = parsedStringId2.resourceId;
        if (!resourceId)
            return "Error: Wrong resource ID: " + stringResourceId;

        var resource = traceLogPlayer.replayWorldResource(resourceId);
        if (!resource)
            return "Error: Resource with the given ID has not been replayed yet.";

        return this._makeResourceState(stringResourceId, traceLogId, resource.toDataURL());
    },

    /**
     * @return {CanvasAgent.TraceLogId}
     */
    _makeTraceLogId: function()
    {
        return "{\"injectedScriptId\":" + injectedScriptId + ",\"traceLogId\":" + (++this._lastTraceLogId) + "}";
    },

    /**
     * @param {number} resourceId
     * @return {CanvasAgent.ResourceId}
     */
    _makeStringResourceId: function(resourceId)
    {
        return "{\"injectedScriptId\":" + injectedScriptId + ",\"resourceId\":" + resourceId + "}";
    },

    /**
     * @param {CanvasAgent.ResourceId} stringResourceId
     * @param {string} description
     * @return {!CanvasAgent.ResourceInfo}
     */
    _makeResourceInfo: function(stringResourceId, description)
    {
        return {
            id: stringResourceId,
            description: description
        };
    },

    /**
     * @param {CanvasAgent.ResourceId} stringResourceId
     * @param {CanvasAgent.TraceLogId} traceLogId
     * @param {string} imageURL
     * @return {!CanvasAgent.ResourceState}
     */
    _makeResourceState: function(stringResourceId, traceLogId, imageURL)
    {
        return {
            id: stringResourceId,
            traceLogId: traceLogId,
            imageURL: imageURL
        };
    },

    /**
     * @param {string} stringId
     * @return {{injectedScriptId: number, traceLogId: ?number, resourceId: ?number}}
     */
    _parseStringId: function(stringId)
    {
        return InjectedScriptHost.evaluate("(" + stringId + ")");
    }
}

var injectedCanvasModule = new InjectedCanvasModule();
return injectedCanvasModule;

})
