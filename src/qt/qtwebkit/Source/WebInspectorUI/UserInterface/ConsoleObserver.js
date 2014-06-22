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

WebInspector.ConsoleObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.ConsoleObserver.prototype = {
    constructor: WebInspector.ConsoleObserver,

    // Events defined by the "Console" domain (see WebCore/inspector/Inspector.json).

    messageAdded: function(message)
    {
        if (message.level === "warning" || message.level === "error")
            WebInspector.issueManager.issueWasAdded(message.source, message.level, message.text, message.url, message.line, message.column || 0, message.parameters);

        if (message.url === "[native code]") {
            if (message.type === "profile")
                WebInspector.profileManager.profileWasStartedFromConsole(message.text);
            else if (message.type === "profileEnd")
                WebInspector.profileManager.profileWasEndedFromConsole();
        }

        if (message.source === "console-api" && message.type === "clear")
            return;

        WebInspector.logManager.messageWasAdded(message.source, message.level, message.text, message.type, message.url, message.line, message.column || 0, message.repeatCount, message.parameters, message.stackTrace, message.networkRequestId);
    },

    messageRepeatCountUpdated: function(count)
    {
        WebInspector.logManager.messageRepeatCountUpdated(count);
    },

    messagesCleared: function()
    {
        WebInspector.logManager.messagesCleared();
    }
};

WebInspector.ConsoleObserver.prototype.__proto__ = WebInspector.Object.prototype;
