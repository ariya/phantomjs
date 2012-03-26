/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

if (!window.InspectorFrontendHost) {

WebInspector.InspectorFrontendHostStub = function()
{
    this._attachedWindowHeight = 0;
}

WebInspector._platformFlavor = WebInspector.PlatformFlavor.MacLeopard;

WebInspector.InspectorFrontendHostStub.prototype = {
    platform: function()
    {
        var match = navigator.userAgent.match(/Windows NT/);
        if (match)
            return "windows";
        match = navigator.userAgent.match(/Mac OS X/);
        if (match)
            return "mac";
        return "linux";
    },

    port: function()
    {
        return "unknown";
    },

    bringToFront: function()
    {
        this._windowVisible = true;
    },

    closeWindow: function()
    {
        this._windowVisible = false;
    },

    disconnectFromBackend: function()
    {
        this._windowVisible = false;
    },

    attach: function()
    {
    },

    detach: function()
    {
    },

    search: function(sourceRow, query)
    {
    },

    setAttachedWindowHeight: function(height)
    {
    },

    moveWindowBy: function(x, y)
    {
    },

    setExtensionAPI: function(script)
    {
    },

    loaded: function()
    {
    },

    localizedStringsURL: function()
    {
        return undefined;
    },

    hiddenPanels: function()
    {
        return "";
    },

    inspectedURLChanged: function(url)
    {
    },

    copyText: function()
    {
    },

    saveAs: function(fileName, content)
    {
        var builder = new WebKitBlobBuilder();
        builder.append(content);
        var blob = builder.getBlob("application/octet-stream");
    
        var fr = new FileReader();
        fr.onload = function(e) {
            // Force download
            window.location = this.result;
        }
        fr.readAsDataURL(blob);
    },

    canAttachWindow: function()
    {
        return false;
    },

    sendMessageToBackend: function(message)
    {
    },

    loadSessionSetting: function()
    {
    }
}

InspectorFrontendHost = new WebInspector.InspectorFrontendHostStub();

}
