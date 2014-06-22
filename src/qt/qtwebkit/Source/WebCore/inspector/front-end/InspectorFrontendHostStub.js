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

/**
 * @constructor
 * @implements {InspectorFrontendHostAPI}
 */
WebInspector.InspectorFrontendHostStub = function()
{
    this._attachedWindowHeight = 0;
    this.isStub = true;
    this._fileBuffers = {};
    WebInspector.documentCopyEventFired = this.documentCopy.bind(this);
}

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

    requestSetDockSide: function(side)
    {
        InspectorFrontendAPI.setDockSide(side);
    },

    setAttachedWindowHeight: function(height)
    {
    },

    setAttachedWindowWidth: function(width)
    {
    },

    moveWindowBy: function(x, y)
    {
    },

    setInjectedScriptForOrigin: function(origin, script)
    {
    },

    loaded: function()
    {
    },

    localizedStringsURL: function()
    {
        return undefined;
    },

    inspectedURLChanged: function(url)
    {
        document.title = WebInspector.UIString(Preferences.applicationTitle, url);
    },

    documentCopy: function(event)
    {
        if (!this._textToCopy)
            return;
        event.clipboardData.setData("text", this._textToCopy);
        event.preventDefault();
        delete this._textToCopy;
    },

    copyText: function(text)
    {
        this._textToCopy = text;
        if (!document.execCommand("copy")) {
            var screen = new WebInspector.ClipboardAccessDeniedScreen();
            screen.showModal();
        }
    },

    openInNewTab: function(url)
    {
        window.open(url, "_blank");
    },

    canSave: function()
    {
        return true;
    },

    save: function(url, content, forceSaveAs)
    {
        if (this._fileBuffers[url])
            throw new Error("Concurrent file modification denied.");

        this._fileBuffers[url] = [content];
        setTimeout(WebInspector.fileManager.savedURL.bind(WebInspector.fileManager, url), 0);
    },

    append: function(url, content)
    {
        var buffer = this._fileBuffers[url];
        if (!buffer)
            throw new Error("File is not open for write yet.");

        buffer.push(content);
        setTimeout(WebInspector.fileManager.appendedToURL.bind(WebInspector.fileManager, url), 0);
    },

    close: function(url)
    {
        var content = this._fileBuffers[url];
        delete this._fileBuffers[url];

        if (!content)
            return;

        var lastSlashIndex = url.lastIndexOf("/");
        var fileNameSuffix = (lastSlashIndex === -1) ? url : url.substring(lastSlashIndex + 1);

        var blob = new Blob(content, { type: "application/octet-stream" });
        var objectUrl = window.URL.createObjectURL(blob);
        window.location = objectUrl + "#" + fileNameSuffix;

        function cleanup()
        {
            window.URL.revokeObjectURL(objectUrl);
        }
        setTimeout(cleanup, 0);
    },

    sendMessageToBackend: function(message)
    {
    },

    recordActionTaken: function(actionCode)
    {
    },

    recordPanelShown: function(panelCode)
    {
    },

    recordSettingChanged: function(settingCode)
    {
    },

    loadResourceSynchronously: function(url)
    {
        return loadXHR(url);
    },

    supportsFileSystems: function()
    {
        return false;
    },

    requestFileSystems: function()
    {
    },

    addFileSystem: function()
    {
    },

    removeFileSystem: function(fileSystemPath)
    {
    },

    isolatedFileSystem: function(fileSystemId, registeredName)
    {
        return null;
    },

    setZoomFactor: function(zoom)
    {
    },

    isUnderTest: function()
    {
        return false;
    }
}

InspectorFrontendHost = new WebInspector.InspectorFrontendHostStub();
Preferences.localizeUI = false;

// Default implementation; platforms will override.
WebInspector.clipboardAccessDeniedMessage = function()
{
    return "";
}

/**
 * @constructor
 * @extends {WebInspector.HelpScreen}
 */
WebInspector.ClipboardAccessDeniedScreen = function()
{
    WebInspector.HelpScreen.call(this, WebInspector.UIString("Clipboard access is denied"));
    var platformMessage = WebInspector.clipboardAccessDeniedMessage();
    if (platformMessage) {
        var p = this.contentElement.createChild("p");
        p.addStyleClass("help-section");
        p.textContent = platformMessage;
    }
}

WebInspector.ClipboardAccessDeniedScreen.prototype = {
    __proto__: WebInspector.HelpScreen.prototype
}

}

/**
 * @constructor
 * @extends {WebInspector.HelpScreen}
 */
WebInspector.RemoteDebuggingTerminatedScreen = function(reason)
{
    WebInspector.HelpScreen.call(this, WebInspector.UIString("Detached from the target"));
    var p = this.contentElement.createChild("p");
    p.addStyleClass("help-section");
    p.createChild("span").textContent = "Remote debugging has been terminated with reason: ";
    p.createChild("span", "error-message").textContent = reason;
    p.createChild("br");
    p.createChild("span").textContent = "Please re-attach to the new target.";
}

WebInspector.RemoteDebuggingTerminatedScreen.prototype = {
    __proto__: WebInspector.HelpScreen.prototype
}
