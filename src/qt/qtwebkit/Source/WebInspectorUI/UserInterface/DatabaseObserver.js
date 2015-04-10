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

WebInspector.DatabaseObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.DatabaseObserver._callbacks = {};

WebInspector.DatabaseObserver.prototype = {
    constructor: WebInspector.DatabaseObserver,

    // Events defined by the "Database" domain (see WebCore/inspector/Inspector.json).

    addDatabase: function(database)
    {
        WebInspector.storageManager.databaseWasAdded(database.id, database.domain, database.name, database.version);
    },

    // COMPATIBILITY (iOS 6): This event was removed in favor of a more async DatabaseAgent.executeSQL.
    sqlTransactionSucceeded: function(transactionId, columnNames, values)
    {
        if (!WebInspector.DatabaseObserver._callbacks[transactionId])
            return;

        var callback = WebInspector.DatabaseObserver._callbacks[transactionId];
        delete WebInspector.DatabaseObserver._callbacks[transactionId];

        if (callback)
            callback(columnNames, values, null);
    },

    // COMPATIBILITY (iOS 6): This event was removed in favor of a more async DatabaseAgent.executeSQL.
    sqlTransactionFailed: function(transactionId, sqlError)
    {
        if (!WebInspector.DatabaseObserver._callbacks[transactionId])
            return;

        var callback = WebInspector.DatabaseObserver._callbacks[transactionId];
        delete WebInspector.DatabaseObserver._callbacks[transactionId];

        if (callback)
            callback(null, null, sqlError);
    }
};

WebInspector.DatabaseObserver.prototype.__proto__ = WebInspector.Object.prototype;
