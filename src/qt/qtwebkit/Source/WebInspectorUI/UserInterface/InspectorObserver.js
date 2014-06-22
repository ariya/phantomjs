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

WebInspector.InspectorObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.InspectorObserver.prototype = {
    constructor: WebInspector.InspectorObserver,

    // Events defined by the "Inspector" domain (see WebCore/inspector/Inspector.json).

    evaluateForTestInFrontend: function(testCallId, script)
    {
        // FIXME: Not implemented.
    },

    inspect: function(payload, hints)
    {
        var remoteObject = WebInspector.RemoteObject.fromPayload(payload);
        if (remoteObject.subtype === "node") {
            WebInspector.domTreeManager.inspectNodeObject(remoteObject);
            WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.resourceSidebarPanel;
            return;
        }

        if (hints.databaseId)
            WebInspector.storageManager.inspectDatabase(hints.databaseId);
        else if (hints.domStorageId)
            WebInspector.storageManager.inspectDOMStorage(hints.domStorageId);

        WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.resourceSidebarPanel;

        remoteObject.release();
    },

    detached: function(reason)
    {
        // FIXME: Not implemented.
    }
};

WebInspector.InspectorObserver.prototype.__proto__ = WebInspector.Object.prototype;
