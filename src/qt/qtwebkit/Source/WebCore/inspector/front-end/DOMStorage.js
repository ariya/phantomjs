/*
 * Copyright (C) 2008 Nokia Inc.  All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
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
 * @param {string} securityOrigin
 * @param {boolean} isLocalStorage
 */
WebInspector.DOMStorage = function(securityOrigin, isLocalStorage)
{
    this._securityOrigin = securityOrigin;
    this._isLocalStorage = isLocalStorage;
}

/**
 * @param {string} securityOrigin
 * @param {boolean} isLocalStorage
 * @return {DOMStorageAgent.StorageId}
 */
WebInspector.DOMStorage.storageId = function(securityOrigin, isLocalStorage)
{
    return { securityOrigin: securityOrigin, isLocalStorage: isLocalStorage };
}

WebInspector.DOMStorage.prototype = {

    /** @return {DOMStorageAgent.StorageId} */
    get id()
    {
        return WebInspector.DOMStorage.storageId(this._securityOrigin, this._isLocalStorage);
    },

    /** @return {string} */
    get securityOrigin()
    {
        return this._securityOrigin;
    },

    /** @return {boolean} */
    get isLocalStorage()
    {
        return this._isLocalStorage;
    },

    /**
     * @param {function(?Protocol.Error, Array.<DOMStorageAgent.Item>):void=} callback
     */
    getItems: function(callback)
    {
        DOMStorageAgent.getDOMStorageItems(this.id, callback);
    },

    /**
     * @param {string} key
     * @param {string} value
     * @param {function(?Protocol.Error):void=} callback
     */
    setItem: function(key, value, callback)
    {
        DOMStorageAgent.setDOMStorageItem(this.id, key, value, callback);
    },

    /**
     * @param {string} key
     * @param {function(?Protocol.Error):void=} callback
     */
    removeItem: function(key, callback)
    {
        DOMStorageAgent.removeDOMStorageItem(this.id, key, callback);
    }
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.DOMStorageModel = function()
{
    this._storages = {};
    InspectorBackend.registerDOMStorageDispatcher(new WebInspector.DOMStorageDispatcher(this));
    DOMStorageAgent.enable();
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginAdded, this._securityOriginAdded, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginRemoved, this._securityOriginRemoved, this);
}

WebInspector.DOMStorageModel.Events = {
    DOMStorageAdded: "DOMStorageAdded",
    DOMStorageRemoved: "DOMStorageRemoved",
    DOMStorageItemsCleared: "DOMStorageItemsCleared",
    DOMStorageItemRemoved: "DOMStorageItemRemoved",
    DOMStorageItemAdded: "DOMStorageItemAdded",
    DOMStorageItemUpdated: "DOMStorageItemUpdated"
}

WebInspector.DOMStorageModel.prototype = {

    /**
     * @param {WebInspector.Event} event
     */
    _securityOriginAdded: function(event)
    {
        var securityOrigin = /** @type {string} */ (event.data);
        var localStorageKey = this._storageKey(securityOrigin, true);
        console.assert(!this._storages[localStorageKey]);
        var localStorage = new WebInspector.DOMStorage(securityOrigin, true);
        this._storages[localStorageKey] = localStorage;
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageAdded, localStorage);

        var sessionStorageKey = this._storageKey(securityOrigin, false);
        console.assert(!this._storages[sessionStorageKey]);
        var sessionStorage = new WebInspector.DOMStorage(securityOrigin, false);
        this._storages[sessionStorageKey] = sessionStorage;
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageAdded, sessionStorage);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _securityOriginRemoved: function(event)
    {
        var securityOrigin = /** @type {string} */ (event.data);
        var localStorageKey = this._storageKey(securityOrigin, true);
        var localStorage = this._storages[localStorageKey];
        console.assert(localStorage);
        delete this._storages[localStorageKey];
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageRemoved, localStorage);

        var sessionStorageKey = this._storageKey(securityOrigin, false);
        var sessionStorage = this._storages[sessionStorageKey];
        console.assert(sessionStorage);
        delete this._storages[sessionStorageKey];
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageRemoved, sessionStorage);
    },

    /**
     * @param {string} securityOrigin
     * @param {boolean} isLocalStorage
     * @return {string}
     */
    _storageKey: function(securityOrigin, isLocalStorage)
    {
        return JSON.stringify(WebInspector.DOMStorage.storageId(securityOrigin, isLocalStorage));
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     */
    _domStorageItemsCleared: function(storageId)
    {
        var domStorage = this.storageForId(storageId);
        var storageData = {
            storage: domStorage
        };
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageItemsCleared, storageData);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     */
    _domStorageItemRemoved: function(storageId, key)
    {
        var domStorage = this.storageForId(storageId);
        var storageData = {
            storage: domStorage,
            key: key
        };
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageItemRemoved, storageData);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     * @param {string} newValue
     */
    _domStorageItemAdded: function(storageId, key, newValue)
    {
        var domStorage = this.storageForId(storageId);
        var storageData = {
            storage: domStorage,
            key: key,
            newValue: newValue
        };
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageItemAdded, storageData);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     * @param {string} oldValue
     * @param {string} newValue
     */
    _domStorageItemUpdated: function(storageId, key, oldValue, newValue)
    {
        var domStorage = this._storages[storageId];
        var storageData = {
            storage: domStorage,
            key: key,
            oldValue: oldValue,
            newValue: newValue
        };
        this.dispatchEventToListeners(WebInspector.DOMStorageModel.Events.DOMStorageItemUpdated, storageData);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @return {WebInspector.DOMStorage}
     */
    storageForId: function(storageId)
    {
        return this._storages[JSON.stringify(storageId)];
    },

    /**
     * @return {Array.<WebInspector.DOMStorage>}
     */
    storages: function()
    {
        var result = [];
        for (var id in this._storages)
            result.push(this._storages[id]);
        return result;
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {DOMStorageAgent.Dispatcher}
 * @param {WebInspector.DOMStorageModel} model
 */
WebInspector.DOMStorageDispatcher = function(model)
{
    this._model = model;
}

WebInspector.DOMStorageDispatcher.prototype = {

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     */
    domStorageItemsCleared: function(storageId)
    {
        this._model._domStorageItemsCleared(storageId);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     */
    domStorageItemRemoved: function(storageId, key)
    {
        this._model._domStorageItemRemoved(storageId, key);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     * @param {string} newValue
     */
    domStorageItemAdded: function(storageId, key, newValue)
    {
        this._model._domStorageItemAdded(storageId, key, newValue);
    },

    /**
     * @param {DOMStorageAgent.StorageId} storageId
     * @param {string} key
     * @param {string} oldValue
     * @param {string} newValue
     */
    domStorageItemUpdated: function(storageId, key, oldValue, newValue)
    {
        this._model._domStorageItemUpdated(storageId, key, oldValue, newValue);
    },
}

/**
 * @type {WebInspector.DOMStorageModel}
 */
WebInspector.domStorageModel = null;
