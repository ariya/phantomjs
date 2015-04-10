/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
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
 * @param {WebInspector.DatabaseModel} model
 */
WebInspector.Database = function(model, id, domain, name, version)
{
    this._model = model;
    this._id = id;
    this._domain = domain;
    this._name = name;
    this._version = version;
}

WebInspector.Database.prototype = {
    /** @return {string} */
    get id()
    {
        return this._id;
    },

    /** @return {string} */
    get name()
    {
        return this._name;
    },

    set name(x)
    {
        this._name = x;
    },

    /** @return {string} */
    get version()
    {
        return this._version;
    },

    set version(x)
    {
        this._version = x;
    },

    /** @return {string} */
    get domain()
    {
        return this._domain;
    },

    set domain(x)
    {
        this._domain = x;
    },

    /**
     * @param {function(Array.<string>)} callback
     */
    getTableNames: function(callback)
    {
        function sortingCallback(error, names)
        {
            if (!error)
                callback(names.sort());
        }
        DatabaseAgent.getDatabaseTableNames(this._id, sortingCallback);
    },

    /**
     * @param {string} query
     * @param {function(Array.<string>=, Array.<*>=)} onSuccess
     * @param {function(string)} onError
     */
    executeSql: function(query, onSuccess, onError)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {Array.<string>=} columnNames
         * @param {Array.<*>=} values
         * @param {DatabaseAgent.Error=} errorObj
         */
        function callback(error, columnNames, values, errorObj)
        {
            if (error) {
                onError(error);
                return;
            }
            if (errorObj) {
                var message;
                if (errorObj.message)
                    message = errorObj.message;
                else if (errorObj.code == 2)
                    message = WebInspector.UIString("Database no longer has expected version.");
                else
                    message = WebInspector.UIString("An unexpected error %s occurred.", errorObj.code);
                onError(message);
                return;
            }
            onSuccess(columnNames, values);
        }
        DatabaseAgent.executeSQL(this._id, query, callback.bind(this));
    }
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.DatabaseModel = function()
{
    this._databases = [];
    InspectorBackend.registerDatabaseDispatcher(new WebInspector.DatabaseDispatcher(this));
    DatabaseAgent.enable();
}

WebInspector.DatabaseModel.Events = {
    DatabaseAdded: "DatabaseAdded"
}

WebInspector.DatabaseModel.prototype = {
    /**
     * @return {Array.<WebInspector.Database>}
     */
    databases: function()
    {
        var result = [];
        for (var databaseId in this._databases)
            result.push(this._databases[databaseId]);
        return result;
    },

    /**
     * @param {DatabaseAgent.DatabaseId} databaseId
     * @return {WebInspector.Database}
     */
    databaseForId: function(databaseId)
    {
        return this._databases[databaseId];
    },

    /**
     * @param {WebInspector.Database} database
     */
    _addDatabase: function(database)
    {
        this._databases.push(database);
        this.dispatchEventToListeners(WebInspector.DatabaseModel.Events.DatabaseAdded, database);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {DatabaseAgent.Dispatcher}
 * @param {WebInspector.DatabaseModel} model
 */
WebInspector.DatabaseDispatcher = function(model)
{
    this._model = model;
}

WebInspector.DatabaseDispatcher.prototype = {
    /**
     * @param {DatabaseAgent.Database} payload
     */
    addDatabase: function(payload)
    {
        this._model._addDatabase(new WebInspector.Database(
            this._model,
            payload.id,
            payload.domain,
            payload.name,
            payload.version));
    }
}

/**
 * @type {WebInspector.DatabaseModel}
 */
WebInspector.databaseModel = null;
