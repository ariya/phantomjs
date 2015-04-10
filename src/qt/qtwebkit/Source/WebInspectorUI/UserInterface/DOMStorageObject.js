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

WebInspector.DOMStorageObject = function(id, host, isLocalStorage)
{
    this._id = id;
    this._host = host;
    this._isLocalStorage = isLocalStorage;
};

WebInspector.DOMStorageObject.prototype = {
    constructor: WebInspector.DOMStorageObject,
    
    get id()
    {
        return this._id;
    },

    get host()
    {
        return this._host;
    },

    isLocalStorage: function()
    {
        return this._isLocalStorage;
    },

    getEntries: function(callback)
    {
        // COMPATIBILITY (iOS 6): The getDOMStorageItems function was later renamed to getDOMStorageItems.
        if (DOMStorageAgent.getDOMStorageEntries)
            DOMStorageAgent.getDOMStorageEntries(this._id, callback);
        else
            DOMStorageAgent.getDOMStorageItems(this._id, callback);
    },

    removeItem: function(key, callback)
    {
        DOMStorageAgent.removeDOMStorageItem(this._id, key, callback);
    },

    setItem: function(key, value, callback)
    {
        DOMStorageAgent.setDOMStorageItem(this._id, key, value, callback);
    }
};
