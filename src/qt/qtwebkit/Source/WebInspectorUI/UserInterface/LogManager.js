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

WebInspector.LogManager = function()
{
    WebInspector.Object.call(this);

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
}

WebInspector.LogManager.Event = {
    SessionStarted: "log-manager-session-was-started",
    Cleared: "log-manager-cleared",
    MessageAdded: "log-manager-message-added",
    ActiveLogCleared: "log-manager-current-log-cleared",
    PreviousMessageRepeatCountUpdated: "log-manager-previous-message-repeat-count-updated"
};

WebInspector.LogManager.prototype = {
    constructor: WebInspector.LogManager,

    // Public

    messageWasAdded: function(source, level, text, type, url, line, column, repeatCount, parameters, stackTrace, requestId)
    {
        // Called from WebInspector.ConsoleObserver.

        // FIXME: Pass a request. We need a way to get it from the request ID.
        var consoleMessage = WebInspector.ConsoleMessage.create(source, level, text, type, url, line, column, repeatCount, parameters, stackTrace, null);

        this.dispatchEventToListeners(WebInspector.LogManager.Event.MessageAdded, {message: consoleMessage});

        console.assert(!consoleMessage._element || !consoleMessage._element.parentNode, "This console message shouldn't be added to a view. To add it you need to use clone().");
    },

    messagesCleared: function()
    {
        // Called from WebInspector.ConsoleObserver.

        // We don't want to clear messages on reloads. We can't determine that easily right now.
        // FIXME: <rdar://problem/13767079> Console.messagesCleared should include a reason
        this._shouldClearMessages = true;
        setTimeout(function() {
            if (this._shouldClearMessages)
                this.dispatchEventToListeners(WebInspector.LogManager.Event.ActiveLogCleared);
            delete this._shouldClearMessages;
        }.bind(this), 0);
    },

    messageRepeatCountUpdated: function(count)
    {
        // Called from WebInspector.ConsoleObserver.

        this.dispatchEventToListeners(WebInspector.LogManager.Event.PreviousMessageRepeatCountUpdated, {count: count});
    },

    requestClearMessages: function()
    {
        ConsoleAgent.clearMessages();
    },

    // Private

    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        var oldMainResource = event.data.oldMainResource;
        var newMainResource = event.target.mainResource;
        if (oldMainResource.url !== newMainResource.url)
            this.dispatchEventToListeners(WebInspector.LogManager.Event.Cleared);
        else
            this.dispatchEventToListeners(WebInspector.LogManager.Event.SessionStarted);

        delete this._shouldClearMessages;
    }
};

WebInspector.LogManager.prototype.__proto__ = WebInspector.Object.prototype;
