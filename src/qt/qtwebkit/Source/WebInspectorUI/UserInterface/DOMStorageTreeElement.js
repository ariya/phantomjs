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

WebInspector.DOMStorageTreeElement = function(representedObject)
{
    console.assert(representedObject instanceof WebInspector.DOMStorageObject);

    if (representedObject.isLocalStorage())
        var className = WebInspector.DOMStorageTreeElement.LocalStorageIconStyleClassName;
    else
        var className = WebInspector.DOMStorageTreeElement.SessionStorageIconStyleClassName;

    WebInspector.StorageTreeElement.call(this, className, WebInspector.displayNameForHost(representedObject.host), representedObject);
};

WebInspector.DOMStorageTreeElement.LocalStorageIconStyleClassName = "local-storage-icon";
WebInspector.DOMStorageTreeElement.SessionStorageIconStyleClassName = "session-storage-icon";

WebInspector.DOMStorageTreeElement.prototype = {
    constructor: WebInspector.DOMStorageTreeElement,

    // Public

    get name()
    {
        return WebInspector.displayNameForHost(this.representedObject.host);
    },

    get categoryName()
    {
        if (this.representedObject.isLocalStorage())
            return WebInspector.UIString("Local Storage");
        return WebInspector.UIString("Session Storage");
    }
};

WebInspector.DOMStorageTreeElement.prototype.__proto__ = WebInspector.StorageTreeElement.prototype;
