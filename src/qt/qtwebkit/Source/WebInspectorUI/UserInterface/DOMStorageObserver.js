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

WebInspector.DOMStorageObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.DOMStorageObserver.prototype = {
    constructor: WebInspector.DOMStorageObserver,

    // Events defined by the "DOMStorage" domain (see WebCore/inspector/Inspector.json).

    // COMPATIBILITY (iOS 6): This event no longer exists. It is still needed and called on iOS 6.
    addDOMStorage: function(storage)
    {
        WebInspector.storageManager.domStorageWasAdded(storage.id, storage.host, storage.isLocalStorage);
    },

    // COMPATIBILITY (iOS 6): This event was split into the granular events below.
    updateDOMStorage: function(storageId)
    {
        WebInspector.storageManager.domStorageWasUpdated(storageId);
    },

    domStorageItemsCleared: function(storageId)
    {
        // FIXME: Handle this granular event better by only updating what changed. <rdar://problem/13223981>
        this.updateDOMStorage(storageId);
    },

    domStorageItemRemoved: function(storageId, key)
    {
        // FIXME: Handle this granular event better by only updating what changed. <rdar://problem/13223981>
        this.updateDOMStorage(storageId);
    },

    domStorageItemAdded: function(storageId, key, newValue)
    {
        // FIXME: Handle this granular event better by only updating what changed. <rdar://problem/13223981>
        this.updateDOMStorage(storageId);
    },

    domStorageItemUpdated: function(storageId, key, oldValue, newValue)
    {
        // FIXME: Handle this granular event better by only updating what changed. <rdar://problem/13223981>
        this.updateDOMStorage(storageId);
    }
};

WebInspector.DOMStorageObserver.prototype.__proto__ = WebInspector.Object.prototype;
