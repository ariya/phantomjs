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

WebInspector.DatabaseObject = function(id, host, name, version)
{
    this._id = id;
    this._host = host ? host : WebInspector.UIString("Local File");
    this._name = name;
    this._version = version;
};

WebInspector.DatabaseObject.prototype = {
    constructor: WebInspector.DatabaseObject,
    
    get id()
    {
        return this._id;
    },

    get host()
    {
        return this._host;
    },

    get name()
    {
        return this._name;
    },
    
    get version()
    {
        return this._version;
    },
    
    getTableNames: function(callback)
    {
        function sortingCallback(error, names)
        {
            if (!error)
                callback(names.sort());
        }
        
        DatabaseAgent.getDatabaseTableNames(this._id, sortingCallback);
    },

    executeSQL: function(query, successCallback, errorCallback)
    {
        function queryCallback(columnNames, values, sqlError)
        {
            if (sqlError) {
                var message;

                switch (sqlError.code) {
                case SQLException.VERSION_ERR:
                    message = WebInspector.UIString("Database no longer has expected version.");
                    break;
                case SQLException.TOO_LARGE_ERR:
                    message = WebInspector.UIString("Data returned from the database is too large.");
                    break;
                default:
                    message = WebInspector.UIString("An unexpected error occurred.");
                    break;
                }

                errorCallback(message);
                return;
            }

            successCallback(columnNames, values);
        }

        function callback(error, result)
        {
            if (error) {
                errorCallback(WebInspector.UIString("An unexpected error occurred."));
                return;
            }

            // COMPATIBILITY (iOS 6): Newer versions of DatabaseAgent.executeSQL can delay before
            // sending the results. The version on iOS 6 instead returned a transactionId that
            // would be used later in the sqlTransactionSucceeded or sqlTransactionFailed events.
            if ("transactionId" in result) {
                if (!result.success) {
                    errorCallback(WebInspector.UIString("An unexpected error occurred."));
                    return;
                }

                WebInspector.DatabaseObserver._callbacks[result.transactionId] = queryCallback;
                return;
            }

            queryCallback(result.columnNames, result.values, result.sqlError);
        }

        // COMPATIBILITY (iOS 6): Since the parameters of the DatabaseAgent.executeSQL callback differ
        // we need the result object to lookup parameters by name.
        callback.expectsResultObject = true;

        DatabaseAgent.executeSQL(this._id, query, callback);
    }
};
