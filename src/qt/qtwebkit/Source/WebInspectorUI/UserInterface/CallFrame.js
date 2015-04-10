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

WebInspector.CallFrame = function(id, sourceCodeLocation, functionName, thisObject, scopeChain, nativeCode)
{
    WebInspector.Object.call(this);

    console.assert(!sourceCodeLocation || sourceCodeLocation instanceof WebInspector.SourceCodeLocation);
    console.assert(!thisObject || thisObject instanceof WebInspector.RemoteObject);
    console.assert(!scopeChain || scopeChain instanceof Array);

    this._id = id || null;
    this._sourceCodeLocation = sourceCodeLocation || null;
    this._functionName = functionName || null;
    this._thisObject = thisObject || null;
    this._scopeChain = scopeChain || [];
    this._nativeCode = nativeCode || false;
};

WebInspector.CallFrame.prototype = {
    constructor: WebInspector.CallFrame,

    // Public

    get id()
    {
        return this._id;
    },

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    },

    get functionName()
    {
        return this._functionName;
    },

    get nativeCode()
    {
        return this._nativeCode;
    },

    get thisObject()
    {
        return this._thisObject;
    },

    get scopeChain()
    {
        return this._scopeChain;
    },

    collectScopeChainVariableNames: function(callback)
    {
        var result = {this: true};

        var pendingRequests = this._scopeChain.length;

        function propertiesCollected(properties)
        {
            for (var i = 0; properties && i < properties.length; ++i)
                result[properties[i].name] = true;

            if (--pendingRequests)
                return;

            callback(result);
        }

        for (var i = 0; i < this._scopeChain.length; ++i)
            this._scopeChain[i].object.getAllProperties(propertiesCollected);
    }
};

WebInspector.CallFrame.prototype.__proto__ = WebInspector.Object.prototype;
