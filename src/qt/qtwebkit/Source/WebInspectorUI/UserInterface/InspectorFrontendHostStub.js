/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013 Seokju Kwon (seokju.kwon@gmail.com)
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

    WebInspector.InspectorFrontendHostStub.prototype = {

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

        setToolbarHeight: function(width)
        {
        },

        moveWindowBy: function(x, y)
        {
        },

        loaded: function()
        {
        },

        localizedStringsURL: function()
        {
            return undefined;
        },

        inspectedURLChanged: function(title)
        {
            document.title = title;
        },

        copyText: function(text)
        {
            this._textToCopy = text;
            if (!document.execCommand("copy"))
                console.error("Clipboard access is denied");
        },

        openInNewTab: function(url)
        {
            window.open(url, "_blank");
        },

        save: function(url, content, forceSaveAs)
        {
        },

        sendMessageToBackend: function(message)
        {
            if (WebInspector.socket)
                WebInspector.socket.send(message);
        },

        loadResourceSynchronously: function(url)
        {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", url, false);
            xhr.send(null);

            if (xhr.status === 200)
                return xhr.responseText;
            return null;
        }
    }

    InspectorFrontendHost = new WebInspector.InspectorFrontendHostStub();
}

